/*
 * multimaster.c
 *
 * Multimaster based on logical replication
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"
#include "miscadmin.h"

#include "libpq-fe.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "common/username.h"

#include "postmaster/postmaster.h"
#include "postmaster/bgworker.h"
#include "storage/lwlock.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "storage/procarray.h"
#include "access/xlogdefs.h"
#include "access/xact.h"
#include "access/xtm.h"
#include "access/transam.h"
#include "access/subtrans.h"
#include "access/commit_ts.h"
#include "access/xlog.h"
#include "storage/proc.h"
#include "executor/executor.h"
#include "access/twophase.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/timeout.h"
#include "utils/tqual.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "commands/dbcommands.h"
#include "postmaster/autovacuum.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"
#include "replication/walsender.h"
#include "replication/walsender_private.h"
#include "replication/slot.h"
#include "replication/message.h"
#include "port/atomics.h"
#include "tcop/utility.h"
#include "nodes/makefuncs.h"
#include "access/htup_details.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "pglogical_output/hooks.h"
#include "parser/analyze.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "tcop/pquery.h"
#include "lib/ilist.h"

#include "multimaster.h"
#include "ddd.h"

typedef struct { 
    TransactionId xid;    /* local transaction ID   */
	GlobalTransactionId gtid; /* global transaction ID assigned by coordinator of transaction */
	bool  isTwoPhase;     /* user level 2PC */
	bool  isReplicated;   /* transaction on replica */
	bool  isDistributed;  /* transaction performed INSERT/UPDATE/DELETE and has to be replicated to other nodes */
	bool  isPrepared;     /* transaction is perpared at first stage of 2PC */
	bool  isSuspended;    /* prepared transaction is suspended because coordinator node is switch to offline */
    bool  isTransactionBlock; /* is transaction block */
	bool  containsDML;    /* transaction contains DML statements */
	XidStatus status;     /* transaction status */
    csn_t snapshot;       /* transaction snaphsot */
	csn_t csn;            /* CSN */
	pgid_t gid;           /* global transaction identifier (used by 2pc) */
} MtmCurrentTrans;

typedef enum 
{
	MTM_STATE_LOCK_ID
} MtmLockIds;

#define MTM_SHMEM_SIZE (128*1024*1024)
#define MTM_HASH_SIZE  100003
#define MTM_MAP_SIZE   MTM_HASH_SIZE
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000
#define MAX_WAIT_LOOPS   10000 // 1000000 
#define STATUS_POLL_DELAY USECS_PER_SEC

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mtm_start_replication);
PG_FUNCTION_INFO_V1(mtm_stop_replication);
PG_FUNCTION_INFO_V1(mtm_drop_node);
PG_FUNCTION_INFO_V1(mtm_add_node);
PG_FUNCTION_INFO_V1(mtm_poll_node);
PG_FUNCTION_INFO_V1(mtm_recover_node);
PG_FUNCTION_INFO_V1(mtm_get_snapshot);
PG_FUNCTION_INFO_V1(mtm_get_csn);
PG_FUNCTION_INFO_V1(mtm_get_trans_by_gid);
PG_FUNCTION_INFO_V1(mtm_get_trans_by_xid);
PG_FUNCTION_INFO_V1(mtm_get_last_csn);
PG_FUNCTION_INFO_V1(mtm_get_nodes_state);
PG_FUNCTION_INFO_V1(mtm_get_cluster_state);
PG_FUNCTION_INFO_V1(mtm_get_cluster_info);
PG_FUNCTION_INFO_V1(mtm_make_table_local);
PG_FUNCTION_INFO_V1(mtm_dump_lock_graph);
PG_FUNCTION_INFO_V1(mtm_inject_2pc_error);
PG_FUNCTION_INFO_V1(mtm_check_deadlock);

static Snapshot MtmGetSnapshot(Snapshot snapshot);
static void MtmInitialize(void);
static void MtmXactCallback(XactEvent event, void *arg);
static void MtmBeginTransaction(MtmCurrentTrans* x);
static void MtmPrePrepareTransaction(MtmCurrentTrans* x);
static void MtmPostPrepareTransaction(MtmCurrentTrans* x);
static void MtmAbortPreparedTransaction(MtmCurrentTrans* x);
static void MtmPreCommitPreparedTransaction(MtmCurrentTrans* x);
static void MtmEndTransaction(MtmCurrentTrans* x, bool commit);
static bool MtmTwoPhaseCommit(MtmCurrentTrans* x);
static TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static bool MtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static TransactionId MtmAdjustOldestXid(TransactionId xid);
static bool MtmDetectGlobalDeadLock(PGPROC* proc);
static void MtmAddSubtransactions(MtmTransState* ts, TransactionId* subxids, int nSubxids);
static char const* MtmGetName(void);
static size_t MtmGetTransactionStateSize(void);
static void MtmSerializeTransactionState(void* ctx);
static void MtmDeserializeTransactionState(void* ctx);
static void MtmInitializeSequence(int64* start, int64* step);

static void MtmCheckClusterLock(void);
static void MtmCheckSlots(void);
static void MtmAddSubtransactions(MtmTransState* ts, TransactionId *subxids, int nSubxids);

static void MtmShmemStartup(void);

static BgwPool* MtmPoolConstructor(void);
static bool MtmRunUtilityStmt(PGconn* conn, char const* sql, char **errmsg);
static void MtmBroadcastUtilityStmt(char const* sql, bool ignoreError);
static void MtmProcessDDLCommand(char const* queryString, bool transactional);

MtmState* Mtm;

VacuumStmt* MtmVacuumStmt;
IndexStmt*  MtmIndexStmt;
DropStmt*   MtmDropStmt;
MemoryContext MtmApplyContext;

HTAB* MtmXid2State;
HTAB* MtmGid2State;
static HTAB* MtmLocalTables;

static bool MtmIsRecoverySession;
static MtmConnectionInfo* MtmConnections;

static MtmCurrentTrans MtmTx;
static dlist_head MtmLsnMapping = DLIST_STATIC_INIT(MtmLsnMapping);

static TransactionManager MtmTM = 
{ 
	PgTransactionIdGetStatus, 
	PgTransactionIdSetTreeStatus,
	MtmGetSnapshot, 
	PgGetNewTransactionId, 
	MtmGetOldestXmin, 
	PgTransactionIdIsInProgress, 
	PgGetGlobalTransactionId, 
	MtmXidInMVCCSnapshot, 
	MtmDetectGlobalDeadLock, 
	MtmGetName,
	MtmGetTransactionStateSize,
	MtmSerializeTransactionState,
	MtmDeserializeTransactionState,
	MtmInitializeSequence
};

char const* const MtmNodeStatusMnem[] = 
{ 
	"Initialization", 
	"Offline", 
	"Connected",
	"Online",
	"Recovery",
	"InMinor",
	"OutOfService"
};

char const* const MtmTxnStatusMnem[] = 
{ 
	"InProgress", 
	"Committed", 
	"Aborted",
	"Unknown"
};

bool  MtmDoReplication;
char* MtmDatabaseName;
char* MtmDatabaseUser;

int   MtmNodes;
int   MtmNodeId;
int   MtmReplicationNodeId;
int   MtmArbiterPort;
int   MtmConnectTimeout;
int   MtmReconnectTimeout;
int   MtmNodeDisableDelay;
int   MtmTransSpillThreshold;
int   MtmMaxNodes;
int   MtmHeartbeatSendTimeout;
int   MtmHeartbeatRecvTimeout;
int   MtmMin2PCTimeout;
int   MtmMax2PCRatio;
bool  MtmUseDtm;
bool  MtmPreserveCommitOrder;
bool  MtmVolksWagenMode;

TransactionId  MtmUtilityProcessedInXid;

static char* MtmConnStrs;
static char* MtmClusterName;
static int   MtmQueueSize;
static int   MtmWorkers;
static int   MtmVacuumDelay;
static int   MtmMinRecoveryLag;
static int   MtmMaxRecoveryLag;
static int   MtmGcPeriod;
static bool  MtmIgnoreTablesWithoutPk;
static int   MtmLockCount;

static ExecutorStart_hook_type PreviousExecutorStartHook;
static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;

static nodemask_t lastKnownMatrix[MAX_NODES];

static void MtmExecutorStart(QueryDesc *queryDesc, int eflags);
static void MtmExecutorFinish(QueryDesc *queryDesc);
static void MtmProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);

/*
 * -------------------------------------------
 * Synchronize access to MTM structures.
 * Using LWLock seems to be more efficient (at our benchmarks)
 * Multimaster uses trash of 2N+1 lwlocks, where N is number of nodes.
 * locks[0] is used to synchronize access to multimaster state, 
 * locks[1..N] are used to provide exclusive access to replication session for each node
 * locks[N+1..2*N] are used to synchronize access to distributed lock graph at each node
 * -------------------------------------------
 */
void MtmLock(LWLockMode mode)
{
	timestamp_t start, stop;
	if (mode == LW_EXCLUSIVE || MtmLockCount != 0) { 
		if (MtmLockCount++ != 0) { 
			return;
		}
	}
	start = MtmGetSystemTime();
	LWLockAcquire((LWLockId)&Mtm->locks[MTM_STATE_LOCK_ID], mode);
	stop = MtmGetSystemTime();
	if (stop > start + MSEC_TO_USEC(MtmHeartbeatSendTimeout)) { 
		MTM_LOG1("%d: obtaining %s lock takes %ld microseconds", MyProcPid, (mode == LW_EXCLUSIVE ? "exclusive" : "shared"), stop - start);
	}	
	Mtm->lastLockHolder = MyProcPid;
}

void MtmUnlock(void)
{
	if (MtmLockCount != 0 && --MtmLockCount != 0) { 
		return;
	}
	LWLockRelease((LWLockId)&Mtm->locks[MTM_STATE_LOCK_ID]);
	Mtm->lastLockHolder = 0;
}

void MtmLockNode(int nodeId, LWLockMode mode)
{
	Assert(nodeId > 0 && nodeId <= MtmMaxNodes*2);
	LWLockAcquire((LWLockId)&Mtm->locks[nodeId], mode);
}

void MtmUnlockNode(int nodeId)
{
	Assert(nodeId > 0 && nodeId <= MtmMaxNodes*2);
	LWLockRelease((LWLockId)&Mtm->locks[nodeId]);	
}

/*
 * -------------------------------------------
 * System time manipulation functions
 * -------------------------------------------
 */


timestamp_t MtmGetSystemTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (timestamp_t)tv.tv_sec*USECS_PER_SEC + tv.tv_usec;
}

/*
 * Get adjusted system time: taking in account time shift
 */
timestamp_t MtmGetCurrentTime(void)
{
    return MtmGetSystemTime() + Mtm->timeShift;
}

void MtmSleep(timestamp_t interval)
{
    struct timespec ts;
    struct timespec rem;
    ts.tv_sec = interval/USECS_PER_SEC;
    ts.tv_nsec = interval%USECS_PER_SEC*1000;

    while (nanosleep(&ts, &rem) < 0) { 
        Assert(errno == EINTR);
        ts = rem;
    }
}
    
/** 
 * Return ascending unique timestamp which is used as CSN
 */
csn_t MtmAssignCSN()
{
    csn_t csn = MtmGetCurrentTime();
    if (csn <= Mtm->csn) { 
        csn = ++Mtm->csn;
    } else { 
        Mtm->csn = csn;
    }
    return csn;
}

/**
 * "Adjust" system clock if we receive message from future 
 */
csn_t MtmSyncClock(csn_t global_csn)
{
    csn_t local_csn;
    while ((local_csn = MtmAssignCSN()) < global_csn) { 
        Mtm->timeShift += global_csn - local_csn;
    }
    return local_csn;
}

/*
 * Distribute transaction manager functions
 */ 
static char const* MtmGetName(void)
{
	return MULTIMASTER_NAME;
}

static size_t 
MtmGetTransactionStateSize(void)
{
	return sizeof(MtmTx);
}

static void
MtmSerializeTransactionState(void* ctx)
{
	memcpy(ctx, &MtmTx, sizeof(MtmTx));
}

static void
MtmDeserializeTransactionState(void* ctx)
{
	memcpy(&MtmTx, ctx, sizeof(MtmTx));
}


static void
MtmInitializeSequence(int64* start, int64* step)
{
	if (MtmVolksWagenMode)
	{
		*start = 1;
		*step  = 1;
	}
	else
	{
		*start = MtmNodeId;
		*step  = MtmMaxNodes;
	}
}


/*
 * -------------------------------------------
 * Visibility&snapshots
 * -------------------------------------------
 */

csn_t MtmTransactionSnapshot(TransactionId xid)
{
	csn_t snapshot = INVALID_CSN;
	
	MtmLock(LW_SHARED);
	if (Mtm->status == MTM_ONLINE) {
		MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
		if (ts != NULL && !ts->isLocal) { 
			snapshot = ts->snapshot;
			Assert(ts->gtid.node == MtmNodeId || MtmIsRecoverySession); 		
		}
	}
	MtmUnlock();
    return snapshot;
}


Snapshot MtmGetSnapshot(Snapshot snapshot)
{
    snapshot = PgGetSnapshotData(snapshot);
	RecentGlobalDataXmin = RecentGlobalXmin = Mtm->oldestXid;
    return snapshot;
}


TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
    TransactionId xmin = PgGetOldestXmin(NULL, false); /* consider all backends */
	if (TransactionIdIsValid(xmin)) { 
		MtmLock(LW_EXCLUSIVE);
		xmin = MtmAdjustOldestXid(xmin);
		MtmUnlock();
	}
	return xmin;
}

bool MtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot)
{	
#if TRACE_SLEEP_TIME
    static timestamp_t firstReportTime;
    static timestamp_t prevReportTime;
    static timestamp_t totalSleepTime;
    static timestamp_t maxSleepTime;
#endif
	timestamp_t start = MtmGetSystemTime();
    timestamp_t delay = MIN_WAIT_TIMEOUT;
	int i;
    Assert(xid != InvalidTransactionId);
	
	if (!MtmUseDtm) { 
		return PgXidInMVCCSnapshot(xid, snapshot);
	}
	MtmLock(LW_SHARED);

#if TRACE_SLEEP_TIME
    if (firstReportTime == 0) {
        firstReportTime = MtmGetCurrentTime();
    }
#endif
    
	for (i = 0; i < MAX_WAIT_LOOPS; i++)
    {
        MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
        if (ts != NULL /*&& ts->status != TRANSACTION_STATUS_IN_PROGRESS*/)
        {
            if (ts->csn > MtmTx.snapshot) { 
                MTM_LOG4("%d: tuple with xid=%d(csn=%ld) is invisibile in snapshot %ld",
						 MyProcPid, xid, ts->csn, MtmTx.snapshot);
				if (MtmGetSystemTime() - start > USECS_PER_SEC) { 
					elog(WARNING, "Backend %d waits for transaction %s (%lu) status %ld usecs", MyProcPid, ts->gid, (long)xid, MtmGetSystemTime() - start);
				}
				MtmUnlock();
                return true;
            }
            if (ts->status == TRANSACTION_STATUS_UNKNOWN)
            {
                MTM_LOG3("%d: wait for in-doubt transaction %u in snapshot %lu", MyProcPid, xid, MtmTx.snapshot);
                MtmUnlock();
#if TRACE_SLEEP_TIME
                {
                timestamp_t delta, now = MtmGetCurrentTime();
#endif
                MtmSleep(delay);
#if TRACE_SLEEP_TIME
                delta = MtmGetCurrentTime() - now;
                totalSleepTime += delta;
                if (delta > maxSleepTime) {
                    maxSleepTime = delta;
                }
                if (now > prevReportTime + USECS_PER_SEC*10) { 
                    prevReportTime = now;
                    if (firstReportTime == 0) { 
                        firstReportTime = now;
                    } else { 
                        MTM_LOG3("Snapshot sleep %lu of %lu usec (%f%%), maximum=%lu", totalSleepTime, now - firstReportTime, totalSleepTime*100.0/(now - firstReportTime), maxSleepTime);
                    }
                }
                }
#endif
                if (delay*2 <= MAX_WAIT_TIMEOUT) {
                    delay *= 2;
                }
				MtmLock(LW_SHARED);
            }
            else
            {
                bool invisible = ts->status != TRANSACTION_STATUS_COMMITTED;
                MTM_LOG4("%d: tuple with xid=%d(csn= %ld) is %s in snapshot %ld",
						 MyProcPid, xid, ts->csn, invisible ? "rollbacked" : "committed", MtmTx.snapshot);
                MtmUnlock();
				if (MtmGetSystemTime() - start > USECS_PER_SEC) { 
					elog(WARNING, "Backend %d waits for %s transaction %s (%lu) %ld usecs", MyProcPid, invisible ? "rollbacked" : "committed", 
						 ts->gid, (long)xid, MtmGetSystemTime() - start);
				}
                return invisible;
            }
        }
        else
        {
            MTM_LOG4("%d: visibility check is skept for transaction %u in snapshot %lu", MyProcPid, xid, MtmTx.snapshot);
			MtmUnlock();
			return PgXidInMVCCSnapshot(xid, snapshot);
        }
    }
	MtmUnlock();
	elog(ERROR, "Failed to get status of XID %lu in %ld usec", (long)xid, MtmGetSystemTime() - start);
	return true;
}    



/*
 * There can be different oldest XIDs at different cluster node.
 * We collest oldest CSNs from all nodes and choose minimum from them.
 * If no such XID can be located, then return previously observed oldest XID
 */
static TransactionId 
MtmAdjustOldestXid(TransactionId xid)
{
	int i;   
	csn_t oldestSnapshot = INVALID_CSN;
	MtmTransState *prev = NULL;
	MtmTransState *ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
	MTM_LOG2("%d: MtmAdjustOldestXid(%d): snapshot=%ld, csn=%ld, status=%d", MyProcPid, xid, ts != NULL ? ts->snapshot : 0, ts != NULL ? ts->csn : 0, ts != NULL ? ts->status : -1);
	Mtm->gcCount = 0;

	if (ts != NULL) { 
		oldestSnapshot = ts->snapshot;
		Assert(oldestSnapshot != INVALID_CSN);
		if (Mtm->nodes[MtmNodeId-1].oldestSnapshot < oldestSnapshot) { 
			Mtm->nodes[MtmNodeId-1].oldestSnapshot = oldestSnapshot;
		} else {
			oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
		}
		for (i = 0; i < Mtm->nAllNodes; i++) { 
			if (!BIT_CHECK(Mtm->disabledNodeMask, i)
				&& Mtm->nodes[i].oldestSnapshot < oldestSnapshot) 
			{ 
				oldestSnapshot = Mtm->nodes[i].oldestSnapshot;
			}
		}
		if (oldestSnapshot > MtmVacuumDelay*USECS_PER_SEC) { 
			oldestSnapshot -= MtmVacuumDelay*USECS_PER_SEC;
		} else { 
			oldestSnapshot = 0;
		}
		
		for (ts = Mtm->transListHead; 
			 ts != NULL 
				 && (ts->status == TRANSACTION_STATUS_ABORTED || ts->status == TRANSACTION_STATUS_COMMITTED) 
				 && ts->csn < oldestSnapshot
				 && !ts->isPinned
				 && TransactionIdPrecedes(ts->xid, xid);
			 prev = ts, ts = ts->next) 
		{ 
			if (prev != NULL) { 
				/* Remove information about too old transactions */
				hash_search(MtmXid2State, &prev->xid, HASH_REMOVE, NULL);
				hash_search(MtmGid2State, &prev->gid, HASH_REMOVE, NULL);
			}
		}
	} 

	if (MtmUseDtm && !MtmVolksWagenMode) 
	{ 
		if (prev != NULL) { 
			MTM_LOG2("%d: MtmAdjustOldestXid: oldestXid=%d, prev->xid=%d, prev->status=%s, prev->snapshot=%ld, ts->xid=%d, ts->status=%d, ts->snapshot=%ld, oldestSnapshot=%ld", 
					 MyProcPid, xid, prev->xid, MtmTxnStatusMnem[prev->status], prev->snapshot, (ts ? ts->xid : 0), (ts ? ts->status : -1), (ts ? ts->snapshot : -1), oldestSnapshot);
			Mtm->transListHead = prev;
			Mtm->oldestXid = xid = prev->xid;            
		} else if (TransactionIdPrecedes(Mtm->oldestXid, xid)) {  
			xid = Mtm->oldestXid;
		}
	} else { 
		if (prev != NULL) { 
			MTM_LOG2("%d: MtmAdjustOldestXid: oldestXid=%d, prev->xid=%d, prev->status=%s, prev->snapshot=%ld, ts->xid=%d, ts->status=%d, ts->snapshot=%ld, oldestSnapshot=%ld", 
					 MyProcPid, xid, prev->xid, MtmTxnStatusMnem[prev->status], prev->snapshot, (ts ? ts->xid : 0), (ts ? ts->status : -1), (ts ? ts->snapshot : -1), oldestSnapshot);
			Mtm->transListHead = prev;
		}
	}
    return xid;
}

/*
 * -------------------------------------------
 * Transaction list manipulation.
 * All distributed transactions are linked in L1-list ordered by transaction start time.
 * This list is inspected by MtmAdjustOldestXid and transactions which are not used in any snapshot at any node
 * are removed from the list and from the hash.
 * -------------------------------------------
 */

static void MtmTransactionListAppend(MtmTransState* ts)
{
	if (!ts->isEnqueued) { 
		ts->isEnqueued = true;
		ts->next = NULL;
		ts->nSubxids = 0;
		*Mtm->transListTail = ts;
		Mtm->transListTail = &ts->next;
	}
}

static void MtmTransactionListInsertAfter(MtmTransState* after, MtmTransState* ts)
{
    ts->next = after->next;
    after->next = ts;
	ts->isEnqueued = true;
    if (Mtm->transListTail == &after->next) { 
        Mtm->transListTail = &ts->next;
    }
}

static void MtmAddSubtransactions(MtmTransState* ts, TransactionId* subxids, int nSubxids)
{
    int i;
	ts->nSubxids = nSubxids;
    for (i = 0; i < nSubxids; i++) { 
        bool found;
		MtmTransState* sts;
		Assert(TransactionIdIsValid(subxids[i]));
        sts = (MtmTransState*)hash_search(MtmXid2State, &subxids[i], HASH_ENTER, &found);
        Assert(!found);
		sts->isActive = false;
		sts->isPinned = false;
        sts->status = ts->status;
        sts->csn = ts->csn;
		sts->votingCompleted = true;
        MtmTransactionListInsertAfter(ts, sts);
    }
}

