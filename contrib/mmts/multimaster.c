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
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/xlogdefs.h"
#include "access/xact.h"
#include "access/xtm.h"
#include "access/transam.h"
#include "access/subtrans.h"
#include "access/commit_ts.h"
#include "access/xlog.h"
#include "storage/proc.h"
#include "storage/procarray.h"
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
    TransactionId xid;
	GlobalTransactionId gtid
	bool  is_local;       /* transaction on replica */
	bool  is_distributed; /* transaction performs INSERT/UPDATE/DELETE and has to be replicated to other nodes */
    csn_t snapshot;       /* transaction snaphsot   */
} DtmCurrentTrans;

typedef uint64 timestamp_t;

#define DTM_SHMEM_SIZE (64*1024*1024)
#define DTM_HASH_SIZE  1000003
#define USEC 1000000
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000

#define BIT_SET(mask, bit) ((mask) & ((int64)1 << (bit)))

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mm_start_replication);
PG_FUNCTION_INFO_V1(mm_stop_replication);
PG_FUNCTION_INFO_V1(mm_drop_node);

static Snapshot DtmGetSnapshot(Snapshot snapshot);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static void DtmUpdateRecentXmin(Snapshot snapshot);
static void DtmInitialize(void);
static void DtmXactCallback(XactEvent event, void *arg);

static Snapshot DtmGetSnapshot(Snapshot snapshot);
static TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static bool DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static TransactionId DtmAdjustOldestXid(TransactionId xid);
static bool DtmDetectGlobalDeadLock(PGPROC* proc);
static cid_t DtmGetCsn(TransactionId xid);
static void DtmAddSubtransactions(DtmTransState* ts, TransactionId* subxids, int nSubxids);
static char const* DtmGetName(void);

static void DtmShmemStartup(void);

static BgwPool* MMPoolConstructor(void);
static bool MMRunUtilityStmt(PGconn* conn, char const* sql);
static void MMBroadcastUtilityStmt(char const* sql, bool ignoreError);
static void MMVoteForTransaction(DtmTransState* ts);

static HTAB* xid2state;
static DtmCurrentTrans dtm_tx;

static TransactionManager DtmTM = { 
	PgTransactionIdGetStatus, 
	PgTransactionIdSetTreeStatus, 
	DtmGetSnapshot, 
	PgGetNewTransactionId, 
	DtmGetOldestXmin, 
	PgTransactionIdIsInProgress, 
	PgGetGlobalTransactionId, 
	DtmXidInMVCCSnapshot, 
	DtmDetectGlobalDeadLock, 
	DtmGetName 
};

DtmState* dtm;
bool  MMDoReplication;
char* MMDatabaseName;

char* MMConnStrs;
int   MMNodeId;
int   MMArbiterPort;

static int   MMNodes;
static int   MMQueueSize;
static int   MMWorkers;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;


static void MMExecutorFinish(QueryDesc *queryDesc);
static void MMProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);

/*
 *  System time manipulation functions
 */


static timestamp_t dtm_get_current_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (timestamp_t)tv.tv_sec*USEC + tv.tv_usec + local->time_shift;
}

static void dtm_sleep(timestamp_t interval)
{
    struct timespec ts;
    struct timespec rem;
    ts.tv_sec = 0;
    ts.tv_nsec = interval*1000;

    while (nanosleep(&ts, &rem) < 0) { 
        totalSleepInterrupts += 1;
        Assert(errno == EINTR);
        ts = rem;
    }
}
    
static csn_t dtm_get_csn()
{
    csn_t csn = dtm_get_current_time();
    if (csn <= dtm->csn) { 
        csn = ++dtm->csn;
    } else { 
        dtm->csn = csn;
    }
    return csn;
}

