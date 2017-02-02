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
static bool MtmSendToNode(int node, void const* buf, int size, time_t reconnectTimeout);

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
	elog(LOG, "Register background workers");
	RegisterBackgroundWorker(&MtmSenderWorker);
	RegisterBackgroundWorker(&MtmRecevierWorker);
	RegisterBackgroundWorker(&MtmMonitorWorker);
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
        elog(LOG, "Arbiter failed to add socket to epoll set: %d", errno);
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
		elog(LOG, "Arbiter failed to unregister socket from epoll set: %d", errno);
    } 
#else
	FD_CLR(fd, &inset); 
#endif
}


static void MtmDisconnect(int node)
{
	MtmUnregisterSocket(sockets[node]);
	close(sockets[node]);
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
	} while ((rc = select(sd+1, forWrite ? NULL : &set, forWrite ? &set : NULL, NULL, &tv)) < 0 && errno == EINTR);

	return rc;
}

static bool MtmWriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) {
		int rc = MtmWaitSocket(sd, true, MtmHeartbeatSendTimeout);
		if (rc == 1) { 
			while ((rc = send(sd, src, size, 0)) < 0 && errno == EINTR);			
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
	while ((rc = recv(sd, buf, buf_size, 0)) < 0 && errno == EINTR);			
	if (rc <= 0 && (errno == EAGAIN || errno == EINPROGRESS)) { 
		rc = MtmWaitSocket(sd, false, MtmHeartbeatSendTimeout);
		if (rc == 1) { 
			while ((rc = recv(sd, buf, buf_size, 0)) < 0 && errno == EINTR);			
		}
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

static void MtmCheckResponse(MtmArbiterMessage* resp)
{
	if (resp->lockReq) {
		BIT_SET(Mtm->globalLockerMask, resp->node-1);
	} else { 
		BIT_CLEAR(Mtm->globalLockerMask, resp->node-1);
	}
	if (BIT_CHECK(resp->disabledNodeMask, MtmNodeId-1) 
		&& !BIT_CHECK(Mtm->disabledNodeMask, resp->node-1)
		&& Mtm->status != MTM_RECOVERY
		&& Mtm->status != MTM_RECOVERED
		&& Mtm->nodes[MtmNodeId-1].lastStatusChangeTime + MSEC_TO_USEC(MtmNodeDisableDelay) < MtmGetSystemTime()) 
	{ 
		elog(WARNING, "Node %d thinks that I am dead, while I am %s (message %s)", resp->node, MtmNodeStatusMnem[Mtm->status], MtmMessageKindMnem[resp->code]);
		BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
		Mtm->nConfigChanges += 1;
		MtmSwitchClusterMode(MTM_RECOVERY);
	} else if (BIT_CHECK(Mtm->disabledNodeMask, resp->node-1) && sockets[resp->node-1] < 0) { 
		/* We receive heartbeat from disabled node.
		 * Looks like it is restarted.
		 * Try to reconnect to it.
		 */
		elog(WARNING, "Receive heartbeat from disabled node %d", resp->node);		
		BIT_SET(Mtm->reconnectMask, resp->node-1);
	}	
}

static void MtmScheduleHeartbeat()
{
//	Assert(!last_sent_heartbeat || last_sent_heartbeat + MSEC_TO_USEC(MtmHeartbeatRecvTimeout) >= MtmGetSystemTime());
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
	msg.code = MSG_HEARTBEAT;
	msg.disabledNodeMask = Mtm->disabledNodeMask;
	msg.connectivityMask = SELF_CONNECTIVITY_MASK;
	msg.oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
	msg.lockReq = Mtm->nodeLockerMask != 0;
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
				if (!MtmSendToNode(i, &msg, sizeof(msg), MtmHeartbeatRecvTimeout)) { 
					elog(LOG, "Arbiter failed to send heartbeat to node %d", i+1);
				} else {
					if (last_heartbeat_to_node[i] + MSEC_TO_USEC(MtmHeartbeatSendTimeout)*2 < now) { 
						MTM_LOG1("Last heartbeat to node %d was sent %lld microseconds ago", i+1, now - last_heartbeat_to_node[i]);
					}
					last_heartbeat_to_node[i] = now;
					/* Connectivity mask can be cleared by MtmWatchdog: in this case sockets[i] >= 0 */
					if (BIT_CHECK(SELF_CONNECTIVITY_MASK, i)) { 
						MTM_LOG1("Force reconnect to node %d", i+1);    
						close(sockets[i]);
						sockets[i] = -1;
						MtmReconnectNode(i+1); /* set reconnect mask to force node reconnent */
						//MtmOnNodeConnect(i+1);
					}
					MTM_LOG4("Send heartbeat to node %d with timestamp %lld", i+1, now);    
				}
			} else { 
				MTM_LOG2("Do not send heartbeat to node %d, busy mask %lld, status %s", i+1, busy_mask, MtmNodeStatusMnem[Mtm->status]);
			}
		}
	}
	
}

