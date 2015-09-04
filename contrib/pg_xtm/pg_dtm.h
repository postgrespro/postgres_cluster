#ifndef DTM_BACKEND_H
#define DTM_BACKEND_H

typedef int nodeid_t;

typedef struct { 
    TransactionId xid;
    bool  is_global;
    bool  is_prepared;
    cid_t cid;
    cid_t snapshot;
} DtmTransState;

typedef char const* GlobalTransactionId;

/* Initialize DTM extension */
void  DtmInitialize(void);
/* Invoked at start of any local or global transaction */
void  DtmLocalBegin(DtmTransState* x);
/* Extend local transaction to global by assigning upper bound CSN which is returned to coordinator */
cid_t DtmLocalExtend(DtmTransState* x, GlobalTransactionId gtid);
/* Function called at first access to any datanode except first one involved in distributed transaction */
cid_t DtmLocalAccess(DtmTransState* x, GlobalTransactionId gtid, cid_t snapshot);
/* Mark transaction as in-doubt */
void  DtmLocalBeginPrepare(GlobalTransactionId gtid, nodeid_t coordinator);
/* Choose CSN for global transaction */
cid_t DtmLocalPrepare(GlobalTransactionId gtid, cid_t cid);
/* Assign CSN to global transaction */
void  DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid);
/* Do local commit of global transaction */
void  DtmLocalCommitPrepared(DtmTransState* x, GlobalTransactionId gtid);
/* Do local abort of global transaction */
void  DtmLocalAbortPrepared(DtmTransState* x, GlobalTransactionId gtid);
/* Do local commit of global transaction */
void  DtmLocalCommit(DtmTransState* x);
/* Do local abort of global transaction */
void  DtmLocalAbort(DtmTransState* x);
/* Invoked at the end of any local or global transaction: free transaction state */
void  DtmLocalEnd(DtmTransState* x);

#endif
