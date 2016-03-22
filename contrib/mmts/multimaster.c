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
#include "utils/tqual.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "postmaster/autovacuum.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "utils/syscache.h"
#include "replication/walsender.h"
#include "replication/walsender_private.h"
#include "replication/slot.h"
#include "port/atomics.h"
#include "tcop/utility.h"
#include "nodes/makefuncs.h"
#include "access/htup_details.h"
#include "catalog/indexing.h"

#include "multimaster.h"
#include "ddd.h"
#include "paxos.h"

typedef struct { 
    TransactionId xid;    /* local transaction ID   */
	GlobalTransactionId gtid; /* global transaction ID assigned by coordinator of transaction */
	bool  isReplicated;   /* transaction on replica */
	bool  isDistributed;  /* transaction performed INSERT/UPDATE/DELETE and has to be replicated to other nodes */
	bool  isPrepared;     /* transaction is perpared at first stage of 2PC */
	bool  containsDML;    /* transaction contains DML statements */
	XidStatus status;     /* transaction status */
    csn_t snapshot;       /* transaction snaphsot */
	csn_t csn;            /* CSN */
	char  gid[MULTIMASTER_MAX_GID_SIZE]; /* global transaction identifier (used by 2pc) */
} MtmCurrentTrans;

typedef struct {
	char gid[MULTIMASTER_MAX_GID_SIZE];
	MtmTransState* state;
} MtmTransMap;

/* #define USE_SPINLOCK 1 */

typedef enum 
{
	MTM_STATE_LOCK_ID
} MtmLockIds;

#define MTM_SHMEM_SIZE (64*1024*1024)
#define MTM_HASH_SIZE  100003
#define MTM_MAP_SIZE   1003
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000
#define STATUS_POLL_DELAY USEC

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mtm_start_replication);
PG_FUNCTION_INFO_V1(mtm_stop_replication);
PG_FUNCTION_INFO_V1(mtm_drop_node);
PG_FUNCTION_INFO_V1(mtm_recover_node);
PG_FUNCTION_INFO_V1(mtm_get_snapshot);
PG_FUNCTION_INFO_V1(mtm_get_nodes_state);
PG_FUNCTION_INFO_V1(mtm_get_cluster_state);

static Snapshot MtmGetSnapshot(Snapshot snapshot);
static void MtmInitialize(void);
static void MtmXactCallback(XactEvent event, void *arg);
static void MtmBeginTransaction(MtmCurrentTrans* x);
static void MtmPrePrepareTransaction(MtmCurrentTrans* x);
static void MtmPostPrepareTransaction(MtmCurrentTrans* x);
static void MtmAbortPreparedTransaction(MtmCurrentTrans* x);
static void MtmEndTransaction(MtmCurrentTrans* x, bool commit);
static TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static bool MtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static TransactionId MtmAdjustOldestXid(TransactionId xid);
static bool MtmDetectGlobalDeadLock(PGPROC* proc);
static void MtmAddSubtransactions(MtmTransState* ts, TransactionId* subxids, int nSubxids);
static char const* MtmGetName(void);
static void MtmCheckClusterLock(void);
static void MtmCheckSlots(void);
static void MtmAddSubtransactions(MtmTransState* ts, TransactionId *subxids, int nSubxids);

static void MtmShmemStartup(void);

static BgwPool* MtmPoolConstructor(void);
static bool MtmRunUtilityStmt(PGconn* conn, char const* sql);
static void MtmBroadcastUtilityStmt(char const* sql, bool ignoreError);

MtmState* Mtm;

HTAB* MtmXid2State;
static HTAB* MtmGid2State;

static MtmCurrentTrans MtmTx;

static TransactionManager MtmTM = { 
	PgTransactionIdGetStatus, 
	PgTransactionIdSetTreeStatus,
	MtmGetSnapshot, 
	PgGetNewTransactionId, 
	MtmGetOldestXmin, 
	PgTransactionIdIsInProgress, 
	PgGetGlobalTransactionId, 
	MtmXidInMVCCSnapshot, 
	MtmDetectGlobalDeadLock, 
	MtmGetName 
};

char const* const MtmNodeStatusMnem[] = 
{ 
	"Intialization", 
	"Offline", 
	"Connected",
	"Online",
	"Recovery"
};

bool  MtmDoReplication;
char* MtmDatabaseName;

int   MtmNodeId;
int   MtmArbiterPort;
int   MtmNodes;
int   MtmConnectAttempts;
int   MtmConnectTimeout;
int   MtmKeepaliveTimeout;
int   MtmReconnectAttempts;

static char* MtmConnStrs;
static int MtmQueueSize;
static int MtmWorkers;
static int MtmMinRecoveryLag;
static int MtmMaxRecoveryLag;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;


static void MtmExecutorFinish(QueryDesc *queryDesc);
static void MtmProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);

/*
 * -------------------------------------------
 * Synchronize access to MTM structures.
 * Using LWLock seems to be  more efficient (at our benchmarks)
 * -------------------------------------------
 */
void MtmLock(LWLockMode mode)
{
#ifdef USE_SPINLOCK
	SpinLockAcquire(&Mtm->spinlock);
#else
	LWLockAcquire((LWLockId)&Mtm->locks[MTM_STATE_LOCK_ID], mode);
#endif
}

void MtmUnlock(void)
{
#ifdef USE_SPINLOCK
	SpinLockRelease(&Mtm->spinlock);
#else
	LWLockRelease((LWLockId)&Mtm->locks[MTM_STATE_LOCK_ID]);
#endif
}

void MtmLockNode(int nodeId)
{
	Assert(nodeId > 0 && nodeId <= MtmNodes);
	LWLockAcquire((LWLockId)&Mtm->locks[nodeId], LW_EXCLUSIVE);
}

void MtmUnlockNode(int nodeId)
{
	Assert(nodeId > 0 && nodeId <= MtmNodes);
	LWLockRelease((LWLockId)&Mtm->locks[nodeId]);	
}

/*
 * -------------------------------------------
 * System time manipulation functions
 * -------------------------------------------
 */


timestamp_t MtmGetCurrentTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (timestamp_t)tv.tv_sec*USEC + tv.tv_usec + Mtm->timeShift;
}

