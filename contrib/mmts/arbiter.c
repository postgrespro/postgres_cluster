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

#ifdef USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/select.h>
#endif


#include "multimaster.h"

#define MAX_CONNECT_ATTEMPTS 10
#define MAX_ROUTES           16
#define BUFFER_SIZE          1024

typedef struct
{
	TransactionId dxid; /* Transaction ID at destination node */
	TransactionId sxid; /* Transaction IO at sender node */  
    int           node; /* Sender node ID */
	csn_t         csn;  /* local CSN in case of sending data from replica to master, global CSN master->replica */
} DtmCommitMessage;

typedef struct 
{
	int used;
	DtmCommitMessage data[BUFFER_SIZE];
} DtmBuffer;

static int* sockets;
static DtmState* ds;

static void DtmTransSender(Datum arg);
static void DtmTransReceiver(Datum arg);

static BackgroundWorker DtmSender = {
	"mm-sender",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	1, /* restrart in one second (is it possible to restort immediately?) */
	DtmTransSender
};

static BackgroundWorker DtmRecevier = {
	"mm-receiver",
	BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION, /* do not need connection to the database */
	BgWorkerStart_ConsistentState,
	1, /* restrart in one second (is it possible to restort immediately?) */
	DtmTransReceiver
};

void MMArbiterInitialize(void)
{
	RegisterBackgroundWorker(&DtmSender);
	RegisterBackgroundWorker(&DtmRecevier);
}

static int resolve_host_by_name(const char *hostname, unsigned* addrs, unsigned* n_addrs)
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

static void registerSocket(int fd, int i)
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



static int connectSocket(char const* host, int port)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[MAX_ROUTES];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
    int max_attempts = MAX_CONNECT_ATTEMPTS;
	int sd;

    sock_inet.sin_family = AF_INET;
	sock_inet.sin_port = htons(port);

	if (!resolve_host_by_name(host, addrs, &n_addrs)) {
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

static void openConnections()
{
	int nNodes = MMNodes;
	int i;
	char* connStr = pstrdup(MMConnStrs);

	sockets = (int*)palloc(sizeof(int)*nNodes);

	for (i = 0; i < nNodes; i++) {
		char* host = strstr(connStr, "host=");
		char* end;
		if (host == NULL) {
			elog(ERROR, "Invalid connection string: '%s'", MMConnStrs);
		}
		host += 5;
		for (end = host; *end != ' ' && *end != ',' && *end != '\0'; end++);
		if (*end != '\0') { 
			*end = '\0';
			connStr = end + 1;
		} else { 
			connStr = end;
		}
		sockets[i] = i+1 != MMNodeId ? connectSocket(host, MMArbiterPort + i + 1) : -1;
	}
}

static void acceptConnections()
{
	struct sockaddr_in sock_inet;
	int i;
	int sd;
    int on = 1;
	int nNodes = MMNodes-1;

	sockets = (int*)palloc(sizeof(int)*nNodes);

	sock_inet.sin_family = AF_INET;
	sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_inet.sin_port = htons(MMArbiterPort + MMNodeId);

    sd = socket(sock_inet.sin_family, SOCK_STREAM, 0);
	if (sd < 0) {
		elog(ERROR, "Failed to create socket: %d", errno);
	}
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    if (bind(sd, (struct sockaddr*)&sock_inet, sizeof(sock_inet)) < 0) {
		elog(ERROR, "Failed to bind socket: %d", errno);
	}	
    if (listen(sd, MMNodes-1) < 0) {
		elog(ERROR, "Failed to listen socket: %d", errno);
	}	

	for (i = 0; i < nNodes; i++) {
		int fd = accept(sd, NULL, NULL);
		if (fd < 0) {
			elog(ERROR, "Failed to accept socket: %d", errno);
		}	
		registerSocket(fd, i);
		sockets[i] = fd;
	}
	close(sd);
}

static void writeSocket(int sd, void const* buf, int size)
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

static int readSocket(int sd, void* buf, int buf_size)
{
	int rc = recv(sd, buf, buf_size, 0);
	if (rc <= 0) { 
		elog(ERROR, "Arbiter failed to read socket: %d", rc);
	}
	return rc;
}


static void DtmTransSender(Datum arg)
{
	int nNodes = MMNodes;
	int i;
	DtmBuffer* txBuffer = (DtmBuffer*)palloc(sizeof(DtmBuffer)*nNodes);
	
	sockets = (int*)palloc(sizeof(int)*nNodes);
	ds = MMGetState();

	openConnections();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (true) {
		DtmTransState* ts;		
		PGSemaphoreLock(&ds->votingSemaphore);
		CHECK_FOR_INTERRUPTS();

		SpinLockAcquire(&ds->votingSpinlock);
		ts = ds->votingTransactions;
		ds->votingTransactions = NULL;
		SpinLockRelease(&ds->votingSpinlock);

		for (; ts != NULL; ts = ts->nextVoting) {
			if (ts->gtid.node == MMNodeId) { 
				/* Coordinator is broadcasting confirmations to replicas */
				for (i = 0; i < nNodes; i++) { 
					if (TransactionIdIsValid(ts->xids[i])) { 
						if (txBuffer[i].used == BUFFER_SIZE) { 
							writeSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(DtmCommitMessage));
							txBuffer[i].used = 0;
						}
						DTM_TRACE("Send notification %ld to replica %d from coordinator %d for transaction %d (local transaction %d)\n", 
								  ts->csn, i+1, MMNodeId, ts->xid, ts->xids[i]);

						txBuffer[i].data[txBuffer[i].used].dxid = ts->xids[i];
						txBuffer[i].data[txBuffer[i].used].sxid = ts->xid;
						txBuffer[i].data[txBuffer[i].used].csn = ts->csn;
						txBuffer[i].data[txBuffer[i].used].node = MMNodeId;
						txBuffer[i].used += 1;
					}
				}
			} else { 
				/* Replica is notifying master */
				i = ts->gtid.node-1;
				if (txBuffer[i].used == BUFFER_SIZE) { 
					writeSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(DtmCommitMessage));
					txBuffer[i].used = 0;
				}
				DTM_TRACE("Send notification %ld to coordinator %d from node %d for transaction %d (local transaction %d)\n", 
						  ts->csn, ts->gtid.node, MMNodeId, ts->gtid.xid, ts->xid);
				txBuffer[i].data[txBuffer[i].used].dxid = ts->gtid.xid;
				txBuffer[i].data[txBuffer[i].used].sxid = ts->xid;
				txBuffer[i].data[txBuffer[i].used].node = MMNodeId;
				txBuffer[i].data[txBuffer[i].used].csn = ts->csn;
				txBuffer[i].used += 1;
			}
		}
		for (i = 0; i < nNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				writeSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(DtmCommitMessage));
				txBuffer[i].used = 0;
			}
		}		
	}
}

