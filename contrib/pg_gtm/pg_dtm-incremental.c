/*
 * pg_dtm.c
 *
 * Pluggable distributed transaction manager
 *
 */

#include <unistd.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/xlogdefs.h"
#include "access/xact.h"
#include "access/transam.h"
#include "access/xlog.h"
#include "access/csnlog.h"
#include "access/twophase.h"
#include "utils/hsearch.h"
#include "utils/tqual.h"

#include "libdtm.h"
#include "pg_dtm.h"

#define DTM_MAX_PARALLEL_DIST_TRANS 100
#define DTM_HASH_INIT_SIZE  1024
#define MAX_CID        ((cid_t)-1)
#define INVALID_INDEX  ((size_t)-1)
#define INVALID_CID 0
#define LOW 0
#define UP  1

typedef struct
{
	cid_t last_gcid;
    cid_t local_cid;
	volatile slock_t lock;
#ifndef READS_FROM_GCID_MAP
    size_t reads_from_gcid;
#endif 
    size_t free_index;
	DtmGlobalTransState trans[DTM_MAX_PARALLEL_DIST_TRANS]; // PPG-FIXME: why do we need this array?
} DtmNodeState;

typedef struct
{
	cid_t cid;
	TransactionId tid;
} DtmTransId;

typedef struct
{
    TransactionIdStatus status;
    cid_t csn;
} DtmTransStatus;

//#define DTM_TRACE(x) 
#define DTM_TRACE(x) fprintf x

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static HTAB* gcidup2lsnap; // map<cid, cid>
static HTAB* gcid2lcid; // map<cid, cid>
static HTAB* tid2status;
#ifdef READS_FROM_GCID_MAP
static HTAB* reads_from_gcid; // map<cid, list<DtmTransState>>
#endif
static DtmNodeState* local;
static DtmTransState dtm_tx;
static DTMConn dtm_conn;
static cid_t prepared_gcid;

void _PG_init(void);
void _PG_fini(void);
void dtm_local_prepare(DtmTransState* x, cid_t newGcid);

static void pg_dtm_shmem_startup(void);
static Size pg_dtm_memsize(void);
static void pg_dtm_xact_callback(XactEvent event, void *arg);
static bool dtm_ensure_connection(void);

/*
 *  ***************************************************************************
 */

void
_PG_init(void)
{
	DTM_TRACE((stderr, "DTM_PG_init \n"));

	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out without hooking into any of
	 * the main system.  (We don't throw error here because it seems useful to
	 * allow the pg_stat_statements functions to be created even when the
	 * module isn't active.  The functions must protect themselves against
	 * being called then, however.)
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	// DtmInitialize();

	RequestAddinShmemSpace(pg_dtm_memsize());
	RequestAddinLWLocks(1);

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = pg_dtm_shmem_startup;

	/*
	 * Initialize tx callbacks
	 */
	RegisterXactCallback(pg_dtm_xact_callback, NULL);
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	shmem_startup_hook = prev_shmem_startup_hook;
}

/*
 * Estimate shared memory space needed.
 */
static Size
pg_dtm_memsize(void)
{
	Size        size;

	size = MAXALIGN(sizeof(DtmNodeState));
	size = add_size(size, 3*DTM_HASH_INIT_SIZE);

	return size;
}


/*
 * shmem_startup hook: allocate or attach to shared memory,
 * then load any pre-existing statistics from file.
 * Also create and load the query-texts file, which is expected to exist
 * (even if empty) while the module is enabled.
 */
static void
pg_dtm_shmem_startup(void)
{
	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	DtmInitialize();
}

static cid_t
pg_dtm_get_prepared_gcid()
{
    if (prepared_gcid != INVALID_CID) { 
        return prepared_gcid;
    } else { 
        const char* gid = GetLockedGlobalTransactionId();
        Assert(gid != NULL);
        return atol(gid);
    }            
}

