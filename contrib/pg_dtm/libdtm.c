#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "libdtm.h"

#ifdef TEST
// standalone test without postgres functions
#define palloc malloc
#define pfree free
#endif

typedef struct DTMConnData *DTMConn;

typedef struct DTMConnData
{
	int sock;
} DTMConnData;

static char *dtmhost = NULL;
static int dtmport = 0;
static char* dtm_unix_sock_dir;

typedef unsigned long long xid_t;

// Returns true if the write was successful.
static bool dtm_write_char(DTMConn dtm, char c)
{
	return write(dtm->sock, &c, 1) == 1;
}

// Returns true if the read was successful.
static bool dtm_read_char(DTMConn dtm, char *c)
{
	return read(dtm->sock, c, 1) == 1;
}

// // Returns true if the write was successful.
// static bool dtm_write_bool(DTMConn dtm, bool b)
// {
// 	return dtm_write_char(dtm, b ? '+' : '-');
// }

// Returns true if the read was successful.
static bool dtm_read_bool(DTMConn dtm, bool *b)
{
	char c = '\0';
	if (dtm_read_char(dtm, &c))
	{
		if (c == '+')
		{
			*b = true;
			return true;
		}
		if (c == '-')
		{
			*b = false;
			return true;
		}
	}
	return false;
}

// Returns true if the write was successful.
static bool dtm_write_hex16(DTMConn dtm, xid_t i)
{
	char buf[17];
	if (snprintf(buf, 17, "%016llx", i) != 16)
	{
		return false;
	}
	return write(dtm->sock, buf, 16) == 16;
}

// Returns true if the read was successful.
static bool dtm_read_hex16(DTMConn dtm, xid_t *i)
{
	char buf[16];
	if (read(dtm->sock, buf, 16) != 16)
	{
		return false;
	}
	if (sscanf(buf, "%016llx", i) != 1)
	{
		return false;
	}
	return true;
}

// Returns true if the read was successful.
static bool dtm_read_snapshot(DTMConn dtm, Snapshot s)
{
	int i;
	xid_t number;

	if (!dtm_read_hex16(dtm, &number)) return false;
	s->xmin = number;
	Assert(s->xmin == number); // the number should fits into xmin field size

	if (!dtm_read_hex16(dtm, &number)) return false;
	s->xmax = number;
	Assert(s->xmax == number); // the number should fit into xmax field size

	if (!dtm_read_hex16(dtm, &number)) return false;
	s->xcnt = number;
	Assert(s->xcnt == number); // the number should definitely fit into xcnt field size

	DtmInitSnapshot(s);

	for (i = 0; i < s->xcnt; i++)
	{
		if (!dtm_read_hex16(dtm, &number)) return false;
		s->xip[i] = number;
		Assert(s->xip[i] == number); // the number should fit into xip[i] size
	}

	return true;
}

// Returns true if the read was successful.
static bool dtm_read_status(DTMConn dtm, XidStatus *s)
{
	char statuschar;
	if (!dtm_read_char(dtm, &statuschar))
	{
		fprintf(stderr, "dtm_read_status: failed to get char\n");
		return false;
	}

	switch (statuschar)
	{
		case '0':
			*s =  TRANSACTION_STATUS_UNKNOWN;
			break;
		case 'c':
			*s =  TRANSACTION_STATUS_COMMITTED;
			break;
		case 'a':
			*s =  TRANSACTION_STATUS_ABORTED;
			break;
		case '?':
			*s =  TRANSACTION_STATUS_IN_PROGRESS;
			break;
		default:
			fprintf(stderr, "dtm_read_status: unexpected char '%c'\n", statuschar);
			return false;
	}
	return true;
}