static void DtmTransReceiver(Datum arg)
{
	int nNodes = MMNodes-1;
	int i, j, rc;
	DtmBuffer* rxBuffer = (DtmBuffer*)palloc(sizeof(DtmBuffer)*nNodes);
	HTAB* xid2state;

#if USE_EPOLL
	struct epoll_event* events = (struct epoll_event*)palloc(sizeof(struct epoll_event)*nNodes);
    epollfd = epoll_create(nNodes);
#else
    FD_ZERO(&inset);
    max_fd = 0;
#endif
	
	ds = MMGetState();

	acceptConnections();
	xid2state = MMCreateHash();

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
				rxBuffer[i].used += readSocket(sockets[i], (char*)rxBuffer[i].data + rxBuffer[i].used, BUFFER_SIZE-rxBuffer[i].used);
				nResponses = rxBuffer[i].used/sizeof(DtmCommitMessage);

				LWLockAcquire(ds->hashLock, LW_SHARED);						

				for (j = 0; j < nResponses; j++) { 
					DtmCommitMessage* msg = &rxBuffer[i].data[j];
					DtmTransState* ts = (DtmTransState*)hash_search(xid2state, &msg->dxid, HASH_FIND, NULL);
					Assert(ts != NULL);
					if (msg->csn > ts->csn) { 
						ts->csn = msg->csn;
					}
					Assert((unsigned)(msg->node-1) <= (unsigned)nNodes);
					ts->xids[msg->node-1] = msg->sxid;
					DTM_TRACE("Receive response %ld for transaction %d votes %d from node %d (transaction %d)\n", 
							  msg->csn, msg->dxid, ts->nVotes+1, msg->node, msg->sxid);
					Assert(ts->nVotes > 0 && ts->nVotes < ds->nNodes);
					if (++ts->nVotes == ds->nNodes) { 
						SetLatch(&ProcGlobal->allProcs[ts->procno].procLatch);
					}
				}
				LWLockRelease(ds->hashLock);
				
				rxBuffer[i].used -= nResponses*sizeof(DtmCommitMessage);
				if (rxBuffer[i].used != 0) { 
					memmove(rxBuffer[i].data, (char*)rxBuffer[i].data + nResponses*sizeof(DtmCommitMessage), rxBuffer[i].used);
				}
			}
		}
	}
}

