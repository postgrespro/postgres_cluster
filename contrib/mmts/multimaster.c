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
#include "replication/slot.h"
#include "port/atomics.h"
#include "tcop/utility.h"

#include "multimaster.h"

typedef struct { 
    TransactionId xid;    /* local transaction ID   */
	GlobalTransactionId gtid; /* global transaction ID assigned by coordinator of transaction */
	bool  isReplicated;   /* transaction on replica */
	bool  isDistributed;  /* transaction performed INSERT/UPDATE/DELETE and has to be replicated to other nodes */
	bool  containsDML;    /* transaction contains DML statements */
    csn_t snapshot;       /* transaction snaphsot   */
} MtmCurrentTrans;

/* #define USE_SPINLOCK 1 */

typedef uint64 timestamp_t;

#define MTM_SHMEM_SIZE (64*1024*1024)
#define MTM_HASH_SIZE  100003
#define USEC 1000000
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mtm_start_replication);
PG_FUNCTION_INFO_V1(mtm_stop_replication);
PG_FUNCTION_INFO_V1(mtm_drop_node);
PG_FUNCTION_INFO_V1(mtm_get_snapshot);

static Snapshot MtmGetSnapshot(Snapshot snapshot);
static void MtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static void MtmInitialize(void);
static void MtmXactCallback(XactEvent event, void *arg);
static void MtmBeginTransaction(MtmCurrentTrans* x);
static void MtmPrepareTransaction(MtmCurrentTrans* x);
static void MtmEndTransaction(MtmCurrentTrans* x, bool commit);
static TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static bool MtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static TransactionId MtmAdjustOldestXid(TransactionId xid);
static bool MtmDetectGlobalDeadLock(PGPROC* proc);
static void MtmAddSubtransactions(MtmTransState* ts, TransactionId* subxids, int nSubxids);
static char const* MtmGetName(void);

static void MtmShmemStartup(void);

static BgwPool* MtmPoolConstructor(void);
static bool MtmRunUtilityStmt(PGconn* conn, char const* sql);
static void MtmBroadcastUtilityStmt(char const* sql, bool ignoreError);
static void MtmVoteForTransaction(MtmTransState* ts);

static HTAB* xid2state;
static MtmCurrentTrans dtmTx;
static MtmState* dtm;

static TransactionManager MtmTM = { 
	PgTransactionIdGetStatus, 
	MtmSetTransactionStatus, 
	MtmGetSnapshot, 
	PgGetNewTransactionId, 
	MtmGetOldestXmin, 
	PgTransactionIdIsInProgress, 
	PgGetGlobalTransactionId, 
	MtmXidInMVCCSnapshot, 
	MtmDetectGlobalDeadLock, 
	MtmGetName 
};

bool  MtmDoReplication;
char* MtmDatabaseName;

char* MtmConnStrs;
int   MtmNodeId;
int   MtmArbiterPort;
int   MtmNodes;

static int MtmQueueSize;
static int MtmWorkers;
static int MtmVacuumDelay;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;


static void MtmExecutorFinish(QueryDesc *queryDesc);
static void MtmProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);

void MtmLock(LWLockMode mode)
{
#ifdef USE_SPINLOCK
	SpinLockAcquire(&dtm->hashSpinlock);
#else
	LWLockAcquire(dtm->hashLock, mode);
#endif
}

void MtmUnlock(void)
{
#ifdef USE_SPINLOCK
	SpinLockRelease(&dtm->hashSpinlock);
#else
	LWLockRelease(dtm->hashLock);
#endif
}


/*
 *  System time manipulation functions
 */

static timestamp_t MtmGetCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (timestamp_t)tv.tv_sec*USEC + tv.tv_usec + dtm->timeShift;
}

static void MtmSleep(timestamp_t interval)
{
    struct timespec ts;
    struct timespec rem;
    ts.tv_sec = 0;
    ts.tv_nsec = interval*1000;

    while (nanosleep(&ts, &rem) < 0) { 
        Assert(errno == EINTR);
        ts = rem;
    }
}
    
csn_t MtmAssignCSN()
{
    csn_t csn = MtmGetCurrentTime();
    if (csn <= dtm->csn) { 
        csn = ++dtm->csn;
    } else { 
        dtm->csn = csn;
    }
    return csn;
}

