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

#define MIN_DELAY  10000
#define MAX_DELAY 100000

void _PG_init(void);
void _PG_fini(void);

static void DtmEnsureConnection(void);
static Snapshot DtmGetSnapshot(Snapshot snapshot);
static void DtmCopySnapshot(Snapshot dst, Snapshot src);
static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static XidStatus DtmGetGloabalTransStatus(TransactionId xid);
static void DtmUpdateRecentXmin(void);
// static bool IsInDtmSnapshot(TransactionId xid);
static bool DtmTransactionIsInProgress(TransactionId xid);

static NodeId DtmNodeId;
static DTMConn DtmConn;
static SnapshotData DtmSnapshot = { HeapTupleSatisfiesMVCC };
static bool DtmHasSnapshot = false;
static bool DtmGlobalTransaction = false;
static TransactionManager DtmTM = { DtmGetTransactionStatus, DtmSetTransactionStatus, DtmGetSnapshot, DtmTransactionIsInProgress };
static DTMConn DtmConn;

#define XTM_TRACE(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#define XTM_CONNECT_ATTEMPTS 10

static void DtmEnsureConnection(void)
{
    int attempt = 0;
    XTM_TRACE("XTM: DtmEnsureConnection\n");
    while (attempt < XTM_CONNECT_ATTEMPTS) { 
        if (DtmConn) {
            break;
        }
        XTM_TRACE("XTM: DtmEnsureConnection, attempt #%u\n", attempt);
        DtmConn = DtmConnect("127.0.0.1", 5431);
        attempt++;
    }
}

static void DumpSnapshot(Snapshot s, char *name)
{
	int i;
	char buf[10000];
	char *cursor = buf;
	cursor += sprintf(
		cursor,
		"snapshot %s: xmin=%d, xmax=%d, active=[",
		name, s->xmin, s->xmax
	);
	for (i = 0; i < s->xcnt; i++) {
		if (i == 0) {
			cursor += sprintf(cursor, "%d", s->xip[i]);
		} else {
			cursor += sprintf(cursor, ", %d", s->xip[i]);
		}
	}
	cursor += sprintf(cursor, "]");
	XTM_TRACE("%s\n", buf);
}

static void DtmCopySnapshot(Snapshot dst, Snapshot src)
{
    int i, j, n;
    static TransactionId* buf;
    TransactionId prev = InvalidTransactionId;

    XTM_TRACE("XTM: DtmCopySnapshot for transaction%u\n", GetCurrentTransactionId());
    DumpSnapshot(dst, "local");
    DumpSnapshot(src, "DTM");

    if (buf == NULL) { 
        buf = (TransactionId *)malloc(GetMaxSnapshotXidCount() * sizeof(TransactionId) * 2);
    }    

    GetLocalSnapshotData(dst);

    if (dst->xmin > src->xmin) { 
        dst->xmin = src->xmin;
    }
    if (dst->xmax > src->xmax) { 
        dst->xmax = src->xmax;
    }
    
    memcpy(buf, dst->xip, dst->xcnt*sizeof(TransactionId));
    memcpy(buf + dst->xcnt, src->xip, src->xcnt*sizeof(TransactionId));
    qsort(buf, dst->xcnt + src->xcnt, sizeof(TransactionId), xidComparator);    
    for (i = 0, j = 0, n = dst->xcnt + src->xcnt; i < n && buf[i] < dst->xmax; i++) { 
        if (buf[i] != prev) { 
            dst->xip[j++] = prev = buf[i];
        }
    }
    dst->xcnt = j;
    DumpSnapshot(dst, "Merged");
}

static void DtmUpdateRecentXmin(void)
{
    TransactionId xmin = DtmSnapshot.xmin;

    XTM_TRACE("XTM: DtmUpdateRecentXmin \n");

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
        //RecentXmin = xmin;
    }   
}

static Snapshot DtmGetSnapshot(Snapshot snapshot)
{
    XTM_TRACE("XTM: DtmGetSnapshot \n");
    snapshot = GetLocalSnapshotData(snapshot);        
    if (DtmHasSnapshot) {  
        DtmCopySnapshot(snapshot, &DtmSnapshot);
        //DtmUpdateRecentXmin();
    }
    return snapshot;
}

static bool IsInDtmSnapshot(TransactionId xid)
{
    return DtmHasSnapshot
        && (xid >= DtmSnapshot.xmax 
            || bsearch(&xid, DtmSnapshot.xip, DtmSnapshot.xcnt, sizeof(TransactionId), xidComparator) != NULL);
}

