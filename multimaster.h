#ifndef __MULTIMASTER_H__
#define __MULTIMASTER_H__

#include "bytebuf.h"
#include "bgwpool.h"
#include "bkb.h"

#include "access/clog.h"
#include "pglogical_output/hooks.h"
#include "commands/vacuum.h"
#include "libpq-fe.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#ifndef MTM_TRACE
#define MTM_TRACE   0
#endif

#define MTM_TAG "[MTM] "
#define MTM_ELOG(level,fmt,...) elog(level, MTM_TAG fmt, ## __VA_ARGS__)
#define MTM_ERRMSG(fmt,...)     errmsg(MTM_TAG fmt, ## __VA_ARGS__)

#if DEBUG_LEVEL == 0
#define MTM_LOG1(fmt, ...) elog(LOG, "[MTM] " fmt, ## __VA_ARGS__) 
#define MTM_LOG2(fmt, ...) 
#define MTM_LOG3(fmt, ...) 
#define MTM_LOG4(fmt, ...) 
#elif  DEBUG_LEVEL == 1
#define MTM_LOG1(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG2(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG3(fmt, ...) 
#define MTM_LOG4(fmt, ...) 
#elif  DEBUG_LEVEL == 2
#define MTM_LOG1(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG2(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG3(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG4(fmt, ...) 
#elif  DEBUG_LEVEL >= 3
#define MTM_LOG1(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG2(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG3(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#define MTM_LOG4(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__) 
#endif

#if MTM_TRACE == 0
#define MTM_TXTRACE(tx, event)
#else
#define MTM_TXTRACE(tx, event) \
		fprintf(stderr, MTM_TAG "%s, %lld, %s, %d\n", tx->gid, (long long)MtmGetSystemTime(), event, MyProcPid)
#endif

#define MULTIMASTER_NAME                "multimaster"
#define MULTIMASTER_SCHEMA_NAME         "mtm"
#define MULTIMASTER_LOCAL_TABLES_TABLE  "local_tables"
#define MULTIMASTER_SLOT_PATTERN        "mtm_slot_%d"
#define MULTIMASTER_MIN_PROTO_VERSION   1
#define MULTIMASTER_MAX_PROTO_VERSION   1
#define MULTIMASTER_MAX_GID_SIZE        32
#define MULTIMASTER_MAX_SLOT_NAME_SIZE  16
#define MULTIMASTER_MAX_CONN_STR_SIZE   128
#define MULTIMASTER_MAX_HOST_NAME_SIZE  64
#define MULTIMASTER_MAX_LOCAL_TABLES    256
#define MULTIMASTER_MAX_CTL_STR_SIZE    256
#define MULTIMASTER_LOCK_BUF_INIT_SIZE  4096
#define MULTIMASTER_BROADCAST_SERVICE   "mtm_broadcast"
#define MULTIMASTER_ADMIN               "mtm_admin"
#define MULTIMASTER_PRECOMMITTED        "precommitted"

#define MULTIMASTER_DEFAULT_ARBITER_PORT 5433

#define MB ((size_t)1024*1024)

#define USEC_TO_MSEC(t) ((t)/1000)
#define MSEC_TO_USEC(t) ((timestamp_t)(t)*1000)

#define Natts_mtm_ddl_log 2
#define Anum_mtm_ddl_log_issued		1
#define Anum_mtm_ddl_log_query		2

#define Natts_mtm_local_tables 2
#define Anum_mtm_local_tables_rel_schema 1
#define Anum_mtm_local_tables_rel_name	 2

#define Natts_mtm_trans_state   15
#define Natts_mtm_nodes_state   17
#define Natts_mtm_cluster_state 20

typedef ulong64 csn_t; /* commit serial number */
#define INVALID_CSN  ((csn_t)-1)

typedef ulong64 lsn_t;
#define INVALID_LSN  InvalidXLogRecPtr

typedef char pgid_t[MULTIMASTER_MAX_GID_SIZE];

#define SELF_CONNECTIVITY_MASK  (Mtm->nodes[MtmNodeId-1].connectivityMask)

