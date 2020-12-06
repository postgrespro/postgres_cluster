/*-------------------------------------------------------------------------
 *
 * twophase.h
 *	  Two-phase-commit related declarations.
 *
 *
 * Portions Copyright (c) 1996-2020, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/twophase.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TWOPHASE_H
#define TWOPHASE_H

#include "access/xact.h"
#include "access/xlogdefs.h"
#include "datatype/timestamp.h"
#include "storage/lock.h"

typedef struct PreparedTransactionData
{
	Oid			owner;
	char		gid[GIDSIZE];	/* The GID assigned to the prepared xact */
	char        state_3pc[MAX_3PC_STATE_SIZE]; /* 3PC transaction state  */
} PreparedTransactionData, *PreparedTransaction;

/*
 * GlobalTransactionData is defined in twophase.c; other places have no
 * business knowing the internal definition.
 */
typedef struct GlobalTransactionData *GlobalTransaction;

/* GUC variable */
extern PGDLLIMPORT int max_prepared_xacts;

extern Size TwoPhaseShmemSize(void);
extern void TwoPhaseShmemInit(void);

extern void AtAbort_Twophase(void);
extern void PostPrepare_Twophase(void);

extern PGPROC *TwoPhaseGetDummyProc(TransactionId xid, bool lock_held);
extern BackendId TwoPhaseGetDummyBackendId(TransactionId xid, bool lock_held);

extern GlobalTransaction MarkAsPreparing(TransactionId xid, const char *gid,
										 TimestampTz prepared_at,
										 Oid owner, Oid databaseid,
										 const char *state_3pc);

extern void StartPrepare(GlobalTransaction gxact);
extern void EndPrepare(GlobalTransaction gxact);
extern bool StandbyTransactionIdIsPrepared(TransactionId xid);

extern TransactionId PrescanPreparedTransactions(TransactionId **xids_p,
												 int *nxids_p);
extern void StandbyRecoverPreparedTransactions(void);
extern void RecoverPreparedTransactions(void);

extern void CheckPointTwoPhase(XLogRecPtr redo_horizon);

extern void FinishPreparedTransaction(const char *gid, bool isCommit,
									  bool missing_ok);

extern int GetPreparedTransactions(PreparedTransaction* pxacts);

extern bool SetPreparedTransactionState(char const* gid, char const* state,
										bool missing_ok);

extern void PrepareRedoAdd(char *buf, XLogRecPtr start_lsn,
						   XLogRecPtr end_lsn, RepOriginId origin_id);
extern void PrepareRedoRemove(TransactionId xid, bool giveWarning);
extern void restoreTwoPhaseData(void);
#endif							/* TWOPHASE_H */
