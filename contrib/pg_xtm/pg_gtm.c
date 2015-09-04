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
#include "access/twophase.h"
#include "utils/hsearch.h"
#include "utils/tqual.h"

#include "libdtm.h"
#include "pg_dtm.h"

#define DTM_HASH_INIT_SIZE  1024
#define FIRST_CID      1
#define INVALID_CID    0

typedef struct
{	cid_t last_gcid;
    cid_t local_cid;
	volatile slock_t lock;
} DtmNodeState;

typedef struct
{
	cid_t cid;
	TransactionId tid;
} DtmTransId;

typedef struct
{
    cid_t global_cid;
    cid_t local_cid;
} DtmTransMap;

typedef struct
{
    TransactionIdStatus status;
    cid_t cid;
} DtmTransStatus;

//#define DTM_TRACE(x) 
#define DTM_TRACE(x) fprintf x

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static HTAB* gcid2lcid; // map<cid, cid>
static HTAB* tid2status;

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
static void pg_dtm_ensure_connection(void);

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

	RequestAddinShmemSpace(pg_dtm_memsize());
	RequestAddinLWLocks(1);

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = pg_dtm_shmem_startup;
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
            DtmLocalAbort(&dtm_tx);
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_ABORT_PREPARED:
            if (prepared_gcid != INVALID_CID) { 
				DTM_TRACE((stderr, "Backend %d abort prepared transaction %llu\n", getpid(), prepared_gcid));
				pg_dtm_ensure_connection();
				DtmGlobalRollback(dtm_conn, prepared_gcid);
			}
			break;

		case XACT_EVENT_COMMIT:
            DtmLocalCommit(&dtm_tx);
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT_PREPARED:
            if (prepared_gcid != INVALID_CID) { 
				DTM_TRACE((stderr, "Backend %d commit prepared transaction %llu\n", getpid(), prepared_gcid));
				pg_dtm_ensure_connection();
				DtmGlobalCommit(dtm_conn, prepared_gcid);
			}
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
    pg_dtm_ensure_connection();
    prepared_gcid = DtmGlobalPrepare(dtm_conn);
	DTM_TRACE((stderr, "Backend %d receive prepared GCID %llu\n", getpid(), prepared_gcid));
	PG_RETURN_INT64(prepared_gcid);
}

/*
 *  ***************************************************************************
 */
static void pg_dtm_ensure_connection(void)
{
    while (true) { 
        if (dtm_conn) {
            break;
        }        
        dtm_conn = DtmConnect("127.0.0.1", 5431);
    }
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
	VisibilityCheckResult result = XID_IN_DOUBT;
    if (dtm_tx.is_global) { 
        Assert(xid != InvalidTransactionId);
        SpinLockAcquire(&local->lock);
        while (true)
        {
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &xid, HASH_FIND, NULL);
            if (ts != NULL)
            {
                *hintstatus = ts->status;
                if (ts->cid > dtm_tx.local_snapshot) { 
                    result = XID_INVISIBLE;
                    break;
                }
                if (ts->status == XID_INPROGRESS)
                {
                    //uint64 oldCount = local->count;
                    //uint64 newCount;                        
                    DTM_TRACE((stderr, "Wait for in-doubt transaction %u\n", xid));                    
                    SpinLockRelease(&local->lock);
                    //XactLockTableWait(xid, NULL, NULL, XLTW_None);
                    pg_usleep(100000);
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
    }
	SpinLockRelease(&local->lock);
	return result;
}


void DtmInitialize()
{
	bool found;
	static HASHCTL info;

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
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	local = (DtmNodeState*)ShmemInitStruct("dtm", sizeof(DtmNodeState), &found);
	if (!found)
	{
		local->last_gcid = FIRST_CID;
        local->local_cid = 0;
		SpinLockInit(&local->lock);
	}
	LWLockRelease(AddinShmemInitLock);

	/*
	 * Initialize tx callbacks
	 */
	RegisterXactCallback(pg_dtm_xact_callback, NULL);
}


void DtmLocalBegin(DtmTransState* x)
{
    if (x->tid == InvalidTransactionId) { 
        SpinLockAcquire(&local->lock);
        x->tid = GetCurrentTransactionId();
        x->is_global = false;
        x->is_prepared = false;
        x->global_cid = INVALID_CID;
        x->local_snapshot = local->local_cid;	
        x->global_snapshot = local->last_gcid;
        DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %llu\n", x->tid, x->local_snapshot));
        SpinLockRelease(&local->lock);
    }
}

void DtmLocalPrepare(DtmTransState* x, cid_t newGcid)
{
	Assert(x->is_global);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId* id;
        DtmTransStatus* ts;

		x->local_cid = ++local->local_cid;
        x->is_prepared = true;
		x->global_cid = newGcid;
		id = (DtmTransId*)hash_search(gcid2lcid, &x->global_cid, HASH_ENTER, NULL);
		id->cid = x->local_cid;
		id->tid = x->tid;

		ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_ENTER, NULL);
        ts->status = XID_INPROGRESS;
        ts->cid = x->local_cid;

	}
	SpinLockRelease(&local->lock);
}

