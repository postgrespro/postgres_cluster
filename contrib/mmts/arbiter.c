/*
 * arbiter.c
 *
 * Coordinate global transaction commit
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "postmaster/bgworker.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/xlogdefs.h"
#include "access/xact.h"
#include "access/xtm.h"
#include "access/transam.h"
#include "access/subtrans.h"
#include "access/commit_ts.h"
#include "access/xlog.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "executor/executor.h"
#include "access/twophase.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/tqual.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "postmaster/autovacuum.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "utils/syscache.h"
#include "replication/walsender.h"
#include "replication/slot.h"
#include "port/atomics.h"
#include "tcop/utility.h"

#ifndef USE_EPOLL
#ifdef __linux__
#define USE_EPOLL 1
#else
#define USE_EPOLL 0
#endif
#endif

#if USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/select.h>
#endif


#include "multimaster.h"

#define MAX_ROUTES      16
#define BUFFER_SIZE     1024
#define HANDSHAKE_MAGIC 0xCAFEDEED

typedef struct
{
	MtmMessageCode code; /* Message code: MSG_READY, MSG_PREPARE, MSG_COMMIT, MSG_ABORT */
    int            node; /* Sender node ID */
	TransactionId  dxid; /* Transaction ID at destination node */
	TransactionId  sxid; /* Transaction ID at sender node */  
	csn_t          csn;  /* local CSN in case of sending data from replica to master, global CSN master->replica */
	nodemask_t     disabledNodeMask; /* bitmask of disabled nodes at the sender of message */
} MtmArbiterMessage;

typedef struct 
{
	int used;
	MtmArbiterMessage data[BUFFER_SIZE];
} MtmBuffer;

static int*      sockets;
static char**    hosts;
static int       gateway;
static MtmState* ds;

static void MtmTransSender(Datum arg);
static void MtmTransReceiver(Datum arg);

static char const* const messageText[] = 
{
	"INVALID",
	"HANDSHAKE",
	"READY",
	"PREPARE",
	"COMMIT",
	"ABORT",
	"PREPARED",
	"COMMITTED",
	"ABORTED",
	"STATUS"
};


static BackgroundWorker MtmSender = {
	"mtm-sender",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	1, /* restart in one second (is it possible to restart immediately?) */
	MtmTransSender
};

static BackgroundWorker MtmRecevier = {
	"mtm-receiver",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	1, /* restart in one second (is it possible to restart immediately?) */
	MtmTransReceiver
};

void MtmArbiterInitialize(void)
{
	RegisterBackgroundWorker(&MtmSender);
	RegisterBackgroundWorker(&MtmRecevier);
}

static int 
MtmResolveHostByName(const char *hostname, unsigned* addrs, unsigned* n_addrs)
{
    struct sockaddr_in sin;
    struct hostent* hp;
    unsigned i;

    sin.sin_addr.s_addr = inet_addr(hostname);
    if (sin.sin_addr.s_addr != INADDR_NONE) {
        memcpy(&addrs[0], &sin.sin_addr.s_addr, sizeof(sin.sin_addr.s_addr));
        *n_addrs = 1;
        return 1;
    }

    hp = gethostbyname(hostname);
    if (hp == NULL || hp->h_addrtype != AF_INET) {
        return 0;
    }
    for (i = 0; hp->h_addr_list[i] != NULL && i < *n_addrs; i++) {
        memcpy(&addrs[i], hp->h_addr_list[i], sizeof(addrs[i]));
    }
    *n_addrs = i;
    return 1;
}

#if USE_EPOLL
static int    epollfd;
#else
static int    max_fd;
static fd_set inset;
#endif

static void MtmRegisterSocket(int fd, int node)
{
#if USE_EPOLL
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u32 = node;        
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        elog(ERROR, "Arbiter failed to add socket to epoll set: %d", errno);
    } 
#else
    FD_SET(fd, &inset);    
    if (fd > max_fd) {
        max_fd = fd;
    }
#endif          
}     