static csn_t dtm_sync(csn_t global_csn)
{
    csn_t dtm_csn;
#if 1
    while ((dtm_csn = dtm_get_csn()) < global_csn) { 
        dtm->time_shift += global_csn - dtm_csn;
    }
#else
    while ((dtm_csn = dtm_get_csn()) < global_csn) { 
        LWLockRelease(&dtm->hashLock);
#if TRACE_SLEEP_TIME
        {
        timestamp_t now = dtm_get_current_time();
        static timestamp_t firstReportTime;
        static timestamp_t prevReportTime;
        static timestamp_t totalSleepTime;
#endif
        dtm_sleep(global_csn - dtm_csn);
#if TRACE_SLEEP_TIME
        totalSleepTime += dtm_get_current_time() - now;
        if (now > prevReportTime + USEC) { 
            prevReportTime = now;
            if (firstReportTime == 0) { 
                firstReportTime = now;
            } else { 
                fprintf(stderr, "Sync sleep %lu of %lu usec (%f%%)\n", totalSleepTime, now - firstReportTime, totalSleepTime*100.0/(now - firstReportTime));
            }
        }
        }
#endif
		LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
    }
#endif
    return dtm_csn;
}

/*
 * Distribute transaction manager functions
 */ 
static char const* DtmGetName(void)
{
	return "mmts";
}

Snapshot DtmGetSnapshot(Snapshot snapshot)
{
    snapshot = PgGetSnapshotData(snapshot);
    RecentGlobalDataXmin = RecentGlobalXmin = DtmAdjustOldestXid(RecentGlobalDataXmin);
    return snapshot;
}


TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
    TransactionId xmin = PgGetOldestXmin(rel, ignoreVacuum);
    xmin = DtmAdjustOldestXid(xmin);
    return xmin;
}

