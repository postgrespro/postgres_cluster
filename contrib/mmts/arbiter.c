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

#define MAX_CONNECT_ATTEMPTS 10
#define MAX_ROUTES           16
#define BUFFER_SIZE          1024

typedef enum
{ 
	MSG_READY,
	MSG_PREPARE,
	MSG_COMMIT,
	MSG_ABORT,
	MSG_PREPARED,
	MSG_COMMITTED,
	MSG_ABORTED
} MessageCode;


typedef struct
{
	MessageCode   code; /* Message code: MSG_READY, MSG_PREPARE, MSG_COMMIT, MSG_ABORT */
    int           node; /* Sender node ID */
	TransactionId dxid; /* Transaction ID at destination node */
	TransactionId sxid; /* Transaction IO at sender node */  
	csn_t         csn;  /* local CSN in case of sending data from replica to master, global CSN master->replica */
} MtmCommitMessage;

typedef struct 
{
	int used;
	MtmCommitMessage data[BUFFER_SIZE];
} MtmBuffer;

static int* sockets;
static MtmState* ds;

static void MtmTransSender(Datum arg);
static void MtmTransReceiver(Datum arg);

static char const* const messageText[] = 
{
	"READY",
	"PREPARE",
	"COMMIT",
	"ABORT",
	"PREPARED",
	"COMMITTED",
	"ABORTED"
};


static BackgroundWorker MtmSender = {
	"mm-sender",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	1, /* restart in one second (is it possible to restart immediately?) */
	MtmTransSender
};

static BackgroundWorker MtmRecevier = {
	"mm-receiver",
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

static void MtmRegisterSocket(int fd, int i)
{
#if USE_EPOLL
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u32 = i;        
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        elog(ERROR, "Failed to add socket to epoll set: %d", errno);
    } 
#else
    FD_SET(fd, &inset);    
    if (fd > max_fd) {
        max_fd = fd;
    }
#endif          
}     



static int MtmConnectSocket(char const* host, int port)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[MAX_ROUTES];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
    int max_attempts = MAX_CONNECT_ATTEMPTS;
	int sd;

    sock_inet.sin_family = AF_INET;
	sock_inet.sin_port = htons(port);

	if (!MtmResolveHostByName(host, addrs, &n_addrs)) {
		elog(ERROR, "Failed to resolve host '%s' by name", host);
	}
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
	    elog(ERROR, "Failed to create socket: %d", errno);
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
				elog(ERROR, "Arbiter failed to connect to %s:%d: %d", host, port, errno);
			} else { 
				max_attempts -= 1;
				sleep(1);
			}
			continue;
		} else {
			int optval = 1;
			setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
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
		sockets[i] = i+1 != MtmNodeId ? MtmConnectSocket(host, MtmArbiterPort + i + 1) : -1;
	}
}

static void MtmAcceptConnections()
{
	struct sockaddr_in sock_inet;
	int i;
	int sd;
    int on = 1;
	int nNodes = MtmNodes-1;

	sockets = (int*)palloc(sizeof(int)*nNodes);

	sock_inet.sin_family = AF_INET;
	sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_inet.sin_port = htons(MtmArbiterPort + MtmNodeId);

    sd = socket(sock_inet.sin_family, SOCK_STREAM, 0);
	if (sd < 0) {
		elog(ERROR, "Failed to create socket: %d", errno);
	}
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    if (bind(sd, (struct sockaddr*)&sock_inet, sizeof(sock_inet)) < 0) {
		elog(ERROR, "Failed to bind socket: %d", errno);
	}	
    if (listen(sd, MtmNodes-1) < 0) {
		elog(ERROR, "Failed to listen socket: %d", errno);
	}	

	for (i = 0; i < nNodes; i++) {
		int fd = accept(sd, NULL, NULL);
		if (fd < 0) {
			elog(ERROR, "Failed to accept socket: %d", errno);
		}	
		MtmRegisterSocket(fd, i);
		sockets[i] = fd;
	}
	close(sd);
}

static void MtmWriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) {
        int n = send(sd, src, size, 0);
        if (n <= 0) {
            elog(ERROR, "Write socket failed: %d", errno);
        }
        size -= n;
        src += n;
    }
}

static int MtmReadSocket(int sd, void* buf, int buf_size)
{
	int rc = recv(sd, buf, buf_size, 0);
	if (rc <= 0) { 
		elog(ERROR, "Arbiter failed to read socket: %d", rc);
	}
	return rc;
}