static void MtmUnregisterSocket(int fd)
{
#if USE_EPOLL
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
#else
	FD_CLR(fd, &inset); 
#endif
}


static void MtmDisconnect(int node)
{
	close(sockets[node]);
	MtmUnregisterSocket(sockets[node]);
	sockets[node] = -1;
}

static bool MtmWriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) {
        int n = send(sd, src, size, 0);
        if (n <= 0) {
			return false;
        }
        size -= n;
        src += n;
    }
	return true;
}

static int MtmReadSocket(int sd, void* buf, int buf_size)
{
	int rc = recv(sd, buf, buf_size, 0);
	if (rc <= 0) { 
		return -1;
	}
	return rc;
}



static int MtmConnectSocket(char const* host, int port, int max_attempts)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[MAX_ROUTES];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
	int sd;

    sock_inet.sin_family = AF_INET;
	sock_inet.sin_port = htons(port);

	if (!MtmResolveHostByName(host, addrs, &n_addrs)) {
		elog(ERROR, "Arbiter failed to resolve host '%s' by name", host);
	}
  Retry:
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
	    elog(ERROR, "Arbiter failed to create socket: %d", errno);
    }
    while (1) {
		int rc = -1;
		for (i = 0; i < n_addrs; ++i) {
			memcpy(&sock_inet.sin_addr, &addrs[i], sizeof sock_inet.sin_addr);
			do {
				rc = connect(sd, (struct sockaddr*)&sock_inet, sizeof(sock_inet));
			} while (rc < 0 && errno == EINTR);
			
			if (rc >= 0 || errno == EINPROGRESS) {
				break;
			}
		}
		if (rc < 0) {
			if ((errno != ENOENT && errno != ECONNREFUSED && errno != EINPROGRESS) || max_attempts == 0) {
				elog(WARNING, "Arbiter failed to connect to %s:%d: %d", host, port, errno);
				return -1;
			} else { 
				max_attempts -= 1;
				MtmSleep(MtmConnectTimeout);
			}
			continue;
		} else {
			int optval = 1;
			MtmArbiterMessage msg;
			setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
			setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char const*)&optval, sizeof(optval));

			msg.code = MSG_HANDSHAKE;
			msg.node = MtmNodeId;
			msg.dxid = HANDSHAKE_MAGIC;
			msg.sxid = ShmemVariableCache->nextXid;
			msg.csn  = MtmGetCurrentTime();
			msg.disabledNodeMask = ds->disabledNodeMask;
			if (!MtmWriteSocket(sd, &msg, sizeof msg)) { 
				elog(WARNING, "Arbiter failed to send handshake message to %s:%d: %d", host, port, errno);
				close(sd);
				goto Retry;
			}
			if (MtmReadSocket(sd, &msg, sizeof msg) != sizeof(msg)) { 
				elog(WARNING, "Arbiter failed to receive response for handshake message from %s:%d: errno=%d", host, port, errno);
				close(sd);
				goto Retry;
			}
			if (msg.code != MSG_STATUS || msg.dxid != HANDSHAKE_MAGIC) {
				elog(WARNING, "Arbiter get unexpected response %d for handshake message from %s:%d", msg.code, host, port);
				close(sd);
				goto Retry;
			}
				
			/* Some node cnosidered that I am dead, so switch to recovery mode */
			if (BIT_CHECK(msg.disabledNodeMask, MtmNodeId-1)) { 
				elog(WARNING, "Node is switched to recovery mode");
				ds->status = MTM_RECOVERY;
			}
			/* Combine disable masks from all node. Is it actually correct or we should better check availability of nodes ourselves? */
			ds->disabledNodeMask |= msg.disabledNodeMask;
			return sd;
		}
    }
}


