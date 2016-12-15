/*-------------------------------------------------------------------------
 *
 * waitlsn.c
 *	  WaitLSN statment: WAITLSN
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/commands/waitlsn.c
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Wait for LSN been replayed on slave as of 9.5:
 * README
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/pg_lsn.h"
#include "storage/latch.h"
#include "miscadmin.h"
#include "storage/spin.h"
#include "storage/backendid.h"
#include "access/xact.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/xlog_fn.h"
#include "utils/timestamp.h"
#include "storage/pmsignal.h"
#include "access/xlog.h"
#include "access/xlogdefs.h"
#include "commands/waitlsn.h"
#include "storage/proc.h"
#include "access/transam.h"

/* Latches Own-DisownLatch and AbortCаllBack */
static uint32 GetSHMEMSize(void);
static void WLDisownLatchAbort(XactEvent event, void *arg);
static void WLOwnLatch(void);
static void WLDisownLatch(void);

void		_PG_init(void);

/* Shared memory structures */
typedef struct
{
	int					pid;
	volatile slock_t	slock;
	Latch				latch;
} BIDLatch;

typedef struct
{
	char		dummy;
	BIDLatch	l_arr[FLEXIBLE_ARRAY_MEMBER];
} GlobState;

static volatile GlobState  *state;
bool						is_latch_owned = false;

static void
WLOwnLatch(void)
{
	SpinLockAcquire(&state->l_arr[MyBackendId].slock);
	OwnLatch(&state->l_arr[MyBackendId].latch);
	is_latch_owned = true;
	state->l_arr[MyBackendId].pid = MyProcPid;
	SpinLockRelease(&state->l_arr[MyBackendId].slock);
}

static void
WLDisownLatch(void)
{
	SpinLockAcquire(&state->l_arr[MyBackendId].slock);
	DisownLatch(&state->l_arr[MyBackendId].latch);
	is_latch_owned = false;
	state->l_arr[MyBackendId].pid = 0;
	SpinLockRelease(&state->l_arr[MyBackendId].slock);
}

/* CallBack function */
static void
WLDisownLatchAbort(XactEvent event, void *arg)
{
	if (is_latch_owned && (event == XACT_EVENT_PARALLEL_ABORT ||
						   event == XACT_EVENT_ABORT))
	{
		WLDisownLatch();
	}
}

/* Module load callback */
void
_PG_init(void)
{
	if (!IsUnderPostmaster)
		RegisterXactCallback(WLDisownLatchAbort, NULL);
}

static uint32
GetSHMEMSize(void)
{
	return offsetof(GlobState, l_arr) + sizeof(BIDLatch) * (MaxConnections+1);
}

void
WaitLSNShmemInit(void)
{
	bool	found;
	uint32	i;

	state = (GlobState *) ShmemInitStruct("pg_wait_lsn",
										  GetSHMEMSize(),
										  &found);
	if (!found)
	{
		for (i = 0; i < (MaxConnections+1); i++)
		{
			state->l_arr[i].pid = 0;
			SpinLockInit(&state->l_arr[i].slock);
			InitSharedLatch(&state->l_arr[i].latch);
		}
	}
}

void
WaitLSNSetLatch(void)
{
	uint32 i;
	for (i = 0; i < (MaxConnections+1); i++)
	{
		SpinLockAcquire(&state->l_arr[i].slock);
		if (state->l_arr[i].pid != 0)
			SetLatch(&state->l_arr[i].latch);
		SpinLockRelease(&state->l_arr[i].slock);
	}
}

void
WaitLSNUtility(const char *lsn, const int delay)
{
	XLogRecPtr		trg_lsn;
	XLogRecPtr		cur_lsn;
	int				latch_events;
	uint_fast64_t	tdelay = delay;
	long			secs;
	int				microsecs;
	TimestampTz		timer = GetCurrentTimestamp();

	trg_lsn = DatumGetLSN(DirectFunctionCall1(pg_lsn_in, CStringGetDatum(lsn)));

	if (delay > 0)
		latch_events = WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH;
	else
		latch_events = WL_LATCH_SET | WL_POSTMASTER_DEATH;

	WLOwnLatch();

	for (;;)
	{
		cur_lsn = GetXLogReplayRecPtr(NULL);

		/* If LSN had been Replayed */
		if (trg_lsn <= cur_lsn)
			break;

		/* If the postmaster dies, finish immediately */
		if (!PostmasterIsAlive())
			break;

		/* If Delay time is over */
		if (latch_events & WL_TIMEOUT)
		{
			if (TimestampDifferenceExceeds(timer,GetCurrentTimestamp(),tdelay))
				break;
			TimestampDifference(timer,GetCurrentTimestamp(),&secs, &microsecs);
			tdelay -= (secs*1000 + microsecs/1000);
			timer = GetCurrentTimestamp();
		}
		MyPgXact->xmin = InvalidTransactionId;
		WaitLatch(&state->l_arr[MyBackendId].latch, latch_events, tdelay);
		ResetLatch(&state->l_arr[MyBackendId].latch);
		CHECK_FOR_INTERRUPTS();

	}

	WLDisownLatch();

	if (trg_lsn > cur_lsn)
		elog(NOTICE,"LSN is not reached. Try to make bigger delay.");
}
