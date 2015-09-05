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

typedef struct
{
     XidStatus (*GetTransactionStatus)(TransactionId xid, XLogRecPtr *lsn);
     void (*SetTransactionStatus)(TransactionId xid, int nsubxids, TransactionId *subxids, XidStatus status, XLogRecPtr lsn);
     Snapshot (*GetSnapshot)(Snapshot snapshot);
} TransactionManager;

extern TransactionManager* TM;
extern TransactionManager DefaultTM;

#endif