static void MtmOpenConnections()
{
	int nNodes = MtmNodes;
	int i;
	char* connStr = pstrdup(MtmConnStrs);

	sockets = (int*)palloc(sizeof(int)*nNodes);
	hosts = (char**)palloc(sizeof(char*)*nNodes);

	for (i = 0; i < nNodes; i++) {
		char* host = strstr(connStr, "host=");
		char* end;
		if (host == NULL) {
			elog(ERROR, "Invalid connection string: '%s'", MtmConnStrs);
		}
		host += 5;
		for (end = host; *end != ' ' && *end != ',' && *end != '\0'; end++);
		if (*end != '\0') { 
			*end = '\0';
			connStr = end + 1;
		} else { 
			connStr = end;
		}
		hosts[i] = host;
		if (i+1 != MtmNodeId) { 
			sockets[i] = MtmConnectSocket(host, MtmArbiterPort + i + 1, MtmConnectAttempts);
			if (sockets[i] < 0) { 
				MtmOnNodeDisconnect(i+1);
			} 
		} else {
			sockets[i] = -1;
		}
	}
	if (ds->nNodes < MtmNodes/2+1) { /* no quorum */
		elog(WARNING, "Node is out of quorum: only %d nodes from %d are accssible", ds->nNodes, MtmNodes);
		ds->status = MTM_OFFLINE;
	} else if (ds->status == MTM_INITIALIZATION) { 
		elog(WARNING, "Switch to CONNECTED mode");
		ds->status = MTM_CONNECTED;
	}
}


static bool MtmSendToNode(int node, void const* buf, int size)
{
	while (sockets[node] < 0 || !MtmWriteSocket(sockets[node], buf, size)) { 
		elog(WARNING, "Arbiter failed to write socket: %d", errno);
		if (sockets[node] >= 0) { 
			close(sockets[node]);
		}
		sockets[node] = MtmConnectSocket(hosts[node], MtmArbiterPort + node + 1, MtmReconnectAttempts);
		if (sockets[node] < 0) { 
			MtmOnNodeDisconnect(node+1);
			return false;
		}
	}
	return true;
}

static int MtmReadFromNode(int node, void* buf, int buf_size)
{
	int rc = MtmReadSocket(sockets[node], buf, buf_size);
	if (rc <= 0) { 
		elog(WARNING, "Arbiter failed to read from node=%d, rc=%d, errno=%d", node+1, rc, errno);
		MtmDisconnect(node);
	}
	return rc;
}

static void MtmAcceptOneConnection()
{
	int fd = accept(gateway, NULL, NULL);
	if (fd < 0) {
		elog(WARNING, "Arbiter failed to accept socket: %d", errno);
	} else { 	
		MtmArbiterMessage msg;
		int rc = MtmReadSocket(fd, &msg, sizeof msg);
		if (rc < sizeof(msg)) { 
			elog(WARNING, "Arbiter failed to handshake socket: %d, errno=%d", rc, errno);
		} else if (msg.code != MSG_HANDSHAKE && msg.dxid != HANDSHAKE_MAGIC) { 
			elog(WARNING, "Arbiter get unexpected handshake message %d", msg.code);
			close(fd);
		} else{ 			
			Assert(msg.node > 0 && msg.node <= MtmNodes && msg.node != MtmNodeId);
			msg.code = MSG_STATUS;
			msg.disabledNodeMask = ds->disabledNodeMask;
			msg.dxid = HANDSHAKE_MAGIC;
			msg.sxid = ShmemVariableCache->nextXid;
			msg.csn  = MtmGetCurrentTime();
			if (!MtmWriteSocket(fd, &msg, sizeof msg)) { 
				elog(WARNING, "Arbiter failed to write response for handshake message to node %d", msg.node);
				close(fd);
			} else { 
				elog(NOTICE, "Arbiter established connection with node %d", msg.node); 
				BIT_CLEAR(ds->connectivityMask, msg.node-1);
				MtmOnNodeConnect(msg.node);
				MtmRegisterSocket(fd, msg.node-1);
				sockets[msg.node-1] = fd;
			}
		}
	}
}
	

