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
#include "tcop/utility.h"
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
static HTAB *gtid2xid;
static DtmNodeState *local;
static uint64 totalSleepInterrupts;
static int	DtmVacuumDelay = 2; /* sec */

DtmCurrentTrans dtm_tx; // XXXX: make static

static ProcessUtility_hook_type process_utility_hook_next = NULL;
static char *prepared_xact_event_gid = NULL;

static bool DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static void DtmAdjustOldestXid(void);
static bool DtmDetectGlobalDeadLock(PGPROC *proc);
static void DtmAddSubtransactions(DtmTransStatus * ts, TransactionId *subxids, int nSubxids);
static char const *DtmGetName(void);
static size_t DtmGetTransactionStateSize(void);
static void DtmSerializeTransactionState(void* ctx);
static void DtmDeserializeTransactionState(void* ctx);
static void global_snapshot_utility_hook(PlannedStmt *pstmt,
										 const char *queryString,
										 ProcessUtilityContext context,
										 ParamListInfo params,
										 QueryEnvironment *queryEnv,
										 DestReceiver *dest, char *completionTag);



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
	timestamp_t waketm = dtm_get_current_time() + interval;
	while (1948)
	{
		timestamp_t sleepfor = waketm - dtm_get_current_time();

		pg_usleep(sleepfor);
		if (dtm_get_current_time() < waketm)
		{
			totalSleepInterrupts += 1;
			Assert(errno == EINTR);
			continue;
		}
		break;
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
	size = add_size(size, (sizeof(DtmTransId) + sizeof(DtmTransStatus) + HASH_PER_ELEM_OVERHEAD * 2) * DTM_HASH_INIT_SIZE);

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

		case XACT_EVENT_ABORT:
			DtmLocalAbort(&dtm_tx);
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT:
			DtmLocalCommit(&dtm_tx);
			DtmLocalEnd(&dtm_tx);
			break;

		case XACT_EVENT_ABORT_PREPARED:
			DtmLocalAbortPrepared(&dtm_tx);
			break;

		case XACT_EVENT_COMMIT_PREPARED:
			DtmLocalCommitPrepared(&dtm_tx);
			break;

		case XACT_EVENT_PRE_PREPARE:
			DtmLocalSavePreparedState(&dtm_tx);
			DtmLocalEnd(&dtm_tx);
			break;

		default:
			break;
	}
}

/*
 *	***************************************************************************
 */

static uint32
dtm_xid_hash_fn(const void *key, Size keysize)
{
	return (uint32) *(TransactionId *) key;
}

static int
dtm_xid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId *) key1 - *(TransactionId *) key2;
}

static uint32
dtm_gtid_hash_fn(const void *key, Size keysize)
{
	GlobalTransactionId id = (GlobalTransactionId) key;
	uint32		h = 0;

	while (*id != 0)
	{
		h = h * 31 + *id++;
	}
	return h;
}

static void *
dtm_gtid_keycopy_fn(void *dest, const void *src, Size keysize)
{
	return strcpy((char *) dest, (GlobalTransactionId) src);
}

static int
dtm_gtid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return strcmp((GlobalTransactionId) key1, (GlobalTransactionId) key2);
}

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

	/* remove old entries */
	for (ts = local->trans_list_head; ts != NULL && ts->cid < cutoff_time; prev = ts, ts = ts->next)
	{
		if (prev != NULL)
		{
			hash_search(xid2status, &prev->xid, HASH_REMOVE, NULL);
			deleted++;
		}
	}

	/* fix the list */
	if (prev != NULL)
		local->trans_list_head = prev;

	/* learn and set oldest xid */
	if (local->trans_list_head)
		oldest_xid = local->trans_list_head->xid;

	for (ts = local->trans_list_head; ts != NULL; ts = ts->next)
	{
		if (TransactionIdPrecedes(ts->xid, oldest_xid))
			oldest_xid = ts->xid;
		total++;
	}

	SpinLockRelease(&local->lock);

	DTM_TRACE((stderr, "DtmAdjustOldestXid total=%d, deleted=%d, xid=%d, prev=%p, ts=%p", total, deleted, oldest_xid, prev, ts));

	/* Assert in procarray.c is unhappy if oldest_xid is not quite normal, uh */
	if (TransactionIdIsNormal(oldest_xid))
		ProcArraySetGlobalSnapshotXmin(oldest_xid);
}


