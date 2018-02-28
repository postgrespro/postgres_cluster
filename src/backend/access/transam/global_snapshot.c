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
#include "access/global_snapshot.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/procarray.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/xlogdefs.h"
#include "access/xact.h"
#include "access/xtm.h"
#include "access/transam.h"
#include "access/subtrans.h"
#include "access/xlog.h"
#include "access/clog.h"
#include "access/twophase.h"
#include "executor/spi.h"
#include "utils/hsearch.h"
#include <utils/guc.h>
#include "utils/tqual.h"
#include "utils/builtins.h"

#define DTM_HASH_INIT_SIZE	1000000
#define INVALID_CID    0
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000
#define HASH_PER_ELEM_OVERHEAD 64

#define USEC 1000000

#define TRACE_SLEEP_TIME 1

typedef uint64 timestamp_t;

/* Distributed transaction state kept in shared memory */
typedef struct DtmTransStatus
{
	TransactionId xid;
	XidStatus	status;
	int			nSubxids;
	cid_t		cid;			/* CSN */
	struct DtmTransStatus *next;/* pointer to next element in finished
								 * transaction list */
}	DtmTransStatus;

/* State of DTM node */
typedef struct
{
	cid_t		cid;			/* last assigned CSN; used to provide unique
								 * ascending CSNs */
	long		time_shift;		/* correction to system time */
	volatile slock_t lock;		/* spinlock to protect access to hash table  */
	DtmTransStatus *trans_list_head;	/* L1 list of finished transactions
										 * present in xid2status hash table.
										 * This list is used to perform
										 * cleanup of too old transactions */
	DtmTransStatus **trans_list_tail;
}	DtmNodeState;

/* Structure used to map global transaction identifier to XID */
typedef struct
{
	char		gtid[MAX_GTID_SIZE];
	TransactionId xid;
	TransactionId *subxids;
	int			nSubxids;
}	DtmTransId;


#define DTM_TRACE(x)
/* #define DTM_TRACE(x) fprintf x */

// static shmem_startup_hook_type prev_shmem_startup_hook;
static HTAB *xid2status;
static DtmNodeState *local;
static uint64 totalSleepInterrupts;
static int	DtmVacuumDelay = 15; /* sec */
static bool finishing_prepared;


DtmCurrentTrans dtm_tx; // XXXX: make static

static bool DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static void DtmAdjustOldestXid(void);
static void DtmInitGlobalXmin(TransactionId xid);
static bool DtmDetectGlobalDeadLock(PGPROC *proc);
static void DtmAddSubtransactions(DtmTransStatus * ts, TransactionId *subxids, int nSubxids);
static void DtmAdjustSubtransactions(DtmTransStatus *ts);
static char const *DtmGetName(void);
static size_t DtmGetTransactionStateSize(void);
static void DtmSerializeTransactionState(void* ctx);
static void DtmDeserializeTransactionState(void* ctx);

static void DtmLocalFinish(bool is_commit);

static TransactionManager DtmTM = {
	PgTransactionIdGetStatus,
	PgTransactionIdSetTreeStatus,
	PgGetSnapshotData,
	PgGetNewTransactionId,
	PgGetOldestXmin,
	PgTransactionIdIsInProgress,
	PgGetGlobalTransactionId,
	DtmXidInMVCCSnapshot,
	DtmDetectGlobalDeadLock,
	DtmGetName,
	DtmGetTransactionStateSize,
	DtmSerializeTransactionState,
	DtmDeserializeTransactionState,
	PgInitializeSequence
};

void		_PG_init(void);
void		_PG_fini(void);


// static void dtm_shmem_startup(void);
static void dtm_xact_callback(XactEvent event, void *arg);
static timestamp_t dtm_get_current_time();
static void dtm_sleep(timestamp_t interval);
static cid_t dtm_get_cid();
static cid_t dtm_sync(cid_t cid);

/*
 *	Time manipulation functions
 */

/* Get current time with microscond resolution */
static timestamp_t
dtm_get_current_time()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (timestamp_t) tv.tv_sec * USEC + tv.tv_usec + local->time_shift;
}

