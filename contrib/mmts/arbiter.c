/*
 * multimaster.c
 *
 * Multimaster based on logical replication
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "libpq-fe.h"
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

#include "multimaster.h"

#define MAX_CONNECT_ATTEMPTS 10
#define BUFFER_SIZE 1024
#define BUFFER_SIZE 1024

typedef struct
{
	TransactionId xid;
	csn_t         csn;
} DtmCommitMessage;

typedef struct 
{
	DtmCommitMessage data[BUFFER_SIZE];
	int used;
} DtmBuffer;

static int* sockets;

static BackgroundWorker DtmSender = {
	"mm-sender",
	0, /* do not need connection to the database */
	BgWorkerStart_PostmasterStart,
	1, /* restrart in one second (is it possible to restort immediately?) */
	DtmTransSender
};

static BackgroundWorker DtmRecevier = {
	"mm-receiver",
	0, /* do not need connection to the database */
	BgWorkerStart_PostmasterStart,
	1, /* restrart in one second (is it possible to restort immediately?) */
	DtmTransReceiver
};

void MMArbiterInitialize()
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

#ifdef USE_EPOLL
static int    epollfd;
#else
static int    max_fd;
static fd_set inset;
#endif

inline void registerSocket(int fd, int i)
{
#ifdef USE_EPOLL
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u32 = i;        
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        char buf[ERR_BUF_SIZE];
        sprintf(buf, "Failed to add socket %d to epoll set", fd);
        shub->params->error_handler(buf, SHUB_FATAL_ERROR);
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
    unsigned addrs[128];
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
				elog(ERROR, "Sockhub failed to connect to %s:%d: %d", host, port, errno);
			} else { 
				max_attempts -= 1;
				sleep(1);
			}
			continue;
		} else {
			int optval = 1;
			setsockopt(shub->output, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
			return sd;
		}
    }
}

static void openConnections()
{
	int nNodes = dtm->nNodes;
	int i;
	char* connStr = pstrdup(MMConnStrs);

	sockets = (int*)palloc(sizeof(int)*nNodes);

	for (i = 0; i < nNodes; i++) {
		char* host = strstr(connStr, "host=");
		char* end;
		if (host == NULL) {
			elog(ERROR, "Invalid connection string: '%s'", MMConnStrs);
		}
		for (end = host+5; *end != ' ' && *end != ',' && end != '\0'; end++);
		*end = '\0';
		connStr = end + 1;
		sockets[i] = i+1 != MMNodeId ? connectSocket(host, MMArbiterPort + i) : -1;
	}
}

static void acceptConnections()
{
	int nNodes = dtm->nNodes-1;
	sockaddr_in sock_inet;
	int i;
	int sd;
    int on = 1;

	sockets = (int*)palloc(sizeof(int)*nNodes);

	sock_inet.sin_family = AF_INET;
	sock_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_inet.sin_port = htons(MMArbiterPort + MMNodeId);

    sd = socket(u.sock.sa_family, SOCK_STREAM, 0);
	if (sd < 0) {
		elog(ERROR, "Failed to create socket: %d", errno);
	}
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    if (bind(fd, (sockaddr*)&sock_init, nNodes-1) < 0) {
		elog(ERROR, "Failed to bind socket: %d", errno);
	}	

	for (i = 0; i < nNodes; i++) {
		int fd = accept(sd, NULL, NULL);
		if (fd < 0) {
			elog(ERROR, "Failed to accept socket: %d", errno);
		}	
		registerSocket(fd, i);
		sockets[i] = fd;
	}
}

static void WriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) {
        int n = send(sd, src, size, 0);
        if (n <= 0) {
            return 0;
        }
        size -= n;
        src += n;
    }
}

static int ReadSocket(int sd, void* buf, int buf_size)
{
	int rc = recv(sd, buf, buf_size, 0);
	if (rc <= 0) { 
		elog(ERROR, "Arbiter failed to read socket: %d", rc);
	}
	return rc;
}


