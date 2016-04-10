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
	csn_t          csn;  /* Local CSN in case of sending data from replica to master, global CSN master->replica */
	nodemask_t     disabledNodeMask; /* Bitmask of disabled nodes at the sender of message */
	csn_t          oldestSnapshot; /* Oldest snapshot used by active transactions at this node */
} MtmArbiterMessage;

typedef struct 
{
	MtmArbiterMessage hdr;
	char connStr[MULTIMASTER_MAX_CONN_STR_SIZE];
} MtmHandshakeMessage;

typedef struct 
{
	int used;
	MtmArbiterMessage data[BUFFER_SIZE];
} MtmBuffer;

static int*      sockets;
static int       gateway;

static void MtmTransSender(Datum arg);
static void MtmTransReceiver(Datum arg);

/*
 * static char const* const messageText[] = 
 * {
 *	"INVALID",
 *	"HANDSHAKE",
 *	"READY",
 *	"PREPARE",
 *	"PREPARED",
 *	"ABORTED",
 *	"STATUS"
 *};
 */

static BackgroundWorker MtmSender = {
	"mtm-sender",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	MULTIMASTER_BGW_RESTART_TIMEOUT,
	MtmTransSender
};

static BackgroundWorker MtmRecevier = {
	"mtm-receiver",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	MULTIMASTER_BGW_RESTART_TIMEOUT,
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

static int stop = 0;
static void SetStop(int sig)
{
	stop = 1;
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



static void MtmSetSocketOptions(int sd)
{
#ifdef TCP_NODELAY
	int on = 1;
	if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&on, sizeof(on)) < 0) {
		elog(WARNING, "Failed to set TCP_NODELAY: %m");
	}
#endif
	if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char const*)&on, sizeof(on)) < 0) {
		elog(WARNING, "Failed to set SO_KEEPALIVE: %m");
	}

	if (tcp_keepalives_idle) { 
#ifdef TCP_KEEPIDLE
		if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE,
					   (char *) &tcp_keepalives_idle, sizeof(tcp_keepalives_idle)) < 0)
		{
			elog(WARNING, "Failed to set TCP_KEEPIDLE: %m");
		}
#else
#ifdef TCP_KEEPALIVE
		if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPALIVE,
					   (char *) &tcp_keepalives_idle, sizeof(tcp_keepalives_idle)) < 0) 
		{
			elog(WARNING, "Failed to set TCP_KEEPALIVE: %m");
		}
#endif
#endif
	}
#ifdef TCP_KEEPINTVL
	if (tcp_keepalives_interval) { 
		if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL,
					   (char *) &tcp_keepalives_interval, sizeof(tcp_keepalives_interval)) < 0)
		{
			elog(WARNING, "Failed to set TCP_KEEPINTVL: %m");
		}
	}
#endif
#ifdef TCP_KEEPCNT
	if (tcp_keepalives_count) {
		if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPCNT,
					   (char *) &tcp_keepalives_count, sizeof(tcp_keepalives_count)) < 0)
		{
			elog(WARNING, "Failed to set TCP_KEEPCNT: %m");
		}
	}
