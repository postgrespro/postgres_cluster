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
#define TX_BUFFER_SIZE 1024

typedef struct
{
	TransactionId xid;
	csn_t         csn;
} DtmCommitMessage;

typedef struct 
{
	DtmCOmmitMessage buf[TX_BUFFER_SIZE];
	int used;
} DtmTxBuffer;

static int* sockets;
static DtmCommitMessage** txBuffers;

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

	for (i = 0; i < nNodes-1; i++) {
		sockets[i] = accept(sd, NULL, NULL);
		if (sockets[i] < 0) {
			elog(ERROR, "Failed to accept socket: %d", errno);
		}	
	}
}

static void DtmTransSender(Datum arg)
{
	txBuffer = (DtmCommitMessage*)
	openConnections();

	while (true) {
		DtmTransState* ts;
		PGSemaphoreLock(&dtm->semphore);

		LWLockAcquire(&dtm->hashLock, LW_EXCLUSIVE);
		for (ts = dtm->pendingTransactions; ts != NULL; ts = ts->nextPending) {
			int node = ts->gtid.node;
			Assert(node != MMNodeId);
			sockets
}

static void DtmTransReceiver(Datum arg)
{
	acceptConnections();
}

