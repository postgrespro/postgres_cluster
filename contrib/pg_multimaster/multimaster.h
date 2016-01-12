#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#include "bytebuf.h"

#define XTM_TRACE(fmt, ...)
//#define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#define XTM_INFO(fmt, ...)

typedef int nodeid_t;
typedef uint64 cid_t;

typedef struct { 
    TransactionId xid;
    bool  is_global;
    bool  is_prepared;
    cid_t cid;
    cid_t snapshot;
} DtmCurrentTrans;

typedef char const* GlobalTransactionId;

/* Initialize DTM extension */
void  DtmInitialize(void);
/* Invoked at start of any local or global transaction */
void  DtmLocalBegin(DtmCurrentTrans* x);
/* Extend local transaction to global by assigning upper bound CSN which is returned to coordinator */
cid_t DtmLocalExtend(DtmCurrentTrans* x, GlobalTransactionId gtid);
/* Function called at first access to any datanode except first one involved in distributed transaction */
cid_t DtmLocalAccess(DtmCurrentTrans* x, GlobalTransactionId gtid, cid_t snapshot);
/* Mark transaction as in-doubt */
void  DtmLocalBeginPrepare(GlobalTransactionId gtid);
/* Choose CSN for global transaction */
cid_t DtmLocalPrepare(GlobalTransactionId gtid, cid_t cid);
/* Assign CSN to global transaction */
void  DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid);
/* Do local commit of global transaction */
void  DtmLocalCommitPrepared(DtmCurrentTrans* x, GlobalTransactionId gtid);
/* Do local abort of global transaction */
void  DtmLocalAbortPrepared(DtmCurrentTrans* x, GlobalTransactionId gtid);
/* Do local commit of global transaction */
void  DtmLocalCommit(DtmCurrentTrans* x);
/* Do local abort of global transaction */
void  DtmLocalAbort(DtmCurrentTrans* x);
/* Invoked at the end of any local or global transaction: free transaction state */
void  DtmLocalEnd(DtmCurrentTrans* x);
/* Save global preapred transactoin state */
void DtmLocalSavePreparedState(GlobalTransactionId gtid);

extern int  MMStartReceivers(char* nodes, int node_id);
extern void MMBeginTransaction(void);
extern void MMJoinTransaction(TransactionId xid);
extern bool MMIsLocalTransaction(TransactionId xid);
extern void MMReceiverStarted(void);
extern void MMExecute(void* work, int size);
extern void MMExecutor(int id, void* work, size_t size);

extern char* MMDatabaseName;

#endif