static void DtmTransSender(Datum arg)
{
	int nNodes = dtm->nNodes;
	int i;
	DtmTxBuffer* txBuffer = (DtmTxBuffer*)palloc(sizeof(DtmTxBuffer)*nNodes);
	
	sockets = (int*)palloc(sizeof(int)*nNodes);

	openConnections();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (true) {
		DtmTransState* ts;		
		PGSemaphoreLock(&dtm->semphore);
		CHECK_FOR_INTERRUPTS();

		SpinLockAcquire(&dtm->spinlock);
		ts = dtm->pendingTransactions;
		dtm->pendingTransactions = NULL;
		SpinLockRelease(&dtm->spinlock);

		for (; ts != NULL; ts = ts->nextPending) {
			i = ts->gtid.node-1;
			Assert(i != MMNodeId);
			if (txBuffer[i].used == BUFFER_SIZE) { 
				WriteSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(DtmCommitRequest));
				txBuffer[i].used = 0;
			}
			txBuffer[i].data[txBuffer[i].used].xid = ts->xid;
			txBuffer[i].data[txBuffer[i].used].csn = ts->csn;
			txBuffer[i].used += 1;
		}
		for (i = 0; i < nNodes; i++) { 
			if (txBuffer[i].used != 0) { 
				WriteSocket(sockets[i], txBuffer[i].data, txBuffer[i].used*sizeof(DtmCommitRequest));
				txBuffer[i].used = 0;
			}
		}		
	}
}

static void DtmTransReceiver(Datum arg)
{
	int nNodes = dtm->nNodes-1;
	int i, j, rc;
	int rxBufPos = 0;
	DtmBuffer* rxBuffer = (DtmBuffer*)palloc(sizeof(DtmBuffer)*nNodes);
	HTAB* xid2state;

#ifdef USE_EPOLL
	struct epoll_event* events = (struct epoll_event*)palloc(SIZEOF(struct epoll_event)*nNodes);
    epollfd = epoll_create(nNodes);
#else
    FD_ZERO(&inset);
    max_fd = 0;
#endif
	
	acceptConnections();
	xid2state = MMCreateHash();

	for (i = 0; i < nNodes; i++) { 
		txBuffer[i].used = 0;
	}

	while (true) {
#ifdef USE_EPOLL
        rc = epoll_wait(epollfd, events, MAX_EVENTS, shub->in_buffer_used == 0 ? -1 : shub->params->delay);
		if (rc < 0) { 
			elog(ERROR, "epoll failed: %d", errno);
		}
		for (j = 0; j < rc; j++) {
			i = events[j].data.u32;
			if (events[j].events & EPOLLERR) {
				struct sockaddr_in insock;
				socklen_t len = sizeof(insock);
				getpeername(fd, (struct sockaddr*)&insock, &len);
				elog(WARNING, "Loose connection with %s", inet_ntoa(insock.sin_addr_));
				epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
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
				rxBuffer[i].used += ReadSocket(sockets[i], (char*)rxBuffer[i].data + rxBuffer[i].used, RX_BUFFER_SIZE-rxBufPos);
				nResponses = rxBuffer[i].used/sizeof(DtmCommitRequest);

				LWLockAcquire(&dtm->hashLock, LW_SHARED);						

				for (j = 0; j < nResponses; j++) { 
					DtmCommitRequest* req = &rxBuffer[i].data[j];
					DtmTransState* ts = (DtmTransState*)hash_search(xid2state, &req->xid, HASH_FIND, NULL);
					Assert(ts != NULL);
					if (req->csn > ts->csn) { 
						ts->csn = req->csn;
					}
					if (ts->nVotes == dtm->nNodes-1) { 
						SetLatch(&ProcGlobal->allProcs[ts->pid].procLatch);
					}
				}
				if (rxBuffer[i].used != nResponses*sizeof(DtmCommitRequest)) { 
					rxBuffer[i].used -= nResponses*sizeof(DtmCommitRequest);
					memmove(rxBuffer[i].data, (char*)rxBuffer[i].data + nResponses*sizeof(DtmCommitRequest), rxBuffer[i].used);
				}
			}
		}
	}
}