void MtmAdjustSubtransactions(MtmTransState* ts)
{
	int i;
	int nSubxids = ts->nSubxids;
	MtmTransState* sts = ts;

    for (i = 0; i < nSubxids; i++) {
		sts = sts->next;
		sts->status = ts->status;
		sts->csn = ts->csn;
	}
}

/*
 * -------------------------------------------
 * Transaction control
 * -------------------------------------------
 */


static void
MtmXactCallback(XactEvent event, void *arg)
{
    switch (event) 
    {
	  case XACT_EVENT_START:
	    MtmBeginTransaction(&MtmTx);
        break;
	  case XACT_EVENT_PRE_PREPARE:
		MtmPrePrepareTransaction(&MtmTx);
		break;
	  case XACT_EVENT_POST_PREPARE:
		MtmPostPrepareTransaction(&MtmTx);
		break;
	  case XACT_EVENT_ABORT_PREPARED:
		MtmAbortPreparedTransaction(&MtmTx);
		break;
	  case XACT_EVENT_PRE_COMMIT_PREPARED:
		MtmPreCommitPreparedTransaction(&MtmTx);
		break;
	  case XACT_EVENT_COMMIT:
		MtmEndTransaction(&MtmTx, true);
		break;
	  case XACT_EVENT_ABORT: 
		MtmEndTransaction(&MtmTx, false);
		break;
	  case XACT_EVENT_COMMIT_COMMAND:
		if (!MtmTx.isTransactionBlock) { 
			MtmTwoPhaseCommit(&MtmTx);
		}
		break;
	  default:
        break;
	}
}

/* 
 * Check if this is "normal" user transaction which should be distributed to other nodes
 */
static bool
MtmIsUserTransaction()
{
	return !IsAutoVacuumLauncherProcess() &&
		IsNormalProcessingMode() &&
		MtmDoReplication &&
		!am_walsender &&
		!IsBackgroundWorker &&
		!IsAutoVacuumWorkerProcess();
}

void 
MtmResetTransaction()
{
	MtmCurrentTrans* x = &MtmTx;
	x->snapshot = INVALID_CSN;
	x->xid = InvalidTransactionId;
	x->gtid.xid = InvalidTransactionId;
	x->isDistributed = false;
	x->isPrepared = false;
	x->isSuspended = false;
	x->isTwoPhase = false;
	x->csn = INVALID_CSN;
	x->status = TRANSACTION_STATUS_UNKNOWN;
	x->gid[0] = '\0';
}


static const char* const isoLevelStr[] = 
{
	"read uncommitted", 
	"read committed", 
	"repeatable read", 
	"serializable"
};

static void 
MtmBeginTransaction(MtmCurrentTrans* x)
{
    if (x->snapshot == INVALID_CSN) { 
		TransactionId xmin = (Mtm->gcCount >= MtmGcPeriod) ? PgGetOldestXmin(NULL, false) : InvalidTransactionId; /* Get oldest xmin outside critical section */

		MtmLock(LW_EXCLUSIVE);	
		if (TransactionIdIsValid(xmin) && Mtm->gcCount >= MtmGcPeriod) {
			MtmAdjustOldestXid(xmin);
		}

		x->xid = GetCurrentTransactionIdIfAny();
        x->isReplicated = MtmIsLogicalReceiver;
        x->isDistributed = MtmIsUserTransaction();
		x->isPrepared = false;
		x->isSuspended = false;
		x->isTwoPhase = false;
		x->isTransactionBlock = IsTransactionBlock();
		/* Application name can be changed usnig PGAPPNAME environment variable */
		if (x->isDistributed && Mtm->status != MTM_ONLINE && strcmp(application_name, MULTIMASTER_ADMIN) != 0) { 
			/* Reject all user's transactions at offline cluster. 
			 * Allow execution of transaction by bg-workers to make it possible to perform recovery.
			 */
			MtmUnlock();			
			elog(ERROR, "Multimaster node is not online: current status %s", MtmNodeStatusMnem[Mtm->status]);
		}
		x->containsDML = false;
        x->snapshot = MtmAssignCSN();	
		x->gtid.xid = InvalidTransactionId;
		x->gid[0] = '\0';
		x->status = TRANSACTION_STATUS_IN_PROGRESS;

		/*
		 * Check if there is global multimaster lock preventing new transaction from commit to make a chance to wal-senders to catch-up.
		 * Only "own" transactions are blocked. Transactions replicated from other nodes (including recovered transaction) should be proceeded
		 * and should not cause cluster status change.
		 */
		if (x->isDistributed && x->isReplicated) { 
			MtmCheckClusterLock();
		}

		MtmUnlock();

        MTM_LOG3("%d: MtmLocalTransaction: %s transaction %u uses local snapshot %lu", 
				 MyProcPid, x->isDistributed ? "distributed" : "local", x->xid, x->snapshot);
    }
}


static MtmTransState* 
MtmCreateTransState(MtmCurrentTrans* x)
{
	bool found;
	MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &x->xid, HASH_ENTER, &found);
	ts->status = TRANSACTION_STATUS_IN_PROGRESS;
	ts->snapshot = x->snapshot;
	ts->isLocal = true;
	ts->isPrepared = false;
	ts->isTwoPhase = x->isTwoPhase;
	ts->isPinned = false;
	ts->votingCompleted = false;
	if (!found) {
		ts->isEnqueued = false;
		ts->isActive = false;
	}
	if (TransactionIdIsValid(x->gtid.xid)) { 		
		Assert(x->gtid.node != MtmNodeId);
		ts->gtid = x->gtid;
	} else { 
		/* I am coordinator of transaction */
		ts->gtid.xid = x->xid;
		ts->gtid.node = MtmNodeId;
	}
	strcpy(ts->gid, x->gid);
	return ts;
}

	
	
/* 
 * Prepare transaction for two-phase commit.
 * This code is executed by PRE_PREPARE hook before PREPARE message is sent to replicas by logical replication
 */
static void
MtmPrePrepareTransaction(MtmCurrentTrans* x)
{ 
	MtmTransState* ts;
	MtmTransMap* tm;
	TransactionId* subxids;
	MTM_TXTRACE(x, "PrePrepareTransaction Start");

	if (!x->isDistributed) {
		return;
	}

	if (Mtm->inject2PCError == 1) { 
		Mtm->inject2PCError = 0;
		elog(ERROR, "ERROR INJECTION for transaction %s (%lu)", x->gid, (long)x->xid);
	}
	x->xid = GetCurrentTransactionId();
	Assert(TransactionIdIsValid(x->xid));

	if (!IsBackgroundWorker && Mtm->status != MTM_ONLINE) { 
		/* Do not take in account bg-workers which are performing recovery */
		elog(ERROR, "Abort current transaction because this cluster node is in %s status", MtmNodeStatusMnem[Mtm->status]);			
	}
	if (TransactionIdIsValid(x->gtid.xid) && BIT_CHECK(Mtm->disabledNodeMask, x->gtid.node-1)) {
		/* Coordinator of transaction is disabled: just abort transaction without any further steps */
		elog(ERROR, "Abort transaction %s (%lu) because it's coordinator %d was disabled", x->gid, (long)x->xid, x->gtid.node);
	}

	MtmLock(LW_EXCLUSIVE);

	ts = MtmCreateTransState(x);
	/* 
	 * Invalid CSN prevent replication of transaction by logical replication 
	 */	   
	ts->isLocal = x->isReplicated || !x->containsDML;
	ts->snapshot = x->snapshot;
	ts->csn = MtmAssignCSN();	
	ts->procno = MyProc->pgprocno;
	ts->votingCompleted = false;
	ts->participantsMask = (((nodemask_t)1 << Mtm->nAllNodes) - 1) & ~Mtm->disabledNodeMask & ~((nodemask_t)1 << (MtmNodeId-1));
	ts->nConfigChanges = Mtm->nConfigChanges;
	ts->votedMask = 0;
	ts->nSubxids = xactGetCommittedChildren(&subxids);
	if (!ts->isActive) {
		ts->isActive = true;
		Mtm->nActiveTransactions += 1;
	}
	x->isPrepared = true;
	x->csn = ts->csn;
	
	Assert(*x->gid != '\0');
	tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_ENTER, NULL);
	tm->state = ts;
	tm->status = TRANSACTION_STATUS_IN_PROGRESS;
	MTM_LOG2("Prepare transaction %s", x->gid);
	
	Mtm->transCount += 1;
	Mtm->gcCount += 1;

	MtmTransactionListAppend(ts);
	MtmAddSubtransactions(ts, subxids, ts->nSubxids);
	MTM_LOG3("%d: MtmPrePrepareTransaction prepare commit of %d (gtid.xid=%d, gtid.node=%d, CSN=%ld)", 
			 MyProcPid, x->xid, ts->gtid.xid, ts->gtid.node, ts->csn);
	MtmUnlock();
	MTM_TXTRACE(x, "PrePrepareTransaction Finish");
}

/*
 * Check heartbeats
 */
bool MtmWatchdog(timestamp_t now)
{
	int i, n = Mtm->nAllNodes;
	bool allAlive = true;
	for (i = 0; i < n; i++) { 
		if (i+1 != MtmNodeId && !BIT_CHECK(Mtm->disabledNodeMask, i)) {
			if (Mtm->nodes[i].lastHeartbeat != 0
				&& now > Mtm->nodes[i].lastHeartbeat + MSEC_TO_USEC(MtmHeartbeatRecvTimeout)) 
			{ 
				elog(WARNING, "Heartbeat is not received from node %d during %d msec", 
					 i+1, (int)USEC_TO_MSEC(now - Mtm->nodes[i].lastHeartbeat));
				MtmOnNodeDisconnect(i+1);				
				allAlive = false;
			}
		}
	}
	return allAlive;
}

/* 
 * Mark transaction as precommitted
 */
void MtmPrecommitTransaction(char const* gid)
{
	MtmLock(LW_EXCLUSIVE);
	{
		MtmTransMap* tm = (MtmTransMap*)hash_search(MtmGid2State, gid, HASH_FIND, NULL);
		if (tm == NULL) {
			MtmUnlock();
			elog(WARNING, "MtmPrecommitTransaction: transaction '%s' is not found", gid);
		} else { 
			MtmTransState* ts = tm->state;
			Assert(ts != NULL);
			if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) {
				ts->status = TRANSACTION_STATUS_UNKNOWN;
				ts->csn = MtmAssignCSN();
				MtmAdjustSubtransactions(ts);
				if (Mtm->status != MTM_RECOVERY) {
					MtmSend2PCMessage(ts, MSG_PRECOMMITTED);
				}
				MtmUnlock();
				Assert(replorigin_session_origin != InvalidRepOriginId);
				if (!IsTransactionState()) {
					MtmResetTransaction();
					StartTransactionCommand();
					SetPreparedTransactionState(ts->gid, MULTIMASTER_PRECOMMITTED);
					CommitTransactionCommand();
				} else { 
					SetPreparedTransactionState(ts->gid, MULTIMASTER_PRECOMMITTED);
				}
			} else {
				elog(WARNING, "MtmPrecommitTransaction: transaction '%s' is already in %s state", gid, MtmTxnStatusMnem[ts->status]);
				MtmUnlock();
			}
		}
	}
}
				
				
	


static bool 
MtmVotingCompleted(MtmTransState* ts)
{
	nodemask_t liveNodesMask = (((nodemask_t)1 << Mtm->nAllNodes) - 1) & ~Mtm->disabledNodeMask & ~((nodemask_t)1 << (MtmNodeId-1));

	if (!ts->isPrepared) { /* We can not just abort precommitted transactions */
		if (ts->nConfigChanges != Mtm->nConfigChanges)
		{ 
			elog(WARNING, "Abort transaction %s (%lu) because cluster configuration is changed from %lx to %lx since transaction start", 
				 ts->gid, (long)ts->xid, ts->participantsMask, liveNodesMask);
			MtmAbortTransaction(ts);
			return true;
		}
		/* If cluster configuration was not changed, then node mask should not changed as well */
		Assert(ts->participantsMask == liveNodesMask);
	}
	
	if (ts->votingCompleted) { 
		return true;
	}
	if (ts->status == TRANSACTION_STATUS_IN_PROGRESS
		&& (ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) /* all live participants voted */
	{
		if (ts->isPrepared) { 			
			ts->csn = MtmAssignCSN();
			ts->votingCompleted = true;
			ts->status = TRANSACTION_STATUS_UNKNOWN;
			return true;
		} else {
			MTM_LOG1("Transaction %s is considered as prepared (status=%s participants=%lx disabled=%lx, voted=%lx)", 
					 ts->gid, MtmTxnStatusMnem[ts->status], ts->participantsMask, Mtm->disabledNodeMask, ts->votedMask);
			ts->isPrepared = true;
			if (ts->isTwoPhase) {
				ts->votingCompleted = true;
				return true;
			} else if (MtmUseDtm) {
				ts->votedMask = 0;
				Assert(replorigin_session_origin == InvalidRepOriginId);
				MtmUnlock();
				SetPreparedTransactionState(ts->gid, MULTIMASTER_PRECOMMITTED);	
				MtmLock(LW_EXCLUSIVE);
				//MtmSend2PCMessage(ts, MSG_PRECOMMIT);
				return false;
			} else {
				ts->status = TRANSACTION_STATUS_UNKNOWN;
				ts->votingCompleted = true;
				return true;
			}
		}
	}
	return Mtm->status != MTM_ONLINE /* node is not online */
		|| ts->status == TRANSACTION_STATUS_ABORTED; /* or transaction was aborted */
}

static void
Mtm2PCVoting(MtmCurrentTrans* x, MtmTransState* ts)
{
	int result = 0;
	timestamp_t prepareTime = ts->csn - ts->snapshot;
	timestamp_t timeout = Max(prepareTime + MSEC_TO_USEC(MtmMin2PCTimeout), prepareTime*MtmMax2PCRatio/100);
	timestamp_t start = MtmGetSystemTime();
	timestamp_t deadline = start + timeout;
	timestamp_t now;

	Assert(ts->csn > ts->snapshot);

	/* Wait votes from all nodes until: */
	while (!MtmVotingCompleted(ts))
	{ 
		MtmUnlock();
		MTM_TXTRACE(x, "PostPrepareTransaction WaitLatch Start");
		result = WaitLatch(&MyProc->procLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, MtmHeartbeatRecvTimeout);
		MTM_TXTRACE(x, "PostPrepareTransaction WaitLatch Finish");
		/* Emergency bailout if postmaster has died */
		if (result & WL_POSTMASTER_DEATH) { 
			proc_exit(1);
		}
		if (result & WL_LATCH_SET) { 
			ResetLatch(&MyProc->procLatch);			
		}
		now = MtmGetSystemTime();
		MtmLock(LW_EXCLUSIVE);
		if (now > deadline) { 
			if (ts->isPrepared) { 
				elog(LOG, "Distributed transaction %s (%lu) is not committed in %ld msec", ts->gid, (long)ts->xid, USEC_TO_MSEC(now - start));
			} else {
				elog(WARNING, "Commit of distributed transaction %s (%lu) is canceled because of %ld msec timeout expiration", 
					 ts->gid, (long)ts->xid, USEC_TO_MSEC(timeout));
				MtmAbortTransaction(ts);
				break;
			}
		}
	}
	if (ts->status != TRANSACTION_STATUS_ABORTED && !ts->votingCompleted) { 
		if (ts->isPrepared) { 
			// GetNewTransactionId(false); /* force increment of transaction counter */
			// elog(ERROR, "Commit of distributed transaction %s is suspended because node is switched to %s mode", ts->gid, MtmNodeStatusMnem[Mtm->status]);
			elog(WARNING, "Commit of distributed transaction %s is suspended because node is switched to %s mode", ts->gid, MtmNodeStatusMnem[Mtm->status]);				
			x->isSuspended = true;
		} else { 
			if (Mtm->status != MTM_ONLINE) { 
				elog(WARNING, "Commit of distributed transaction %s (%lu) is canceled because node is switched to %s mode", 
					 ts->gid, (long)ts->xid, MtmNodeStatusMnem[Mtm->status]);
			} else { 
				elog(WARNING, "Commit of distributed transaction %s (%lu) is canceled because cluster configuration was changed", 
					 ts->gid, (long)ts->xid);
			} 
			MtmAbortTransaction(ts);
		}
	}
	x->status = ts->status;
	MTM_LOG3("%d: Result of vote: %d", MyProcPid, MtmTxnStatusMnem[ts->status]);
}
		
static void
MtmPostPrepareTransaction(MtmCurrentTrans* x)
{ 
	MtmTransState* ts;
	MTM_TXTRACE(x, "PostPrepareTransaction Start");

	if (!x->isDistributed) {
		MTM_TXTRACE(x, "not distributed?");
		return;
	}

	if (Mtm->inject2PCError == 2) { 
		Mtm->inject2PCError = 0;
		elog(ERROR, "ERROR INJECTION for transaction %s (%lu)", x->gid, (long)x->xid);
	}
	MtmLock(LW_EXCLUSIVE);
	ts = (MtmTransState*)hash_search(MtmXid2State, &x->xid, HASH_FIND, NULL);
	Assert(ts != NULL);
	//if (x->gid[0]) MTM_LOG1("Preparing transaction %d (%s) at %ld", x->xid, x->gid, MtmGetCurrentTime());
	if (!MtmIsCoordinator(ts) || Mtm->status == MTM_RECOVERY) {
		MTM_TXTRACE(x, "recovery?");
		MTM_LOG3("Preparing transaction %d (%s) at %ld", x->xid, x->gid, MtmGetCurrentTime());
		Assert(x->gid[0]);
		ts->votingCompleted = true;
		MTM_TXTRACE(x, "recovery? 1");
		if (Mtm->status != MTM_RECOVERY/* || Mtm->recoverySlot != MtmReplicationNodeId*/) {
			MTM_TXTRACE(x, "recovery? 2"); 
			MtmSend2PCMessage(ts, MSG_PREPARED); /* send notification to coordinator */
			if (!MtmUseDtm) { 
				ts->status = TRANSACTION_STATUS_UNKNOWN;
			}
		} else {
			MTM_TXTRACE(x, "recovery? 3");
			ts->status = TRANSACTION_STATUS_UNKNOWN;
		}
		MTM_TXTRACE(x, "recovery? 4");
		MtmUnlock();
		MTM_TXTRACE(x, "recovery? 5");
		MtmResetTransaction();
		MTM_TXTRACE(x, "recovery? 6");
	} else { 
		MTM_TXTRACE(x, "not recovery?");
		Mtm2PCVoting(x, ts);
		MtmUnlock();
		if (x->isTwoPhase) { 
			MtmResetTransaction();
		}
	}
	MTM_TXTRACE(x, "recovery? 7");
	//if (x->gid[0]) MTM_LOG1("Prepared transaction %d (%s) csn=%ld at %ld: %d", x->xid, x->gid, ts->csn, MtmGetCurrentTime(), ts->status);
	if (Mtm->inject2PCError == 3) { 
		Mtm->inject2PCError = 0;
		elog(ERROR, "ERROR INJECTION for transaction %s (%lu)", x->gid, (long)x->xid);
	}

	MTM_TXTRACE(x, "PostPrepareTransaction Finish");
}

static void 
MtmPreCommitPreparedTransaction(MtmCurrentTrans* x)
{
    MtmTransMap* tm;
	MtmTransState* ts;

    if (Mtm->status == MTM_RECOVERY || x->isReplicated || x->isPrepared) { /* Ignore auto-2PC originated by multimaster */ 
        return;
    }
	MtmLock(LW_EXCLUSIVE);
	tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_FIND, NULL);
	if (tm == NULL) {
		elog(WARNING, "Global transaciton ID '%s' is not found", x->gid);
	} else {
 		Assert(tm->state != NULL);
		MTM_LOG3("Commit prepared transaction %d with gid='%s'", x->xid, x->gid);
		ts = tm->state;

		Assert(MtmIsCoordinator(ts));

		ts->votingCompleted = false;
		ts->votedMask = 0;
		ts->procno = MyProc->pgprocno;
		MTM_LOG2("Coordinator of transaction %s sends MSG_PRECOMMIT", ts->gid);
		Assert(replorigin_session_origin == InvalidRepOriginId);
		MtmUnlock();
		SetPreparedTransactionState(ts->gid, MULTIMASTER_PRECOMMITTED);
		//MtmSend2PCMessage(ts, MSG_PRECOMMIT);
		MtmLock(LW_EXCLUSIVE);

		Mtm2PCVoting(x, ts);

		x->xid = ts->xid;
		x->csn = ts->csn;
		x->isPrepared = true;
	}
	MtmUnlock();
}

static void 
MtmAbortPreparedTransaction(MtmCurrentTrans* x)
{
	MtmTransMap* tm;
#if 0
	if (Mtm->status == MTM_RECOVERY) { 
		return;
	}
#endif
	if (x->status != TRANSACTION_STATUS_ABORTED) { 
		MtmLock(LW_EXCLUSIVE);
		//tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_FIND, NULL);
		tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_REMOVE, NULL);
		if (tm == NULL) { 
			elog(WARNING, "Global transaciton ID '%s' is not found", x->gid);
		} else { 
			Assert(tm->state != NULL);
			MTM_LOG1("Abort prepared transaction %s (%lu)", x->gid, (long)x->xid);
			MtmAbortTransaction(tm->state);
		}
		MtmUnlock();
		x->status = TRANSACTION_STATUS_ABORTED;
	} else { 
		MTM_LOG1("Transaction %s (%lu) is already aborted", x->gid, (long)x->xid);
	}
}

static void 
MtmLogAbortLogicalMessage(int nodeId, char const* gid)
{
	MtmAbortLogicalMessage msg;
	XLogRecPtr lsn;
	strcpy(msg.gid, gid);
	msg.origin_node = nodeId;
	msg.origin_lsn = replorigin_session_origin_lsn;
	lsn = LogLogicalMessage("A", (char*)&msg, sizeof msg, false); 
	XLogFlush(lsn);
	MTM_LOG1("MtmLogAbortLogicalMessage node=%d transaction=%s lsn=%lx", nodeId, gid, lsn);
}

