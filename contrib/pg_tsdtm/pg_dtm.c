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
#include "access/subtrans.h"
#include "access/xlog.h"
#include "access/clog.h"
#include "access/twophase.h"
#include "executor/spi.h"
#include "utils/hsearch.h"
#include "utils/tqual.h"
#include <utils/guc.h>

#include "pg_dtm.h"

#define DTM_HASH_INIT_SIZE	1000000
#define INVALID_CID    0
#define MIN_WAIT_TIMEOUT 1000
#define MAX_WAIT_TIMEOUT 100000
#define MAX_GTID_SIZE  16
#define HASH_PER_ELEM_OVERHEAD 64

#define USEC 1000000

#define TRACE_SLEEP_TIME 1

typedef uint64 timestamp_t;

typedef struct DtmTransStatus
{
	TransactionId xid;
	XidStatus	status;
	int			nSubxids;
	cid_t		cid;
	struct DtmTransStatus *next;
}	DtmTransStatus;

typedef struct
{
	cid_t		cid;
	long		time_shift;
	volatile slock_t lock;
	DtmTransStatus *trans_list_head;
	DtmTransStatus **trans_list_tail;
}	DtmNodeState;

typedef struct
{
	char		gtid[MAX_GTID_SIZE];
	TransactionId xid;
	TransactionId *subxids;
	int			nSubxids;
}	DtmTransId;


#define DTM_TRACE(x)
/* #define DTM_TRACE(x) fprintf x */

static shmem_startup_hook_type prev_shmem_startup_hook;
static HTAB *xid2status;
static HTAB *gtid2xid;
static DtmNodeState *local;
static DtmCurrentTrans dtm_tx;
static uint64 totalSleepInterrupts;
static int	DtmVacuumDelay;
static bool DtmRecordCommits;

static Snapshot DtmGetSnapshot(Snapshot snapshot);
static TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static bool DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);
static TransactionId DtmAdjustOldestXid(TransactionId xid);
static bool DtmDetectGlobalDeadLock(PGPROC *proc);
static cid_t DtmGetCsn(TransactionId xid);
static void DtmAddSubtransactions(DtmTransStatus * ts, TransactionId *subxids, int nSubxids);
static char const *DtmGetName(void);

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

void		_PG_init(void);
void		_PG_fini(void);


static void dtm_shmem_startup(void);
static Size dtm_memsize(void);
static void dtm_xact_callback(XactEvent event, void *arg);
static timestamp_t dtm_get_current_time();
static void dtm_sleep(timestamp_t interval);
static cid_t dtm_get_cid();
static cid_t dtm_sync(cid_t cid);

/*
 *	***************************************************************************
 */


static timestamp_t
dtm_get_current_time()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (timestamp_t) tv.tv_sec * USEC + tv.tv_usec + local->time_shift;
}

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

static cid_t
dtm_sync(cid_t global_cid)
{
	cid_t		local_cid;
#if 1
	while ((local_cid = dtm_get_cid()) < global_cid)
	{
		local->time_shift += global_cid - local_cid;
	}
#else
	while ((local_cid = dtm_get_cid()) < global_cid)
	{
		SpinLockRelease(&local->lock);
#if TRACE_SLEEP_TIME
		{
			timestamp_t now = dtm_get_current_time();
			static timestamp_t firstReportTime;
			static timestamp_t prevReportTime;
			static timestamp_t totalSleepTime;
#endif
			dtm_sleep(global_cid - local_cid);
#if TRACE_SLEEP_TIME
			totalSleepTime += dtm_get_current_time() - now;
			if (now > prevReportTime + USEC)
			{
				prevReportTime = now;
				if (firstReportTime == 0)
				{
					firstReportTime = now;
				}
				else
				{
					fprintf(stderr, "Sync sleep %lu of %lu usec (%f%%)\n", totalSleepTime, now - firstReportTime, totalSleepTime * 100.0 / (now - firstReportTime));
				}
			}
		}
#endif
		SpinLockAcquire(&local->lock);
	}
#endif
	return local_cid;
}

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

	DefineCustomIntVariable(
							"dtm.vacuum_delay",
					"Minimal age of records which can be vacuumed (seconds)",
							NULL,
							&DtmVacuumDelay,
							10,
							1,
							INT_MAX,
							PGC_BACKEND,
							0,
							NULL,
							NULL,
							NULL
		);

	DefineCustomBoolVariable(
							 "dtm.record_commits",
							 "Store information about committed global transactions in pg_committed_xacts table",
							 NULL,
							 &DtmRecordCommits,
							 false,
							 PGC_BACKEND,
							 0,
							 NULL,
							 NULL,
							 NULL
		);


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
	Size		size;

	size = MAXALIGN(sizeof(DtmNodeState));
	size = add_size(size, (sizeof(DtmTransId) + sizeof(DtmTransStatus) + HASH_PER_ELEM_OVERHEAD * 2) * DTM_HASH_INIT_SIZE);

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
	if (prev_shmem_startup_hook)
	{
		prev_shmem_startup_hook();
	}
	DtmInitialize();
}

