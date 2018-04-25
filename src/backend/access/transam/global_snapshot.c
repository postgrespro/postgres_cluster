/*-------------------------------------------------------------------------
 *
 * global_snapshot.c
 *		Support for cross-node snapshot isolation.
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/access/transam/global_snapshot.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/global_csn_log.h"
#include "access/global_snapshot.h"
#include "access/transam.h"
#include "access/twophase.h"
#include "access/xact.h"
#include "portability/instr_time.h"
#include "storage/lmgr.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "storage/shmem.h"
#include "storage/spin.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/snapmgr.h"
#include "miscadmin.h"

/* Raise a warning if imported global_csn exceeds ours by this value. */
#define SNAP_DESYNC_COMPLAIN (1*NSECS_PER_SEC) /* 1 second */

/*
 * GlobalSnapshotState
 *
 * Do not trust local clocks to be strictly monotonical and save last acquired
 * value so later we can compare next timestamp with it. Accessed through
 * GlobalSnapshotGenerate() and GlobalSnapshotSync().
 */
typedef struct
{
	GlobalCSN		 last_global_csn;
	volatile slock_t lock;
} GlobalSnapshotState;

static GlobalSnapshotState *gsState;


/*
 * GUC to delay advance of oldestXid for this amount of time. Also determines
 * the size GlobalSnapshotXidMap circular buffer.
 */
int global_snapshot_defer_time;

/*
 * Enables this module.
 */
extern bool track_global_snapshots;

/*
 * GlobalSnapshotXidMap
 *
 * To be able to install global snapshot that points to past we need to keep
 * old versions of tuples and therefore delay advance of oldestXid.  Here we
 * keep track of correspondence between snapshot's global_csn and oldestXid
 * that was set at the time when the snapshot was taken.  Much like the
 * snapshot too old's OldSnapshotControlData does, but with finer granularity
 * to seconds.
 *
 * Different strategies can be employed to hold oldestXid (e.g. we can track
 * oldest global_csn-based snapshot among cluster nodes and map it oldestXid
 * on each node) but here implemented one that tries to avoid cross-node
 * communications which are tricky in case of postgres_fdw.
 *
 * On each snapshot acquisition GlobalSnapshotMapXmin() is called and stores
 * correspondence between current global_csn and oldestXmin in a sparse way:
 * global_csn is rounded to seconds (and here we use the fact that global_csn
 * is just a timestamp) and oldestXmin is stored in the circular buffer where
 * rounded global_csn acts as an offset from current circular buffer head.
 * Size of the circular buffer is controlled by global_snapshot_defer_time GUC.
 *
 * When global snapshot arrives from different node we check that its
 * global_csn is still in our map, otherwise we'll error out with "snapshot too
 * old" message.  If global_csn is successfully mapped to oldestXid we move
 * backend's pgxact->xmin to proc->originalXmin and fill pgxact->xmin to
 * mapped oldestXid.  That way GetOldestXmin() can take into account backends
 * with imported global snapshot and old tuple versions will be preserved.
 *
 * Also while calculating oldestXmin for our map in presence of imported
 * global snapshots we should use proc->originalXmin instead of pgxact->xmin
 * that was set during import.  Otherwise, we can create a feedback loop:
 * xmin's of imported global snapshots were calculated using our map and new
 * entries in map going to be calculated based on that xmin's, and there is
 * a risk to stuck forever with one non-increasing oldestXmin.  All other
 * callers of GetOldestXmin() are using pgxact->xmin so the old tuple versions
 * are preserved.
 */
typedef struct GlobalSnapshotXidMap
{
	int				 head;				/* offset of current freshest value */
	int				 size;				/* total size of circular buffer */
	GlobalCSN_atomic last_csn_seconds;	/* last rounded global_csn that changed
										 * xmin_by_second[] */
	TransactionId   *xmin_by_second;	/* circular buffer of oldestXmin's */
}
GlobalSnapshotXidMap;

static GlobalSnapshotXidMap *gsXidMap;


/* Estimate shared memory space needed */
Size
GlobalSnapshotShmemSize(void)
{
	Size	size = 0;

	if (track_global_snapshots || global_snapshot_defer_time > 0)
	{
		size += MAXALIGN(sizeof(GlobalSnapshotState));
	}

	if (global_snapshot_defer_time > 0)
	{
		size += sizeof(GlobalSnapshotXidMap);
		size += global_snapshot_defer_time*sizeof(TransactionId);
		size = MAXALIGN(size);
	}

	return size;
}