#endif
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
    while (1) {
		int rc = -1;

		sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd < 0) {
			elog(ERROR, "Arbiter failed to create socket: %d", errno);
		}
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
				elog(WARNING, "Arbiter failed to connect to %s:%d: error=%d", host, port, errno);
				return -1;
			} else { 
				max_attempts -= 1;
				elog(WARNING, "Arbiter trying to connect to %s:%d: error=%d", host, port, errno);
				MtmSleep(5*MtmConnectTimeout);
			}
			continue;
		} else {
			MtmHandshakeMessage req;
			MtmArbiterMessage   resp;
			MtmSetSocketOptions(sd);
			req.hdr.code = MSG_HANDSHAKE;
			req.hdr.node = MtmNodeId;
			req.hdr.dxid = HANDSHAKE_MAGIC;
			req.hdr.sxid = ShmemVariableCache->nextXid;
			req.hdr.csn  = MtmGetCurrentTime();
			req.hdr.disabledNodeMask = Mtm->disabledNodeMask;
			strcpy(req.connStr, Mtm->nodes[MtmNodeId-1].con.connStr);
			if (!MtmWriteSocket(sd, &req, sizeof req)) { 
				elog(WARNING, "Arbiter failed to send handshake message to %s:%d: %d", host, port, errno);
				close(sd);
				goto Retry;
			}
			if (MtmReadSocket(sd, &resp, sizeof resp) != sizeof(resp)) { 
				elog(WARNING, "Arbiter failed to receive response for handshake message from %s:%d: errno=%d", host, port, errno);
				close(sd);
				goto Retry;
			}
			if (resp.code != MSG_STATUS || resp.dxid != HANDSHAKE_MAGIC) {
				elog(WARNING, "Arbiter get unexpected response %d for handshake message from %s:%d", resp.code, host, port);
				close(sd);
				goto Retry;
			}
				
			/* Some node considered that I am dead, so switch to recovery mode */
			if (BIT_CHECK(resp.disabledNodeMask, MtmNodeId-1)) { 
				elog(WARNING, "Node %d think that I am dead", resp.node);
				BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
				MtmSwitchClusterMode(MTM_RECOVERY);
			}
			return sd;
		}
    }
}


static void MtmOpenConnections()
{
	int nNodes = MtmNodes;
	int i;

	sockets = (int*)palloc(sizeof(int)*nNodes);

	for (i = 0; i < nNodes; i++) {
		if (i+1 != MtmNodeId) { 
			sockets[i] = MtmConnectSocket(Mtm->nodes[i].con.hostName, MtmArbiterPort + i + 1, MtmConnectAttempts);
			if (sockets[i] < 0) { 
				MtmOnNodeDisconnect(i+1);
			} 
		} else {
			sockets[i] = -1;
		}
	}
	if (Mtm->nNodes < MtmNodes/2+1) { /* no quorum */
		elog(WARNING, "Node is out of quorum: only %d nodes from %d are accssible", Mtm->nNodes, MtmNodes);
		MtmSwitchClusterMode(MTM_IN_MINORITY);
	} else if (Mtm->status == MTM_INITIALIZATION) { 
		MtmSwitchClusterMode(MTM_CONNECTED);
	}
}


