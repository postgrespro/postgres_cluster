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
#include "port/atomics.h"
#include "tcop/utility.h"
#include "sockhub/sockhub.h"

#include "libdtm.h"
#include "multimaster.h"
#include "bgwpool.h"

typedef struct
{
	LWLockId hashLock;
	LWLockId xidLock;
	TransactionId minXid;  /* XID of oldest transaction visible by any active transaction (local or global) */
	TransactionId nextXid; /* next XID for local transaction */
	size_t nReservedXids;  /* number of XIDs reserved for local transactions */
    int    nNodes;
    pg_atomic_uint32 nReceivers;
    bool initialized;
    BgwPool pool;
} DtmState;

typedef struct
{
    TransactionId xid;
    int count;
} LocalTransaction;

#define DTM_SHMEM_SIZE (64*1024*1024)
#define DTM_HASH_SIZE  1003

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mm_start_replication);
PG_FUNCTION_INFO_V1(mm_stop_replication);

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

static bool TransactionIdIsInSnapshot(TransactionId xid, Snapshot snapshot);
static bool TransactionIdIsInDoubt(TransactionId xid);

static void DtmShmemStartup(void);
static void DtmBackgroundWorker(Datum arg);

static void MMMarkTransAsLocal(TransactionId xid);
static BgwPool* MMPoolConstructor(void);
static bool MMRunUtilityStmt(PGconn* conn, char const* sql);

static HTAB* xid_in_doubt;
static HTAB* local_trans;
static DtmState* dtm;

static TransactionId DtmNextXid;
static SnapshotData DtmSnapshot = { HeapTupleSatisfiesMVCC };
static bool DtmHasGlobalSnapshot;
static int DtmLocalXidReserve;
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
    DtmDetectGlobalDeadLock
};

bool  MMDoReplication;
char* MMDatabaseName;

static char* MMConnStrs;
static int   MMNodeId;
static int   MMNodes;
static int   MMQueueSize;
static int   MMWorkers;

static char* DtmHost;
static int   DtmPort;
static int   DtmBufferSize;
static bool  DtmVoted;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;
static ProcessUtility_hook_type PreviousProcessUtilityHook;
static shmem_startup_hook_type PreviousShmemStartupHook;


static void MMExecutorFinish(QueryDesc *queryDesc);
static void MMProcessUtility(Node *parsetree, const char *queryString,
							 ProcessUtilityContext context, ParamListInfo params,
							 DestReceiver *dest, char *completionTag);
static bool MMIsDistributedTrans;

static BackgroundWorker DtmWorker = {
	"sockhub",
	0, /* do not need connection to the database */
	BgWorkerStart_PostmasterStart,
	1, /* restrart in one second (is it possible to restort immediately?) */
	DtmBackgroundWorker
};


static void DumpSnapshot(Snapshot s, char *name)
{
#ifdef DUMP_SNAPSHOT
	int i;
	char buf[10000];
	char *cursor = buf;
	cursor += sprintf(
		cursor,
		"snapshot %s(%p) for transaction %d: xmin=%d, xmax=%d, active=[",
		name, s, GetCurrentTransactionId(), s->xmin, s->xmax
	);
	for (i = 0; i < s->xcnt; i++) {
		if (i == 0) {
			cursor += sprintf(cursor, "%d", s->xip[i]);
		} else {
			cursor += sprintf(cursor, ", %d", s->xip[i]);
		}
	}
	cursor += sprintf(cursor, "]");
	XTM_INFO("%s\n", buf);
#endif
}

/* In snapshots provided by DTMD xip array is sorted, so we can use bsearch */
static bool TransactionIdIsInSnapshot(TransactionId xid, Snapshot snapshot)
{
	return xid >= snapshot->xmax
		|| bsearch(&xid, snapshot->xip, snapshot->xcnt, sizeof(TransactionId), xidComparator) != NULL;
}

/* Transaction is considered as in-doubt if it is globally committed by DTMD but local commit is not yet completed.
 * It can happen because we report DTMD about transaction commit in SetTransactionStatus, which is called inside commit
 * after saving transaction state in WAL but before releasing locks. So DTMD can include this transaction in snapshot
 * before local commit is completed and transaction is marked as completed in local CLOG.
 *
 * We use xid_in_doubt hash table to mark transactions which are "precommitted". Entry is inserted in hash table
 * before seding status to DTMD and removed after receving response from DTMD and setting transaction status in local CLOG.
 * So information about transaction should always present either in xid_in_doubt either in CLOG.
 */