bool DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot)
{
#if TRACE_SLEEP_TIME
    static timestamp_t firstReportTime;
    static timestamp_t prevReportTime;
    static timestamp_t totalSleepTime;
    static timestamp_t maxSleepTime;
#endif
    timestamp_t delay = MIN_WAIT_TIMEOUT;
    Assert(xid != InvalidTransactionId);

	LWLockAcquire(&dtm->hashLock, LW_SHARED);

#if TRACE_SLEEP_TIME
    if (firstReportTime == 0) {
        firstReportTime = dtm_get_current_time();
    }
#endif
    while (true)
    {
        DtmTransState* ts = (DtmTransState*)hash_search(xid2state, &xid, HASH_FIND, NULL);
        if (ts != NULL)
        {
            if (ts->csn > dtm_tx.snapshot) { 
                DTM_TRACE((stderr, "%d: tuple with xid=%d(csn=%lld) is invisibile in snapshot %lld\n",
                           getpid(), xid, ts->csn, dtm_tx.snapshot));
                LWLockRelease(&dtm->hashLock);
                return true;
            }
            if (ts->status == TRANSACTION_STATUS_IN_PROGRESS)
            {
                DTM_TRACE((stderr, "%d: wait for in-doubt transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
                LWLockRelease(&dtm->hashLock);
#if TRACE_SLEEP_TIME
                {
                timestamp_t delta, now = dtm_get_current_time();
#endif
                dtm_sleep(delay);
#if TRACE_SLEEP_TIME
                delta = dtm_get_current_time() - now;
                totalSleepTime += delta;
                if (delta > maxSleepTime) {
                    maxSleepTime = delta;
                }
                if (now > prevReportTime + USEC*10) { 
                    prevReportTime = now;
                    if (firstReportTime == 0) { 
                        firstReportTime = now;
                    } else { 
                        fprintf(stderr, "Snapshot sleep %lu of %lu usec (%f%%), maximum=%lu\n", totalSleepTime, now - firstReportTime, totalSleepTime*100.0/(now - firstReportTime), maxSleepTime);
                    }
                }
                }
#endif
                if (delay*2 <= MAX_WAIT_TIMEOUT) {
                    delay *= 2;
                }
				LWLockAcquire(&dtm->hashLock, LW_SHARED);
            }
            else
            {
                bool invisible = ts->status != TRANSACTION_STATUS_COMMITTED;
                DTM_TRACE((stderr, "%d: tuple with xid=%d(csn= %lld) is %s in snapshot %lld\n",
                           getpid(), xid, ts->csn, invisible ? "rollbacked" : "committed", dtm_tx.snapshot));
                LWLockRelease(&dtm->hashLock);
                return invisible;
            }
        }
        else
        {
            DTM_TRACE((stderr, "%d: visibility check is skept for transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
            break;
        }
    }
	LWLockRelease(&dtm->hashLock);
	return PgXidInMVCCSnapshot(xid, snapshot);
}    

static uint32 dtm_xid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int dtm_xid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 - *(TransactionId*)key2;
}

static void DtmTransactionListAppend(DtmTransState* ts)
{
    ts->next = NULL;
    *dtm->trans_list_tail = ts;
    dtm->trans_list_tail = &ts->next;
}

static void DtmTransactionListInsertAfter(DtmTransState* after, DtmTransState* ts)
{
    ts->next = after->next;
    after->next = ts;
    if (dtm->trans_list_tail == &after->next) { 
        dtm->trans_list_tail = &ts->next;
    }
}

static void DtmAddSubtransactions(DtmTransState* ts, TransactionId* subxids, int nSubxids)
{
    int i;
    for (i = 0; i < nSubxids; i++) { 
        bool found;
		DtmTransState* sts;
		Assert(TransactionIdIsValid(subxids[i]));
        sts = (DtmTransState*)hash_search(xid2state, &subxids[i], HASH_ENTER, &found);
        Assert(!found);
        sts->status = ts->status;
        sts->csn = ts->csn;
        sts->nSubxids = 0;
        DtmTransactionListInsertAfter(ts, sts);
    }
}

static TransactionId DtmAdjustOldestXid(TransactionId xid)
{
    if (TransactionIdIsValid(xid)) { 
        DtmTransState *ts, *prev = NULL;
        
		LWLockAcquire(&dtm->hashLock, LW_EXCLSUIVE);
        ts = (DtmTransState*)hash_search(xid2state, &xid, HASH_FIND, NULL);
        if (ts != NULL) { 
            timestamp_t cutoff_time = ts->csn - DtmVacuumDelay*USEC;
			
			for (ts = dtm->trans_list_head; ts != NULL && ts->csn < cutoff_time; prev = ts, ts = ts->next) { 
				if (prev != NULL) { 
					hash_search(xid2state, &prev->xid, HASH_REMOVE, NULL);
				}
			}
        }
        if (prev != NULL) { 
            dtm->trans_list_head = prev;
            dtm->minXid = xid = prev->xid;            
        } else { 
            xid = dtm->minXid;
        }
		LWLockRelease(&dtm->hashLock);
    }
    return xid;
}

static void DtmInitialize()
{
	bool found;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	dtm = ShmemInitStruct("dtm", sizeof(DtmState), &found);
	if (!found)
	{
		dtm->hashLock = LWLockAssign();
		dtm->csn = dtm_get_current_time();
		dtm->minXid = InvalidTransactionId;
        dtm->nNodes = MMNodes;
		dtm->disabledNodeMask = 0;
		dtm->pendingTransactions = NULL;
        pg_atomic_write_u32(&dtm->nReceivers, 0);
        dtm->initialized = false;
        BgwPoolInit(&dtm->pool, MMExecutor, MMDatabaseName, MMQueueSize);
		RegisterXactCallback(DtmXactCallback, NULL);
		RegisterSubXactCallback(DtmSubXactCallback, NULL);
	}
	xid2state = MMCreateHash();
	LWLockRelease(AddinShmemInitLock);
    MMDoReplication = true;
	TM = &DtmTM;
}

static void
DtmXactCallback(XactEvent event, void *arg)
{
	//XTM_INFO("%d: DtmXactCallbackevent=%d nextxid=%d\n", getpid(), event, DtmNextXid);
    switch (event) 
    {
    case XACT_EVENT_START: 
	    DtmBeginTransaction(&dtm_tx);
        break;
    case XACT_EVENT_PRE_COMMIT:
		DtmPrepareTransaction(&dtm_tx);
		break;
      default:
        break;
	}
}

void DtmBeginTransaction(DtmCurrentTrans* x)
{
    if (!TransactionIdIsValid(x->xid)) { 
		LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
        x->xid = GetCurrentTransactionIdIfAny();
        x->is_local = false;
        x->is_distributed = false;
        x->snapshot = dtm_get_csn();	
		x->gtid.xid = InvalidTransactionId;
		LWLockRelease(dtm->hashLock);
        DTM_TRACE((stderr, "DtmLocalTransaction: transaction %u uses local snapshot %lu\n", x->xid, x->snapshot));
    }
}

/* 
 * We need to pass snapshot to WAL-sender, so create record in transaction status hash table 
 * before commit
 */
static void DtmPrepareTransaction(DtmCurrentTrans* x)
{ 
	DtmTransState* ts;
	Assert(TransactionIdIsValid(x->xid));
	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2state, &x->xid, HASH_ENTER, NULL);
	ts->snapshot = x->is_local ? x->snapshot : INVALID_CSN;
	ts->status = TRANSACTION_STATUS_UNKNOWN;
	ts->csn = dtm_get_csn();	
	if (!TransactionIdIsValid(x->gtid.xid)) 
	{
		ts->gtid.xid = xid;
		ts->gtid.node = MMNodeId;
	} else {
		ts->gtid = x->gtid;
	}
	LWLockRelease(dtm->hashLock);
}

static XidStatus DtmCommitTransaction(TransactionId xid, int nsubxids, TransactionId *subxids)
{
	DtmTransState* ts;
	csn_t csn;
	int i;
	int nSubxids;
	XidStatus status;

	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2state, &xid, HASH_FIND, NULL);
	Assert(ts != NULL); /* should be created by DtmPrepareTransaction */

	/* now transaction is in doubt state */
	ts->status = TRANSACTION_STATUS_IN_PROGRESS;
	csn = dtm_get_csn();	
	if (csn > ts->csn) {
		ts->csn = csn;
	}
	DtmTransactionListAppend(ts);
	DtmAddSubtransactions(ts, subxids, nsubxids);

	MMVoteForTransaction(ts); /* wait until transaction at all nodes are prepared */
	csn = ts->csn;
	if (csn != INVALID_CSN) { 
		dtm_sync(csn);
		status = TRANSACTION_STATUS_COMMITTED;
	} else { 
		status = TRANSACTION_STATUS_ABORTED;
	}
	ts->status = status;
	for (i = 0; i < nsubxids; i++) { 	
		ts = ts->next;
		ts->status = status;
		ts->csn = csn;
	}        
	LWLockRelease(dtm->hashLock);
	return status;
}
	
static void DtmAbortTransaction(TransactionId xid, int nsubxids, TransactionId *subxids)
{
	int i;
	DtmTransState* ts;

	if (!TransactionIdIsValid(dtm_tx.gtid.xid)) 
	{
		dtm_tx.gtid.xid = xid;
		dtm_tx.gtid.node = MMNodeId;
	}

	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2state, &xid, HASH_FIND, NULL);
	Assert(ts != NULL); /* should be created by DtmPrepareTransaction */
	ts->status = TRANSACTION_STATUS_ABORTED;	
	for (i = 0; i < nSubxids; i++) { 	
		ts->status = status;
		ts = ts->next;
		ts->status = TRANSACTION_STATUS_ABORTED;
	}        
	LWLockRelease(dtm->hashLock);

	ArbiterVoteTransaction(dtm_tx.gtid, false);
}	
		
	