/* This function shoudl be called from all places where sender can be blocked.
 * It checks send_heartbeat flag set by timer and if it is set hthen sends heartbeats to all alive nodes 
 */
void MtmCheckHeartbeat()
{
	if (send_heartbeat && !stop) {
		send_heartbeat = false;
		enable_timeout_after(heartbeat_timer, MtmHeartbeatSendTimeout);
		MtmSendHeartbeat();
	}			
}


static int MtmConnectSocket(int node, int port, time_t timeout)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[MAX_ROUTES];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
	MtmHandshakeMessage req;
	MtmArbiterMessage   resp;
	int sd;
	timestamp_t start = MtmGetSystemTime();
	char const* host = Mtm->nodes[node].con.hostName;
	nodemask_t save_mask = busy_mask;
	timestamp_t afterWait;
	timestamp_t beforeWait;

    sock_inet.sin_family = AF_INET;
	sock_inet.sin_port = htons(port);

	if (!MtmResolveHostByName(host, addrs, &n_addrs)) {
		elog(LOG, "Arbiter failed to resolve host '%s' by name", host);
		return -1;
	}
	BIT_SET(busy_mask, node);
	
  Retry:
    while (1) {
		int rc = -1;
		sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd < 0) {
			elog(LOG, "Arbiter failed to create socket: %d", errno);
			busy_mask = save_mask;
			return -1;
		}
		rc = fcntl(sd, F_SETFL, O_NONBLOCK);
		if (rc < 0) {
			elog(LOG, "Arbiter failed to switch socket to non-blocking mode: %d", errno);
			busy_mask = save_mask;
			return -1;
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
		if (rc == 0) {
			break;
		}
		beforeWait = MtmGetSystemTime();
		if (errno != EINPROGRESS || start + MSEC_TO_USEC(timeout) < beforeWait ) {
			elog(WARNING, "Arbiter failed to connect to %s:%d: error=%d", host, port, errno);
			close(sd);
			busy_mask = save_mask;
			return -1;
		} else {
			rc = MtmWaitSocket(sd, true, MtmHeartbeatSendTimeout);
			if (rc == 1) {
				socklen_t optlen = sizeof(int); 
				if (getsockopt(sd, SOL_SOCKET, SO_ERROR, (void*)&rc, &optlen) < 0) { 
					elog(WARNING, "Arbiter failed to getsockopt for %s:%d: error=%d", host, port, errno);
					close(sd);
					busy_mask = save_mask;
					return -1;
				}
				if (rc == 0) { 
					break;
				} else { 
					elog(WARNING, "Arbiter trying to connect to %s:%d: rc=%d, error=%d", host, port, rc, errno);
				}
			} else { 
				elog(WARNING, "Arbiter waiting socket to %s:%d: rc=%d, error=%d", host, port, rc, errno);
			}
			close(sd);
			afterWait = MtmGetSystemTime();
			if (afterWait < beforeWait + MSEC_TO_USEC(MtmHeartbeatSendTimeout)) {
				MtmSleep(beforeWait + MSEC_TO_USEC(MtmHeartbeatSendTimeout) - afterWait);
			}
		}
	}
	MtmSetSocketOptions(sd);
	req.hdr.code = MSG_HANDSHAKE;
	req.hdr.node = MtmNodeId;
	req.hdr.dxid = HANDSHAKE_MAGIC;
	req.hdr.sxid = ShmemVariableCache->nextXid;
	req.hdr.csn  = MtmGetCurrentTime();
	req.hdr.disabledNodeMask = Mtm->disabledNodeMask;
	req.hdr.connectivityMask = SELF_CONNECTIVITY_MASK;
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
	
	MtmLock(LW_EXCLUSIVE);
	MtmCheckResponse(&resp);
	MtmUnlock();

	MtmOnNodeConnect(node+1);

	busy_mask = save_mask;

	return sd;
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
			sockets[i] = MtmConnectSocket(i, Mtm->nodes[i].con.arbiterPort, MtmConnectTimeout);
			if (sockets[i] < 0) { 
				MtmOnNodeDisconnect(i+1);
			} 
		}
	}
	if (Mtm->nLiveNodes < Mtm->nAllNodes/2+1) { /* no quorum */
		elog(WARNING, "Node is out of quorum: only %d nodes of %d are accessible", Mtm->nLiveNodes, Mtm->nAllNodes);
		MtmSwitchClusterMode(MTM_IN_MINORITY);
	} else if (Mtm->status == MTM_INITIALIZATION) { 
		MtmSwitchClusterMode(MTM_CONNECTED);
	}
}


