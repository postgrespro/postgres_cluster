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
#include "access/xtm.h"
#include "access/transam.h"
#include "access/xlog.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "access/twophase.h"
#include <utils/guc.h>
#include "utils/hsearch.h"
#include "utils/tqual.h"
#include "utils/array.h"
#include "utils/builtins.h"

#include "libdtm.h"

void _PG_init(void);
void _PG_fini(void);

static void DtmEnsureConnection(void);
static Snapshot DtmGetSnapshot(Snapshot snapshot);
static void DtmCopySnapshot(Snapshot dst, Snapshot src);
static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static bool TransactionIsInDtmSnapshot(TransactionId xid);

static NodeId DtmNodeId;

static DTMConn DtmConn;
static SnapshotData DtmSnapshot = {HeapTupleSatisfiesMVCC};
static bool DtmHasSnapshot = false;
static TransactionManager DtmTM = { DtmGetTransactionStatus, DtmSetTransactionStatus, DtmGetSnapshot };
static DTMConn DtmConn;

static void DtmEnsureConnection(void)
{
    while (true) { 
        if (DtmConn) {
            break;
        }        
        DtmConn = DtmConnect("127.0.0.1", 5431);
    }
}

static void DtmCopySnapshot(Snapshot dst, Snapshot src)
{
    DtmInitSnapshot(dst);
    memcpy(dst->xip, src->xip, src->xcnt*sizeof(TransactionId));
    dst->xmax = src->xmax;
    dst->xmin = src->xmin;
    dst->xcnt = src->xcnt;
    dst->curcid = src->curcid;
}

static Snapshot DtmGetSnapshot(Snapshot snapshot)
{
    if (DtmHasSnapshot) { 
        DtmCopySnapshot(snapshot, &DtmSnapshot);
        return snapshot;
    }
    return GetLocalSnapshotData(snapshot);
}

static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn)
{
    XidStatus status = CLOGTransactionIdGetStatus(xid, lsn);
    if (status == TRANSACTION_STATUS_IN_PROGRESS) { 
#if 0
        && DtmHasSnapshot && !TransactionIdIsInProgress(xid)) { 
        unsigned delay = 1000;
        while (true) { 
            DtmEnsureConnection();    
            status = DtmGlobalGetTransStatus(DtmConn, DtmNodeId, xid);
            if (status == TRANSACTION_STATUS_IN_PROGRESS) { 
                pg_usleep(delay);
                if (delay < 100000)  { 
                    delay *= 2;
                }
            } else { 
                break;
            }
        }
#endif
        status = DtmGlobalGetTransStatus(DtmConn, DtmNodeId, xid);
        if (status != TRANSACTION_STATUS_IN_PROGRESS) { 
            CLOGTransactionIdSetTreeStatus(xid, 0, NULL, status, InvalidXLogRecPtr);
        }
    }
    return status;
}

static bool TransactionIsInDtmSnapshot(TransactionId xid)
{
    return bsearch(&xid, DtmSnapshot.xip, DtmSnapshot.xcnt,
                   sizeof(TransactionId), xidComparator) != NULL;
}



static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
    if (DtmHasSnapshot) { 
        /* Already should be IN_PROGRESS */
        /* CLOGTransactionIdSetTreeStatus(xid, nsubxids, subxids, TRANSACTION_STATUS_IN_PROGRESS, lsn); */
        if (status == TRANSACTION_STATUS_COMMITTED) { 
            ProcArrayAdd(&ProcGlobal->allProcs[AllocGXid(xid)]);
        }
        DtmHasSnapshot = false;
        DtmEnsureConnection();
        if (!DtmGlobalSetTransStatus(DtmConn, DtmNodeId, xid, status) && status != TRANSACTION_STATUS_ABORTED) { 
            elog(ERROR, "DTMD failed to set transaction status");
        }
    } else {
        CLOGTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
    }
}


/*
 *  ***************************************************************************
 */

extern bool (*TransactionIsInCurrentSnapshot)(TransactionId xid);

void
_PG_init(void)
{
    TM = &DtmTM;
    // TransactionIsInCurrentSnapshot = TransactionIsInDtmSnapshot;

	DefineCustomIntVariable("dtm.node_id",
                            "Identifier of node in distributed cluster for DTM",
							NULL,
							&DtmNodeId,
							0,
							0, 
                            INT_MAX,
							PGC_BACKEND,
							0,
							NULL,
							NULL,
							NULL);
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
}

/*
 *  ***************************************************************************
 */

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dtm_global_transaction);
PG_FUNCTION_INFO_V1(dtm_get_snapshot);

Datum
dtm_global_transaction(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid;
    ArrayType* nodes = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType* xids = PG_GETARG_ARRAYTYPE_P(1);
    gtid.xids = (TransactionId*)ARR_DATA_PTR(xids);
    gtid.nodes = (NodeId*)ARR_DATA_PTR(nodes);
    gtid.nNodes = ArrayGetNItems(ARR_NDIM(nodes), ARR_DIMS(nodes));    
    DtmEnsureConnection();
    DtmGlobalStartTransaction(DtmConn, &gtid);
	PG_RETURN_VOID();
}

        

       

    

Datum
dtm_get_snapshot(PG_FUNCTION_ARGS)
{
    TransactionId xmin;
    DtmEnsureConnection();
    DtmGlobalGetSnapshot(DtmConn, DtmNodeId, GetCurrentTransactionId(), &DtmSnapshot);

    VacuumProcArray(&DtmSnapshot);

    /* Move it to DtmGlobalGetSnapshot? */
    xmin = DtmSnapshot.xmin;
    if (xmin != InvalidTransactionId) { 
        xmin -= vacuum_defer_cleanup_age;
        if (!TransactionIdIsNormal(xmin)) {
            xmin = FirstNormalTransactionId;
        }
        if (RecentGlobalDataXmin > xmin) { 
            RecentGlobalDataXmin = xmin;
        }
        if (RecentGlobalXmin > xmin) { 
            RecentGlobalXmin = xmin;
        }
        RecentXmin = xmin;
    }   
	DtmSnapshot.curcid = GetCurrentCommandId(false);
    DtmHasSnapshot = true;
	PG_RETURN_VOID();
}