static void 
MtmEndTransaction(MtmCurrentTrans* x, bool commit)
{
	MTM_LOG2("%d: End transaction %d, prepared=%d, replicated=%d, distributed=%d, 2pc=%d, gid=%s -> %s", 
			 MyProcPid, x->xid, x->isPrepared, x->isReplicated, x->isDistributed, x->isTwoPhase, x->gid, commit ? "commit" : "abort");
	if (x->status != TRANSACTION_STATUS_ABORTED && x->isDistributed && (x->isPrepared || x->isReplicated) && !x->isTwoPhase) {
		MtmTransState* ts = NULL;
		MtmLock(LW_EXCLUSIVE);
		if (x->isPrepared) { 
			ts = (MtmTransState*)hash_search(MtmXid2State, &x->xid, HASH_FIND, NULL);
			Assert(ts != NULL);
			Assert(strcmp(x->gid, ts->gid) == 0);
		} else if (x->gid[0]) { 
			MtmTransMap* tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_FIND, NULL);
			if (tm != NULL) {
				ts = tm->state;
			} else { 
				MTM_LOG1("%d: GID %s not found", MyProcPid, x->gid);
			}
		}
		if (ts != NULL) { 
			if (*ts->gid)  
				MTM_LOG2("TRANSLOG: %s transaction gid=%s xid=%d node=%d dxid=%d status %s", 
						 (commit ? "commit" : "rollback"), ts->gid, ts->xid, ts->gtid.node, ts->gtid.xid, MtmTxnStatusMnem[ts->status]);
			if (commit) {
				if (!(ts->status == TRANSACTION_STATUS_UNKNOWN 
					  || (ts->status == TRANSACTION_STATUS_IN_PROGRESS && Mtm->status == MTM_RECOVERY)))  
				{
					elog(ERROR, "Attempt to commit %s transaction %s (%lu)", 
						 MtmTxnStatusMnem[ts->status], ts->gid, (long)ts->xid);
				}
				if (x->csn > ts->csn || Mtm->status == MTM_RECOVERY) {
					Assert(x->csn != INVALID_CSN);
					ts->csn = x->csn;
					MtmSyncClock(ts->csn);
				}
				Mtm->lastCsn = ts->csn;
				ts->status = TRANSACTION_STATUS_COMMITTED;
				Assert(ts->isActive);
				ts->isActive = false;
				Assert(Mtm->nActiveTransactions != 0);
				Mtm->nActiveTransactions -= 1;
				MtmAdjustSubtransactions(ts);
			} else { 
				MTM_LOG1("%d: abort transaction %s (%lu) is called from MtmEndTransaction", MyProcPid, x->gid, (long)x->xid);
				MtmAbortTransaction(ts);
			}
		}
		if (!commit && x->isReplicated && TransactionIdIsValid(x->gtid.xid)) { 
			Assert(Mtm->status != MTM_RECOVERY || Mtm->recoverySlot != MtmNodeId);
			/* 
			 * Send notification only if ABORT happens during transaction processing at replicas, 
			 * do not send notification if ABORT is received from master 
			 */
			MTM_LOG1("%d: send ABORT notification for transaction %s (%lu) local xid=%lu to coordinator %d", 
					 MyProcPid, x->gid, (long)x->gtid.xid, (long)x->xid, x->gtid.node);
			if (ts == NULL) { 
				bool found;
				Assert(TransactionIdIsValid(x->xid));
				ts = (MtmTransState*)hash_search(MtmXid2State, &x->xid, HASH_ENTER, &found);
				if (!found) { 
					ts->isEnqueued = false;
					ts->isActive = false;
				}
				ts->status = TRANSACTION_STATUS_ABORTED;
				ts->isLocal = true;
				ts->isPrepared = false;
				ts->isPinned = false;
				ts->snapshot = x->snapshot;
				ts->isTwoPhase = x->isTwoPhase;
				ts->csn = MtmAssignCSN();	
				ts->gtid = x->gtid;
				ts->nSubxids = 0;
				ts->votingCompleted = true;
				strcpy(ts->gid, x->gid);
				if (ts->isActive) { 
					ts->isActive = false;
					Assert(Mtm->nActiveTransactions != 0);
					Mtm->nActiveTransactions -= 1;
				}
				MtmTransactionListAppend(ts);
				if (*x->gid) { 
					replorigin_session_origin_lsn = InvalidXLogRecPtr;
					MTM_TXTRACE(x, "MtmEndTransaction/MtmLogAbortLogicalMessage");
					MtmLogAbortLogicalMessage(MtmNodeId, x->gid);
				}
			}
			MtmSend2PCMessage(ts, MSG_ABORTED); /* send notification to coordinator */
		} else if (x->status == TRANSACTION_STATUS_ABORTED && x->isReplicated && !x->isPrepared) {
			hash_search(MtmXid2State, &x->xid, HASH_REMOVE, NULL);
		}
		MtmUnlock();
	}
	MtmResetTransaction();
	if (!MyReplicationSlot) { 
		MtmCheckSlots();
	}
}

/* 
 * Send arbiter's message
 */
void MtmSendMessage(MtmArbiterMessage* msg) 
{
	SpinLockAcquire(&Mtm->queueSpinlock);
	{
		MtmMessageQueue* mq = Mtm->freeQueue;
		MtmMessageQueue* sendQueue = Mtm->sendQueue;
		if (mq == NULL) {
			mq = (MtmMessageQueue*)ShmemAlloc(sizeof(MtmMessageQueue));
		} else { 
			Mtm->freeQueue = mq->next;
		}
		mq->msg = *msg;
		mq->next = sendQueue;
		Mtm->sendQueue = mq;
		if (sendQueue == NULL) { 
			/* singal semaphore only once for the whole list */
			PGSemaphoreUnlock(&Mtm->sendSemaphore);
		}
	}
	SpinLockRelease(&Mtm->queueSpinlock);
}

/*
 * Send arbiter's 2PC message. Right now only responses to coordinates are 
 * sent through arbiter. Brodcasts from coordinator to noes are done 
 * using logical decoding.
 */
void MtmSend2PCMessage(MtmTransState* ts, MtmMessageCode cmd)
{
	MtmArbiterMessage msg;
	msg.code = cmd;
	msg.sxid = ts->xid;
	msg.csn  = ts->csn;
	msg.disabledNodeMask = Mtm->disabledNodeMask;
	msg.connectivityMask = SELF_CONNECTIVITY_MASK;
	msg.oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
	memcpy(msg.gid, ts->gid, MULTIMASTER_MAX_GID_SIZE);

	if (MtmIsCoordinator(ts)) {
		int i;
		Assert(false); // All broadcasts are now done through logical decoding
		for (i = 0; i < Mtm->nAllNodes; i++)
		{
			if (BIT_CHECK(ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask, i))
			{
				Assert(TransactionIdIsValid(ts->xids[i]));
				msg.node = i+1;
				msg.dxid = ts->xids[i];
				MtmSendMessage(&msg);
			}
		}
	} else if (!BIT_CHECK(Mtm->disabledNodeMask, ts->gtid.node-1)) {
		MTM_LOG2("Send %s message to node %d xid=%d gid=%s", MtmMessageKindMnem[cmd], ts->gtid.node, ts->gtid.xid, ts->gid);
		msg.node = ts->gtid.node;
		msg.dxid = ts->gtid.xid;
		MtmSendMessage(&msg);
	}
}

/* 
 * Broadcase poll state message to all nodes. 
 * This function is used to gather information about state of prepared transaction
 * at node startup or after crash of some node.
 */
static void MtmBroadcastPollMessage(MtmTransState* ts)
{
	int i;
	MtmArbiterMessage msg;
	msg.code = MSG_POLL_REQUEST;
	msg.disabledNodeMask = Mtm->disabledNodeMask;
	msg.connectivityMask = SELF_CONNECTIVITY_MASK;
	msg.oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
	memcpy(msg.gid, ts->gid, MULTIMASTER_MAX_GID_SIZE);
	ts->votedMask = 0;

	for (i = 0; i < Mtm->nAllNodes; i++)
	{
		if (BIT_CHECK(ts->participantsMask & ~Mtm->disabledNodeMask, i))
		{
			msg.node = i+1;
			MTM_LOG3("Send request for transaction %s to node %d", msg.gid, msg.node);
			MtmSendMessage(&msg);
		}
	}
}

/*
 * Restore state of recovered prepared transaction in memory.
 * This function is called at system startup to make it possible to 
 * handle this prepared transactions in normal way.
 */
static void	MtmLoadPreparedTransactions(void)
{
	PreparedTransaction pxacts;
	int n = GetPreparedTransactions(&pxacts);
	int i;

	for (i = 0; i < n; i++) { 
		bool found;
		char const* gid = pxacts[i].gid;
		MtmTransMap* tm = (MtmTransMap*)hash_search(MtmGid2State, gid, HASH_ENTER, &found);
		if (!found || tm->state == NULL) {
			TransactionId xid = GetNewTransactionId(false);
			MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_ENTER, &found);
			MTM_LOG1("Recover prepared transaction %s (%lu) state=%s", gid, (long)xid, pxacts[i].state_3pc);
			MyPgXact->xid = InvalidTransactionId; /* dirty hack:((( */
			Assert(!found);
			Mtm->nActiveTransactions += 1;
			ts->isEnqueued = false;
			ts->isActive = true;
			ts->status = strcmp(pxacts[i].state_3pc, MULTIMASTER_PRECOMMITTED) == 0 ? TRANSACTION_STATUS_UNKNOWN : TRANSACTION_STATUS_IN_PROGRESS;
			ts->isLocal = true;
			ts->isPrepared = true;
			ts->isPinned = false;
			ts->snapshot = INVALID_CSN;
			ts->isTwoPhase = false;
			ts->csn = 0; /* should be replaced with real CSN by poll result */
			ts->gtid.node = MtmNodeId;
			ts->gtid.xid = xid;
			ts->nSubxids = 0;
			ts->votingCompleted = true;
			ts->participantsMask = (((nodemask_t)1 << Mtm->nAllNodes) - 1) & ~Mtm->disabledNodeMask & ~((nodemask_t)1 << (MtmNodeId-1));
			ts->nConfigChanges = Mtm->nConfigChanges;
			ts->votedMask = 0;
			strcpy(ts->gid, gid);
			MtmTransactionListAppend(ts);			
			tm->status = ts->status;
			tm->state = ts;
			MtmBroadcastPollMessage(ts);
		}
	}
	MTM_LOG1("Recover %d prepared transactions", n);
	if (pxacts) { 
		pfree(pxacts);
	}
}

static void MtmStartRecovery()
{
	MtmLock(LW_EXCLUSIVE);
	BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
	Mtm->nConfigChanges += 1;
	MtmSwitchClusterMode(MTM_RECOVERY);
	Mtm->recoveredLSN = InvalidXLogRecPtr;
	MtmUnlock();
}


/*
 * Prepare context for applying transaction at replica
 */
void MtmJoinTransaction(GlobalTransactionId* gtid, csn_t globalSnapshot)
{
	MtmTx.gtid = *gtid;
	MtmTx.xid = GetCurrentTransactionId();
	MtmTx.isReplicated = true;
	MtmTx.isDistributed = true;
	MtmTx.containsDML = true;

	if (globalSnapshot != INVALID_CSN) {
		MtmLock(LW_EXCLUSIVE);
		MtmSyncClock(globalSnapshot);	
		MtmTx.snapshot = globalSnapshot;	
		if (Mtm->status != MTM_RECOVERY) { 
			MtmTransState* ts = MtmCreateTransState(&MtmTx); /* we need local->remote xid mapping for deadlock detection */
			if (!ts->isActive) { 
				ts->isActive = true;
				Mtm->nActiveTransactions += 1;
			}
		}
		MtmUnlock();
	} else { 
		globalSnapshot = MtmTx.snapshot;
	}
	if (!TransactionIdIsValid(gtid->xid)) { 
		/* In case of recovery InvalidTransactionId is passed */
		if (Mtm->status != MTM_RECOVERY) { 
			elog(WARNING, "Node %d tries to recover node %d which is in %s mode", gtid->node, MtmNodeId,  MtmNodeStatusMnem[Mtm->status]);
			MtmStartRecovery();
		}
	} else if (Mtm->status == MTM_RECOVERY) { 
		/* When recovery is completed we get normal transaction ID and switch to normal mode */
		MtmRecoveryCompleted();
	}
}

void  MtmSetCurrentTransactionGID(char const* gid)
{
	MTM_LOG3("Set current transaction xid=%d GID %s", MtmTx.xid, gid);
	strcpy(MtmTx.gid, gid);
	MtmTx.isDistributed = true;
	MtmTx.isReplicated = true;
}

TransactionId MtmGetCurrentTransactionId(void)
{
	return MtmTx.xid;
}

XidStatus MtmGetCurrentTransactionStatus(void)
{
	return MtmTx.status;
}

/* 
 * Perform atomic exchange of global transaction status.
 * The problem is that because of concurrent applying transactions at replica by multiple
 * threads we can proceed ABORT request before PREPARE - when transaction is not yet 
 * applied at this node and there is MtmTransState associated with this transactions.
 * We remember information about status of this transaction in MtmTransMap.
 */
XidStatus MtmExchangeGlobalTransactionStatus(char const* gid, XidStatus new_status)
{
	MtmTransMap* tm;
	bool found;
	XidStatus old_status = TRANSACTION_STATUS_IN_PROGRESS;

	Assert(gid[0]);
	MtmLock(LW_EXCLUSIVE);
	tm = (MtmTransMap*)hash_search(MtmGid2State, gid, HASH_ENTER, &found);
	if (found) {
		old_status = tm->status;
		if (old_status != TRANSACTION_STATUS_ABORTED) { 
			tm->status = new_status;
		}
		if (tm->state != NULL && old_status == TRANSACTION_STATUS_IN_PROGRESS) { 
			/* Return UNKNOWN to mark that transaction was prepared */
			if (new_status != TRANSACTION_STATUS_UNKNOWN) { 
				MTM_LOG1("Change status of in-progress transaction %s to %s", gid, MtmTxnStatusMnem[new_status]);
			}
			old_status = TRANSACTION_STATUS_UNKNOWN;
		}
	} else { 
		MTM_LOG2("Set status of unknown transaction %s to %s", gid, MtmTxnStatusMnem[new_status]);
		tm->state = NULL;
		tm->status = new_status;
	}
	MtmUnlock();
	return old_status;
}

void  MtmSetCurrentTransactionCSN(csn_t csn)
{
	MTM_LOG3("Set current transaction CSN %ld", csn);
	MtmTx.csn = csn;
	MtmTx.isDistributed = true;
	MtmTx.isReplicated = true;
}


csn_t MtmGetTransactionCSN(TransactionId xid)
{
	MtmTransState* ts;
	csn_t csn;
	MtmLock(LW_SHARED);
	ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
	Assert(ts != NULL);
	csn = ts->csn;
	MtmUnlock();
	return csn;
}
	
/* 
 * Wakeup coordinator's backend when voting is completed
 */
void MtmWakeUpBackend(MtmTransState* ts)
{
	if (!ts->votingCompleted) {
		MTM_TXTRACE(ts, "MtmWakeUpBackend");
		MTM_LOG3("Wakeup backed procno=%d, pid=%d", ts->procno, ProcGlobal->allProcs[ts->procno].pid);
		ts->votingCompleted = true;
		SetLatch(&ProcGlobal->allProcs[ts->procno].procLatch); 
	}
}


/* 
 * Abort the transaction if it is not yet aborted
 */
void MtmAbortTransaction(MtmTransState* ts)
{	
	Assert(MtmLockCount != 0); /* should be invoked with exclsuive lock */
	if (ts->status != TRANSACTION_STATUS_ABORTED) { 
		if (ts->status == TRANSACTION_STATUS_COMMITTED) { 
			elog(LOG, "Attempt to rollback already committed transaction %s (%lu)", ts->gid, (long)ts->xid);
		} else { 
			MTM_LOG1("Rollback active transaction %s (%lu) %d:%lu status %s", ts->gid, (long)ts->xid, ts->gtid.node, (long)ts->gtid.xid, MtmTxnStatusMnem[ts->status]);
			ts->status = TRANSACTION_STATUS_ABORTED;
			MtmAdjustSubtransactions(ts);
			if (ts->isActive) {
				ts->isActive = false;
				Assert(Mtm->nActiveTransactions != 0);
				Mtm->nActiveTransactions -= 1;
			}
		}
	}
}

/*
 * -------------------------------------------
 * HA functions
 * -------------------------------------------
 */

/* 
 * Handle critical errors while applying transaction at replica.
 * Such errors should cause shutdown of this cluster node to allow other nodes to continue serving client requests.
 * Other error will be just reported and ignored
 */
void MtmHandleApplyError(void)
{
	ErrorData *edata = CopyErrorData();
	switch (edata->sqlerrcode) { 
		case ERRCODE_DISK_FULL:
		case ERRCODE_INSUFFICIENT_RESOURCES:
		case ERRCODE_IO_ERROR:
		case ERRCODE_DATA_CORRUPTED:
		case ERRCODE_INDEX_CORRUPTED:
		  /* Should we really treate this errors as fatal? 
		case ERRCODE_SYSTEM_ERROR:
		case ERRCODE_INTERNAL_ERROR:
		case ERRCODE_OUT_OF_MEMORY:
		  */
			elog(WARNING, "Node is excluded from cluster because of non-recoverable error %d, %s, pid=%u",
				edata->sqlerrcode, edata->message, getpid());
			MtmSwitchClusterMode(MTM_OUT_OF_SERVICE);
			kill(PostmasterPid, SIGQUIT);
			break;
	}
	FreeErrorData(edata);
}

/**
 * Check status of all prepared transactions with coordinator at disabled node.
 * Actually, if node is precommitted (state == UNKNOWN) at any of nodes, then is is prepared at all nodes and so can be committed. 
 * But if coordinator of transaction is crashed, we made a decision about transaction commit only if transaction is precommitted at ALL live nodes.
 * The reason is that we want to avoid extra polling to obtain maximum CSN from all nodes to assign it to committed transaction.
 * Called only from MtmDisableNode in critical section.
 */
static void MtmPollStatusOfPreparedTransactionsForDisabledNode(int disabledNodeId)
{
	MtmTransState *ts;
	for (ts = Mtm->transListHead; ts != NULL; ts = ts->next) { 
		if (TransactionIdIsValid(ts->gtid.xid) 
			&& ts->gtid.node == disabledNodeId 
			&& ts->votingCompleted
			&& (ts->status == TRANSACTION_STATUS_UNKNOWN || ts->status == TRANSACTION_STATUS_IN_PROGRESS)) 
		{
			Assert(ts->gid[0]);
			if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) { 
				elog(LOG, "Abort transaction %s because its coordinator is disabled and it is not prepared at node %d", ts->gid, MtmNodeId);
				MtmFinishPreparedTransaction(ts, false);
			} else {
				MTM_LOG1("Poll state of transaction %s (%lu)", ts->gid, (long)ts->xid);				
				MtmBroadcastPollMessage(ts);
			}
		} else {
			MTM_LOG1("Skip transaction %s (%lu) with status %s gtid.node=%d gtid.xid=%lu votedMask=%lx", 
					 ts->gid, (long)ts->xid, MtmTxnStatusMnem[ts->status], ts->gtid.node, (long)ts->gtid.xid, ts->votedMask);
		}
	}
}

/*
 * Poll status of all active prepared transaction.
 * This function is called before start of recovery to prevent blocking of recovery process by some 
 * prepared transaction which is not recovered
 */
static void MtmPollStatusOfPreparedTransactions()
{
	MtmTransState *ts;
	for (ts = Mtm->transListHead; ts != NULL; ts = ts->next) { 
		if (TransactionIdIsValid(ts->gtid.xid) 
			&& ts->votingCompleted /* If voting is not yet completed, then there is some backend coordinating this transaction */
			&& (ts->status == TRANSACTION_STATUS_UNKNOWN || ts->status == TRANSACTION_STATUS_IN_PROGRESS)) 
		{
			Assert(ts->gid[0]);
			MTM_LOG1("Poll state of transaction %s (%lu) from node %d", ts->gid, (long)ts->xid, ts->gtid.node);				
			MtmBroadcastPollMessage(ts);
		} else {
			MTM_LOG2("Skip prepared transaction %s (%d) with status %s gtid.node=%d gtid.xid=%lu votedMask=%lx", 
					 ts->gid, (long)ts->xid, MtmTxnStatusMnem[ts->status], ts->gtid.node, (long)ts->gtid.xid, ts->votedMask);
		}
	}
}

/*
 * Node is disabled if it is not part of clique built using connectivity masks of all nodes.
 * There is no warranty that all noeds will make the same decision about clique, btu as far as we want to avoid
 * some global coordinator (which will be SPOF), we have to rely on Bron–Kerbosch algorithm locating maximum clique in graph
 */
static void MtmDisableNode(int nodeId)
{
	timestamp_t now = MtmGetSystemTime();
	elog(WARNING, "Disable node %d at xlog position %lx, last status change time %d msec ago", nodeId, GetXLogInsertRecPtr(), 
		 (int)USEC_TO_MSEC(now - Mtm->nodes[nodeId-1].lastStatusChangeTime));
	BIT_SET(Mtm->disabledNodeMask, nodeId-1);
	Mtm->nConfigChanges += 1;
	Mtm->nodes[nodeId-1].timeline += 1;
	Mtm->nodes[nodeId-1].lastStatusChangeTime = now;
	Mtm->nodes[nodeId-1].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
	if (nodeId != MtmNodeId) { 
		Mtm->nLiveNodes -= 1;		
	}
	if (Mtm->nLiveNodes >= Mtm->nAllNodes/2+1) {
		/* Make decision about prepared transaction status only in quorum */
		MtmPollStatusOfPreparedTransactionsForDisabledNode(nodeId);
	}
} 

/* 
 * Node is anabled when it's recovery is completed.
 * This why node is mostly marked as recovered when logical sender/receiver to this node is (re)started.
 */
static void MtmEnableNode(int nodeId)
{ 
	BIT_CLEAR(Mtm->disabledNodeMask, nodeId-1);
	BIT_CLEAR(Mtm->reconnectMask, nodeId-1);
	Mtm->nConfigChanges += 1;
	Mtm->nodes[nodeId-1].lastStatusChangeTime = MtmGetSystemTime();
	Mtm->nodes[nodeId-1].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
	if (nodeId != MtmNodeId) { 
		Mtm->nLiveNodes += 1;			
	}
	elog(WARNING, "Enable node %d at xlog position %lx", nodeId, GetXLogInsertRecPtr());
}

/* 
 * Function call when recovery of node is completed
 */
void MtmRecoveryCompleted(void)
{
	int i;
	MTM_LOG1("Recovery of node %d is completed, disabled mask=%llx, connectivity mask=%llx, endLSN=%lx, live nodes=%d",
			 MtmNodeId, (long long) Mtm->disabledNodeMask, 
			 (long long)SELF_CONNECTIVITY_MASK, GetXLogInsertRecPtr(), Mtm->nLiveNodes);
	MtmLock(LW_EXCLUSIVE);
	Mtm->recoverySlot = 0;
	Mtm->recoveredLSN = GetXLogInsertRecPtr();
	BIT_CLEAR(Mtm->disabledNodeMask, MtmNodeId-1);
	Mtm->nConfigChanges += 1;
	Mtm->reconnectMask |= SELF_CONNECTIVITY_MASK; /* try to reestablish all connections */
	Mtm->nodes[MtmNodeId-1].lastStatusChangeTime = MtmGetSystemTime();
	for (i = 0; i < Mtm->nAllNodes; i++) { 
		Mtm->nodes[i].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
	}
	/* Mode will be changed to online once all logical receiver are connected */
	elog(LOG, "Recovery completed with %d active receivers and %d started senders from %d", Mtm->nReceivers, Mtm->nSenders, Mtm->nLiveNodes-1);
	MtmSwitchClusterMode(Mtm->nReceivers == Mtm->nLiveNodes-1 && Mtm->nSenders == Mtm->nLiveNodes-1 ? MTM_ONLINE : MTM_CONNECTED);
	MtmUnlock();
}



