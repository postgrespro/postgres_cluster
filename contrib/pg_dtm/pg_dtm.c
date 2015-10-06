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
#include "access/commit_ts.h"
#include "access/xlog.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "access/twophase.h"
#include <utils/guc.h>
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

#include "libdtm.h"

typedef struct
{
	LWLockId hashLock;
	LWLockId xidLock;
    TransactionId minXid;  /* XID of oldest transaction visible by any active transaction (local or global) */
	TransactionId nextXid; /* next XID for local transaction */
    size_t nReservedXids;  /* number of XIDs reserved for local transactions */   
} DtmState;


#define DTM_SHMEM_SIZE (1024*1024)
#define DTM_HASH_SIZE  1003

void _PG_init(void);
void _PG_fini(void);

static Snapshot DtmGetSnapshot(Snapshot snapshot);
static void DtmMergeWithGlobalSnapshot(Snapshot snapshot);
static XidStatus DtmGetTransactionStatus(TransactionId xid, XLogRecPtr *lsn);
static void DtmSetTransactionStatus(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
static void DtmUpdateRecentXmin(Snapshot snapshot);
static void DtmInitialize(void);
static void DtmXactCallback(XactEvent event, void *arg);
static TransactionId DtmGetNextXid(void);
static TransactionId DtmGetNewTransactionId(bool isSubXact);
static TransactionId DtmGetOldestXmin(Relation rel, bool ignoreVacuum);
static TransactionId DtmGetGlobalTransactionId(void);

static bool TransactionIdIsInSnapshot(TransactionId xid, Snapshot snapshot);
static bool TransactionIdIsInDoubt(TransactionId xid);

static void DtmShmemStartup(void);

static shmem_startup_hook_type prev_shmem_startup_hook;
static HTAB* xid_in_doubt;
static DtmState* dtm;
static Snapshot CurrentTransactionSnapshot;

static TransactionId DtmNextXid;
static SnapshotData DtmSnapshot = { HeapTupleSatisfiesMVCC };
static bool DtmHasGlobalSnapshot;
static bool DtmGlobalXidAssigned;
static int DtmLocalXidReserve;
static int DtmCurcid;
static Snapshot DtmLastSnapshot;
static TransactionManager DtmTM = { DtmGetTransactionStatus, DtmSetTransactionStatus, DtmGetSnapshot, DtmGetNewTransactionId, DtmGetOldestXmin, PgTransactionIdIsInProgress, DtmGetGlobalTransactionId, PgXidInMVCCSnapshot };

static char *DtmHost;
static int DtmPort;


#define XTM_TRACE(fmt, ...)
//#define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#define XTM_INFO(fmt, ...)

static void DumpSnapshot(Snapshot s, char *name)
{
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

	if (!TransactionIdIsInSnapshot(xid, &DtmSnapshot)) { /* transaction is completed according to the snaphot */
		LWLockAcquire(dtm->hashLock, LW_SHARED);
		inDoubt = hash_search(xid_in_doubt, &xid, HASH_FIND, NULL) != NULL;
		LWLockRelease(dtm->hashLock);
		if (!inDoubt) {
			XLogRecPtr lsn;
			inDoubt = DtmGetTransactionStatus(xid, &lsn) != TRANSACTION_STATUS_IN_PROGRESS;
		}
		if (inDoubt) {
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

	Assert(TransactionIdIsValid(src->xmin) && TransactionIdIsValid(src->xmax));

  GetLocalSnapshot:
    /*
     * Check that global and local snapshots are consistent: transactions marked as completed in global snapohsot 
     * should be completed locally 
     */
	dst = PgGetSnapshotData(dst);
	for (i = 0; i < dst->xcnt; i++) {
		if (TransactionIdIsInDoubt(dst->xip[i])) {
			goto GetLocalSnapshot;
		}
	}
	for (xid = dst->xmax; xid < src->xmax; xid++) {
		if (TransactionIdIsInDoubt(xid)) {
			goto GetLocalSnapshot;
		}
	}
	DumpSnapshot(dst, "local");
	DumpSnapshot(src, "DTM");

	if (src->xmax < dst->xmax) dst->xmax = src->xmax;

	if (src->xmin < dst->xmin) {
		dst->xmin = src->xmin;
	}

	n = dst->xcnt;
	Assert(src->xcnt + n <= GetMaxSnapshotXidCount());
	memcpy(dst->xip + n, src->xip, src->xcnt*sizeof(TransactionId));
	n += src->xcnt;

	qsort(dst->xip, n, sizeof(TransactionId), xidComparator);
	xid = InvalidTransactionId;

	for (i = 0, j = 0; i < n && dst->xip[i] < dst->xmax; i++) {
		if (dst->xip[i] != xid) {
			dst->xip[j++] = xid = dst->xip[i];
		}
	}
	dst->xcnt = j;

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

	if (TransactionIdIsValid(globalXmin)) {
		globalXmin -= vacuum_defer_cleanup_age;
		if (!TransactionIdIsNormal(globalXmin)) {
			globalXmin = FirstNormalTransactionId;
		}
		if (TransactionIdPrecedes(globalXmin, localXmin)) {
			localXmin = globalXmin;
		}
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

	if (TransactionIdIsValid(xmin)) {
		xmin -= vacuum_defer_cleanup_age;
		if (!TransactionIdIsNormal(xmin)) {
			xmin = FirstNormalTransactionId;
		}
		if (TransactionIdFollows(RecentGlobalDataXmin, xmin)) {
			RecentGlobalDataXmin = xmin;
		}
		if (TransactionIdFollows(RecentGlobalXmin, xmin)) {
			RecentGlobalXmin = xmin;
		}
	}
	if (TransactionIdFollows(RecentXmin, snapshot->xmin)) {
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
	if (TransactionIdIsValid(DtmNextXid)) {
		XTM_INFO("Use global XID %d\n", DtmNextXid);
		xid = DtmNextXid;

		if (TransactionIdPrecedesOrEquals(ShmemVariableCache->nextXid, xid)) {
            /* Advance ShmemVariableCache->nextXid formward until new Xid */               
			while (TransactionIdPrecedes(ShmemVariableCache->nextXid, xid)) {
				XTM_INFO("Extend CLOG for global transaction to %d\n", ShmemVariableCache->nextXid);
				ExtendCLOG(ShmemVariableCache->nextXid);
				ExtendCommitTs(ShmemVariableCache->nextXid);
				ExtendSUBTRANS(ShmemVariableCache->nextXid);
				TransactionIdAdvance(ShmemVariableCache->nextXid);
			}
			dtm->nReservedXids = 0;
		}
	} else {
		if (dtm->nReservedXids == 0) {
			dtm->nReservedXids = DtmGlobalReserve(ShmemVariableCache->nextXid, DtmLocalXidReserve, &dtm->nextXid);
			Assert(dtm->nReservedXids > 0);
			Assert(TransactionIdFollowsOrEquals(dtm->nextXid, ShmemVariableCache->nextXid));

            /* Advance ShmemVariableCache->nextXid formward until new Xid */               
			while (TransactionIdPrecedes(ShmemVariableCache->nextXid, dtm->nextXid)) {
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
    Assert(!DtmGlobalXidAssigned); /* We should not assign new Xid if we do not use previous one */

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

		if (IsUnderPostmaster &&
			TransactionIdFollowsOrEquals(xid, xidStopLimit))
		{
			char	   *oldest_datname = get_database_name(oldest_datoid);

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
		else if (TransactionIdFollowsOrEquals(xid, xidWarnLimit))
		{
			char	   *oldest_datname = get_database_name(oldest_datoid);

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
	if (TransactionIdFollowsOrEquals(xid, ShmemVariableCache->nextXid)) {
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
	if (xid == ShmemVariableCache->nextXid) {
		TransactionIdAdvance(ShmemVariableCache->nextXid);
	} else {
		Assert(TransactionIdPrecedes(xid, ShmemVariableCache->nextXid));
	}

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
			int			nxids = mypgxact->nxids;

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
    if (DtmGlobalXidAssigned) { 
        /* If DtmGlobalXidAssigned is set, we are in transaction performing dtm_begin_transaction or dtm_join_transaction
         * which PRECEDS actual transaction for which Xid is received.
         * This transaction doesn't need to take in accountn global snapshot
         */
        return PgGetSnapshotData(snapshot);
	}
	if (TransactionIdIsValid(DtmNextXid) && snapshot != &CatalogSnapshotData) {
		if (!DtmHasGlobalSnapshot && (snapshot != DtmLastSnapshot || DtmCurcid != snapshot->curcid)) {
			DtmGlobalGetSnapshot(DtmNextXid, &DtmSnapshot, &dtm->minXid);
		}
		DtmCurcid = snapshot->curcid;
		DtmLastSnapshot = snapshot;
		DtmMergeWithGlobalSnapshot(snapshot);
		if (!IsolationUsesXactSnapshot()) {
            /* Use single global snapshot during all transaction for repeatable read isolation level, 
             * but obtain new global snapshot each time it is requested for read committed isolation level
             */
			DtmHasGlobalSnapshot = false;
		}
	} else {
        /* For local transactions and catalog snapshots use default GetSnapshotData implementation */
		snapshot = PgGetSnapshotData(snapshot);
	}
	DtmUpdateRecentXmin(snapshot);
	CurrentTransactionSnapshot = snapshot;
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
	if (!RecoveryInProgress()) {
		if (!DtmGlobalXidAssigned && TransactionIdIsValid(DtmNextXid)) {
			CurrentTransactionSnapshot = NULL;
			if (status == TRANSACTION_STATUS_ABORTED) {
				PgTransactionIdSetTreeStatus(xid, nsubxids, subxids, status, lsn);
				DtmGlobalSetTransStatus(xid, status, false);
				XTM_INFO("Abort transaction %d\n", xid);
				return;
			} else {
				XTM_INFO("Begin commit transaction %d\n", xid);
                /* Mark transaction as on-doubt in xid_in_doubt hash table */
				LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
				hash_search(xid_in_doubt, &DtmNextXid, HASH_ENTER, NULL);
				LWLockRelease(dtm->hashLock);
				DtmGlobalSetTransStatus(xid, status, true);
				XTM_INFO("Commit transaction %d\n", xid);
			}
		} else {
			XTM_INFO("Set transaction %u status in local CLOG" , xid);
		}
	} else {
		XidStatus gs;
		gs = DtmGlobalGetTransStatus(xid, false);
		if (gs != TRANSACTION_STATUS_UNKNOWN) {
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
        RegisterXactCallback(DtmXactCallback, NULL);
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


	TM = &DtmTM;
}

static void
DtmXactCallback(XactEvent event, void *arg)
{
    XTM_INFO("%d: DtmXactCallbackevent=%d isGlobal=%d, nextxid=%d\n", getpid(), event, DtmGlobalXidAssigned, DtmNextXid);
	if (event == XACT_EVENT_COMMIT || event == XACT_EVENT_ABORT) {
		if (DtmGlobalXidAssigned) {
            /* DtmGlobalXidAssigned is set when Xid for global transaction is recieved.
             * But it happens in separate local transaction preceding this global transaction at this backend.
             * So this variable is used as indicator that we are still in local transaction preceeding global transaction.
             * When this local transaction is completed we are ready to assign Xid to global transaction.
             */
			DtmGlobalXidAssigned = false;
		} else if (TransactionIdIsValid(DtmNextXid)) {
			if (event == XACT_EVENT_COMMIT) {
                /* Now transaction status is already written in CLOG, so we can remove information about it from hash table */
				LWLockAcquire(dtm->hashLock, LW_EXCLUSIVE);
				hash_search(xid_in_doubt, &DtmNextXid, HASH_REMOVE, NULL);
				LWLockRelease(dtm->hashLock);
			} else { 
                /* Transaction at the node can be aborted because of transaction failure at some other node
                 * before it starts doing anything and assigned Xid, in this case Postgres is not calling SetTransactionStatus,
                 * so we have to send report to DTMD here 
                 */
                if (!TransactionIdIsValid(GetCurrentTransactionIdIfAny())) {
                    DtmGlobalSetTransStatus(DtmNextXid, TRANSACTION_STATUS_ABORTED, false);
                }
            }
			DtmNextXid = InvalidTransactionId;
			DtmLastSnapshot = NULL;
		}
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

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in imcs_shmem_startup().
	 */
	RequestAddinShmemSpace(DTM_SHMEM_SIZE);
	RequestAddinLWLocks(2);

	DefineCustomIntVariable(
		"dtm.local_xid_reserve",
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

	DefineCustomStringVariable(
		"dtm.host",
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
		"dtm.port",
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

	TuneToDtm(DtmHost, DtmPort);

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = DtmShmemStartup;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	shmem_startup_hook = prev_shmem_startup_hook;
}


static void DtmShmemStartup(void)
{
	if (prev_shmem_startup_hook) {
		prev_shmem_startup_hook();
	}
	DtmInitialize();
}

/*
 *  ***************************************************************************
 */

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dtm_begin_transaction);
PG_FUNCTION_INFO_V1(dtm_join_transaction);
PG_FUNCTION_INFO_V1(dtm_get_current_snapshot_xmax);
PG_FUNCTION_INFO_V1(dtm_get_current_snapshot_xmin);
PG_FUNCTION_INFO_V1(dtm_get_current_snapshot_xcnt);

Datum
dtm_get_current_snapshot_xmin(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32(CurrentTransactionSnapshot->xmin);
}

Datum
dtm_get_current_snapshot_xmax(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32(CurrentTransactionSnapshot->xmax);
}

Datum
dtm_get_current_snapshot_xcnt(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32(CurrentTransactionSnapshot->xcnt);
}

Datum
dtm_begin_transaction(PG_FUNCTION_ARGS)
{
	Assert(!TransactionIdIsValid(DtmNextXid));
    if (dtm == NULL) { 
        elog(ERROR, "DTM is not properly initialized, please check that pg_dtm plugin was added to shared_preload_libraries list in postgresql.conf");
    }
	DtmNextXid = DtmGlobalStartTransaction(&DtmSnapshot, &dtm->minXid);
	Assert(TransactionIdIsValid(DtmNextXid));
	XTM_INFO("%d: Start global transaction %d, dtm->minXid=%d\n", getpid(), DtmNextXid, dtm->minXid);

	DtmHasGlobalSnapshot = true;
	DtmGlobalXidAssigned = true;
	DtmLastSnapshot = NULL;

	PG_RETURN_INT32(DtmNextXid);
}

Datum dtm_join_transaction(PG_FUNCTION_ARGS)
{
	Assert(!TransactionIdIsValid(DtmNextXid));
	DtmNextXid = PG_GETARG_INT32(0);
	Assert(TransactionIdIsValid(DtmNextXid));

	DtmGlobalGetSnapshot(DtmNextXid, &DtmSnapshot, &dtm->minXid);
	XTM_INFO("%d: Join global transaction %d, dtm->minXid=%d\n", getpid(), DtmNextXid, dtm->minXid);

	DtmHasGlobalSnapshot = true;
	DtmGlobalXidAssigned = true;
	DtmLastSnapshot = NULL;

	PG_RETURN_VOID();
}