/*
 * Check tuple visibility based on CSN of current transaction. If there is no
 * information about transaction with this XID, then use standard PostgreSQL
 * visibility rules.
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
				DTM_TRACE((stderr, "%d: tuple with xid=%d(csn="UINT64_FORMAT") is invisibile in snapshot "UINT64_FORMAT"\n",
						   getpid(), xid, ts->cid, dtm_tx.snapshot));
				SpinLockRelease(&local->lock);
				return true;
			}
			if (ts->status == TRANSACTION_STATUS_IN_PROGRESS)
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

				DTM_TRACE((stderr, "%d: tuple with xid=%d(csn= "UINT64_FORMAT") is %s in snapshot "UINT64_FORMAT"\n",
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

	process_utility_hook_next = ProcessUtility_hook;
	ProcessUtility_hook	= global_snapshot_utility_hook;
}

/*
 * Start transaction at local node.
 * Associate local snapshot (current time) with this transaction.
 */
void
DtmLocalBegin(DtmCurrentTrans * x)
{
	if (!TransactionIdIsValid(x->xid))
	{
		SpinLockAcquire(&local->lock);
		x->snapshot = dtm_get_cid();
		SpinLockRelease(&local->lock);
		DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %lu\n", x->xid, x->snapshot));
	}
}

/*
 * Transaction is going to be distributed.
 * Returns snapshot of current transaction.
 */
cid_t
DtmLocalExtend(DtmCurrentTrans * x, GlobalTransactionId gtid)
{
	return x->snapshot;
}

/*
 * This function is executed on all nodes joining distributed transaction.
 * global_cid is snapshot taken from node who initiated this transaction.
 */
cid_t
DtmLocalAccess(DtmCurrentTrans * x, GlobalTransactionId gtid, cid_t global_cid)
{
	cid_t		local_cid;

	/* Pull the clock hand until coordinator's snapshot time will come */
	SpinLockAcquire(&local->lock);
	{
		local_cid = dtm_sync(global_cid);
	}
	SpinLockRelease(&local->lock);

	/* Assign the snapshot */
	x->snapshot = global_cid;

	if (global_cid < local_cid - DtmVacuumDelay * USEC)
	{
		elog(ERROR, "Too old snapshot: requested %ld, current %ld", global_cid, local_cid);
	}
	return global_cid;
}

/*
 * Set transaction status to in-doubt. Now all transactions who potentially
 * might see tuples updated by this transaction have to wait until it is
 * either committed or aborted. This is only needed for distributed xacts. If
 * gid is empty string, we work with current xact, otherwise search in
 * gtid2xact for prepared one.
 */
void
DtmLocalBeginPrepare(GlobalTransactionId gtid)
{
	DtmTransStatus *ts;
	DtmTransId *id;
	TransactionId xid;
	int nSubxids;
	TransactionId *subxids;
	SpinLockAcquire(&local->lock);

	/* Previously prepared transaction */
	if (gtid[0])
	{
		id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_FIND, NULL);
		if (id == NULL)
		{
			elog(DEBUG1, "Failed to set xact %s status to in-doubt: its xid not found", gtid);
			goto end;
		}
		xid = id->xid;
		nSubxids = id->nSubxids;
		subxids = id->subxids;
	}
	else { /* Actual current transaction, materialize xid */
		TransactionId *subxids;

		nSubxids = xactGetCommittedChildren(&subxids);
		xid = GetCurrentTransactionId();
	}

	ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, NULL);
	Assert(TransactionIdIsValid(id->xid));
	/* in-doubt status */
	ts->status = TRANSACTION_STATUS_IN_PROGRESS;
	ts->cid = dtm_get_cid();
	ts->nSubxids = nSubxids;
	DtmAddSubtransactions(ts, subxids, nSubxids);
	DtmTransactionListAppend(ts);

end:
	SpinLockRelease(&local->lock);
}

/*
 * Choose maximal CSN among all nodes. This function returns maximum of passed
 * (global) and local (current time) CSNs.
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
 * Adjust system time according to the received maximal CSN. If gid is empty
 * string, we work with current xact, otherwise search in gtid2xact for
 * prepared one.
 */
void
DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid)
{
	DtmTransStatus *ts;
	DtmTransId *id;
	int			i;
	TransactionId xid;

	SpinLockAcquire(&local->lock);

	/* Previously prepared transaction */
	if (gtid[0])
	{
		id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_FIND, NULL);
		if (id == NULL)
		{
			/*
			 * If xact is not found in gtid2xid, either reboot occured since
			 * it was prepared (in which case it is safe to use PG rules) or
			 * user violated the protocol: gtid should be empty string if xact
			 * wasn't prepared.
			 */
			elog(DEBUG1, "Failed to set prepared xact %s csn: its xid not found", gtid);
			goto end;
		}
		xid = id->xid;
	}
	else { /* Actual current transaction */
		xid = GetCurrentTransactionId();
	}

	ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_FIND, NULL);
	Assert(ts != NULL);
	ts->cid = cid;
	for (i = 0; i < ts->nSubxids; i++)
	{
		ts = ts->next;
		ts->cid = cid;
	}
	dtm_sync(cid);

	DTM_TRACE((stderr, "DtmLocalEndPrepare %u(%s) with CSN %lu\n", xid, gtid, cid));
