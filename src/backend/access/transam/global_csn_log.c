/*-----------------------------------------------------------------------------
 *
 * global_csn_log.c
 *		Track global commit sequence numbers of finished transactions
 *
 * Implementation of cross-node transaction isolation relies on commit sequence
 * number (CSN) based visibility rules.  This module provides SLRU to store
 * CSN for each transaction.  This mapping need to be kept only for xid's
 * greater then oldestXid, but that can require arbitrary large amounts of
 * memory in case of long-lived transactions.  Because of same lifetime and
 * persistancy requirements this module is quite similar to subtrans.c
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/access/transam/global_csn_log.c
 *
 *-----------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/global_csn_log.h"
#include "access/slru.h"
#include "access/subtrans.h"
#include "access/transam.h"
#include "miscadmin.h"
#include "pg_trace.h"
#include "utils/snapmgr.h"

bool track_global_snapshots;

/*
 * Defines for GlobalCSNLog page sizes.  A page is the same BLCKSZ as is used
 * everywhere else in Postgres.
 *
 * Note: because TransactionIds are 32 bits and wrap around at 0xFFFFFFFF,
 * GlobalCSNLog page numbering also wraps around at
 * 0xFFFFFFFF/GLOBAL_CSN_LOG_XACTS_PER_PAGE, and GlobalCSNLog segment numbering at
 * 0xFFFFFFFF/CLOG_XACTS_PER_PAGE/SLRU_PAGES_PER_SEGMENT.  We need take no
 * explicit notice of that fact in this module, except when comparing segment
 * and page numbers in TruncateGlobalCSNLog (see GlobalCSNLogPagePrecedes).
 */

/* We store the commit GlobalCSN for each xid */
#define GCSNLOG_XACTS_PER_PAGE (BLCKSZ / sizeof(GlobalCSN))

#define TransactionIdToPage(xid)	((xid) / (TransactionId) GCSNLOG_XACTS_PER_PAGE)
#define TransactionIdToPgIndex(xid) ((xid) % (TransactionId) GCSNLOG_XACTS_PER_PAGE)

/*
 * Link to shared-memory data structures for CLOG control
 */
static SlruCtlData GlobalCSNLogCtlData;
#define GlobalCsnlogCtl (&GlobalCSNLogCtlData)

static int	ZeroGlobalCSNLogPage(int pageno);
static bool GlobalCSNLogPagePrecedes(int page1, int page2);
static void GlobalCSNLogSetPageStatus(TransactionId xid, int nsubxids,
									  TransactionId *subxids,
									  GlobalCSN csn, int pageno);
static void GlobalCSNLogSetCSNInSlot(TransactionId xid, GlobalCSN csn,
									  int slotno);

/*
 * GlobalCSNLogSetCSN
 *
 * Record GlobalCSN of transaction and its subtransaction tree.
 *
 * xid is a single xid to set status for. This will typically be the top level
 * transactionid for a top level commit or abort. It can also be a
 * subtransaction when we record transaction aborts.
 *
 * subxids is an array of xids of length nsubxids, representing subtransactions
 * in the tree of xid. In various cases nsubxids may be zero.
 *
 * csn is the commit sequence number of the transaction. It should be
 * AbortedGlobalCSN for abort cases.
 */
void
GlobalCSNLogSetCSN(TransactionId xid, int nsubxids,
					 TransactionId *subxids, GlobalCSN csn)
{
	int			pageno;
	int			i = 0;
	int			offset = 0;

	/* Callers of GlobalCSNLogSetCSN() must check GUC params */
	Assert(track_global_snapshots);

	Assert(TransactionIdIsValid(xid));

	pageno = TransactionIdToPage(xid);		/* get page of parent */
	for (;;)
	{
		int			num_on_page = 0;

		while (i < nsubxids && TransactionIdToPage(subxids[i]) == pageno)
		{
			num_on_page++;
			i++;
		}

		GlobalCSNLogSetPageStatus(xid,
							num_on_page, subxids + offset,
							csn, pageno);
		if (i >= nsubxids)
			break;

		offset = i;
		pageno = TransactionIdToPage(subxids[offset]);
		xid = InvalidTransactionId;
	}
}

/*
 * Record the final state of transaction entries in the csn log for
 * all entries on a single page.  Atomic only on this page.
 *
 * Otherwise API is same as TransactionIdSetTreeStatus()
 */
static void
GlobalCSNLogSetPageStatus(TransactionId xid, int nsubxids,
						   TransactionId *subxids,
						   GlobalCSN csn, int pageno)
{
	int			slotno;
	int			i;

	LWLockAcquire(GlobalCSNLogControlLock, LW_EXCLUSIVE);

	slotno = SimpleLruReadPage(GlobalCsnlogCtl, pageno, true, xid);

	/* Subtransactions first, if needed ... */
	for (i = 0; i < nsubxids; i++)
	{
		Assert(GlobalCsnlogCtl->shared->page_number[slotno] == TransactionIdToPage(subxids[i]));
		GlobalCSNLogSetCSNInSlot(subxids[i],	csn, slotno);
	}

	/* ... then the main transaction */
	if (TransactionIdIsValid(xid))
		GlobalCSNLogSetCSNInSlot(xid, csn, slotno);

	GlobalCsnlogCtl->shared->page_dirty[slotno] = true;

	LWLockRelease(GlobalCSNLogControlLock);
}