static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
	XTM_INFO("%d: DtmSetTransactionStatus %u = %u\n", getpid(), xid, status);
	if (!RecoveryInProgress())
	{
		if (status == TRANSACTION_STATUS_ABORTED || !dtm_tx.is_distributed)
		{
			DtmAbortTransaction(xid, nsubxids, xibxods);	
			XTM_INFO("Abort transaction %d\n", xid);
		}
		else
		{
			if (DtmCommitTransaction(xid, nsubxids, xids) == TRANSACTION_STATUS_COMMITTED) { 
				XTM_INFO("Commit transaction %d\n", xid);
			} else { 
				PgTransactionIdSetTreeStatus(xid, nsubxids, TRANSACTION_STATUS_ABORTED, status, lsn);
				dtm_tx.is_distributed = false; 
				MarkAsAborted();
				END_CRIT_SECTION();
				elog(ERROR, "Commit of transaction %d is rejected by DTM", xid);                    
			}
		}
	}
	PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
}

bool DtmDetectGlobalDeadLock(PGPROC* proc)
{
    elog(WARNING, "Global deadlock?");
    return true;
}

static void DtmShmemStartup(void)
{
	if (PreviousShmemStartupHook) {
		PreviousShmemStartupHook();
	}
	DtmInitialize();
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
		"multimaster.workers",
		"Number of multimaster executor workers per node",
		NULL,
		&MMWorkers,
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
		&MMQueueSize,
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
		"multimaster.arpiter_port",
		"Base value for assigning arbiter ports",
		NULL,
		&MMAtbiterPort,
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
		&MMConnStrs,
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
		&MMNodeId,
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
	 * resources in dtm_shmem_startup().
	 */
	RequestAddinShmemSpace(DTM_SHMEM_SIZE + MMQueueSize);
	RequestAddinLWLocks(1);

    MMNodes = MMStartReceivers(MMConnStrs, MMNodeId);
    if (MMNodes < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
    }
    BgwPoolStart(MMWorkers, MMPoolConstructor);

	MMArbiterInitialize();

	/*
	 * Install hooks.
	 */
	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = DtmShmemStartup;

	PreviousExecutorFinishHook = ExecutorFinish_hook;
	ExecutorFinish_hook = MMExecutorFinish;

	PreviousProcessUtilityHook = ProcessUtility_hook;
	ProcessUtility_hook = MMProcessUtility;
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

void MMJoinTransaction(GlonalTransactionId gtid, csn_t snapshot)
{
	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	dtm_sync(snapshot);
	LWLockRelease(dtm->hashLock);
	
	dtm_tx.gtid = gtid;
	dtm_tx.xid = GetCurrentTransactionId();
	dtm_tx.snapshot = snapshot;	
	dtm_tx.is_local = true;
	dtm_tx.is_distributed = false;
}
 
void MMReceiverStarted()
{
     if (pg_atomic_fetch_add_u32(&dtm->nReceivers, 1) == dtm->nNodes-2) {
         dtm->initialized = true;
     }
}

csn_t MMTransactionSnapshot(TransactionId xid)
{
	TransStatus* ts;
	csn_t snapshot = INVALID_CSN;

	LWLockAcquire(dtm->hashLock, LW_SHARED);
    ts = hash_search(xid2state, &xid, HASH_FIND, NULL);
    if (ts != NULL) { 
		snapshot = ts->snapshot;
	}
	LWLockRelease(dtm->hashLock);

    return snapshot;
}

Datum
mm_start_replication(PG_FUNCTION_ARGS)
{
    MMDoReplication = true;
    PG_RETURN_VOID();
}

Datum
mm_stop_replication(PG_FUNCTION_ARGS)
{
    MMDoReplication = false;
    dtm_tx.is_distributed = false;
    PG_RETURN_VOID();
}

Datum
mm_drop_node(PG_FUNCTION_ARGS)
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
			MMBroadcastUtilityStmt(psprintf("select mm_drop_node(%d,%s)", nodeId, dropSlot ? "true" : "false"), true);
		}
		if (dropSlot) 
		{
			ReplicationSlotDrop(psprintf("mm_slot_%d", nodeId));
		}		
	}
    PG_RETURN_VOID();
}
		
