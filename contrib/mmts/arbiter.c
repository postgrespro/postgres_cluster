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
#include <fcntl.h>

#ifdef WITH_RSOCKET
#include <rdma/rsocket.h>
#endif

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "pg_socket.h"
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
#include "utils/timeout.h"
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
#include "libpq/ip.h"


#ifndef USE_EPOLL
#ifdef __linux__
#define USE_EPOLL 0
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

#define MAX_ROUTES       16
#define INIT_BUFFER_SIZE 1024
#define HANDSHAKE_MAGIC  0xCAFEDEED

static int*        sockets;
static int         gateway;
static bool        send_heartbeat;
static timestamp_t last_sent_heartbeat;
static TimeoutId   heartbeat_timer;
static nodemask_t  busy_mask;
static timestamp_t last_heartbeat_to_node[MAX_NODES];

static void MtmSender(Datum arg);
static void MtmReceiver(Datum arg);
static void MtmMonitor(Datum arg);
static void MtmSendHeartbeat(void);
static bool MtmSendToNode(int node, void const* buf, int size);

char const* const MtmMessageKindMnem[] = 
{
	"INVALID",
	"HANDSHAKE",
	"PREPARED",
	"PRECOMMIT",
	"PRECOMMITTED",
	"ABORTED",
	"STATUS",
	"HEARTBEAT",
	"POLL_REQUEST",
	"POLL_STATUS"
};

static BackgroundWorker MtmSenderWorker = {
	"mtm-sender",
	BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION, 
	BgWorkerStart_ConsistentState,
	MULTIMASTER_BGW_RESTART_TIMEOUT,
	MtmSender
};

static BackgroundWorker MtmRecevierWorker = {
	"mtm-receiver",
	BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION, 
	BgWorkerStart_ConsistentState,
	MULTIMASTER_BGW_RESTART_TIMEOUT,
	MtmReceiver
};

static BackgroundWorker MtmMonitorWorker = {
	"mtm-monitor",
	BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION, 
	BgWorkerStart_ConsistentState,
	MULTIMASTER_BGW_RESTART_TIMEOUT,
	MtmMonitor
};


void MtmArbiterInitialize(void)
{
	MTM_ELOG(LOG, "Register background workers");
	RegisterBackgroundWorker(&MtmSenderWorker);
	RegisterBackgroundWorker(&MtmRecevierWorker);
	RegisterBackgroundWorker(&MtmMonitorWorker);
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
		MTM_ELOG(LOG, "Arbiter failed to add socket to epoll set: %s", strerror(errno));
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
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) < 0) { 
		MTM_ELOG(LOG, "Arbiter failed to unregister socket from epoll set: %s", strerror(errno));
    } 
#else
	FD_CLR(fd, &inset); 
#endif
}


static void MtmDisconnect(int node)
{
	MtmUnregisterSocket(sockets[node]);
	pg_closesocket(sockets[node], MtmUseRDMA);
	sockets[node] = -1;
	MtmOnNodeDisconnect(node+1);
}

static int MtmWaitSocket(int sd, bool forWrite, timestamp_t timeoutMsec)
{	
	struct timeval tv;
	fd_set set;
	int rc;
	timestamp_t deadline = MtmGetSystemTime() + MSEC_TO_USEC(timeoutMsec);

	do { 
		timestamp_t now;
		MtmCheckHeartbeat();
        now = MtmGetSystemTime();
        if (now > deadline) { 
			now = deadline;
		}
		FD_ZERO(&set); 
		FD_SET(sd, &set); 
		tv.tv_sec = (deadline - now)/USECS_PER_SEC; 
		tv.tv_usec = (deadline - now)%USECS_PER_SEC;
	} while ((rc = pg_select(sd+1, forWrite ? NULL : &set, forWrite ? &set : NULL, NULL, &tv, MtmUseRDMA)) < 0 && errno == EINTR);

	return rc;
}

static bool MtmWriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) {
		int rc = MtmWaitSocket(sd, true, MtmHeartbeatSendTimeout);
		if (rc == 1) { 
			while ((rc = pg_send(sd, src, size, 0, MtmUseRDMA)) < 0 && errno == EINTR);			
			if (rc < 0) {
				if (errno == EINPROGRESS) { 
					continue;
				}
				return false;
			}
			size -= rc;
			src += rc;
		} else if (rc < 0) { 
			return false;
		}
    }
	return true;
}

static int MtmReadSocket(int sd, void* buf, int buf_size)
{
	int rc;
	while ((rc = pg_recv(sd, buf, buf_size, 0, MtmUseRDMA)) < 0 && errno == EINTR);			
	if (rc <= 0 && (errno == EAGAIN || errno == EINPROGRESS)) { 
		rc = MtmWaitSocket(sd, false, MtmHeartbeatSendTimeout);
		if (rc == 1) { 
			while ((rc = pg_recv(sd, buf, buf_size, 0, MtmUseRDMA)) < 0 && errno == EINTR);			
		}
	}
	return rc;
}



static void MtmSetSocketOptions(int sd)
{
#ifdef TCP_NODELAY
	int on = 1;
	if (pg_setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&on, sizeof(on), MtmUseRDMA) < 0) {
		MTM_ELOG(WARNING, "Failed to set TCP_NODELAY: %m");
	}