static void
pg_dtm_xact_callback(XactEvent event, void *arg)
{
    DTM_TRACE((stderr, "Backend %d pg_dtm_xact_callback %d\n", getpid(), event));
	switch (event)
	{
		case XACT_EVENT_START:
            DtmLocalBegin(&dtm_tx);
			break;

		case XACT_EVENT_PREPARE:
            if (dtm_tx.is_global) { 
                cid_t gcid = pg_dtm_get_prepared_gcid();
				DTM_TRACE((stderr, "Backend %d prepare transaction %llu\n", getpid(), gcid));
				DtmLocalPrepare(&dtm_tx, gcid);
            }
			break;

		case XACT_EVENT_ABORT:
            if (dtm_tx.is_prepared) {
				DTM_TRACE((stderr, "Backend %d abort transaction %u\n", getpid(), dtm_tx.tid));
				DtmLocalAbort(&dtm_tx);
            }
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_ABORT_PREPARED:
            if (prepared_gcid != INVALID_CID) { 
				DTM_TRACE((stderr, "Backend %d abort prepared transaction %llu\n", getpid(), prepared_gcid));
				while (!dtm_ensure_connection()) {}
				DtmGlobalRollback(dtm_conn, prepared_gcid);
			}
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT:
            if (dtm_tx.is_prepared) {
				DTM_TRACE((stderr, "Backend %d commit transaction %u\n", getpid(), dtm_tx.tid));
				DtmLocalCommit(&dtm_tx);
            }
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT_PREPARED:
            if (prepared_gcid != INVALID_CID) { 
				DTM_TRACE((stderr, "Backend %d commit prepared transaction %llu\n", getpid(), prepared_gcid));
				while (!dtm_ensure_connection()) {}
				DtmGlobalCommit(dtm_conn, prepared_gcid);
			}
			DtmLocalEnd(&dtm_tx);
			break;

		default:
			break;
	}
}

/*
 *  ***************************************************************************
 */

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dtm_extend);
PG_FUNCTION_INFO_V1(dtm_access);
PG_FUNCTION_INFO_V1(dtm_prepare);

Datum
dtm_extend(PG_FUNCTION_ARGS)
{
	DTM_TRACE((stderr, "Backend %d extends transaction to global\n", getpid()));
	PG_RETURN_INT64( DtmLocalExtend(&dtm_tx) );
}

Datum
dtm_access(PG_FUNCTION_ARGS)
{
	DTM_TRACE((stderr, "Backend %d joins transaction\n", getpid()));
	DtmLocalAccess(&dtm_tx, PG_GETARG_INT64(0));
	PG_RETURN_VOID();
}

Datum
dtm_prepare(PG_FUNCTION_ARGS)
{
    while (!dtm_ensure_connection()) {}
    prepared_gcid = DtmGlobalPrepare(dtm_conn);
	DTM_TRACE((stderr, "Backend %d receive prepared GCID %llu\n", getpid(), prepared_gcid));
	PG_RETURN_INT64(prepared_gcid);
}

/*
 *  ***************************************************************************
 */
static bool dtm_ensure_connection(void)
{
	if (dtm_conn)
		return true;

	dtm_conn = DtmConnect("127.0.0.1", 5431);
	if (dtm_conn)
		return true;

	DTM_TRACE((stderr, "failed to connect to dtmd\n"));
	return false;
}

static cid_t get_local_csn()
{
	TimeLineID tli;
	return RecoveryInProgress() ? GetXLogReplayRecPtr(&tli) : GetXLogInsertRecPtr();
}

static void set_local_csn(TransactionId xid, cid_t cid)
{
    TransactionIdSetCommitLSN(xid, 0, NULL, (XLogRecPtr)cid);
}

static uint32 dtm_cid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(cid_t*)key;
}

static int dtm_cid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(cid_t*)key1 == *(cid_t*)key2;
}

static uint32 dtm_tid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int dtm_tid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 == *(TransactionId*)key2;
}

static VisibilityCheckResult DtmVisibilityCheck(TransactionId xid, Snapshot snapshot, TransactionIdStatus *hintstatus)
{
	VisibilityCheckResult result;
	SpinLockAcquire(&local->lock);
	while (true)
	{
		DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &xid, HASH_FIND, NULL);
		if (ts != NULL)
		{
			*hintstatus = ts->status;
			if (ts->csn > dtm_tx.local_snapshot) { 
                result = XID_INVISIBLE;
                break;
            }
            if (ts->status == XID_INPROGRESS)
			{
                DTM_TRACE((stderr, "Wait for in-doubt transaction %u\n", xid));
				SpinLockRelease(&local->lock);
				XactLockTableWait(xid, NULL, NULL, XLTW_None);
				SpinLockAcquire(&local->lock);
			}
			else
			{
				result = ts->status == XID_COMMITTED ? XID_VISIBLE : XID_INVISIBLE;
				break;
			}
		}
		else
		{
            DTM_TRACE((stderr, "Visibility check is skept for transaction %u\n", xid));
			result = XID_IN_DOUBT;
			break;
		}
	}
	SpinLockRelease(&local->lock);
	return result;
}


