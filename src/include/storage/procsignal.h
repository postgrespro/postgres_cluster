/*-------------------------------------------------------------------------
 *
 * procsignal.h
 *	  Routines for interprocess signalling
 *
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/procsignal.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PROCSIGNAL_H
#define PROCSIGNAL_H

#include "storage/backendid.h"


#define NUM_CUSTOM_PROCSIGNALS 64

/*
 * Reasons for signalling a Postgres child process (a backend or an auxiliary
 * process, like checkpointer).  We can cope with concurrent signals for different
 * reasons.  However, if the same reason is signaled multiple times in quick
 * succession, the process is likely to observe only one notification of it.
 * This is okay for the present uses.
 *
 * Also, because of race conditions, it's important that all the signals be
 * defined so that no harm is done if a process mistakenly receives one.
 */
typedef enum
{
	INVALID_PROCSIGNAL = -1,	/* Must be first */

	PROCSIG_CATCHUP_INTERRUPT,	/* sinval catchup interrupt */
	PROCSIG_NOTIFY_INTERRUPT,	/* listen/notify interrupt */
	PROCSIG_PARALLEL_MESSAGE,	/* message from cooperating parallel backend */

	/* Recovery conflict reasons */
	PROCSIG_RECOVERY_CONFLICT_DATABASE,
	PROCSIG_RECOVERY_CONFLICT_TABLESPACE,
	PROCSIG_RECOVERY_CONFLICT_LOCK,
	PROCSIG_RECOVERY_CONFLICT_SNAPSHOT,
	PROCSIG_RECOVERY_CONFLICT_BUFFERPIN,
	PROCSIG_RECOVERY_CONFLICT_STARTUP_DEADLOCK,

	PROCSIG_CUSTOM_1,
	/*
	 * PROCSIG_CUSTOM_2,
	 * ...,
	 * PROCSIG_CUSTOM_N-1,
	 */
	PROCSIG_CUSTOM_N = PROCSIG_CUSTOM_1 + NUM_CUSTOM_PROCSIGNALS - 1,

	NUM_PROCSIGNALS				/* Must be last! */
} ProcSignalReason;

/* Handler of custom process signal */
typedef void (*ProcSignalHandler_type) (void);

/*
 * prototypes for functions in procsignal.c
 */
extern Size ProcSignalShmemSize(void);
extern void ProcSignalShmemInit(void);

extern void ProcSignalInit(int pss_idx);
extern ProcSignalReason RegisterCustomProcSignalHandler(ProcSignalHandler_type handler);
extern ProcSignalHandler_type AssignCustomProcSignalHandler(ProcSignalReason reason,
			   ProcSignalHandler_type handler);
extern ProcSignalHandler_type GetCustomProcSignalHandler(ProcSignalReason reason);
extern int SendProcSignal(pid_t pid, ProcSignalReason reason,
			   BackendId backendId);

extern void CheckAndHandleCustomSignals(void);

extern void procsignal_sigusr1_handler(SIGNAL_ARGS);

#endif   /* PROCSIGNAL_H */