static bool TransactionIdIsInDoubt(TransactionId xid)
{
	bool inDoubt;

	if (!TransactionIdIsInSnapshot(xid, &DtmSnapshot))
	{ /* transaction is completed according to the snaphot */
		LWLockAcquire(dtm->hashLock, LW_SHARED);
		inDoubt = hash_search(xid_in_doubt, &xid, HASH_FIND, NULL) != NULL;
		LWLockRelease(dtm->hashLock);
#if 0 /* We do not need to wait until transaction locks are released, do we? */
		if (!inDoubt)
		{
			XLogRecPtr lsn;
			inDoubt = DtmGetTransactionStatus(xid, &lsn) != TRANSACTION_STATUS_IN_PROGRESS;
		}
#endif
		if (inDoubt)
		{
			XTM_INFO("Wait for transaction %d to complete\n", xid);
			XactLockTableWait(xid, NULL, NULL, XLTW_None);
			return true;
		}
	}
	return false;
}

/* Merge local and global snapshots.
 * Produce most restricted (conservative) snapshot which treate transaction as in-progress if is is marked as in-progress
 * either in local, either in global snapshots
 */
static void DtmMergeWithGlobalSnapshot(Snapshot dst)
{
	int i, j, n;
	TransactionId xid;
	Snapshot src = &DtmSnapshot;

	if (true || !(TransactionIdIsValid(src->xmin) && TransactionIdIsValid(src->xmax))) { 
        PgGetSnapshotData(dst);
        return;
    }

GetLocalSnapshot:
	/*
	 * Check that global and local snapshots are consistent: transactions marked as completed in global snapshot
	 * should be completed locally
	 */
	dst = PgGetSnapshotData(dst);
	for (i = 0; i < dst->xcnt; i++)
		if (TransactionIdIsInDoubt(dst->xip[i]))
			goto GetLocalSnapshot;
	for (xid = dst->xmax; xid < src->xmax; xid++)
		if (TransactionIdIsInDoubt(xid))
			goto GetLocalSnapshot;

    GetCurrentTransactionId();
    DumpSnapshot(dst, "local");
	DumpSnapshot(src, "DTM");

	if (src->xmax < dst->xmax) dst->xmax = src->xmax;
	if (src->xmin < dst->xmin) dst->xmin = src->xmin;
	Assert(src->subxcnt == 0);

	if (src->xcnt + dst->subxcnt + dst->xcnt <= GetMaxSnapshotXidCount())
	{
		Assert(dst->subxcnt == 0);
		memcpy(dst->xip + dst->xcnt, src->xip, src->xcnt*sizeof(TransactionId));
		n = dst->xcnt + src->xcnt;

		qsort(dst->xip, n, sizeof(TransactionId), xidComparator);
		xid = InvalidTransactionId;

		for (i = 0, j = 0; i < n && dst->xip[i] < dst->xmax; i++)
			if (dst->xip[i] != xid)
				dst->xip[j++] = xid = dst->xip[i];
		dst->xcnt = j;
	}
	else
	{
		Assert(src->xcnt + dst->subxcnt + dst->xcnt <= GetMaxSnapshotSubxidCount());
		memcpy(dst->subxip + dst->subxcnt, dst->xip, dst->xcnt*sizeof(TransactionId));
		memcpy(dst->subxip + dst->subxcnt + dst->xcnt, src->xip, src->xcnt*sizeof(TransactionId));
		n = dst->xcnt + dst->subxcnt + src->xcnt;

		qsort(dst->subxip, n, sizeof(TransactionId), xidComparator);
		xid = InvalidTransactionId;

		for (i = 0, j = 0; i < n && dst->subxip[i] < dst->xmax; i++)
			if (dst->subxip[i] != xid)
				dst->subxip[j++] = xid = dst->subxip[i];
		dst->subxcnt = j;
		dst->xcnt = 0;
	}
	DumpSnapshot(dst, "merged");
}

/*
 * Get oldest Xid visible by any active transaction (global or local)
 * Take in account global Xmin received from DTMD
 */
static TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum)
{
	TransactionId localXmin = PgGetOldestXmin(rel, ignoreVacuum);
	TransactionId globalXmin = dtm->minXid;
	XTM_INFO("XTM: DtmGetOldestXmin localXmin=%d, globalXmin=%d\n", localXmin, globalXmin);

	if (TransactionIdIsValid(globalXmin))
	{
		globalXmin -= vacuum_defer_cleanup_age;
		if (!TransactionIdIsNormal(globalXmin))
			globalXmin = FirstNormalTransactionId;
		if (TransactionIdPrecedes(globalXmin, localXmin))
			localXmin = globalXmin;
		XTM_INFO("XTM: DtmGetOldestXmin adjusted localXmin=%d, globalXmin=%d\n", localXmin, globalXmin);
	}
	return localXmin;
}

/*
 * Update local Recent*Xmin variables taken in account MinXmin received from DTMD
 */
