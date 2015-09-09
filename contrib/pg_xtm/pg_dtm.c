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
static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
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

static Snapshot DtmGetSnapshot(Snapshot snapshot)
{
    if (DtmHasSnapshot) { 
        return &DtmSnapshot;
    }
    return GetLocalSnapshotData(snapshot);
}

static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn)
{
    XidStatus status = CLOGTransactionIdGetStatus(xid, lsn);
    if (status == TRANSACTION_STATUS_IN_PROGRESS) { 
        DtmEnsureConnection();    
        status = DtmGlobalGetTransStatus(DtmConn, DtmNodeId, xid);
        CLOGTransactionIdSetTreeStatus(xid, 0, NULL, status, InvalidXLogRecPtr);
    }
    return status;
}


static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
    CLOGTransactionIdSetTreeStatus(xid, nsubxids, subxids, TRANSACTION_STATUS_IN_PROGRESS, lsn); 
    DtmHasSnapshot = false;
    DtmEnsureConnection();
    if (DtmGlobalSetTransStatus(DtmConn, DtmNodeId, xid, status)) { 
        elog(ERROR, "DTMD failed to set transaction status");
    }
}


/*
 *  ***************************************************************************
 */

void
_PG_init(void)
{
    TM = &DtmTM;

	DefineCustomIntVariable("dtm.node.id",
                            "Identifier of node in distributed cluster for DTM",
							NULL,
							&DtmNodeId,
							0,
							0, 
                            INT_MAX,
							PGC_POSTMASTER,
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