/*
 * Execute statement with specified parameters and check its result
 */
static bool MMRunUtilityStmt(PGconn* conn, char const* sql)
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

static void MMBroadcastUtilityStmt(char const* sql, bool ignoreError)
{
	char* conn_str = pstrdup(MMConnStrs);
	char* conn_str_end = conn_str + strlen(conn_str);
	int i = 0;
	int64 disabledNodeMask = dtm->disabledNodeMask;
	int failedNode = -1;
	char const* errorMsg = NULL;
	PGconn **conns = palloc0(sizeof(PGconn*)*MMNodes);
    
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
	Assert(i == MMNodes);
	
	for (i = 0; i < MMNodes; i++) 
	{ 
		if (conns[i]) 
		{
			if (!MMRunUtilityStmt(conns[i], "BEGIN TRANSACTION") && !ignoreError)
			{
				errorMsg = "Failed to start transaction at node %d";
				failedNode = i;
				break;
			}
			if (!MMRunUtilityStmt(conns[i], sql) && !ignoreError)
			{
				errorMsg = "Failed to run command at node %d";
				failedNode = i;
				break;
			}
		}
	}
	if (failedNode >= 0 && !ignoreError)  
	{
		for (i = 0; i < MMNodes; i++) 
		{ 
			if (conns[i])
			{
				MMRunUtilityStmt(conns[i], "ROLLBACK TRANSACTION");
			}
		}
	} else { 
		for (i = 0; i < MMNodes; i++) 
		{ 
			if (conns[i] && !MMRunUtilityStmt(conns[i], "COMMIT TRANSACTION") && !ignoreError) 
			{ 
				errorMsg = "Commit failed at node %d";
				failedNode = i;
			}
		}
	}			
	for (i = 0; i < MMNodes; i++) 
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

static void MMProcessUtility(Node *parsetree, const char *queryString,
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
			dtm_tx.is_distributed = false;
		}
	} else { 		
		MMBroadcastUtilityStmt(queryString, false);
	}
}