/* Init shared memory structures */
void
GlobalSnapshotShmemInit()
{
	bool found;

	if (track_global_snapshots || global_snapshot_defer_time > 0)
	{
		gsState = ShmemInitStruct("gsState",
								sizeof(GlobalSnapshotState),
								&found);
		if (!found)
		{
			gsState->last_global_csn = 0;
			SpinLockInit(&gsState->lock);
		}
	}

	if (global_snapshot_defer_time > 0)
	{
		gsXidMap = ShmemInitStruct("gsXidMap",
								   sizeof(GlobalSnapshotXidMap),
								   &found);
		if (!found)
		{
			int i;

			pg_atomic_init_u64(&gsXidMap->last_csn_seconds, 0);
			gsXidMap->head = 0;
			gsXidMap->size = global_snapshot_defer_time;
			gsXidMap->xmin_by_second =
							ShmemAlloc(sizeof(TransactionId)*gsXidMap->size);

			for (i = 0; i < gsXidMap->size; i++)
				gsXidMap->xmin_by_second[i] = InvalidTransactionId;
		}
	}
}

/*
 * GlobalSnapshotStartup
 *
 * Set gsXidMap entries to oldestActiveXID during startup.
 */
void
GlobalSnapshotStartup(TransactionId oldestActiveXID)
{
	/*
	 * Run only if we have initialized shared memory and gsXidMap
	 * is enabled.
	 */
	if (IsNormalProcessingMode() && global_snapshot_defer_time > 0)
	{
		int i;

		Assert(TransactionIdIsValid(oldestActiveXID));
		for (i = 0; i < gsXidMap->size; i++)
			gsXidMap->xmin_by_second[i] = oldestActiveXID;
		ProcArraySetGlobalSnapshotXmin(oldestActiveXID);
	}
}

/*
 * GlobalSnapshotMapXmin
 *
 * Maintain circular buffer of oldestXmins for several seconds in past. This
 * buffer allows to shift oldestXmin in the past when backend is importing
 * global transaction. Otherwise old versions of tuples that were needed for
 * this transaction can be recycled by other processes (vacuum, HOT, etc).
 *
 * Locking here is not trivial. Called upon each snapshot creation after
 * ProcArrayLock is released. Such usage creates several race conditions. It
 * is possible that backend who got global_csn called GlobalSnapshotMapXmin()
 * only after other backends managed to get snapshot and complete
 * GlobalSnapshotMapXmin() call, or even committed. This is safe because
 *
 *      * We already hold our xmin in MyPgXact, so our snapshot will not be
 * 	      harmed even though ProcArrayLock is released.
 *
 *		* snapshot_global_csn is always pessmistically rounded up to the next
 *		  second.
 *
 *      * For performance reasons, xmin value for particular second is filled
 *        only once. Because of that instead of writing to buffer just our
 *        xmin (which is enough for our snapshot), we bump oldestXmin there --
 *        it mitigates the possibility of damaging someone else's snapshot by
 *        writing to the buffer too advanced value in case of slowness of
 *        another backend who generated csn earlier, but didn't manage to
 *        insert it before us.
 *
 *		* if GlobalSnapshotMapXmin() founds a gap in several seconds between
 *		  current call and latest completed call then it should fill that gap
 *		  with latest known values instead of new one. Otherwise it is
 *		  possible (however highly unlikely) that this gap also happend
 *		  between taking snapshot and call to GlobalSnapshotMapXmin() for some
 *		  backend. And we are at risk to fill circullar buffer with
 *		  oldestXmin's that are bigger then they actually were.
 */