void MtmSleep(timestamp_t interval)
{
    struct timespec ts;
    struct timespec rem;
    ts.tv_sec = interval/1000000;
    ts.tv_nsec = interval%1000000*1000;

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

/*
 * -------------------------------------------
 * Visibility&snapshots
 * -------------------------------------------
 */

csn_t MtmTransactionSnapshot(TransactionId xid)
{
	MtmTransState* ts;
	csn_t snapshot = INVALID_CSN;

	MtmLock(LW_SHARED);
    ts = hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
    if (ts != NULL) { 
		snapshot = ts->snapshot;
	}
	MtmUnlock();

    return snapshot;
}


Snapshot MtmGetSnapshot(Snapshot snapshot)
{
    snapshot = PgGetSnapshotData(snapshot);
	RecentGlobalDataXmin = RecentGlobalXmin = MtmAdjustOldestXid(RecentGlobalDataXmin);
    return snapshot;
}


TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
    TransactionId xmin = PgGetOldestXmin(NULL, ignoreVacuum); /* consider all backends */
    xmin = MtmAdjustOldestXid(xmin);
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
    timestamp_t delay = MIN_WAIT_TIMEOUT;
    Assert(xid != InvalidTransactionId);

	MtmLock(LW_SHARED);

#if TRACE_SLEEP_TIME
    if (firstReportTime == 0) {
        firstReportTime = MtmGetCurrentTime();
    }
#endif
    while (true)
    {
        MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
        if (ts != NULL && ts->status != TRANSACTION_STATUS_IN_PROGRESS)
        {
            if (ts->csn > MtmTx.snapshot) { 
                MTM_TUPLE_TRACE("%d: tuple with xid=%d(csn=%ld) is invisibile in snapshot %ld\n",
								MyProcPid, xid, ts->csn, MtmTx.snapshot);
                MtmUnlock();
                return true;
            }
            if (ts->status == TRANSACTION_STATUS_UNKNOWN)
            {
                MTM_TRACE("%d: wait for in-doubt transaction %u in snapshot %lu\n", MyProcPid, xid, MtmTx.snapshot);
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
                if (now > prevReportTime + USEC*10) { 
                    prevReportTime = now;
                    if (firstReportTime == 0) { 
                        firstReportTime = now;
                    } else { 
                        MTM_TRACE("Snapshot sleep %lu of %lu usec (%f%%), maximum=%lu\n", totalSleepTime, now - firstReportTime, totalSleepTime*100.0/(now - firstReportTime), maxSleepTime);
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
                MTM_TUPLE_TRACE("%d: tuple with xid=%d(csn= %ld) is %s in snapshot %ld\n",
								MyProcPid, xid, ts->csn, invisible ? "rollbacked" : "committed", MtmTx.snapshot);
                MtmUnlock();
                return invisible;
            }
        }
        else
        {
            MTM_TUPLE_TRACE("%d: visibility check is skept for transaction %u in snapshot %lu\n", MyProcPid, xid, MtmTx.snapshot);
            break;
        }
    }
	MtmUnlock();
	return PgXidInMVCCSnapshot(xid, snapshot);
}    



/*
 * There can be different oldest XIDs at different cluster node.
 * We collest oldest CSNs from all nodes and choose minimum from them.
 * If no such XID can be located, then return previously observed oldest XID
 */
static TransactionId 
MtmAdjustOldestXid(TransactionId xid)
{
    if (TransactionIdIsValid(xid)) { 
        MtmTransState *ts, *prev = NULL;
        
		MtmLock(LW_EXCLUSIVE);
        ts = (MtmTransState*)hash_search(MtmXid2State, &xid, HASH_FIND, NULL);
        if (ts != NULL && ts->status == TRANSACTION_STATUS_COMMITTED) {  /* committed transactions have same CSNs at all nodes */
			csn_t oldestSnapshot;
			int i;

			Mtm->nodes[MtmNodeId-1].oldestSnapshot = oldestSnapshot = ts->csn;
			for (i = 0; i < MtmNodes; i++) { 
				if (Mtm->nodes[i].oldestSnapshot < oldestSnapshot) { 
					oldestSnapshot = Mtm->nodes[i].oldestSnapshot;
				}
			}
			for (ts = Mtm->transListHead; ts != NULL && ts->csn < oldestSnapshot; prev = ts, ts = ts->next) { 
				Assert(ts->status == TRANSACTION_STATUS_COMMITTED || ts->status == TRANSACTION_STATUS_ABORTED || ts->status == TRANSACTION_STATUS_IN_PROGRESS);
				if (prev != NULL) { 
					/* Remove information about too old transactions */
					hash_search(MtmXid2State, &prev->xid, HASH_REMOVE, NULL);
				}
			}
        }
        if (prev != NULL) { 
            Mtm->transListHead = prev;
            Mtm->oldestXid = xid = prev->xid;            
        } else { 
            xid = Mtm->oldestXid;
        }
		MtmUnlock();
    }
    return xid;
}

/*
 * -------------------------------------------
 * Transaction list manipulation
 * -------------------------------------------
 */


static void MtmTransactionListAppend(MtmTransState* ts)
{
    ts->next = NULL;
	ts->nSubxids = 0;
    *Mtm->transListTail = ts;
    Mtm->transListTail = &ts->next;
}

static void MtmTransactionListInsertAfter(MtmTransState* after, MtmTransState* ts)
{
    ts->next = after->next;
    after->next = ts;
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
        sts->status = ts->status;
        sts->csn = ts->csn;
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
	  case XACT_EVENT_COMMIT:
		MtmEndTransaction(&MtmTx, true);
		break;
	  case XACT_EVENT_ABORT: 
		MtmEndTransaction(&MtmTx, false);
		break;
	  default:
        break;
	}
}

/* 
 * Check if this is "normal" user trnsaction which shoudl be distributed to other nodes
 */
static bool
MtmIsUserTransaction()
{
	return !IsAutoVacuumLauncherProcess() && IsNormalProcessingMode() && MtmDoReplication && !am_walsender && !IsBackgroundWorker && !IsAutoVacuumWorkerProcess();
}

static void 
MtmResetTransaction(MtmCurrentTrans* x)
{
	x->snapshot = INVALID_CSN;
	x->xid = InvalidTransactionId;
	x->gtid.xid = InvalidTransactionId;
	x->isDistributed = false;
	x->isPrepared = false;
	x->isPrepared = false;
	x->status = TRANSACTION_STATUS_UNKNOWN;
}

static void 
MtmBeginTransaction(MtmCurrentTrans* x)
{
    if (x->snapshot == INVALID_CSN) { 
		MtmLock(LW_EXCLUSIVE);
		x->xid = GetCurrentTransactionIdIfAny();
        x->isReplicated = false;
        x->isDistributed = MtmIsUserTransaction();
		x->isPrepared = false;
		if (x->isDistributed && Mtm->status != MTM_ONLINE) { 
			/* reject all user's transactions at offline cluster */
			MtmUnlock();			
			Assert(Mtm->status == MTM_ONLINE);
			elog(ERROR, "Multimaster node is not online: current status %s", MtmNodeStatusMnem[Mtm->status]);
		}
		x->containsDML = false;
        x->snapshot = MtmAssignCSN();	
		x->gtid.xid = InvalidTransactionId;
		x->gid[0] = '\0';
		x->status = TRANSACTION_STATUS_IN_PROGRESS;
		MtmUnlock();

        MTM_TRACE("%d: MtmLocalTransaction: %s transaction %u uses local snapshot %lu\n", 
				  MyProcPid, x->isDistributed ? "distributed" : "local", x->xid, x->snapshot);
    }
}

/* 
 * Prepare transaction for two-phase commit.
 * This code is executed by PRE_PREPARE hook before PREPARE message is sent to replicas by logical replication
 */
static void
MtmPrePrepareTransaction(MtmCurrentTrans* x)
{ 
	MtmTransState* ts;
	TransactionId *subxids;
	
	if (!x->isDistributed) {
		return;
	}

	x->xid = GetCurrentTransactionId();
	Assert(TransactionIdIsValid(x->xid));

	if (Mtm->disabledNodeMask != 0) { 
		MtmRefreshClusterStatus(true);
		if (Mtm->status != MTM_ONLINE) { 
			elog(ERROR, "Abort current transaction because this cluster node is not online");			
		}
	}

	MtmLock(LW_EXCLUSIVE);

	/*
	 * Check if there is global multimaster lock preventing new transaction from commit to make a chance to wal-senders to catch-up
	 */
	MtmCheckClusterLock();

	ts = hash_search(MtmXid2State, &x->xid, HASH_ENTER, NULL);
	ts->status = TRANSACTION_STATUS_IN_PROGRESS;
	/* 
	 * Invalid CSN prevent replication of transaction by logical replication 
	 */	   
	ts->snapshot = x->isReplicated || !x->containsDML ? INVALID_CSN : x->snapshot;
	ts->csn = MtmAssignCSN();	
	ts->gtid = x->gtid;
	ts->procno = MyProc->pgprocno;
	ts->nVotes = 1; /* I am voted myself */
	ts->votingCompleted = false;
	ts->cmd = MSG_INVALID;
	ts->nSubxids = xactGetCommittedChildren(&subxids);

	x->isPrepared = true;
	x->csn = ts->csn;

	Mtm->transCount += 1;

	if (TransactionIdIsValid(x->gtid.xid)) { 		
		Assert(x->gtid.node != MtmNodeId);
		ts->gtid = x->gtid;
	} else { 
		/* I am coordinator of transaction */
		ts->gtid.xid = x->xid;
		ts->gtid.node = MtmNodeId;
	}
	MtmTransactionListAppend(ts);
	MtmAddSubtransactions(ts, subxids, ts->nSubxids);
	MTM_TRACE("%d: MtmPrePrepareTransaction prepare commit of %d CSN=%ld\n", MyProcPid, x->xid, ts->csn);
	MtmUnlock();

}

static void
MtmPostPrepareTransaction(MtmCurrentTrans* x)
{ 
	MtmTransState* ts;

	MtmLock(LW_EXCLUSIVE);
	ts = hash_search(MtmXid2State, &x->xid, HASH_FIND, NULL);
	Assert(ts != NULL);

	if (!MtmIsCoordinator(ts)) {
		MtmTransMap* tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_ENTER, NULL);
		Assert(x->gid[0]);
		tm->state = ts;
		MtmSendNotificationMessage(ts, MSG_READY); /* send notification to coordinator */
		MtmUnlock();
		MtmResetTransaction(x);
	} else { 
		/* wait votes from all nodes */
		while (!ts->votingCompleted) { 
			MtmUnlock();
			WaitLatch(&MyProc->procLatch, WL_LATCH_SET, -1);
			ResetLatch(&MyProc->procLatch);			
			MtmLock(LW_SHARED);
		}
		x->status = ts->status;
		MTM_TRACE("%d: Result of vote: %d\n", MyProcPid, ts->status);
		MtmUnlock();
	}
}