typedef enum
{ 
	PGLOGICAL_COMMIT,
	PGLOGICAL_PREPARE,
	PGLOGICAL_COMMIT_PREPARED,
	PGLOGICAL_ABORT_PREPARED,
	PGLOGICAL_PRECOMMIT_PREPARED
} PGLOGICAL_EVENT;

/* Identifier of global transaction */
typedef struct 
{
	int node;          /* Zero based index of node initiating transaction */
	TransactionId xid; /* Transaction ID at this node */
} GlobalTransactionId;

#define EQUAL_GTID(x,y) ((x).node == (y).node && (x).xid == (y).xid)

typedef enum
{ 
	MSG_INVALID,
	MSG_HANDSHAKE,
	MSG_PREPARED,
	MSG_PRECOMMIT,
	MSG_PRECOMMITTED,
	MSG_ABORTED,
	MSG_STATUS,
	MSG_HEARTBEAT,
	MSG_POLL_REQUEST,
	MSG_POLL_STATUS
} MtmMessageCode;

typedef enum
{
	MTM_INITIALIZATION, /* Initial status */
	MTM_OFFLINE,        /* Node is excluded from cluster */
	MTM_CONNECTED,      /* Arbiter is established connections with other nodes */
	MTM_ONLINE,         /* Ready to receive client's queries */
	MTM_RECOVERY,       /* Node is in recovery process */
	MTM_RECOVERED,      /* Node is recovered by is not yet switched to ONLINE because not all sender/receivers are restarted */
	MTM_IN_MINORITY,    /* Node is out of quorum */
	MTM_OUT_OF_SERVICE  /* Node is not available to to critical, non-recoverable error */
} MtmNodeStatus;

typedef enum
{
	REPLMODE_EXIT,         /* receiver should exit */
	REPLMODE_RECOVERED,    /* recovery of receiver node is completed so drop old slot and restart replication from the current position in WAL */
	REPLMODE_RECOVERY,     /* perform recovery of the node by applying all data from the slot from specified point */
	REPLMODE_CREATE_NEW,   /* destination node is recovered: drop old slot and restart from roveredLsn position */
	REPLMODE_OPEN_EXISTED  /* normal mode: use existed slot or create new one and start receiving data from it from the remembered position */
} MtmReplicationMode;

typedef struct
{
	MtmMessageCode code;   /* Message code: MSG_PREPARE, MSG_PRECOMMIT, MSG_COMMIT, MSG_ABORT,... */
    int            node;   /* Sender node ID */	
	bool           lockReq;/* Whether sender node needs to lock cluster tpo et wal-sender caught-up and complete recovery */
	TransactionId  dxid;   /* Transaction ID at destination node */
	TransactionId  sxid;   /* Transaction ID at sender node */  
    XidStatus      status; /* Transaction status */	
	csn_t          csn;    /* Local CSN in case of sending data from replica to master, global CSN master->replica */
	csn_t          oldestSnapshot; /* Oldest snapshot used by active transactions at this node */
	nodemask_t     disabledNodeMask; /* Bitmask of disabled nodes at the sender of message */
	nodemask_t     connectivityMask; /* Connectivity bitmask at the sender of message */
	pgid_t         gid;    /* Global transaction identifier */
} MtmArbiterMessage;

/*
 * Abort logical message is send by replica when error is happen while applying prepared transaction.
 * In this case we do not have prepared transaction and can not do abort-prepared.
 * But we have to record the fact of abort to be able to replay it in case of crash of coordinator of this transaction.
 * We are using logical abort message with code 'A' for it
 */
typedef struct MtmAbortLogicalMessage
{
	pgid_t    gid;
	int       origin_node;
	lsn_t     origin_lsn;
} MtmAbortLogicalMessage;

typedef struct MtmMessageQueue
{
	MtmArbiterMessage msg;
	struct MtmMessageQueue* next;
} MtmMessageQueue;

typedef struct 
{
	MtmArbiterMessage hdr;
	char connStr[MULTIMASTER_MAX_CONN_STR_SIZE];
} MtmHandshakeMessage;

typedef struct 
{
	int used;
	int size;
	MtmArbiterMessage* data;
} MtmBuffer;