void
GlobalSnapshotMapXmin(GlobalCSN snapshot_global_csn)
{
	int offset, gap, i;
	GlobalCSN csn_seconds;
	GlobalCSN last_csn_seconds;
	volatile TransactionId oldest_deferred_xmin;
	TransactionId current_oldest_xmin, previous_oldest_xmin;

	/* Callers should check config values */
	Assert(global_snapshot_defer_time > 0);
	Assert(gsXidMap != NULL);

	/*
	 * Round up global_csn to the next second -- pessimistically and safely.
	 */
	csn_seconds = (snapshot_global_csn / NSECS_PER_SEC + 1);

	/*
	 * Fast-path check. Avoid taking exclusive GlobalSnapshotXidMapLock lock
	 * if oldestXid was already written to xmin_by_second[] for this rounded
	 * global_csn.
	 */
	if (pg_atomic_read_u64(&gsXidMap->last_csn_seconds) >= csn_seconds)
		return;

	/* Ok, we have new entry (or entries) */
	LWLockAcquire(GlobalSnapshotXidMapLock, LW_EXCLUSIVE);

	/* Re-check last_csn_seconds under lock */
	last_csn_seconds = pg_atomic_read_u64(&gsXidMap->last_csn_seconds);
	if (last_csn_seconds >= csn_seconds)
	{
		LWLockRelease(GlobalSnapshotXidMapLock);
		return;
	}
	pg_atomic_write_u64(&gsXidMap->last_csn_seconds, csn_seconds);

	/*
	 * Count oldest_xmin.
	 *
	 * It was possible to calculate oldest_xmin during corresponding snapshot
	 * creation, but GetSnapshotData() intentionally reads only PgXact, but not
	 * PgProc. And we need info about originalXmin (see comment to gsXidMap)
	 * which is stored in PgProc because of threats in comments around PgXact
	 * about extending it with new fields. So just calculate oldest_xmin again,
	 * that anyway happens quite rarely.
	 */
	current_oldest_xmin = GetOldestXmin(NULL, PROCARRAY_NON_IMPORTED_XMIN);

	previous_oldest_xmin = gsXidMap->xmin_by_second[gsXidMap->head];

	Assert(TransactionIdIsNormal(current_oldest_xmin));
	Assert(TransactionIdIsNormal(previous_oldest_xmin));

	gap = csn_seconds - last_csn_seconds;
	offset = csn_seconds % gsXidMap->size;

	/* Sanity check before we update head and gap */
	Assert( gap >= 1 );
	Assert( (gsXidMap->head + gap) % gsXidMap->size == offset );

	gap = gap > gsXidMap->size ? gsXidMap->size : gap;
	gsXidMap->head = offset;

	/* Fill new entry with current_oldest_xmin */
	gsXidMap->xmin_by_second[offset] = current_oldest_xmin;

	/*
	 * If we have gap then fill it with previous_oldest_xmin for reasons
	 * outlined in comment above this function.
	 */
	for (i = 1; i < gap; i++)
	{
		offset = (offset + gsXidMap->size - 1) % gsXidMap->size;
		gsXidMap->xmin_by_second[offset] = previous_oldest_xmin;
	}

	oldest_deferred_xmin =
		gsXidMap->xmin_by_second[ (gsXidMap->head + 1) % gsXidMap->size ];

	LWLockRelease(GlobalSnapshotXidMapLock);

	/*
	 * Advance procArray->global_snapshot_xmin after we released
	 * GlobalSnapshotXidMapLock. Since we gather not xmin but oldestXmin, it
	 * never goes backwards regardless of how slow we can do that.
	 */
	Assert(TransactionIdFollowsOrEquals(oldest_deferred_xmin,
										ProcArrayGetGlobalSnapshotXmin()));
	ProcArraySetGlobalSnapshotXmin(oldest_deferred_xmin);
}


/*
 * GlobalSnapshotToXmin
 *
 * Get oldestXmin that took place when snapshot_global_csn was taken.
 */
TransactionId
GlobalSnapshotToXmin(GlobalCSN snapshot_global_csn)
{
	TransactionId xmin;
	GlobalCSN csn_seconds;
	volatile GlobalCSN last_csn_seconds;

	/* Callers should check config values */
	Assert(global_snapshot_defer_time > 0);
	Assert(gsXidMap != NULL);

	/* Round down to get conservative estimates */
	csn_seconds = (snapshot_global_csn / NSECS_PER_SEC);

	LWLockAcquire(GlobalSnapshotXidMapLock, LW_SHARED);
	last_csn_seconds = pg_atomic_read_u64(&gsXidMap->last_csn_seconds);
	if (csn_seconds > last_csn_seconds)
	{
		/* we don't have entry for this global_csn yet, return latest known */
		xmin = gsXidMap->xmin_by_second[gsXidMap->head];
	}
	else if (last_csn_seconds - csn_seconds < gsXidMap->size)
	{
		/* we are good, retrieve value from our map */
		Assert(last_csn_seconds % gsXidMap->size == gsXidMap->head);
		xmin = gsXidMap->xmin_by_second[csn_seconds % gsXidMap->size];
	}
	else
	{
		/* requested global_csn is too old, let caller know */
		xmin = InvalidTransactionId;
	}
	LWLockRelease(GlobalSnapshotXidMapLock);

	return xmin;
}

