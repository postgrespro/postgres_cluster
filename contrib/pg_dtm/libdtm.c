#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "postgres.h"
#include "libdtm.h"
#include "dtmd/include/proto.h"
#include "dtmd/include/dtmdlimits.h"
#include "sockhub/sockhub.h"

#ifdef TEST
// standalone test without postgres functions
#define palloc malloc
#define pfree free
#endif

#define COMMAND_BUFFER_SIZE 1024
#define RESULTS_SIZE 1024 // size in 32-bit numbers

typedef struct DTMConnData *DTMConn;

typedef struct DTMConnData
{
	char *host; // use unix socket if host is NULL
	int port;
	int sock;
} DTMConnData;

static bool connected = false;
static int leader = 0;
static int connum = 0;
static DTMConnData conns[MAX_SERVERS];
static char *dtm_unix_sock_dir;

typedef unsigned xid_t;

static void DiscardConnection()
{
	if (connected)
	{
		close(conns[leader].sock);
		conns[leader].sock = -1;
		connected = false;
	}
	leader = (leader + 1) % connum;
	fprintf(stderr, "pid=%d: next candidate is %s:%d (%d of %d)\n", getpid(), conns[leader].host, conns[leader].port, leader, connum);
}

static int dtm_recv_results(DTMConn dtm, int maxlen, xid_t *results)
{
	ShubMessageHdr msg;
	int recved;
	int needed;

	recved = 0;
	needed = sizeof(ShubMessageHdr);
	while (recved < needed)
	{
		int newbytes = read(dtm->sock, (char*)&msg + recved, needed - recved);
		if (newbytes == -1)
		{
			DiscardConnection();
			elog(WARNING, "Failed to recv results header from arbiter");
			return 0;
		}
		if (newbytes == 0)
		{
			DiscardConnection();
			elog(WARNING, "Arbiter closed connection during recv");
			return 0;
		}
		recved += newbytes;
	}

	recved = 0;
	needed = msg.size;
	assert(needed % sizeof(xid_t) == 0);
	if (needed > maxlen * sizeof(xid_t))
	{
		elog(ERROR, "The message body will not fit into the results array");
		return 0;
	}
	while (recved < needed)
	{
		int newbytes = read(dtm->sock, (char*)results + recved, needed - recved);
		if (newbytes == -1)
		{
			DiscardConnection();
			elog(WARNING, "Failed to recv results body from arbiter");
			return 0;
		}
		if (newbytes == 0)
		{
			DiscardConnection();
			elog(WARNING, "Arbiter closed connection during recv");
			return 0;
		}
		recved += newbytes;
	}
	return needed / sizeof(xid_t);
}

// Connects to the specified DTM.
static bool DtmConnect(DTMConn conn)
{
	int sd;

	if (conn->host == NULL)
	{
		perror("unix socket not supported yet");
		*(int*)0 = 0;
		/*
		// use a UNIX socket
		struct sockaddr sock;
		int len = offsetof(struct sockaddr, sa_data) + snprintf(sock.sa_data, sizeof(sock.sa_data), "%s/p%u", dtm_unix_sock_dir, port);
		sock.sa_family = AF_UNIX;

		sd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sd == -1)
		{
			DiscardConnection();
			perror("failed to create a unix socket");
			return false;
		}
		if (connect(sd, &sock, len) == -1)
		{
			DiscardConnection();
			perror("failed to connect to the address");
			close(sd);
			return false;
		}
		conn->sock = sd;
		return (connected = true);
		*/
	}
	else
	{
		// use an IP socket
		struct addrinfo *addrs = NULL;
		struct addrinfo hint;
		char portstr[6];
		struct addrinfo *a;

		memset(&hint, 0, sizeof(hint));
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_family = AF_INET;
		snprintf(portstr, 6, "%d", conn->port);
		hint.ai_protocol = getprotobyname("tcp")->p_proto;

		if (getaddrinfo(conn->host, portstr, &hint, &addrs))
		{
			DiscardConnection();
			perror("failed to resolve address");
			return false;
		}

		for (a = addrs; a != NULL; a = a->ai_next)
		{
			int one = 1;

			sd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
			if (sd == -1)
			{
				perror("failed to create a socket");
				continue;
			}
			setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

			if (connect(sd, a->ai_addr, a->ai_addrlen) == -1)
			{
				perror("failed to connect to an address");
				close(sd);
				continue;
			}

			// success
			freeaddrinfo(addrs);
			conn->sock = sd;
			return (connected = true);
		}
		freeaddrinfo(addrs);
	}
	DiscardConnection();
	fprintf(stderr, "could not connect\n");
	return false;
}