void DtmInitialize()
{
	bool found;
	static HASHCTL info;
	int i;

	info.keysize = sizeof(cid_t);
	info.entrysize = sizeof(cid_t);
	info.hash = dtm_cid_hash_fn;
	info.match = dtm_cid_match_fn;
	gcidup2lsnap = ShmemInitHash("gcidup2lsnap",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

	info.keysize = sizeof(cid_t);
	info.entrysize = sizeof(DtmTransId);
	info.hash = dtm_cid_hash_fn;
	info.match = dtm_cid_match_fn;
	gcid2lcid = ShmemInitHash("gcid2lcid",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

#ifdef READS_FROM_GCID_MAP
	info.keysize = sizeof(cid_t);
	info.entrysize = sizeof(List*);
	info.hash = dtm_cid_hash_fn;
	info.match = dtm_cid_match_fn;
	reads_from_gcid = ShmemInitHash("reads_from_gcid",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);
#endif

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(DtmTransStatus);
	info.hash = dtm_tid_hash_fn;
	info.match = dtm_tid_match_fn;
	tid2status = ShmemInitHash("tid2status",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

	RegisterTransactionVisibilityCallback(DtmVisibilityCheck);
	local = (DtmNodeState*)ShmemInitStruct("dtm", sizeof(DtmNodeState), &found);
	if (!found)
	{
		local->last_gcid = INVALID_CID;
        local->local_cid = 0;
#ifndef READS_FROM_GCID_MAP
        local->reads_from_gcid = INVALID_INDEX;
#endif
        local->free_index = 0;
        for (i = 0; i < DTM_MAX_PARALLEL_DIST_TRANS-1; i++) {
            local->trans[i].next = i + 1;
        }
        local->trans[DTM_MAX_PARALLEL_DIST_TRANS-1].next = INVALID_INDEX;
		SpinLockInit(&local->lock);
	}
}

static void dtm_alloc_trans(DtmTransState* x)
{
    Assert(local->free_index != INVALID_INDEX);
    x->global = &local->trans[local->free_index];
    local->free_index = x->global->next;
}

void DtmLocalBegin(DtmTransState* x)
{
    if (!x->global) { 
        SpinLockAcquire(&local->lock);
        {
            dtm_alloc_trans(x);
            x->tid = GetCurrentTransactionId();
            x->is_global = false;
            x->is_prepared = false;
            x->global->cid = 0;
            x->local_snapshot = local->local_cid;	
            DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %llu\n", x->tid, x->local_snapshot));
#ifdef READS_FROM_GCID_MAP
            {
                bool found;
                List **p_reads_from_current_gcid = (List**)hash_search(reads_from_gcid, &local->last_gcid, HASH_ENTER, &found);
                if (!found) {
                    *p_reads_from_current_gcid = NIL; // initialize as an empty list
                }
                *p_reads_from_current_gcid = lappend(*p_reads_from_current_gcid, x);
            }
#else
            /* include in L1 list of transactions reading from global CID */
            x->global->next = local->reads_from_gcid;
            local->reads_from_gcid = x->global - local->trans;
#endif

            x->global->cid_range[LOW] = local->last_gcid;
            x->global->cid_range[UP] = MAX_CID;
        }
        SpinLockRelease(&local->lock);
    }
}

void DtmLocalPrepare(DtmTransState* x, cid_t newGcid)
{
	Assert(x->is_global);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId* id;
        size_t i;    
        DtmTransStatus* ts;

		x->local_cid = ++local->local_cid;
        x->is_prepared = true;
		x->global->cid = newGcid;
		id = (DtmTransId*)hash_search(gcid2lcid, &x->global->cid, HASH_ENTER, NULL);
		id->cid = x->local_cid;
		id->tid = x->tid;

		ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_ENTER, NULL);
        ts->status = XID_INPROGRESS;
        ts->csn = x->local_cid;

#ifdef READS_FROM_GCID_MAP
		{
			List **p_reads_from_current_gcid = (List**)hash_search(reads_from_gcid, &local->last_gcid, HASH_FIND, NULL);
			if (p_reads_from_current_gcid) {
				ListCell *c;
				foreach(c, *p_reads_from_current_gcid) {
					((DtmGlobalTransState*)c->data.ptr_value)->cid_range[UP] = newGcid;
				}
			}
		}
#else
        DTM_TRACE((stderr, "DtmLocalPrepare: prepare transaction %u with GCID %llu\n", x->tid, newGcid));
        for (i = local->reads_from_gcid; i != INVALID_INDEX; i = local->trans[i].next) { 
            DTM_TRACE((stderr, "DtmLocalPrepare: update upper range for transaction %llu from %llu to %llu\n", local->trans[i].cid, local->trans[i].cid_range[UP], newGcid));
            local->trans[i].cid_range[UP] = newGcid;
        }    
#endif
		local->last_gcid = newGcid;
	}
	SpinLockRelease(&local->lock);
}

/* called by coordiantor at second stage of 2PC */
void DtmLocalCommit(DtmTransState* x)
{
	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_FIND, NULL);
		if (ts) {
            DTM_TRACE((stderr, "Global ransaction %u is committed\n", x->tid));
			ts->status = XID_COMMITTED;
		} else { 
            DTM_TRACE((stderr, "Local transaction %u is committed\n", x->tid));
        }            
	}
	SpinLockRelease(&local->lock);
}