/* Sleep for specified amount of time */
static void
dtm_sleep(timestamp_t interval)
{
	struct timespec ts;
	struct timespec rem;

	ts.tv_sec = 0;
	ts.tv_nsec = interval * 1000;

	while (nanosleep(&ts, &rem) < 0)
	{
		totalSleepInterrupts += 1;
		Assert(errno == EINTR);
		ts = rem;
	}
}

/* Get unique ascending CSN.
 * This function is called inside critical section
 */
static cid_t
dtm_get_cid()
{
	cid_t		cid = dtm_get_current_time();

	if (cid <= local->cid)
	{
		cid = ++local->cid;
	}
	else
	{
		local->cid = cid;
	}
	return cid;
}

/*
 * Adjust system time
 */
static cid_t
dtm_sync(cid_t global_cid)
{
	cid_t		local_cid;

	while ((local_cid = dtm_get_cid()) < global_cid)
	{
		local->time_shift += global_cid - local_cid;
	}
	return local_cid;
}

/*
 * Estimate shared memory space needed.
 */
Size
GlobalSnapshotShmemSize(void)
{
	Size		size;

	size = MAXALIGN(sizeof(DtmNodeState));
	size = add_size(size, DTM_HASH_INIT_SIZE *
			(sizeof(DtmTransId) + sizeof(DtmTransStatus) + HASH_PER_ELEM_OVERHEAD * 2));

	return size;
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

		case XACT_EVENT_ABORT_PREPARED:
			finishing_prepared = true;
			DtmAdjustOldestXid();
			break;

		case XACT_EVENT_COMMIT_PREPARED:
			finishing_prepared = true;
			DtmAdjustOldestXid();
			break;

		case XACT_EVENT_COMMIT:
			DtmLocalFinish(true);
			finishing_prepared = false;
			break;

		case XACT_EVENT_ABORT:
			DtmLocalFinish(false);
			finishing_prepared = false;
			break;

		case XACT_EVENT_PRE_PREPARE:
			DtmLocalSavePreparedState(&dtm_tx);
			break;

		default:
			break;
	}
}

/*
 *	***************************************************************************
 */

static char const *
DtmGetName(void)
{
	return "pg_tsdtm";
}

static void
DtmTransactionListAppend(DtmTransStatus * ts)
{
	ts->next = NULL;
	*local->trans_list_tail = ts;
	local->trans_list_tail = &ts->next;
}

static void
DtmTransactionListInsertAfter(DtmTransStatus * after, DtmTransStatus * ts)
{
	ts->next = after->next;
	after->next = ts;
	if (local->trans_list_tail == &after->next)
	{
		local->trans_list_tail = &ts->next;
	}
}

static void 
DtmAdjustSubtransactions(DtmTransStatus *ts)
{
	int i;
	int nSubxids = ts->nSubxids;
	DtmTransStatus* sts = ts;

	for (i = 0; i < nSubxids; i++) {
		sts = sts->next;
		sts->status = ts->status;
		Assert(sts->cid == ts->cid);
	}
}

/*
 * There can be different oldest XIDs at different cluster node.
 * Seince we do not have centralized aribiter, we have to rely in DtmVacuumDelay.
 * This function takes XID which PostgreSQL consider to be the latest and try to find XID which
 * is older than it more than DtmVacuumDelay.
 * If no such XID can be located, then return previously observed oldest XID
 */
static void
DtmAdjustOldestXid()
{
	DtmTransStatus *ts,
				   *prev = NULL;
	timestamp_t		cutoff_time;
	TransactionId   oldest_xid = InvalidTransactionId;
	int				total = 0,
					deleted = 0;

	cutoff_time = dtm_get_current_time() - DtmVacuumDelay * USEC;

	SpinLockAcquire(&local->lock);

	for (ts = local->trans_list_head; ts != NULL && ts->cid < cutoff_time; prev = ts, ts = ts->next)
	{
		if (prev != NULL)
		{
			hash_search(xid2status, &prev->xid, HASH_REMOVE, NULL);
			deleted++;
		}
	}

	if (prev != NULL)
		local->trans_list_head = prev;

	if (local->trans_list_head)
		oldest_xid = local->trans_list_head->xid;

	for (ts = local->trans_list_head; ts != NULL; ts = ts->next)
	{
		if (TransactionIdPrecedes(ts->xid, oldest_xid))
			oldest_xid = ts->xid;
		total++;
	}

	SpinLockRelease(&local->lock);

	ProcArraySetGlobalSnapshotXmin(oldest_xid);

	// elog(LOG, "DtmAdjustOldestXid total=%d, deleted=%d, xid=%d, prev=%p, ts=%p", total, deleted, oldest_xid, prev, ts);
}