end:
	SpinLockRelease(&local->lock);
}

/*
 * Commit prepared transaction
 */
void
DtmLocalCommitPrepared(DtmCurrentTrans * x)
{
	Assert(prepared_xact_event_gid != NULL);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, prepared_xact_event_gid,
													HASH_FIND, NULL);
		DtmTransStatus *ts;
		int			i;
		DtmTransStatus *sts;

		/*
		 * If id is not found, reboot had occured since prepare; it is safe to
		 * use PG rules.
		 */
		if (id != NULL)
		{
			Assert(TransactionIdIsValid(id->xid));

			ts = (DtmTransStatus *) hash_search(xid2status, &id->xid, HASH_FIND, NULL);
			/*
			 * If ts is not found, user is not interested in global snapshot,
			 * that's fine.
			 */
			if (ts != NULL)
			{
				/* Mark transaction as committed */
				ts->status = TRANSACTION_STATUS_COMMITTED;
				sts = ts;
				for (i = 0; i < ts->nSubxids; i++)
				{
					sts = sts->next;
					Assert(sts->cid == ts->cid);
					sts->status = TRANSACTION_STATUS_COMMITTED;
				}

				DTM_TRACE((stderr, "Prepared transaction %u(%s) is committed\n", id->xid,
						   prepared_xact_event_gid));
				/* Remove xact from gtid2xid */
				if (id->nSubxids != 0)
					free(id->subxids);
				hash_search(gtid2xid, prepared_xact_event_gid, HASH_REMOVE, NULL);
			}
		}
	}
	SpinLockRelease(&local->lock);

	DtmAdjustOldestXid();
}

/*
 *  Set currently executing transaction status to committed
 */
void
DtmLocalCommit(DtmCurrentTrans * x)
{
	TransactionId xid = GetCurrentTransactionIdIfAny();

	/*
	 * Xid is valid, add it to xid2csn map (if not yet) and mark as comitted
	 */
	if (TransactionIdIsValid(xid))
	{
		bool		found;
		DtmTransStatus *ts;

		SpinLockAcquire(&local->lock);

		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, &found);
		ts->status = TRANSACTION_STATUS_COMMITTED;
		if (found)
		{
			/* xid is already there: global transaction */
			int			i;
			DtmTransStatus *sts = ts;

			for (i = 0; i < ts->nSubxids; i++)
			{
				sts = sts->next;
				Assert(sts->cid == ts->cid);
				sts->status = TRANSACTION_STATUS_COMMITTED;
			}
		}
		else
		{
			/* local transaction, receive csn */
			TransactionId *subxids;

			ts->cid = dtm_get_cid();
			DtmTransactionListAppend(ts);
			ts->nSubxids = xactGetCommittedChildren(&subxids);
			DtmAddSubtransactions(ts, subxids, ts->nSubxids);
		}

		SpinLockRelease(&local->lock);

		DTM_TRACE((stderr, "Transaction %u is committed\n", xid));
		DtmAdjustOldestXid();
	}
}

/*
 * Abort prepared transaction
 */
void
DtmLocalAbortPrepared(DtmCurrentTrans * x)
{
	Assert(prepared_xact_event_gid != NULL);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, prepared_xact_event_gid,
													HASH_FIND, NULL);
		DtmTransStatus *ts;
		int			i;
		DtmTransStatus *sts;

		/*
		 * If id is not found, reboot had occured since prepare; it is safe to
		 * use PG rules.
		 */
		if (id != NULL)
		{
			Assert(TransactionIdIsValid(id->xid));

			ts = (DtmTransStatus *) hash_search(xid2status, &id->xid, HASH_FIND, NULL);
			/*
			 * If ts is not found, user is not interested in global snapshot,
			 * that's fine.
			 */
			if (ts != NULL)
			{
				/* Mark transaction as aborted */
				ts->status = TRANSACTION_STATUS_ABORTED;
				sts = ts;
				for (i = 0; i < ts->nSubxids; i++)
				{
					sts = sts->next;
					Assert(sts->cid == ts->cid);
					sts->status = TRANSACTION_STATUS_ABORTED;
				}

				/* Remove xact from gtid2xid */
				if (id->nSubxids != 0)
					free(id->subxids);
				hash_search(gtid2xid, prepared_xact_event_gid, HASH_REMOVE, NULL);
				DTM_TRACE((stderr, "Prepared transaction %u(%s) is aborted\n", x->xid, prepared_xact_event_gid));
			}
		}
	}
	SpinLockRelease(&local->lock);
}