#endif
	if (pg_setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char const*)&on, sizeof(on), MtmUseRDMA) < 0) {
		MTM_ELOG(WARNING, "Failed to set SO_KEEPALIVE: %m");
	}

	if (tcp_keepalives_idle) { 
#ifdef TCP_KEEPIDLE
		if (pg_setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE,
						  (char *) &tcp_keepalives_idle, sizeof(tcp_keepalives_idle), MtmUseRDMA) < 0)
		{
			MTM_ELOG(WARNING, "Failed to set TCP_KEEPIDLE: %m");
		}
#else
#ifdef TCP_KEEPALIVE
		if (pg_setsockopt(sd, IPPROTO_TCP, TCP_KEEPALIVE,
						  (char *) &tcp_keepalives_idle, sizeof(tcp_keepalives_idle), MtmUseRDMA) < 0) 
		{
			MTM_ELOG(WARNING, "Failed to set TCP_KEEPALIVE: %m");
		}
#endif
#endif
	}
#ifdef TCP_KEEPINTVL
	if (tcp_keepalives_interval) { 
		if (pg_setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL,
						  (char *) &tcp_keepalives_interval, sizeof(tcp_keepalives_interval), MtmUseRDMA) < 0)
		{
			MTM_ELOG(WARNING, "Failed to set TCP_KEEPINTVL: %m");
		}
	}
#endif
#ifdef TCP_KEEPCNT
	if (tcp_keepalives_count) {
		if (pg_setsockopt(sd, IPPROTO_TCP, TCP_KEEPCNT,
						  (char *) &tcp_keepalives_count, sizeof(tcp_keepalives_count), MtmUseRDMA) < 0)
		{
			MTM_ELOG(WARNING, "Failed to set TCP_KEEPCNT: %m");
		}
	}
#endif
}

/*
 * Check response message and update onde state
 */
static void MtmCheckResponse(MtmArbiterMessage* resp)
{
	if (resp->lockReq) {
		BIT_SET(Mtm->inducedLockNodeMask, resp->node-1);
	} else { 
		BIT_CLEAR(Mtm->inducedLockNodeMask, resp->node-1);
	}
	if (resp->locked) {
		BIT_SET(Mtm->currentLockNodeMask, resp->node-1);
	} else { 
		BIT_CLEAR(Mtm->currentLockNodeMask, resp->node-1);
	}
	if (BIT_CHECK(resp->disabledNodeMask, MtmNodeId-1) 
		&& !BIT_CHECK(Mtm->disabledNodeMask, resp->node-1)
		&& Mtm->status != MTM_RECOVERY
		&& Mtm->status != MTM_RECOVERED
		&& Mtm->nodes[MtmNodeId-1].lastStatusChangeTime + MSEC_TO_USEC(MtmNodeDisableDelay) < MtmGetSystemTime()) 
	{ 
		MTM_ELOG(WARNING, "Node %d thinks that I'm dead, while I'm %s (message %s)", resp->node, MtmNodeStatusMnem[Mtm->status], MtmMessageKindMnem[resp->code]);
		BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
		Mtm->nConfigChanges += 1;
		MtmSwitchClusterMode(MTM_RECOVERY);
	} else if (BIT_CHECK(Mtm->disabledNodeMask, resp->node-1) && sockets[resp->node-1] < 0) { 
		/* We receive heartbeat from disabled node.
		 * Looks like it is restarted.
		 * Try to reconnect to it.
		 */
		MTM_ELOG(WARNING, "Receive heartbeat from disabled node %d", resp->node);		
		BIT_SET(Mtm->reconnectMask, resp->node-1);
	}	
}

static void MtmScheduleHeartbeat()
{
	if (!stop) { 
		enable_timeout_after(heartbeat_timer, MtmHeartbeatSendTimeout);
		send_heartbeat = true;
	}
	PGSemaphoreUnlock(&Mtm->sendSemaphore);
}
	
static void MtmSendHeartbeat()
{
	int i;
	MtmArbiterMessage msg;
	timestamp_t now = MtmGetSystemTime();
	MtmInitMessage(&msg, MSG_HEARTBEAT);
	msg.node = MtmNodeId;
	msg.csn = now;
	if (last_sent_heartbeat != 0 && last_sent_heartbeat + MSEC_TO_USEC(MtmHeartbeatSendTimeout)*2 < now) { 
		MTM_LOG1("More than %lld microseconds since last heartbeat", now - last_sent_heartbeat);
	}
	last_sent_heartbeat = now;

	for (i = 0; i < Mtm->nAllNodes; i++)
	{
		if (i+1 != MtmNodeId) { 
			if (!BIT_CHECK(busy_mask, i)
				&& (Mtm->status != MTM_ONLINE 
					|| sockets[i] >= 0 
					|| !BIT_CHECK(Mtm->disabledNodeMask, i)
					|| BIT_CHECK(Mtm->reconnectMask, i)))
			{ 
				if (!MtmSendToNode(i, &msg, sizeof(msg))) {
					MTM_ELOG(LOG, "Arbiter failed to send heartbeat to node %d", i+1);
				} else {
					if (last_heartbeat_to_node[i] + MSEC_TO_USEC(MtmHeartbeatSendTimeout)*2 < now) { 
						MTM_LOG1("Last heartbeat to node %d was sent %lld microseconds ago", i+1, now - last_heartbeat_to_node[i]);
					}
					last_heartbeat_to_node[i] = now;
					/* Connectivity mask can be cleared by MtmWatchdog: in this case sockets[i] >= 0 */
					if (BIT_CHECK(SELF_CONNECTIVITY_MASK, i)) { 
						MTM_LOG1("Force reconnect to node %d", i+1);    
						pg_closesocket(sockets[i], MtmUseRDMA);
						sockets[i] = -1;
						MtmReconnectNode(i+1); /* set reconnect mask to force node reconnent */
					}
					MTM_LOG4("Send heartbeat to node %d with timestamp %lld", i+1, now);    
				}
			} else { 
				MTM_LOG2("Do not send heartbeat to node %d, busy mask %lld, status %s", i+1, busy_mask, MtmNodeStatusMnem[Mtm->status]);
			}
		}
	}
	
}