static bool dtm_send_command(DTMConn dtm, xid_t cmd, int argc, ...)
{
	va_list argv;
	int i;
	int sent;
	char buf[COMMAND_BUFFER_SIZE];
	int datasize;
	char *cursor = buf;

	ShubMessageHdr *msg = (ShubMessageHdr*)cursor;
	msg->chan = 0;
	msg->code = MSG_FIRST_USER_CODE;
	msg->size = sizeof(xid_t) * (argc + 1);
	cursor += sizeof(ShubMessageHdr);

	*(xid_t*)cursor = cmd;
	cursor += sizeof(xid_t);

	va_start(argv, argc);
	for (i = 0; i < argc; i++)
	{
		*(xid_t*)cursor = va_arg(argv, xid_t);
		cursor += sizeof(xid_t);
	}
	va_end(argv);

	datasize = cursor - buf;
	assert(msg->size + sizeof(ShubMessageHdr) == datasize);
	assert(datasize <= COMMAND_BUFFER_SIZE);

	sent = 0;
	while (sent < datasize)
	{
		int newbytes = write(dtm->sock, buf + sent, datasize - sent);
		if (newbytes == -1)
		{
			DiscardConnection();
			elog(ERROR, "Failed to send a command to arbiter");
			return false;
		}
		sent += newbytes;
	}
	return true;
}

void DtmGlobalConfig(char *servers, char *sock_dir)
{
	char *hstate, *pstate;
	char *hostport, *host, *portstr;
	int port;
	int i;

	for (i = 0; i < connum; i++)
		if (conns[i].host)
			free(conns[i].host);
	connum = 0;

	fprintf(stderr, "parsing '%s'\n", servers);
	hostport = strtok_r(servers, ",", &hstate);
	while (hostport)
	{
		fprintf(stderr, "hostport = '%s'\n", hostport);
		host = strtok_r(hostport, ":", &pstate);
		fprintf(stderr, "host = '%s'\n", hostport);
		if (!host) break;

		portstr = strtok_r(NULL, ":", &pstate);
		fprintf(stderr, "portstr = '%s'\n", portstr);
		if (portstr)
			port = atoi(portstr);
		else
			port = 5431;
		fprintf(stderr, "port = %d\n", port);

		if (!sock_dir) {
			conns[connum].host = strdup(host);
		} else {
			conns[connum].host = NULL;
		}
		conns[connum].port = port;
		connum++;

		hostport = strtok_r(NULL, ",", &hstate);
	}

	dtm_unix_sock_dir = sock_dir;
}

static DTMConn GetConnection()
{
	int tries = 3 * connum;
	while (!connected && (tries > 0))
	{
		DTMConn c = conns + leader;
		if (DtmConnect(c))
		{
			xid_t results[RESULTS_SIZE];
			int reslen;
			if (!dtm_send_command(c, CMD_HELLO, 0))
			{
				tries--;
				continue;
			}
			reslen = dtm_recv_results(c, RESULTS_SIZE, results);
			if ((reslen < 1) || (results[0] != RES_OK))
			{
				tries--;
				continue;
			}
		}
		else
		{
			int timeout_ms = 1000;
			struct timespec timeout = {0, timeout_ms * 1000000};
			nanosleep(&timeout, NULL);

			tries--;
			if (c->host)
			{
				elog(WARNING, "Failed to connect to DTMD at tcp %s:%d", c->host, c->port);
			}
			else
			{
				elog(WARNING, "Failed to connect to DTMD at unix %d", c->port);
			}
		}
	}
	if (!tries)
	{
		return NULL;
	}
	return conns + leader;
}

void DtmInitSnapshot(Snapshot snapshot)
{
	#ifdef TEST
	if (snapshot->xip == NULL)
	{
		snapshot->xip = malloc(snapshot->xcnt * sizeof(TransactionId));
	}
	#else
	if (snapshot->xip == NULL)
	{
		/*
		 * First call for this snapshot. Snapshot is same size whether or not
		 * we are in recovery, see later comments.
		 */
		snapshot->xip = (TransactionId *)
			malloc(GetMaxSnapshotSubxidCount() * sizeof(TransactionId));
		if (snapshot->xip == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		Assert(snapshot->subxip == NULL);
		snapshot->subxip = (TransactionId *)
			malloc(GetMaxSnapshotSubxidCount() * sizeof(TransactionId));
		if (snapshot->subxip == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
	}
	#endif
}

TransactionId DtmGlobalStartTransaction(Snapshot snapshot, TransactionId *gxmin)
{
	int i;
	xid_t xid;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();
	if (!dtm) {
		goto failure;
	}

	assert(snapshot != NULL);

	// command
	if (!dtm_send_command(dtm, CMD_BEGIN, 0)) goto failure;

	// results
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen < 5) goto failure;
	if (results[0] != RES_OK) goto failure;
	xid = results[1];
	*gxmin = results[2];

	DtmInitSnapshot(snapshot);
	snapshot->xmin = results[3];
	snapshot->xmax = results[4];
	snapshot->xcnt = reslen - 5;

	for (i = 0; i < snapshot->xcnt; i++)
	{
		snapshot->xip[i] = results[5 + i];
	}

	return xid;
failure:
	DiscardConnection();
	fprintf(stderr, "DtmGlobalStartTransaction: transaction failed to start\n");
	return INVALID_XID;
}

void DtmGlobalGetSnapshot(TransactionId xid, Snapshot snapshot, TransactionId *gxmin)
{
	int i;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();
	if (!dtm) {
		goto failure;
	}

	assert(snapshot != NULL);

	// command
	if (!dtm_send_command(dtm, CMD_SNAPSHOT, 1, xid)) goto failure;

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen < 4) goto failure;
	if (results[0] != RES_OK) goto failure;
	*gxmin = results[1];
	DtmInitSnapshot(snapshot);
	snapshot->xmin = results[2];
	snapshot->xmax = results[3];
	snapshot->xcnt = reslen - 4;

	for (i = 0; i < snapshot->xcnt; i++)
	{
		snapshot->xip[i] = results[4 + i];
	}

	return;
failure:
	DiscardConnection();
	elog(ERROR,
		"DtmGlobalGetSnapshot: failed to"
		" get the snapshot for xid = %d\n",
		xid
	);
}

