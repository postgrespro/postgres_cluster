/*-------------------------------------------------------------------------
 *
 * twophase.h
 *	  Two-phase-commit related declarations.
 *
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/twophase.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TWOPHASE_H
#define TWOPHASE_H

#include "access/xlogdefs.h"
#include "access/xact.h"
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
extern int	max_prepared_xacts;

extern Size TwoPhaseShmemSize(void);
extern void TwoPhaseShmemInit(void);

extern void AtAbort_Twophase(void);
extern void PostPrepare_Twophase(void);

extern PGPROC *TwoPhaseGetDummyProc(TransactionId xid);
extern BackendId TwoPhaseGetDummyBackendId(TransactionId xid);

extern GlobalTransaction MarkAsPreparing(TransactionId xid, const char *gid,
				TimestampTz prepared_at,
				Oid owner, Oid databaseid);

extern void StartPrepare(GlobalTransaction gxact);
extern void EndPrepare(GlobalTransaction gxact);
extern bool StandbyTransactionIdIsPrepared(TransactionId xid);

extern TransactionId PrescanPreparedTransactions(TransactionId **xids_p,
							int *nxids_p);
extern void ParsePrepareRecord(uint8 info, char *xlrec,
							xl_xact_parsed_prepare *parsed);
extern void StandbyRecoverPreparedTransactions(bool overwriteOK);
extern void RecoverPreparedTransactions(void);

extern void RecreateTwoPhaseFile(TransactionId xid, void *content, int len);
extern void RemoveTwoPhaseFile(TransactionId xid, bool giveWarning);

extern void CheckPointTwoPhase(XLogRecPtr redo_horizon);

extern void FinishPreparedTransaction(const char *gid, bool isCommit);

extern const char *GetLockedGlobalTransactionId(void);

extern int FinishAllPreparedTransactions(bool isCommit);

extern int GetPreparedTransactions(PreparedTransaction* pxacts);

extern void SetPrepareTransactionState(char const* gid, char const* state);

#endif   /* TWOPHASE_H */
