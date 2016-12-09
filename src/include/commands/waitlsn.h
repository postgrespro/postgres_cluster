/*-------------------------------------------------------------------------
 *
 * waitlsn.h
 *	  WaitLSN notification: WAITLSN
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 2016, Regents of PostgresPRO
 *
 * src/include/commands/waitlsn.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef WAITLSN_H
#define WAITLSN_H

extern void WaitLSNUtility(const char *lsn, const int delay);
extern void WaitLSNShmemInit(void);
extern void WaitLSNSetLatch(void);

#endif   /* WAITLSN_H */