/*
 * Sets the commit status of a single transaction.
 */
static void
GlobalCSNLogSetCSNInSlot(TransactionId xid, GlobalCSN csn, int slotno)
{
	int			entryno = TransactionIdToPgIndex(xid);
	GlobalCSN *ptr;

	Assert(LWLockHeldByMe(GlobalCSNLogControlLock));

	ptr = (GlobalCSN *) (GlobalCsnlogCtl->shared->page_buffer[slotno] + entryno * sizeof(XLogRecPtr));

	*ptr = csn;
}

/*
 * Interrogate the state of a transaction in the log.
 *
 * NB: this is a low-level routine and is NOT the preferred entry point
 * for most uses; TransactionIdGetGlobalCSN() in global_snapshot.c is the
 * intended caller.
 */
GlobalCSN
GlobalCSNLogGetCSN(TransactionId xid)
{
	int			pageno = TransactionIdToPage(xid);
	int			entryno = TransactionIdToPgIndex(xid);
	int			slotno;
	GlobalCSN *ptr;
	GlobalCSN	global_csn;

	/* Callers of GlobalCSNLogGetCSN() must check GUC params */
	Assert(track_global_snapshots);

	/* Can't ask about stuff that might not be around anymore */
	Assert(TransactionIdFollowsOrEquals(xid, TransactionXmin));

	/* lock is acquired by SimpleLruReadPage_ReadOnly */

	slotno = SimpleLruReadPage_ReadOnly(GlobalCsnlogCtl, pageno, xid);
	ptr = (GlobalCSN *) (GlobalCsnlogCtl->shared->page_buffer[slotno] + entryno * sizeof(XLogRecPtr));
	global_csn = *ptr;

	LWLockRelease(GlobalCSNLogControlLock);

	return global_csn;
}

/*
 * Number of shared GlobalCSNLog buffers.
 */
static Size
GlobalCSNLogShmemBuffers(void)
{
	return Min(32, Max(4, NBuffers / 512));
}

/*
 * Reserve shared memory for GlobalCsnlogCtl.
 */
Size
GlobalCSNLogShmemSize(void)
{
	if (!track_global_snapshots)
		return 0;

	return SimpleLruShmemSize(GlobalCSNLogShmemBuffers(), 0);
}

/*
 * Initialization of shared memory for GlobalCSNLog.
 */
void
GlobalCSNLogShmemInit(void)
{
	if (!track_global_snapshots)
		return;

	GlobalCsnlogCtl->PagePrecedes = GlobalCSNLogPagePrecedes;
	SimpleLruInit(GlobalCsnlogCtl, "GlobalCSNLog Ctl", GlobalCSNLogShmemBuffers(), 0,
				  GlobalCSNLogControlLock, "pg_global_csn", LWTRANCHE_GLOBAL_CSN_LOG_BUFFERS);
}

/*
 * This func must be called ONCE on system install.  It creates the initial
 * GlobalCSNLog segment.  The pg_global_csn directory is assumed to have been
 * created by initdb, and GlobalCSNLogShmemInit must have been called already.
 */
void
BootStrapGlobalCSNLog(void)
{
	int			slotno;

	if (!track_global_snapshots)
		return;

	LWLockAcquire(GlobalCSNLogControlLock, LW_EXCLUSIVE);

	/* Create and zero the first page of the commit log */
	slotno = ZeroGlobalCSNLogPage(0);

	/* Make sure it's written out */
	SimpleLruWritePage(GlobalCsnlogCtl, slotno);
	Assert(!GlobalCsnlogCtl->shared->page_dirty[slotno]);

	LWLockRelease(GlobalCSNLogControlLock);
}

/*
 * Initialize (or reinitialize) a page of GlobalCSNLog to zeroes.
 *
 * The page is not actually written, just set up in shared memory.
 * The slot number of the new page is returned.
 *
 * Control lock must be held at entry, and will be held at exit.
 */
static int
ZeroGlobalCSNLogPage(int pageno)
{
	Assert(LWLockHeldByMe(GlobalCSNLogControlLock));
	return SimpleLruZeroPage(GlobalCsnlogCtl, pageno);
}

/*
 * This must be called ONCE during postmaster or standalone-backend startup,
 * after StartupXLOG has initialized ShmemVariableCache->nextXid.
 *
 * oldestActiveXID is the oldest XID of any prepared transaction, or nextXid
 * if there are none.
 */