/* This function should be called from all places where sender can be blocked.
 * It checks send_heartbeat flag set by timer and if it is set then sends heartbeats to all alive nodes 
 */
void MtmCheckHeartbeat()
{
	if (send_heartbeat && !stop) {
		send_heartbeat = false;
		enable_timeout_after(heartbeat_timer, MtmHeartbeatSendTimeout);
		MtmSendHeartbeat();
	}			
}


static int MtmConnectSocket(int node, int port)
{
 	struct addrinfo *addrs = NULL;
	struct addrinfo *addr;
	struct addrinfo hint;
	char portstr[MAXPGPATH];
	MtmHandshakeMessage req;
	MtmArbiterMessage   resp;
	int sd = -1;
	int rc;
	char const* host = Mtm->nodes[node].con.hostName;
	nodemask_t save_mask = busy_mask;

	/* Initialize hint structure */
	MemSet(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_UNSPEC;

	snprintf(portstr, sizeof(portstr), "%d", port);

	rc = pg_getaddrinfo_all(host, portstr, &hint, &addrs);
	if (rc != 0)
	{
		MTM_ELOG(LOG, "Arbiter failed to resolve host '%s' by name: %s", host, gai_strerror(rc));
		return -1;
	}
	BIT_SET(busy_mask, node);
	
  Retry:

	sd = pg_socket(AF_INET, SOCK_STREAM, 0, MtmUseRDMA);
	if (sd < 0) {
		MTM_ELOG(LOG, "Arbiter failed to create socket: %s", strerror(errno));
		goto Error;
	}
	rc = pg_fcntl(sd, F_SETFL, O_NONBLOCK, MtmUseRDMA);
	if (rc < 0) {
		MTM_ELOG(LOG, "Arbiter failed to switch socket to non-blocking mode: %s", strerror(errno));
		goto Error;
	}
	for (addr = addrs; addr != NULL; addr = addr->ai_next)
	{
		do {
			rc = pg_connect(sd, addr->ai_addr, addr->ai_addrlen, MtmUseRDMA);
		} while (rc < 0 && errno == EINTR);

		if (rc >= 0 || errno == EINPROGRESS) {
			break;
		}
	}

	if (rc != 0 && errno == EINPROGRESS) {
		rc = MtmWaitSocket(sd, true, MtmHeartbeatSendTimeout);
		if (rc == 1) {
			socklen_t	optlen = sizeof(int);
			int			errcode;

			if (getsockopt(sd, SOL_SOCKET, SO_ERROR, (void*)&errcode, &optlen) < 0) {
				MTM_ELOG(WARNING, "Arbiter failed to getsockopt for %s:%d: %s", host, port, strerror(errcode));
				goto Error;
			}
			if (errcode != 0) {
				MTM_ELOG(WARNING, "Arbiter trying to connect to %s:%d: %s", host, port, strerror(errcode));
				goto Error;
			}
		} else {
			MTM_ELOG(WARNING, "Arbiter waiting socket to %s:%d: %s", host, port, strerror(errno));
		}
	}
	else if (rc != 0) {
		MTM_ELOG(WARNING, "Arbiter failed to connect to %s:%d: (%d) %s", host, port, rc, strerror(errno));
		goto Error;
	}

	MtmSetSocketOptions(sd);
	MtmInitMessage(&req.hdr, MSG_HANDSHAKE);
	req.hdr.node = MtmNodeId;
	req.hdr.dxid = HANDSHAKE_MAGIC;
	req.hdr.sxid = ShmemVariableCache->nextXid;
	req.hdr.csn  = MtmGetCurrentTime();
	strcpy(req.connStr, Mtm->nodes[MtmNodeId-1].con.connStr);
	if (!MtmWriteSocket(sd, &req, sizeof req)) { 
		MTM_ELOG(WARNING, "Arbiter failed to send handshake message to %s:%d: %s", host, port, strerror(errno));
		pg_closesocket(sd, MtmUseRDMA);
		goto Retry;
	}
	if (MtmReadSocket(sd, &resp, sizeof resp) != sizeof(resp)) { 
		MTM_ELOG(WARNING, "Arbiter failed to receive response for handshake message from %s:%d: %s", host, port, strerror(errno));
		pg_closesocket(sd, MtmUseRDMA);
		goto Retry;
	}
	if (resp.code != MSG_STATUS || resp.dxid != HANDSHAKE_MAGIC) {
		MTM_ELOG(WARNING, "Arbiter get unexpected response %d for handshake message from %s:%d", resp.code, host, port);
		pg_closesocket(sd, MtmUseRDMA);
		goto Retry;
	}
	if (addrs)
		pg_freeaddrinfo_all(hint.ai_family, addrs);

	MtmLock(LW_EXCLUSIVE);
	MtmCheckResponse(&resp);
	MtmUnlock();

	MtmOnNodeConnect(node+1);

	busy_mask = save_mask;
	
	return sd;

Error:
	busy_mask = save_mask;
	if (sd >= 0) { 
		pg_closesocket(sd, MtmUseRDMA);
	}
	if (addrs) {
		pg_freeaddrinfo_all(hint.ai_family, addrs);
	}
	return -1;
}


static void MtmOpenConnections()
{
	int nNodes = MtmMaxNodes;
	int i;

	sockets = (int*)palloc(sizeof(int)*nNodes);

	for (i = 0; i < nNodes; i++) {
		sockets[i] = -1;
	}
	for (i = 0; i < nNodes; i++) {
		if (i+1 != MtmNodeId && i < Mtm->nAllNodes) { 
			sockets[i] = MtmConnectSocket(i, Mtm->nodes[i].con.arbiterPort);
			if (sockets[i] < 0) { 
				MtmOnNodeDisconnect(i+1);
			} 
		}
	}
	if (Mtm->nLiveNodes < Mtm->nAllNodes/2+1) { /* no quorum */
		MTM_ELOG(WARNING, "Node is out of quorum: only %d nodes of %d are accessible", Mtm->nLiveNodes, Mtm->nAllNodes);
		MtmSwitchClusterMode(MTM_IN_MINORITY);
	} else if (Mtm->status == MTM_INITIALIZATION) { 
		MtmSwitchClusterMode(MTM_CONNECTED);
	}
}


static bool MtmSendToNode(int node, void const* buf, int size)
{	
	bool result = true;
	nodemask_t save_mask = busy_mask;
	BIT_SET(busy_mask, node);
	while (true) {
#if 0
		/* Original intention was to reestablish connection when reconnect mask is set to avoid hanged-up connection.
		 * But reconnectMask is set not only when connection is broken, so breaking connection in all this cases cause avalanche of connection failures.
		 */
		if (sockets[node] >= 0 && BIT_CHECK(Mtm->reconnectMask, node)) {
			MTM_ELOG(WARNING, "Arbiter is forced to reconnect to node %d", node+1); 
			pg_closesocket(sockets[node], MtmUseRDMA);
			sockets[node] = -1;
		}
#endif
		if (BIT_CHECK(Mtm->reconnectMask, node)) {
			MtmLock(LW_EXCLUSIVE);		
			BIT_CLEAR(Mtm->reconnectMask, node);
			MtmUnlock();
		}
		if (sockets[node] < 0 || !MtmWriteSocket(sockets[node], buf, size)) {
			if (sockets[node] >= 0) { 
				MTM_ELOG(WARNING, "Arbiter fail to write to node %d: %s", node+1, strerror(errno));
				pg_closesocket(sockets[node], MtmUseRDMA);
				sockets[node] = -1;
			}
			sockets[node] = MtmConnectSocket(node, Mtm->nodes[node].con.arbiterPort);
			if (sockets[node] < 0) { 
				MtmOnNodeDisconnect(node+1);
				result = false;
				break;
			}
			MTM_LOG1("Arbiter reestablish connection with node %d", node+1);
		} else { 
			result = true;
			break;
		}
	}
	busy_mask = save_mask;
	return result;
}

static int MtmReadFromNode(int node, void* buf, int buf_size)
{
	int rc = MtmReadSocket(sockets[node], buf, buf_size);
	if (rc <= 0) { 
		MTM_ELOG(WARNING, "Arbiter failed to read from node=%d: %s", node+1, strerror(errno));
		MtmDisconnect(node);
	}
	return rc;
}

static void MtmAcceptOneConnection()
{
	int fd = pg_accept(gateway, NULL, NULL, MtmUseRDMA);
	if (fd < 0) {
		MTM_ELOG(WARNING, "Arbiter failed to accept socket: %s", strerror(errno));
	} else { 	
		MtmHandshakeMessage req;
		MtmArbiterMessage resp;		
		int rc = pg_fcntl(fd, F_SETFL, O_NONBLOCK, MtmUseRDMA);
		if (rc < 0) {
			MTM_ELOG(ERROR, "Arbiter failed to switch socket to non-blocking mode: %s", strerror(errno));
		}
		rc = MtmReadSocket(fd, &req, sizeof req);
		if (rc < sizeof(req)) { 
			MTM_ELOG(WARNING, "Arbiter failed to handshake socket: %d, errno=%d", rc, errno);
			pg_closesocket(fd, MtmUseRDMA);
		} else if (req.hdr.code != MSG_HANDSHAKE && req.hdr.dxid != HANDSHAKE_MAGIC) { 
			MTM_ELOG(WARNING, "Arbiter failed to handshake socket: %s", strerror(errno));
			pg_closesocket(fd, MtmUseRDMA);
		} else { 
			int node = req.hdr.node-1;
			Assert(node >= 0 && node < Mtm->nAllNodes && node+1 != MtmNodeId);

			MtmLock(LW_EXCLUSIVE);
			MtmCheckResponse(&req.hdr);
			MtmUnlock();

			MtmInitMessage(&resp, MSG_STATUS);
			resp.dxid = HANDSHAKE_MAGIC;
			resp.sxid = ShmemVariableCache->nextXid;
			resp.csn  = MtmGetCurrentTime();
			resp.node = MtmNodeId;
			MtmUpdateNodeConnectionInfo(&Mtm->nodes[node].con, req.connStr);
			if (!MtmWriteSocket(fd, &resp, sizeof resp)) { 
				MTM_ELOG(WARNING, "Arbiter failed to write response for handshake message to node %d", node+1);
				pg_closesocket(fd, MtmUseRDMA);
			} else { 
				MTM_LOG1("Arbiter established connection with node %d", node+1); 
				if (sockets[node] >= 0) { 
					MtmUnregisterSocket(sockets[node]);
				}
				sockets[node] = fd;
				MtmRegisterSocket(fd, node);
				MtmOnNodeConnect(node+1);
			}
		}
	}
}
	

static void MtmAcceptIncomingConnections()
{
	struct sockaddr_in sock_inet;
    int on = 1;
	int i;
	int nNodes = MtmMaxNodes;

	sockets = (int*)palloc(sizeof(int)*nNodes);
	for (i = 0; i < nNodes; i++) { 
		sockets[i] = -1;
	}
	sock_inet.sin_family = AF_INET;
	sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_inet.sin_port = htons(MtmArbiterPort);

    gateway = pg_socket(sock_inet.sin_family, SOCK_STREAM, 0, MtmUseRDMA);
	if (gateway < 0) {
		MTM_ELOG(ERROR, "Arbiter failed to create socket: %s", strerror(errno));
	}
    if (pg_setsockopt(gateway, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on, MtmUseRDMA) < 0) {
		MTM_ELOG(ERROR, "Arbiter failed to set options for socket: %s", strerror(errno));
	}			

    if (pg_bind(gateway, (struct sockaddr*)&sock_inet, sizeof(sock_inet), MtmUseRDMA) < 0) {
		MTM_ELOG(ERROR, "Arbiter failed to bind socket: %s", strerror(errno));
	}	
    if (pg_listen(gateway, nNodes, MtmUseRDMA) < 0) {
		MTM_ELOG(ERROR, "Arbiter failed to listen socket: %s", strerror(errno));
	}	

	sockets[MtmNodeId-1] = gateway;
	MtmRegisterSocket(gateway, MtmNodeId-1);
}


static void MtmAppendBuffer(MtmBuffer* txBuffer, MtmArbiterMessage* msg)
{
	MtmBuffer* buf = &txBuffer[msg->node-1];
	if (buf->used == buf->size) {
		if (buf->size == 0) { 
			buf->size = INIT_BUFFER_SIZE;
			buf->data = palloc(buf->size * sizeof(MtmArbiterMessage));
		} else { 
			buf->size *= 2;
			buf->data = repalloc(buf->data, buf->size * sizeof(MtmArbiterMessage));
		}
	}
	msg->node = MtmNodeId;
	buf->data[buf->used++] = *msg;
}


static void MtmSender(Datum arg)
{
	int nNodes = MtmMaxNodes;
	int i;

	MtmBuffer* txBuffer = (MtmBuffer*)palloc0(sizeof(MtmBuffer)*nNodes);
	MTM_ELOG(LOG, "Start arbiter sender %d", MyProcPid);
	InitializeTimeouts();

	pqsignal(SIGINT, SetStop);
	pqsignal(SIGQUIT, SetStop);
	pqsignal(SIGTERM, SetStop);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to a database */
	BackgroundWorkerInitializeConnection(MtmDatabaseName, NULL);

	/* Start heartbeat times */
	heartbeat_timer = RegisterTimeout(USER_TIMEOUT, MtmScheduleHeartbeat);
	enable_timeout_after(heartbeat_timer, MtmHeartbeatSendTimeout);

	MtmOpenConnections();

	while (!stop) {
		MtmMessageQueue *curr, *next;		
		PGSemaphoreLock(&Mtm->sendSemaphore);
		CHECK_FOR_INTERRUPTS();

		MtmCheckHeartbeat();
		/* 
		 * Use shared lock to improve locality,
		 * because all other process modifying this list are using exclusive lock 
		 */
		SpinLockAcquire(&Mtm->queueSpinlock);

		for (curr = Mtm->sendQueue; curr != NULL; curr = next) {
			next = curr->next;
			MtmAppendBuffer(txBuffer, &curr->msg);
			curr->next = Mtm->freeQueue;
			Mtm->freeQueue = curr;
		}
		Mtm->sendQueue = NULL;

		SpinLockRelease(&Mtm->queueSpinlock);

		for (i = 0; i < Mtm->nAllNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				MtmSendToNode(i, txBuffer[i].data, txBuffer[i].used*sizeof(MtmArbiterMessage));
				txBuffer[i].used = 0;
			}
		}		
		CHECK_FOR_INTERRUPTS();
		MtmCheckHeartbeat();
	}
	MTM_ELOG(LOG, "Stop arbiter sender %d", MyProcPid);
	proc_exit(1); /* force restart of this bgwroker */
}


