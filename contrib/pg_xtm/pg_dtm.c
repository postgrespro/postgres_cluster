/*
 * pg_dtm.c
 *
 * Pluggable distributed transaction manager
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

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

#define DTM_HASH_INIT_SIZE  100000
#define INVALID_CID    0
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000
#define MAX_GTID_SIZE  16
#define HASH_PER_ELEM_OVERHEAD 64

typedef uint64 timestamp_t;

typedef enum
{
	XID_COMMITTED,
	XID_ABORTED,
	XID_INPROGRESS
} TransactionIdStatus;

typedef struct 
{
    nodeid_t node_id;
	cid_t last_cid;
	volatile slock_t lock;
} DtmNodeState;

typedef struct
{
    TransactionId xid;
    TransactionIdStatus status;
    bool is_coordinator;
    cid_t cid;
} DtmTransStatus;

typedef struct
{
    char gtid[MAX_GTID_SIZE];
    TransactionId xid;
} DtmTransId;
              
//#define DTM_TRACE(x) 
#define DTM_TRACE(x) fprintf x

static shmem_startup_hook_type prev_shmem_startup_hook;
static HTAB* xid2status;
static HTAB* gtid2xid;
static DtmNodeState* local;
static DtmTransState dtm_tx;
static DTMConn dtm_conn;

void _PG_init(void);
void _PG_fini(void);


static void dtm_shmem_startup(void);
static Size dtm_memsize(void);
static void dtm_xact_callback(XactEvent event, void *arg);
static void dtm_ensure_connection(void);

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

	RequestAddinShmemSpace(dtm_memsize());

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = dtm_shmem_startup;
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
dtm_memsize(void)
{
	Size        size;

	size = MAXALIGN(sizeof(DtmNodeState));
	size = add_size(size, (sizeof(DtmTransId) + sizeof(DtmTransStatus) + HASH_PER_ELEM_OVERHEAD*2)*DTM_HASH_INIT_SIZE);

	return size;
}


/*
 * shmem_startup hook: allocate or attach to shared memory,
 * then load any pre-existing statistics from file.
 * Also create and load the query-texts file, which is expected to exist
 * (even if empty) while the module is enabled.
 */
static void
dtm_shmem_startup(void)
{
	if (prev_shmem_startup_hook) {
		prev_shmem_startup_hook();
    }
	DtmInitialize();
}

static GlobalTransactionId dtm_get_global_trans_id()
{
    return GetLockedGlobalTransactionId();
}