static void 
MtmAbortPreparedTransaction(MtmCurrentTrans* x)
{
	MtmTransMap* tm;

	if (x->status != TRANSACTION_STATUS_ABORTED) { 
		MtmLock(LW_EXCLUSIVE);
		tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_REMOVE, NULL);
		Assert(tm != NULL);
		tm->state->status = TRANSACTION_STATUS_ABORTED;
		MtmAdjustSubtransactions(tm->state);
		MtmUnlock();
		x->status = TRANSACTION_STATUS_ABORTED;
	}
}

static void 
MtmEndTransaction(MtmCurrentTrans* x, bool commit)
{
	MTM_TRACE("%d: End transaction %d, prepared=%d, replicated=%d, distributed=%d, gid=%s -> %s\n", 
			  MyProcPid, x->xid, x->isPrepared, x->isReplicated, x->isDistributed, x->gid, commit ? "commit" : "abort");
	if (x->status != TRANSACTION_STATUS_ABORTED && x->isDistributed && (x->isPrepared || x->isReplicated)) {
		MtmTransState* ts = NULL;
		MtmLock(LW_EXCLUSIVE);
		if (x->isPrepared) { 
			ts = hash_search(MtmXid2State, &x->xid, HASH_FIND, NULL);
			Assert(ts != NULL);
		} else if (x->gid[0]) { 
			MtmTransMap* tm = (MtmTransMap*)hash_search(MtmGid2State, x->gid, HASH_REMOVE, NULL);
			if (tm != NULL) {
				ts = tm->state;
			} else { 
				MTM_TRACE("%d: GID %s not found\n", MyProcPid, x->gid);
			}
		}
		if (ts != NULL) { 
			if (commit) {
				Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
				ts->status = TRANSACTION_STATUS_COMMITTED;
				if (x->csn > ts->csn) {
					ts->csn = x->csn;
					MtmSyncClock(ts->csn);
				}
			} else { 
				ts->status = TRANSACTION_STATUS_ABORTED;
			}
			MtmAdjustSubtransactions(ts);
		}
		if (!commit && x->isReplicated && TransactionIdIsValid(x->gtid.xid)) { 
			/* 
			 * Send notification only if ABORT happens during transaction processing at replicas, 
			 * do not send notification if ABORT is receiver from master 
			 */
			MTM_TRACE("%d: send ABORT notification to coordinator %d\n", MyProcPid, x->gtid.node);
			if (ts == NULL) { 
				Assert(TransactionIdIsValid(x->xid));
				ts = hash_search(MtmXid2State, &x->xid, HASH_ENTER, NULL);
				ts->status = TRANSACTION_STATUS_ABORTED;
				ts->snapshot = INVALID_CSN;
				ts->csn = MtmAssignCSN();	
				ts->gtid = x->gtid;
				ts->nSubxids = 0;
				ts->cmd = MSG_INVALID;				
				MtmTransactionListAppend(ts);
			}
			MtmSendNotificationMessage(ts, MSG_ABORTED); /* send notification to coordinator */
		}				
		MtmUnlock();
	}
	MtmResetTransaction(x);
	MtmCheckSlots();
}

