/*-------------------------------------------------------------------------
 *
 * xtm.c
 *		PostgreSQL implementation of transaction manager protocol
 *
 * This module defines default iplementaiton of PostgreSQL transaction manager protocol
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/access/transam/xtm.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/transam.h"
#include "access/xtm.h"

TransactionId PgGetGlobalTransactionId(void)
{
	return InvalidTransactionId;
}

TransactionManager PgTM = {
	PgTransactionIdGetStatus,
	PgTransactionIdSetTreeStatus,
	PgGetSnapshotData,
	PgGetNewTransactionId,
	PgGetOldestXmin,
	PgTransactionIdIsInProgress,
	PgGetGlobalTransactionId,
	PgXidInMVCCSnapshot
};

TransactionManager* TM = &PgTM;

TransactionManager* GetTransactionManager(void)
{
	return TM;
}