/**
 * Check state of replication slots. If some of them are too much lag behind wal, then drop this slots to avoid 
 * WAL overflow
 */
static void 
MtmCheckSlots()
{
	if (MtmMaxRecoveryLag != 0 && Mtm->disabledNodeMask != 0) 
	{
		int i;
		for (i = 0; i < max_replication_slots; i++) { 
			ReplicationSlot* slot = &ReplicationSlotCtl->replication_slots[i];
			int nodeId;
			if (slot->in_use 
				&& sscanf(slot->data.name.data, MULTIMASTER_SLOT_PATTERN, &nodeId) == 1
				&& BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)
				&& slot->data.confirmed_flush + MtmMaxRecoveryLag < GetXLogInsertRecPtr()) 
			{
				elog(WARNING, "Drop slot for node %d which lag %ld is larger than threshold %d", 
						 nodeId,
						 GetXLogInsertRecPtr() - slot->data.restart_lsn,
						 MtmMaxRecoveryLag);
				ReplicationSlotDrop(slot->data.name.data);
			}
		}
	}
}

/*
 * Get lag between replication slot position (dsata proceeded by WAL sender) and current position in WAL 
 */
static int64 MtmGetSlotLag(int nodeId)
{
	int i;
	for (i = 0; i < max_replication_slots; i++) { 
		ReplicationSlot* slot = &ReplicationSlotCtl->replication_slots[i];
		int node;
		if (slot->in_use 
			&& sscanf(slot->data.name.data, MULTIMASTER_SLOT_PATTERN, &node) == 1
			&& node == nodeId)
		{
			return GetXLogInsertRecPtr() - slot->data.confirmed_flush;
		}
	}
	return -1;
}


/*
 * This function is called by WAL sender when start sending new transaction.
 * It returns true if specified node is in recovery mode. In this case we should send to it all transactions from WAL, 
 * not only coordinated by self node as in normal mode.
 */
bool MtmIsRecoveredNode(int nodeId)
{
	if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) { 
		if (!MtmIsRecoverySession) { 
			elog(ERROR, "Node %d is marked as disabled but is not in recovery mode", nodeId);
		}
		return true;
	} else { 
		MtmIsRecoverySession = false; /* recovery is completed */
		return false;
	}
}

/* 
 * Check if wal sender replayed all transactions from WAL log.
 * It can never happen if there are many active transactions.
 * In this case we wait until gap between sent and current position in the 
 * WAL becomes smaller than threshold value MtmMinRecoveryLag and 
 * after it prohibit start of new transactions until WAL is completely replayed.
 */
bool MtmRecoveryCaughtUp(int nodeId, XLogRecPtr slotLSN)
{
	bool caughtUp = false;
	MtmLock(LW_EXCLUSIVE);
	if (MtmIsRecoveredNode(nodeId)) { 
		XLogRecPtr walLSN = GetXLogInsertRecPtr();
		if (slotLSN == walLSN && Mtm->nActiveTransactions == 0) {
			if (BIT_CHECK(Mtm->nodeLockerMask, nodeId-1)) { 
				MTM_LOG1("Node %d is caught-up", nodeId);	
				BIT_CLEAR(Mtm->walSenderLockerMask, MyWalSnd - WalSndCtl->walsnds);
				BIT_CLEAR(Mtm->nodeLockerMask, nodeId-1);
				Mtm->nLockers -= 1;
			} else { 
				MTM_LOG1("%d: node %d is caught-up without locking cluster", MyProcPid, nodeId);	
				/* We are lucky: caught-up without locking cluster! */
			}
			MtmEnableNode(nodeId);
			caughtUp = true;
		} else if (!BIT_CHECK(Mtm->nodeLockerMask, nodeId-1)
				   && slotLSN + MtmMinRecoveryLag > walLSN) 
		{ 
			/*
			 * Wal sender almost catched up.
			 * Lock cluster preventing new transaction to start until wal is completely replayed.
			 * We have to maintain two bitmasks: one is marking wal sender, another - correspondent nodes. 
			 * Is there some better way to establish mapping between nodes ad WAL-seconder?
			 */
			MTM_LOG1("Node %d is almost caught-up: slot position %lx, WAL position %lx, active transactions %d", 
				 nodeId, slotLSN, walLSN, Mtm->nActiveTransactions);
			Assert(MyWalSnd != NULL); /* This function is called by WAL-sender, so it should not be NULL */
			BIT_SET(Mtm->nodeLockerMask, nodeId-1);
			BIT_SET(Mtm->walSenderLockerMask, MyWalSnd - WalSndCtl->walsnds);
			Mtm->nLockers += 1;
		} else { 
			MTM_LOG2("Continue recovery of node %d, slot position %lx, WAL position %lx,"
			" WAL sender position %lx, lockers %d, active transactions %d", nodeId, slotLSN,
			walLSN, MyWalSnd->sentPtr, Mtm->nLockers, Mtm->nActiveTransactions);
		}
	}
	MtmUnlock();
	return caughtUp;
}

/*
 * This function is called inside critical section
 */
void MtmSwitchClusterMode(MtmNodeStatus mode)
{
	Mtm->status = mode;
	Mtm->nodes[MtmNodeId-1].lastStatusChangeTime = MtmGetSystemTime();
	MTM_LOG1("Switch to %s mode", MtmNodeStatusMnem[mode]);
	/* ??? Something else to do here? */
}


/*
 * If there are recovering nodes which are catching-up WAL, check the status and prevent new transaction from commit to give
 * WAL-sender a chance to catch-up WAL, completely synchronize replica and switch it to normal mode.
 * This function is called before transaction prepare with multimaster lock set.
 */
static void 
MtmCheckClusterLock()
{	
	timestamp_t delay = MIN_WAIT_TIMEOUT;
	while (true)
	{
		nodemask_t mask = Mtm->walSenderLockerMask;
		if (mask != 0) {
			if (Mtm->nActiveTransactions == 0) { 
				XLogRecPtr currLogPos = GetXLogInsertRecPtr();
				int i;
				for (i = 0; mask != 0; i++, mask >>= 1) { 
					if (mask & 1) { 
						if (WalSndCtl->walsnds[i].sentPtr != currLogPos) {
							/* recovery is in progress */
							break;
						} else { 
							/* recovered replica catched up with master */
							MTM_LOG1("WAL-sender %d complete recovery", i);
							BIT_CLEAR(Mtm->walSenderLockerMask, i);
						}
					}
				}
			}
			if (mask != 0) { 
				/* some "almost catch-up" wal-senders are still working. */
				/* Do not start new transactions until them are completed. */
				MtmUnlock();
				MtmSleep(delay);
				if (delay*2 <= MAX_WAIT_TIMEOUT) { 
					delay *= 2;
				}
				MtmLock(LW_EXCLUSIVE);
				continue;
			} else {  
				/* All lockers have synchronized their logs */
				/* Remove lock and mark them as recovered */
				MTM_LOG1("Complete recovery of %d nodes (node mask %lx)", Mtm->nLockers, (long) Mtm->nodeLockerMask);
				Assert(Mtm->walSenderLockerMask == 0);
				Assert((Mtm->nodeLockerMask & Mtm->disabledNodeMask) == Mtm->nodeLockerMask);
				Mtm->disabledNodeMask &= ~Mtm->nodeLockerMask;
				Mtm->nConfigChanges += 1;
				Mtm->nLiveNodes += Mtm->nLockers;
				Mtm->nLockers = 0;
				Mtm->nodeLockerMask = 0;
				MtmCheckQuorum();
			}
		}
		break;
	}
}	

/**
 * Build internode connectivity mask. 1 - means that node is disconnected.
 */
static void
MtmBuildConnectivityMatrix(nodemask_t* matrix)
{
	int i, j, n = Mtm->nAllNodes;
	bool changed = false;

	for (i = 0; i < n; i++) { 
		matrix[i] = Mtm->nodes[i].connectivityMask;
		if (lastKnownMatrix[i] != matrix[i])
		{
			changed = true;
			lastKnownMatrix[i] = matrix[i];
		}
	}

	/* Print matrix if changed */
	if (changed)
	{
		fprintf(stderr, "Connectivity matrix:\n");
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
				putc(BIT_CHECK(matrix[i], j) ? 'X' : '+', stderr);
			putc('\n', stderr);
		}
		fputs("-----------------------\n", stderr);
	}

	/* make matrix symetric: required for Bron–Kerbosch algorithm */
	for (i = 0; i < n; i++) { 
		for (j = 0; j < i; j++) { 
			matrix[i] |= ((matrix[j] >> i) & 1) << j;
			matrix[j] |= ((matrix[i] >> j) & 1) << i;
		}
		matrix[i] &= ~((nodemask_t)1 << i);
	}
}


/**
 * Build connectivity graph, find clique in it and extend disabledNodeMask by nodes not included in clique.
 * This fnuctions is called by arbiter monitor process with period MtmHeartbeatSendTimeout
 */
void MtmRefreshClusterStatus()
{
	nodemask_t mask, newClique, disabled;
	nodemask_t matrix[MAX_NODES];
	int cliqueSize;
	nodemask_t oldClique = ~Mtm->disabledNodeMask & (((nodemask_t)1 << Mtm->nAllNodes)-1);
	int i;

	MtmBuildConnectivityMatrix(matrix);
	newClique = MtmFindMaxClique(matrix, Mtm->nAllNodes, &cliqueSize);

	if (newClique == oldClique) {
		/* Nothing is changed */
		return;
	}

	do {
		/* Otherwise make sure that all nodes have a chance to replicate their connectivity mask and we have the "consistent" picture.
		 * Obviously we can not get true consistent snapshot, but at least try to wait heartbeat send timeout is expired and
		 * connectivity graph is stabilized.
		 */
		oldClique = newClique;		
		MtmSleep(MSEC_TO_USEC(MtmHeartbeatSendTimeout)*2); /* double timeout to condider worst case when heartbeat send interval is added with refresh cluster status interval */
		MtmBuildConnectivityMatrix(matrix);
		newClique = MtmFindMaxClique(matrix, Mtm->nAllNodes, &cliqueSize);
	} while (newClique != oldClique);

	if (cliqueSize >= Mtm->nAllNodes/2+1) { /* have quorum */
		fprintf(stderr, "Old mask: ");
		for (i = 0; i <  Mtm->nAllNodes; i++) { 
			putc(BIT_CHECK(Mtm->disabledNodeMask, i) ? '-' : '+', stderr);
		}
		putc('\n', stderr);
		fprintf(stderr, "New mask: ");
		for (i = 0; i <  Mtm->nAllNodes; i++) { 
			putc(BIT_CHECK(newClique, i) ? '+' : '-', stderr);
		}
		putc('\n', stderr);

		MTM_LOG1("Find clique %lx, disabledNodeMask %lx", (long)newClique, (long)Mtm->disabledNodeMask);
		MtmLock(LW_EXCLUSIVE);
		disabled = ~newClique & (((nodemask_t)1 << Mtm->nAllNodes)-1) & ~Mtm->disabledNodeMask; /* new disabled nodes mask */
		
		if (disabled) { 
			timestamp_t now = MtmGetSystemTime();
			for (i = 0, mask = disabled; mask != 0; i++, mask >>= 1) {
				if (mask & 1) { 
					if (Mtm->nodes[i].lastStatusChangeTime + MSEC_TO_USEC(MtmNodeDisableDelay) < now) {
						MtmDisableNode(i+1);
					}
				}
			}
			MtmCheckQuorum();
		}			
#if 0
		if (disabled) {
			MtmTransState *ts;
			/* Interrupt voting for active transaction and abort them */
			for (ts = Mtm->transListHead; ts != NULL; ts = ts->next) { 
				MTM_LOG3("Active transaction gid='%s', coordinator=%d, xid=%d, status=%s, gtid.xid=%d",
						 ts->gid, ts->gtid.node, ts->xid, MtmTxnStatusMnen[ts->status], ts->gtid.xid);
				if (MtmIsCoordinator(ts) && !ts->votingCompleted && ts->status != TRANSACTION_STATUS_ABORTED) {
					MtmAbortTransaction(ts);
					MtmWakeUpBackend(ts);
				}
			}
		}
#endif
		MtmUnlock();
		if (BIT_CHECK(Mtm->disabledNodeMask, MtmNodeId-1)) { 
			if (Mtm->status == MTM_ONLINE) {
				/* I was excluded from cluster:( */
				MtmSwitchClusterMode(MTM_OFFLINE);
			}
		} else if (Mtm->status == MTM_OFFLINE) {
			/* Should we somehow restart logical receivers? */ 			
			MtmStartRecovery();
		}
	} else { 
		MTM_LOG1("Clique %lx has no quorum", (long)newClique);
		MtmSwitchClusterMode(MTM_IN_MINORITY);
	}
}

/*
 * Check if there is quorum: current node see more than half of all nodes
 */
void MtmCheckQuorum(void)
{
	if (Mtm->nLiveNodes < Mtm->nAllNodes/2+1) {
		if (Mtm->status == MTM_ONLINE) { /* out of quorum */
			elog(WARNING, "Node is in minority: disabled mask %lx", (long) Mtm->disabledNodeMask);
			MtmSwitchClusterMode(MTM_IN_MINORITY);
		}
	} else {
		if (Mtm->status == MTM_IN_MINORITY) { 
			MTM_LOG1("Node is in majority: disabled mask %lx", (long) Mtm->disabledNodeMask);
			MtmSwitchClusterMode(MTM_ONLINE);
		}
	}
}
			
/* 
 * This function is called in case of non-recoverable connection failure with this node.
 * Non-recoverable means that connections can not be reestablish using specified number of attempts.
 * It sets bit in connectivity mask and register delayed refresh of cluster status which build connectivity matrix 
 * and determine clique of connected nodes. Timeout here is needed to allow all nodes to exchanges their connectivity masks (them
 * are sent together with any arbiter message, including heartbeats.
 */
void MtmOnNodeDisconnect(int nodeId)
{ 
	timestamp_t now = MtmGetSystemTime();
	if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1))
	{
		/* Node is already disabled */
		return;
	}
	if (Mtm->nodes[nodeId-1].lastStatusChangeTime + MSEC_TO_USEC(MtmNodeDisableDelay) > now) 
	{ 
		/* Avoid false detection of node failure and prevent node status blinking */
		return;
	}
	MtmLock(LW_EXCLUSIVE);
	BIT_SET(SELF_CONNECTIVITY_MASK, nodeId-1);
	BIT_SET(Mtm->reconnectMask, nodeId-1);
	elog(LOG, "Disconnect node %d connectivity mask %llx", 
		 nodeId, (long long)SELF_CONNECTIVITY_MASK);
	MtmUnlock();
}

/*
 * This method is called when connection with node is reestablished
 */
void MtmOnNodeConnect(int nodeId)
{
	MtmLock(LW_EXCLUSIVE);	
	elog(LOG, "Connect node %d connectivity mask %llx", nodeId, (long long)SELF_CONNECTIVITY_MASK);
	BIT_CLEAR(SELF_CONNECTIVITY_MASK, nodeId-1);
	BIT_SET(Mtm->reconnectMask, nodeId-1); /* force sender to reestablish connection and send heartbeat */
	MtmUnlock();
}

/*
 * Set reconnect mask to force reconnection attempt to the node
 */
void MtmReconnectNode(int nodeId)
{
	MtmLock(LW_EXCLUSIVE);	
	BIT_SET(Mtm->reconnectMask, nodeId-1);
	MtmUnlock();
}



/*
 * -------------------------------------------
 * Node initialization
 * -------------------------------------------
 */

/*
 * Initialize Xid2State hash table to obtain status of transaction by its local XID. 
 * Size of this hash table should be limited by MtmAdjustOldestXid function which performs cleanup
 * of transaction list and from the list and from the hash table transactions which XIDs are not used in any snapshot at any node
 */
static HTAB* 
MtmCreateXidMap(void)
{
	HASHCTL info;
	HTAB* htab;
	Assert(MtmMaxNodes > 0);
	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(MtmTransState) + (MtmMaxNodes-1)*sizeof(TransactionId);
	htab = ShmemInitHash(
		"MtmXid2State",
		MTM_HASH_SIZE, MTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_BLOBS
	);
	return htab;
}

/*
 * Initialize Gid2State hash table to obtain status of transaction by GID. 
 * Size of this hash table should be limited by MtmAdjustOldestXid function which performs cleanup
 * of transaction list and from the list and from the hash table transactions which XIDs are not used in any snapshot at any node
 */
static HTAB* 
MtmCreateGidMap(void)
{
	HASHCTL info;
	HTAB* htab;
	memset(&info, 0, sizeof(info));
	info.keysize = MULTIMASTER_MAX_GID_SIZE;
	info.entrysize = sizeof(MtmTransMap);
	htab = ShmemInitHash(
		"MtmGid2State",
		MTM_MAP_SIZE, MTM_MAP_SIZE,
		&info,
		HASH_ELEM 
	);
	return htab;
}

/* 
 * Initialize hash table used to mark local (not distributed) tables
 */
static HTAB* 
MtmCreateLocalTableMap(void)
{
	HASHCTL info;
	HTAB* htab;
	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(Oid);
	htab = ShmemInitHash(
		"MtmLocalTables",
		MULTIMASTER_MAX_LOCAL_TABLES, MULTIMASTER_MAX_LOCAL_TABLES,
		&info,
		0 
	);
	return htab;
}

static void MtmMakeRelationLocal(Oid relid)
{
	if (OidIsValid(relid)) { 
		MtmLock(LW_EXCLUSIVE);		
		hash_search(MtmLocalTables, &relid, HASH_ENTER, NULL);
		MtmUnlock();		
	}
}	


void MtmMakeTableLocal(char* schema, char* name)
{
	RangeVar* rv = makeRangeVar(schema, name, -1);
	Oid relid = RangeVarGetRelid(rv, NoLock, true);
	MtmMakeRelationLocal(relid);
}


typedef struct { 
	NameData schema;
	NameData name;
} MtmLocalTablesTuple;

static void MtmLoadLocalTables(void)
{
	RangeVar	   *rv;
	Relation		rel;
	SysScanDesc		scan;
	HeapTuple		tuple;

	Assert(IsTransactionState());

	rv = makeRangeVar(MULTIMASTER_SCHEMA_NAME, MULTIMASTER_LOCAL_TABLES_TABLE, -1);
	rel = heap_openrv_extended(rv, RowExclusiveLock, true);
	if (rel != NULL) { 
		scan = systable_beginscan(rel, 0, true, NULL, 0, NULL);
		
		while (HeapTupleIsValid(tuple = systable_getnext(scan)))
		{
			MtmLocalTablesTuple	*t = (MtmLocalTablesTuple*) GETSTRUCT(tuple);
			MtmMakeTableLocal(NameStr(t->schema), NameStr(t->name));
		}

		systable_endscan(scan);
		heap_close(rel, RowExclusiveLock);
	}
}

/*
 * Multimaster control file is used to prevent erroneous inclusion of node in the cluster.
 * It contains cluster name (any user defined identifier) and node id.
 * In case of creating new cluster node using pg_basebackup this file is copied together will
 * all other PostgreSQL files and so new node will know ID of the cluster node from which it
 * is cloned. It is necessary to complete synchronization of new node with the rest of the cluster.
 */
static void MtmCheckControlFile(void)
{
	char controlFilePath[MAXPGPATH];
	char buf[MULTIMASTER_MAX_CTL_STR_SIZE];
	FILE* f;
	snprintf(controlFilePath, MAXPGPATH, "%s/global/mmts_control", DataDir);
	f = fopen(controlFilePath, "r");
	if (f != NULL && fgets(buf, sizeof buf, f)) {
		char* sep = strchr(buf, ':');
		if (sep == NULL) {
			elog(FATAL, "File mmts_control doesn't contain cluster name");
		}
		*sep = '\0';
		if (strcmp(buf, MtmClusterName) != 0) { 
			elog(FATAL, "Database belongs to some other cluster %s rather than %s", buf, MtmClusterName);
		}
		if (sscanf(sep+1, "%d", &Mtm->donorNodeId) != 1) { 
			elog(FATAL, "File mmts_control doesn't contain node id");
		}
		fclose(f);
	} else { 
		if (f != NULL) { 
			fclose(f);
		}
		f = fopen(controlFilePath, "w");
		if (f == NULL) { 
			elog(FATAL, "Failed to create mmts_control file: %m");
		}
		Mtm->donorNodeId = MtmNodeId;
		fprintf(f, "%s:%d\n", MtmClusterName, Mtm->donorNodeId);
		fclose(f);
	}
}

/* 
 * Perform initialization of multimaster state. 
 * This function is called from shared memory startup hook (after completion of initialization of shared memory)
 */