void MtmSendNotificationMessage(MtmTransState* ts, MtmMessageCode cmd)
{
	MtmTransState* votingList;

	votingList = Mtm->votingTransactions;
	ts->nextVoting = votingList;
	ts->cmd = cmd;
	Mtm->votingTransactions = ts;

	if (votingList == NULL) { 
		/* singal semaphore only once for the whole list */
		PGSemaphoreUnlock(&Mtm->votingSemaphore);
	}
}

void MtmJoinTransaction(GlobalTransactionId* gtid, csn_t globalSnapshot)
{
	MtmLock(LW_EXCLUSIVE);
	MtmSyncClock(globalSnapshot);	
	MtmUnlock();
	
	if (!TransactionIdIsValid(gtid->xid)) { 
		/* In case of recovery InvalidTransactionId is passed */
		Assert(Mtm->status == MTM_RECOVERY);
	} else if (Mtm->status == MTM_RECOVERY) { 
		/* When recovery is completed we get normal transaction ID and switch to normal mode */
		Mtm->recoverySlot = 0;
		MtmSwitchClusterMode(MTM_ONLINE);
	}
	MtmTx.gtid = *gtid;
	MtmTx.xid = GetCurrentTransactionId();
	MtmTx.snapshot = globalSnapshot;	
	MtmTx.isReplicated = true;
	MtmTx.isDistributed = true;
	MtmTx.containsDML = true;
}

