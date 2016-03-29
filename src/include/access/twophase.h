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
#include "datatype/timestamp.h"
#include "storage/lock.h"
#include "access/xlogreader.h"

/*
 * This struct describes one global transaction that is in prepared state
 * or attempting to become prepared.
 *
 * The lifecycle of a global transaction is:
 *
 * 1. After checking that the requested GID is not in use, set up an entry in
 * the TwoPhaseState->prepXacts array with the correct GID and valid = false,
 * and mark it as locked by my backend.
 *
 * 2. After successfully completing prepare, set valid = true and enter the
 * referenced PGPROC into the global ProcArray.
 *
 * 3. To begin COMMIT PREPARED or ROLLBACK PREPARED, check that the entry is
 * valid and not locked, then mark the entry as locked by storing my current
 * backend ID into locking_backend.  This prevents concurrent attempts to
 * commit or rollback the same prepared xact.
 *
 * 4. On completion of COMMIT PREPARED or ROLLBACK PREPARED, remove the entry
 * from the ProcArray and the TwoPhaseState->prepXacts array and return it to
 * the freelist.
 *
 * Note that if the preparing transaction fails between steps 1 and 2, the
 * entry must be removed so that the GID and the GlobalTransaction struct
 * can be reused.  See AtAbort_Twophase().
 *
 * typedef struct GlobalTransactionData *GlobalTransaction appears in
 * twophase.h
 *
 * Note that the max value of GIDSIZE must fit in the uint16 gidlen,
 * specified in TwoPhaseFileHeader.
 */
#define GIDSIZE 200

typedef struct GlobalTransactionData *GlobalTransaction;

typedef struct GlobalTransactionData
{
	GlobalTransaction next;		/* list link for free list */
	int			pgprocno;		/* ID of associated dummy PGPROC */
	BackendId	dummyBackendId; /* similar to backend id for backends */
	TimestampTz prepared_at;	/* time of preparation */

	/*
	 * Note that we need to keep track of two LSNs for each GXACT.
	 * We keep track of the start LSN because this is the address we must
	 * use to read state data back from WAL when committing a prepared GXACT.
	 * We keep track of the end LSN because that is the LSN we need to wait
	 * for prior to commit.
	 */
	XLogRecPtr	prepare_start_lsn;	/* XLOG offset of prepare record start */
	XLogRecPtr	prepare_end_lsn;	/* XLOG offset of prepare record end */

	Oid			owner;			/* ID of user that executed the xact */
	BackendId	locking_backend;	/* backend currently working on the xact */
	bool		valid;			/* TRUE if PGPROC entry is in proc array */
	bool		ondisk;			/* TRUE if prepare state file is on disk */
	char		gid[GIDSIZE];	/* The GID assigned to the prepared xact */
}	GlobalTransactionData;

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
extern void RecoverPreparedFromFiles(bool overwriteOK);
extern void RecoverPreparedFromXLOG(XLogReaderState *record);

extern void RecreateTwoPhaseFile(TransactionId xid, void *content, int len);
extern void RemoveTwoPhaseFile(TransactionId xid, bool giveWarning);

extern void CheckPointTwoPhase(XLogRecPtr redo_horizon);

extern void FinishPreparedTransaction(const char *gid, bool isCommit);

extern void XlogRedoFinishPrepared(TransactionId xid, bool isCommit);


extern GlobalTransaction RecoverPreparedFromBuffer(char *buf, bool forceOverwriteOK);

extern void MarkAsPrepared(GlobalTransaction gxact);
#endif   /* TWOPHASE_H */