static void MtmInitialize()
{
	bool found;
	int i;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	Mtm = (MtmState*)ShmemInitStruct(MULTIMASTER_NAME, sizeof(MtmState) + sizeof(MtmNodeInfo)*(MtmMaxNodes-1), &found);
	if (!found)
	{
		Mtm->status = MTM_INITIALIZATION;
		Mtm->recoverySlot = 0;
		Mtm->locks = GetNamedLWLockTranche(MULTIMASTER_NAME);
		Mtm->csn = MtmGetCurrentTime();
		Mtm->lastCsn = INVALID_CSN;
		Mtm->oldestXid = FirstNormalTransactionId;
        Mtm->nLiveNodes = MtmNodes;
        Mtm->nAllNodes = MtmNodes;
		Mtm->disabledNodeMask = 0;
		Mtm->pglogicalReceiverMask = 0;
		Mtm->pglogicalSenderMask = 0;
		Mtm->walSenderLockerMask = 0;
		Mtm->nodeLockerMask = 0;
		Mtm->reconnectMask = 0;
		Mtm->recoveredLSN = InvalidXLogRecPtr;
		Mtm->nLockers = 0;
		Mtm->nActiveTransactions = 0;
		Mtm->votingTransactions = NULL;
        Mtm->transListHead = NULL;
        Mtm->transListTail = &Mtm->transListHead;		
        Mtm->nReceivers = 0;
        Mtm->nSenders = 0;
		Mtm->timeShift = 0;		
		Mtm->transCount = 0;
		Mtm->gcCount = 0;
		Mtm->nConfigChanges = 0;
		Mtm->recoveryCount = 0;
		Mtm->localTablesHashLoaded = false;
		Mtm->preparedTransactionsLoaded = false;
		Mtm->inject2PCError = 0;
		Mtm->sendQueue = NULL;
		Mtm->freeQueue = NULL;
		for (i = 0; i < MtmNodes; i++) {
			Mtm->nodes[i].oldestSnapshot = 0;
			Mtm->nodes[i].disabledNodeMask = 0;
			Mtm->nodes[i].connectivityMask = 0;
			Mtm->nodes[i].lockGraphUsed = 0;
			Mtm->nodes[i].lockGraphAllocated = 0;
			Mtm->nodes[i].lockGraphData = NULL;
			Mtm->nodes[i].transDelay = 0;
			Mtm->nodes[i].lastStatusChangeTime = MtmGetSystemTime();
			Mtm->nodes[i].con = MtmConnections[i];
			Mtm->nodes[i].flushPos = 0;
			Mtm->nodes[i].lastHeartbeat = 0;
			Mtm->nodes[i].restartLSN = InvalidXLogRecPtr;
			Mtm->nodes[i].originId = InvalidRepOriginId;
			Mtm->nodes[i].timeline = 0;
		}
		Mtm->nodes[MtmNodeId-1].originId = DoNotReplicateId;
		/* All transaction originated from the current node should be ignored during recovery */
		Mtm->nodes[MtmNodeId-1].restartLSN = (XLogRecPtr)PG_UINT64_MAX;
		PGSemaphoreCreate(&Mtm->sendSemaphore);
		PGSemaphoreReset(&Mtm->sendSemaphore);
		SpinLockInit(&Mtm->queueSpinlock);
		BgwPoolInit(&Mtm->pool, MtmExecutor, MtmDatabaseName, MtmDatabaseUser, MtmQueueSize, MtmWorkers);
		RegisterXactCallback(MtmXactCallback, NULL);
		MtmTx.snapshot = INVALID_CSN;
		MtmTx.xid = InvalidTransactionId;		
	}
	MtmXid2State = MtmCreateXidMap();
	MtmGid2State = MtmCreateGidMap();
	MtmLocalTables = MtmCreateLocalTableMap();
    MtmDoReplication = true;
	TM = &MtmTM;
	LWLockRelease(AddinShmemInitLock);

	MtmCheckControlFile();
}

static void 
MtmShmemStartup(void)
{
	if (PreviousShmemStartupHook) {
		PreviousShmemStartupHook();
	}
	MtmInitialize();
}

/*
 * Parse node connection string.
 * This function is called at cluster startup and while adding new cluster node
 */
void MtmUpdateNodeConnectionInfo(MtmConnectionInfo* conn, char const* connStr)
{
	char const* host;
	char const* end;
	int         hostLen;
	char*       port;
	int         connStrLen = (int)strlen(connStr);

	if (connStrLen >= MULTIMASTER_MAX_CONN_STR_SIZE) {
		elog(ERROR, "Too long (%d) connection string '%s': limit is %d", 
			 connStrLen, connStr, MULTIMASTER_MAX_CONN_STR_SIZE-1);
	}
	strcpy(conn->connStr, connStr);

	host = strstr(connStr, "host=");
	if (host == NULL) {
		elog(ERROR, "Host not specified in connection string: '%s'", connStr);
	}
	host += 5;
	for (end = host; *end != ' ' && *end != '\0'; end++);
	hostLen = end - host;
	if (hostLen >= MULTIMASTER_MAX_HOST_NAME_SIZE) {
		elog(ERROR, "Too long (%d) host name '%.*s': limit is %d", 
			 hostLen, hostLen, host, MULTIMASTER_MAX_HOST_NAME_SIZE-1);
	}
	memcpy(conn->hostName, host, hostLen);
	conn->hostName[hostLen] = '\0';

	port = strstr(connStr, "arbiter_port=");
	if (port != NULL) {
		if (sscanf(port+13, "%d", &conn->arbiterPort) != 1) {
			elog(ERROR, "Invalid arbiter port: %s", port+13);
		}
	} else { 
		conn->arbiterPort = MULTIMASTER_DEFAULT_ARBITER_PORT;
	}

	elog(WARNING, "Using arbiter port: %d", conn->arbiterPort);
}

/*
 * Parse "multimaster.conn_strings" configuration parameter and 
 * set connection string for each node using MtmUpdateNodeConnectionInfo
 */
static void MtmSplitConnStrs(void)
{
	int i;
	FILE* f = NULL;
	char buf[MULTIMASTER_MAX_CTL_STR_SIZE];
	MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);

	if (*MtmConnStrs == '@') { 
		f = fopen(MtmConnStrs+1, "r");
		for (i = 0; fgets(buf, sizeof buf, f) != NULL; i++) {
			if (strlen(buf) <= 1) {
				elog(ERROR, "Empty lines are not allowed in %s file", MtmConnStrs+1);
			}
		}
	} else { 
		char* p = MtmConnStrs;
		for (i = 0; *p != '\0'; p++, i++) { 
			if ((p = strchr(p, ',')) == NULL) { 
				i += 1;
				break;
			}
		}
	}
			
	if (i > MAX_NODES) { 
		elog(ERROR, "Multimaster with more than %d nodes is not currently supported", MAX_NODES);
	}
	if (i < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
	}	
	if (MtmMaxNodes == 0) {
		MtmMaxNodes = i;
	} else if (MtmMaxNodes < i) { 
        elog(ERROR, "More than %d nodes are specified", MtmMaxNodes);
	}			
	MtmNodes = i;
	MtmConnections = (MtmConnectionInfo*)palloc(MtmMaxNodes*sizeof(MtmConnectionInfo));

	if (f != NULL) { 
		fseek(f, SEEK_SET, 0);
		for (i = 0; fgets(buf, sizeof buf, f) != NULL; i++) {
			size_t len = strlen(buf);
			if (buf[len-1] == '\n') { 
				buf[len-1] = '\0';
			}				
			MtmUpdateNodeConnectionInfo(&MtmConnections[i], buf);
		}
		fclose(f);
	} else { 												  
		char* copy = pstrdup(MtmConnStrs);
		char* connStr = copy;
		char* connStrEnd = connStr + strlen(connStr);

		for (i = 0; connStr < connStrEnd; i++) { 
			char* p = strchr(connStr, ',');
			if (p == NULL) { 
				p = connStrEnd;
			}
			*p = '\0';			
			MtmUpdateNodeConnectionInfo(&MtmConnections[i], connStr);
			connStr = p + 1;
		}
		pfree(copy);
	}
	if (MtmNodeId == INT_MAX) { 
		if (gethostname(buf, sizeof buf) != 0) {
			elog(ERROR, "Failed to get host name: %m");
		}
		for (i = 0; i < MtmNodes; i++) {
			if (strcmp(MtmConnections[i].hostName, buf) == 0) { 
				if (MtmNodeId == INT_MAX) { 
					MtmNodeId = i+1;
				} else {
					elog(ERROR, "multimaster.node_id is not explicitly specified and more than one nodes are configured for host %s", buf);
				}
			}
		}
		if (MtmNodeId == INT_MAX) { 
			elog(ERROR, "multimaster.node_id and host name %s can not be located in connection strings list", buf);
		}
	} else if (MtmNodeId > i) {
		elog(ERROR, "Multimaster node id %d is out of range [%d..%d]", MtmNodeId, 1, MtmNodes);
	}
	{
		char* connStr = MtmConnections[MtmNodeId-1].connStr;
		char* dbName = strstr(connStr, "dbname="); // XXX: shoud we care about string 'itisnotdbname=xxx'?
		char* dbUser = strstr(connStr, "user=");
		char* end;
		size_t len;
		
		if (dbName == NULL)
			elog(ERROR, "Database is not specified in connection string: '%s'", connStr);
		
		if (dbUser == NULL)
		{
			char *errstr;
			const char *username = get_user_name(&errstr);
			if (!username)
				elog(FATAL, "Database user is not specified in connection string '%s', fallback failed: %s", connStr, errstr);
			else
				elog(WARNING, "Database user is not specified in connection string '%s', fallback to '%s'", connStr, username);
			MtmDatabaseUser = pstrdup(username);
		}
		else
		{
			dbUser += 5;
			end = strchr(dbUser, ' ');
			if (!end) end = strchr(dbUser, '\0');
			Assert(end != NULL);
			len = end - dbUser;
			MtmDatabaseUser = pnstrdup(dbUser, len);
		}
		
		dbName += 7;
		end = strchr(dbName, ' ');
		if (!end) end = strchr(dbName, '\0');
		Assert(end != NULL);
		len = end - dbName;
		MtmDatabaseName = pnstrdup(dbName, len);
	}
	MemoryContextSwitchTo(old_context);
}

/*
 * Check correctness of multimaster configuration
 */
static bool ConfigIsSane(void)
{
	bool ok = true;

	if (DefaultXactIsoLevel != XACT_REPEATABLE_READ)
	{
		elog(WARNING, "multimaster requires default_transaction_isolation = 'repeatable read'");
		ok = false;
	}

	if (MtmMaxNodes < 1)
	{
		elog(WARNING, "multimaster requires multimaster.max_nodes > 0");
		ok = false;
	}

	if (max_prepared_xacts < 1)
	{
		elog(WARNING,
			 "multimaster requires max_prepared_transactions > 0, "
			 "because all transactions are implicitly two-phase");
		ok = false;
	}

	{
		int workers_required = 2 * MtmMaxNodes + MtmWorkers + 1;
		if (max_worker_processes < workers_required)
		{
			elog(WARNING,
				 "multimaster requires max_worker_processes >= %d",
				 workers_required);
			ok = false;
		}
	}

	if (wal_level != WAL_LEVEL_LOGICAL)
	{
		elog(WARNING,
			 "multimaster requires wal_level = 'logical', "
			 "because it is build on top of logical replication");
		ok = false;
	}

	if (max_wal_senders < MtmMaxNodes)
	{
		elog(WARNING,
			 "multimaster requires max_wal_senders >= %d (multimaster.max_nodes), ",
			 MtmMaxNodes);
		ok = false;
	}

	if (max_replication_slots < MtmMaxNodes)
	{
		elog(WARNING,
			 "multimaster requires max_replication_slots >= %d (multimaster.max_nodes), ",
			 MtmMaxNodes);
		ok = false;
	}

	return ok;
}

