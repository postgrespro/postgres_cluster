#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#define XTM_TRACE(fmt, ...)
#define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
//#define XTM_INFO(fmt, ...)

extern int  MMStartReceivers(char* nodes, int node_id);
extern void MMBeginTransaction(void);
extern void MMJoinTransaction(TransactionId xid);
extern bool MMIsLocalTransaction(TransactionId xid);
extern void MMReceiverStarted(void);

#endif