/*
 * GlobalSnapshotGenerate
 *
 * Generate GlobalCSN which is actually a local time. Also we are forcing
 * this time to be always increasing. Since now it is not uncommon to have
 * millions of read transactions per second we are trying to use nanoseconds
 * if such time resolution is available.
 */
GlobalCSN
GlobalSnapshotGenerate(bool locked)
{
	instr_time	current_time;
	GlobalCSN	global_csn;

	Assert(track_global_snapshots || global_snapshot_defer_time > 0);

	/*
	 * TODO: create some macro that add small random shift to current time.
	 */
	INSTR_TIME_SET_CURRENT(current_time);
	global_csn = (GlobalCSN) INSTR_TIME_GET_NANOSEC(current_time);

	/* TODO: change to atomics? */
	if (!locked)
		SpinLockAcquire(&gsState->lock);

	if (global_csn <= gsState->last_global_csn)
		global_csn = ++gsState->last_global_csn;
	else
		gsState->last_global_csn = global_csn;

	if (!locked)
		SpinLockRelease(&gsState->lock);

	return global_csn;
}

/*
 * GlobalSnapshotSync
 *
 * Due to time desynchronization on different nodes we can receive global_csn
 * which is greater than global_csn on this node. To preserve proper isolation
 * this node needs to wait when such global_csn comes on local clock.
 *
 * This should happend relatively rare if nodes have running NTP/PTP/etc.
 * Complain if wait time is more than SNAP_SYNC_COMPLAIN.
 */
void
GlobalSnapshotSync(GlobalCSN remote_gcsn)
{
	GlobalCSN	local_gcsn;
	GlobalCSN	delta;

	Assert(track_global_snapshots);

	for(;;)
	{
		SpinLockAcquire(&gsState->lock);
		if (gsState->last_global_csn > remote_gcsn)
		{
			/* Everything is fine */
			SpinLockRelease(&gsState->lock);
			return;
		}
		else if ((local_gcsn = GlobalSnapshotGenerate(true)) >= remote_gcsn)
		{
			/*
			 * Everything is fine too, but last_global_csn wasn't updated for
			 * some time.
			 */
			SpinLockRelease(&gsState->lock);
			return;
		}
		SpinLockRelease(&gsState->lock);

		/* Okay we need to sleep now */
		delta = remote_gcsn - local_gcsn;
		if (delta > SNAP_DESYNC_COMPLAIN)
			ereport(WARNING,
				(errmsg("remote global snapshot exceeds ours by more than a second"),
				 errhint("Consider running NTPd on servers participating in global transaction")));

		/* TODO: report this sleeptime somewhere? */
		pg_usleep((long) (delta/NSECS_PER_USEC));

		/*
		 * Loop that checks to ensure that we actually slept for specified
		 * amount of time.
		 */
	}

	Assert(false); /* Should not happend */
	return;
}

/*
 * TransactionIdGetGlobalCSN
 *
 * Get GlobalCSN for specified TransactionId taking care about special xids,
 * xids beyond TransactionXmin and InDoubt states.
 */
GlobalCSN
TransactionIdGetGlobalCSN(TransactionId xid)
{
	GlobalCSN global_csn;

	Assert(track_global_snapshots);

	/* Handle permanent TransactionId's for which we don't have mapping */
	if (!TransactionIdIsNormal(xid))
	{
		if (xid == InvalidTransactionId)
			return AbortedGlobalCSN;
		if (xid == FrozenTransactionId || xid == BootstrapTransactionId)
			return FrozenGlobalCSN;
		Assert(false); /* Should not happend */
	}

	/*
	 * For xids which less then TransactionXmin GlobalCSNLog can be already
	 * trimmed but we know that such transaction is definetly not concurrently
	 * running according to any snapshot including timetravel ones. Callers
	 * should check TransactionDidCommit after.
	 */
	if (TransactionIdPrecedes(xid, TransactionXmin))
		return FrozenGlobalCSN;

	/* Read GlobalCSN from SLRU */
	global_csn = GlobalCSNLogGetCSN(xid);

	/*
	 * If we faced InDoubt state then transaction is beeing committed and we
	 * should wait until GlobalCSN will be assigned so that visibility check
	 * could decide whether tuple is in snapshot. See also comments in
	 * GlobalSnapshotPrecommit().
	 */
	if (GlobalCSNIsInDoubt(global_csn))
	{
		XactLockTableWait(xid, NULL, NULL, XLTW_None);
		global_csn = GlobalCSNLogGetCSN(xid);
		Assert(GlobalCSNIsNormal(global_csn) ||
				GlobalCSNIsAborted(global_csn));
	}

	Assert(GlobalCSNIsNormal(global_csn) ||
			GlobalCSNIsInProgress(global_csn) ||
			GlobalCSNIsAborted(global_csn));

	return global_csn;
}