static void MtmAcceptIncomingConnections()
{
	struct sockaddr_in sock_inet;
    int on = 1;
	int i;

	sockets = (int*)palloc(sizeof(int)*MtmNodes);

	sock_inet.sin_family = AF_INET;
	sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_inet.sin_port = htons(MtmArbiterPort + MtmNodeId);

    gateway = socket(sock_inet.sin_family, SOCK_STREAM, 0);
	if (gateway < 0) {
		elog(ERROR, "Arbiter failed to create socket: %d", errno);
	}
    setsockopt(gateway, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    if (bind(gateway, (struct sockaddr*)&sock_inet, sizeof(sock_inet)) < 0) {
		elog(ERROR, "Arbiter failed to bind socket: %d", errno);
	}	
    if (listen(gateway, MtmNodes) < 0) {
		elog(ERROR, "Arbiter failed to listen socket: %d", errno);
	}	

	sockets[MtmNodeId-1] = gateway;
	MtmRegisterSocket(gateway, MtmNodeId-1);

	for (i = 0; i < MtmNodes-1; i++) {
		MtmAcceptOneConnection();
	}
}


static void MtmAppendBuffer(MtmBuffer* txBuffer, TransactionId xid, int node, MtmTransState* ts)
{
	MtmBuffer* buf = &txBuffer[node];
	if (buf->used == BUFFER_SIZE) { 
		if (!MtmSendToNode(node, buf->data, buf->used*sizeof(MtmArbiterMessage))) {			
			buf->used = 0;
			return;
		}
		buf->used = 0;
	}
	MTM_TRACE("Send message %s CSN=%ld to node %d from node %d for global transaction %d/local transaction %d\n", 
			  messageText[ts->cmd], ts->csn, node+1, MtmNodeId, ts->gtid.xid, ts->xid);
	Assert(ts->cmd != MSG_INVALID);
	buf->data[buf->used].code = ts->cmd;
	buf->data[buf->used].dxid = xid;
	buf->data[buf->used].sxid = ts->xid;
	buf->data[buf->used].csn =  ts->csn;
	buf->data[buf->used].node = MtmNodeId;
	buf->data[buf->used].disabledNodeMask = ds->disabledNodeMask;
	buf->used += 1;
}

static void MtmBroadcastMessage(MtmBuffer* txBuffer, MtmTransState* ts)
{
	int i;
	int n = 1;
	for (i = 0; i < MtmNodes; i++)
	{
		if (TransactionIdIsValid(ts->xids[i])) { 
			Assert(i+1 != MtmNodeId);
			MtmAppendBuffer(txBuffer, ts->xids[i], i, ts);
			n += 1;
		}
	}
	Assert(n == ds->nNodes);
}

static void MtmTransSender(Datum arg)
{
	int nNodes = MtmNodes;
	int i;
	MtmBuffer* txBuffer = (MtmBuffer*)palloc(sizeof(MtmBuffer)*nNodes);
	
	ds = MtmGetState();

	MtmOpenConnections();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (true) {
		MtmTransState* ts;		
		PGSemaphoreLock(&ds->votingSemaphore);
		CHECK_FOR_INTERRUPTS();

		/* 
		 * Use shared lock to improve locality,
		 * because all other process modifying this list are using exclusive lock 
		 */
		MtmLock(LW_SHARED); 

		for (ts = ds->votingTransactions; ts != NULL; ts = ts->nextVoting) {
			if (MtmIsCoordinator(ts)) { 
				MtmBroadcastMessage(txBuffer, ts);
			} else { 
				MtmAppendBuffer(txBuffer, ts->gtid.xid, ts->gtid.node-1, ts);
			}
			ts->cmd = MSG_INVALID;
		}
		ds->votingTransactions = NULL;
		MtmUnlock();

		for (i = 0; i < nNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				MtmSendToNode(i, txBuffer[i].data, txBuffer[i].used*sizeof(MtmArbiterMessage));
				txBuffer[i].used = 0;
			}
		}		
	}
}