csn_t MtmSyncClock(csn_t global_csn)
{
    csn_t local_csn;
    while ((local_csn = MtmAssignCSN()) < global_csn) { 
        dtm->timeShift += global_csn - local_csn;
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

Snapshot MtmGetSnapshot(Snapshot snapshot)
{
    snapshot = PgGetSnapshotData(snapshot);
	RecentGlobalDataXmin = RecentGlobalXmin = MtmAdjustOldestXid(RecentGlobalDataXmin);
    return snapshot;
}


TransactionId MtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
    TransactionId xmin = PgGetOldestXmin(rel, ignoreVacuum);
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
        MtmTransState* ts = (MtmTransState*)hash_search(xid2state, &xid, HASH_FIND, NULL);
        if (ts != NULL && ts->status != TRANSACTION_STATUS_IN_PROGRESS)
        {
            if (ts->csn > dtmTx.snapshot) { 
                MTM_TUPLE_TRACE("%d: tuple with xid=%d(csn=%ld) is invisibile in snapshot %ld\n",
								getpid(), xid, ts->csn, dtmTx.snapshot);
                MtmUnlock();
                return true;
            }
            if (ts->status == TRANSACTION_STATUS_UNKNOWN)
            {
                MTM_TRACE("%d: wait for in-doubt transaction %u in snapshot %lu\n", getpid(), xid, dtmTx.snapshot);
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
								getpid(), xid, ts->csn, invisible ? "rollbacked" : "committed", dtmTx.snapshot);
                MtmUnlock();
                return invisible;
            }
        }
        else
        {
            MTM_TUPLE_TRACE("%d: visibility check is skept for transaction %u in snapshot %lu\n", getpid(), xid, dtmTx.snapshot);
            break;
        }
    }
	MtmUnlock();
	return PgXidInMVCCSnapshot(xid, snapshot);
}    