static void
DtmInitGlobalXmin(TransactionId xmin)
{
	TransactionId current_xmin;

	Assert(TransactionIdIsValid(xmin));

	/* Better change to CAS */
	current_xmin = ProcArrayGetGlobalSnapshotXmin();
	if (!TransactionIdIsValid(current_xmin))
		ProcArraySetGlobalSnapshotXmin(xmin);
}


/*
 * Check tuple bisibility based on CSN of current transaction.
 * If there is no niformation about transaction with this XID, then use standard PostgreSQL visibility rules.
 */
bool
DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot)
{
	timestamp_t delay = MIN_WAIT_TIMEOUT;

	Assert(xid != InvalidTransactionId);

	SpinLockAcquire(&local->lock);

	while (true)
	{
		DtmTransStatus *ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_FIND, NULL);

		if (ts != NULL)
		{
			if (ts->cid > dtm_tx.snapshot)
			{
				DTM_TRACE((stderr, "%d: tuple with xid=%d(csn=%lld) is invisibile in snapshot %lld\n",
						   getpid(), xid, ts->cid, dtm_tx.snapshot));
				SpinLockRelease(&local->lock);
				return true;
			}
			if (ts->status == TRANSACTION_STATUS_UNKNOWN)
			{
				DTM_TRACE((stderr, "%d: wait for in-doubt transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
				SpinLockRelease(&local->lock);

				dtm_sleep(delay);

				if (delay * 2 <= MAX_WAIT_TIMEOUT)
					delay *= 2;
				SpinLockAcquire(&local->lock);
			}
			else
			{
				bool		invisible = ts->status == TRANSACTION_STATUS_ABORTED;

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

void
DtmInitialize()
{
	bool		found;
	static HASHCTL info;

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(DtmTransStatus);
	xid2status = ShmemInitHash("xid2status",
							   DTM_HASH_INIT_SIZE, DTM_HASH_INIT_SIZE,
							   &info,
							   HASH_ELEM | HASH_BLOBS);

	TM = &DtmTM;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	local = (DtmNodeState *) ShmemInitStruct("dtm", sizeof(DtmNodeState), &found);
	if (!found)
	{
		local->time_shift = 0;
		local->cid = dtm_get_current_time();
		local->trans_list_head = NULL;
		local->trans_list_tail = &local->trans_list_head;
		SpinLockInit(&local->lock);
		RegisterXactCallback(dtm_xact_callback, NULL);
	}
	LWLockRelease(AddinShmemInitLock);
}

/*
 * Start transaction at local node.
 * Associate local snapshot (current time) with this transaction.
 */
void
DtmLocalBegin(DtmCurrentTrans * x)
{
	SpinLockAcquire(&local->lock); // XXX: move to snapshot aquire?
	x->snapshot = dtm_get_cid();
	SpinLockRelease(&local->lock);
}

/*
 * Transaction is going to be distributed.
 * Returns snapshot of current transaction.
 */
cid_t
DtmLocalExtend(GlobalTransactionId gtid)
{
	DtmCurrentTrans *x = &dtm_tx;

	if (gtid != NULL)
	{
		strncpy(x->gtid, gtid, MAX_GTID_SIZE);
	}
	DtmInitGlobalXmin(TransactionXmin);
	return x->snapshot;
}

/*
 * This function is executed on all nodes joining distributed transaction.
 * global_cid is snapshot taken from node initiated this transaction
 */
cid_t
DtmLocalAccess(DtmCurrentTrans * x, GlobalTransactionId gtid, cid_t global_cid)
{
	cid_t		local_cid;

	// Check that snapshot isn't set?

	SpinLockAcquire(&local->lock);
	{
		local_cid = dtm_sync(global_cid);
		x->snapshot = global_cid;
	}
	strncpy(x->gtid, gtid, MAX_GTID_SIZE);
	SpinLockRelease(&local->lock);

	if (global_cid < local_cid - DtmVacuumDelay * USEC)
	{
		elog(ERROR, "Too old snapshot: requested %ld, current %ld", global_cid, local_cid);
	}

	DtmInitGlobalXmin(TransactionXmin);
	return global_cid;
}

/*
 * Set transaction status to in-doubt. Now all transactions accessing tuples updated by this transaction have to
 * wait until it is either committed either aborted
 */
void
DtmLocalBeginPrepare(GlobalTransactionId gtid)
{
	TransactionId xid = TwoPhaseGetTransactionId(gtid);

	if (!TransactionIdIsValid(xid))
	{
		// XXX: check that it is global tx with the same xid, XactTopTransactionId?
		xid = GetCurrentTransactionId();
	}

	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus *ts;
		bool found;

		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, &found);
		ts->status = TRANSACTION_STATUS_UNKNOWN;
		ts->cid = dtm_get_cid();
		if (!found)
			ts->nSubxids = 0;
		DtmAdjustSubtransactions(ts);
	}
	SpinLockRelease(&local->lock);
}

/*
 * Choose maximal CSN among all nodes.
 * This function returns maximum of passed (global) and local (current time) CSNs.
 */
cid_t
DtmLocalPrepare(GlobalTransactionId gtid, cid_t global_cid)
{
	cid_t		local_cid;

	SpinLockAcquire(&local->lock);
	local_cid = dtm_get_cid();
	if (local_cid > global_cid)
	{
		global_cid = local_cid;
	}
	SpinLockRelease(&local->lock);
	return global_cid;
}

/*
 * Adjust system time according to the received maximal CSN
 */
void
DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid)
{
	TransactionId xid = TwoPhaseGetTransactionId(gtid);

	if (!TransactionIdIsValid(xid))
	{
		// XXX: check that it is global tx with the same xid, XactTopTransactionId?
		xid = GetCurrentTransactionId();
	}

	dtm_tx.xid = xid;

	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus *ts;
		DtmTransId *id;
		int			i;

		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_FIND, NULL);
		Assert(ts != NULL);
		ts->cid = cid;
		DtmAdjustSubtransactions(ts);
		dtm_sync(cid);

		DTM_TRACE((stderr, "Prepare transaction %u(%s) with CSN %lu\n", id->xid, gtid, cid));
	}
	SpinLockRelease(&local->lock);

}

