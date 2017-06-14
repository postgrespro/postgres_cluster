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

TransactionId
PgGetGlobalTransactionId(void)
{
	return InvalidTransactionId;
}

bool
PgDetectGlobalDeadLock(PGPROC *proc)
{
	return false;
}

char const *
PgGetTransactionManagerName(void)
{
	return "postgres";
}

size_t 
PgGetTransactionStateSize(void)
{
	return 0;
}

void
PgSerializeTransactionState(void* ctx)
{
}

void
PgDeserializeTransactionState(void* ctx)
{
}

void 
PgInitializeSequence(int64* init, int64* step)
{
	*init = 1;
	*step = 1;
}


void* PgCreateSavepointContext(void)
{
	return NULL;
}

void PgRestoreSavepointContext(void* ctx)
{
}

void PgReleaseSavepointContext(void* ctx)
{
}

TransactionManager PgTM = {
	PgTransactionIdGetStatus,
	PgTransactionIdSetTreeStatus,
	PgGetSnapshotData,
	PgGetNewTransactionId,
	PgGetOldestXmin,
	PgTransactionIdIsInProgress,
	PgGetGlobalTransactionId,
	PgXidInMVCCSnapshot,
	PgDetectGlobalDeadLock,
	PgGetTransactionManagerName,
	PgGetTransactionStateSize,
	PgSerializeTransactionState,
	PgDeserializeTransactionState,
	PgInitializeSequence,
	PgCreateSavepointContext,
	PgRestoreSavepointContext,
	PgReleaseSavepointContext
};

TransactionManager *TM = &PgTM;

TransactionManager *
GetTransactionManager(void)
{
	return TM;
}