static bool MtmSendToNode(int node, void const* buf, int size)
{	
	while (true) {
		if (sockets[node] >= 0 && BIT_CHECK(Mtm->reconnectMask, node)) {
			elog(WARNING, "Arbiter is forced to reconnect to node %d", node+1); 
			BIT_CLEAR(Mtm->reconnectMask, node);
			close(sockets[node]);
			sockets[node] = -1;
		}
		if (sockets[node] < 0 || !MtmWriteSocket(sockets[node], buf, size)) { 
			if (sockets[node] >= 0) { 
				elog(WARNING, "Arbiter failed to write to node %d: %d", node+1, errno);
				close(sockets[node]);
			}
			sockets[node] = MtmConnectSocket(Mtm->nodes[node].con.hostName, MtmArbiterPort + node + 1, MtmReconnectAttempts);
			if (sockets[node] < 0) { 
				MtmOnNodeDisconnect(node+1);
				return false;
			}
			MTM_LOG3("Arbiter restablished connection with node %d", node+1);
		} else { 
			return true;
		}
	}
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
		MtmHandshakeMessage req;
		MtmArbiterMessage resp;		
		int rc = MtmReadSocket(fd, &req, sizeof req);
		if (rc < sizeof(req)) { 
			elog(WARNING, "Arbiter failed to handshake socket: %d, errno=%d", rc, errno);
		} else if (req.hdr.code != MSG_HANDSHAKE && req.hdr.dxid != HANDSHAKE_MAGIC) { 
			elog(WARNING, "Arbiter get unexpected handshake message %d", req.hdr.code);
			close(fd);
		} else{ 			
			Assert(req.hdr.node > 0 && req.hdr.node <= MtmNodes && req.hdr.node != MtmNodeId);
			resp.code = MSG_STATUS;
			resp.disabledNodeMask = Mtm->disabledNodeMask;
			resp.dxid = HANDSHAKE_MAGIC;
			resp.sxid = ShmemVariableCache->nextXid;
			resp.csn  = MtmGetCurrentTime();
			resp.node = MtmNodeId;
			MtmUpdateNodeConnectionInfo(&Mtm->nodes[req.hdr.node-1].con, req.connStr);
			if (!MtmWriteSocket(fd, &resp, sizeof resp)) { 
				elog(WARNING, "Arbiter failed to write response for handshake message to node %d", resp.node);
				close(fd);
			} else { 
				MTM_LOG1("Arbiter established connection with node %d", req.hdr.node); 
				BIT_CLEAR(Mtm->connectivityMask, req.hdr.node-1);
				MtmRegisterSocket(fd, req.hdr.node-1);
				sockets[req.hdr.node-1] = fd;
				MtmOnNodeConnect(req.hdr.node);
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
	for (i = 0; i < MtmNodes; i++) { 
		sockets[i] = -1;
	}
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
	MTM_LOG3("Send %s message CSN=%ld to node %d from node %d for global transaction %d/local transaction %d", 
			 messageText[ts->cmd], ts->csn, node+1, MtmNodeId, ts->gtid.xid, ts->xid);

	Assert(ts->cmd != MSG_INVALID);
	buf->data[buf->used].code = ts->cmd;
	buf->data[buf->used].dxid = xid;
	buf->data[buf->used].sxid = ts->xid;
	buf->data[buf->used].csn  = ts->csn;
	buf->data[buf->used].node = MtmNodeId;
	buf->data[buf->used].disabledNodeMask = Mtm->disabledNodeMask;
	buf->data[buf->used].oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
	buf->used += 1;
}

static void MtmBroadcastMessage(MtmBuffer* txBuffer, MtmTransState* ts)
{
	int i;
	int n = 1;
	for (i = 0; i < MtmNodes; i++)
	{
		if (!BIT_CHECK(Mtm->disabledNodeMask, i) && TransactionIdIsValid(ts->xids[i])) { 
			Assert(i+1 != MtmNodeId);
			MtmAppendBuffer(txBuffer, ts->xids[i], i, ts);
			n += 1;
		}
	}
	Assert(n == Mtm->nNodes);
}


static void MtmTransSender(Datum arg)
{
	sigset_t sset;
	int nNodes = MtmNodes;
	int i;
	MtmBuffer* txBuffer = (MtmBuffer*)palloc(sizeof(MtmBuffer)*nNodes);

	signal(SIGINT, SetStop);
	signal(SIGQUIT, SetStop);
	signal(SIGTERM, SetStop);
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);

	MtmOpenConnections();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (!stop) {
		MtmTransState* ts;		
		PGSemaphoreLock(&Mtm->votingSemaphore);
		CHECK_FOR_INTERRUPTS();

		/* 
		 * Use shared lock to improve locality,
		 * because all other process modifying this list are using exclusive lock 
		 */
		MtmLock(LW_SHARED); 

		for (ts = Mtm->votingTransactions; ts != NULL; ts = ts->nextVoting) {
			if (MtmIsCoordinator(ts)) {
				MtmBroadcastMessage(txBuffer, ts);
			} else {
				MtmAppendBuffer(txBuffer, ts->gtid.xid, ts->gtid.node-1, ts);
			}
		}
		Mtm->votingTransactions = NULL;

		MtmUnlock();

		for (i = 0; i < nNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				MtmSendToNode(i, txBuffer[i].data, txBuffer[i].used*sizeof(MtmArbiterMessage));
				txBuffer[i].used = 0;
			}
		}		
	}
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
	return recovered;
}
#endif