typedef struct
{
	char hostName[MULTIMASTER_MAX_HOST_NAME_SIZE];
	char connStr[MULTIMASTER_MAX_CONN_STR_SIZE];
	int arbiterPort;
	int postmasterPort;
} MtmConnectionInfo;


typedef struct
{
	MtmConnectionInfo con;
	timestamp_t transDelay;
	timestamp_t lastStatusChangeTime;
	timestamp_t receiverStartTime;
	timestamp_t senderStartTime;
	timestamp_t lastHeartbeat;
	nodemask_t  disabledNodeMask;      /* Bitmask of disabled nodes received from this node */
	nodemask_t  connectivityMask;      /* Connectivity mask at this node */
	int         senderPid;
	int         receiverPid;
	lsn_t       flushPos;
	csn_t       oldestSnapshot;        /* Oldest snapshot used by active transactions at this node */	
	lsn_t       restartLSN;
	RepOriginId originId;
	int         timeline;
	void*       lockGraphData;
	int         lockGraphAllocated;
	int         lockGraphUsed;
	uint64      nHeartbeats;
} MtmNodeInfo;

typedef struct MtmL2List
{
	struct MtmL2List* next;
	struct MtmL2List* prev;
} MtmL2List;

typedef struct MtmTransState
{
    TransactionId  xid;
    XidStatus      status; 
	pgid_t         gid;                /* Global transaction ID (used for 2PC) */
	GlobalTransactionId gtid;          /* Transaction id at coordinator */
    csn_t          csn;                /* commit serial number */
    csn_t          snapshot;           /* transaction snapshot, or INVALID_CSN for local transactions */
	int            procno;             /* pgprocno of transaction coordinator waiting for responses from replicas, 
							              used to notify coordinator by arbiter */
	int            nSubxids;           /* Number of subtransanctions */
    struct MtmTransState* next;        /* Next element in L1 list of all finished transaction present in xid2state hash */
	MtmL2List      activeList;         /* L2-list of active transactions */
	bool           votingCompleted;    /* 2PC voting is completed */
	bool           isLocal;            /* Transaction is either replicated, either doesn't contain DML statements, so it should be ignored by pglogical replication */
	bool           isEnqueued;         /* Transaction is inserted in queue */
	bool           isPrepared;         /* Transaction is prepared: now it is safe to commit transaction */
	bool           isActive;           /* Transaction is active */
	bool           isTwoPhase;         /* User level 2PC */
	bool           isPinned;           /* Transaction oid protected from GC */
	int            nConfigChanges;     /* Number of cluster configuration changes at moment of transaction start */
	nodemask_t     participantsMask;   /* Mask of nodes involved in transaction */
	nodemask_t     votedMask;          /* Mask of voted nodes */
	TransactionId  xids[1];            /* [Mtm->nAllNodes]: transaction ID at replicas */
} MtmTransState;

typedef struct {
	pgid_t gid;
	bool   abort;
	XidStatus status;
	MtmTransState* state;
} MtmTransMap;