static uint32 MtmXidHashFunc(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int MtmXidMatchFunc(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 - *(TransactionId*)key2;
}

static void MtmTransactionListAppend(MtmTransState* ts)
{
    ts->next = NULL;
	ts->nSubxids = 0;
    *dtm->transListTail = ts;
    dtm->transListTail = &ts->next;
}

static void MtmTransactionListInsertAfter(MtmTransState* after, MtmTransState* ts)
{
    ts->next = after->next;
    after->next = ts;
    if (dtm->transListTail == &after->next) { 
        dtm->transListTail = &ts->next;
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
        sts = (MtmTransState*)hash_search(xid2state, &subxids[i], HASH_ENTER, &found);
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
 * There can be different oldest XIDs at different cluster node.
 * Seince we do not have centralized aribiter, we have to rely in MtmVacuumDelay.
 * This function takes XID which PostgreSQL consider to be the latest and try to find XID which
 * is older than it more than MtmVacuumDelay.
 * If no such XID can be located, then return previously observed oldest XID
 */
static TransactionId 
MtmAdjustOldestXid(TransactionId xid)
{
    if (TransactionIdIsValid(xid)) { 
        MtmTransState *ts, *prev = NULL;
        
		MtmLock(LW_EXCLUSIVE);
        ts = (MtmTransState*)hash_search(xid2state, &xid, HASH_FIND, NULL);
        if (ts != NULL) { 
            timestamp_t cutoff_time = ts->csn - MtmVacuumDelay*USEC;
			for (ts = dtm->transListHead; ts != NULL && ts->csn < cutoff_time; prev = ts, ts = ts->next) { 
				Assert(ts->status == TRANSACTION_STATUS_COMMITTED || ts->status == TRANSACTION_STATUS_ABORTED || ts->status == TRANSACTION_STATUS_IN_PROGRESS);
				if (prev != NULL) { 
					/* Remove information about too old transactions */
					hash_search(xid2state, &prev->xid, HASH_REMOVE, NULL);
				}
			}
        }
        if (prev != NULL) { 
            dtm->transListHead = prev;
            dtm->oldestXid = xid = prev->xid;            
        } else { 
            xid = dtm->oldestXid;
        }
		MtmUnlock();
    }
    return xid;
}

static void MtmInitialize()
{
	bool found;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	dtm = (MtmState*)ShmemInitStruct(MULTIMASTER_NAME, sizeof(MtmState), &found);
	if (!found)
	{
		dtm->hashLock = (LWLock*)GetNamedLWLockTranche(MULTIMASTER_NAME);
		dtm->csn = MtmGetCurrentTime();
		dtm->oldestXid = FirstNormalTransactionId;
        dtm->nNodes = MtmNodes;
		dtm->disabledNodeMask = 0;
		dtm->votingTransactions = NULL;
        dtm->transListHead = NULL;
        dtm->transListTail = &dtm->transListHead;
        pg_atomic_write_u32(&dtm->nReceivers, 0);
		dtm->timeShift = 0;
        dtm->initialized = false;
		PGSemaphoreCreate(&dtm->votingSemaphore);
		PGSemaphoreReset(&dtm->votingSemaphore);
		SpinLockInit(&dtm->hashSpinlock);
        BgwPoolInit(&dtm->pool, MtmExecutor, MtmDatabaseName, MtmQueueSize);
		RegisterXactCallback(MtmXactCallback, NULL);
		dtmTx.snapshot = INVALID_CSN;
		dtmTx.xid = InvalidTransactionId;
	}
	xid2state = MtmCreateHash();
    MtmDoReplication = true;
	TM = &MtmTM;
	LWLockRelease(AddinShmemInitLock);
}

static void
MtmXactCallback(XactEvent event, void *arg)
{
    switch (event) 
    {
	  case XACT_EVENT_START: 
	    MtmBeginTransaction(&dtmTx);
        break;
	  case XACT_EVENT_PRE_COMMIT:
		MtmPrepareTransaction(&dtmTx);
		break;
	  case XACT_EVENT_COMMIT:
		MtmEndTransaction(&dtmTx, true);
		break;
	  case XACT_EVENT_ABORT: 
		MtmEndTransaction(&dtmTx, false);
		break;
	  default:
        break;
	}
}

static void 
MtmBeginTransaction(MtmCurrentTrans* x)
{
    if (x->snapshot == INVALID_CSN) { 
		MtmLock(LW_EXCLUSIVE);
		x->xid = GetCurrentTransactionIdIfAny();
        x->isReplicated = false;
        x->isDistributed = IsNormalProcessingMode() && dtm->initialized && MtmDoReplication && !am_walsender && !IsBackgroundWorker && !IsAutoVacuumWorkerProcess();
		x->containsDML = false;
        x->snapshot = MtmAssignCSN();	
		x->gtid.xid = InvalidTransactionId;
		MtmUnlock();

        MTM_TRACE("MtmLocalTransaction: %s transaction %u uses local snapshot %lu\n", x->isDistributed ? "distributed" : "local", x->xid, x->snapshot);
    }
}

/* 
 * We need to pass snapshot to WAL-sender, so create record in transaction status hash table 
 * before commit
 */
static void MtmPrepareTransaction(MtmCurrentTrans* x)
{ 
	MtmTransState* ts;
	int i;
	
	if (!x->isDistributed) {
		return;
	}
	x->xid = GetCurrentTransactionId();

	MtmLock(LW_EXCLUSIVE);
	ts = hash_search(xid2state, &x->xid, HASH_ENTER, NULL);
	ts->status = TRANSACTION_STATUS_IN_PROGRESS;	
	ts->snapshot = x->isReplicated ? INVALID_CSN : x->snapshot;
	ts->csn = MtmAssignCSN();	
	ts->gtid = x->gtid;
	ts->cmd = MSG_INVALID;
	ts->procno = MyProc->pgprocno;
	ts->nVotes = 0; 
	ts->done = false;
	if (TransactionIdIsValid(x->gtid.xid)) { 
		ts->gtid = x->gtid;
	} else { 
		ts->gtid.xid = x->xid;
		ts->gtid.node = MtmNodeId;
	}
	for (i = 0; i < MtmNodes; i++) { 
		ts->xids[i] = InvalidTransactionId;
	}
	MtmTransactionListAppend(ts);

	MtmUnlock();

	MTM_TRACE("%d: MtmPrepareTransaction prepare commit of %d CSN=%ld\n", getpid(), x->xid, ts->csn);
}

static void 
MtmEndTransaction(MtmCurrentTrans* x, bool commit)
{
	if (x->isDistributed && commit) { 
		MtmTransState* ts;
		MtmLock(LW_EXCLUSIVE);
		ts = hash_search(xid2state, &x->xid, HASH_FIND, NULL);
		Assert(ts != NULL);
		ts->status = TRANSACTION_STATUS_COMMITTED;
		MtmAdjustSubtransactions(ts);
		MtmUnlock();
	}
	x->snapshot = INVALID_CSN;
	x->xid = InvalidTransactionId;
	x->gtid.xid = InvalidTransactionId;
}

void MtmSendNotificationMessage(MtmTransState* ts)
{
	MtmTransState* votingList;

	votingList = dtm->votingTransactions;
	ts->nextVoting = votingList;
	dtm->votingTransactions = ts;

	if (votingList == NULL) { 
		/* singal semaphore only once for the whole list */
		PGSemaphoreUnlock(&dtm->votingSemaphore);
	}
}

static bool
MtmCommitTransaction(TransactionId xid, int nsubxids, TransactionId *subxids)
{
	MtmTransState* ts;

	MtmLock(LW_EXCLUSIVE);
	ts = hash_search(xid2state, &xid, HASH_FIND, NULL);
	Assert(ts != NULL); /* should be created by MtmPrepareTransaction */
	
	MTM_TRACE("%d: MtmCommitTransaction begin commit of %d CSN=%ld\n", getpid(), xid, ts->csn);
	MtmAddSubtransactions(ts, subxids, nsubxids);

	MtmVoteForTransaction(ts); 

	MtmUnlock();

	MTM_TRACE("%d: MtmCommitTransaction %d status=%d\n", getpid(), xid, ts->status);

	return ts->status != TRANSACTION_STATUS_ABORTED;
}
	
static void 
MtmFinishTransaction(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status)
{
	MtmCurrentTrans* x = &dtmTx;

	if (x->isReplicated) { 
		MtmTransState* ts;
		XidStatus prevStatus = TRANSACTION_STATUS_UNKNOWN;
		bool found;

		Assert(status == TRANSACTION_STATUS_ABORTED);

		MtmLock(LW_EXCLUSIVE);
		ts = hash_search(xid2state, &xid, HASH_ENTER, &found);
		if (!found) {
			ts->snapshot = INVALID_CSN;
			ts->csn = MtmAssignCSN();	
			ts->gtid = x->gtid;
			ts->cmd = MSG_INVALID;
		} else { 			
			prevStatus = ts->status;
		}
		ts->status = status;
		MtmAdjustSubtransactions(ts);
		
		if (prevStatus != TRANSACTION_STATUS_ABORTED) {
			ts->cmd = MSG_ABORTED;
			MtmSendNotificationMessage(ts);
		}
		MtmUnlock();
		MTM_TRACE("%d: MtmFinishTransaction %d CSN=%ld, status=%d\n", getpid(), xid, ts->csn, status);
	}
}	
		
	

static void 
MtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
	MTM_TRACE("%d: MtmSetTransactionStatus %u(%u) = %u, isDistributed=%d\n", getpid(), xid, dtmTx.xid, status, dtmTx.isDistributed);
	if (xid == dtmTx.xid && dtmTx.isDistributed)
	{
		if (status == TRANSACTION_STATUS_ABORTED || !dtmTx.containsDML)
		{
			MtmFinishTransaction(xid, nsubxids, subxids, status);	
			MTM_TRACE("Finish transaction %d, status=%d, DML=%d\n", xid, status, dtmTx.containsDML);
		}
		else
		{
			if (MtmCommitTransaction(xid, nsubxids, subxids)) {
				MTM_TRACE("Commit transaction %d\n", xid);
			} else { 
				PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, TRANSACTION_STATUS_ABORTED, lsn);
				dtmTx.isDistributed = false; 
				MarkAsAborted();
				END_CRIT_SECTION();
				elog(ERROR, "Commit of transaction %d is rejected by DTM", xid);                    
			}
		}
	}
	PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
}

static bool 
MtmDetectGlobalDeadLock(PGPROC* proc)
{
    elog(WARNING, "Global deadlock?");
    return true;
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
 *  ***************************************************************************
 */

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
		"multimaster.vacuum_delay",
		"Minimal age of records which can be vacuumed (seconds)",
		NULL,
		&MtmVacuumDelay,
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
		1024*1024,
	    1024,
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
		1,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in mtm_shmem_startup().
	 */
	RequestAddinShmemSpace(MTM_SHMEM_SIZE + MtmQueueSize);
	RequestNamedLWLockTranche(MULTIMASTER_NAME, 1);

    MtmNodes = MtmStartReceivers(MtmConnStrs, MtmNodeId);
    if (MtmNodes < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
	}		
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



/*
 *  ***************************************************************************
 */

void MtmJoinTransaction(GlobalTransactionId* gtid, csn_t globalSnapshot)
{
	csn_t localSnapshot;

	MtmLock(LW_EXCLUSIVE);
	localSnapshot = MtmSyncClock(globalSnapshot);	
	MtmUnlock();
	
	if (globalSnapshot < localSnapshot - MtmVacuumDelay * USEC)
	{
		elog(ERROR, "Too old snapshot: requested %ld, current %ld", globalSnapshot, localSnapshot);
	}

	dtmTx.gtid = *gtid;
	dtmTx.xid = GetCurrentTransactionId();
	dtmTx.snapshot = globalSnapshot;	
	dtmTx.isReplicated = true;
	dtmTx.isDistributed = true;
	dtmTx.containsDML = true;
}
 
void MtmReceiverStarted()
{
     if (pg_atomic_fetch_add_u32(&dtm->nReceivers, 1) == dtm->nNodes-2) {
         dtm->initialized = true;
     }
}

csn_t MtmTransactionSnapshot(TransactionId xid)
{
	MtmTransState* ts;
	csn_t snapshot = INVALID_CSN;

	MtmLock(LW_SHARED);
    ts = hash_search(xid2state, &xid, HASH_FIND, NULL);
    if (ts != NULL) { 
		snapshot = ts->snapshot;
	}
	MtmUnlock();

    return snapshot;
}

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
    dtmTx.isDistributed = false;
    PG_RETURN_VOID();
}

Datum
mtm_drop_node(PG_FUNCTION_ARGS)
{
	int nodeId = PG_GETARG_INT32(0);
	bool dropSlot = PG_GETARG_BOOL(1);
	if (!BIT_SET(dtm->disabledNodeMask, nodeId-1))
	{
		if (nodeId <= 0 || nodeId > dtm->nNodes) 
		{ 
			elog(ERROR, "NodeID %d is out of range [1,%d]", nodeId, dtm->nNodes);
		}
		dtm->disabledNodeMask |= ((int64)1 << (nodeId-1));
		dtm->nNodes -= 1;
		if (!IsTransactionBlock())
		{
			MtmBroadcastUtilityStmt(psprintf("select mtm_drop_node(%d,%s)", nodeId, dropSlot ? "true" : "false"), true);
		}
		if (dropSlot) 
		{
			ReplicationSlotDrop(psprintf("mtm_slot_%d", nodeId));
		}		
	}
    PG_RETURN_VOID();
}
	
Datum
mtm_get_snapshot(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT64(dtmTx.snapshot);
}
	
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
	int64 disabledNodeMask = dtm->disabledNodeMask;
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
		if (!BIT_SET(disabledNodeMask, i)) 
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

static void MtmProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag)
{
	bool skipCommand;
	switch (nodeTag(parsetree))
	{
		case T_TransactionStmt:
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
	if (skipCommand || IsTransactionBlock()) { 
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
		if (!skipCommand) {
			dtmTx.isDistributed = false;
		}
	} else { 		
		MtmBroadcastUtilityStmt(queryString, false);
	}
}


static void
MtmExecutorFinish(QueryDesc *queryDesc)
{
    if (MtmDoReplication) { 
        CmdType operation = queryDesc->operation;
        EState *estate = queryDesc->estate;
        if (estate->es_processed != 0) { 
            dtmTx.containsDML |= operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE;
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

void MtmExecute(void* work, int size)
{
    BgwPoolExecute(&dtm->pool, work, size);
}
    
static BgwPool* 
MtmPoolConstructor(void)
{
    return &dtm->pool;
}

static void 
MtmVoteForTransaction(MtmTransState* ts)
{
	if (!MtmIsCoordinator(ts)) {
		ts->cmd = ts->status == TRANSACTION_STATUS_ABORTED ? MSG_ABORTED : MSG_READY;
		MtmSendNotificationMessage(ts); /* send READY message to coordinator */
	} else if (++ts->nVotes == dtm->nNodes) { /* everybody already voted except me */
		if (ts->status != TRANSACTION_STATUS_ABORTED) {
			Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
			ts->cmd = MSG_PREPARE;
			ts->nVotes = 1; /* I voted myself */
			MtmSendNotificationMessage(ts);			
		}
	}
	MTM_TRACE("%d: Node %d waiting latch...\n", getpid(), MtmNodeId);
	while (!ts->done) {
		MtmUnlock();
		WaitLatch(&MyProc->procLatch, WL_LATCH_SET, -1);
		ResetLatch(&MyProc->procLatch);			
		MtmLock(LW_SHARED);
	}
	MTM_TRACE("%d: Node %d receives response...\n", getpid(), MtmNodeId);
}

HTAB* MtmCreateHash(void)
{
	HASHCTL info;
	HTAB* htab;
	Assert(MtmNodes > 0);
	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(MtmTransState) + (MtmNodes-1)*sizeof(TransactionId);
	info.hash = MtmXidHashFunc;
	info.match = MtmXidMatchFunc;
	htab = ShmemInitHash(
		"xid2state",
		MTM_HASH_SIZE, MTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);
	return htab;
}

MtmState*	
MtmGetState(void)
{
	return dtm;
}