static void MtmTransReceiver(Datum arg)
{
	sigset_t sset;
	int nNodes = MtmNodes;
	int nResponses;
	int i, j, n, rc;
	MtmBuffer* rxBuffer = (MtmBuffer*)palloc(sizeof(MtmBuffer)*nNodes);

#if USE_EPOLL
	struct epoll_event* events = (struct epoll_event*)palloc(sizeof(struct epoll_event)*nNodes);
    epollfd = epoll_create(nNodes);
#else
    FD_ZERO(&inset);
    max_fd = 0;
#endif

	signal(SIGINT, SetStop);
	signal(SIGQUIT, SetStop);
	signal(SIGTERM, SetStop);
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);
	
	MtmAcceptIncomingConnections();

	for (i = 0; i < nNodes; i++) { 
		rxBuffer[i].used = 0;
	}

	while (!stop) {
#if USE_EPOLL
        n = epoll_wait(epollfd, events, nNodes, MtmKeepaliveTimeout/1000);
		if (n < 0) { 
			if (errno == EINTR) { 
				continue;
			}
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
			struct timeval tv;
			events = inset;
			tv.tv_sec = MtmKeepaliveTimeout/USEC;
			tv.tv_usec = MtmKeepaliveTimeout%USEC;
			do { 
				n = select(max_fd+1, &events, NULL, NULL, &tv);
			} while (n < 0 && errno == EINTR);
		} while (n < 0 && MtmRecovery());
		
		if (n < 0) {
			elog(ERROR, "Arbiter failed to select sockets: %d", errno);
		}
		for (i = 0; i < nNodes; i++) { 
			if (sockets[i] >= 0 && FD_ISSET(sockets[i], &events)) 
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
					MtmTransState* ts = (MtmTransState*)hash_search(MtmXid2State, &msg->dxid, HASH_FIND, NULL);
					Assert(ts != NULL);
					Assert(msg->node > 0 && msg->node <= nNodes && msg->node != MtmNodeId);

					if (BIT_CHECK(msg->disabledNodeMask, MtmNodeId-1) && Mtm->status != MTM_RECOVERY) { 
						elog(PANIC, "Node %d thinks that I was dead: perform hara-kiri not to be a zombie", msg->node);
					}
					Mtm->nodes[msg->node-1].oldestSnapshot = msg->oldestSnapshot;

					if (MtmIsCoordinator(ts)) {
						switch (msg->code) { 
						  case MSG_READY:
							Assert(ts->nVotes < Mtm->nNodes);
							Mtm->nodes[msg->node-1].transDelay += MtmGetCurrentTime() - ts->csn;
							ts->xids[msg->node-1] = msg->sxid;

							if ((~msg->disabledNodeMask & Mtm->disabledNodeMask) != 0) { 
								/* Coordinator's disabled mask is wider than of this node: so reject such transaction to avoid 
								   commit on smaller subset of nodes */
								elog(WARNING, "Coordinator of distributed transaction see less nodes than node %d: %lx instead of %lx",
									 msg->node, (long) Mtm->disabledNodeMask, (long) msg->disabledNodeMask);
								MtmAbortTransaction(ts);
							}

							if (++ts->nVotes == Mtm->nNodes) { 
								/* All nodes are finished their transactions */
								if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) {
									ts->nVotes = 1; /* I voted myself */
									MtmSendNotificationMessage(ts, MSG_PREPARE);									  
								} else { 
									Assert(ts->status == TRANSACTION_STATUS_ABORTED);
									MtmWakeUpBackend(ts);								
								}
							}
							break;						   
						  case MSG_ABORTED:
							Assert(ts->nVotes < Mtm->nNodes);
							if (ts->status != TRANSACTION_STATUS_ABORTED) { 
								Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
								MtmAbortTransaction(ts);
							}
							if (++ts->nVotes == Mtm->nNodes) {
								MtmWakeUpBackend(ts);
							}
							break;
						  case MSG_PREPARED:
							Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
							Assert(ts->nVotes < Mtm->nNodes);
							if (msg->csn > ts->csn) {
								ts->csn = msg->csn;
								MtmSyncClock(ts->csn);
							}
							if (++ts->nVotes == Mtm->nNodes) {
								ts->csn = MtmAssignCSN();
								ts->status = TRANSACTION_STATUS_UNKNOWN;
								MtmWakeUpBackend(ts);
							}
							break;
						  default:
							Assert(false);
						} 
					} else { 
						switch (msg->code) { 
						  case MSG_PREPARE:
							Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);									
							ts->status = TRANSACTION_STATUS_UNKNOWN;
							ts->csn = MtmAssignCSN();
							MtmAdjustSubtransactions(ts);
							MtmSendNotificationMessage(ts, MSG_PREPARED);
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
		if (n == 0 && Mtm->disabledNodeMask != 0) { 
			/* If timeout is expired and there are didabled nodes, then recheck cluster's state */
			MtmRefreshClusterStatus(false);
		}
	}
}