/*
 * Set transaction status to committed
 */
void
DtmLocalFinish(bool is_commit)
{
	DtmCurrentTrans * x = &dtm_tx;
	TransactionId xid = GetCurrentTransactionIdIfAny();

	if (x->gtid[0] && finishing_prepared)
	{
		xid = x->xid;
	}
	else if (!TransactionIdIsValid(xid))
	{
		return;
	}

	SpinLockAcquire(&local->lock);
	{
		bool		found;
		DtmTransStatus *ts;

		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, &found);
		ts->status = is_commit ? TRANSACTION_STATUS_COMMITTED : TRANSACTION_STATUS_ABORTED;

		if (!found)
		{
			ts->cid = dtm_get_cid();
			ts->nSubxids = 0;
			DtmTransactionListAppend(ts);
		}
		DtmAdjustSubtransactions(ts);
	}
	SpinLockRelease(&local->lock);

	// DtmAdjustOldestXid();
}

/*
 * Now only timestapm based dealock detection is supported for pg_tsdtm.
 * Please adjust "deadlock_timeout" parameter in postresql.conf to avoid false
 * deadlock detection.
 */
bool
DtmDetectGlobalDeadLock(PGPROC *proc)
{
	// elog(WARNING, "Global deadlock?");
	return false;
}