static void DtmUpdateRecentXmin(Snapshot snapshot)
{
	TransactionId xmin = dtm->minXid;
	XTM_INFO("XTM: DtmUpdateRecentXmin global xmin=%d, snapshot xmin %d\n", dtm->minXid, DtmSnapshot.xmin);

	if (TransactionIdIsValid(xmin))
	{
		xmin -= vacuum_defer_cleanup_age;
		if (!TransactionIdIsNormal(xmin))
			xmin = FirstNormalTransactionId;
		if (TransactionIdFollows(RecentGlobalDataXmin, xmin))
			RecentGlobalDataXmin = xmin;
		if (TransactionIdFollows(RecentGlobalXmin, xmin))
			RecentGlobalXmin = xmin;
	}
	if (TransactionIdFollows(RecentXmin, snapshot->xmin))
	{
		ProcArrayInstallImportedXmin(snapshot->xmin, GetCurrentTransactionId());
		RecentXmin = snapshot->xmin;
	}
}

/*
 * Get new XID. For global transaction is it previsly set by dtm_begin_transaction or dtm_join_transaction.
 * Local transactions are using range of local Xids obtains from DTM.
 */
static TransactionId DtmGetNextXid()
{
	TransactionId xid;
	LWLockAcquire(dtm->xidLock, LW_EXCLUSIVE);
	if (TransactionIdIsValid(DtmNextXid))
	{
		XTM_INFO("Use global XID %d\n", DtmNextXid);
		xid = DtmNextXid;

		if (TransactionIdPrecedesOrEquals(ShmemVariableCache->nextXid, xid))
		{
			/* Advance ShmemVariableCache->nextXid formward until new Xid */
			while (TransactionIdPrecedes(ShmemVariableCache->nextXid, xid))
			{
				XTM_INFO("Extend CLOG for global transaction to %d\n", ShmemVariableCache->nextXid);
				ExtendCLOG(ShmemVariableCache->nextXid);
				ExtendCommitTs(ShmemVariableCache->nextXid);
				ExtendSUBTRANS(ShmemVariableCache->nextXid);
				TransactionIdAdvance(ShmemVariableCache->nextXid);
			}
			dtm->nReservedXids = 0;
		}
	}
	else
	{
		if (dtm->nReservedXids == 0)
		{
            XTM_INFO("%d: reserve new XID range\n", getpid());
			dtm->nReservedXids = DtmGlobalReserve(ShmemVariableCache->nextXid, DtmLocalXidReserve, &dtm->nextXid);
			Assert(dtm->nReservedXids > 0);
			Assert(TransactionIdFollowsOrEquals(dtm->nextXid, ShmemVariableCache->nextXid));

			/* Advance ShmemVariableCache->nextXid formward until new Xid */
			while (TransactionIdPrecedes(ShmemVariableCache->nextXid, dtm->nextXid))
			{
				XTM_INFO("Extend CLOG for local transaction to %d\n", ShmemVariableCache->nextXid);
				ExtendCLOG(ShmemVariableCache->nextXid);
				ExtendCommitTs(ShmemVariableCache->nextXid);
				ExtendSUBTRANS(ShmemVariableCache->nextXid);
				TransactionIdAdvance(ShmemVariableCache->nextXid);
			}
		}
		Assert(ShmemVariableCache->nextXid == dtm->nextXid);
		xid = dtm->nextXid++;
		dtm->nReservedXids -= 1;
		XTM_INFO("Obtain new local XID %d\n", xid);
	}
	LWLockRelease(dtm->xidLock);
	return xid;
}

TransactionId
DtmGetGlobalTransactionId()
{
	return DtmNextXid;
}

/*
 * We have to cut&paste copde of GetNewTransactionId from varsup because we change way of advancing ShmemVariableCache->nextXid
 */