static void MtmAppendBuffer(MessageCode code, MtmBuffer* txBuffer, TransactionId xid, int node, MtmTransState* ts)
{
	MtmBuffer* buf = &txBuffer[node];
	if (buf->used == BUFFER_SIZE) { 
		MtmWriteSocket(sockets[node], buf->data, buf->used*sizeof(MtmCommitMessage));
		buf->used = 0;
	}
	DTM_TRACE("Send message %s CSN=%ld to node %d from node %d for global transaction %d/local transaction %d\n", 
			  messageText[code], ts->csn, node, MtmNodeId, ts->gtid.xid, ts->xid);
	buf->data[buf->used].code = code;
	buf->data[buf->used].dxid = xid;
	buf->data[buf->used].sxid = ts->xid;
	buf->data[buf->used].csn =  ts->csn;
	buf->data[buf->used].node = MtmNodeId;
	buf->used += 1;
}

static void MtmBroadcastMessage(MessageCode code, MtmBuffer* txBuffer, MtmTransState* ts)
{
	int i;
	int n = 1;
	for (i = 0; i < MtmNodes; i++)
	{
		if (TransactionIdIsValid(ts->xids[i])) { 
			MtmAppendBuffer(code, txBuffer, ts->xids[i], i, ts);
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
	
	sockets = (int*)palloc(sizeof(int)*nNodes);
	ds = MtmGetState();

	MtmOpenConnections();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (true) {
		MtmTransState* ts;		
		PGSemaphoreLock(&ds->votingSemaphore);
		CHECK_FOR_INTERRUPTS();

		SpinLockAcquire(&ds->votingSpinlock);
		ts = ds->votingTransactions;
		ds->votingTransactions = NULL;
		SpinLockRelease(&ds->votingSpinlock);

		for (; ts != NULL; ts = ts->nextVoting) {
			if (MtmIsCoordinator(ts)) {
				/* Coordinator is broadcasting message to replicas */
				MessageCode code = MSG_ABORT;
				switch (ts->status) { 
				  case TRANSACTION_STATUS_IN_PROGRESS:
					code = MSG_PREPARE;
					break;
				  case TRANSACTION_STATUS_COMMITTED:
					code = MSG_COMMIT;
					break;
				  case TRANSACTION_STATUS_ABORTED:
					code = MSG_ABORT;
					break;
				  default:
					Assert(false);
				}
				MtmBroadcastMessage(code, txBuffer, ts);
			} else { 
				/* Replica is notifying master about it's state */
				MessageCode code = MSG_ABORT;
				switch (ts->status) { 
				  case TRANSACTION_STATUS_UNKNOWN:
					code = MSG_READY;
					break;
				  case TRANSACTION_STATUS_IN_PROGRESS:
					code = MSG_PREPARED;
					break;
				  case TRANSACTION_STATUS_COMMITTED:
					code = MSG_COMMITTED;
					break;
				  case TRANSACTION_STATUS_ABORTED:
					code = MSG_ABORTED;
					break;
				  default:
					Assert(false);
				}
				MtmAppendBuffer(code, txBuffer, ts->gtid.xid, ts->gtid.node-1, ts);
			}
		}
		for (i = 0; i < nNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				MtmWriteSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(MtmCommitMessage));
				txBuffer[i].used = 0;
			}
		}		
	}
}

static void MtmWakeUpBackend(MtmTransState* ts)
{
	SetLatch(&ProcGlobal->allProcs[ts->procno].procLatch); 
}

