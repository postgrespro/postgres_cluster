#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#include "bytebuf.h"
#include "bgwpool.h"

#define XTM_TRACE(fmt, ...)
/* #define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__) */
#define XTM_INFO(fmt, ...)

typedef uint64 csn_t;
#define INVALID_CSN  ((csn_t)-1)

typedef struct 
{
	int node;
	TransactionId xid;
} GlobalTransactionId;

typedef struct DtmTransState
{
    TransactionId xid;
	GlobalTransactionId gtid;
    XidStatus     status;
    csn_t         csn;      /* commit serial number   */
    csn_t         snapshot; /* transaction snapshot, or 0 for local transactions */
	int           nVotes;
	int           pid;
	struct DtmTransState* nextPending;
    struct DtmTransState* next;
} DtmTransState;

typedef struct
{
	volatile slock_t spinlock;
	PGSemaphoreData semaphore;
	LWLockId hashLock;
	TransactionId minXid;  /* XID of oldest transaction visible by any active transaction (local or global) */
	int64  disabledNodeMask;
    int    nNodes;
    pg_atomic_uint32 nReceivers;
    bool initialized;
	csn_t csn;
	DtmTransState* pendingTransactions;
    BgwPool pool;
	Latch latch;
} DtmState;

extern char* MMConnStrs;
extern int   MMNodeId
extern int   MMArbiterPort;

extern DtmState* dtm;

extern void MMArbiterInitialize();
extern int  MMStartReceivers(char* nodes, int node_id);
extern GlobalTransactionId MMBeginTransaction(void);
extern csn_t MMTransactionSnapshot(TransactionId xid);
extern void MMJoinTransaction(GlobalTransactionId gtid, csn_t snapshot);
extern void MMReceiverStarted(void);
extern void MMExecute(void* work, int size);
extern void MMExecutor(int id, void* work, size_t size);
extern HTAB* MMCreateHash();

extern char* MMDatabaseName;

#endif