/*
 * Set currently executing transaction status to aborted
 */
void
DtmLocalAbort(DtmCurrentTrans * x)
{
	TransactionId xid = GetCurrentTransactionIdIfAny();

	/*
	 * Xid is valid, add it to xid2csn map (if not yet) and mark as aborted
	 */
	if (TransactionIdIsValid(xid))
	{
		bool		found;
		DtmTransStatus *ts;

		SpinLockAcquire(&local->lock);

		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_ENTER, &found);
		ts->status = TRANSACTION_STATUS_ABORTED;
		if (found)
		{
			/* xid is already there: global transaction */
			int			i;
			DtmTransStatus *sts = ts;

			for (i = 0; i < ts->nSubxids; i++)
			{
				sts = sts->next;
				Assert(sts->cid == ts->cid);
				sts->status = TRANSACTION_STATUS_ABORTED;
			}
		}
		else
		{
			/* local transaction, receive csn */
			TransactionId *subxids;

			ts->cid = dtm_get_cid();
			DtmTransactionListAppend(ts);
			ts->nSubxids = xactGetCommittedChildren(&subxids);
			DtmAddSubtransactions(ts, subxids, ts->nSubxids);
		}

		SpinLockRelease(&local->lock);

		DTM_TRACE((stderr, "Transaction %u is aborted\n", xid));
	}
}

/*
 * Cleanup dtm_tx structure
 */
void
DtmLocalEnd(DtmCurrentTrans * x)
{
	x->snapshot = INVALID_CID;
}

/*
 * Not implemented here.
 */
bool
DtmDetectGlobalDeadLock(PGPROC *proc)
{
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
 * The xact is going to be prepared. To be able to learn its xid quickly
 * during COMMIT PREPARED / ABORT PREPARED, save its gid into gid2xid map.
 */
void
DtmLocalSavePreparedState(DtmCurrentTrans * x)
{
	Assert(prepared_xact_event_gid != NULL);
	elog(WARNING, "preparing gid '%s', my pid %d", prepared_xact_event_gid, getpid());

	SpinLockAcquire(&local->lock);
	{
		DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, prepared_xact_event_gid, HASH_ENTER, NULL);

		TransactionId *subxids;
		int			nSubxids = xactGetCommittedChildren(&subxids);

		/* materialize xid */
		id->xid = GetCurrentTransactionId();
		if (nSubxids != 0)
		{
			id->subxids = (TransactionId *) malloc(nSubxids * sizeof(TransactionId));
			id->nSubxids = nSubxids;
			memcpy(id->subxids, subxids, nSubxids * sizeof(TransactionId));
		}
	}
	SpinLockRelease(&local->lock);
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
 * If COMMIT PREPARED or ABORT PREPARED arrived, save gid to learn what are
 * working with.
 */
void
global_snapshot_utility_hook(PlannedStmt *pstmt,
							 const char *queryString,
							 ProcessUtilityContext context,
							 ParamListInfo params,
							 QueryEnvironment *queryEnv,
							 DestReceiver *dest, char *completionTag)
{
	Node	   *parsetree = pstmt->utilityStmt;

	if (nodeTag(parsetree) == T_TransactionStmt)
	{
		TransactionStmt *stmt = (TransactionStmt *) parsetree;
		if (stmt->kind == TRANS_STMT_PREPARE ||
			stmt->kind == TRANS_STMT_COMMIT_PREPARED ||
			stmt->kind == TRANS_STMT_ROLLBACK_PREPARED)
		{
			elog(WARNING, "preparing/commiting/aborting gid %s", stmt->gid);
			prepared_xact_event_gid = stmt->gid;
		}
	}

	(process_utility_hook_next ? process_utility_hook_next : standard_ProcessUtility)
		(pstmt, queryString, context, params, queryEnv, dest, completionTag);

	/*
	 * Unfortunately we can't reset prepared_xact_event_gid here, because
	 * PREPARE (and its callbacks) is called from the following
	 * CommitTransactionCommand, not from process utility hook
	 */
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
	cid_t		cid = DtmLocalExtend(&dtm_tx, gtid);

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