TransactionId
DtmGetNewTransactionId(bool isSubXact)
{
	TransactionId xid;

	XTM_INFO("%d: GetNewTransactionId\n", getpid());
	/*
	 * Workers synchronize transaction state at the beginning of each parallel
	 * operation, so we can't account for new XIDs after that point.
	 */
	if (IsInParallelMode())
		elog(ERROR, "cannot assign TransactionIds during a parallel operation");

	/*
	 * During bootstrap initialization, we return the special bootstrap
	 * transaction id.
	 */
	if (IsBootstrapProcessingMode())
	{
		Assert(!isSubXact);
		MyPgXact->xid = BootstrapTransactionId;
		return BootstrapTransactionId;
	}

	/* safety check, we should never get this far in a HS slave */
	if (RecoveryInProgress())
		elog(ERROR, "cannot assign TransactionIds during recovery");

	LWLockAcquire(XidGenLock, LW_EXCLUSIVE);
	xid = DtmGetNextXid();

	/*----------
	 * Check to see if it's safe to assign another XID.  This protects against
	 * catastrophic data loss due to XID wraparound.  The basic rules are:
	 *
	 * If we're past xidVacLimit, start trying to force autovacuum cycles.
	 * If we're past xidWarnLimit, start issuing warnings.
	 * If we're past xidStopLimit, refuse to execute transactions, unless
	 * we are running in single-user mode (which gives an escape hatch
	 * to the DBA who somehow got past the earlier defenses).
	 *
	 * Note that this coding also appears in GetNewMultiXactId.
	 *----------
	 */
	if (TransactionIdFollowsOrEquals(xid, ShmemVariableCache->xidVacLimit))
	{
		/*
		 * For safety's sake, we release XidGenLock while sending signals,
		 * warnings, etc.  This is not so much because we care about
		 * preserving concurrency in this situation, as to avoid any
		 * possibility of deadlock while doing get_database_name(). First,
		 * copy all the shared values we'll need in this path.
		 */
		TransactionId xidWarnLimit = ShmemVariableCache->xidWarnLimit;
		TransactionId xidStopLimit = ShmemVariableCache->xidStopLimit;
		TransactionId xidWrapLimit = ShmemVariableCache->xidWrapLimit;
		Oid			oldest_datoid = ShmemVariableCache->oldestXidDB;

		LWLockRelease(XidGenLock);

		/*
		 * To avoid swamping the postmaster with signals, we issue the autovac
		 * request only once per 64K transaction starts.  This still gives
		 * plenty of chances before we get into real trouble.
		 */
		if (IsUnderPostmaster && (xid % 65536) == 0)
			SendPostmasterSignal(PMSIGNAL_START_AUTOVAC_LAUNCHER);

		if (IsUnderPostmaster && TransactionIdFollowsOrEquals(xid, xidStopLimit))
		{
			char *oldest_datname = get_database_name(oldest_datoid);

			/* complain even if that DB has disappeared */
			if (oldest_datname)
				ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					errmsg("database is not accepting commands to avoid wraparound data loss in database \"%s\"",
						oldest_datname),
					errhint("Stop the postmaster and vacuum that database in single-user mode.\n"
						"You might also need to commit or roll back old prepared transactions.")));
			else
				ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					errmsg("database is not accepting commands to avoid wraparound data loss in database with OID %u",
						oldest_datoid),
					errhint("Stop the postmaster and vacuum that database in single-user mode.\n"
						"You might also need to commit or roll back old prepared transactions.")));
		}
		else
		if (TransactionIdFollowsOrEquals(xid, xidWarnLimit))
		{
			char *oldest_datname = get_database_name(oldest_datoid);

			/* complain even if that DB has disappeared */
			if (oldest_datname)
				ereport(WARNING,
					(errmsg("database \"%s\" must be vacuumed within %u transactions",
						oldest_datname,
						xidWrapLimit - xid),
					errhint("To avoid a database shutdown, execute a database-wide VACUUM in that database.\n"
						"You might also need to commit or roll back old prepared transactions.")));
			else
				ereport(WARNING,
					(errmsg("database with OID %u must be vacuumed within %u transactions",
						oldest_datoid,
						xidWrapLimit - xid),
					errhint("To avoid a database shutdown, execute a database-wide VACUUM in that database.\n"
						"You might also need to commit or roll back old prepared transactions.")));
		}

		/* Re-acquire lock and start over */
		LWLockAcquire(XidGenLock, LW_EXCLUSIVE);
		xid = DtmGetNextXid();
	}

	/*
	 * If we are allocating the first XID of a new page of the commit log,
	 * zero out that commit-log page before returning. We must do this while
	 * holding XidGenLock, else another xact could acquire and commit a later
	 * XID before we zero the page.  Fortunately, a page of the commit log
	 * holds 32K or more transactions, so we don't have to do this very often.
	 *
	 * Extend pg_subtrans and pg_commit_ts too.
	 */
	if (TransactionIdFollowsOrEquals(xid, ShmemVariableCache->nextXid))
	{
		ExtendCLOG(xid);
		ExtendCommitTs(xid);
		ExtendSUBTRANS(xid);
	}
	/*
	 * Now advance the nextXid counter.  This must not happen until after we
	 * have successfully completed ExtendCLOG() --- if that routine fails, we
	 * want the next incoming transaction to try it again.  We cannot assign
	 * more XIDs until there is CLOG space for them.
	 */
	if (xid == ShmemVariableCache->nextXid)
		TransactionIdAdvance(ShmemVariableCache->nextXid);
	else
		Assert(TransactionIdPrecedes(xid, ShmemVariableCache->nextXid));

	/*
	 * We must store the new XID into the shared ProcArray before releasing
	 * XidGenLock.  This ensures that every active XID older than
	 * latestCompletedXid is present in the ProcArray, which is essential for
	 * correct OldestXmin tracking; see src/backend/access/transam/README.
	 *
	 * XXX by storing xid into MyPgXact without acquiring ProcArrayLock, we
	 * are relying on fetch/store of an xid to be atomic, else other backends
	 * might see a partially-set xid here.  But holding both locks at once
	 * would be a nasty concurrency hit.  So for now, assume atomicity.
	 *
	 * Note that readers of PGXACT xid fields should be careful to fetch the
	 * value only once, rather than assume they can read a value multiple
	 * times and get the same answer each time.
	 *
	 * The same comments apply to the subxact xid count and overflow fields.
	 *
	 * A solution to the atomic-store problem would be to give each PGXACT its
	 * own spinlock used only for fetching/storing that PGXACT's xid and
	 * related fields.
	 *
	 * If there's no room to fit a subtransaction XID into PGPROC, set the
	 * cache-overflowed flag instead.  This forces readers to look in
	 * pg_subtrans to map subtransaction XIDs up to top-level XIDs. There is a
	 * race-condition window, in that the new XID will not appear as running
	 * until its parent link has been placed into pg_subtrans. However, that
	 * will happen before anyone could possibly have a reason to inquire about
	 * the status of the XID, so it seems OK.  (Snapshots taken during this
	 * window *will* include the parent XID, so they will deliver the correct
	 * answer later on when someone does have a reason to inquire.)
	 */
	{
		/*
		 * Use volatile pointer to prevent code rearrangement; other backends
		 * could be examining my subxids info concurrently, and we don't want
		 * them to see an invalid intermediate state, such as incrementing
		 * nxids before filling the array entry.  Note we are assuming that
		 * TransactionId and int fetch/store are atomic.
		 */
		volatile PGPROC *myproc = MyProc;
		volatile PGXACT *mypgxact = MyPgXact;

		if (!isSubXact)
			mypgxact->xid = xid;
		else
		{
			int nxids = mypgxact->nxids;

			if (nxids < PGPROC_MAX_CACHED_SUBXIDS)
			{
				myproc->subxids.xids[nxids] = xid;
				mypgxact->nxids = nxids + 1;
			}
			else
				mypgxact->overflowed = true;
		}
	}

	LWLockRelease(XidGenLock);

	return xid;
}