// Connects to the specified DTM.
static DTMConn DtmConnect(char *host, int port)
{
	DTMConn dtm;
	int sd;

	if (strcmp(host, "localhost") == 0)
	{
		struct sockaddr sock;
		int len = offsetof(struct sockaddr, sa_data) + snprintf(sock.sa_data, sizeof(sock.sa_data), "%s/p%u", dtm_unix_sock_dir, port);
		sock.sa_family = AF_UNIX;

		sd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sd == -1)
		{
			perror("failed to create a unix socket");
		}
		if (connect(sd, &sock, len) == -1)
		{
			perror("failed to connect to the address");
			close(sd);
			return NULL;
		}
		dtm = malloc(sizeof(DTMConnData));
		dtm->sock = sd;
		return dtm;
	}
	else
	{
		struct addrinfo *addrs = NULL;
		struct addrinfo hint;
		char portstr[6];
		struct addrinfo *a;

		memset(&hint, 0, sizeof(hint));
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_family = AF_INET;
		snprintf(portstr, 6, "%d", port);
		hint.ai_protocol = getprotobyname("tcp")->p_proto;
		if (getaddrinfo(host, portstr, &hint, &addrs))
		{
			perror("failed to resolve address");
			return NULL;
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
			dtm = malloc(sizeof(DTMConnData));
			dtm->sock = sd;
			return dtm;
		}
		freeaddrinfo(addrs);
	}
	fprintf(stderr, "could not connect\n");
	return NULL;
}

/*
// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
static void DtmDisconnect(DTMConn dtm)
{
	close(dtm->sock);
	free(dtm);
}
*/

static bool dtm_query(DTMConn dtm, char cmd, int argc, ...)
{
	va_list argv;
	int i;

	if (!dtm_write_char(dtm, cmd)) return false;
	if (!dtm_write_hex16(dtm, argc)) return false;

	va_start(argv, argc);
	for (i = 0; i < argc; i++)
	{
		xid_t arg = va_arg(argv, xid_t);
		if (!dtm_write_hex16(dtm, arg))
		{
			va_end(argv);
			return false;
		}
	}
	va_end(argv);

	return true;
}

void DtmGlobalConfig(char *host, int port, char* sock_dir) {
	if (dtmhost) {
		free(dtmhost);
		dtmhost = NULL;
	}
	dtmhost = strdup(host);
	dtmport = port;
	dtm_unix_sock_dir = sock_dir;
}

static DTMConn GetConnection()
{
	static DTMConn dtm = NULL;
	if (dtm == NULL)
	{
		if (dtmhost) {
			dtm = DtmConnect(dtmhost, dtmport);
			if (dtm == NULL)
			{
				elog(ERROR, "Failed to connect to DTMD %s:%d", dtmhost, dtmport);
			}
		} else {
			/* elog(ERROR, "DTMD address not specified"); */
		}
	}
	return dtm;
}

