#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

extern int  LogicalReplicationStartReceivers(char* nodes, int node_id);
extern void MultimasterBeginTransaction(void);
extern void MultimasterJoinTransaction(TransactionId xid);
extern bool MultimasterIsExternalTransaction(TransactionId xid);

extern bool isBackgroundWorker;

#endif
