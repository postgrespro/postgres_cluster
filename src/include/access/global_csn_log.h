/*
 * global_csn_log.h
 *
 * Commit-Sequence-Number log.
 *
 * Portions Copyright (c) 1996-2014, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/global_csn_log.h
 */
#ifndef CSNLOG_H
#define CSNLOG_H

#include "access/xlog.h"
#include "utils/snapshot.h"

extern void GlobalCSNLogSetCSN(TransactionId xid, int nsubxids,
							   TransactionId *subxids, GlobalCSN csn);
extern GlobalCSN GlobalCSNLogGetCSN(TransactionId xid);

extern Size GlobalCSNLogShmemSize(void);
extern void GlobalCSNLogShmemInit(void);
extern void BootStrapGlobalCSNLog(void);
extern void StartupGlobalCSNLog(TransactionId oldestActiveXID);
extern void ShutdownGlobalCSNLog(void);
extern void CheckPointGlobalCSNLog(void);
extern void ExtendGlobalCSNLog(TransactionId newestXact);
extern void TruncateGlobalCSNLog(TransactionId oldestXact);

#endif   /* CSNLOG_H */