static GlobalTransactionId
dtm_get_global_trans_id()
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
			DtmLocalSavePreparedState(dtm_get_global_trans_id());
			DtmLocalEnd(&dtm_tx);
			break;

		default:
			break;
	}
}

/*
 *	***************************************************************************
 */

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dtm_extend);
PG_FUNCTION_INFO_V1(dtm_access);
PG_FUNCTION_INFO_V1(dtm_begin_prepare);
PG_FUNCTION_INFO_V1(dtm_prepare);
PG_FUNCTION_INFO_V1(dtm_end_prepare);
PG_FUNCTION_INFO_V1(dtm_get_csn);

Datum
dtm_extend(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
	cid_t		cid = DtmLocalExtend(&dtm_tx, gtid);

	DTM_TRACE((stderr, "Backend %d extends transaction %u(%s) to global with cid=%lu\n", getpid(), dtm_tx.xid, gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
dtm_access(PG_FUNCTION_ARGS)
{
	cid_t		cid = PG_GETARG_INT64(0);
	GlobalTransactionId gtid = PG_GETARG_CSTRING(1);

	DTM_TRACE((stderr, "Backend %d joins transaction %u(%s) with cid=%lu\n", getpid(), dtm_tx.xid, gtid, cid));
	cid = DtmLocalAccess(&dtm_tx, gtid, cid);
	PG_RETURN_INT64(cid);
}

Datum
dtm_begin_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = PG_GETARG_CSTRING(0);

	DtmLocalBeginPrepare(gtid);
	DTM_TRACE((stderr, "Backend %d begins prepare of transaction %s\n", getpid(), gtid));
	PG_RETURN_VOID();
}

Datum
dtm_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
	cid_t		cid = PG_GETARG_INT64(1);

	cid = DtmLocalPrepare(gtid, cid);
	DTM_TRACE((stderr, "Backend %d prepares transaction %s with cid=%lu\n", getpid(), gtid, cid));
	PG_RETURN_INT64(cid);
}

Datum
dtm_end_prepare(PG_FUNCTION_ARGS)
{
	GlobalTransactionId gtid = PG_GETARG_CSTRING(0);
	cid_t		cid = PG_GETARG_INT64(1);

	DTM_TRACE((stderr, "Backend %d ends prepare of transactions %s with cid=%lu\n", getpid(), gtid, cid));
	DtmLocalEndPrepare(gtid, cid);
	PG_RETURN_VOID();
}

Datum
dtm_get_csn(PG_FUNCTION_ARGS)
{
	TransactionId xid = PG_GETARG_INT32(0);
	cid_t		csn = DtmGetCsn(xid);

	PG_RETURN_INT64(csn);
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

static TransactionId
DtmAdjustOldestXid(TransactionId xid)
{
	if (TransactionIdIsValid(xid))
	{
		DtmTransStatus *ts,
				   *prev = NULL;
		timestamp_t now = dtm_get_current_time();
		timestamp_t cutoff_time = now - DtmVacuumDelay * USEC;

		SpinLockAcquire(&local->lock);
		ts = (DtmTransStatus *) hash_search(xid2status, &xid, HASH_FIND, NULL);
		if (ts != NULL)
		{
			cutoff_time = ts->cid - DtmVacuumDelay * USEC;

			for (ts = local->trans_list_head; ts != NULL && ts->cid < cutoff_time; prev = ts, ts = ts->next)
			{
				if (prev != NULL)
				{
					/*
					 * intf(stderr, "Remove xid %d from hash at %lu\n",
					 * prev->xid, now);
					 */
					hash_search(xid2status, &prev->xid, HASH_REMOVE, NULL);
				}
			}
		}
		if (prev != NULL)
		{
			local->trans_list_head = prev;
			xid = prev->xid;
		}
		else
		{
			xid = FirstNormalTransactionId;
		}
		SpinLockRelease(&local->lock);
	}
	return xid;
}

Snapshot
DtmGetSnapshot(Snapshot snapshot)
{
	snapshot = PgGetSnapshotData(snapshot);
	RecentGlobalDataXmin = RecentGlobalXmin = DtmAdjustOldestXid(RecentGlobalDataXmin);
	return snapshot;
}

TransactionId
DtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
	TransactionId xmin = PgGetOldestXmin(rel, ignoreVacuum);

	xmin = DtmAdjustOldestXid(xmin);
	return xmin;
}

bool
DtmXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot)
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
	if (firstReportTime == 0)
	{
		firstReportTime = dtm_get_current_time();
	}
