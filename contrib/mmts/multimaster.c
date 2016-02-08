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

#include "arbiter.h"
#include "sockhub.h"
#include "multimaster.h"
#include "bgwpool.h"

typedef struct DtmTransStatus
{
    TransactionId xid;
    XidStatus status;
    csn_t csn
	bool is_local;
    struct DtmTransStatus* next;
} DtmTransStatus;

typedef struct
{
	LWLockId hashLock;
	TransactionId minXid;  /* XID of oldest transaction visible by any active transaction (local or global) */
	int64  disabledNodeMask;
    int    nNodes;
    pg_atomic_uint32 nReceivers;
    bool initialized;
    BgwPool pool;
} DtmState;

typedef struct { 
    TransactionId xid;
	bool  is_local;
	bool  is_distributed;
    csn_t csn;
    csn_t snapshot;
} DtmCurrentTrans;


#define DTM_SHMEM_SIZE (64*1024*1024)
#define DTM_HASH_SIZE  1003

#define BIT_SET(mask, bit) ((mask) & ((int64)1 << (bit)))
#define MAKE_GTID(xid) (((GlobalTransactionId)MMNodeId << 32) | xid)

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mm_start_replication);
PG_FUNCTION_INFO_V1(mm_stop_replication);
PG_FUNCTION_INFO_V1(mm_drop_node);

static Snapshot DtmGetSnapshot(Snapshot snapshot);
static void DtmMergeWithGlobalSnapshot(Snapshot snapshot);
static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static void DtmUpdateRecentXmin(Snapshot snapshot);
static void DtmInitialize(void);
static void DtmSubXactCallback(SubXactEvent event, SubTransactionId mySubid, SubTransactionId parentSubid, void *arg);
static void DtmXactCallback(XactEvent event, void *arg);
static TransactionId DtmGetNextXid(void);
static TransactionId DtmGetNewTransactionId(bool isSubXact);
static TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static TransactionId DtmGetGlobalTransactionId(void);

static bool DtmDetectGlobalDeadLock(PGPROC* proc);
static void DtmSerializeLock(PROCLOCK* lock, void* arg);
static char const* DtmGetName(void);

static bool TransactionIdIsInSnapshot(TransactionId xid, Snapshot snapshot);
static bool TransactionIdIsInDoubt(TransactionId xid);

static void DtmShmemStartup(void);
static void DtmBackgroundWorker(Datum arg);

static void MMMarkTransAsLocal(TransactionId xid);
static BgwPool* MMPoolConstructor(void);
static bool MMRunUtilityStmt(PGconn* conn, char const* sql);
static void MMBroadcastUtilityStmt(char const* sql, bool ignoreError);

static HTAB* xid_in_doubt;
static HTAB* local_trans;
static DtmState* dtm;

static TransactionId DtmNextXid;
static SnapshotData DtmSnapshot = { HeapTupleSatisfiesMVCC };
static bool DtmHasGlobalSnapshot;
static int DtmLocalXidReserve;
static CommandId DtmCurcid;
static Snapshot DtmLastSnapshot;
static TransactionManager DtmTM = {
	DtmGetTransactionStatus,
	DtmSetTransactionStatus,
	DtmGetSnapshot,
	DtmGetNewTransactionId,
	DtmGetOldestXmin,
	PgTransactionIdIsInProgress,
	DtmGetGlobalTransactionId,
	PgXidInMVCCSnapshot,
    DtmDetectGlobalDeadLock,
	DtmGetName
};

bool  MMDoReplication;
char* MMDatabaseName;

static char* MMConnStrs;
static int   MMNodeId;
static int   MMNodes;
static int   MMQueueSize;
static int   MMWorkers;

static char *Arbiters;
static char *ArbitersCopy;
static int   DtmBufferSize;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;


static void MMExecutorFinish(QueryDesc *queryDesc);
static void MMProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);


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

    SpinLockAcquire(&local->lock);

#if TRACE_SLEEP_TIME
    if (firstReportTime == 0) {
        firstReportTime = dtm_get_current_time();
    }