void DtmInitSnapshot(Snapshot snapshot) 
{
	#ifdef TEST
	if (snapshot->xip == NULL)
	{
		snapshot->xip = malloc(snapshot->xcnt * sizeof(TransactionId));
		// FIXME: is this enough for tests?
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

// Starts a new global transaction of nParticipants size. Returns the
// transaction id, fills the 'snapshot' and 'gxmin' on success. 'gxmin' is the
// smallest xmin among all snapshots known to DTM. Returns INVALID_XID
// otherwise.
TransactionId DtmGlobalStartTransaction(Snapshot snapshot, TransactionId *gxmin)
{
	bool ok;
	xid_t xid;
	xid_t number;
	DTMConn dtm = GetConnection();

	// query
	if (!dtm_query(dtm, 'b', 0)) goto failure;

	// response
	if (!dtm_read_bool(dtm, &ok)) goto failure;
	if (!ok) goto failure;
	if (!dtm_read_hex16(dtm, &xid)) goto failure;
	if (!dtm_read_hex16(dtm, &number)) goto failure;
	*gxmin = number;
	if (!dtm_read_snapshot(dtm, snapshot)) goto failure;

	return xid;
failure:
	fprintf(stderr, "DtmGlobalStartTransaction: transaction failed to start\n");
	return INVALID_XID;
}

// Asks the DTM for a fresh snapshot. Fills the 'snapshot' and 'gxmin' on
// success. 'gxmin' is the smallest xmin among all snapshots known to DTM.
void DtmGlobalGetSnapshot(TransactionId xid, Snapshot snapshot, TransactionId *gxmin)
{
	bool ok;
	xid_t number;
	DTMConn dtm = GetConnection();

	assert(snapshot != NULL);

	// query
	if (!dtm_query(dtm, 'h', 1, xid)) goto failure;

	// response
	if (!dtm_read_bool(dtm, &ok)) goto failure;
	if (!ok) goto failure;

	if (!dtm_read_hex16(dtm, &number)) goto failure;
	*gxmin = number;
	if (!dtm_read_snapshot(dtm, snapshot)) goto failure;
	return;
failure:
	fprintf(
		stderr,
		"DtmGlobalGetSnapshot: failed to"
		" get the snapshot for xid = %d\n",
		xid
	);
}

// Commits transaction only once all participants have called this function,
// does not change CLOG otherwise. Set 'wait' to 'true' if you want this call
// to return only after the transaction is considered finished by the DTM.
// Returns the status on success, or -1 otherwise.
XidStatus DtmGlobalSetTransStatus(TransactionId xid, XidStatus status, bool wait)
{
	bool ok;
	XidStatus actual_status;
	DTMConn dtm = GetConnection();

	switch (status)
	{
		case TRANSACTION_STATUS_COMMITTED:
			// query
			if (!dtm_query(dtm, 'y', 2, xid, wait)) goto failure;
			break;
		case TRANSACTION_STATUS_ABORTED:
			// query
			if (!dtm_query(dtm, 'n', 2, xid, wait)) goto failure;
			break;
		default:
			assert(false); // should not happen
			goto failure;
	}

	if (!dtm_read_bool(dtm, &ok)) goto failure;
	if (!ok) goto failure;

	if (!dtm_read_status(dtm, &actual_status)) goto failure;
	return actual_status;
failure:
	fprintf(
		stderr,
		"DtmGlobalSetTransStatus: failed to vote"
		" %s the transaction xid = %d\n",
		(status == TRANSACTION_STATUS_COMMITTED) ? "for" : "against",
		xid
	);
	return -1;
}

// Gets the status of the transaction identified by 'xid'. Returns the status
// on success, or -1 otherwise. If 'wait' is true, then it does not return
// until the transaction is finished.
XidStatus DtmGlobalGetTransStatus(TransactionId xid, bool wait)
{
	bool ok;
	XidStatus s;
	DTMConn dtm = GetConnection();

	if (!dtm_query(dtm, 's', 2, xid, wait)) goto failure;

	if (!dtm_read_bool(dtm, &ok)) goto failure;
	if (!ok) return -1;

	if (!dtm_read_status(dtm, &s)) goto failure;
	return s;
failure:
	fprintf(
		stderr,
		"DtmGlobalGetTransStatus: failed to get"
		" the status of xid = %d\n",
		xid
	);
	return -1;
}

// Reserves at least 'nXids' successive xids for local transactions. The xids
// reserved are not less than 'xid' in value. Returns the actual number of xids
// reserved, and sets the 'first' xid accordingly. The number of xids reserved
// is guaranteed to be at least nXids.
// In other words, *first ≥ xid and result ≥ nXids.
int DtmGlobalReserve(TransactionId xid, int nXids, TransactionId *first)
{
	bool ok;
	xid_t xmin, xmax;
	int count;
	DTMConn dtm = GetConnection();

	if (!dtm_query(dtm, 'r', 2, xid, nXids)) goto failure;

	if (!dtm_read_bool(dtm, &ok)) goto failure;
	if (!ok) goto failure;

	if (!dtm_read_hex16(dtm, &xmin)) goto failure;
	if (!dtm_read_hex16(dtm, &xmax)) goto failure;

	*first = xmin;
	count = xmax - xmin + 1;
	Assert(*first >= xid);
	Assert(count >= nXids);
	return count;
failure:
	fprintf(
		stderr,
		"DtmGlobalReserve: failed to reserve"
		" %d transaction starting from = %d\n",
		nXids, xid
	);
	return 0;
}