void
_PG_init(void)
{
	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out without hooking into any of
	 * the main system.  (We don't throw error here because it seems useful to
	 * allow the cs_* functions to be created even when the
	 * module isn't active.  The functions must protect themselves against
	 * being called then, however.)
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	DefineCustomIntVariable(
		"multimaster.heartbeat_send_timeout", 
		"Timeout in milliseconds of sending heartbeat messages",
		"Period of broadcasting heartbeat messages by arbiter to all nodes",
		&MtmHeartbeatSendTimeout,
		1000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.heartbeat_recv_timeout", 
		"Timeout in milliseconds of receiving heartbeat messages",
		"If no heartbeat message is received from node within this period, it assumed to be dead",
		&MtmHeartbeatRecvTimeout,
		10000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.gc_period",
		"Number of distributed transactions after which garbage collection is started",
		"Multimaster is building xid->csn hash map which has to be cleaned to avoid hash overflow. This parameter specifies interval of invoking garbage collector for this map",
		&MtmGcPeriod,
		MTM_HASH_SIZE/10,
		1,
	    INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.max_nodes",
		"Maximal number of cluster nodes",
		"This parameters allows to add new nodes to the cluster, default value 0 restricts number of nodes to one specified in multimaster.conn_strings",
		&MtmMaxNodes,
		0,
		0,
		MAX_NODES,
		PGC_POSTMASTER,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomIntVariable(
		"multimaster.trans_spill_threshold",
		"Maximal size (Mb) of transaction after which transaction is written to the disk",
		NULL,
		&MtmTransSpillThreshold,
		1000, /* 1Gb */
		0,
		INT_MAX,
		PGC_BACKEND,
		0,\
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.node_disable_delay",
		"Minimal amount of time (msec) between node status change",
		"This delay is used to avoid false detection of node failure and to prevent blinking of node status node",
		&MtmNodeDisableDelay,
		2000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.min_recovery_lag",
		"Minimal lag of WAL-sender performing recovery after which cluster is locked until recovery is completed",
		"When wal-sender almost catch-up WAL current position we need to stop 'Achilles tortile competition' and "
		"temporary stop commit of new transactions until node will be completely repared",
		&MtmMinRecoveryLag,
		100000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.max_recovery_lag",
		"Maximal lag of replication slot of failed node after which this slot is dropped to avoid transaction log overflow",
		"Dropping slot makes it not possible to recover node using logical replication mechanism, it will be ncessary to completely copy content of some other nodes " 
		"using basebackup or similar tool. Zero value of parameter disable dropping slot.",
		&MtmMaxRecoveryLag,
		100000000,
		0,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomBoolVariable(
		"multimaster.ignore_tables_without_pk",
		"Do not replicate tables withpout primary key",
		NULL,
		&MtmIgnoreTablesWithoutPk,
		false,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomBoolVariable(
		"multimaster.use_dtm",
		"Use distributed transaction manager",
		NULL,
		&MtmUseDtm,
		true,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomBoolVariable(
		"multimaster.preserve_commit_order",
		"Transactions from one node will be committed in same order al all nodes",
		NULL,
		&MtmPreserveCommitOrder,
		true,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomBoolVariable(
		"multimaster.volkswagen_mode",
		"Pretend to be normal postgres. This means skip some NOTICE's and use local sequences. Default false.",
		NULL,
		&MtmVolksWagenMode,
		false,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.workers",
		"Number of multimaster executor workers",
		NULL,
		&MtmWorkers,
		8,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.max_workers",
		"Maximal number of multimaster dynamic executor workers",
		NULL,
		&MtmMaxWorkers,
		100,
		0,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.vacuum_delay",
		"Minimal age of records which can be vacuumed (seconds)",
		NULL,
		&MtmVacuumDelay,
		1,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.min_2pc_timeout",
		"Minimal timeout between receiving PREPARED message from nodes participated in transaction to coordinator (milliseconds)",
		NULL,
		&MtmMin2PCTimeout,
		2000, /* 2 seconds */
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.max_2pc_ratio",
		"Maximal ratio (in percents) between prepare time at different nodes: if T is time of preparing transaction at some node,"
			" then transaction can be aborted if prepared responce was not received in T*MtmMax2PCRatio/100",
		NULL,
		&MtmMax2PCRatio,
		200, /* 2 times */
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.queue_size",
		"Multimaster queue size",
		NULL,
		&MtmQueueSize,
		256*1024*1024,
	    1024*1024,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.arbiter_port",
		"Base value for assigning arbiter ports",
		NULL,
		&MtmArbiterPort,
		MULTIMASTER_DEFAULT_ARBITER_PORT,
	    0,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomStringVariable(
		"multimaster.conn_strings",
		"Multimaster node connection strings separated by commas, i.e. 'replication=database dbname=postgres host=localhost port=5001,replication=database dbname=postgres host=localhost port=5002'",
		NULL,
		&MtmConnStrs,
		"",
		PGC_BACKEND, /* context */
		0,           /* flags */
		NULL,        /* GucStringCheckHook check_hook */
		NULL,        /* GucStringAssignHook assign_hook */
		NULL         /* GucShowHook show_hook */
	);
    
	DefineCustomStringVariable(
		"multimaster.cluster_name",
		"Name of the cluster",
		NULL,
		&MtmClusterName,
		"mmts",
		PGC_BACKEND, /* context */
		0,           /* flags */
		NULL,        /* GucStringCheckHook check_hook */
		NULL,        /* GucStringAssignHook assign_hook */
		NULL         /* GucShowHook show_hook */
	);
    
	DefineCustomIntVariable(
		"multimaster.node_id",
		"Multimaster node ID",
		NULL,
		&MtmNodeId,
		INT_MAX,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.connect_timeout",
		"Multimaster nodes connect timeout",
		"Interval in milliseconds for establishing connection with cluster node",
		&MtmConnectTimeout,
		10000, /* 10 seconds */
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.reconnect_timeout",
		"Multimaster nodes reconnect timeout",
		"Interval in milliseconds for establishing connection with cluster node",
		&MtmReconnectTimeout,
		5000, /* 5 seconds */
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	if (!ConfigIsSane()) {
		elog(ERROR, "Multimaster config is insane, refusing to work");
	}

	/* This will also perform some checks on connection strings */
	MtmSplitConnStrs();

    MtmStartReceivers();

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in mtm_shmem_startup().
	 */
	RequestAddinShmemSpace(MTM_SHMEM_SIZE + MtmQueueSize);
	RequestNamedLWLockTranche(MULTIMASTER_NAME, 1 + MtmMaxNodes*2);

    BgwPoolStart(MtmWorkers, MtmPoolConstructor);

	MtmArbiterInitialize();

	/*
	 * Install hooks.
	 */
	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = MtmShmemStartup;

	PreviousExecutorStartHook = ExecutorStart_hook;
	ExecutorStart_hook = MtmExecutorStart;

	PreviousExecutorFinishHook = ExecutorFinish_hook;
	ExecutorFinish_hook = MtmExecutorFinish;

	PreviousProcessUtilityHook = ProcessUtility_hook;
	ProcessUtility_hook = MtmProcessUtility;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
	ExecutorFinish_hook = PreviousExecutorFinishHook;
	ProcessUtility_hook = PreviousProcessUtilityHook;	
}


/*
 * This functions is called by pglogical receiver main function when receiver background worker is started.
 * We switch to ONLINE mode when all receviers are connected.
 * As far as background worker can be restarted multiple times, use node bitmask.
 */
void MtmReceiverStarted(int nodeId)
{
	MtmLock(LW_EXCLUSIVE);
	if (!BIT_CHECK(Mtm->pglogicalReceiverMask, nodeId-1)) { 
		BIT_SET(Mtm->pglogicalReceiverMask, nodeId-1);
		if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) {
			MtmEnableNode(nodeId);
			MtmCheckQuorum();
		}
		elog(LOG, "Start %d receivers and %d senders from %d cluster status %s", Mtm->nReceivers+1, Mtm->nSenders, Mtm->nLiveNodes-1, MtmNodeStatusMnem[Mtm->status]);
		if (++Mtm->nReceivers == Mtm->nLiveNodes-1 && Mtm->nSenders == Mtm->nLiveNodes-1 && Mtm->status == MTM_CONNECTED) { 
			MtmSwitchClusterMode(MTM_ONLINE);
		}
	}
	MtmUnlock();
}

/* 
 * Recovery slot is node ID from which new or crash node is performing recovery.
 * This function is called in case of logical receiver error to make it possible to try to perform
 * recovery from some other node
 */
void MtmReleaseRecoverySlot(int nodeId)
{
	if (Mtm->recoverySlot == nodeId) { 
		Mtm->recoverySlot = 0;
	}
}		

/* 
 * Rollback transaction originated from the specified node.
 * This function is called either for commit logical message with AbortPrepared flag either for abort prepared logical message.
 */
void MtmRollbackPreparedTransaction(int nodeId, char const* gid)
{
	char state3pc[MAX_3PC_STATE_SIZE];
	XidStatus status = MtmExchangeGlobalTransactionStatus(gid, TRANSACTION_STATUS_ABORTED);
	MTM_LOG1("Abort prepared transaction %s status %s from node %d originId=%d", gid, MtmTxnStatusMnem[status], nodeId, Mtm->nodes[nodeId-1].originId);
	if (status == TRANSACTION_STATUS_UNKNOWN || (status == TRANSACTION_STATUS_IN_PROGRESS && GetPreparedTransactionState(gid, state3pc))) 
	{ 
		MTM_LOG1("PGLOGICAL_ABORT_PREPARED commit: gid=%s #2", gid);
		MtmResetTransaction();
		StartTransactionCommand();
		MtmBeginSession(nodeId);
		MtmSetCurrentTransactionGID(gid);
		FinishPreparedTransaction(gid, false);
		CommitTransactionCommand();
		MtmEndSession(nodeId, true);
	} else if (status == TRANSACTION_STATUS_IN_PROGRESS) {
		MtmBeginSession(nodeId);
		MtmLogAbortLogicalMessage(nodeId, gid);
		MtmEndSession(nodeId, true);
	}
}	

/*
 * Wrapper arround FinishPreparedTransaction function.
 * This function shoudl proper context for invocation of this function.
 * This function is called with MTM mutex locked.
 * It should unlock mutex before calling FinishPreparedTransaction to avoid deadlocks.
 * ts object is pinned to prevent deallocation while lock is released.
 */
void MtmFinishPreparedTransaction(MtmTransState* ts, bool commit)
{
	bool insideTransaction = IsTransactionState();

	Assert(ts->votingCompleted);

	ts->isPinned = true;
	MtmUnlock();

	MtmResetTransaction();
	
	if (!insideTransaction) { 
		StartTransactionCommand();
	}
	MtmSetCurrentTransactionCSN(ts->csn);
	MtmSetCurrentTransactionGID(ts->gid);
	FinishPreparedTransaction(ts->gid, commit);
	
	if (!insideTransaction) { 
		CommitTransactionCommand();
		Assert(ts->status == commit ? TRANSACTION_STATUS_COMMITTED : TRANSACTION_STATUS_ABORTED);
	}

	MtmLock(LW_EXCLUSIVE);				
	ts->isPinned = false;
}

/* 
 * Determine when and how we should open replication slot.
 * Druing recovery we need to open only one replication slot from which node should receive all transactions.
 * Slots at other nodes should be removed 
 */
MtmReplicationMode MtmGetReplicationMode(int nodeId, sig_atomic_t volatile* shutdown)
{
	MtmReplicationMode mode = REPLMODE_OPEN_EXISTED;

	MtmLock(LW_EXCLUSIVE);
	
	if (!Mtm->preparedTransactionsLoaded)
	{
		/* We must restore state of prepared (but no committed or aborted) transaction before start of recovery. */
		MtmLoadPreparedTransactions();
		Mtm->preparedTransactionsLoaded = true;
	}

	while ((Mtm->status != MTM_CONNECTED && Mtm->status != MTM_ONLINE) || BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) 
	{ 	
		if (*shutdown) 
		{ 
			MtmUnlock();
			return REPLMODE_EXIT;
		}
		/* We are not interested in receiving any deteriorated logical messages from recovered node, do recreate slot */
		if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) { 
			mode = REPLMODE_CREATE_NEW;
		}
		MTM_LOG2("%d: receiver slot mode %s", MyProcPid, MtmNodeStatusMnem[Mtm->status]);
		if (Mtm->status == MTM_RECOVERY) { 
			mode = REPLMODE_RECOVERED;
			if ((Mtm->recoverySlot == 0 && (Mtm->donorNodeId == MtmNodeId || Mtm->donorNodeId == nodeId))
				|| Mtm->recoverySlot == nodeId) 
			{ 
				/* Choose for recovery first available slot or slot of donor node (if any) */
				elog(WARNING, "Process %d starts recovery from node %d", MyProcPid, nodeId);
				Mtm->recoverySlot = nodeId;
				Mtm->nReceivers = 0;
				Mtm->nSenders = 0;
				Mtm->recoveryCount += 1;
				Mtm->pglogicalReceiverMask = 0;
				Mtm->pglogicalSenderMask = 0;
				MtmPollStatusOfPreparedTransactions();
				MtmUnlock();
				return REPLMODE_RECOVERY;
			}
		}
		MtmUnlock();
		/* delay opening of other slots until recovery is completed */
		MtmSleep(STATUS_POLL_DELAY);
		MtmLock(LW_EXCLUSIVE);
	}
	if (mode == REPLMODE_RECOVERED) { 
		MTM_LOG1("%d: Restart replication from node %d after end of recovery", MyProcPid, nodeId);
	} else if (mode == REPLMODE_CREATE_NEW) { 
		MTM_LOG1("%d: Start replication from recovered node %d", MyProcPid, nodeId);
	} else { 
		MTM_LOG1("%d: Continue replication from node %d", MyProcPid, nodeId);
	}
	BIT_SET(Mtm->reconnectMask, nodeId-1); /* arbiter should try to reestblish connection with this node */
	MtmUnlock();
	return mode;		
}
			
static bool MtmIsBroadcast() 
{
	return application_name != NULL && strcmp(application_name, MULTIMASTER_BROADCAST_SERVICE) == 0;
}

void MtmRecoverNode(int nodeId)
{
	if (nodeId <= 0 || nodeId > Mtm->nLiveNodes) 
	{ 
		elog(ERROR, "NodeID %d is out of range [1,%d]", nodeId, Mtm->nLiveNodes);
	}
	if (!BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) { 
		elog(ERROR, "Node %d was not disabled", nodeId);
	}
	if (!MtmIsBroadcast())
	{
		MtmBroadcastUtilityStmt(psprintf("select pg_create_logical_replication_slot('" MULTIMASTER_SLOT_PATTERN "', '" MULTIMASTER_NAME "')", nodeId), true);
	}
}
	
	
void MtmDropNode(int nodeId, bool dropSlot)
{
	MtmLock(LW_EXCLUSIVE);

	if (!BIT_CHECK(Mtm->disabledNodeMask, nodeId-1))
	{
		if (nodeId <= 0 || nodeId > Mtm->nLiveNodes) 
		{ 
			MtmUnlock();
			elog(ERROR, "NodeID %d is out of range [1,%d]", nodeId, Mtm->nLiveNodes);
		}
		MtmDisableNode(nodeId);
		MtmCheckQuorum();
		if (!MtmIsBroadcast())
		{
			MtmBroadcastUtilityStmt(psprintf("select mtm.drop_node(%d,%s)", nodeId, dropSlot ? "true" : "false"), true);
		}
		if (dropSlot) 
		{
			ReplicationSlotDrop(psprintf(MULTIMASTER_SLOT_PATTERN, nodeId));
		}		
	}

	MtmUnlock();
}

static void
MtmOnProcExit(int code, Datum arg)
{
	if (MtmReplicationNodeId > 0) { 
		Mtm->nodes[MtmReplicationNodeId-1].senderPid = -1;
		MTM_LOG1("WAL-sender to %d is terminated", MtmReplicationNodeId); 
		/* MtmOnNodeDisconnect(MtmReplicationNodeId); */
	}
}

static void 
MtmReplicationStartupHook(struct PGLogicalStartupHookArgs* args)
{
	ListCell *param;
	bool recoveryCompleted = false;
	XLogRecPtr recoveryStartPos = InvalidXLogRecPtr;

	MtmIsRecoverySession = false;
	Mtm->nodes[MtmReplicationNodeId-1].senderPid = MyProcPid;
	Mtm->nodes[MtmReplicationNodeId-1].senderStartTime = MtmGetSystemTime();
	foreach(param, args->in_params)
	{
		DefElem    *elem = lfirst(param);
		if (strcmp("mtm_replication_mode", elem->defname) == 0) { 
			if (elem->arg != NULL && strVal(elem->arg) != NULL) { 
				if (strcmp(strVal(elem->arg), "recovery") == 0) { 
					MtmIsRecoverySession = true;
				} else if (strcmp(strVal(elem->arg), "recovered") == 0) { 
					recoveryCompleted = true;
				} else if (strcmp(strVal(elem->arg), "open_existed") != 0 && strcmp(strVal(elem->arg), "create_new") != 0) { 
					elog(ERROR, "Illegal recovery mode %s", strVal(elem->arg));
				}
			} else { 
				elog(ERROR, "Replication mode is not specified");
			}				
		} else if (strcmp("mtm_restart_pos", elem->defname) == 0) { 
			if (elem->arg != NULL && strVal(elem->arg) != NULL) {
				sscanf(strVal(elem->arg), "%lx", &recoveryStartPos);
			} else { 
				elog(ERROR, "Restart position is not specified");
			}
		} else if (strcmp("mtm_recovered_pos", elem->defname) == 0) { 
			if (elem->arg != NULL && strVal(elem->arg) != NULL) {
				XLogRecPtr recoveredLSN;
				sscanf(strVal(elem->arg), "%lx", &recoveredLSN);
				MTM_LOG1("Recovered position of node %d is %lx", MtmReplicationNodeId, recoveredLSN); 
				if (Mtm->nodes[MtmReplicationNodeId-1].restartLSN < recoveredLSN) { 
					MTM_LOG2("[restartlsn] node %d: %lx -> %lx (MtmReplicationStartupHook)", MtmReplicationNodeId, Mtm->nodes[MtmReplicationNodeId-1].restartLSN, recoveredLSN);
					Assert(Mtm->nodes[MtmReplicationNodeId-1].restartLSN == InvalidXLogRecPtr 
						   || recoveredLSN < Mtm->nodes[MtmReplicationNodeId-1].restartLSN + MtmMaxRecoveryLag);
					Mtm->nodes[MtmReplicationNodeId-1].restartLSN = recoveredLSN;
				}
			} else { 
				elog(ERROR, "Recovered position is not specified");
			}
		}
	}
	MtmLock(LW_EXCLUSIVE);
	if (MtmIsRecoverySession) {		
		MTM_LOG1("%d: Node %d start recovery of node %d at position %lx", MyProcPid, MtmNodeId, MtmReplicationNodeId, recoveryStartPos);
		Assert(MyReplicationSlot != NULL);
		if (recoveryStartPos < MyReplicationSlot->data.restart_lsn) { 
			elog(WARNING, "Specified recovery start position %lx is beyond restart lsn %lx", recoveryStartPos, MyReplicationSlot->data.restart_lsn);
		}
		if (!BIT_CHECK(Mtm->disabledNodeMask,  MtmReplicationNodeId-1)) {
			MtmDisableNode(MtmReplicationNodeId);
			MtmCheckQuorum();
		}
	} else if (BIT_CHECK(Mtm->disabledNodeMask,  MtmReplicationNodeId-1)) {
		if (recoveryCompleted) { 
			MTM_LOG1("Node %d consider that recovery of node %d is completed: start normal replication", MtmNodeId, MtmReplicationNodeId); 
			MtmEnableNode(MtmReplicationNodeId);
			MtmCheckQuorum();
		} else {
			/* Force arbiter to reestablish connection with this node, send heartbeat to inform this node that it was disabled and should perform recovery */
			BIT_SET(Mtm->reconnectMask, MtmReplicationNodeId-1);
			MtmUnlock();
			elog(ERROR, "Disabled node %d tries to reconnect without recovery", MtmReplicationNodeId); 
		}
	} else {
		MTM_LOG1("Node %d start logical replication to node %d in normal mode", MtmNodeId, MtmReplicationNodeId); 
	}
	if (!BIT_CHECK(Mtm->pglogicalSenderMask, MtmReplicationNodeId-1)) { 
		elog(LOG, "Start %d senders and %d receivers from %d cluster status %s", Mtm->nSenders+1, Mtm->nReceivers, Mtm->nLiveNodes-1, MtmNodeStatusMnem[Mtm->status]);
		BIT_SET(Mtm->pglogicalSenderMask, MtmReplicationNodeId-1);
		if (++Mtm->nSenders == Mtm->nLiveNodes-1 && Mtm->nReceivers == Mtm->nLiveNodes-1 && Mtm->status == MTM_CONNECTED) { 
			/* All logical replication connections from and to this node are established, so we can switch cluster to online mode */
			MtmSwitchClusterMode(MTM_ONLINE);
		}
	}
	BIT_SET(Mtm->reconnectMask, MtmReplicationNodeId-1); /* arbiter should try to reestablish connection with this node */
	MtmUnlock();
	on_shmem_exit(MtmOnProcExit, 0);
}

XLogRecPtr MtmGetFlushPosition(int nodeId)
{
	return Mtm->nodes[nodeId-1].flushPos;
}

/**
 * Keep track of progress of WAL writer.
 * We need to notify WAL senders at other nodes which logical records 
 * are flushed to the disk and so can survive failure. In asynchronous commit mode
 * WAL is flushed by WAL writer. Current flish position can be obtained by GetFlushRecPtr().
 * So on applying new logical record we insert it in the MtmLsnMapping and compare
 * their poistions in local WAL log with current flush position.
 * The records which are flushed to the disk by WAL writer are removed from the list
 * and mapping ing mtm->nodes[].flushPos is updated for this node.
 */
void  MtmUpdateLsnMapping(int node_id, XLogRecPtr end_lsn)
{
	dlist_mutable_iter iter;
	MtmFlushPosition* flushpos;
	XLogRecPtr local_flush = GetFlushRecPtr();
	MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);

	if (end_lsn != InvalidXLogRecPtr) {
		/* Track commit lsn */
		flushpos = (MtmFlushPosition *) palloc(sizeof(MtmFlushPosition));
		flushpos->node_id = node_id;
		flushpos->local_end = XactLastCommitEnd;
		flushpos->remote_end = end_lsn;
		dlist_push_tail(&MtmLsnMapping, &flushpos->node);
	}
	MtmLock(LW_EXCLUSIVE);
	dlist_foreach_modify(iter, &MtmLsnMapping)
	{
		flushpos = dlist_container(MtmFlushPosition, node, iter.cur);
		if (flushpos->local_end <= local_flush)
		{
			if (Mtm->nodes[node_id-1].flushPos < flushpos->remote_end) { 
				Mtm->nodes[node_id-1].flushPos = flushpos->remote_end;
			}
			dlist_delete(iter.cur);
			pfree(flushpos);
		} else { 
			break;
		}
	}
	MtmUnlock();
	MemoryContextSwitchTo(old_context);
}


static void 
MtmReplicationShutdownHook(struct PGLogicalShutdownHookArgs* args)
{
	MtmLock(LW_EXCLUSIVE);
	if (MtmReplicationNodeId >= 0 && BIT_CHECK(Mtm->pglogicalSenderMask, MtmReplicationNodeId-1)) { 
		BIT_CLEAR(Mtm->pglogicalSenderMask, MtmReplicationNodeId-1);
		Mtm->nSenders -= 1;
		MTM_LOG1("Logical replication to node %d is stopped", MtmReplicationNodeId); 
		/* MtmOnNodeDisconnect(MtmReplicationNodeId); */
		MtmReplicationNodeId = -1; /* defuse MtmOnProcExit hook */
	}
	MtmUnlock();
}

/* 
 * Filter transactions which should be replicated to other nodes.
 * This filter is applied at sender side (WAL sender).
 * Final filtering is also done at destination side by MtmFilterTransaction function.
 */
static bool 
MtmReplicationTxnFilterHook(struct PGLogicalTxnFilterArgs* args)
{
	/* Do not replicate any transactions in recovery mode (because we should apply
	 * changes sent to us rather than send our own pending changes)
	 * and transactions received from other nodes 
	 * (originId should be non-zero in this case)
	 * unless we are performing recovery of disabled node 
	 * (in this case all transactions should be sent)
	 */
	bool res = Mtm->status != MTM_RECOVERY
		&& (args->origin_id == InvalidRepOriginId 
			|| MtmIsRecoveredNode(MtmReplicationNodeId));
	if (!res) { 
		MTM_LOG2("Filter transaction with origin_id=%d", args->origin_id);
	}
	return res;
}

/**
 * Filter record corresponding to local (non-distributed) tables
 */
static bool 
MtmReplicationRowFilterHook(struct PGLogicalRowFilterArgs* args)
{
	bool isDistributed;
	MtmLock(LW_SHARED);
	if (!Mtm->localTablesHashLoaded) { 
		MtmUnlock();
		MtmLock(LW_EXCLUSIVE);
		if (!Mtm->localTablesHashLoaded) { 
			MtmLoadLocalTables();
			Mtm->localTablesHashLoaded = true;
		}
	}
	isDistributed = hash_search(MtmLocalTables, &RelationGetRelid(args->changed_rel), HASH_FIND, NULL) == NULL;
	MtmUnlock();
	return isDistributed;
}

/*
 * Filter received transactions at destination side.
 * This function is executed by receiver, 
 * so there are no race conditions and it is possible to update nodes[i].restartLSN without lock. 
 * It is more efficient to filter records at senders size (done by MtmReplicationTxnFilterHook) to avoid sending useless data through network. But asynchronous nature of 
 * logical replications makes it not possible to guarantee (at least I failed to do it)
 * that replica do not receive deteriorated data.
 */
bool MtmFilterTransaction(char* record, int size)
{
	StringInfoData s;
	uint8       flags;
	XLogRecPtr  origin_lsn;
	XLogRecPtr  end_lsn;
	XLogRecPtr  restart_lsn;
	int         replication_node;
	int         origin_node;
	char const* gid = "";
	char 		msgtype PG_USED_FOR_ASSERTS_ONLY;
	bool        duplicate = false;

    s.data = record;
    s.len = size;
    s.maxlen = -1;
	s.cursor = 0;

	msgtype = pq_getmsgbyte(&s);
	Assert(msgtype == 'C');
	flags = pq_getmsgbyte(&s); /* flags */
	replication_node = pq_getmsgbyte(&s);

	/* read fields */
	pq_getmsgint64(&s); /* commit_lsn */
	end_lsn = pq_getmsgint64(&s); /* end_lsn */
	pq_getmsgint64(&s); /* commit_time */
	
	origin_node = pq_getmsgbyte(&s);
	origin_lsn = pq_getmsgint64(&s);

	Assert(replication_node == MtmReplicationNodeId && 
		   origin_node != 0 &&
		   (Mtm->status == MTM_RECOVERY || origin_node == replication_node));	

    switch (PGLOGICAL_XACT_EVENT(flags))
    {
	  case PGLOGICAL_PREPARE:
	  case PGLOGICAL_PRECOMMIT_PREPARED:
	  case PGLOGICAL_ABORT_PREPARED:
		gid = pq_getmsgstring(&s);
		break;
	  case PGLOGICAL_COMMIT_PREPARED:
		pq_getmsgint64(&s); /* CSN */
		gid = pq_getmsgstring(&s);
		break;
	  default:
		break;
	}
	restart_lsn = origin_node == MtmReplicationNodeId ? end_lsn : origin_lsn;
    if (Mtm->nodes[origin_node-1].restartLSN < restart_lsn) {
		Assert(Mtm->nodes[origin_node-1].restartLSN == InvalidXLogRecPtr 
			   || restart_lsn < Mtm->nodes[origin_node-1].restartLSN + MtmMaxRecoveryLag);
		MTM_LOG2("[restartlsn] node %d: %lx -> %lx (MtmFilterTransaction)", MtmReplicationNodeId, Mtm->nodes[MtmReplicationNodeId-1].restartLSN, restart_lsn);
		Mtm->nodes[origin_node-1].restartLSN = restart_lsn;
    } else {
		duplicate = true;
	}

	if (duplicate) {
		MTM_LOG1("Ignore transaction %s from node %d flags=%x because our LSN position %lx for origin node %d is greater or equal than LSN %lx of this transaction (end_lsn=%lx, origin_lsn=%lx)", 
				 gid, replication_node, flags, Mtm->nodes[origin_node-1].restartLSN, origin_node, restart_lsn, end_lsn, origin_lsn);
	} else {
		MTM_LOG2("Apply transaction %s from node %d lsn %lx, flags=%x, origin node %d, original lsn=%lx, current lsn=%lx", 
				 gid, replication_node, end_lsn, flags, origin_node, origin_lsn, restart_lsn);
	}

	return duplicate;
}

void MtmSetupReplicationHooks(struct PGLogicalHooks* hooks)
{
	hooks->startup_hook = MtmReplicationStartupHook;
	hooks->shutdown_hook = MtmReplicationShutdownHook;
	hooks->txn_filter_hook = MtmReplicationTxnFilterHook;
	hooks->row_filter_hook = MtmReplicationRowFilterHook;
}

/*
 * Setup replication session origin to include origin location in WAL and 
 * update slot position.
 * Sessions are not reetrant so we have to use exclusive lock here.
 */
void MtmBeginSession(int nodeId)
{
	MtmLockNode(nodeId, LW_EXCLUSIVE);
	Assert(replorigin_session_origin == InvalidRepOriginId);
	replorigin_session_origin = Mtm->nodes[nodeId-1].originId;
	Assert(replorigin_session_origin != InvalidRepOriginId);
	MTM_LOG3("%d: Begin setup replorigin session: %d", MyProcPid, replorigin_session_origin);
	replorigin_session_setup(replorigin_session_origin);
	MTM_LOG3("%d: End setup replorigin session: %d", MyProcPid, replorigin_session_origin);
}

/* 
 * Release replication session
 */
void MtmEndSession(int nodeId, bool unlock)
{
	if (replorigin_session_origin != InvalidRepOriginId) { 
		MTM_LOG2("%d: Begin reset replorigin session for node %d: %d, progress %lx", MyProcPid, nodeId, replorigin_session_origin, replorigin_session_get_progress(false));
		replorigin_session_origin = InvalidRepOriginId;
		replorigin_session_origin_lsn = InvalidXLogRecPtr;
		replorigin_session_origin_timestamp = 0;
		replorigin_session_reset();
		if (unlock) { 
			MtmUnlockNode(nodeId);
		}
		MTM_LOG3("%d: End reset replorigin session: %d", MyProcPid, replorigin_session_origin);
	}
}
	

/*
 * -------------------------------------------
 * SQL API functions
 * -------------------------------------------
 */


Datum
mtm_start_replication(PG_FUNCTION_ARGS)
{
    MtmDoReplication = true;
    PG_RETURN_VOID();
}

Datum
mtm_stop_replication(PG_FUNCTION_ARGS)
{
    MtmDoReplication = false;
    MtmTx.isDistributed = false;
    PG_RETURN_VOID();
}

Datum
mtm_drop_node(PG_FUNCTION_ARGS)
{
	int nodeId = PG_GETARG_INT32(0);
	bool dropSlot = PG_GETARG_BOOL(1);
	MtmDropNode(nodeId, dropSlot);
    PG_RETURN_VOID();
}

Datum
mtm_add_node(PG_FUNCTION_ARGS)
{
	char *connStr = text_to_cstring(PG_GETARG_TEXT_PP(0));

	if (Mtm->nAllNodes == MtmMaxNodes) { 
		elog(ERROR, "Maximal number of nodes %d is reached", MtmMaxNodes);
	}
	if (!MtmIsBroadcast())
	{
		MtmBroadcastUtilityStmt(psprintf("select mtm.add_node('%s')", connStr), true);
	} 
	else 
	{ 
		int nodeId;
		MtmLock(LW_EXCLUSIVE);	
		nodeId = Mtm->nAllNodes;
		elog(NOTICE, "Add node %d: '%s'", nodeId+1, connStr);

		MtmUpdateNodeConnectionInfo(&Mtm->nodes[nodeId].con, connStr);

		if (*MtmConnStrs == '@') {
			FILE* f = fopen(MtmConnStrs+1, "a");
			fprintf(f, "%s\n", connStr);
			fclose(f);
		}

		Mtm->nodes[nodeId].transDelay = 0;
		Mtm->nodes[nodeId].lastStatusChangeTime = MtmGetSystemTime();
		Mtm->nodes[nodeId].flushPos = 0;
		Mtm->nodes[nodeId].oldestSnapshot = 0;

		BIT_SET(Mtm->disabledNodeMask, nodeId);
		Mtm->nConfigChanges += 1;
		Mtm->nAllNodes += 1;
		MtmUnlock();

		MtmStartReceiver(nodeId+1, true);
	}
    PG_RETURN_VOID();
}
	
Datum
mtm_poll_node(PG_FUNCTION_ARGS)
{
	int nodeId = PG_GETARG_INT32(0);
	bool nowait = PG_GETARG_BOOL(1);
	bool online = true;
	while ((nodeId == MtmNodeId && Mtm->status != MTM_ONLINE)
		   || (nodeId != MtmNodeId && BIT_CHECK(Mtm->disabledNodeMask, nodeId-1))) 
	{ 
		if (nowait) { 
			online = false;
			break;
		} else { 
			MtmSleep(STATUS_POLL_DELAY);
		}
	}
	if (!nowait) { 
		/* Just wait some time until logical repication channels will be reestablished */
		MtmSleep(MSEC_TO_USEC(MtmNodeDisableDelay));
	}
    PG_RETURN_BOOL(online);
}
	
Datum
mtm_recover_node(PG_FUNCTION_ARGS)
{
	int nodeId = PG_GETARG_INT32(0);
	MtmRecoverNode(nodeId);
    PG_RETURN_VOID();
}
	
Datum
mtm_get_snapshot(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(MtmTx.snapshot);
}


Datum
mtm_get_last_csn(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(Mtm->lastCsn);
}

Datum
mtm_get_csn(PG_FUNCTION_ARGS)
{
	TransactionId xid = PG_GETARG_INT64(0);
	MtmTransState* ts;
	csn_t csn = INVALID_CSN;

	MtmLock(LW_SHARED);
    ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
    if (ts != NULL) { 
		csn = ts->csn;
	}
	MtmUnlock();

    return csn;
}

typedef struct
{
	int       nodeId;
	TupleDesc desc;
    Datum     values[Natts_mtm_nodes_state];
    bool      nulls[Natts_mtm_nodes_state];
} MtmGetNodeStateCtx;

Datum
mtm_get_nodes_state(PG_FUNCTION_ARGS)
{
    FuncCallContext* funcctx;
	MtmGetNodeStateCtx* usrfctx;
	MemoryContext oldcontext;
	int64 lag;
    bool is_first_call = SRF_IS_FIRSTCALL();

    if (is_first_call) { 
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);       
		usrfctx = (MtmGetNodeStateCtx*)palloc(sizeof(MtmGetNodeStateCtx));
		get_call_result_type(fcinfo, NULL, &usrfctx->desc);
		usrfctx->nodeId = 1;
		memset(usrfctx->nulls, false, sizeof(usrfctx->nulls));
		funcctx->user_fctx = usrfctx;
		MemoryContextSwitchTo(oldcontext);      
    }
    funcctx = SRF_PERCALL_SETUP();	
	usrfctx = (MtmGetNodeStateCtx*)funcctx->user_fctx;
	if (usrfctx->nodeId > Mtm->nAllNodes) {
		SRF_RETURN_DONE(funcctx);      
	}
	usrfctx->values[0] = Int32GetDatum(usrfctx->nodeId);
	usrfctx->values[1] = BoolGetDatum(BIT_CHECK(Mtm->disabledNodeMask, usrfctx->nodeId-1));
	usrfctx->values[2] = BoolGetDatum(BIT_CHECK(SELF_CONNECTIVITY_MASK, usrfctx->nodeId-1));
	usrfctx->values[3] = BoolGetDatum(BIT_CHECK(Mtm->nodeLockerMask, usrfctx->nodeId-1));
	lag = MtmGetSlotLag(usrfctx->nodeId);
	usrfctx->values[4] = Int64GetDatum(lag);
	usrfctx->nulls[4] = lag < 0;
	usrfctx->values[5] = Int64GetDatum(Mtm->transCount ? Mtm->nodes[usrfctx->nodeId-1].transDelay/Mtm->transCount : 0);
	usrfctx->values[6] = TimestampTzGetDatum(time_t_to_timestamptz(Mtm->nodes[usrfctx->nodeId-1].lastStatusChangeTime/USECS_PER_SEC));
	usrfctx->values[7] = Int64GetDatum(Mtm->nodes[usrfctx->nodeId-1].oldestSnapshot);
	usrfctx->values[8] = Int32GetDatum(Mtm->nodes[usrfctx->nodeId-1].senderPid);
	usrfctx->values[9] = TimestampTzGetDatum(time_t_to_timestamptz(Mtm->nodes[usrfctx->nodeId-1].senderStartTime/USECS_PER_SEC));
	usrfctx->values[10] = Int32GetDatum(Mtm->nodes[usrfctx->nodeId-1].receiverPid);
	usrfctx->values[11] = TimestampTzGetDatum(time_t_to_timestamptz(Mtm->nodes[usrfctx->nodeId-1].receiverStartTime/USECS_PER_SEC));   
	usrfctx->values[12] = CStringGetTextDatum(Mtm->nodes[usrfctx->nodeId-1].con.connStr);
	usrfctx->values[13] = Int64GetDatum(Mtm->nodes[usrfctx->nodeId-1].connectivityMask);
	usrfctx->nodeId += 1;

	SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(heap_form_tuple(usrfctx->desc, usrfctx->values, usrfctx->nulls)));
}

Datum
mtm_get_trans_by_gid(PG_FUNCTION_ARGS)
{
	TupleDesc desc;
    Datum     values[Natts_mtm_trans_state];
    bool      nulls[Natts_mtm_trans_state] = {false};
	MtmTransState* ts;
	MtmTransMap* tm;
	char *gid = text_to_cstring(PG_GETARG_TEXT_PP(0));
	int i;

	MtmLock(LW_SHARED);
	tm = (MtmTransMap*)hash_search(MtmGid2State, gid, HASH_FIND, NULL);
	if (tm == NULL) {
		MtmUnlock();
		PG_RETURN_NULL();
	}

	values[1] = CStringGetTextDatum(gid);

	ts = tm->state;
	if (ts == NULL) { 
		values[0] = CStringGetTextDatum(MtmTxnStatusMnem[tm->status]);
		for (i = 2; i < Natts_mtm_trans_state; i++) { 
			nulls[i] = true;
		}
	} else { 
		values[0] = CStringGetTextDatum(MtmTxnStatusMnem[ts->status]);		
		values[2] = Int64GetDatum(ts->xid);
		values[3] = Int32GetDatum(ts->gtid.node);
		values[4] = Int64GetDatum(ts->gtid.xid);
		values[5] = TimestampTzGetDatum(time_t_to_timestamptz(ts->csn/USECS_PER_SEC));  
		values[6] = TimestampTzGetDatum(time_t_to_timestamptz(ts->snapshot/USECS_PER_SEC));  
		values[7] = BoolGetDatum(ts->isLocal);
		values[8] = BoolGetDatum(ts->isPrepared);
		values[9] = BoolGetDatum(ts->isActive);
		values[10] = BoolGetDatum(ts->isTwoPhase);
		values[11] = BoolGetDatum(ts->votingCompleted);
		values[12] = Int64GetDatum(ts->participantsMask);
		values[13] = Int64GetDatum(ts->votedMask);
		values[14] = Int32GetDatum(ts->nConfigChanges);
	}
	MtmUnlock();

	get_call_result_type(fcinfo, NULL, &desc);
	PG_RETURN_DATUM(HeapTupleGetDatum(heap_form_tuple(desc, values, nulls)));
}
	
Datum
mtm_get_trans_by_xid(PG_FUNCTION_ARGS)
{
	TupleDesc desc;
    Datum     values[Natts_mtm_trans_state];
    bool      nulls[Natts_mtm_trans_state] = {false};
	TransactionId xid = PG_GETARG_INT64(0);
	MtmTransState* ts;

	MtmLock(LW_SHARED);
	ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
	if (ts == NULL) {
		MtmUnlock();
		PG_RETURN_NULL();
	}

	values[0] = CStringGetTextDatum(MtmTxnStatusMnem[ts->status]);		
	values[1] = CStringGetTextDatum(ts->gid);	
	values[2] = Int64GetDatum(ts->xid);
	values[3] = Int32GetDatum(ts->gtid.node);
	values[4] = Int64GetDatum(ts->gtid.xid);
	values[5] = TimestampTzGetDatum(time_t_to_timestamptz(ts->csn/USECS_PER_SEC));  
	values[6] = TimestampTzGetDatum(time_t_to_timestamptz(ts->snapshot/USECS_PER_SEC));  
	values[7] = BoolGetDatum(ts->isLocal);
	values[8] = BoolGetDatum(ts->isPrepared);
	values[9] = BoolGetDatum(ts->isActive);
	values[10] = BoolGetDatum(ts->isTwoPhase);
	values[11] = BoolGetDatum(ts->votingCompleted);
	values[12] = Int64GetDatum(ts->participantsMask);
	values[13] = Int64GetDatum(ts->votedMask);
	MtmUnlock();

	get_call_result_type(fcinfo, NULL, &desc);
	PG_RETURN_DATUM(HeapTupleGetDatum(heap_form_tuple(desc, values, nulls)));
}
	
Datum
mtm_get_cluster_state(PG_FUNCTION_ARGS)
{
	TupleDesc desc;
    Datum     values[Natts_mtm_cluster_state];
    bool      nulls[Natts_mtm_cluster_state] = {false};
	get_call_result_type(fcinfo, NULL, &desc);

	values[0] = CStringGetTextDatum(MtmNodeStatusMnem[Mtm->status]);
	values[1] = Int64GetDatum(Mtm->disabledNodeMask);
	values[2] = Int64GetDatum(SELF_CONNECTIVITY_MASK);
	values[3] = Int64GetDatum(Mtm->nodeLockerMask);
	values[4] = Int32GetDatum(Mtm->nLiveNodes);
	values[5] = Int32GetDatum(Mtm->nAllNodes);
	values[6] = Int32GetDatum((int)Mtm->pool.active);
	values[7] = Int32GetDatum((int)Mtm->pool.pending);
	values[8] = Int64GetDatum(BgwPoolGetQueueSize(&Mtm->pool));
	values[9] = Int64GetDatum(Mtm->transCount);
	values[10] = Int64GetDatum(Mtm->timeShift);
	values[11] = Int32GetDatum(Mtm->recoverySlot);
	values[12] = Int64GetDatum(hash_get_num_entries(MtmXid2State));
	values[13] = Int64GetDatum(hash_get_num_entries(MtmGid2State));
	values[14] = Int64GetDatum(Mtm->oldestXid);
	values[15] = Int32GetDatum(Mtm->nConfigChanges);

	PG_RETURN_DATUM(HeapTupleGetDatum(heap_form_tuple(desc, values, nulls)));
}


typedef struct
{
	int       nodeId;
} MtmGetClusterInfoCtx;

static void erase_option_from_connstr(const char *option, char *connstr)
{
	char *needle = psprintf("%s=", option);
	while (1) {
		char *found = strstr(connstr, needle);
		if (found == NULL) break;
		while (*found != '\0' && *found != ' ') {
			*found = ' ';
			found++;
		}
	}
	pfree(needle);
}

PGconn *PQconnectdb_safe(const char *conninfo)
{
	PGconn *conn;
	char *safe_connstr = pstrdup(conninfo);
	erase_option_from_connstr("arbiter_port", safe_connstr);

	conn = PQconnectdb(safe_connstr);

	pfree(safe_connstr);
	return conn;
}

Datum
mtm_get_cluster_info(PG_FUNCTION_ARGS)
{

    FuncCallContext* funcctx;
	MtmGetClusterInfoCtx* usrfctx;
	MemoryContext oldcontext;
	TupleDesc desc;
    bool is_first_call = SRF_IS_FIRSTCALL();
	int i;
	PGconn* conn;
	PGresult *result;
	char* values[Natts_mtm_cluster_state];
	HeapTuple tuple;

    if (is_first_call) { 
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);       
		usrfctx = (MtmGetClusterInfoCtx*)palloc(sizeof(MtmGetNodeStateCtx));
		get_call_result_type(fcinfo, NULL, &desc);
		funcctx->attinmeta = TupleDescGetAttInMetadata(desc);
		usrfctx->nodeId = 0;
		funcctx->user_fctx = usrfctx;
		MemoryContextSwitchTo(oldcontext);      
    }
    funcctx = SRF_PERCALL_SETUP();	
	usrfctx = (MtmGetClusterInfoCtx*)funcctx->user_fctx;
	while (++usrfctx->nodeId <= Mtm->nAllNodes && BIT_CHECK(Mtm->disabledNodeMask, usrfctx->nodeId-1));
	if (usrfctx->nodeId > Mtm->nAllNodes) {
		SRF_RETURN_DONE(funcctx);      
	}	
	conn = PQconnectdb_safe(Mtm->nodes[usrfctx->nodeId-1].con.connStr);
	if (PQstatus(conn) != CONNECTION_OK) {
		elog(ERROR, "Failed to establish connection '%s' to node %d: error = %s", Mtm->nodes[usrfctx->nodeId-1].con.connStr, usrfctx->nodeId, PQerrorMessage(conn));
	}
	result = PQexec(conn, "select * from mtm.get_cluster_state()");

	if (PQresultStatus(result) != PGRES_TUPLES_OK || PQntuples(result) != 1) { 
		elog(ERROR, "Failed to receive data from %d", usrfctx->nodeId);
	}

	for (i = 0; i < Natts_mtm_cluster_state; i++) { 
		values[i] = PQgetvalue(result, 0, i);
	}
	tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
	PQclear(result);
	PQfinish(conn);
	SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
}


Datum mtm_make_table_local(PG_FUNCTION_ARGS)
{
	Oid	reloid = PG_GETARG_OID(1);
	RangeVar   *rv;
	Relation	rel;
	TupleDesc	tupDesc;
	HeapTuple	tup;
	Datum		values[Natts_mtm_local_tables];
	bool		nulls[Natts_mtm_local_tables];

	MtmMakeRelationLocal(reloid);
	
	rv = makeRangeVar(MULTIMASTER_SCHEMA_NAME, MULTIMASTER_LOCAL_TABLES_TABLE, -1);
	rel = heap_openrv(rv, RowExclusiveLock);
	if (rel != NULL) {
		char* tableName = get_rel_name(reloid);
		Oid   schemaid = get_rel_namespace(reloid);
		char* schemaName = get_namespace_name(schemaid);

		tupDesc = RelationGetDescr(rel);

		/* Form a tuple. */
		memset(nulls, false, sizeof(nulls));
		
		values[Anum_mtm_local_tables_rel_schema - 1] = CStringGetTextDatum(schemaName);
		values[Anum_mtm_local_tables_rel_name - 1] = CStringGetTextDatum(tableName);

		tup = heap_form_tuple(tupDesc, values, nulls);
		
		/* Insert the tuple to the catalog. */
		simple_heap_insert(rel, tup);
		
		/* Update the indexes. */
		CatalogUpdateIndexes(rel, tup);
		
		/* Cleanup. */
		heap_freetuple(tup);
		heap_close(rel, RowExclusiveLock);

		MtmTx.containsDML = true;
	}
	return false;
}

Datum mtm_dump_lock_graph(PG_FUNCTION_ARGS)
{
	StringInfo s = makeStringInfo();
	int i;
	for (i = 0; i < Mtm->nAllNodes; i++)
	{
		size_t lockGraphSize;
		char  *lockGraphData;
		MtmLockNode(i + 1 + MtmMaxNodes, LW_SHARED);
		lockGraphSize = Mtm->nodes[i].lockGraphUsed;
		lockGraphData = palloc(lockGraphSize);
		memcpy(lockGraphData, Mtm->nodes[i].lockGraphData, lockGraphSize);
		MtmUnlockNode(i + 1 + MtmMaxNodes);

		if (lockGraphData) {
			GlobalTransactionId *gtid = (GlobalTransactionId *) lockGraphData;
			GlobalTransactionId *last = (GlobalTransactionId *) (lockGraphData + lockGraphSize);
			appendStringInfo(s, "node-%d lock graph: ", i+1);
			while (gtid != last) { 
				GlobalTransactionId *src = gtid++;
				appendStringInfo(s, "%d:%lu -> ", src->node, (long)src->xid);
				while (gtid->node != 0) {
					GlobalTransactionId *dst = gtid++;
					appendStringInfo(s, "%d:%lu, ", dst->node, (long)dst->xid);
				}
				gtid += 1;
			}
			appendStringInfo(s, "\n");
		}
	}
	return CStringGetTextDatum(s->data);
}

Datum mtm_inject_2pc_error(PG_FUNCTION_ARGS)
{
	Mtm->inject2PCError = PG_GETARG_INT32(0);
    PG_RETURN_VOID();
}

/*
 * -------------------------------------------
 * Broadcast utulity statements
 * -------------------------------------------
 */

/*
 * Execute statement with specified parameters and check its result
 */
static bool MtmRunUtilityStmt(PGconn* conn, char const* sql, char **errmsg)
{
	PGresult *result = PQexec(conn, sql);
	int status = PQresultStatus(result);

	bool ret = status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;

	if (!ret) {
		char *errstr = PQresultErrorMessage(result);
		int errlen = strlen(errstr);
		if (errlen > 9) { 
			*errmsg = palloc0(errlen);

			/* Strip "ERROR:  " from beginning and "\n" from end of error string */
			strncpy(*errmsg, errstr + 8, errlen - 1 - 8);
		}
	}

	PQclear(result);
	return ret;
}

static void 
MtmNoticeReceiver(void *i, const PGresult *res)
{
	char *notice = PQresultErrorMessage(res);
	char *stripped_notice;
	int len = strlen(notice);

	/* Skip notices from other nodes */
	if ( (*(int *)i) != MtmNodeId - 1)
		return;

	stripped_notice = palloc0(len);

	if (*notice == 'N')
	{
		/* Strip "NOTICE:  " from beginning and "\n" from end of error string */
		strncpy(stripped_notice, notice + 9, len - 1 - 9);
		elog(NOTICE, "%s", stripped_notice);
	}
	else if (*notice == 'W')
	{
		/* Strip "WARNING:  " from beginning and "\n" from end of error string */
		strncpy(stripped_notice, notice + 10, len - 1 - 10);
		elog(WARNING, "%s", stripped_notice);
	}
	else
	{
		stripped_notice = notice;
		elog(WARNING, "%s", stripped_notice);
	}

	MTM_LOG1("%s", stripped_notice);
	pfree(stripped_notice);
}

static void MtmBroadcastUtilityStmt(char const* sql, bool ignoreError)
{
	int i = 0;
	nodemask_t disabledNodeMask = Mtm->disabledNodeMask;
	int failedNode = -1;
	char const* errorMsg = NULL;
	PGconn **conns = palloc0(sizeof(PGconn*)*Mtm->nAllNodes);
	char* utility_errmsg;
	int nNodes = Mtm->nAllNodes;

	for (i = 0; i < nNodes; i++) 
	{ 
		if (!BIT_CHECK(disabledNodeMask, i)) 
		{
			conns[i] = PQconnectdb_safe(psprintf("%s application_name=%s", Mtm->nodes[i].con.connStr, MULTIMASTER_BROADCAST_SERVICE));
			if (PQstatus(conns[i]) != CONNECTION_OK)
			{
				if (ignoreError) 
				{ 
					PQfinish(conns[i]);
					conns[i] = NULL;
				} else { 
					failedNode = i;
					do { 
						PQfinish(conns[i]);
					} while (--i >= 0);                             
					elog(ERROR, "Failed to establish connection '%s' to node %d, error = %s", Mtm->nodes[failedNode].con.connStr, failedNode+1, PQerrorMessage(conns[i]));
				}
			}
			PQsetNoticeReceiver(conns[i], MtmNoticeReceiver, &i);
		}
	}
	Assert(i == nNodes);
    
	for (i = 0; i < nNodes; i++) 
	{ 
		if (conns[i]) 
		{
			if (!MtmRunUtilityStmt(conns[i], "BEGIN TRANSACTION", &utility_errmsg) && !ignoreError)
			{
				errorMsg = "Failed to start transaction at node %d";
				failedNode = i;
				break;
			}
			if (!MtmRunUtilityStmt(conns[i], sql, &utility_errmsg) && !ignoreError)
			{
				if (i + 1 == MtmNodeId)
					errorMsg = utility_errmsg;
				else
				{
					elog(ERROR, "%s", utility_errmsg);
					errorMsg = "Failed to run command at node %d";
				}

				failedNode = i;
				break;
			}
		}
	}
	if (failedNode >= 0 && !ignoreError)  
	{
		for (i = 0; i < nNodes; i++) 
		{ 
			if (conns[i])
			{
				MtmRunUtilityStmt(conns[i], "ROLLBACK TRANSACTION", &utility_errmsg);
			}
		}
	} else { 
		for (i = 0; i < nNodes; i++) 
		{ 
			if (conns[i] && !MtmRunUtilityStmt(conns[i], "COMMIT TRANSACTION", &utility_errmsg) && !ignoreError) 
			{ 
				errorMsg = "Commit failed at node %d";
				failedNode = i;
			}
		}
	}                       
	for (i = 0; i < nNodes; i++) 
	{ 
		if (conns[i])
		{
			PQfinish(conns[i]);
		}
	}
	if (!ignoreError && failedNode >= 0) 
	{ 
		elog(ERROR, errorMsg, failedNode+1);
	}
}

/*
 * Genenerate global transaction identifier for two-pahse commit.
 * It should be unique for all nodes
 */
static void
MtmGenerateGid(char* gid)
{
	static int localCount;
	sprintf(gid, "MTM-%d-%d-%d", MtmNodeId, MyProcPid, ++localCount);
}

/*
 * Replace normal commit with two-phase commit.
 * It is called either for commit of standalone command either for commit of transaction block.
 */
static bool MtmTwoPhaseCommit(MtmCurrentTrans* x)
{
	// if (MyXactAccessedTempRel)
	// {
	// 	/*
	// 	 * XXX: this tx anyway goes to subscribers later, but without
	// 	 * surrounding begin/commit. Now it will be filtered out on receiver side.
	// 	 * Probably there is more clever way to do that.
	// 	 */
	// 	x->isDistributed = false;
	// 	if (!MtmVolksWagenMode)
	// 		elog(NOTICE, "MTM: Transaction was not replicated as it accesed temporary relation");
	// 	return false;
	// }

	if (!x->isReplicated && x->isDistributed && x->containsDML) {
		MtmGenerateGid(x->gid);
		if (!x->isTransactionBlock) { 
			BeginTransactionBlock();
			x->isTransactionBlock = true;
			CommitTransactionCommand();
			StartTransactionCommand();
		}
		if (!PrepareTransactionBlock(x->gid))
		{
			if (!MtmVolksWagenMode)
				elog(WARNING, "Failed to prepare transaction %s", x->gid);
			/* ??? Should we do explicit rollback */
		} else { 	
			CommitTransactionCommand();
			if (x->isSuspended) { 
				elog(WARNING, "Transaction %s (%lu) is left in prepared state because coordinator node is not online", x->gid, (long)x->xid);
			} else { 				
				StartTransactionCommand();
				if (x->status == TRANSACTION_STATUS_ABORTED) { 
					FinishPreparedTransaction(x->gid, false);
					elog(ERROR, "Transaction %s (%lu) is aborted by DTM", x->gid, (long)x->xid);
				} else {
					FinishPreparedTransaction(x->gid, true);
				}
			}
		}
		return true;
	}
	return false;
}


/*
 * -------------------------------------------
 * GUC Context Handling
 * -------------------------------------------
 */

// XXX: is it defined somewhere?
#define GUC_KEY_MAXLEN 255
#define MTM_GUC_HASHSIZE 20

typedef struct MtmGucEntry
{
	char	key[GUC_KEY_MAXLEN];
	dlist_node	list_node;
	char   *value;
} MtmGucEntry;

static HTAB *MtmGucHash = NULL;
static dlist_head MtmGucList = DLIST_STATIC_INIT(MtmGucList);

static void MtmGucInit(void)
{
	HASHCTL		hash_ctl;

	MemSet(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = GUC_KEY_MAXLEN;
	hash_ctl.entrysize = sizeof(MtmGucEntry);
	hash_ctl.hcxt = TopMemoryContext;
	MtmGucHash = hash_create("MtmGucHash",
						MTM_GUC_HASHSIZE,
						&hash_ctl,
						HASH_ELEM | HASH_CONTEXT);
}

static void MtmGucDiscard()
{
	dlist_iter iter;

	if (dlist_is_empty(&MtmGucList))
		return;

	dlist_foreach(iter, &MtmGucList)
	{
		MtmGucEntry *cur_entry = dlist_container(MtmGucEntry, list_node, iter.cur);
		pfree(cur_entry->value);
	}
	dlist_init(&MtmGucList);

	hash_destroy(MtmGucHash);
	MtmGucInit();
}

static inline void MtmGucUpdate(const char *key, char *value)
{
	MtmGucEntry *hentry;
	bool found;

	hentry = (MtmGucEntry*)hash_search(MtmGucHash, key, HASH_ENTER, &found);
	if (found)
	{
		pfree(hentry->value);
		dlist_delete(&hentry->list_node);
	}
	hentry->value = value;
	dlist_push_tail(&MtmGucList, &hentry->list_node);
}

static inline void MtmGucRemove(const char *key)
{
	MtmGucEntry *hentry;
	bool found;

	hentry = (MtmGucEntry*)hash_search(MtmGucHash, key, HASH_FIND, &found);
	if (found)
	{
		pfree(hentry->value);
		dlist_delete(&hentry->list_node);
		hash_search(MtmGucHash, key, HASH_REMOVE, NULL);
	}
}

static void MtmGucSet(VariableSetStmt *stmt, const char *queryStr)
{
	MemoryContext oldcontext;

	if (!MtmGucHash)
		MtmGucInit();

	oldcontext = MemoryContextSwitchTo(TopMemoryContext);

	switch (stmt->kind)
	{
		case VAR_SET_VALUE:
			MtmGucUpdate(stmt->name, ExtractSetVariableArgs(stmt));
			break;

		case VAR_SET_DEFAULT:
			MtmGucRemove(stmt->name);
			break;

		case VAR_RESET:
			if (strcmp(stmt->name, "session_authorization") == 0)
				MtmGucRemove("role");
			MtmGucRemove(stmt->name);
			break;

		case VAR_RESET_ALL:
			/* XXX: shouldn't we keep auth/role here? */
			MtmGucDiscard();
			break;

		case VAR_SET_CURRENT:
		case VAR_SET_MULTI:
			break;
	}

	MemoryContextSwitchTo(oldcontext);
}

char* MtmGucSerialize(void)
{
	StringInfo serialized_gucs;
	dlist_iter iter;
	int nvars = 0;

	serialized_gucs = makeStringInfo();

	dlist_foreach(iter, &MtmGucList)
	{
		MtmGucEntry *cur_entry = dlist_container(MtmGucEntry, list_node, iter.cur);

		appendStringInfoString(serialized_gucs, "SET ");
		appendStringInfoString(serialized_gucs, cur_entry->key);
		appendStringInfoString(serialized_gucs, " TO ");

		/* quite a crutch */
		if (strstr(cur_entry->key, "_mem") != NULL)
		{
			appendStringInfoString(serialized_gucs, "'");
			appendStringInfoString(serialized_gucs, cur_entry->value);
			appendStringInfoString(serialized_gucs, "'");
		}
		else
		{
			appendStringInfoString(serialized_gucs, cur_entry->value);
		}
		appendStringInfoString(serialized_gucs, "; ");
		nvars++;
	}

	return serialized_gucs->data;
}

/*
 * -------------------------------------------
 * DDL Handling
 * -------------------------------------------
 */

static void MtmProcessDDLCommand(char const* queryString, bool transactional)
{
	char* gucCtx = MtmGucSerialize();
	if (*gucCtx) {
		queryString = psprintf("RESET SESSION AUTHORIZATION; reset all; %s; %s", gucCtx, queryString);
	} else { 
		queryString = psprintf("RESET SESSION AUTHORIZATION; reset all; %s", queryString);
	}
	MTM_LOG3("Sending utility: %s", queryString);
	if (transactional) {
		/* Transactional DDL */
		LogLogicalMessage("D", queryString, strlen(queryString) + 1, true);
		MtmTx.containsDML = true;
	} else {	
		MTM_LOG1("Execute concurrent DDL: %s", queryString);
		/* Concurrent DDL */
		XLogFlush(LogLogicalMessage("C", queryString, strlen(queryString) + 1, false));
	}
}

static void MtmFinishDDLCommand()
{
	LogLogicalMessage("E", "", 1, true);
}

void MtmUpdateLockGraph(int nodeId, void const* messageBody, int messageSize)
{
	int allocated;
	MtmLockNode(nodeId + MtmMaxNodes, LW_EXCLUSIVE);
	allocated = Mtm->nodes[nodeId-1].lockGraphAllocated;
	if (messageSize > allocated) { 
		allocated = Max(Max(MULTIMASTER_LOCK_BUF_INIT_SIZE, allocated*2), messageSize);
		Mtm->nodes[nodeId-1].lockGraphData = ShmemAlloc(allocated);
		Mtm->nodes[nodeId-1].lockGraphAllocated = allocated;
	}
	memcpy(Mtm->nodes[nodeId-1].lockGraphData, messageBody, messageSize);
	Mtm->nodes[nodeId-1].lockGraphUsed = messageSize;
	MtmUnlockNode(nodeId + MtmMaxNodes);
	MTM_LOG1("Update deadlock graph for node %d size %d", nodeId, messageSize);
}

static void MtmProcessUtility(Node *parsetree, const char *queryString,
							  ProcessUtilityContext context, ParamListInfo params,
							  DestReceiver *dest, char *completionTag)
{
	bool skipCommand = false;
	bool executed = false;

	MTM_LOG3("%d: Process utility statement %s", MyProcPid, queryString);
	switch (nodeTag(parsetree))
	{
		case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;
				switch (stmt->kind)
				{					
				case TRANS_STMT_BEGIN:
				case TRANS_STMT_START:
  				    MtmTx.isTransactionBlock = true;
				    break;
				case TRANS_STMT_COMMIT:
  				    if (MtmTwoPhaseCommit(&MtmTx)) { 
						return;
					}
					break;
				case TRANS_STMT_PREPARE:
  				    MtmTx.isTwoPhase = true;
  				    strcpy(MtmTx.gid, stmt->gid);
					break;
					/* nobreak */
				case TRANS_STMT_COMMIT_PREPARED:
				case TRANS_STMT_ROLLBACK_PREPARED:
				    Assert(!MtmTx.isTwoPhase);
  				    strcpy(MtmTx.gid, stmt->gid);
					break;
				default:
					break;
				}
			}
			/* no break */
		case T_PlannedStmt:
		case T_ClosePortalStmt:
		case T_FetchStmt:
		case T_DoStmt:
		case T_CreateTableSpaceStmt:
		case T_AlterTableSpaceOptionsStmt:
		case T_CommentStmt:
		case T_PrepareStmt:
		case T_ExecuteStmt:
		case T_DeallocateStmt:
		case T_NotifyStmt:
		case T_ListenStmt:
		case T_UnlistenStmt:
		case T_LoadStmt:
		case T_ClusterStmt:
		case T_VariableShowStmt:
		case T_ReassignOwnedStmt:
		case T_LockStmt:
		case T_CheckPointStmt:
		case T_ReindexStmt:
			skipCommand = true;
			break;

		case T_VacuumStmt:
		  skipCommand = true;		  
		  if (context == PROCESS_UTILITY_TOPLEVEL) {			  
			  MtmProcessDDLCommand(queryString, false);
			  MtmTx.isDistributed = false;
		  } else if (MtmApplyContext != NULL) {
			  MemoryContext oldContext = MemoryContextSwitchTo(MtmApplyContext);
			  Assert(oldContext != MtmApplyContext);
			  MtmVacuumStmt = (VacuumStmt*)copyObject(parsetree);
			  MemoryContextSwitchTo(oldContext);
			  return;
		  }
		  break;

		case T_CreateDomainStmt:
			/* Detect temp tables access */
			{
				CreateDomainStmt *stmt = (CreateDomainStmt *) parsetree;
				HeapTuple	typeTup;
				Form_pg_type baseType;
				Form_pg_type elementType;
				Form_pg_class pgClassStruct;
				int32		basetypeMod;
				Oid			elementTypeOid;
				Oid			tableOid;
				HeapTuple pgClassTuple;
				HeapTuple elementTypeTuple;

				typeTup = typenameType(NULL, stmt->typeName, &basetypeMod);
				baseType = (Form_pg_type) GETSTRUCT(typeTup);
				elementTypeOid = baseType->typelem;
				ReleaseSysCache(typeTup);

				if (elementTypeOid == InvalidOid)
					break;

				elementTypeTuple = SearchSysCache1(TYPEOID, elementTypeOid);
				elementType = (Form_pg_type) GETSTRUCT(elementTypeTuple);
				tableOid = elementType->typrelid;
				ReleaseSysCache(elementTypeTuple);

				if (tableOid == InvalidOid)
					break;

				pgClassTuple = SearchSysCache1(RELOID, tableOid);
				pgClassStruct = (Form_pg_class) GETSTRUCT(pgClassTuple);
				if (pgClassStruct->relpersistence == 't')
					MyXactAccessedTempRel = true;
				ReleaseSysCache(pgClassTuple);
			}
			break;

		case T_ExplainStmt:
			/*
			 * EXPLAIN ANALYZE can create side-effects.
			 * Better to catch that by some general mechanism of detecting
			 * catalog and heap writes.
			 */
			{
				ExplainStmt *stmt = (ExplainStmt *) parsetree;
				ListCell   *lc;

				skipCommand = true;
				foreach(lc, stmt->options)
				{
					DefElem    *opt = (DefElem *) lfirst(lc);
					if (strcmp(opt->defname, "analyze") == 0)
						skipCommand = false;
				}
			}
			break;

		/* Save GUC context for consequent DDL execution */
		case T_DiscardStmt:
			{
				DiscardStmt *stmt = (DiscardStmt *) parsetree;

				if (!IsTransactionBlock() && stmt->target == DISCARD_ALL)
				{
					skipCommand = true;
					MtmGucDiscard();
				}
			}
			break;
		case T_VariableSetStmt:
			{
				VariableSetStmt *stmt = (VariableSetStmt *) parsetree;

				/* Prevent SET TRANSACTION from replication */
				if (stmt->kind == VAR_SET_MULTI)
					skipCommand = true;

				if (!IsTransactionBlock())
				{
					skipCommand = true;
					MtmGucSet(stmt, queryString);
				}
			}
			break;

		case T_IndexStmt:
			{
				IndexStmt *indexStmt = (IndexStmt *) parsetree;
				if (indexStmt->concurrent) 
				{
					 if (context == PROCESS_UTILITY_TOPLEVEL) {
						 MtmProcessDDLCommand(queryString, false);
						 MtmTx.isDistributed = false;
						 skipCommand = true;
						 /* 
						  * Index is created at replicas completely asynchronously, so to prevent unintended interleaving with subsequent 
						  * commands in this session, just wait here for a while.
						  * It will help to pass regression tests but will not be enough for construction of real large indexes
						  * where difference between completion of this operation at different nodes is unlimited
						  */
						 MtmSleep(USECS_PER_SEC);
					 } else if (MtmApplyContext != NULL) { 
						 MemoryContext oldContext = MemoryContextSwitchTo(MtmApplyContext);
						 Assert(oldContext != MtmApplyContext);
						 MtmIndexStmt = (IndexStmt*)copyObject(indexStmt);
						 MemoryContextSwitchTo(oldContext);
						 return;
					 }
				}
			}
			break;

		case T_DropStmt:
			{
				DropStmt *stmt = (DropStmt *) parsetree;
				if (stmt->removeType == OBJECT_INDEX && stmt->concurrent)
				{
					if (context == PROCESS_UTILITY_TOPLEVEL) {
						MtmProcessDDLCommand(queryString, false);
						MtmTx.isDistributed = false;
						skipCommand = true;
					} else if (MtmApplyContext != NULL) {
						 MemoryContext oldContext = MemoryContextSwitchTo(MtmApplyContext);
						 Assert(oldContext != MtmApplyContext);
						 MtmDropStmt = (DropStmt*)copyObject(stmt);
						 MemoryContextSwitchTo(oldContext);
						 return;
					}
				}
			}
			break;

		/* Copy need some special care */
	    case T_CopyStmt:
		{
			CopyStmt *copyStatement = (CopyStmt *) parsetree;
			skipCommand = true;
			if (copyStatement->is_from) { 
				RangeVar *relation = copyStatement->relation;
				
				if (relation != NULL)
				{
					Oid relid = RangeVarGetRelid(relation, NoLock, true);
					if (OidIsValid(relid))
					{
						Relation rel = heap_open(relid, ShareLock);
						if (RelationNeedsWAL(rel)) {
							MtmTx.containsDML = true;
						}	
						heap_close(rel, ShareLock);
					}
				}
			}
			break;
		}

	    default:
			skipCommand = false;
			break;
	}

	/* XXX: dirty. Clear on new tx */
	if (!skipCommand && (context == PROCESS_UTILITY_TOPLEVEL || MtmUtilityProcessedInXid != GetCurrentTransactionId()))
		MtmUtilityProcessedInXid = InvalidTransactionId;

	if (!skipCommand && !MtmTx.isReplicated && (MtmUtilityProcessedInXid == InvalidTransactionId)) {
		MtmUtilityProcessedInXid = GetCurrentTransactionId();

		if (context == PROCESS_UTILITY_TOPLEVEL)
			MtmProcessDDLCommand(queryString, true);
		else
			MtmProcessDDLCommand(ActivePortal->sourceText, true);

		executed = true;
	}

	if (PreviousProcessUtilityHook != NULL)
	{
		PreviousProcessUtilityHook(parsetree, queryString, context,
								   params, dest, completionTag);
	}
	else
	{
		standard_ProcessUtility(parsetree, queryString, context,
								params, dest, completionTag);
	}
    if (!MtmVolksWagenMode && MtmTx.isDistributed && XactIsoLevel != XACT_REPEATABLE_READ) { 
		elog(ERROR, "Isolation level %s is not supported by multimaster", isoLevelStr[XactIsoLevel]);
	}

	if (MyXactAccessedTempRel)
	{
		MTM_LOG1("Xact accessed temp table, stopping replication");
		MtmTx.isDistributed = false; /* Skip */
		MtmTx.snapshot = INVALID_CSN;
	}

	if (executed)
	{
		MtmFinishDDLCommand();
	}
}

static void
MtmExecutorStart(QueryDesc *queryDesc, int eflags)
{
	bool ddl_generating_call = false;
	ListCell   *tlist;

	foreach(tlist, queryDesc->plannedstmt->planTree->targetlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tlist);

		if (tle->resname && strcmp(tle->resname, "lo_create") == 0)
		{
			ddl_generating_call = true;
			break;
		}

		if (tle->resname && strcmp(tle->resname, "lo_unlink") == 0)
		{
			ddl_generating_call = true;
			break;
		}
	}

	if (ddl_generating_call && !MtmTx.isReplicated)
		MtmProcessDDLCommand(ActivePortal->sourceText, true);

	if (PreviousExecutorStartHook != NULL)
		PreviousExecutorStartHook(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
}

static void
MtmExecutorFinish(QueryDesc *queryDesc)
{
	/*
	 * If tx didn't wrote to XLOG then there is nothing to commit on other nodes.
	 */
    if (MtmDoReplication) { 
        CmdType operation = queryDesc->operation;
        EState *estate = queryDesc->estate;
        if (estate->es_processed != 0 && (operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE)) { 			
			int i;
			for (i = 0; i < estate->es_num_result_relations; i++) { 
				Relation rel = estate->es_result_relations[i].ri_RelationDesc;
				if (RelationNeedsWAL(rel)) {
					if (MtmIgnoreTablesWithoutPk) {
						if (!rel->rd_indexvalid) {
							RelationGetIndexList(rel);
						}
						if (rel->rd_replidindex == InvalidOid) { 
							MtmMakeRelationLocal(RelationGetRelid(rel));
							continue;
						}
					}
					MTM_LOG3("MtmTx.containsDML = true // WAL");
					MtmTx.containsDML = true;
					break;
				}
			}
        }
    }

    if (PreviousExecutorFinishHook != NULL)
    {
        PreviousExecutorFinishHook(queryDesc);
    }
    else
    {
        standard_ExecutorFinish(queryDesc);
    }
}        

/*
 * -------------------------------------------
 * Executor pool interface
 * -------------------------------------------
 */

void MtmExecute(void* work, int size)
{
	if (Mtm->status == MTM_RECOVERY) { 
		/* During recovery apply changes sequentially to preserve commit order */
		MtmExecutor(work, size);
	} else { 
		BgwPoolExecute(&Mtm->pool, work, size);
	}
}
    
static BgwPool* 
MtmPoolConstructor(void)
{
    return &Mtm->pool;
}

/*
 * -------------------------------------------
 * Deadlock detection
 * -------------------------------------------
 */

static void
MtmGetGtid(TransactionId xid, GlobalTransactionId* gtid)
{
	MtmTransState* ts;

	MtmLock(LW_SHARED);
	ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
	if (ts != NULL) { 
		*gtid = ts->gtid;
	} else { 
		gtid->node = MtmNodeId;
		gtid->xid = xid;
	}
	MtmUnlock();
}


static void 
MtmSerializeLock(PROCLOCK* proclock, void* arg)
{
    ByteBuffer* buf = (ByteBuffer*)arg;
    LOCK* lock = proclock->tag.myLock;
    PGPROC* proc = proclock->tag.myProc; 
	GlobalTransactionId gtid;
    if (lock != NULL) {
        PGXACT* srcPgXact = &ProcGlobal->allPgXact[proc->pgprocno];
        
        if (TransactionIdIsValid(srcPgXact->xid) && proc->waitLock == lock) { 
            LockMethod lockMethodTable = GetLocksMethodTable(lock);
            int numLockModes = lockMethodTable->numLockModes;
            int conflictMask = lockMethodTable->conflictTab[proc->waitLockMode];
            SHM_QUEUE *procLocks = &(lock->procLocks);
            int lm;

			MtmGetGtid(srcPgXact->xid, &gtid);  /* waiting transaction */
			
            ByteBufferAppend(buf, &gtid, sizeof(gtid));

            proclock = (PROCLOCK *) SHMQueueNext(procLocks, procLocks,
                                                 offsetof(PROCLOCK, lockLink));
            while (proclock)
            {
                if (proc != proclock->tag.myProc) { 
                    PGXACT* dstPgXact = &ProcGlobal->allPgXact[proclock->tag.myProc->pgprocno];
                    if (TransactionIdIsValid(dstPgXact->xid)) { 
                        Assert(srcPgXact->xid != dstPgXact->xid);
                        for (lm = 1; lm <= numLockModes; lm++)
                        {
                            if ((proclock->holdMask & LOCKBIT_ON(lm)) && (conflictMask & LOCKBIT_ON(lm)))
                            {
                                MTM_LOG3("%d: %u(%u) waits for %u(%u)", MyProcPid, srcPgXact->xid, proc->pid, dstPgXact->xid, proclock->tag.myProc->pid);
                                MtmGetGtid(dstPgXact->xid, &gtid); /* transaction holding lock */
								ByteBufferAppend(buf, &gtid, sizeof(gtid)); 
                                break;
                            }
                        }
                    }
                }
                proclock = (PROCLOCK *) SHMQueueNext(procLocks, &proclock->lockLink,
                                                     offsetof(PROCLOCK, lockLink));
            }
			gtid.node = 0;
			gtid.xid = 0;
            ByteBufferAppend(buf, &gtid, sizeof(gtid)); /* end of lock owners list */
        }
    }
}

static bool 
MtmDetectGlobalDeadLockForXid(TransactionId xid)
{
	bool hasDeadlock = false;
    if (TransactionIdIsValid(xid)) { 
		ByteBuffer buf;
		MtmGraph graph;
		GlobalTransactionId gtid; 
		int i;
		
        ByteBufferAlloc(&buf);
        EnumerateLocks(MtmSerializeLock, &buf);

		Assert(replorigin_session_origin == InvalidRepOriginId);
		XLogFlush(LogLogicalMessage("L", buf.data, buf.used, false));

		MtmGraphInit(&graph);
		MtmGraphAdd(&graph, (GlobalTransactionId*)buf.data, buf.used/sizeof(GlobalTransactionId));
        ByteBufferFree(&buf);
		for (i = 0; i < Mtm->nAllNodes; i++) { 
			if (i+1 != MtmNodeId && !BIT_CHECK(Mtm->disabledNodeMask, i)) { 
				size_t lockGraphSize;
				void* lockGraphData;
				MtmLockNode(i + 1 + MtmMaxNodes, LW_SHARED);
				lockGraphSize = Mtm->nodes[i].lockGraphUsed;
				lockGraphData = palloc(lockGraphSize);
				memcpy(lockGraphData, Mtm->nodes[i].lockGraphData, lockGraphSize);
				MtmUnlockNode(i + 1 + MtmMaxNodes);

				if (lockGraphData == NULL) {
					return true;
				} else { 
					MtmGraphAdd(&graph, (GlobalTransactionId*)lockGraphData, lockGraphSize/sizeof(GlobalTransactionId));
				}
			}
		}
		MtmGetGtid(xid, &gtid);
		hasDeadlock = MtmGraphFindLoop(&graph, &gtid);
		elog(LOG, "Distributed deadlock check by backend %d for %u:%lu = %d", MyProcPid, gtid.node, (long)gtid.xid, hasDeadlock);
		if (!hasDeadlock) { 
			/* There is no deadlock loop in graph, but deadlock can be caused by lack of apply workers: if all of them are busy, then some transactions
			 * can not be appied just because there are no vacant workers and it cause additional dependency between transactions which is not 
			 * refelected in lock graph 
			 */
			timestamp_t lastPeekTime = BgwGetLastPeekTime(&Mtm->pool);
			if (lastPeekTime != 0 && MtmGetSystemTime() - lastPeekTime >= MSEC_TO_USEC(DeadlockTimeout)) { 
				hasDeadlock = true;
				elog(WARNING, "Apply workers were blocked more than %d msec", 
					 (int)USEC_TO_MSEC(MtmGetSystemTime() - lastPeekTime));
			} else { 
				MTM_LOG1("Enable deadlock timeout in backend %d for transaction %lu", MyProcPid, (long)xid);
				enable_timeout_after(DEADLOCK_TIMEOUT, DeadlockTimeout);
			}
		}
	}
    return hasDeadlock;
}

static bool 
MtmDetectGlobalDeadLock(PGPROC* proc)
{
    PGXACT* pgxact = &ProcGlobal->allPgXact[proc->pgprocno];

	MTM_LOG1("Detect global deadlock for %lu by backend %d", (long)pgxact->xid, MyProcPid);

    return MtmDetectGlobalDeadLockForXid(pgxact->xid);
}

Datum mtm_check_deadlock(PG_FUNCTION_ARGS)
{
	TransactionId xid = PG_GETARG_INT64(0);
    PG_RETURN_BOOL(MtmDetectGlobalDeadLockForXid(xid));
}