/*
 * XidInvisibleInGlobalSnapshot
 *
 * Version of XidInMVCCSnapshot for global transactions. For non-imported
 * global snapshots this should give same results as XidInLocalMVCCSnapshot
 * (except that aborts will be shown as invisible without going to clog) and to
 * ensure such behaviour XidInMVCCSnapshot is coated with asserts that checks
 * identicalness of XidInvisibleInGlobalSnapshot/XidInLocalMVCCSnapshot in
 * case of ordinary snapshot.
 */
bool
XidInvisibleInGlobalSnapshot(TransactionId xid, Snapshot snapshot)
{
	GlobalCSN csn;

	Assert(track_global_snapshots);

	csn = TransactionIdGetGlobalCSN(xid);

	if (GlobalCSNIsNormal(csn))
	{
		if (csn < snapshot->global_csn)
			return false;
		else
			return true;
	}
	else if (GlobalCSNIsFrozen(csn))
	{
		/* It is bootstrap or frozen transaction */
		return false;
	}
	else
	{
		/* It is aborted or in-progress */
		Assert(GlobalCSNIsAborted(csn) || GlobalCSNIsInProgress(csn));
		if (GlobalCSNIsAborted(csn))
			Assert(TransactionIdDidAbort(xid));
		return true;
	}
}


/*****************************************************************************
 * Functions to handle distributed commit on transaction coordinator:
 * GlobalSnapshotPrepareCurrent() / GlobalSnapshotAssignCsnCurrent().
 * Correspoding functions for remote nodes are defined in twophase.c:
 * pg_global_snapshot_prepare/pg_global_snapshot_assign.
 *****************************************************************************/


/*
 * GlobalSnapshotPrepareCurrent
 *
 * Set InDoubt state for currently active transaction and return commit's
 * global snapshot.
 */
GlobalCSN
GlobalSnapshotPrepareCurrent()
{
	TransactionId xid = GetCurrentTransactionIdIfAny();

	if (!track_global_snapshots)
		ereport(ERROR,
			(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("could not prepare transaction for global commit"),
				errhint("Make sure the configuration parameter \"%s\" is enabled.",
						"track_global_snapshots")));

	if (TransactionIdIsValid(xid))
	{
		TransactionId *subxids;
		int nsubxids = xactGetCommittedChildren(&subxids);
		GlobalCSNLogSetCSN(xid, nsubxids,
									subxids, InDoubtGlobalCSN);
	}

	/* Nothing to write if we don't heve xid */

	return GlobalSnapshotGenerate(false);
}

/*
 * GlobalSnapshotAssignCsnCurrent
 *
 * Asign GlobalCSN for currently active transaction. GlobalCSN is supposedly
 * maximal among of values returned by GlobalSnapshotPrepareCurrent and
 * pg_global_snapshot_prepare.
 */
void
GlobalSnapshotAssignCsnCurrent(GlobalCSN global_csn)
{
	if (!track_global_snapshots)
		ereport(ERROR,
			(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("could not prepare transaction for global commit"),
				errhint("Make sure the configuration parameter \"%s\" is enabled.",
						"track_global_snapshots")));

	if (!GlobalCSNIsNormal(global_csn))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("pg_global_snapshot_assign expects normal global_csn")));

	/* Skip emtpty transactions */
	if (!TransactionIdIsValid(GetCurrentTransactionIdIfAny()))
		return;

	/* Set global_csn and defuse ProcArrayEndTransaction from assigning one */
	pg_atomic_write_u64(&MyProc->assignedGlobalCsn, global_csn);
}


/*****************************************************************************
 * Functions to handle global and local transactions commit.
 *
 * For local transactions GlobalSnapshotPrecommit sets InDoubt state before
 * ProcArrayEndTransaction is called and transaction data potetntially becomes
 * visible to other backends. ProcArrayEndTransaction (or ProcArrayRemove in
 * twophase case) then acquires global_csn under ProcArray lock and stores it
 * in proc->assignedGlobalCsn. It's important that global_csn for commit is
 * generated under ProcArray lock, otherwise global and local snapshots won't
 * be equivalent. Consequent call to GlobalSnapshotCommit will write
 * proc->assignedGlobalCsn to GlobalCSNLog.
 *
 * Same rules applies to global transaction, except that global_csn is already
 * assigned by GlobalSnapshotAssignCsnCurrent/pg_global_snapshot_assign and
 * GlobalSnapshotPrecommit is basically no-op.
 *
 * GlobalSnapshotAbort is slightly different comparing to commit because abort
 * can skip InDoubt phase and can be called for transaction subtree.
 *****************************************************************************/