static bool MtmSendToNode(int node, void const* buf, int size, time_t reconnectTimeout)
{	
	bool result = true;
	nodemask_t save_mask = busy_mask;
	BIT_SET(busy_mask, node);
	while (true) {
#if 0
		/* Original intention was to reestablish connectect when reconnet mask is set to avoid hanged-up connection.
		 * But reconnectMask is set not only when connection is broken, so breaking connection in all this cases cause avalunch of connection failures.
		 */
		if (sockets[node] >= 0 && BIT_CHECK(Mtm->reconnectMask, node)) {
			elog(WARNING, "Arbiter is forced to reconnect to node %d", node+1); 
			close(sockets[node]);
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
				elog(WARNING, "Arbiter fail to write to node %d: %d", node+1, errno);
				close(sockets[node]);
				sockets[node] = -1;
			}
			sockets[node] = MtmConnectSocket(node, Mtm->nodes[node].con.arbiterPort, reconnectTimeout);
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
		int rc = fcntl(fd, F_SETFL, O_NONBLOCK);
		if (rc < 0) {
			elog(ERROR, "Arbiter failed to switch socket to non-blocking mode: %d", errno);
		}
		rc = MtmReadSocket(fd, &req, sizeof req);
		if (rc < sizeof(req)) { 
			elog(WARNING, "Arbiter failed to handshake socket: %d, errno=%d", rc, errno);
		} else if (req.hdr.code != MSG_HANDSHAKE && req.hdr.dxid != HANDSHAKE_MAGIC) { 
			elog(WARNING, "Arbiter get unexpected handshake message %d", req.hdr.code);
			close(fd);
		} else { 
			int node = req.hdr.node-1;
			Assert(node >= 0 && node < Mtm->nAllNodes && node+1 != MtmNodeId);

			MtmLock(LW_EXCLUSIVE);
			MtmCheckResponse(&req.hdr);
			MtmUnlock();

			resp.code = MSG_STATUS;
			resp.disabledNodeMask = Mtm->disabledNodeMask;
			resp.connectivityMask = SELF_CONNECTIVITY_MASK;
			resp.dxid = HANDSHAKE_MAGIC;
			resp.sxid = ShmemVariableCache->nextXid;
			resp.csn  = MtmGetCurrentTime();
			resp.node = MtmNodeId;
			MtmUpdateNodeConnectionInfo(&Mtm->nodes[node].con, req.connStr);
			if (!MtmWriteSocket(fd, &resp, sizeof resp)) { 
				elog(WARNING, "Arbiter failed to write response for handshake message to node %d", node+1);
				close(fd);
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

    gateway = socket(sock_inet.sin_family, SOCK_STREAM, 0);
	if (gateway < 0) {
		elog(ERROR, "Arbiter failed to create socket: %d", errno);
	}
    setsockopt(gateway, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    if (bind(gateway, (struct sockaddr*)&sock_inet, sizeof(sock_inet)) < 0) {
		elog(ERROR, "Arbiter failed to bind socket: %d", errno);
	}	
    if (listen(gateway, nNodes) < 0) {
		elog(ERROR, "Arbiter failed to listen socket: %d", errno);
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
	sigset_t sset;
	int nNodes = MtmMaxNodes;
	int i;

	MtmBuffer* txBuffer = (MtmBuffer*)palloc0(sizeof(MtmBuffer)*nNodes);
	elog(LOG, "Start arbiter sender %d", MyProcPid);
	InitializeTimeouts();

	signal(SIGINT, SetStop);
	signal(SIGQUIT, SetStop);
	signal(SIGTERM, SetStop);
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);

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
				MtmSendToNode(i, txBuffer[i].data, txBuffer[i].used*sizeof(MtmArbiterMessage), MtmReconnectTimeout);
				txBuffer[i].used = 0;
			}
		}		
		CHECK_FOR_INTERRUPTS();
		MtmCheckHeartbeat();
	}
	elog(LOG, "Stop arbiter sender %d", MyProcPid);
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

static void MtmMonitor(Datum arg)
{
	sigset_t sset;

	signal(SIGINT, SetStop);
	signal(SIGQUIT, SetStop);
	signal(SIGTERM, SetStop);
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);
	
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
	sigset_t sset;
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

	signal(SIGINT, SetStop);
	signal(SIGQUIT, SetStop);
	signal(SIGTERM, SetStop);
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);
	
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
			elog(ERROR, "Arbiter failed to poll sockets: %d", errno);
		}
		for (j = 0; j < n; j++) {
			i = events[j].data.u32;
			if (events[j].events & EPOLLERR) {
				elog(WARNING, "Arbiter lost connection with node %d", i+1);
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
						elog(LOG, "Node %d changes it connectivity mask from %llx to %llx", node, Mtm->nodes[node-1].connectivityMask, msg->connectivityMask);
					}

					Mtm->nodes[node-1].oldestSnapshot = msg->oldestSnapshot;
					Mtm->nodes[node-1].disabledNodeMask = msg->disabledNodeMask;
					Mtm->nodes[node-1].connectivityMask = msg->connectivityMask;
					Mtm->nodes[node-1].lastHeartbeat = MtmGetSystemTime();

					MtmCheckResponse(msg);
					MTM_LOG2("Receive response %s for transaction %s from node %d", MtmMessageKindMnem[msg->code], msg->gid, msg->node);

					switch (msg->code) {
					  case MSG_HEARTBEAT:
						MTM_LOG4("Receive HEARTBEAT from node %d with timestamp %lld delay %lld", 
								 node, msg->csn, USEC_TO_MSEC(MtmGetSystemTime() - msg->csn)); 
						continue;
					  case MSG_POLL_REQUEST:
						Assert(*msg->gid);
						tm = (MtmTransMap*)hash_search(MtmGid2State, msg->gid, HASH_FIND, NULL);
						if (tm == NULL || tm->state == NULL) { 
							elog(WARNING, "Request for unexisted transaction %s from node %d", msg->gid, node);
							msg->status = TRANSACTION_STATUS_ABORTED;
						} else {
							msg->status = tm->state->status;
							msg->csn = tm->state->csn;
							MTM_LOG1("Send response %s for transaction %s to node %d", MtmTxnStatusMnem[msg->status], msg->gid, msg->node);
						}
						msg->disabledNodeMask = Mtm->disabledNodeMask;
						msg->connectivityMask = SELF_CONNECTIVITY_MASK;
						msg->oldestSnapshot = Mtm->nodes[MtmNodeId-1].oldestSnapshot;
						msg->code = MSG_POLL_STATUS;	
						MtmSendMessage(msg);
						continue;
					  case MSG_POLL_STATUS:
						Assert(*msg->gid);
						tm = (MtmTransMap*)hash_search(MtmGid2State, msg->gid, HASH_FIND, NULL);
						if (tm == NULL || tm->state == NULL) { 
							elog(WARNING, "Response for unexisted transaction %s from node %d", msg->gid, node);
						} else {
							ts = tm->state;
							BIT_SET(ts->votedMask, node-1);
							if (ts->status == TRANSACTION_STATUS_UNKNOWN || ts->status == TRANSACTION_STATUS_IN_PROGRESS) { 
								if (msg->status == TRANSACTION_STATUS_IN_PROGRESS || msg->status == TRANSACTION_STATUS_ABORTED) {
									elog(LOG, "Abort prepared transaction %s because it is in state %s at node %d",
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
										elog(LOG, "Commit transaction %s because it is prepared at all live nodes", msg->gid);		

										replorigin_session_origin = DoNotReplicateId;
										MtmFinishPreparedTransaction(ts, true);
										replorigin_session_origin = InvalidRepOriginId;
									} else { 
										MTM_LOG1("Receive response for transaction %s -> %s, participants=%llx, voted=%llx", 
												 msg->gid, MtmTxnStatusMnem[msg->status], ts->participantsMask, ts->votedMask);		
									}
								} else {
									elog(LOG, "Receive response %s for transaction %s for node %d, votedMask %llx, participantsMask %llx",
										 MtmTxnStatusMnem[msg->status], msg->gid, node, ts->votedMask, ts->participantsMask & ~Mtm->disabledNodeMask);
									continue;
								}
							} else if (ts->status == TRANSACTION_STATUS_ABORTED && msg->status == TRANSACTION_STATUS_COMMITTED) {
								elog(WARNING, "Transaction %s is aborted at node %d but committed at node %d", msg->gid, MtmNodeId, node);
							} else if (msg->status == TRANSACTION_STATUS_ABORTED && ts->status == TRANSACTION_STATUS_COMMITTED) {
								elog(WARNING, "Transaction %s is committed at node %d but aborted at node %d", msg->gid, MtmNodeId, node);
							} else { 
								elog(LOG, "Receive response %s for transaction %s status %s for node %d, votedMask %llx, participantsMask %llx",
									 MtmTxnStatusMnem[msg->status], msg->gid, MtmTxnStatusMnem[ts->status], node, ts->votedMask, ts->participantsMask & ~Mtm->disabledNodeMask);
							}
						}
						continue;
					  default:
						break;
					}
					if (BIT_CHECK(msg->disabledNodeMask, node-1)) {
						elog(WARNING, "Ignore message from dead node %d\n", node);
						continue;
					}
					ts = (MtmTransState*)hash_search(MtmXid2State, &msg->dxid, HASH_FIND, NULL);
					if (ts == NULL) { 
						elog(WARNING, "Ignore response for unexisted transaction %llu from node %d", (long64)msg->dxid, node);
						continue;
					}
					Assert(msg->code == MSG_ABORTED || strcmp(msg->gid, ts->gid) == 0);
					if (BIT_CHECK(ts->votedMask, node-1)) {
						elog(WARNING, "Receive deteriorated %s response for transaction %s (%llu) from node %d",
							 MtmMessageKindMnem[msg->code], ts->gid, (long64)ts->xid, node);
						continue;
					}
					BIT_SET(ts->votedMask, node-1);

					if (MtmIsCoordinator(ts)) {
						switch (msg->code) { 
						  case MSG_PREPARED:
							MTM_TXTRACE(ts, "MtmTransReceiver got MSG_PREPARED");
							if (ts->status == TRANSACTION_STATUS_COMMITTED) { 
								elog(WARNING, "Receive PREPARED response for already committed transaction %llu from node %d",
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
								elog(WARNING, "Coordinator of distributed transaction %s (%llu) see less nodes than node %d: %llx instead of %llx",
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
										//MtmSend2PCMessage(ts, MSG_PRECOMMIT);	
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
								elog(WARNING, "Receive ABORTED response for already committed transaction %s (%llu) from node %d",
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
                                elog(WARNING, "Receive PRECOMMITTED response for already committed transaction %s (%llu) from node %d",
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
								elog(WARNING, "Receive PRECOMMITTED response for aborted transaction %s (%llu) from node %d", 
									 ts->gid, (long64)ts->xid, node); // How it can happen? SHould we use assert here?
								if ((ts->participantsMask & ~Mtm->disabledNodeMask & ~ts->votedMask) == 0) {
									MtmWakeUpBackend(ts);
								}
							}	
							break;
						  default:
							Assert(false);
						} 
					} else { 
						switch (msg->code) { 
						  case MSG_PRECOMMIT:
							Assert(false); // Now sent through pglogical 
							if (ts->status == TRANSACTION_STATUS_IN_PROGRESS) {
								ts->status = TRANSACTION_STATUS_UNKNOWN;
								ts->csn = MtmAssignCSN();
								MtmAdjustSubtransactions(ts);
								MtmSend2PCMessage(ts, MSG_PRECOMMITTED);
							} else if (ts->status == TRANSACTION_STATUS_ABORTED) {
								MtmSend2PCMessage(ts, MSG_ABORTED);
							} else { 
								elog(WARNING, "Transaction %s is already %s", ts->gid, MtmTxnStatusMnem[ts->status]);
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
		if (Mtm->status == MTM_ONLINE) { 
			now = MtmGetSystemTime();
			/* Check for heartbeats only in case of timeout expiration: it means that we do not have unproceeded events.
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