#endif
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
			if (ts->status == TRANSACTION_STATUS_IN_PROGRESS)
			{
				DTM_TRACE((stderr, "%d: wait for in-doubt transaction %u in snapshot %lu\n", getpid(), xid, dtm_tx.snapshot));
				SpinLockRelease(&local->lock);
#if TRACE_SLEEP_TIME
				{
					timestamp_t delta,
								now = dtm_get_current_time();
#endif
					dtm_sleep(delay);
#if TRACE_SLEEP_TIME
					delta = dtm_get_current_time() - now;
					totalSleepTime += delta;
					if (delta > maxSleepTime)
					{
						maxSleepTime = delta;
					}
					if (now > prevReportTime + USEC * 10)
					{
						prevReportTime = now;
						if (firstReportTime == 0)
						{
							firstReportTime = now;
						}
						else
						{
							fprintf(stderr, "Snapshot sleep %lu of %lu usec (%f%%), maximum=%lu\n", totalSleepTime, now - firstReportTime, totalSleepTime * 100.0 / (now - firstReportTime), maxSleepTime);
						}
					}
				}
#endif
				if (delay * 2 <= MAX_WAIT_TIMEOUT)
				{
					delay *= 2;
				}
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
}


void
DtmLocalBegin(DtmCurrentTrans * x)
{
	if (!TransactionIdIsValid(x->xid))
	{
		SpinLockAcquire(&local->lock);
		x->xid = GetCurrentTransactionId();
		Assert(TransactionIdIsValid(x->xid));
		x->cid = INVALID_CID;
		x->is_global = false;
		x->is_prepared = false;
		x->snapshot = dtm_get_cid();
		SpinLockRelease(&local->lock);
		DTM_TRACE((stderr, "DtmLocalBegin: transaction %u uses local snapshot %lu\n", x->xid, x->snapshot));
	}
}

cid_t
DtmLocalExtend(DtmCurrentTrans * x, GlobalTransactionId gtid)
{
	if (gtid != NULL)
	{
		SpinLockAcquire(&local->lock);
		{
			DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_ENTER, NULL);

			id->xid = x->xid;
			id->nSubxids = 0;
			id->subxids = 0;
		}
		SpinLockRelease(&local->lock);
	}
	x->is_global = true;
	return x->snapshot;
}

cid_t
DtmLocalAccess(DtmCurrentTrans * x, GlobalTransactionId gtid, cid_t global_cid)
{
	cid_t		local_cid;

	SpinLockAcquire(&local->lock);
	{
		if (gtid != NULL)
		{
			DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_ENTER, NULL);

			id->xid = x->xid;
			id->nSubxids = 0;
			id->subxids = 0;
		}
		local_cid = dtm_sync(global_cid);
		x->snapshot = global_cid;
		x->is_global = true;
	}
	SpinLockRelease(&local->lock);
	if (global_cid < local_cid - DtmVacuumDelay * USEC)
	{
		elog(ERROR, "Too old snapshot: requested %ld, current %ld", global_cid, local_cid);
	}
	return global_cid;
}

void
DtmLocalBeginPrepare(GlobalTransactionId gtid)
{
	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus *ts;
		DtmTransId *id;

		id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_FIND, NULL);
		Assert(id != NULL);
		Assert(TransactionIdIsValid(id->xid));
		ts = (DtmTransStatus *) hash_search(xid2status, &id->xid, HASH_ENTER, NULL);
		ts->status = TRANSACTION_STATUS_IN_PROGRESS;
		ts->cid = dtm_get_cid();
		ts->nSubxids = id->nSubxids;
		DtmTransactionListAppend(ts);
		DtmAddSubtransactions(ts, id->subxids, id->nSubxids);
	}
	SpinLockRelease(&local->lock);
}

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

void
DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid)
{
	SpinLockAcquire(&local->lock);
	{
		DtmTransStatus *ts;
		DtmTransId *id;
		int			i;

		id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_FIND, NULL);
		Assert(id != NULL);

		ts = (DtmTransStatus *) hash_search(xid2status, &id->xid, HASH_FIND, NULL);
		Assert(ts != NULL);
		ts->cid = cid;
		for (i = 0; i < ts->nSubxids; i++)
		{
			ts = ts->next;
			ts->cid = cid;
		}
		dtm_sync(cid);

		DTM_TRACE((stderr, "Prepare transaction %u(%s) with CSN %lu\n", id->xid, gtid, cid));
	}
	SpinLockRelease(&local->lock);
	if (DtmRecordCommits)
	{
		char		stmt[MAX_GTID_SIZE + 64];
		int			rc;

		sprintf(stmt, "insert into pg_committed_xacts values ('%s')", gtid);
		SPI_connect();
		rc = SPI_execute(stmt, true, 0);
		SPI_finish();
		if (rc != SPI_OK_INSERT)
		{
			elog(ERROR, "Failed to insert GTID %s in table pg_committed_xacts", gtid);
		}
	}
}

