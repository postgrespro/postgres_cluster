/*
 * xtm.h
 *
 * PostgreSQL transaction-commit-log manager
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/xtm.h
 */
#ifndef XTM_H
#define XTM_H

#include "access/clog.h"
#include "utils/snapmgr.h"
#include "utils/relcache.h"

typedef struct
{
	/* Get current transaction status (encapsulation of TransactionIdGetStatus in clog.c) */
	XidStatus (*GetTransactionStatus)(TransactionId xid, XLogRecPtr *lsn);

	/* Set current transaction status (encapsulation of TransactionIdGetStatus in clog.c) */
	void (*SetTransactionStatus)(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);

	/* Get current transaction snaphot (encapsulation of GetSnapshotData in procarray.c) */
	Snapshot (*GetSnapshot)(Snapshot snapshot);

	/* Assign new Xid to transaction (encapsulation of GetNewTransactionId in varsup.c) */
	TransactionId (*GetNewTransactionId)(bool isSubXact);

	/* Get oldest transaction Xid that was running when any current transaction was started (encapsulation of GetOldestXmin in procarray.c) */
	TransactionId (*GetOldestXmin)(Relation rel, bool ignoreVacuum);

	/* Check if current transaction is not yet completed (encapsulation of TransactionIdIsInProgress in procarray.c) */
	bool (*IsInProgress)(TransactionId xid);

	/* Get global transaction XID: returns XID of current transaction if it is global, InvalidTransactionId otherwise */
	TransactionId (*GetGlobalTransactionId)(void);

	/* Is the given XID still-in-progress according to the snapshot (encapsulation of XidInMVCCSnapshot in tqual.c) */
	bool (*IsInSnapshot)(TransactionId xid, Snapshot snapshot);
} TransactionManager;

/* Get pointer to transaction manager: actually returns content of TM variable */
TransactionManager* GetTransactionManager(void);

extern TransactionManager* TM;  /* Current transaction manager (can be substituted by extensions) */
extern TransactionManager PgTM; /* Standard PostgreSQL transaction manager */

/* Standard PostgreSQL function implementing TM interface */
extern bool PgXidInMVCCSnapshot(TransactionId xid, Snapshot snapshot);

extern void PgTransactionIdSetTreeStatus(TransactionId xid, int nsubxids,
										TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
extern XidStatus PgTransactionIdGetStatus(TransactionId xid, XLogRecPtr *lsn);

extern Snapshot PgGetSnapshotData(Snapshot snapshot);

extern TransactionId PgGetOldestXmin(Relation rel, bool ignoreVacuum);

extern bool PgTransactionIdIsInProgress(TransactionId xid);

extern TransactionId PgGetGlobalTransactionId(void);

extern TransactionId PgGetNewTransactionId(bool isSubXact);

#endif