static Snapshot DtmGetSnapshot(Snapshot snapshot)
{
	if (TransactionIdIsValid(DtmNextXid) && snapshot != &CatalogSnapshotData)
	{
		if (!DtmHasGlobalSnapshot) {
			DtmGlobalGetSnapshot(DtmNextXid, &DtmSnapshot, &dtm->minXid);
        }
		DtmLastSnapshot = snapshot;
		DtmMergeWithGlobalSnapshot(snapshot);
		if (!IsolationUsesXactSnapshot())
		{
			/* Use single global snapshot during all transaction for repeatable read isolation level,
			 * but obtain new global snapshot each time it is requested for read committed isolation level
			 */
			//DtmHasGlobalSnapshot = false;
		}
	}
	else
	{
		/* For local transactions and catalog snapshots use default GetSnapshotData implementation */
		snapshot = PgGetSnapshotData(snapshot);
	}
	DtmUpdateRecentXmin(snapshot);
	return snapshot;
}

static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn)
{
	/* Because of global snapshots we can ask for status of transaction which is not yet started locally: so we have
	 * to compare xid with ShmemVariableCache->nextXid before accessing CLOG
	 */
	XidStatus status = xid >= ShmemVariableCache->nextXid
		? TRANSACTION_STATUS_IN_PROGRESS
		: PgTransactionIdGetStatus(xid, lsn);
	XTM_TRACE("XTM: DtmGetTransactionStatus\n");
	return status;
}

