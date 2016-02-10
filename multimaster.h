#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#include "bytebuf.h"
#include "bgwpool.h"

#define DTM_TRACE(fmt, ...)
/* #define XTM_INFO(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__) */
#define DTM_INFO(fmt, ...)

#define BIT_SET(mask, bit) ((mask) & ((int64)1 << (bit)))

#define MULTIMASTER_NAME "mmts"

typedef uint64 csn_t; /* commit serial number */
#define INVALID_CSN  ((csn_t)-1)

/* Identifier of global transaction */
typedef struct 
{
	int node;          /* Zero based index of node initiating transaction */
	TransactionId xid; /* Transaction ID at this node */
} GlobalTransactionId;

typedef struct DtmTransState
{
    TransactionId xid;
    XidStatus     status; 
	GlobalTransactionId gtid;
    csn_t         csn;      /* commit serial number */
    csn_t         snapshot; /* transaction snapshot, or INVALID_CSN for local transactions */
	int           nVotes;   /* number of votes received from replcas for this transaction: 
							   finally should be nNodes-1 */
	int           procno;   /* pgprocno of transaction coordinator waiting for responses from replicas, 
							   used to notify coordinator by arbiter */
	struct DtmTransState* nextVoting; /* Next element in L1-list of voting transactions. */
    struct DtmTransState* next; /* Next element in L1 list of all finished transaction present in xid2state hash */
	TransactionId xids[1];  /* transaction ID at replicas: varying size MMNodes */
} DtmTransState;

typedef struct
{
	volatile slock_t votingSpinlock;   /* spinlock used to protect votingTransactions list */
	PGSemaphoreData votingSemaphore;   /* semaphore used to notify mm-sender about new responses to coordinator */
	LWLockId hashLock;                 /* lock to synchronize access to hash table */
	TransactionId oldestXid;           /* XID of oldest transaction visible by any active transaction (local or global) */
	int64  disabledNodeMask;           /* bitmask of disable nodes (so no more than 64 nodes in multimaster:) */
    int    nNodes;                     /* number of active nodes */
    pg_atomic_uint32 nReceivers;       /* number of initialized logical receivers (used to determine moment when MM intialization is completed */
	long timeShift;                    /* local time correction */
    bool initialized;             
	csn_t csn;                         /* last obtained CSN: used to provide unique acending CSNs based on system time */
	DtmTransState* votingTransactions; /* L1-list of replicated transactions sendings notifications to coordinator.
									 	 This list is used to pass information to mm-sender BGW */
    DtmTransState* transListHead;      /* L1 list of all finished transactions present in xid2state hash.
									 	  It is cleanup by DtmGetOldestXmin */
    DtmTransState** transListTail;     /* Tail of L1 list of all finished transactionds, used to append new elements.
								  		  This list is expected to be in CSN ascending order, by strict order may be violated */
    BgwPool pool;                      /* Pool of background workers for applying logical replication patches */
} DtmState;

extern char* MMConnStrs;
extern int   MMNodeId;
extern int   MMNodes;
extern int   MMArbiterPort;
extern char* MMDatabaseName;

extern void  MMArbiterInitialize(void);
extern int   MMStartReceivers(char* nodes, int node_id);
extern csn_t MMTransactionSnapshot(TransactionId xid);
extern void  MMJoinTransaction(GlobalTransactionId* gtid, csn_t snapshot);
extern void  MMReceiverStarted(void);
extern void  MMExecute(void* work, int size);
extern void  MMExecutor(int id, void* work, size_t size);
extern HTAB* MMCreateHash(void);
extern DtmState* MMGetState(void);

#endif
