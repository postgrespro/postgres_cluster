#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

extern int  MMStartReceivers(char* nodes, int node_id);
extern void MMBeginTransaction(void);
extern void MMJoinTransaction(TransactionId xid);
extern bool MMIsLocalTransaction(TransactionId xid);

extern bool isBackgroundWorker;

#endif