XidStatus DtmGlobalSetTransStatus(TransactionId xid, XidStatus status, bool wait)
{
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();
	if (!dtm) {
		goto failure;
	}

	switch (status)
	{
		case TRANSACTION_STATUS_COMMITTED:
			if (!dtm_send_command(dtm, CMD_FOR, 2, xid, wait)) goto failure;
			break;
		case TRANSACTION_STATUS_ABORTED:
			if (!dtm_send_command(dtm, CMD_AGAINST, 2, xid, wait)) goto failure;
			break;
		default:
			assert(false); // should not happen
			goto failure;
	}

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 1) goto failure;
	switch (results[0])
	{
		case RES_TRANSACTION_COMMITTED:
			return TRANSACTION_STATUS_COMMITTED;
		case RES_TRANSACTION_ABORTED:
			return TRANSACTION_STATUS_ABORTED;
		case RES_TRANSACTION_INPROGRESS:
			return TRANSACTION_STATUS_IN_PROGRESS;
		default:
			goto failure;
	}
failure:
	DiscardConnection();
	fprintf(
		stderr,
		"DtmGlobalSetTransStatus: failed to vote"
		" %s the transaction xid = %d\n",
		(status == TRANSACTION_STATUS_COMMITTED) ? "for" : "against",
		xid
	);
	return -1;
}

XidStatus DtmGlobalGetTransStatus(TransactionId xid, bool wait)
{
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();
	if (!dtm) {
		goto failure;
	}

	// command
	if (!dtm_send_command(dtm, CMD_STATUS, 2, xid, wait)) goto failure;

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 1) goto failure;
	switch (results[0])
	{
		case RES_TRANSACTION_COMMITTED:
			return TRANSACTION_STATUS_COMMITTED;
		case RES_TRANSACTION_ABORTED:
			return TRANSACTION_STATUS_ABORTED;
		case RES_TRANSACTION_INPROGRESS:
			return TRANSACTION_STATUS_IN_PROGRESS;
		case RES_TRANSACTION_UNKNOWN:
			return TRANSACTION_STATUS_UNKNOWN;
		default:
			goto failure;
	}
failure:
	DiscardConnection();
	fprintf(
		stderr,
		"DtmGlobalGetTransStatus: failed to get"
		" the status of xid = %d\n",
		xid
	);
	return -1;
}

int DtmGlobalReserve(TransactionId xid, int nXids, TransactionId *first)
{
	xid_t xmin, xmax;
	int count;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();
	if (!dtm) {
		goto failure;
	}

	// command
	if (!dtm_send_command(dtm, CMD_RESERVE, 2, xid, nXids)) goto failure;

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 3) goto failure;
	if (results[0] != RES_OK) goto failure;
	xmin = results[1];
	xmax = results[2];

	*first = xmin;
	count = xmax - xmin + 1;
	Assert(*first >= xid);
	Assert(count >= nXids);
	return count;
failure:
	DiscardConnection();
	fprintf(
		stderr,
		"DtmGlobalReserve: failed to reserve"
		" %d transaction starting from = %d\n",
		nXids, xid
	);
	return 0;
}

bool DtmGlobalDetectDeadLock(int port, TransactionId xid, void* data, int size)
{
	int msg_size = size + sizeof(xid)*3;
	int data_size = sizeof(ShubMessageHdr) + msg_size;
	char* buf = (char*)malloc(data_size);
	ShubMessageHdr* msg = (ShubMessageHdr*)buf;
	xid_t* body = (xid_t*)(msg+1);
	int sent;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

	msg->chan = 0;
	msg->code = MSG_FIRST_USER_CODE;
	msg->size = msg_size;

	*body++ = CMD_DEADLOCK;
	*body++ = port;
	*body++ = xid;
	memcpy(body, data, size);

	sent = 0;
	while (sent < data_size)
	{
		int new_bytes = write(dtm->sock, buf + sent, data_size - sent);
		if (new_bytes == -1)
		{
			elog(ERROR, "Failed to send a command to arbiter");
			return false;
		}
		sent += new_bytes;
	}

	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 1 || (results[0] != RES_OK && results[0] != RES_DEADLOCK))
	{
		fprintf(
			stderr,
			"DtmGlobalDetectDeadLock: failed"
			" to check xid=%u for deadlock\n",
			xid
		);
		return false;
	}
	return results[0] == RES_DEADLOCK;
}