static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn)
{
	XTM_INFO("%d: DtmSetTransactionStatus %u = %u\n", getpid(), xid, status);
	if (!RecoveryInProgress())
	{
		if (TransactionIdIsValid(DtmNextXid))
		{
            DtmVoted = true;
			if (status == TRANSACTION_STATUS_ABORTED || !MMIsDistributedTrans)
			{
				PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
				DtmGlobalSetTransStatus(xid, TRANSACTION_STATUS_ABORTED, false);
				XTM_INFO("Abort transaction %d\n", xid);
				return;
			}
			else
			{
                XidStatus verdict;
				XTM_INFO("Begin commit transaction %d\n", xid);
				/* Mark transaction as in-doubt in xid_in_doubt hash table */
				LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
				hash_search(xid_in_doubt, &DtmNextXid, HASH_ENTER, NULL);
				LWLockRelease(dtm->hashLock);
                verdict = DtmGlobalSetTransStatus(xid, status, true);
                if (verdict != status) { 
                    XTM_INFO("Commit of transaction %d is rejected by arbiter: staus=%d\n", xid, verdict);
                    DtmNextXid = InvalidTransactionId;
                    DtmLastSnapshot = NULL;
                    MMIsDistributedTrans = false; 
                    MarkAsAborted();
                    END_CRIT_SECTION();
                    elog(ERROR, "Commit of transaction %d is rejected by DTM", xid);                    
                } else { 
                    XTM_INFO("Commit transaction %d\n", xid);
                }
			}
		}
		else
		{
			XTM_INFO("Set transaction %u status in local CLOG\n" , xid);
		}
	}
	else if (status != TRANSACTION_STATUS_ABORTED) 
	{
		XidStatus gs;
		gs = DtmGlobalGetTransStatus(xid, false);
		if (gs != TRANSACTION_STATUS_UNKNOWN) { 
            Assert(gs != TRANSACTION_STATUS_IN_PROGRESS);
			status = gs;
        }
	}
	PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
}

static uint32 dtm_xid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int dtm_xid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 - *(TransactionId*)key2;
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
      //XTM_INFO("%d: normal=%d, initialized=%d, replication=%d, bgw=%d, vacuum=%d\n", 
      //           getpid(), IsNormalProcessingMode(), dtm->initialized, MMDoReplication, IsBackgroundWorker, IsAutoVacuumWorkerProcess());
        if (IsNormalProcessingMode() && dtm->initialized && MMDoReplication && !am_walsender && !IsBackgroundWorker && !IsAutoVacuumWorkerProcess()) { 
            MMBeginTransaction();
        }
        break;
#if 0
    case XACT_EVENT_PRE_COMMIT:
    case XACT_EVENT_PARALLEL_PRE_COMMIT:
    { 
        TransactionId xid = GetCurrentTransactionIdIfAny();
        if (!MMIsDistributedTrans && TransactionIdIsValid(xid)) {
            XTM_INFO("%d: Will ignore transaction %u\n", getpid(), xid);
            MMMarkTransAsLocal(xid);               
        }
        break;
    }