static void MtmWakeUpBackend(MtmTransState* ts)
{
	ts->done = true;
	SetLatch(&ProcGlobal->allProcs[ts->procno].procLatch); 
}

#if !USE_EPOLL
static bool MtmRecovery()
{
	int nNodes = MtmNodes;
	bool recovered = false;
    int i;

    for (i = 0; i < nNodes; i++) {
		int sd = sockets[i];
        if (sd >= 0 && FD_ISSET(sd, &inset)) {
            struct timeval tm = {0,0};
            fd_set tryset;
            FD_ZERO(&tryset);
            FD_SET(sd, &tryset);
            if (select(sd+1, &tryset, NULL, NULL, &tm) < 0) {
				elog(WARNING, "Arbiter lost connection with node %d", i+1);
				MtmDisconnect(i);
				recovered = true;
            }
        }
    }
	return recorvered;
}
#endif

static void MtmTransReceiver(Datum arg)
{
	int nNodes = MtmNodes;
	int nResponses;
	int i, j, n, rc;
	MtmBuffer* rxBuffer = (MtmBuffer*)palloc(sizeof(MtmBuffer)*nNodes);
	HTAB* xid2state;

#if USE_EPOLL
	struct epoll_event* events = (struct epoll_event*)palloc(sizeof(struct epoll_event)*nNodes);
    epollfd = epoll_create(nNodes);
#else
    FD_ZERO(&inset);
    max_fd = 0;
#endif
	
	ds = MtmGetState();

	MtmAcceptIncomingConnections();
	xid2state = MtmCreateHash();

	for (i = 0; i < nNodes; i++) { 
		rxBuffer[i].used = 0;
	}

	while (true) {
#if USE_EPOLL
        n = epoll_wait(epollfd, events, nNodes, -1);
		if (n < 0) { 
			elog(ERROR, "Arbiter failed to poll sockets: %d", errno);
		}
		for (j = 0; j < n; j++) {
			i = events[j].data.u32;
			if (events[j].events & EPOLLERR) {
				elog(WARNING, "Arbiter lost connection with node %d", i+1);
				MtmDisconnect(j);
			} 
			else if (events[j].events & EPOLLIN)  
#else
        fd_set events;
		do { 
			events = inset;
			rc = select(max_fd+1, &events, NULL, NULL, NULL);
		} while (rc < 0 && MtmRecovery());
		
		if (rc < 0) { 
			elog(ERROR, "Arbiter failed to select sockets: %d", errno);
		}
		for (i = 0; i < nNodes; i++) { 
			if (FD_ISSET(sockets[i], &events)) 
#endif
			{
				if (i+1 == MtmNodeId) { 
					Assert(sockets[i] == gateway);
					MtmAcceptOneConnection();
					continue;
				}  
				
				rc = MtmReadFromNode(i, (char*)rxBuffer[i].data + rxBuffer[i].used, BUFFER_SIZE-rxBuffer[i].used);
				if (rc <= 0) { 
					continue;
				}

				rxBuffer[i].used += rc;
				nResponses = rxBuffer[i].used/sizeof(MtmArbiterMessage);

				MtmLock(LW_EXCLUSIVE);						

				for (j = 0; j < nResponses; j++) { 
					MtmArbiterMessage* msg = &rxBuffer[i].data[j];
					MtmTransState* ts = (MtmTransState*)hash_search(xid2state, &msg->dxid, HASH_FIND, NULL);
					Assert(ts != NULL);
					Assert(ts->cmd == MSG_INVALID);
					Assert(msg->node > 0 && msg->node <= nNodes && msg->node != MtmNodeId);
					ts->xids[msg->node-1] = msg->sxid;

					if (MtmIsCoordinator(ts)) { 
						switch (msg->code) { 
						case MSG_READY:
							Assert(ts->status == TRANSACTION_STATUS_ABORTED || ts->status == TRANSACTION_STATUS_IN_PROGRESS);
							Assert(ts->nVotes < ds->nNodes);
							if (++ts->nVotes == ds->nNodes) { 
								/* All nodes are finished their transactions */
								if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) {
									ts->nVotes = 1; /* I voted myself */
									ts->cmd = MSG_PREPARE;
								} else { 
									ts->status = TRANSACTION_STATUS_ABORTED;
									ts->cmd = MSG_ABORT;
									MtmAdjustSubtransactions(ts);
									MtmWakeUpBackend(ts);								
								}
								MtmSendNotificationMessage(ts);									  
							}
							break;
						case MSG_PREPARED:
 						    Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
							Assert(ts->nVotes < ds->nNodes);
							if (msg->csn > ts->csn) {
								ts->csn = msg->csn;
								MtmSyncClock(ts->csn);
							}
							if (++ts->nVotes == ds->nNodes) { 
								/* ts->csn is maximum of CSNs at all nodes */
								ts->nVotes = 1; /* I voted myself */
								ts->cmd = MSG_COMMIT;
								ts->csn = MtmAssignCSN();
								ts->status = TRANSACTION_STATUS_UNKNOWN;
								MtmAdjustSubtransactions(ts);
								MtmSendNotificationMessage(ts);
							}
							break;
						case MSG_COMMITTED:
							Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
							Assert(ts->nVotes < ds->nNodes);
							if (++ts->nVotes == ds->nNodes) { 									
								/* All nodes have the same CSN */
								MtmWakeUpBackend(ts);
							}
							break;
						case MSG_ABORTED:
							Assert(ts->status == TRANSACTION_STATUS_ABORTED || ts->status == TRANSACTION_STATUS_IN_PROGRESS);
							Assert(ts->nVotes < ds->nNodes);
							ts->status = TRANSACTION_STATUS_ABORTED;									
							if (++ts->nVotes == ds->nNodes) { 
								ts->cmd = MSG_ABORT;
								MtmAdjustSubtransactions(ts);
								MtmSendNotificationMessage(ts);		
								MtmWakeUpBackend(ts);								
							}
							break;
						default:
							Assert(false);
						}
					} else { /* replica */
						switch (msg->code) { 
						case MSG_PREPARE:
 					        Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS); 
							if ((msg->disabledNodeMask & ~ds->disabledNodeMask) != 0) { 
								/* Coordinator's disabled mask is wider than my: so reject such transaction to avoid 
								   commit on smaller subset of nodes */
								ts->status = TRANSACTION_STATUS_ABORTED;
								ts->cmd = MSG_ABORT;
								MtmAdjustSubtransactions(ts);
								MtmWakeUpBackend(ts);
							} else { 
								ts->status = TRANSACTION_STATUS_UNKNOWN;
								ts->csn = MtmAssignCSN();
								ts->cmd = MSG_PREPARED;
							}
							MtmSendNotificationMessage(ts);
							break;
						case MSG_COMMIT:
							Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
							Assert(ts->csn < msg->csn);
							ts->csn = msg->csn;
							MtmSyncClock(ts->csn);
							ts->cmd = MSG_COMMITTED;							
							MtmAdjustSubtransactions(ts);
							MtmSendNotificationMessage(ts);
							MtmWakeUpBackend(ts);
							break;
						case MSG_ABORT:
							if (ts->status != TRANSACTION_STATUS_ABORTED) {
								Assert(ts->status == TRANSACTION_STATUS_UNKNOWN || ts->status == TRANSACTION_STATUS_IN_PROGRESS);
								ts->status = TRANSACTION_STATUS_ABORTED;								
								MtmAdjustSubtransactions(ts);
								MtmWakeUpBackend(ts);
							}
							break;
						default:
							Assert(false);
						}
					}
				}
				MtmUnlock();
				
				rxBuffer[i].used -= nResponses*sizeof(MtmArbiterMessage);
				if (rxBuffer[i].used != 0) { 
					memmove(rxBuffer[i].data, (char*)rxBuffer[i].data + nResponses*sizeof(MtmArbiterMessage), rxBuffer[i].used);
				}
			}
		}
	}
}