#if !USE_EPOLL
static bool MtmRecovery()
{
	int nNodes = Mtm->nAllNodes;
	bool recovered = false;
    int i;

    for (i = 0; i < nNodes; i++) {
		int sd = sockets[i];
        if (sd >= 0 && FD_ISSET(sd, &inset)) {
            struct timeval tm = {0,0};
            fd_set tryset;
            FD_ZERO(&tryset);
            FD_SET(sd, &tryset);
            if (pg_select(sd+1, &tryset, NULL, NULL, &tm, MtmUseRDMA) < 0) {
				MTM_ELOG(WARNING, "Arbiter lost connection with node %d", i+1);
				MtmDisconnect(i);
				recovered = true;
            }
        }
    }
	return recovered;
}
#endif

static void MtmMonitor(Datum arg)
{
	pqsignal(SIGINT, SetStop);
	pqsignal(SIGQUIT, SetStop);
	pqsignal(SIGTERM, SetStop);
	
	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to a database */
	BackgroundWorkerInitializeConnection(MtmDatabaseName, NULL);

	while (!stop) {
		int rc = WaitLatch(&MyProc->procLatch, WL_TIMEOUT | WL_POSTMASTER_DEATH, MtmHeartbeatRecvTimeout);
		if (rc & WL_POSTMASTER_DEATH) { 
			break;
		}
		MtmRefreshClusterStatus();
	}
}