void
StartupGlobalCSNLog(TransactionId oldestActiveXID)
{
	int			startPage;
	int			endPage;

	if (!track_global_snapshots)
		return;

	/*
	 * Since we don't expect pg_global_csn to be valid across crashes, we
	 * initialize the currently-active page(s) to zeroes during startup.
	 * Whenever we advance into a new page, ExtendGlobalCSNLog will likewise
	 * zero the new page without regard to whatever was previously on disk.
	 */
	LWLockAcquire(GlobalCSNLogControlLock, LW_EXCLUSIVE);

	startPage = TransactionIdToPage(oldestActiveXID);
	endPage = TransactionIdToPage(ShmemVariableCache->nextXid);

	while (startPage != endPage)
	{
		(void) ZeroGlobalCSNLogPage(startPage);
		startPage++;
		/* must account for wraparound */
		if (startPage > TransactionIdToPage(MaxTransactionId))
			startPage = 0;
	}
	(void) ZeroGlobalCSNLogPage(startPage);

	LWLockRelease(GlobalCSNLogControlLock);
}

/*
 * This must be called ONCE during postmaster or standalone-backend shutdown
 */
void
ShutdownGlobalCSNLog(void)
{
	if (!track_global_snapshots)
		return;

	/*
	 * Flush dirty GlobalCSNLog pages to disk.
	 *
	 * This is not actually necessary from a correctness point of view. We do
	 * it merely as a debugging aid.
	 */
	TRACE_POSTGRESQL_GLOBALCSNLOG_CHECKPOINT_START(false);
	SimpleLruFlush(GlobalCsnlogCtl, false);
	TRACE_POSTGRESQL_GLOBALCSNLOG_CHECKPOINT_DONE(false);
}

/*
 * Perform a checkpoint --- either during shutdown, or on-the-fly
 */
void
CheckPointGlobalCSNLog(void)
{
	if (!track_global_snapshots)
		return;

	/*
	 * Flush dirty GlobalCSNLog pages to disk.
	 *
	 * This is not actually necessary from a correctness point of view. We do
	 * it merely to improve the odds that writing of dirty pages is done by
	 * the checkpoint process and not by backends.
	 */
	TRACE_POSTGRESQL_GLOBALCSNLOG_CHECKPOINT_START(true);
	SimpleLruFlush(GlobalCsnlogCtl, true);
	TRACE_POSTGRESQL_GLOBALCSNLOG_CHECKPOINT_DONE(true);
}

/*
 * Make sure that GlobalCSNLog has room for a newly-allocated XID.
 *
 * NB: this is called while holding XidGenLock.  We want it to be very fast
 * most of the time; even when it's not so fast, no actual I/O need happen
 * unless we're forced to write out a dirty clog or xlog page to make room
 * in shared memory.
 */
void
ExtendGlobalCSNLog(TransactionId newestXact)
{
	int			pageno;

	if (!track_global_snapshots)
		return;

	/*
	 * No work except at first XID of a page.  But beware: just after
	 * wraparound, the first XID of page zero is FirstNormalTransactionId.
	 */
	if (TransactionIdToPgIndex(newestXact) != 0 &&
		!TransactionIdEquals(newestXact, FirstNormalTransactionId))
		return;

	pageno = TransactionIdToPage(newestXact);

	LWLockAcquire(GlobalCSNLogControlLock, LW_EXCLUSIVE);

	/* Zero the page and make an XLOG entry about it */
	ZeroGlobalCSNLogPage(pageno);

	LWLockRelease(GlobalCSNLogControlLock);
}

/*
 * Remove all GlobalCSNLog segments before the one holding the passed
 * transaction ID.
 *
 * This is normally called during checkpoint, with oldestXact being the
 * oldest TransactionXmin of any running transaction.
 */
void
TruncateGlobalCSNLog(TransactionId oldestXact)
{
	int			cutoffPage;

	if (!track_global_snapshots)
		return;

	/*
	 * The cutoff point is the start of the segment containing oldestXact. We
	 * pass the *page* containing oldestXact to SimpleLruTruncate. We step
	 * back one transaction to avoid passing a cutoff page that hasn't been
	 * created yet in the rare case that oldestXact would be the first item on
	 * a page and oldestXact == next XID.  In that case, if we didn't subtract
	 * one, we'd trigger SimpleLruTruncate's wraparound detection.
	 */
	TransactionIdRetreat(oldestXact);
	cutoffPage = TransactionIdToPage(oldestXact);

	SimpleLruTruncate(GlobalCsnlogCtl, cutoffPage);
}

/*
 * Decide which of two GlobalCSNLog page numbers is "older" for truncation
 * purposes.
 *
 * We need to use comparison of TransactionIds here in order to do the right
 * thing with wraparound XID arithmetic.  However, if we are asked about
 * page number zero, we don't want to hand InvalidTransactionId to
 * TransactionIdPrecedes: it'll get weird about permanent xact IDs.  So,
 * offset both xids by FirstNormalTransactionId to avoid that.
 */
static bool
GlobalCSNLogPagePrecedes(int page1, int page2)
{
	TransactionId xid1;
	TransactionId xid2;

	xid1 = ((TransactionId) page1) * GCSNLOG_XACTS_PER_PAGE;
	xid1 += FirstNormalTransactionId;
	xid2 = ((TransactionId) page2) * GCSNLOG_XACTS_PER_PAGE;
	xid2 += FirstNormalTransactionId;

	return TransactionIdPrecedes(xid1, xid2);
}