static void
MMExecutorFinish(QueryDesc *queryDesc)
{
    if (MMDoReplication) { 
        CmdType operation = queryDesc->operation;
        EState *estate = queryDesc->estate;
        if (estate->es_processed != 0) { 
            dtm_tx.is_distributed |= operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE;
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

void MMExecute(void* work, int size)
{
    BgwPoolExecute(&dtm->pool, work, size);
}
    
static BgwPool* MMPoolConstructor(void)
{
    return &dtm->pool;
}


void MMVoteForTransaction(DtmTransState* ts)
{
	LWLockRelease(&dtm->hashLock);
	if (ts->gtid.node == MMNodeId) {
		/* I am master: wait responces from replicas */
		while (ts->nVotes+1 != dtm->nNodes) {
			WaitLatch(&MyProc->procLatch, WL_LATCH_SET, -1);
			ResetLatch(&MyProc->procLatch);			
		}
	} else {
		/* I am replica: first notify master... */
		SpinLockAcquire(&dtm->spinlock);
		ts->nextPending = dtm->pendingTransactions;
		ts->pid = MyProc->pgprocno;
		dtm->pendingTransactions = ts;
		SpinLockRelease(&dtm->spinlock);

		PGSemaphoreUnlock(&dtm->semapahore);
		/* ... and wait response from it */
		WaitLatch(&MyProc->procLatch, WL_LATCH_SET, -1);
		ResetLatch(&MyProc->procLatch);			
	}
	LWLockAcquire(&dtm->hashLock< LW_EXCLUSIVE);
}

HTAB* MMCreateHash();
{
	static HASHCTL info;
	TAB* htab;
	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(DtmTransState);
	info.hash = dtm_xid_hash_fn;
	info.match = dtm_xid_match_fn;
	htab = ShmemInitHash(
		"xid2state",
		DTM_HASH_SIZE, DTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);
	return htab;
}
	