static void MtmReceiver(Datum arg)
{
	int nNodes = MtmMaxNodes;
	int nResponses;
	int i, j, n, rc;
	MtmBuffer* rxBuffer = (MtmBuffer*)palloc0(sizeof(MtmBuffer)*nNodes);
	timestamp_t lastHeartbeatCheck = MtmGetSystemTime();
	timestamp_t now;
	timestamp_t selectTimeout = MtmHeartbeatRecvTimeout;

#if USE_EPOLL
	struct epoll_event* events = (struct epoll_event*)palloc(sizeof(struct epoll_event)*nNodes);
    epollfd = epoll_create(nNodes);
#else
    FD_ZERO(&inset);
    max_fd = 0;
#endif

	pqsignal(SIGINT, SetStop);
	pqsignal(SIGQUIT, SetStop);
	pqsignal(SIGTERM, SetStop);
	
	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to a database */
	BackgroundWorkerInitializeConnection(MtmDatabaseName, NULL);

	MtmAcceptIncomingConnections();

	for (i = 0; i < nNodes; i++) { 
		rxBuffer[i].size = INIT_BUFFER_SIZE;
		rxBuffer[i].data = palloc(INIT_BUFFER_SIZE*sizeof(MtmArbiterMessage));
	}

	while (!stop) {
#if USE_EPOLL
        n = epoll_wait(epollfd, events, nNodes, selectTimeout);
		if (n < 0) { 
			if (errno == EINTR) { 
				continue;
			}
			MTM_ELOG(ERROR, "Arbiter failed to poll sockets: %s", strerror(errno));
		}
		for (j = 0; j < n; j++) {
			i = events[j].data.u32;
			if (events[j].events & EPOLLERR) {
				MTM_ELOG(WARNING, "Arbiter lost connection with node %d", i+1);
				MtmDisconnect(i);
			} 
		}
		for (j = 0; j < n; j++) {
			if (events[j].events & EPOLLIN)  
#else
        fd_set events;
		do { 
			struct timeval tv;
			events = inset;
			tv.tv_sec = selectTimeout/1000;
			tv.tv_usec = selectTimeout%1000*1000;
			do { 
				n = pg_select(max_fd+1, &events, NULL, NULL, &tv, MtmUseRDMA);
			} while (n < 0 && errno == EINTR);
		} while (n < 0 && MtmRecovery());
		
		if (n < 0) {
			MTM_ELOG(ERROR, "Arbiter failed to select sockets: %s", strerror(errno));
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
				
				rc = MtmReadFromNode(i, (char*)rxBuffer[i].data + rxBuffer[i].used, rxBuffer[i].size-rxBuffer[i].used);
				if (rc <= 0) {
					MTM_LOG1("Failed to read response from node %d", i+1);
					continue;
				}

				rxBuffer[i].used += rc;
				nResponses = rxBuffer[i].used/sizeof(MtmArbiterMessage);

				
				MtmLock(LW_EXCLUSIVE);						

				for (j = 0; j < nResponses; j++) { 
					MtmArbiterMessage* msg = &rxBuffer[i].data[j];
					MtmTransState* ts;
					MtmTransMap* tm;
					int node = msg->node;

					Assert(node > 0 && node <= nNodes && node != MtmNodeId);

					if (Mtm->nodes[node-1].connectivityMask != msg->connectivityMask) { 
						MTM_ELOG(LOG, "Node %d changes it connectivity mask from %llx to %llx", node, Mtm->nodes[node-1].connectivityMask, msg->connectivityMask);
					}

					Mtm->nodes[node-1].oldestSnapshot = msg->oldestSnapshot;
					Mtm->nodes[node-1].disabledNodeMask = msg->disabledNodeMask;
					Mtm->nodes[node-1].connectivityMask = msg->connectivityMask;
					Mtm->nodes[node-1].lastHeartbeat = MtmGetSystemTime();

					MtmCheckResponse(msg);
					MTM_LOG2("Receive response %s for transaction %s from node %d", MtmMessageKindMnem[msg->code], msg->gid, node);

					switch (msg->code) {
					  case MSG_HEARTBEAT:
						MTM_LOG4("Receive HEARTBEAT from node %d with timestamp %lld delay %lld", 
								 node, msg->csn, USEC_TO_MSEC(MtmGetSystemTime() - msg->csn)); 
						Mtm->nodes[node-1].nHeartbeats += 1;
						continue;						
					  case MSG_POLL_REQUEST:
						Assert(*msg->gid);
						tm = (MtmTransMap*)hash_search(MtmGid2State, msg->gid, HASH_FIND, NULL);
						if (tm == NULL || tm->state == NULL) { 
							MTM_ELOG(WARNING, "Request for unexisted transaction %s from node %d", msg->gid, node);
							msg->status = TRANSACTION_STATUS_ABORTED;
						} else {
							msg->status = tm->state->status;
							msg->csn = tm->state->csn;
							MTM_LOG1("Send response %s for transaction %s to node %d", MtmTxnStatusMnem[msg->status], msg->gid, node);
						}
						MtmInitMessage(msg, MSG_POLL_STATUS);
						MtmSendMessage(msg);
						continue;
					  case MSG_POLL_STATUS:
						Assert(*msg->gid);
						tm = (MtmTransMap*)hash_search(MtmGid2State, msg->gid, HASH_FIND, NULL);
						if (tm == NULL || tm->state == NULL) { 
							MTM_ELOG(WARNING, "Response for non-existing transaction %s from node %d", msg->gid, node);
						} else {
							ts = tm->state;
							BIT_SET(ts->votedMask, node-1);
							if (ts->status == TRANSACTION_STATUS_UNKNOWN || ts->status == TRANSACTION_STATUS_IN_PROGRESS) { 
								if (msg->status == TRANSACTION_STATUS_IN_PROGRESS || msg->status == TRANSACTION_STATUS_ABORTED) {
									MTM_ELOG(LOG, "Abort prepared transaction %s because it is in state %s at node %d",
										 msg->gid, MtmTxnStatusMnem[msg->status], node);

									replorigin_session_origin = DoNotReplicateId;
									MtmFinishPreparedTransaction(ts, false);
									replorigin_session_origin = InvalidRepOriginId;
								} 
								else if (msg->status == TRANSACTION_STATUS_COMMITTED || msg->status == TRANSACTION_STATUS_UNKNOWN)
								{ 
									if (msg->csn > ts->csn) {
										ts->csn = msg->csn;
										MtmSyncClock(ts->csn);
									}
									if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
										MTM_ELOG(LOG, "Commit transaction %s because it is prepared at all live nodes", msg->gid);		

										replorigin_session_origin = DoNotReplicateId;
										MtmFinishPreparedTransaction(ts, true);
										replorigin_session_origin = InvalidRepOriginId;
									} else { 
										MTM_LOG1("Receive response for transaction %s -> %s, participants=%llx, voted=%llx", 
												 msg->gid, MtmTxnStatusMnem[msg->status], ts->participantsMask, ts->votedMask);		
									}
								} else {
									MTM_ELOG(LOG, "Receive response %s for transaction %s for node %d, votedMask %llx, participantsMask %llx",
										 MtmTxnStatusMnem[msg->status], msg->gid, node, ts->votedMask, ts->participantsMask & ~Mtm->disabledNodeMask);
									continue;
								}
							} else if (ts->status == TRANSACTION_STATUS_ABORTED && msg->status == TRANSACTION_STATUS_COMMITTED) {
								MTM_ELOG(WARNING, "Transaction %s is aborted at node %d but committed at node %d", msg->gid, MtmNodeId, node);
							} else if (msg->status == TRANSACTION_STATUS_ABORTED && ts->status == TRANSACTION_STATUS_COMMITTED) {
								MTM_ELOG(WARNING, "Transaction %s is committed at node %d but aborted at node %d", msg->gid, MtmNodeId, node);
							} else { 
								MTM_ELOG(LOG, "Receive response %s for transaction %s status %s for node %d, votedMask %llx, participantsMask %llx",
									 MtmTxnStatusMnem[msg->status], msg->gid, MtmTxnStatusMnem[ts->status], node, ts->votedMask, ts->participantsMask & ~Mtm->disabledNodeMask);
							}
						}
						continue;
					  default:
						break;
					}
					if (BIT_CHECK(msg->disabledNodeMask, node-1)) {
						MTM_ELOG(WARNING, "Ignore message from dead node %d\n", node);
						continue;
					}
					ts = (MtmTransState*)hash_search(MtmXid2State, &msg->dxid, HASH_FIND, NULL);
					if (ts == NULL) { 
						MTM_ELOG(WARNING, "Ignore response for non-existing transaction %llu from node %d", (long64)msg->dxid, node);
						continue;
					}
					Assert(msg->code == MSG_ABORTED || strcmp(msg->gid, ts->gid) == 0);
					if (BIT_CHECK(ts->votedMask, node-1)) {
						MTM_ELOG(WARNING, "Receive deteriorated %s response for transaction %s (%llu) from node %d",
							 MtmMessageKindMnem[msg->code], ts->gid, (long64)ts->xid, node);
						continue;
					}
					BIT_SET(ts->votedMask, node-1);

					if (MtmIsCoordinator(ts)) {
						switch (msg->code) { 
						  case MSG_PREPARED:
							MTM_TXTRACE(ts, "MtmTransReceiver got MSG_PREPARED");
							if (ts->status == TRANSACTION_STATUS_COMMITTED) { 
								MTM_ELOG(WARNING, "Receive PREPARED response for already committed transaction %llu from node %d",
									 (long64)ts->xid, node);
								continue;
							}
							Mtm->nodes[node-1].transDelay += MtmGetCurrentTime() - ts->csn;
							ts->xids[node-1] = msg->sxid;
							
#if 0
							/* This code seems to be deteriorated because now checking that distributed transaction involves all live nodes is done at replica while applying PREPARE */
							if ((~msg->disabledNodeMask & Mtm->disabledNodeMask) != 0) { 
								/* Coordinator's disabled mask is wider than of this node: so reject such transaction to avoid 
								   commit on smaller subset of nodes */
								MTM_ELOG(WARNING, "Coordinator of distributed transaction %s (%llu) see less nodes than node %d: %llx instead of %llx",
									 ts->gid, (long64)ts->xid, node, Mtm->disabledNodeMask, msg->disabledNodeMask);
								MtmAbortTransaction(ts);
							}
#endif
							if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
								/* All nodes are finished their transactions */
								if (ts->status == TRANSACTION_STATUS_ABORTED) { 
									MtmWakeUpBackend(ts);								
								} else { 
									Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
									MTM_LOG2("Transaction %s is prepared (status=%s participants=%llx disabled=%llx, voted=%llx)", 
											 ts->gid, MtmTxnStatusMnem[ts->status], ts->participantsMask, Mtm->disabledNodeMask, ts->votedMask);
									ts->isPrepared = true;
									if (ts->isTwoPhase) { 
										MtmWakeUpBackend(ts);										
									} else if (MtmUseDtm) { 
										ts->votedMask = 0;
										MTM_TXTRACE(ts, "MtmTransReceiver send MSG_PRECOMMIT");
										Assert(replorigin_session_origin == InvalidRepOriginId);
										MTM_LOG2("SetPreparedTransactionState for %s", ts->gid);
										MtmUnlock();
										MtmResetTransaction();
										StartTransactionCommand();
										SetPreparedTransactionState(ts->gid, MULTIMASTER_PRECOMMITTED);	
										CommitTransactionCommand();
										Assert(!MtmTransIsActive());
										MtmLock(LW_EXCLUSIVE);						
									} else { 
										ts->status = TRANSACTION_STATUS_UNKNOWN;
										MtmWakeUpBackend(ts);
									}
								}
							}
							break;						   
						  case MSG_ABORTED:
							if (ts->status == TRANSACTION_STATUS_COMMITTED) { 
								MTM_ELOG(WARNING, "Receive ABORTED response for already committed transaction %s (%llu) from node %d",
									 ts->gid, (long64)ts->xid, node);
								continue;
							}
							if (ts->status != TRANSACTION_STATUS_ABORTED) { 
								MTM_LOG1("Arbiter receive abort message for transaction %s (%llu) from node %d", ts->gid, (long64)ts->xid, node);
								Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
								MtmAbortTransaction(ts);
							}
							if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
								MtmWakeUpBackend(ts);
							}
							break;
						  case MSG_PRECOMMITTED:
							MTM_TXTRACE(ts, "MtmTransReceiver got MSG_PRECOMMITTED");
                            if (ts->status == TRANSACTION_STATUS_COMMITTED) {
                                MTM_ELOG(WARNING, "Receive PRECOMMITTED response for already committed transaction %s (%llu) from node %d",
                                     ts->gid, (long64)ts->xid, node);
                                continue;
                            }
							if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) {
								if (msg->csn > ts->csn) {
									ts->csn = msg->csn;
									MtmSyncClock(ts->csn);
								}
								if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
									ts->csn = MtmAssignCSN();
									ts->status = TRANSACTION_STATUS_UNKNOWN;
									MtmWakeUpBackend(ts);
								}
							} else { 
								Assert(ts->status == TRANSACTION_STATUS_ABORTED);
								MTM_ELOG(WARNING, "Receive PRECOMMITTED response for aborted transaction %s (%llu) from node %d", 
										 ts->gid, (long64)ts->xid, node); 
								if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
									MtmWakeUpBackend(ts);
								}
							}	
							break;
						  default:
							Assert(false);
						} 
					} else { 
						Assert(false); /* All broadcasts are now sent through pglogical */
					}
				}
				MtmUnlock();
				
				rxBuffer[i].used -= nResponses*sizeof(MtmArbiterMessage);
				if (rxBuffer[i].used != 0) { 
					memmove(rxBuffer[i].data, (char*)rxBuffer[i].data + nResponses*sizeof(MtmArbiterMessage), rxBuffer[i].used);
				}
			}
		}
		if (Mtm->status == MTM_ONLINE) { 
			now = MtmGetSystemTime();
			/* Check for heartbeats only in case of timeout expiration: it means that we do not have non-processed events.
			 * It helps to avoid false node failure detection because of blocking receiver.
			 */
			if (n == 0) {
				selectTimeout = MtmHeartbeatRecvTimeout; /* restore select timeout */ 
				if (now > lastHeartbeatCheck + MSEC_TO_USEC(MtmHeartbeatRecvTimeout)) { 
					if (!MtmWatchdog(now)) { 
						for (i = 0; i < nNodes; i++) { 
							if (Mtm->nodes[i].lastHeartbeat != 0 && sockets[i] >= 0) {
								MTM_LOG1("Last heartbeat from node %d received %lld microseconds ago", i+1, now - Mtm->nodes[i].lastHeartbeat);
							}
						}
					}
					lastHeartbeatCheck = now;
				}
			} else {
				if (now > lastHeartbeatCheck + MSEC_TO_USEC(MtmHeartbeatRecvTimeout)) { 
					/* Switch to non-blocking mode to proceed all pending requests before doing watchdog check */
					selectTimeout = 0;
				}
			}
		} else if (n == 0) { 
			selectTimeout = MtmHeartbeatRecvTimeout; /* restore select timeout */ 
		}
	}
	proc_exit(1); /* force restart of this bgwroker */
}