/* called by coordinator at second stage of 2PC */
void DtmLocalAbort(DtmTransState* x)
{
	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_FIND, NULL);
		if (ts) {
            DTM_TRACE((stderr, "Global transaction %u is aborted\n", x->tid));
			ts->status = XID_ABORTED;
		} else { 
            DTM_TRACE((stderr, "Local transaction %u is aborted\n", x->tid));
        }
	}
	SpinLockRelease(&local->lock);
}

/* Should be called both committed and aborted local and global transactions */
void DtmLocalEnd(DtmTransState* x)
{
    if (x->global != NULL) { 
        size_t i, j, k;
        i = x->global - local->trans;
        x->global = NULL;
        prepared_gcid = INVALID_CID;
        SpinLockAcquire(&local->lock);
        {
            /* Exclude from L1 list */
            for (j = local->reads_from_gcid, k = INVALID_INDEX; i != j && j != INVALID_INDEX; k = j, j = local->trans[j].next);
            if (i == j) { 
                if (k == INVALID_INDEX) { 
                    local->reads_from_gcid = local->trans[i].next;
                } else { 
                    local->trans[k].next = local->trans[i].next;
                }
            }
            /* free global transaction state */
            local->trans[i].next = local->free_index;
            local->free_index = i;
        }
        SpinLockRelease(&local->lock);
    }
}

cid_t DtmLocalExtend(DtmTransState* x)
{
	cid_t gcid_up, gcid_low;
	SpinLockAcquire(&local->lock);
	{
		cid_t* snapshot;
		bool found;

		gcid_up = x->global->cid_range[UP];
		gcid_low = x->global->cid_range[LOW];
		x->is_global = true;

		if (gcid_up == MAX_CID)
		{
			SpinLockRelease(&local->lock);
			while (!dtm_ensure_connection()) {}
			gcid_up = DtmGlobalGetNextCid(dtm_conn);
			DTM_TRACE((stderr, "DtmLocalExtend: transaction %u gets next GCID %llu from DTM\n", x->tid, gcid_up));
			SpinLockAcquire(&local->lock);
		}
		/* If some other transaction is committed while latch was released */
		if (local->last_gcid < gcid_up && local->last_gcid > gcid_low)
		{
			gcid_up = local->last_gcid;
			DTM_TRACE((stderr, "DtmLocalExtend: transaction %u updates gcid_up to %llu\n", x->tid, gcid_up));
		}
		x->global->cid_range[UP] = gcid_up;

		/* Abort tx if  x used another local snapshot in the given interval */
		snapshot = (cid_t*)hash_search(gcidup2lsnap, &gcid_up, HASH_ENTER, &found);
		if (found)
		{
			if (*snapshot > x->local_snapshot)
			{
				DTM_TRACE((stderr, "DtmLocalExtend: transaction %u uses another snapshot %llu for interval %llu with associated snapshot %llu\n",
                           x->tid, x->local_snapshot, gcid_up, *snapshot));
				gcid_up = INVALID_CID;
			}
		}
		else
		{
            DTM_TRACE((stderr, "DtmLocalExtend: transaction %u associates local snapshot %llu with interval %llu\n",
                       x->tid, x->local_snapshot, gcid_up));
			*snapshot = x->local_snapshot;
		}
	}
	SpinLockRelease(&local->lock);
	return gcid_up;
}


/* I am not sure about this function */
static cid_t dtm_find_local_snapshot(cid_t gcid_up)
{
	while (gcid_up <= local->last_gcid)
	{
		DtmTransId* t = (DtmTransId*) hash_search(gcid2lcid, &gcid_up, HASH_FIND, NULL);
		if (t != NULL)
		{
            return t->cid-1;
		}
		gcid_up += 1;
	}
    return local->local_cid;
}

void DtmLocalAccess(DtmTransState* x, cid_t gcid_up)
{
	SpinLockAcquire(&local->lock);
	{
		cid_t* snapshot;
		bool found;
		if (!x->global)
		{
			dtm_alloc_trans(x);
		}
		x->is_global = true;
		x->global->cid_range[UP] = gcid_up;
		/* Use designated local shanshot for global CID range */
		snapshot = (cid_t*)hash_search(gcidup2lsnap, &gcid_up, HASH_ENTER, &found);
		if (!found)
		{
			*snapshot = dtm_find_local_snapshot(gcid_up);            
            DTM_TRACE((stderr, "DtmLocalAccess: transaction %u defines snapshot %llu for interval %llu\n",
                       x->tid, *snapshot, gcid_up));
		} else {
            DTM_TRACE((stderr, "DtmLocalAccess: transaction %u uses snapshot %llu for interval %llu\n",
                       x->tid, *snapshot, gcid_up));
        }
		x->local_snapshot = *snapshot;
	}
	SpinLockRelease(&local->lock);
}