/*
 * GlobalSnapshotAbort
 *
 * Abort transaction in GlobalCsnLog. We can skip InDoubt state for aborts
 * since no concurrent transactions allowed to see aborted data anyway.
 */
void
GlobalSnapshotAbort(PGPROC *proc, TransactionId xid,
					int nsubxids, TransactionId *subxids)
{
	if (!track_global_snapshots)
		return;

	GlobalCSNLogSetCSN(xid, nsubxids, subxids, AbortedGlobalCSN);

	/*
	 * Clean assignedGlobalCsn anyway, as it was possibly set in
	 * GlobalSnapshotAssignCsnCurrent.
	 */
	pg_atomic_write_u64(&proc->assignedGlobalCsn, InProgressGlobalCSN);
}

/*
 * GlobalSnapshotPrecommit
 *
 * Set InDoubt status for local transaction that we are going to commit.
 * This step is needed to achieve consistency between local snapshots and
 * global csn-based snapshots. We don't hold ProcArray lock while writing
 * csn for transaction in SLRU but instead we set InDoubt status before
 * transaction is deleted from ProcArray so the readers who will read csn
 * in the gap between ProcArray removal and GlobalCSN assignment can wait
 * until GlobalCSN is finally assigned. See also TransactionIdGetGlobalCSN().
 *
 * For global transaction this does nothing as InDoubt state was written
 * earlier.
 *
 * This should be called only from parallel group leader before backend is
 * deleted from ProcArray.
 */
void
GlobalSnapshotPrecommit(PGPROC *proc, TransactionId xid,
					int nsubxids, TransactionId *subxids)
{
	GlobalCSN oldAssignedGlobalCsn = InProgressGlobalCSN;
	bool in_progress;

	if (!track_global_snapshots)
		return;

	/* Set InDoubt status if it is local transaction */
	in_progress = pg_atomic_compare_exchange_u64(&proc->assignedGlobalCsn,
												 &oldAssignedGlobalCsn,
												 InDoubtGlobalCSN);
	if (in_progress)
	{
		Assert(GlobalCSNIsInProgress(oldAssignedGlobalCsn));
		GlobalCSNLogSetCSN(xid, nsubxids,
						   subxids, InDoubtGlobalCSN);
	}
	else
	{
		/* Otherwise we should have valid GlobalCSN by this time */
		Assert(GlobalCSNIsNormal(oldAssignedGlobalCsn));
		/* Also global transaction should already be in InDoubt state */
		Assert(GlobalCSNIsInDoubt(GlobalCSNLogGetCSN(xid)));
	}
}

/*
 * GlobalSnapshotCommit
 *
 * Write GlobalCSN that were acquired earlier to GlobalCsnLog. Should be
 * preceded by GlobalSnapshotPrecommit() so readers can wait until we finally
 * finished writing to SLRU.
 *
 * Should be called after ProcArrayEndTransaction, but before releasing
 * transaction locks, so that TransactionIdGetGlobalCSN can wait on this
 * lock for GlobalCSN.
 */
void
GlobalSnapshotCommit(PGPROC *proc, TransactionId xid,
					int nsubxids, TransactionId *subxids)
{
	volatile GlobalCSN assigned_global_csn;

	if (!track_global_snapshots)
		return;

	if (!TransactionIdIsValid(xid))
	{
		assigned_global_csn = pg_atomic_read_u64(&proc->assignedGlobalCsn);
		Assert(GlobalCSNIsInProgress(assigned_global_csn));
		return;
	}

	/* Finally write resulting GlobalCSN in SLRU */
	assigned_global_csn = pg_atomic_read_u64(&proc->assignedGlobalCsn);
	Assert(GlobalCSNIsNormal(assigned_global_csn));
	GlobalCSNLogSetCSN(xid, nsubxids,
						   subxids, assigned_global_csn);

	/* Reset for next transaction */
	pg_atomic_write_u64(&proc->assignedGlobalCsn, InProgressGlobalCSN);
}