#endif
    case XACT_EVENT_COMMIT:
    case XACT_EVENT_ABORT:
		if (TransactionIdIsValid(DtmNextXid))
		{
            if (!DtmVoted) {
                DtmGlobalSetTransStatus(DtmNextXid, TRANSACTION_STATUS_ABORTED, false);
            }
			if (event == XACT_EVENT_COMMIT)
			{
				/*
				 * Now transaction status is already written in CLOG,
				 * so we can remove information about it from hash table
				 */
				LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
				hash_search(xid_in_doubt, &DtmNextXid, HASH_REMOVE, NULL);
				LWLockRelease(dtm->hashLock);
			}
#if 0 /* should be handled now using DtmVoted flag */
			else
			{
				/*
				 * Transaction at the node can be aborted because of transaction failure at some other node
				 * before it starts doing anything and assigned Xid, in this case Postgres is not calling SetTransactionStatus,
				 * so we have to send report to DTMD here
				 */
				if (!TransactionIdIsValid(GetCurrentTransactionIdIfAny())) {
                    XTM_INFO("%d: abort transation on DTMD\n", getpid());
					DtmGlobalSetTransStatus(DtmNextXid, TRANSACTION_STATUS_ABORTED, false);
                }
			}
#endif
			DtmNextXid = InvalidTransactionId;
			DtmLastSnapshot = NULL;
        }
        MMIsDistributedTrans = false;
        break;
      default:
        break;
	}
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
		"multimaster.arbiter_host",
		"The host where DTM daemon resides",
		NULL,
		&DtmHost,
		"127.0.0.1",
		PGC_BACKEND, // context
		0, // flags,
		NULL, // GucStringCheckHook check_hook,
		NULL, // GucStringAssignHook assign_hook,
		NULL // GucShowHook show_hook
	);

	DefineCustomIntVariable(
		"multimaster.arbiter_port",
		"The port DTM daemon is listening",
		NULL,
		&DtmPort,
		5431,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
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

	if (DtmBufferSize != 0)
	{
		DtmGlobalConfig(NULL, DtmPort, Unix_socket_directories);
		RegisterBackgroundWorker(&DtmWorker);
	}
	else
		DtmGlobalConfig(DtmHost, DtmPort, Unix_socket_directories);

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

void MMBeginTransaction(void)
{
	if (TransactionIdIsValid(DtmNextXid))
		elog(ERROR, "MMBeginTransaction should be called only once for global transaction");
	if (dtm == NULL)
		elog(ERROR, "DTM is not properly initialized, please check that pg_dtm plugin was added to shared_preload_libraries list in postgresql.conf");
    Assert(!RecoveryInProgress());
    XTM_INFO("%d: Try to start global transaction\n", getpid());
	DtmNextXid = DtmGlobalStartTransaction(&DtmSnapshot, &dtm->minXid, dtm->nNodes);
	if (!TransactionIdIsValid(DtmNextXid))
		elog(ERROR, "Arbiter was not able to assign XID");
	XTM_INFO("%d: Start global transaction %d, dtm->minXid=%d\n", getpid(), DtmNextXid, dtm->minXid);

    DtmVoted = false;
	DtmHasGlobalSnapshot = true;
	DtmLastSnapshot = NULL;
    MMIsDistributedTrans = false;
}

void MMJoinTransaction(TransactionId xid)
{
	if (TransactionIdIsValid(DtmNextXid))
		elog(ERROR, "dtm_begin/join_transaction should be called only once for global transaction");
	DtmNextXid = xid;
	if (!TransactionIdIsValid(DtmNextXid))
		elog(ERROR, "Arbiter was not able to assign XID");
    DtmVoted = false;

	DtmGlobalGetSnapshot(DtmNextXid, &DtmSnapshot, &dtm->minXid);
	XTM_INFO("%d: Join global transaction %d, dtm->minXid=%d\n", getpid(), DtmNextXid, dtm->minXid);

	DtmHasGlobalSnapshot = true;
	DtmLastSnapshot = NULL;
    MMIsDistributedTrans = true;

    MMMarkTransAsLocal(DtmNextXid);
}
 
void MMReceiverStarted()
{
     if (pg_atomic_fetch_add_u32(&dtm->nReceivers, 1) == dtm->nNodes-2) {
         dtm->initialized = true;
     }
}

void MMMarkTransAsLocal(TransactionId xid)
{
    LocalTransaction* lt;

    Assert(TransactionIdIsValid(xid));
    LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
    lt = hash_search(local_trans, &xid, HASH_ENTER, NULL);
    lt->count = dtm->nNodes-1;
    LWLockRelease(dtm->hashLock);
}

bool MMIsLocalTransaction(TransactionId xid)
{
    LocalTransaction* lt;
    bool result = false;
    LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
    lt = hash_search(local_trans, &xid, HASH_FIND, NULL);
    if (lt != NULL) { 
        result = true;
        Assert(lt->count > 0);
        if (--lt->count == 0) { 
            hash_search(local_trans, &xid, HASH_REMOVE, NULL);
        }
    }
    LWLockRelease(dtm->hashLock);
    return result;
}

void DtmBackgroundWorker(Datum arg)
{
	Shub shub;
	ShubParams params;
	char unix_sock_path[MAXPGPATH];

	snprintf(unix_sock_path, sizeof(unix_sock_path), "%s/p%d", Unix_socket_directories, DtmPort);

	ShubInitParams(&params);

	params.host = DtmHost;
	params.port = DtmPort;
	params.file = unix_sock_path;
	params.buffer_size = DtmBufferSize;

	ShubInitialize(&shub, &params);
	ShubLoop(&shub);
}

static void DtmSerializeLock(PROCLOCK* proclock, void* arg)
{
    ByteBuffer* buf = (ByteBuffer*)arg;
    LOCK* lock = proclock->tag.myLock;
    PGPROC* proc = proclock->tag.myProc; 

    if (lock != NULL) {
        PGXACT* srcPgXact = &ProcGlobal->allPgXact[proc->pgprocno];
        
        if (TransactionIdIsValid(srcPgXact->xid) && proc->waitLock == lock) { 
            LockMethod lockMethodTable = GetLocksMethodTable(lock);
            int numLockModes = lockMethodTable->numLockModes;
            int conflictMask = lockMethodTable->conflictTab[proc->waitLockMode];
            SHM_QUEUE *procLocks = &(lock->procLocks);
            int lm;
            
            ByteBufferAppendInt32(buf, srcPgXact->xid); /* waiting transaction */
            proclock = (PROCLOCK *) SHMQueueNext(procLocks, procLocks,
                                                 offsetof(PROCLOCK, lockLink));
            while (proclock)
            {
                if (proc != proclock->tag.myProc) { 
                    PGXACT* dstPgXact = &ProcGlobal->allPgXact[proclock->tag.myProc->pgprocno];
                    if (TransactionIdIsValid(dstPgXact->xid)) { 
                        Assert(srcPgXact->xid != dstPgXact->xid);
                        for (lm = 1; lm <= numLockModes; lm++)
                        {
                            if ((proclock->holdMask & LOCKBIT_ON(lm)) && (conflictMask & LOCKBIT_ON(lm)))
                            {
                                XTM_INFO("%d: %u(%u) waits for %u(%u)\n", getpid(), srcPgXact->xid, proc->pid, dstPgXact->xid, proclock->tag.myProc->pid);
                                ByteBufferAppendInt32(buf, dstPgXact->xid); /* transaction holding lock */
                                break;
                            }
                        }
                    }
                }
                proclock = (PROCLOCK *) SHMQueueNext(procLocks, &proclock->lockLink,
                                                     offsetof(PROCLOCK, lockLink));
            }
            ByteBufferAppendInt32(buf, 0); /* end of lock owners list */
        }
    }
}

bool DtmDetectGlobalDeadLock(PGPROC* proc)
{
    bool hasDeadlock = false;
    ByteBuffer buf;
    PGXACT* pgxact = &ProcGlobal->allPgXact[proc->pgprocno];

    if (TransactionIdIsValid(pgxact->xid)) { 
        ByteBufferAlloc(&buf);
        XTM_INFO("%d: wait graph begin\n", getpid());
        EnumerateLocks(DtmSerializeLock, &buf);
        XTM_INFO("%d: wait graph end\n", getpid());
        hasDeadlock = DtmGlobalDetectDeadLock(PostPortNumber, pgxact->xid, buf.data, buf.used);
        ByteBufferFree(&buf);
        XTM_INFO("%d: deadlock %sdetected for transaction %u\n", getpid(), hasDeadlock ? "": "not ",  pgxact->xid);
        if (hasDeadlock) {  
            elog(WARNING, "Deadlock detected for transaction %u", pgxact->xid);
        }
    }
    return hasDeadlock;
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
    MMIsDistributedTrans = false;
    PG_RETURN_VOID();
}

/*
 * Execute statement with specified parameters and check its result
 */
static bool MMRunUtilityStmt(PGconn* conn, char const* sql)
{
	PGresult *result = PQexec(conn, sql);
	int status = PQresultStatus(result);
	bool ret = status == PGRES_COMMAND_OK;
	if (!ret) { 
		elog(WARNING, "Command '%s' failed with status %d", sql, status);
	}
	PQclear(result);
	return ret;
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
			MMIsDistributedTrans = false;
		}
	} else { 		
		char* conn_str = pstrdup(MMConnStrs);
		char* conn_str_end = conn_str + strlen(conn_str);
		int i = 0;
		int failedNode = -1;
		char const* errorMsg = NULL;
		PGconn **conns;
		conns = palloc(sizeof(PGconn*)*MMNodes);
        
		while (conn_str < conn_str_end) { 
			char* p = strchr(conn_str, ',');
			if (p == NULL) { 
				p = conn_str_end;
			}
			*p = '\0';        
			conns[i] = PQconnectdb(conn_str);
			if (PQstatus(conns[i]) != CONNECTION_OK)
			{
				failedNode = i;
				do { 
					PQfinish(conns[i]);
				} while (--i >= 0);				
				elog(ERROR, "Failed to establish connection '%s' to node %d", conn_str, failedNode);
			}
			conn_str = p + 1;
			i += 1;
		}
		Assert(i == MMNodes);
		
		for (i = 0; i < MMNodes; i++) { 
			if (!MMRunUtilityStmt(conns[i], "BEGIN TRANSACTION"))
			{
				errorMsg = "Failed to start transaction at node %d";
				failedNode = i;
				break;
			}
			if (!MMRunUtilityStmt(conns[i], queryString))
			{
				errorMsg = "Failed to run command at node %d";
				failedNode = i;
				break;
			}
		}
		if (failedNode >= 0)  
		{
			for (i = 0; i < MMNodes; i++) { 
				MMRunUtilityStmt(conns[i], "ROLLBACK TRANSACTION");
			}
		} else { 
			for (i = 0; i < MMNodes; i++) { 
				if (!MMRunUtilityStmt(conns[i], "COMMIT TRANSACTION")) { 
					errorMsg = "Commit failed at node %d";
					failedNode = i;
				}
			}
		}			
		for (i = 0; i < MMNodes; i++) { 
			PQfinish(conns[i]);
		}
		if (failedNode >= 0) { 
			elog(ERROR, errorMsg, failedNode+1);
		}
	}
}
static void
MMExecutorFinish(QueryDesc *queryDesc)
{
    if (MMDoReplication) { 
        CmdType operation = queryDesc->operation;
        EState *estate = queryDesc->estate;
        if (estate->es_processed != 0) { 
            MMIsDistributedTrans |= operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE;
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