static bool DtmTransactionIsInProgress(TransactionId xid)
{
#if 0
    if (IsInDtmSnapshot(xid)) { 
        unsigned delay = MIN_DELAY;
        XLogRecPtr lsn;
        while (CLOGTransactionIdGetStatus(xid, &lsn) == TRANSACTION_STATUS_IN_PROGRESS) { 
            pg_usleep(delay);
            if (delay < MAX_DELAY)  { 
                delay *= 2;
            }
        }
        return false;
    }
#endif
    XTM_TRACE("XTM: DtmTransactionIsInProgress \n");
    return TransactionIdIsRunning(xid);// || IsInDtmSnapshot(xid);
}

static XidStatus DtmGetGloabalTransStatus(TransactionId xid)
{
    XTM_TRACE("XTM: DtmGetGloabalTransStatus \n");
    while (true) { 
        XidStatus status;
        DtmEnsureConnection();
        status = DtmGlobalGetTransStatus(DtmConn, DtmNodeId, xid, true);
        if (status == TRANSACTION_STATUS_IN_PROGRESS) {
		elog(ERROR, "DTMD reported status in progress");
        } else { 
            return status;
        }
    }
}

static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn)
{
#if 0
    if (IsInDtmSnapshot(xid)) {
        return TRANSACTION_STATUS_IN_PROGRESS;
    }
#endif
    XidStatus status = CLOGTransactionIdGetStatus(xid, lsn);
    XTM_TRACE("XTM: DtmGetTransactionStatus \n");
#if 0
    if (status == TRANSACTION_STATUS_IN_PROGRESS) { 
        status = DtmGetGloabalTransStatus(xid);
        if (status == TRANSACTION_STATUS_UNKNOWN) { 
            status = TRANSACTION_STATUS_IN_PROGRESS;
        }
    }
#endif
    return status;
}


static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
    XTM_TRACE("XTM: DtmSetTransactionStatus %u = %u \n", xid, status);
    if (!RecoveryInProgress()) { 
        if (DtmGlobalTransaction) { 
            /* Already should be IN_PROGRESS */
            /* CLOGTransactionIdSetTreeStatus(xid, nsubxids, subxids, TRANSACTION_STATUS_IN_PROGRESS, lsn); */

            DtmHasSnapshot = false;
            DtmGlobalTransaction = false;
            DtmEnsureConnection();
            if (!DtmGlobalSetTransStatus(DtmConn, DtmNodeId, xid, status) && status != TRANSACTION_STATUS_ABORTED) { 
                elog(ERROR, "DTMD failed to set transaction status");
            }
            status = DtmGetGloabalTransStatus(xid);
            Assert(status == TRANSACTION_STATUS_ABORTED || status == TRANSACTION_STATUS_COMMITTED);
        } else {
            elog(WARNING, "Set transaction %u status in local CLOG" , xid);
        }
    } else { 
        XidStatus gs = DtmGetGloabalTransStatus(xid);
        if (gs != TRANSACTION_STATUS_UNKNOWN) { 
            status = gs;
        }
    }
    CLOGTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
}

/*
 *  ***************************************************************************
 */

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

PG_FUNCTION_INFO_V1(dtm_begin_transaction);
PG_FUNCTION_INFO_V1(dtm_get_snapshot);

Datum
dtm_begin_transaction(PG_FUNCTION_ARGS)
{
    GlobalTransactionId gtid;
    ArrayType* nodes = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType* xids = PG_GETARG_ARRAYTYPE_P(1);
    gtid.xids = (TransactionId*)ARR_DATA_PTR(xids);
    gtid.nodes = (NodeId*)ARR_DATA_PTR(nodes);
    gtid.nNodes = ArrayGetNItems(ARR_NDIM(nodes), ARR_DIMS(nodes));    
    DtmGlobalTransaction = true;
    XTM_TRACE("XTM: dtm_begin_transaction \n");
    if (DtmNodeId == gtid.nodes[0]) { 
        DtmEnsureConnection();
        DtmGlobalStartTransaction(DtmConn, &gtid);
    }
	PG_RETURN_VOID();
}

Datum
dtm_get_snapshot(PG_FUNCTION_ARGS)
{
    DtmEnsureConnection();
    DtmGlobalGetSnapshot(DtmConn, DtmNodeId, GetCurrentTransactionId(), &DtmSnapshot);

    XTM_TRACE("XTM: dtm_get_snapshot \n");

    /* Move it to DtmGlobalGetSnapshot? */
    Assert(!DtmHasSnapshot);
    DtmHasSnapshot = true;
    DtmGlobalTransaction = true;
	PG_RETURN_VOID();
}