typedef struct
{
	MtmNodeStatus status;              /* Status of this node */
	int recoverySlot;                  /* NodeId of recovery slot or 0 if none */
	volatile slock_t queueSpinlock;    /* spinlock used to protect sender queue */
	PGSemaphoreData sendSemaphore;     /* semaphore used to notify mtm-sender about new responses to coordinator */
	LWLockPadded *locks;               /* multimaster lock tranche */
	TransactionId oldestXid;           /* XID of oldest transaction visible by any active transaction (local or global) */
	nodemask_t disabledNodeMask;       /* bitmask of disabled nodes */
	nodemask_t stalledNodeMask;        /* bitmask of stalled nodes (node with dropped replication slot which makes it not possible automatic recovery of such node) */
	nodemask_t stoppedNodeMask;        /* Bitmask of stopped (permanently disabled nodes) */
	nodemask_t pglogicalReceiverMask;  /* bitmask of started pglogic receivers */
	nodemask_t pglogicalSenderMask;    /* bitmask of started pglogic senders */
	nodemask_t walSenderLockerMask;    /* Mask of WAL-senders IDs locking the cluster */
	nodemask_t globalLockerMask;       /* Global cluster mask of locked nodes to perform caught-up (updated using heartbeats) */
	nodemask_t nodeLockerMask;         /* Mask of node IDs which WAL-senders are locking the cluster */
	nodemask_t reconnectMask; 	       /* Mask of nodes connection to which has to be reestablished by sender */
	int        lastLockHolder;         /* PID of process last obtaining the node lock */
	bool   localTablesHashLoaded;      /* Whether data from local_tables table is loaded in shared memory hash table */
	bool   preparedTransactionsLoaded; /* GIDs of prepared transactions are loaded at startup */
	int    inject2PCError;             /* Simulate error during 2PC commit at this node */
    int    nLiveNodes;                 /* Number of active nodes */
    int    nAllNodes;                  /* Total number of nodes */
    int    nReceivers;                 /* Number of initialized logical receivers (used to determine moment when initialization/recovery is completed) */
    int    nSenders;                   /* Number of started WAL senders (used to determine moment when recovery) */
	int    nLockers;                   /* Number of lockers */
	int    nActiveTransactions;        /* Number of active 2PC transactions */
	int    nConfigChanges;             /* Number of cluster configuration changes */
	int    recoveryCount;              /* Number of completed recoveries */
	int    donorNodeId;               /* Cluster node from which this node was populated */
	int64  timeShift;                  /* Local time correction */
	csn_t  csn;                        /* Last obtained timestamp: used to provide unique ascending CSNs based on system time */
	csn_t  lastCsn;                    /* CSN of last committed transaction */
	MtmTransState* votingTransactions; /* L1-list of replicated transactions notifications to coordinator.
									 	 This list is used to pass information to mtm-sender BGW */
    MtmTransState* transListHead;      /* L1 list of all finished transactions present in xid2state hash.
									 	  It is cleanup by MtmGetOldestXmin */
    MtmTransState** transListTail;     /* Tail of L1 list of all finished transactions, used to append new elements.
								  		  This list is expected to be in CSN ascending order, by strict order may be violated */
	MtmL2List activeTransList;         /* List of active transactions */
	ulong64 transCount;                /* Counter of transactions performed by this node */	
	ulong64 gcCount;                   /* Number of global transactions performed since last GC */
	MtmMessageQueue* sendQueue;        /* Messages to be sent by arbiter sender */
	MtmMessageQueue* freeQueue;        /* Free messages */
	lsn_t recoveredLSN;           /* LSN at the moment of recovery completion */
	BgwPool pool;                      /* Pool of background workers for applying logical replication patches */
	MtmNodeInfo nodes[1];              /* [Mtm->nAllNodes]: per-node data */ 
} MtmState;

typedef struct MtmFlushPosition
{
	dlist_node node;
	int        node_id;
	lsn_t      local_end;
	lsn_t      remote_end;
} MtmFlushPosition;


#define MtmIsCoordinator(ts) (ts->gtid.node == MtmNodeId)

extern char const* const MtmNodeStatusMnem[];
extern char const* const MtmTxnStatusMnem[];
extern char const* const MtmMessageKindMnem[];

extern MtmState* Mtm;

extern int   MtmNodeId;
extern int   MtmMaxNodes;
extern int   MtmReplicationNodeId;
extern int   MtmNodes;
extern int   MtmArbiterPort;
extern char* MtmDatabaseName;
extern char* MtmDatabaseUser;
extern int   MtmConnectTimeout;
extern int   MtmReconnectTimeout;
extern int   MtmNodeDisableDelay;
extern int   MtmTransSpillThreshold;
extern int   MtmHeartbeatSendTimeout;
extern int   MtmHeartbeatRecvTimeout;
extern bool  MtmUseDtm;
extern bool  MtmPreserveCommitOrder;
extern HTAB* MtmXid2State;
extern HTAB* MtmGid2State;
extern VacuumStmt* MtmVacuumStmt;
extern IndexStmt*  MtmIndexStmt;
extern DropStmt*   MtmDropStmt;
extern void*   MtmTablespaceStmt; /* CREATE/DELETE tablespace */
extern MemoryContext MtmApplyContext;
extern lsn_t MtmSenderWalEnd;
extern timestamp_t MtmRefreshClusterStatusSchedule;