void  MtmSetCurrentTransactionGID(char const* gid)
{
	MTM_TRACE("Set current transaction GID %s\n", gid);
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

XidStatus MtmGetGlobalTransactionStatus(char const* gid)
{
	XidStatus status;
	MtmTransMap* tm;

	Assert(gid[0]);
	MtmLock(LW_SHARED);
	tm = (MtmTransMap*)hash_search(MtmGid2State, gid, HASH_FIND, NULL);
	if (tm != NULL) {
		status = tm->state->status;
	} else { 
		status = TRANSACTION_STATUS_ABORTED;
	}
	MtmUnlock();
	return status;
}

void  MtmSetCurrentTransactionCSN(csn_t csn)
{
	MTM_TRACE("Set current transaction CSN %ld\n", csn);
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
 * -------------------------------------------
 * HA functions
 * -------------------------------------------
 */


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
 * It returns true if specified node is in recovery mode. In this case we should send all transactions from WAL, 
 * not only coordinated by self node as in normal mode.
 */
bool MtmIsRecoveredNode(int nodeId)
{
	if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) { 
		Assert(MyWalSnd != NULL); /* This function is called by WAL-sender, so it should not be NULL */
		if (!BIT_CHECK(Mtm->nodeLockerMask, nodeId-1)
			&& MyWalSnd->sentPtr + MtmMinRecoveryLag > GetXLogInsertRecPtr()) 
		{ 
			/*
			 * Wal sender almost catched up.
			 * Lock cluster preventing new transaction to start until wal is completely replayed.
			 * We have to maintain two bitmasks: one is marking wal sender, another - correspondent nodes. 
			 * Is there some better way to establish mapping between nodes ad WAL-seconder?
			 */
			MtmLock(LW_EXCLUSIVE);
			BIT_SET(Mtm->nodeLockerMask, nodeId-1);
			BIT_SET(Mtm->walSenderLockerMask, MyWalSnd - WalSndCtl->walsnds);
			Mtm->nLockers += 1;
			MtmUnlock();
		}
		return true;
	}
	return false;
}

void MtmSwitchClusterMode(MtmNodeStatus mode)
{
	Mtm->status = mode;
	elog(WARNING, "Switch to %s mode", MtmNodeStatusMnem[mode]);
	/* ??? Something else to do here? */
}


/*
 * If there are recovering nodes which are catching-up WAL, check the status and prevent new transaction from commit to give
 * WAL-sender a chance to catch-up WAL, completely synchronize replica and switch it to normal mode.
 * This function is called at transaction start with multimaster lock set
 */
static void 
MtmCheckClusterLock()
{	
	while (true)
	{
		nodemask_t mask = Mtm->walSenderLockerMask;
		if (mask != 0) {
			XLogRecPtr currLogPos = GetXLogInsertRecPtr();
			int i;
			timestamp_t delay = MIN_WAIT_TIMEOUT;
			for (i = 0; mask != 0; i++, mask >>= 1) { 
				if (mask & 1) { 
					if (WalSndCtl->walsnds[i].sentPtr != currLogPos) {
						/* recovery is in progress */
						break;
					} else { 
						/* recovered replica catched up with master */
						elog(WARNING, "WAL-sender %d complete receovery", i);
						BIT_CLEAR(Mtm->walSenderLockerMask, i);
					}
				}
			}
			if (mask != 0) { 
				/* some "almost catch-up" wal-senders are still working */
				/* Do not start new transactions until them complete */
				MtmUnlock();
				MtmSleep(delay);
				if (delay*2 <= MAX_WAIT_TIMEOUT) { 
					delay *= 2;
				}
				MtmLock(LW_EXCLUSIVE);
				continue;
			} else {  
				/* All lockers are synchronized their logs */
				/* Remove lock and mark them as receovered */
				elog(WARNING, "Complete recovery of %d nodes (node mask %lx)", Mtm->nLockers, Mtm->nodeLockerMask);
				Assert(Mtm->walSenderLockerMask == 0);
				Assert((Mtm->nodeLockerMask & Mtm->disabledNodeMask) == Mtm->nodeLockerMask);
				Mtm->disabledNodeMask &= ~Mtm->nodeLockerMask;
				Mtm->nNodes += Mtm->nLockers;
				Mtm->nLockers = 0;
				Mtm->nodeLockerMask = 0;
			}
		}
		break;
	}
}	

/**
 * Build internode connectivity mask. 1 - means that node is disconnected.
 */
static void 
MtmBuildConnectivityMatrix(nodemask_t* matrix, bool nowait)
{
	int i, j, n = MtmNodes;
	for (i = 0; i < n; i++) { 
		if (i+1 != MtmNodeId) { 
			void* data = PaxosGet(psprintf("node-mask-%d", i+1), NULL, NULL, nowait);
			matrix[i] = *(nodemask_t*)data;
		} else { 
			matrix[i] = Mtm->connectivityMask;
		}
	}
	/* make matrix symetric: required for Bron–Kerbosch algorithm */
	for (i = 0; i < n; i++) { 
		for (j = 0; j < i; j++) { 
			matrix[i] |= ((matrix[j] >> i) & 1) << j;
		}
	}
}	


/**
 * Build connectivity graph, find clique in it and extend disabledNodeMask by nodes not included in clique.
 * This function returns false if current node is excluded from cluster, true otherwise
 */
void MtmRefreshClusterStatus(bool nowait)
{
	nodemask_t mask, clique;
	nodemask_t matrix[MAX_NODES];
	int clique_size;
	int i;

	MtmBuildConnectivityMatrix(matrix, nowait);

	clique = MtmFindMaxClique(matrix, MtmNodes, &clique_size);
	if (clique_size >= MtmNodes/2+1) { /* have quorum */
		elog(WARNING, "Find clique %lx, disabledNodeMask %lx", clique, Mtm->disabledNodeMask);
		MtmLock(LW_EXCLUSIVE);
		mask = ~clique & (((nodemask_t)1 << MtmNodes)-1) & ~Mtm->disabledNodeMask; /* new disabled nodes mask */
		for (i = 0; mask != 0; i++, mask >>= 1) {
			if (mask & 1) { 
				Mtm->nNodes -= 1;
				BIT_SET(Mtm->disabledNodeMask, i);
			}
		}
		mask = clique & Mtm->disabledNodeMask; /* new enabled nodes mask */		
		for (i = 0; mask != 0; i++, mask >>= 1) {
			if (mask & 1) { 
				Mtm->nNodes += 1;
				BIT_CLEAR(Mtm->disabledNodeMask, i);
			}
		}
		MtmUnlock();
		if (BIT_CHECK(Mtm->disabledNodeMask, MtmNodeId-1)) { 
			if (Mtm->status == MTM_ONLINE) {
				/* I was excluded from cluster:( */
				MtmSwitchClusterMode(MTM_OFFLINE);
			}
		} else if (Mtm->status == MTM_OFFLINE) {
			/* Should we somehow restart logical receivers? */ 
			MtmSwitchClusterMode(MTM_RECOVERY);
		}
	} else { 
		elog(WARNING, "Clique %lx has no quorum", clique);
	}
}

void MtmOnNodeDisconnect(int nodeId)
{
	BIT_SET(Mtm->connectivityMask, nodeId-1);
	PaxosSet(psprintf("node-mask-%d", MtmNodeId), &Mtm->connectivityMask, sizeof Mtm->connectivityMask, false);

	/* Wait more than socket KEEPALIVE timeout to let other nodes update their statuses */
	MtmSleep(MtmKeepaliveTimeout);

	MtmRefreshClusterStatus(false);
}

void MtmOnNodeConnect(int nodeId)
{
	BIT_CLEAR(Mtm->connectivityMask, nodeId-1);
	PaxosSet(psprintf("node-mask-%d", MtmNodeId), &Mtm->connectivityMask, sizeof Mtm->connectivityMask, false);
}

/*
 * Paxos function stubs (until them are miplemented)
 */
void* PaxosGet(char const* key, int* size, PaxosTimestamp* ts, bool nowait)
{
	if (size != NULL) { 
		*size = 0;
	}
	return NULL;
}

void  PaxosSet(char const* key, void const* value, int size, bool nowait)
{}


/*
 * -------------------------------------------
 * Node initialization
 * -------------------------------------------
 */


static HTAB* 
MtmCreateXidMap(void)
{
	HASHCTL info;
	HTAB* htab;
	Assert(MtmNodes > 0);
	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(MtmTransState) + (MtmNodes-1)*sizeof(TransactionId);
	htab = ShmemInitHash(
		"MtmXid2State",
		MTM_HASH_SIZE, MTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_BLOBS
	);
	return htab;
}

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

static void MtmInitialize()
{
	bool found;
	int i;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	Mtm = (MtmState*)ShmemInitStruct(MULTIMASTER_NAME, sizeof(MtmState) + sizeof(MtmNodeInfo)*(MtmNodes-1), &found);
	if (!found)
	{
		Mtm->status = MTM_INITIALIZATION;
		Mtm->recoverySlot = 0;
		Mtm->locks = GetNamedLWLockTranche(MULTIMASTER_NAME);
		Mtm->csn = MtmGetCurrentTime();
		Mtm->oldestXid = FirstNormalTransactionId;
        Mtm->nNodes = MtmNodes;
		Mtm->disabledNodeMask = 0;
		Mtm->connectivityMask = 0;
		Mtm->pglogicalNodeMask = 0;
		Mtm->walSenderLockerMask = 0;
		Mtm->nodeLockerMask = 0;
		Mtm->nLockers = 0;
		Mtm->votingTransactions = NULL;
        Mtm->transListHead = NULL;
        Mtm->transListTail = &Mtm->transListHead;		
        Mtm->nReceivers = 0;
		Mtm->timeShift = 0;
		Mtm->transCount = 0;
		for (i = 0; i < MtmNodes; i++) {
			Mtm->nodes[i].oldestSnapshot = 0;
			Mtm->nodes[i].transDelay = 0;
		}
		PGSemaphoreCreate(&Mtm->votingSemaphore);
		PGSemaphoreReset(&Mtm->votingSemaphore);
		SpinLockInit(&Mtm->spinlock);
        BgwPoolInit(&Mtm->pool, MtmExecutor, MtmDatabaseName, MtmQueueSize);
		RegisterXactCallback(MtmXactCallback, NULL);
		MtmTx.snapshot = INVALID_CSN;
		MtmTx.xid = InvalidTransactionId;		
	}
	MtmXid2State = MtmCreateXidMap();
	MtmGid2State = MtmCreateGidMap();
    MtmDoReplication = true;
	TM = &MtmTM;
	LWLockRelease(AddinShmemInitLock);
}

static void 
MtmShmemStartup(void)
{
	if (PreviousShmemStartupHook) {
		PreviousShmemStartupHook();
	}
	MtmInitialize();
}

void MtmUpdateNodeConnStr(int nodeId, char const* connStr)
{
	char const* host;
	char const* end;
	int         hostLen;

	if (strlen(connStr) >= MULTIMASTER_MAX_CONN_STR_SIZE) {
		elog(ERROR, "Too long (%d) connection string '%s' for node %d, limit is %d", 
			 (int)strlen(connStr), connStr, nodeId, MULTIMASTER_MAX_CONN_STR_SIZE-1);
	}
	strcpy(Mtm->nodes[nodeId-1].connStr, connStr);

	host = strstr(connStr, "host=");
	if (host == NULL) {
		elog(ERROR, "Host not specified in connection string: '%s'", connStr);
	}
	host += 5;
	for (end = host; *end != ' ' && *end != '\0'; end++);
	hostLen = end - host;
	if (hostLen >= MULTIMASTER_MAX_HOST_NAME_SIZE) {
		elog(ERROR, "Too long (%d) host name '%.*s' for node %d, limit is %d", 
			 hostLen, hostLen, host, nodeId, MULTIMASTER_MAX_HOST_NAME_SIZE-1);
	}
	memcpy(Mtm->nodes[nodeId-1].hostName, host, hostLen);
	Mtm->nodes[nodeId-1].hostName[hostLen] = '\0';
}

static void MtmSplitConnStrs(void)
{
	int i;
    char* connStr = strdup(MtmConnStrs);
    char* connStrEnd = connStr + strlen(connStr);

	for (i = 0; connStr < connStrEnd; i++) { 
        char* p = strchr(connStr, ',');
        if (p == NULL) { 
            p = connStrEnd;
        }
		if (i == MAX_NODES) { 
			elog(ERROR, "Multimaster with more than %d nodes is not currently supported", MAX_NODES);
		}	
		*p = '\0';
		MtmUpdateNodeConnStr(i+1, connStr);
		if (i+1 == MtmNodeId) { 
			char* dbName = strstr(connStr, "dbname=");
			char* end;
			size_t len;
			if (dbName == NULL) { 
				elog(ERROR, "Database not specified in connection string: '%s'", connStr);
			}
			dbName += 7;
			for (end = dbName; *end != ' ' && *end != '\0'; end++);
			len = end - dbName;
			MtmDatabaseName = (char*)malloc(len + 1);
			memcpy(MtmDatabaseName, dbName, len);
			MtmDatabaseName[len] = '\0';
		}
		connStr = p + 1;
    }
	free(connStr);
	if (i < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
	}	
	MtmNodes = i;
	if (MtmNodeId > MtmNodes) {
		elog(ERROR, "Invalid node id %d for specified nubmer of nodes %d", MtmNodeId, MtmNodes);
	}
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
		"multimaster.min_recovery_lag",
		"Minamal lag of WAL-sender performing recovery after which cluster is locked until recovery is completed",
		"When wal-sender almost catch-up WAL current position we need to stop 'Achilles tortile compeition' and "
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
		"Dropping slog makes it not possible to recover node using logical replication mechanism, it will be ncessary to completely copy content of some other nodes " 
		"usimg basebackup or similar tool. Zero value of parameter disable droipping slot.",
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

	DefineCustomIntVariable(
		"multimaster.workers",
		"Number of multimaster executor workers per node",
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
		54321,
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
		PGC_BACKEND, // context
		0, // flags,
		NULL, // GucStringCheckHook check_hook,
		NULL, // GucStringAssignHook assign_hook,
		NULL // GucShowHook show_hook
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
		"Interval in microseconds between connection attempts",
		&MtmConnectTimeout,
		1000000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.keepalive_timeout",
		"Multimaster keepalive interval for sockets",
		"Timeout in microseconds before polling state of nodes",
		&MtmKeepaliveTimeout,
		1000000,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.connect_attempts",
		"Multimaster number of connect attemts",
		"Maximal number of attempt to establish connection with other node after which multimaster is give up",
		&MtmConnectAttempts,
		10,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.reconnect_attempts",
		"Multimaster number of reconnect attemts",
		"Maximal number of attempt to reestablish connection with other node after which node is considered to be offline",
		&MtmReconnectAttempts,
		10,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	MtmSplitConnStrs();
    MtmStartReceivers();
		
	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in mtm_shmem_startup().
	 */
	RequestAddinShmemSpace(MTM_SHMEM_SIZE + MtmQueueSize);
	RequestNamedLWLockTranche(MULTIMASTER_NAME, 1 + MtmNodes);

    BgwPoolStart(MtmWorkers, MtmPoolConstructor);

	MtmArbiterInitialize();

	/*
	 * Install hooks.
	 */
	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = MtmShmemStartup;

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


 
void MtmReceiverStarted(int nodeId)
{
	SpinLockAcquire(&Mtm->spinlock);	
	if (!BIT_CHECK(Mtm->pglogicalNodeMask, nodeId-1)) { 
		BIT_SET(Mtm->pglogicalNodeMask, nodeId-1);
		if (++Mtm->nReceivers == Mtm->nNodes-1) {
			Assert(Mtm->status == MTM_CONNECTED);
			MtmSwitchClusterMode(MTM_ONLINE);
		}
     }
	SpinLockRelease(&Mtm->spinlock);	
}

/* 
 * Determine when and how we should open replication slot.
 * Druing recovery we need to open only one replication slot from which node should receive all transactions.
 * Slots at other nodes should be removed 
 */
MtmSlotMode MtmReceiverSlotMode(int nodeId)
{
	while (Mtm->status != MTM_CONNECTED && Mtm->status != MTM_ONLINE) { 		
		if (Mtm->status == MTM_RECOVERY) { 
			if (Mtm->recoverySlot == 0 || Mtm->recoverySlot == nodeId) { 
				/* Choose for recovery first available slot */
				Mtm->recoverySlot = nodeId;
				return SLOT_OPEN_EXISTED;
			}
		}
		/* delay opening of other slots until recovery is completed */
		MtmSleep(STATUS_POLL_DELAY);
	}
	/* After recovery completion we need to drop all other slots to avoid receive of redundant data */
	return Mtm->recoverySlot ? SLOT_CREATE_NEW : SLOT_OPEN_ALWAYS;
}
			
void MtmRecoverNode(int nodeId)
{
	if (nodeId <= 0 || nodeId > Mtm->nNodes) 
	{ 
		elog(ERROR, "NodeID %d is out of range [1,%d]", nodeId, Mtm->nNodes);
	}
	if (!BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) { 
		elog(ERROR, "Node %d was not disabled", nodeId);
	}
	if (!IsTransactionBlock())
	{
		MtmBroadcastUtilityStmt(psprintf("select pg_create_logical_replication_slot('" MULTIMASTER_SLOT_PATTERN "', '" MULTIMASTER_NAME "')", nodeId), true);
	}
}
	
	
void MtmDropNode(int nodeId, bool dropSlot)
{
	if (!BIT_CHECK(Mtm->disabledNodeMask, nodeId-1))
	{
		if (nodeId <= 0 || nodeId > Mtm->nNodes) 
		{ 
			elog(ERROR, "NodeID %d is out of range [1,%d]", nodeId, Mtm->nNodes);
		}
		BIT_SET(Mtm->disabledNodeMask, nodeId-1);
		Mtm->nNodes -= 1;
		if (!IsTransactionBlock())
		{
			MtmBroadcastUtilityStmt(psprintf("select mtm.drop_node(%d,%s)", nodeId, dropSlot ? "true" : "false"), true);
		}
		if (dropSlot) 
		{
			ReplicationSlotDrop(psprintf(MULTIMASTER_SLOT_PATTERN, nodeId));
		}		
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

typedef struct
{
	int       nodeId;
	char*     connStrPtr;
	TupleDesc desc;
    Datum     values[7];
    bool      nulls[7];
} MtmGetNodeStateCtx;

Datum
mtm_get_nodes_state(PG_FUNCTION_ARGS)
{
    FuncCallContext* funcctx;
	MtmGetNodeStateCtx* usrfctx;
	MemoryContext oldcontext;
	char* p;
	int64 lag;
    bool is_first_call = SRF_IS_FIRSTCALL();

    if (is_first_call) { 
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);       
		usrfctx = (MtmGetNodeStateCtx*)palloc(sizeof(MtmGetNodeStateCtx));
		get_call_result_type(fcinfo, NULL, &usrfctx->desc);
		usrfctx->nodeId = 1;
		usrfctx->connStrPtr = pstrdup(MtmConnStrs);
		memset(usrfctx->nulls, false, sizeof(usrfctx->nulls));
		funcctx->user_fctx = usrfctx;
		MemoryContextSwitchTo(oldcontext);      
    }
    funcctx = SRF_PERCALL_SETUP();	
	usrfctx = (MtmGetNodeStateCtx*)funcctx->user_fctx;
	if (usrfctx->nodeId > MtmNodes) {
		SRF_RETURN_DONE(funcctx);      
	}
	usrfctx->values[0] = Int32GetDatum(usrfctx->nodeId);
	usrfctx->values[1] = BoolGetDatum(BIT_CHECK(Mtm->disabledNodeMask, usrfctx->nodeId-1));
	usrfctx->values[2] = BoolGetDatum(BIT_CHECK(Mtm->connectivityMask, usrfctx->nodeId-1));
	usrfctx->values[3] = BoolGetDatum(BIT_CHECK(Mtm->nodeLockerMask, usrfctx->nodeId-1));
	lag = MtmGetSlotLag(usrfctx->nodeId);
	usrfctx->values[4] = Int64GetDatum(lag);
	usrfctx->nulls[4] = lag < 0;
	usrfctx->values[5] = Int64GetDatum(Mtm->transCount ? Mtm->nodes[usrfctx->nodeId-1].transDelay/Mtm->transCount : 0);
	p = strchr(usrfctx->connStrPtr, ',');
	if (p != NULL) { 
		*p++ = '\0';
	}
	usrfctx->values[6] = CStringGetTextDatum(usrfctx->connStrPtr);
	usrfctx->connStrPtr = p;
	usrfctx->nodeId += 1;

	SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(heap_form_tuple(usrfctx->desc, usrfctx->values, usrfctx->nulls)));
}

Datum
mtm_get_cluster_state(PG_FUNCTION_ARGS)
{
	TupleDesc desc;
    Datum     values[10];
    bool      nulls[10] = {false};
	get_call_result_type(fcinfo, NULL, &desc);

	values[0] = CStringGetTextDatum(MtmNodeStatusMnem[Mtm->status]);
	values[1] = Int64GetDatum(Mtm->disabledNodeMask);
	values[2] = Int64GetDatum(Mtm->connectivityMask);
	values[3] = Int64GetDatum(Mtm->nodeLockerMask);
	values[4] = Int32GetDatum(Mtm->nNodes);
	values[5] = Int32GetDatum((int)Mtm->pool.active);
	values[6] = Int64GetDatum(BgwPoolGetQueueSize(&Mtm->pool));
	values[7] = Int64GetDatum(Mtm->transCount);
	values[8] = Int64GetDatum(Mtm->timeShift);
	values[9] = Int32GetDatum(Mtm->recoverySlot);
	nulls[9] = Mtm->recoverySlot == 0;

	PG_RETURN_DATUM(HeapTupleGetDatum(heap_form_tuple(desc, values, nulls)));
}

/*
 * -------------------------------------------
 * Broadcast utulity statements
 * -------------------------------------------
 */

/*
 * Execute statement with specified parameters and check its result
 */
static bool MtmRunUtilityStmt(PGconn* conn, char const* sql)
{
	PGresult *result = PQexec(conn, sql);
	int status = PQresultStatus(result);
	bool ret = status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
	if (!ret) { 
		elog(WARNING, "Command '%s' failed with status %d", sql, status);
	}
	PQclear(result);
	return ret;
}

static void MtmBroadcastUtilityStmt(char const* sql, bool ignoreError)
{
	char* conn_str = pstrdup(MtmConnStrs);
	char* conn_str_end = conn_str + strlen(conn_str);
	int i = 0;
	nodemask_t disabledNodeMask = Mtm->disabledNodeMask;
	int failedNode = -1;
	char const* errorMsg = NULL;
	PGconn **conns = palloc0(sizeof(PGconn*)*MtmNodes);
    
	while (conn_str < conn_str_end) 
	{ 
		char* p = strchr(conn_str, ',');
		if (p == NULL) { 
			p = conn_str_end;
		}
		*p = '\0';
		if (!BIT_CHECK(disabledNodeMask, i)) 
		{
			conns[i] = PQconnectdb(conn_str);
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
					elog(ERROR, "Failed to establish connection '%s' to node %d", conn_str, failedNode);
				}
			}
		}
		conn_str = p + 1;
		i += 1;
	}
	Assert(i == MtmNodes);
    
	for (i = 0; i < MtmNodes; i++) 
	{ 
		if (conns[i]) 
		{
			if (!MtmRunUtilityStmt(conns[i], "BEGIN TRANSACTION") && !ignoreError)
			{
				errorMsg = "Failed to start transaction at node %d";
				failedNode = i;
				break;
			}
			if (!MtmRunUtilityStmt(conns[i], sql) && !ignoreError)
			{
				errorMsg = "Failed to run command at node %d";
				failedNode = i;
				break;
			}
		}
	}
	if (failedNode >= 0 && !ignoreError)  
	{
		for (i = 0; i < MtmNodes; i++) 
		{ 
			if (conns[i])
			{
				MtmRunUtilityStmt(conns[i], "ROLLBACK TRANSACTION");
			}
		}
	} else { 
		for (i = 0; i < MtmNodes; i++) 
		{ 
			if (conns[i] && !MtmRunUtilityStmt(conns[i], "COMMIT TRANSACTION") && !ignoreError) 
			{ 
				errorMsg = "Commit failed at node %d";
				failedNode = i;
			}
		}
	}                       
	for (i = 0; i < MtmNodes; i++) 
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

static bool MtmProcessDDLCommand(char const* queryString)
{
	RangeVar   *rv;
	Relation	rel;
	TupleDesc	tupDesc;
	HeapTuple	tup;
	Datum		values[Natts_mtm_ddl_log];
	bool		nulls[Natts_mtm_ddl_log];
	TimestampTz ts = GetCurrentTimestamp();

	rv = makeRangeVar(MULTIMASTER_SCHEMA_NAME, MULTIMASTER_DDL_TABLE, -1);
	rel = heap_openrv_extended(rv, RowExclusiveLock, true);

	if (rel == NULL) {
		if (!IsTransactionBlock()) {
			MtmBroadcastUtilityStmt(queryString, false);
			return true;
		}
		return false;
	}
		
	tupDesc = RelationGetDescr(rel);

	/* Form a tuple. */
	memset(nulls, false, sizeof(nulls));

	values[Anum_mtm_ddl_log_issued - 1] = TimestampTzGetDatum(ts);
	values[Anum_mtm_ddl_log_query - 1] = CStringGetTextDatum(queryString);

	tup = heap_form_tuple(tupDesc, values, nulls);

	/* Insert the tuple to the catalog. */
	simple_heap_insert(rel, tup);

	/* Update the indexes. */
	CatalogUpdateIndexes(rel, tup);

	/* Cleanup. */
	heap_freetuple(tup);
	heap_close(rel, RowExclusiveLock);

	MtmTx.containsDML = true;
	return false;
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

static void MtmTwoPhaseCommit(char *completionTag)
{
	MtmGenerateGid(MtmTx.gid);
	if (!IsTransactionBlock()) { 
		elog(WARNING, "Start transaction block for %d", MtmTx.xid);
		BeginTransactionBlock();
		CommitTransactionCommand();
		StartTransactionCommand();
	}
	if (!PrepareTransactionBlock(MtmTx.gid))
	{
		elog(WARNING, "Failed to prepare transaction %s", MtmTx.gid);
		/* report unsuccessful commit in completionTag */
		if (completionTag) { 
			strcpy(completionTag, "ROLLBACK");
		}
		/* ??? Should we do explicit rollback */
	} else { 
		CommitTransactionCommand();
		StartTransactionCommand();
		if (MtmGetCurrentTransactionStatus() == TRANSACTION_STATUS_ABORTED) { 
			FinishPreparedTransaction(MtmTx.gid, false);
			elog(ERROR, "Transaction %s is aborted by DTM", MtmTx.gid);
		} else {
			FinishPreparedTransaction(MtmTx.gid, true);
		}
	}
}

static void MtmProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag)
{
	bool skipCommand;
	MTM_TRACE("%d: Process utility statement %s\n", MyProcPid, queryString);
	switch (nodeTag(parsetree))
	{
	    case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;
				switch (stmt->kind)
				{					
				case TRANS_STMT_COMMIT:
					if (MtmTx.isDistributed && MtmTx.containsDML) {
						MtmTwoPhaseCommit(completionTag);
						return;
					}
					break;
				case TRANS_STMT_PREPARE:
				case TRANS_STMT_COMMIT_PREPARED:
				case TRANS_STMT_ROLLBACK_PREPARED:
					elog(ERROR, "Two phase commit is not supported by multimaster");
				default:
					break;
				}
			}
			/* no break */
		case T_PlannedStmt:
		case T_ClosePortalStmt:
		case T_FetchStmt:
		case T_DoStmt:
		case T_CopyStmt:
		case T_PrepareStmt:
		case T_ExecuteStmt:
		case T_NotifyStmt:
		case T_ListenStmt:
		case T_UnlistenStmt:
		case T_LoadStmt:
		case T_VariableSetStmt:
		case T_VariableShowStmt:
			skipCommand = true;
			break;
	    default:
			skipCommand = false;
			break;
	}
	if (!skipCommand && !MtmTx.isReplicated && context == PROCESS_UTILITY_TOPLEVEL) {
		if (MtmProcessDDLCommand(queryString)) { 
			return;
		}
		if (MtmTx.isDistributed && MtmTx.containsDML && !IsTransactionBlock()) { 
			MtmTwoPhaseCommit(completionTag);
		}
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
}


static void
MtmExecutorFinish(QueryDesc *queryDesc)
{
    if (MtmDoReplication) { 
        CmdType operation = queryDesc->operation;
        EState *estate = queryDesc->estate;
        if (estate->es_processed != 0 && (operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE)) { 			
			int i;
			for (i = 0; i < estate->es_num_result_relations; i++) { 
				if (RelationNeedsWAL(estate->es_result_relations[i].ri_RelationDesc)) {
					MtmTx.containsDML = true;
					break;
				}
			}
        }
		if (MtmTx.isDistributed && MtmTx.containsDML && !IsTransactionBlock()) { 
			MtmTwoPhaseCommit(NULL);
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
    BgwPoolExecute(&Mtm->pool, work, size);
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
			
            ByteBufferAppendInt32(buf, gtid.node);
            ByteBufferAppendInt32(buf, gtid.xid); 

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
                                MTM_TRACE("%d: %u(%u) waits for %u(%u)\n", MyProcPid, srcPgXact->xid, proc->pid, dstPgXact->xid, proclock->tag.myProc->pid);
                                MtmGetGtid(dstPgXact->xid, &gtid); /* transaction holding lock */
								ByteBufferAppendInt32(buf, gtid.node); 
								ByteBufferAppendInt32(buf, gtid.xid); 
                                break;
                            }
                        }
                    }
                }
                proclock = (PROCLOCK *) SHMQueueNext(procLocks, &proclock->lockLink,
                                                     offsetof(PROCLOCK, lockLink));
            }
            ByteBufferAppendInt32(buf, 0); /* end of lock owners list */
            ByteBufferAppendInt32(buf, 0); /* end of lock owners list */
        }
    }
}