static void MtmTransReceiver(Datum arg)
{
	int nNodes = MtmNodes-1;
	int i, j, rc;
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

	MtmAcceptConnections();
	xid2state = MtmCreateHash();

	for (i = 0; i < nNodes; i++) { 
		rxBuffer[i].used = 0;
	}

	while (true) {
#if USE_EPOLL
        rc = epoll_wait(epollfd, events, nNodes, -1);
		if (rc < 0) { 
			elog(ERROR, "epoll failed: %d", errno);
		}
		for (j = 0; j < rc; j++) {
			i = events[j].data.u32;
			if (events[j].events & EPOLLERR) {
				struct sockaddr_in insock;
				socklen_t len = sizeof(insock);
				getpeername(sockets[i], (struct sockaddr*)&insock, &len);
				elog(WARNING, "Loose connection with %s", inet_ntoa(insock.sin_addr));
				epoll_ctl(epollfd, EPOLL_CTL_DEL, sockets[i], NULL);
			} 
			else if (events[j].events & EPOLLIN)  
#else
        fd_set events;
        events = inset;
        rc = select(max_fd+1, &events, NULL, NULL, NULL);
		if (rc < 0) { 
			elog(ERROR, "select failed: %d", errno);
		}
		for (i = 0; i < nNodes; i++) { 
			if (FD_ISSET(sockets[i], &events)) 
#endif
			{
				int nResponses;
				rxBuffer[i].used += MtmReadSocket(sockets[i], (char*)rxBuffer[i].data + rxBuffer[i].used, BUFFER_SIZE-rxBuffer[i].used);
				nResponses = rxBuffer[i].used/sizeof(MtmCommitMessage);

				LWLockAcquire(ds->hashLock, LW_EXCLUSIVE);						

				for (j = 0; j < nResponses; j++) { 
					MtmCommitMessage* msg = &rxBuffer[i].data[j];
					MtmTransState* ts = (MtmTransState*)hash_search(xid2state, &msg->dxid, HASH_FIND, NULL);
					Assert(ts != NULL);
					Assert((unsigned)(msg->node-1) <= (unsigned)nNodes);
					ts->xids[msg->node-1] = msg->sxid;

					if (MtmIsCoordinator(ts)) { 
						switch (msg->code) { 
						  case MSG_READY:
							if (ts->status == TRANSACTION_STATUS_UNKNOWN) {
								Assert(ts->nVotes < ds->nNodes);
								if (++ts->nVotes == ds->nNodes) { 
									/* All nodes are ready to commit: assign CSN and switch to in-doubt state */
									ts->nVotes = 1; /* I voted myself */
									ts->csn = MtmAssignCSN();	
									ts->status = TRANSACTION_STATUS_IN_PROGRESS; 
									MtmAdjustSubtransactions(ts);
									MtmSendNotificationMessage(ts);
								}
							} else { 
								Assert(ts->status == TRANSACTION_STATUS_ABORTED);
							}
							break;
						  case MSG_PREPARED:
							Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);							  
							if (msg->csn > ts->csn) {
								ts->csn = msg->csn;
							}
							Assert(ts->nVotes < ds->nNodes);
							if (++ts->nVotes == ds->nNodes) { 
								/* All nodes are in in-doubt state, ts->csn is maximum of CSNs at all nodes */
								/* Now do commit */
								ts->nVotes = 1; /* I voted myself */
								MtmSyncClock(ts->csn);
								ts->status = TRANSACTION_STATUS_COMMITTED;
								MtmAdjustSubtransactions(ts);
								MtmSendNotificationMessage(ts);
							}
							break;
						  case MSG_COMMITTED:
							Assert(ts->status == TRANSACTION_STATUS_COMMITTED);
							Assert(ts->nVotes < ds->nNodes);
							if (++ts->nVotes == ds->nNodes) { 									
								MtmWakeUpBackend(ts);
							}
							break;
						  case MSG_ABORTED:
							if (ts->status != TRANSACTION_STATUS_ABORTED) {
								Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
								ts->status = TRANSACTION_STATUS_ABORTED;									
								MtmAdjustSubtransactions(ts);
								MtmWakeUpBackend(ts);
							}
							break;
						  default:
							Assert(false);
						}
					} else { /* replica */
						switch (msg->code) { 
						  case MSG_PREPARE:
							Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
							ts->csn = MtmAssignCSN();	
							ts->status = TRANSACTION_STATUS_IN_PROGRESS; 
							MtmAdjustSubtransactions(ts);
							MtmSendNotificationMessage(ts);
							break;
						  case MSG_COMMIT:
							Assert(ts->status == TRANSACTION_STATUS_IN_PROGRESS);
							ts->csn = msg->csn;
							MtmSyncClock(ts->csn);
							ts->status = TRANSACTION_STATUS_COMMITTED;
							MtmAdjustSubtransactions(ts);
							MtmSendNotificationMessage(ts);
							MtmWakeUpBackend(ts);
							break;
						  case MSG_ABORT:
							if (ts->status != TRANSACTION_STATUS_ABORTED) {
								Assert(ts->status == TRANSACTION_STATUS_UNKNOWN);
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
				LWLockRelease(ds->hashLock);
				
				rxBuffer[i].used -= nResponses*sizeof(MtmCommitMessage);
				if (rxBuffer[i].used != 0) { 
					memmove(rxBuffer[i].data, (char*)rxBuffer[i].data + nResponses*sizeof(MtmCommitMessage), rxBuffer[i].used);
				}
			}
		}
	}
}