#endif
    while (true)
    {
        DtmTransStatus* ts = (DtmTransStatus*)hash_search(xid2status, &xid, HASH_FIND, NULL);
        if (ts != NULL)
        {
            if (ts->cid > dtm_tx.snapshot) { 
                DTM_TRACE((stderr, "%d: tuple with xid=%d(csn=%lld) is invisibile in snapshot %lld\n",
                           getpid(), xid, ts->cid, dtm_tx.snapshot));
                SpinLockRelease(&local->lock);
                return true;
            }
            if (ts->status == TRANSACTION_STATUS_IN_PROGRESS)
            {
                DTM_TRACE((stderr, "%d: wait for in-doubt transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
                SpinLockRelease(&local->lock);
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
                SpinLockAcquire(&local->lock);
            }
            else
            {
                bool invisible = ts->status == TRANSACTION_STATUS_ABORTED;
                DTM_TRACE((stderr, "%d: tuple with xid=%d(csn= %lld) is %s in snapshot %lld\n",
                           getpid(), xid, ts->cid, invisible ? "rollbacked" : "committed", dtm_tx.snapshot));
                SpinLockRelease(&local->lock);
                return invisible;
            }
        }
        else
        {
            DTM_TRACE((stderr, "%d: visibility check is skept for transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
            break;
        }
    }
	SpinLockRelease(&local->lock);
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

static void DtmTransactionListAppend(DtmTransStatus* ts)
{
    ts->next = NULL;
    *local->trans_list_tail = ts;
    local->trans_list_tail = &ts->next;
}

static void DtmTransactionListInsertAfter(DtmTransStatus* after, DtmTransStatus* ts)
{
    ts->next = after->next;
    after->next = ts;
    if (local->trans_list_tail == &after->next) { 
        local->trans_list_tail = &ts->next;
    }
}

static TransactionId DtmAdjustOldestXid(TransactionId xid)
{
    if (TransactionIdIsValid(xid)) { 
        DtmTransStatus *ts, *prev = NULL;
        timestamp_t cutoff_time = dtm_get_current_time() - DtmVacuumDelay*USEC;
        SpinLockAcquire(&local->lock);
        ts = (DtmTransStatus*)hash_search(xid2status, &xid, HASH_FIND, NULL);
        if (ts != NULL) { 
            cutoff_time = ts->cid - DtmVacuumDelay*USEC;
        }                
        for (ts = local->trans_list_head; ts != NULL && ts->cid < cutoff_time; prev = ts, ts = ts->next) { 
            if (prev != NULL) { 
                hash_search(xid2status, &prev->xid, HASH_REMOVE, NULL);
            }
        }
        if (prev != NULL) { 
            local->trans_list_head = prev;
            xid = prev->xid;            
        } else { 
            xid = FirstNormalTransactionId;
        }
        SpinLockRelease(&local->lock);
    }
    return xid;
}

static void DtmInitialize()
{
	bool found;
	static HASHCTL info;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	dtm = ShmemInitStruct("dtm", sizeof(DtmState), &found);
	if (!found)
	{
		dtm->hashLock = LWLockAssign();
		dtm->xidLock = LWLockAssign();
		dtm->nReservedXids = 0;
		dtm->minXid = InvalidTransactionId;
        dtm->nNodes = MMNodes;
		dtm->disabledNodeMask = 0;
        pg_atomic_write_u32(&dtm->nReceivers, 0);
        dtm->initialized = false;
        BgwPoolInit(&dtm->pool, MMExecutor, MMDatabaseName, MMQueueSize);
		RegisterXactCallback(DtmXactCallback, NULL);
		RegisterSubXactCallback(DtmSubXactCallback, NULL);
	}
	LWLockRelease(AddinShmemInitLock);

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(TransactionId);
	info.hash = dtm_xid_hash_fn;
	info.match = dtm_xid_match_fn;
	xid_in_doubt = ShmemInitHash(
		"xid_in_doubt",
		DTM_HASH_SIZE, DTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(LocalTransaction);
	info.hash = dtm_xid_hash_fn;
	info.match = dtm_xid_match_fn;
	local_trans = ShmemInitHash(
		"local_trans",
		DTM_HASH_SIZE, DTM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);

    MMDoReplication = true;
	TM = &DtmTM;
}

static void
DtmSubXactCallback(SubXactEvent event, SubTransactionId mySubid, SubTransactionId parentSubid, void *arg)
{
	elog(ERROR, "Subtransactions are not currently supported");
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
        x->csn = INVALID_CSN;
        x->is_local = false;
        x->is_distributed = false;
        x->snapshot = dtm_get_csn();	
		LWLockRelease(dtm->hashLock);
        DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %lu\n", x->xid, x->snapshot));
    }
}

/* 
 * We need to pass snapshot to WAL-sender, so create record in transaction status hash table 
 * before commit
 */
static void DtmPrepareTransaction(DtmCurrentTrans* x)
{ 
	DtmTransStatus* ts;
	Assert(TransactionIdIsValid(x->xid));
	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2status, &x->xid, HASH_ENTER, NULL);
	ts->snapshot = x->snapshot;
	ts->status = TRANSACTION_STATUS_UNKNOWN;
	ts->is_local = x->is_local;
	LWLockRelease(dtm->hashLock);
}

static XidStatus DtmCommitTransaction(TransactionId xid, int nsubxids, TransactionId *subxids)
{
	DtmTransStatus* ts;
	GlobalTransactionId gtid = MAKE_GTID(xid);
	csn_t csn;
	int i;
	int nSubxids;
	XidStatus status;
	bool ack = ArbiterVoteTransaction(gtid, true); /* wait until transaction at all nodes are prepared */

	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2status, &xid, HASH_FIND, NULL);
	Assert(ts != NULL); /* should be created by DtmPrepareTransaction */
	ts->status = status = ack ? TRANSACTION_STATUS_IN_PROGRESS : TRANSACTION_STATUS_ABORTED;
	ts->csn = dtm_get_cid();	
	DtmTransactionListAppend(ts);
	DtmAddSubtransactions(ts, subxids, nsubxids);
	LWLockRelease(dtm->hashLock);

	csn = ack ? ArbiterGetCSN(gtid, ts->csn) : INVALID_CSN; /* get max CSN */
	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	if (csn != INVALID_CSN) { 
		ts->csn = csn;
		dtm_sync(csn);
		status = TRANSACTION_STATUS_COMMITTED;
	} else { 
		csn = ts->csn;
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
	DtmTransStatus* ts;
	GlobalTransactionId gtid = MAKE_GTID(xid);

	LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
	ts = hash_search(xid2status, &xid, HASH_FIND, NULL);
	Assert(ts != NULL); /* should be created by DtmPrepareTransaction */
	ts->status = TRANSACTION_STATUS_ABORTED;	
	for (i = 0; i < nSubxids; i++) { 	
		ts->status = status;
		ts = ts->next;
		ts->status = TRANSACTION_STATUS_ABORTED;
	}        
	LWLockRelease(dtm->hashLock);

	ArbiterVoteTransaction(gtid, false);
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
		"multimaster.local_xid_reserve",
		"Number of XIDs reserved by node for local transactions",
		NULL,
		&DtmLocalXidReserve,
		100,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomIntVariable(
		"multimaster.buffer_size",
		"Size of sockhub buffer for connection to DTM daemon, if 0, then direct connection will be used",
		NULL,
		&DtmBufferSize,
		0,
		0,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);

	DefineCustomStringVariable(
		"multimaster.arbiters",
		"The comma separated host:port pairs where arbiters reside",
		NULL,
		&Arbiters,
		"127.0.0.1:5431",
		PGC_BACKEND, // context
		0, // flags,
		NULL, // GucStringCheckHook check_hook,
		NULL, // GucStringAssignHook assign_hook,
		NULL // GucShowHook show_hook
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
	RequestAddinLWLocks(2);

    MMNodes = MMStartReceivers(MMConnStrs, MMNodeId);
    if (MMNodes < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
    }
    BgwPoolStart(MMWorkers, MMPoolConstructor);

	ArbitersCopy = strdup(Arbiters);
	if (DtmBufferSize != 0)
	{
		ArbiterConfig(Arbiters, Unix_socket_directories);
		RegisterBackgroundWorker(&DtmWorker);
	}
	else
		ArbiterConfig(Arbiters, NULL);

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

GlobalTransactionId MMBeginTransaction(void)
{
    dtm_tx.is_distributed = false;
	return dtm_tx.snapshot;
}

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

bool MMIsLocalTransaction(TransactionId xid)
{
	TransStatus* ts;
	bool is_local = false;
	LWLockAcquire(dtm->hashLock, LW_SHARED);
    ts = hash_search(xid2status, &xid, HASH_FIND, NULL);
    if (ts != NULL) { 
		is_local = ts->is_local;
	}
	LWLockRelease(dtm->hashLock);
    return is_local;
}

bool MMDetectGlobalDeadLock(PGPROC* proc)
{
    elog(WARNING, "Global deadlock?");
    return true;
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

static void DtmAddSubtransactions(DtmTransStatus* ts, TransactionId* subxids, int nSubxids)
{
    int i;
    for (i = 0; i < nSubxids; i++) { 
        bool found;
		DtmTransStatus* sts;
		Assert(TransactionIdIsValid(subxids[i]));
        sts = (DtmTransStatus*)hash_search(xid2status, &subxids[i], HASH_ENTER, &found);
        Assert(!found);
        sts->status = ts->status;
        sts->cid = ts->cid;
        sts->nSubxids = 0;
        DtmTransactionListInsertAfter(ts, sts);
    }
}