extern void  MtmArbiterInitialize(void);
extern void  MtmStartReceivers(void);
extern void  MtmStartReceiver(int nodeId, bool dynamic);
extern csn_t MtmDistributedTransactionSnapshot(TransactionId xid, int nodeId, nodemask_t* participantsMask);
extern csn_t MtmAssignCSN(void);
extern csn_t MtmSyncClock(csn_t csn);
extern void  MtmJoinTransaction(GlobalTransactionId* gtid, csn_t snapshot, nodemask_t participantsMask);
extern void  MtmReceiverStarted(int nodeId);
extern MtmReplicationMode MtmGetReplicationMode(int nodeId, sig_atomic_t volatile* shutdown);
extern void  MtmExecute(void* work, int size);
extern void  MtmExecutor(void* work, size_t size);
extern void  MtmSend2PCMessage(MtmTransState* ts, MtmMessageCode cmd);
extern void  MtmSendMessage(MtmArbiterMessage* msg);
extern void  MtmAdjustSubtransactions(MtmTransState* ts);
extern void  MtmLock(LWLockMode mode);
extern void  MtmUnlock(void);
extern void  MtmLockNode(int nodeId, LWLockMode mode);
extern void  MtmUnlockNode(int nodeId);
extern void  MtmStopNode(int nodeId, bool dropSlot);
extern void  MtmReconnectNode(int nodeId);
extern void  MtmRecoverNode(int nodeId);
extern void  MtmOnNodeDisconnect(int nodeId);
extern void  MtmOnNodeConnect(int nodeId);
extern void  MtmWakeUpBackend(MtmTransState* ts);
extern void  MtmSleep(timestamp_t interval); 
extern void  MtmAbortTransaction(MtmTransState* ts);
extern void  MtmSetCurrentTransactionGID(char const* gid);
extern csn_t MtmGetTransactionCSN(TransactionId xid);
extern void  MtmSetCurrentTransactionCSN(csn_t csn);
extern TransactionId MtmGetCurrentTransactionId(void);
extern XidStatus MtmGetCurrentTransactionStatus(void);
extern XidStatus MtmExchangeGlobalTransactionStatus(char const* gid, XidStatus status);
extern bool  MtmIsRecoveredNode(int nodeId);
extern void  MtmRefreshClusterStatus(void);
extern void  MtmSwitchClusterMode(MtmNodeStatus mode);
extern void  MtmUpdateNodeConnectionInfo(MtmConnectionInfo* conn, char const* connStr);
extern void  MtmSetupReplicationHooks(struct PGLogicalHooks* hooks);
extern void  MtmCheckQuorum(void);
extern bool  MtmRecoveryCaughtUp(int nodeId, lsn_t walEndPtr);
extern void  MtmCheckRecoveryCaughtUp(int nodeId, lsn_t slotLSN);
extern void  MtmRecoveryCompleted(void);
extern void  MtmMakeRelationLocal(Oid relid);
extern void  MtmHandleApplyError(void);
extern void  MtmUpdateLsnMapping(int nodeId, lsn_t endLsn);
extern lsn_t MtmGetFlushPosition(int nodeId);
extern bool MtmWatchdog(timestamp_t now);
extern void MtmCheckHeartbeat(void);
extern void MtmResetTransaction(void);
extern void MtmUpdateLockGraph(int nodeId, void const* messageBody, int messageSize);
extern void MtmReleaseRecoverySlot(int nodeId);
extern PGconn *PQconnectdb_safe(const char *conninfo);
extern void MtmBeginSession(int nodeId);
extern void MtmEndSession(int nodeId, bool unlock);
extern void MtmFinishPreparedTransaction(MtmTransState* ts, bool commit);
extern void MtmRollbackPreparedTransaction(int nodeId, char const* gid);
extern bool MtmFilterTransaction(char* record, int size);
extern void MtmPrecommitTransaction(char const* gid);
extern char* MtmGucSerialize(void);
extern bool MtmTransIsActive(void);
extern MtmTransState* MtmGetActiveTransaction(MtmL2List* list);
extern void MtmReleaseLock(void);

#endif