static void
dtm_xact_callback(XactEvent event, void *arg)
{
    DTM_TRACE((stderr, "Backend %d dtm_xact_callback %d\n", getpid(), event));
	switch (event)
	{
		case XACT_EVENT_START:
            DtmLocalBegin(&dtm_tx);
			break;

		case XACT_EVENT_ABORT:
            DtmLocalAbort(&dtm_tx);
            DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT:
            DtmLocalCommit(&dtm_tx);
            DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_ABORT_PREPARED:
            DtmLocalAbortPrepared(&dtm_tx, dtm_get_global_trans_id());
			break;

		case XACT_EVENT_COMMIT_PREPARED:
            DtmLocalCommitPrepared(&dtm_tx, dtm_get_global_trans_id());
			break;

		case XACT_EVENT_PREPARE:
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

PG_FUNCTION_INFO_V1(dtm_register_node);
PG_FUNCTION_INFO_V1(dtm_extend);
PG_FUNCTION_INFO_V1(dtm_access);
PG_FUNCTION_INFO_V1(dtm_begin_prepare);
PG_FUNCTION_INFO_V1(dtm_prepare);
PG_FUNCTION_INFO_V1(dtm_end_prepare);

Datum
dtm_register_node(PG_FUNCTION_ARGS)
{
    local->node_id = PG_GETARG_INT32(0);
	PG_RETURN_VOID();
}

Datum
dtm_extend(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
    cid_t cid = DtmLocalExtend(&dtm_tx, gtid);
	DTM_TRACE((stderr, "Backend %d extends transaction %u(%s) to global with cid=%llu\n", getpid(), dtm_tx.xid, gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
dtm_access(PG_FUNCTION_ARGS)
{
    cid_t cid = PG_GETARG_INT64(0);
    GlobalTransactionId gtid = PG_GETARG_CSTRING(1);
	DTM_TRACE((stderr, "Backend %d joins transaction %u(%s) with cid=%llu\n", getpid(), dtm_tx.xid, gtid, cid));
	cid = DtmLocalAccess(&dtm_tx, gtid, cid);
	PG_RETURN_INT64(cid);
}

Datum
dtm_begin_prepare(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
    nodeid_t coordinator = PG_GETARG_INT32(1);
    DtmLocalBeginPrepare(gtid, coordinator);
	DTM_TRACE((stderr, "Backend %d begins prepare of transaction '%s'\n", getpid(), gtid));
	PG_RETURN_VOID();
}

Datum
dtm_prepare(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
    cid_t cid = PG_GETARG_INT64(1);
    cid = DtmLocalPrepare(gtid, cid);
	DTM_TRACE((stderr, "Backend %d prepares transaction '%s' with cid=%llu\n", getpid(), gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
dtm_end_prepare(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
    cid_t cid = PG_GETARG_INT64(1);
	DTM_TRACE((stderr, "Backend %d ends prepare of transaction '%s' with cid=%llu\n", getpid(), gtid, cid));
	DtmLocalEndPrepare(gtid, cid);
    PG_RETURN_VOID();
}


/*
 *  ***************************************************************************
 */

static void dtm_ensure_connection(void)
{
    while (true) { 
        if (dtm_conn) {
            break;
        }        
        dtm_conn = DtmConnect("127.0.0.1", 5431);
    }
}

static uint32 dtm_xid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int dtm_xid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 - *(TransactionId*)key2;
}

static uint32 dtm_gtid_hash_fn(const void *key, Size keysize)
{
	GlobalTransactionId id = (GlobalTransactionId)key;
    uint32 h = 0;
    while (*id != 0) { 
        h = h*31 + *id++;
    }
    return h;
}

static void* dtm_gtid_keycopy_fn(void *dest, const void *src, Size keysize)
{
	return strcpy((char*)dest, (GlobalTransactionId)src);
}

static int dtm_gtid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return strcmp((GlobalTransactionId)key1, (GlobalTransactionId)key2);
}

static VisibilityCheckResult DtmVisibilityCheck(TransactionId xid)
{
	VisibilityCheckResult result = XID_IN_DOUBT;
    timestamp_t delay = MIN_WAIT_TIMEOUT;

    Assert(xid != InvalidTransactionId);

    SpinLockAcquire(&local->lock);

    while (true)
    {
        DtmTransStatus* ts = (DtmTransStatus*)hash_search(xid2status, &xid, HASH_FIND, NULL);
        if (ts != NULL)
        {
            if (ts->cid > dtm_tx.snapshot) { 
                result = XID_INVISIBLE;
                break;
            }
            if (ts->status == XID_INPROGRESS)
            {
                DTM_TRACE((stderr, "Wait for in-doubt transaction %u\n", xid));
                SpinLockRelease(&local->lock);
                pg_usleep(delay);
                if (delay*2 <= MAX_WAIT_TIMEOUT) {
                    delay *= 2;
                }
                SpinLockAcquire(&local->lock);
            }
            else
            {
                result = ts->status == XID_COMMITTED ? XID_VISIBLE : XID_INVISIBLE;
                DTM_TRACE((stderr, "Backend %d visibility check for %u=%d\n", getpid(), xid, result));
                break;
            }
        }
        else
        {
            //DTM_TRACE((stderr, "Visibility check is skept for transaction %u\n", xid));
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

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(DtmTransStatus);
	info.hash = dtm_xid_hash_fn;
	info.match = dtm_xid_match_fn;
	xid2status = ShmemInitHash("xid2status",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

	info.keysize = MAX_GTID_SIZE;
	info.entrysize = sizeof(DtmTransId);
	info.hash = dtm_gtid_hash_fn;
	info.match = dtm_gtid_match_fn;
	info.keycopy = dtm_gtid_keycopy_fn;
	gtid2xid = ShmemInitHash("gtid2xid",
		DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE | HASH_KEYCOPY);

	RegisterTransactionVisibilityCallback(DtmVisibilityCheck);

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	local = (DtmNodeState*)ShmemInitStruct("dtm", sizeof(DtmNodeState), &found);
	if (!found)
	{
        local->last_cid = INVALID_CID;
        local->node_id = -1;
		SpinLockInit(&local->lock);
	}
	LWLockRelease(AddinShmemInitLock);

	/*
	 * Initialize tx callbacks
	 */
	RegisterXactCallback(dtm_xact_callback, NULL);
}


void DtmLocalBegin(DtmTransState* x)
{
    if (x->xid == InvalidTransactionId) { 
        // SpinLockAcquire(&local->lock);
        x->xid = GetCurrentTransactionId();
        Assert(x->xid != InvalidTransactionId);
        x->cid = INVALID_CID;
        x->is_global = false;
        x->is_prepared = false;
        x->snapshot = local->last_cid;
        // SpinLockRelease(&local->lock);
        DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %llu\n", x->xid, x->snapshot));
    }
}

cid_t DtmLocalExtend(DtmTransState* x, GlobalTransactionId gtid)
{
    if (gtid != NULL) {
        SpinLockAcquire(&local->lock);
        {
            DtmTransId* id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_ENTER, NULL);
            x->is_global = true;
            id->xid = x->xid;
        }
        SpinLockRelease(&local->lock);        
    } else { 
        x->is_global = true;
    }
    return x->snapshot;
}

cid_t DtmLocalAccess(DtmTransState* x, GlobalTransactionId gtid, cid_t cid)
{
    SpinLockAcquire(&local->lock);
    {
        if (gtid != NULL) {
            DtmTransId* id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_ENTER, NULL);
            id->xid = x->xid;
        }
        x->snapshot = cid;
        x->is_global = true;
    }
    SpinLockRelease(&local->lock);
    return cid;
}

void DtmLocalBeginPrepare(GlobalTransactionId gtid, nodeid_t coordinator)
{
    SpinLockAcquire(&local->lock);
    {
        DtmTransStatus* ts;
        DtmTransId* id;

        id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_FIND, NULL);
        Assert(id != NULL);

        ts = (DtmTransStatus*)hash_search(xid2status, &id->xid, HASH_ENTER, NULL);
        ts->status = XID_INPROGRESS;
        ts->cid = INVALID_CID;
        ts->is_coordinator = coordinator == local->node_id;
    }
    SpinLockRelease(&local->lock);
}
   

cid_t DtmLocalPrepare(GlobalTransactionId gtid, cid_t cid)
{
    if (cid == 0) { 
        dtm_ensure_connection();    
        cid = DtmGlobalPrepare(dtm_conn);
    }
    return cid;
}

void DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid)
{
	SpinLockAcquire(&local->lock);
	{
        DtmTransStatus* ts;
        DtmTransId* id;

        id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_FIND, NULL);
        Assert(id != NULL);

        ts = (DtmTransStatus*)hash_search(xid2status, &id->xid, HASH_FIND, NULL);
        Assert(ts != NULL);
        ts->cid = cid;
        
        DTM_TRACE((stderr, "Prepare transaction %u(%s) with CSN %llu\n", id->xid, gtid, cid));
	}
	SpinLockRelease(&local->lock);
}

void DtmLocalCommitPrepared(DtmTransState* x, GlobalTransactionId gtid)
{
    Assert(gtid != NULL);

    SpinLockAcquire(&local->lock);
    {
        DtmTransId* id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_FIND, NULL);
        Assert(id != NULL);

        x->is_prepared = true;
        x->xid = id->xid;
        DTM_TRACE((stderr, "Global transaction %u(%s) is prepared\n", x->xid, gtid));
    }
    SpinLockRelease(&local->lock);
}

void DtmLocalCommit(DtmTransState* x)
{
    if (x->is_prepared) { 
        cid_t gcid = INVALID_CID;

        SpinLockAcquire(&local->lock);
        {
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(xid2status, &x->xid, HASH_FIND, NULL);
            Assert(ts);
            if (ts->cid > local->last_cid) { 
                local->last_cid = ts->cid;
            }
            if (ts->is_coordinator) { 
                gcid = ts->cid;
            }
            ts->status = XID_COMMITTED;
            DTM_TRACE((stderr, "Transaction %u is committed at %llu\n", x->xid, ts->cid));
        }
        SpinLockRelease(&local->lock);
        
        if (gcid != INVALID_CID) { 
            dtm_ensure_connection();
            DtmGlobalCommit(dtm_conn, gcid);
        }
    }
}

void DtmLocalAbortPrepared(DtmTransState* x, GlobalTransactionId gtid)
{
    Assert (gtid != NULL);

    SpinLockAcquire(&local->lock);
    { 
        DtmTransId* id = (DtmTransId*)hash_search(gtid2xid, gtid, HASH_FIND, NULL);
        Assert(id != NULL);

        x->is_global = true;
        x->is_prepared = true;
        x->xid = id->xid;

        DTM_TRACE((stderr, "Global transaction %u(%s) is preaborted\n", x->xid, gtid));
    }
    SpinLockRelease(&local->lock);
}

void DtmLocalAbort(DtmTransState* x)
{
    if (x->is_prepared) { 
        cid_t gcid = INVALID_CID;

        SpinLockAcquire(&local->lock);
        { 
            DtmTransStatus* ts = (DtmTransStatus*)hash_search(xid2status, &x->xid, HASH_FIND, NULL);
            Assert(ts);
            if (ts->is_coordinator) { 
                gcid = ts->cid;
            }
            ts->status = XID_ABORTED;
            DTM_TRACE((stderr, "Local transaction %u is aborted at %llu\n", x->xid, x->cid));
        }
        SpinLockRelease(&local->lock);
        
        if (gcid != INVALID_CID) { 
            dtm_ensure_connection();
            DtmGlobalCommit(dtm_conn, gcid);
        }
    }
}

void DtmLocalEnd(DtmTransState* x)
{
    x->is_global = false;
    x->is_prepared = false;
    x->xid = InvalidTransactionId;
    x->cid = INVALID_CID;
}