static size_t 
DtmGetTransactionStateSize(void)
{
	return sizeof(dtm_tx);
}

static void
DtmSerializeTransactionState(void* ctx)
{
	memcpy(ctx, &dtm_tx, sizeof(dtm_tx));
}

static void
DtmDeserializeTransactionState(void* ctx)
{
	memcpy(&dtm_tx, ctx, sizeof(dtm_tx));
}


cid_t
DtmGetCsn(TransactionId xid)
{
	cid_t		csn = 0;

	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus *ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_FIND, NULL);

		if (ts != NULL)
		{
			csn = ts->cid;
		}
	}
	SpinLockRelease(&local->lock);
	return csn;
}

/*
 * Save state of parepared transaction
 */
void
DtmLocalSavePreparedState(DtmCurrentTrans * x)
{

	if (x->gtid[0])
	{
		TransactionId *subxids;
		TransactionId xid = GetCurrentTransactionId();
		int			  nSubxids = xactGetCommittedChildren(&subxids);

		SpinLockAcquire(&local->lock);
		{
			DtmTransStatus *ts;

			ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, NULL);
			ts->status = TRANSACTION_STATUS_IN_PROGRESS;
			ts->cid = dtm_get_cid();
			ts->nSubxids = nSubxids;
			DtmTransactionListAppend(ts);
			DtmAddSubtransactions(ts, subxids, nSubxids);
		}
		SpinLockRelease(&local->lock);
	}
}

/*
 * Add subtransactions to finished transactions list.
 * Copy CSN and status of parent transaction.
 */
static void
DtmAddSubtransactions(DtmTransStatus * ts, TransactionId *subxids, int nSubxids)
{
	int			i;

	for (i = 0; i < nSubxids; i++)
	{
		bool		found;
		DtmTransStatus *sts;

		Assert(TransactionIdIsValid(subxids[i]));
		sts = (DtmTransStatus *) hash_search(xid2status, &subxids[i], HASH_ENTER, &found);
		Assert(!found);
		sts->status = ts->status;
		sts->cid = ts->cid;
		sts->nSubxids = 0;
		DtmTransactionListInsertAfter(ts, sts);
	}
}


/*
 *
 * SQL functions for global snapshot mamagement.
 *
 */

Datum
pg_global_snaphot_create(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = text_to_cstring(PG_GETARG_TEXT_PP(0));
	cid_t		cid = DtmLocalExtend(gtid);

	DTM_TRACE((stderr, "Backend %d extends transaction %u(%s) to global with cid=%lu\n", getpid(), dtm_tx.xid, gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
pg_global_snaphot_join(PG_FUNCTION_ARGS)
{
	cid_t		cid = PG_GETARG_INT64(0);
	GlobalTransactionId gtid = text_to_cstring(PG_GETARG_TEXT_PP(1));

	DTM_TRACE((stderr, "Backend %d joins transaction %u(%s) with cid=%lu\n", getpid(), dtm_tx.xid, gtid, cid));
	cid = DtmLocalAccess(&dtm_tx, gtid, cid);
	PG_RETURN_INT64(cid);
}

Datum
pg_global_snaphot_begin_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = text_to_cstring(PG_GETARG_TEXT_PP(0));

	DtmLocalBeginPrepare(gtid);
	DTM_TRACE((stderr, "Backend %d begins prepare of transaction %s\n", getpid(), gtid));
	PG_RETURN_VOID();
}

Datum
pg_global_snaphot_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = text_to_cstring(PG_GETARG_TEXT_PP(0));
	cid_t		cid = PG_GETARG_INT64(1);

	cid = DtmLocalPrepare(gtid, cid);
	DTM_TRACE((stderr, "Backend %d prepares transaction %s with cid=%lu\n", getpid(), gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
pg_global_snaphot_end_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = text_to_cstring(PG_GETARG_TEXT_PP(0));
	cid_t		cid = PG_GETARG_INT64(1);

	DTM_TRACE((stderr, "Backend %d ends prepare of transactions %s with cid=%lu\n", getpid(), gtid, cid));
	DtmLocalEndPrepare(gtid, cid);
	PG_RETURN_VOID();
}