/* called by coordiantor at second stage of 2PC */
void DtmLocalCommit(DtmTransState* x)
{
    if (x->is_prepared) { 
        SpinLockAcquire(&local->lock);
        {
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_FIND, NULL);
            Assert(ts);
            DTM_TRACE((stderr, "Global transaction %u is committed\n", x->tid));
			ts->status = XID_COMMITTED;
            local->last_gcid = x->global_cid;
        }
        SpinLockRelease(&local->lock);
    } else { 
        DTM_TRACE((stderr, "Local transaction %u is committed\n", x->tid));
    }
}

/* called by coordinator at second stage of 2PC */
void DtmLocalAbort(DtmTransState* x)
{
    if (x->is_prepared) { 
        SpinLockAcquire(&local->lock);
        {
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &x->tid, HASH_FIND, NULL);
            Assert(ts);
            DTM_TRACE((stderr, "Global transaction %u is aborted\n", x->tid));
			ts->status = XID_ABORTED;
        }
        SpinLockRelease(&local->lock);
    } else { 
        DTM_TRACE((stderr, "Local transaction %u is aborted\n", x->tid));
    }
}

/* Should be called both committed and aborted local and global transactions */
void DtmLocalEnd(DtmTransState* x)
{
    x->is_global = false;
    x->is_prepared = false;
    x->tid = InvalidTransactionId;
    prepared_gcid = INVALID_CID;
}

cid_t DtmLocalExtend(DtmTransState* x)
{
    x->is_global = true;
    return x->global_snapshot;
}


/* I am not sure about this function */
static cid_t dtm_find_local_snapshot(cid_t gcid)
{
    if (gcid == FIRST_CID) { 
        return local->local_cid;
    } else { 
        DtmTransId* t = (DtmTransId*) hash_search(gcid2lcid, &gcid, HASH_FIND, NULL);
        TransactionId xid;
        cid_t cid;
        Assert(t != NULL);
        xid = t->tid;
        cid = t->cid;
        while (true) { 
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(tid2status, &xid, HASH_FIND, NULL);
            Assert (ts != NULL);
            if (ts->status == XID_INPROGRESS) {
                SpinLockRelease(&local->lock);
                pg_usleep(100000);
                SpinLockAcquire(&local->lock);
            } else { 
                break;
            }
        }
        return cid;
    }
}

void DtmLocalAccess(DtmTransState* x, cid_t gcid)
{
	SpinLockAcquire(&local->lock);
	{
		x->is_global = true;
		x->global_snapshot = gcid;
        x->local_snapshot = dtm_find_local_snapshot(gcid);             
	}
	SpinLockRelease(&local->lock);
}