void
DtmLocalCommitPrepared(DtmCurrentTrans * x, GlobalTransactionId gtid)
{
	Assert(gtid != NULL);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_REMOVE, NULL);

		Assert(id != NULL);

		x->is_global = true;
		x->is_prepared = true;
		x->xid = id->xid;
		free(id->subxids);

		DTM_TRACE((stderr, "Global transaction %u(%s) is precommitted\n", x->xid, gtid));
	}
	SpinLockRelease(&local->lock);
}

void
DtmLocalCommit(DtmCurrentTrans * x)
{
	SpinLockAcquire(&local->lock);
	if (TransactionIdIsValid(x->xid))
	{
		bool		found;
		DtmTransStatus *ts;

		ts = (DtmTransStatus *) hash_search(xid2status, &x->xid, HASH_ENTER, &found);
		ts->status = TRANSACTION_STATUS_COMMITTED;
		if (x->is_prepared)
		{
			int			i;
			DtmTransStatus *sts = ts;

			Assert(found);
			Assert(x->is_global);
			for (i = 0; i < ts->nSubxids; i++)
			{
				sts = sts->next;
				Assert(sts->cid == ts->cid);
				sts->status = TRANSACTION_STATUS_COMMITTED;
			}
		}
		else
		{
			TransactionId *subxids;

			Assert(!found);
			ts->cid = dtm_get_cid();
			DtmTransactionListAppend(ts);
			ts->nSubxids = xactGetCommittedChildren(&subxids);
			DtmAddSubtransactions(ts, subxids, ts->nSubxids);
		}
		x->cid = ts->cid;
		DTM_TRACE((stderr, "Local transaction %u is committed at %lu\n", x->xid, x->cid));
	}
	SpinLockRelease(&local->lock);
}

void
DtmLocalAbortPrepared(DtmCurrentTrans * x, GlobalTransactionId gtid)
{
	Assert(gtid != NULL);

	SpinLockAcquire(&local->lock);
	{
		DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_REMOVE, NULL);

		Assert(id != NULL);

		x->is_global = true;
		x->is_prepared = true;
		x->xid = id->xid;
		free(id->subxids);

		DTM_TRACE((stderr, "Global transaction %u(%s) is preaborted\n", x->xid, gtid));
	}
	SpinLockRelease(&local->lock);
}

void
DtmLocalAbort(DtmCurrentTrans * x)
{
	SpinLockAcquire(&local->lock);
	{
		bool		found;
		DtmTransStatus *ts;

		Assert(TransactionIdIsValid(x->xid));
		ts = (DtmTransStatus *) hash_search(xid2status, &x->xid, HASH_ENTER, &found);
		if (x->is_prepared)
		{
			Assert(found);
			Assert(x->is_global);
		}
		else
		{
			Assert(!found);
			ts->cid = dtm_get_cid();
			ts->nSubxids = 0;
			DtmTransactionListAppend(ts);
		}
		x->cid = ts->cid;
		ts->status = TRANSACTION_STATUS_ABORTED;
		DTM_TRACE((stderr, "Local transaction %u is aborted at %lu\n", x->xid, x->cid));
	}
	SpinLockRelease(&local->lock);
}

void
DtmLocalEnd(DtmCurrentTrans * x)
{
	x->is_global = false;
	x->is_prepared = false;
	x->xid = InvalidTransactionId;
	x->cid = INVALID_CID;
}

bool
DtmDetectGlobalDeadLock(PGPROC *proc)
{
	elog(WARNING, "Global deadlock?");
	return true;
}

static cid_t
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

void
DtmLocalSavePreparedState(GlobalTransactionId gtid)
{
	if (gtid != NULL)
	{
		SpinLockAcquire(&local->lock);
		{
			DtmTransId *id = (DtmTransId *) hash_search(gtid2xid, gtid, HASH_FIND, NULL);

			if (id != NULL)
			{
				TransactionId *subxids;
				int			nSubxids = xactGetCommittedChildren(&subxids);

				if (nSubxids != 0)
				{
					id->subxids = (TransactionId *) malloc(nSubxids * sizeof(TransactionId));
					id->nSubxids = nSubxids;
					memcpy(id->subxids, subxids, nSubxids * sizeof(TransactionId));
				}
			}
		}
		SpinLockRelease(&local->lock);
	}
}

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
