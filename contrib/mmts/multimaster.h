#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#include "bytebuf.h"

#define XTM_TRACE(fmt, ...)
/* #define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__) */
#define XTM_INFO(fmt, ...)

typedef int64 GlobalTransactionId;
typedef int64 csn_t;

extern int  MMStartReceivers(char* nodes, int node_id);
extern GlobalTransactionId MMBeginTransaction(void);
extern GlobalTransactionId MMTransactionSnapshot(TransactionId xid);
extern void MMJoinTransaction(GlobalTransactionId gtid, csn_t snapshot);
extern bool MMIsLocalTransaction(TransactionId xid);
extern void MMReceiverStarted(void);
extern void MMExecute(void* work, int size);
extern void MMExecutor(int id, void* work, size_t size);

extern char* MMDatabaseName;

#endif