static bool 
MtmDetectGlobalDeadLock(PGPROC* proc)
{
    ByteBuffer buf;
    PGXACT* pgxact = &ProcGlobal->allPgXact[proc->pgprocno];
	bool hasDeadlock = false;
    if (TransactionIdIsValid(pgxact->xid)) { 
		MtmGraph graph;
		GlobalTransactionId gtid; 
		int i;
		
        ByteBufferAlloc(&buf);
        EnumerateLocks(MtmSerializeLock, &buf);
		PaxosSet(psprintf("lock-graph-%d", MtmNodeId), buf.data, buf.used, true);
		MtmGraphInit(&graph);
		MtmGraphAdd(&graph, (GlobalTransactionId*)buf.data, buf.used/sizeof(GlobalTransactionId));
        ByteBufferFree(&buf);
		for (i = 0; i < MtmNodes; i++) { 
			if (i+1 != MtmNodeId && !BIT_CHECK(Mtm->disabledNodeMask, i)) { 
				int size;
				void* data = PaxosGet(psprintf("lock-graph-%d", i+1), &size, NULL, true);
				if (data == NULL) { 
					return true; /* Just temporary hack until no Paxos */
				} else { 
					MtmGraphAdd(&graph, (GlobalTransactionId*)data, size/sizeof(GlobalTransactionId));
				}
			}
		}
		MtmGetGtid(pgxact->xid, &gtid);
		hasDeadlock = MtmGraphFindLoop(&graph, &gtid);
		elog(WARNING, "Distributed deadlock check for %u:%u = %d", gtid.node, gtid.xid, hasDeadlock);
	}
    return hasDeadlock;
}
