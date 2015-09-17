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

typedef struct DTMConnData {
	int sock;
} DTMConnData;

// Returns true if the write was successful.
static bool dtm_write_char(DTMConn dtm, char c) {
	return write(dtm->sock, &c, 1) == 1;
}

// Returns true if the read was successful.
static bool dtm_read_char(DTMConn dtm, char *c) {
	return read(dtm->sock, c, 1) == 1;
}

// // Returns true if the write was successful.
// static bool dtm_write_bool(DTMConn dtm, bool b) {
// 	return dtm_write_char(dtm, b ? '+' : '-');
// }

// Returns true if the read was successful.
static bool dtm_read_bool(DTMConn dtm, bool *b) {
	char c = '\0';
	if (dtm_read_char(dtm, &c)) {
		if (c == '+') {
			*b = true;
			return true;
		}
		if (c == '-') {
			*b = false;
			return true;
		}
	}
	return false;
}

// Returns true if the write was successful.
static bool dtm_write_hex16(DTMConn dtm, xid_t i) {
	char buf[17];
	if (snprintf(buf, 17, "%016llx", i) != 16) {
		return false;
	}
	return write(dtm->sock, buf, 16) == 16;
}

// Returns true if the read was successful.
static bool dtm_read_hex16(DTMConn dtm, xid_t *i) {
	char buf[16];
	if (read(dtm->sock, buf, 16) != 16) {
		return false;
	}
	if (sscanf(buf, "%016llx", i) != 1) {
		return false;
	}
	return true;
}


// Connects to the specified DTM.
DTMConn DtmConnect(char *host, int port) {
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;
	if (getaddrinfo(host, portstr, &hint, &addrs)) {
		perror("resolve address");
		return NULL;
	}

	for (a = addrs; a != NULL; a = a->ai_next) {
		DTMConn dtm;
		int sock = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
		if (sock == -1) {
			perror("failed to create a socket");
			continue;
		}

		#ifdef NODELAY
		int one = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
		#endif

		if (connect(sock, a->ai_addr, a->ai_addrlen) == -1) {
			perror("failed to connect to an address");
			close(sock);
			continue;
		}

		// success
		freeaddrinfo(addrs);
		dtm = malloc(sizeof(DTMConnData));
		dtm->sock = sock;
		return dtm;
	}

	freeaddrinfo(addrs);
	fprintf(stderr, "could not connect\n");
	return NULL;
}

// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
void DtmDisconnect(DTMConn dtm) {
	close(dtm->sock);
	free(dtm);
}

static bool dtm_query(DTMConn dtm, char cmd, int argc, ...) {
	va_list argv;
	int i;

	if (!dtm_write_char(dtm, cmd)) return false;
	if (!dtm_write_hex16(dtm, argc)) return false;

	va_start(argv, argc);
	for (i = 0; i < argc; i++) {
		xid_t arg = va_arg(argv, xid_t);
		if (!dtm_write_hex16(dtm, arg)) {
			va_end(argv);
			return false;
		}
	}
	va_end(argv);

	return true;
}

// Creates an entry for a new global transaction. Returns 'true' on success, or
// 'false' otherwise.
bool DtmGlobalStartTransaction(DTMConn dtm, GlobalTransactionId *gtid) {
	bool ok;
	int i;

	// query
	if (!dtm_write_char(dtm, 'b')) return false;
	if (!dtm_write_hex16(dtm, 1 + 2 * gtid->nNodes)) return false;
	if (!dtm_write_hex16(dtm, gtid->nNodes)) return false;
	for (i = 0; i < gtid->nNodes; i++) {
		if (!dtm_write_hex16(dtm, gtid->nodes[i])) return false;
		if (!dtm_write_hex16(dtm, gtid->xids[i])) return false;
	}

	// response
	if (!dtm_read_bool(dtm, &ok)) return false;
	return ok;
}

void DtmInitSnapshot(Snapshot snapshot) 
{
	#ifdef TEST
	if (snapshot->xip == NULL) {
		snapshot->xip = malloc(snapshot->xcnt * sizeof(TransactionId));
		// FIXME: is this enough for tests?
	}
	#else
	if (snapshot->xip == NULL) {
		/*
		 * First call for this snapshot. Snapshot is same size whether or not
		 * we are in recovery, see later comments.
		 */
		snapshot->xip = (TransactionId *)
			malloc(GetMaxSnapshotXidCount() * sizeof(TransactionId));
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

// Asks DTM for a fresh snapshot. Returns 'true' on success, or 'false'
// otherwise.
bool DtmGlobalGetSnapshot(DTMConn dtm, NodeId nodeid, TransactionId xid, Snapshot s) {
	bool ok;
	int i;
	xid_t number;

	assert(s != NULL);

	// query
	if (!dtm_query(dtm, 'h', 2, nodeid, xid)) return false;

	// response
	if (!dtm_read_bool(dtm, &ok)) return false;
	if (!ok) return false;

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

	for (i = 0; i < s->xcnt; i++) {
		if (!dtm_read_hex16(dtm, &number)) return false;
		s->xip[i] = number;
		Assert(s->xip[i] == number); // the number should fit into xip[i] size
	}

	//fprintf(stdout, "snapshot: xmin = %#x, xmax = %#x, active =", s->xmin, s->xmax);
	//for (i = 0; i < s->xcnt; i++) {
	//	fprintf(stdout, " %#x", s->xip[i]);
	//}
	//fprintf(stdout, "\n");

	return true;
}

// Commits transaction only once all participants have called this function,
// does not change CLOG otherwise. Returns 'true' on success, 'false' if
// something failed on the daemon side.
bool DtmGlobalSetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, XidStatus status, bool wait)
{
	bool ok;
	switch (status) {
		case TRANSACTION_STATUS_COMMITTED:
			// query
			if (!dtm_query(dtm, 'c', 3, nodeid, xid, wait)) return false;
			break;
		case TRANSACTION_STATUS_ABORTED:
			// query
			if (wait) {
				fprintf(stderr, "'wait' is ignored for aborts\n");
			}
			if (!dtm_query(dtm, 'a', 2, nodeid, xid)) return false;
			break;
		default:
			assert(false); // should not happen
			return false;
	}

	if (!dtm_read_bool(dtm, &ok)) return false;

	return ok;
}

// Gets the status of the transaction identified by 'xid'. Returns the status
// on success, or -1 otherwise. If 'wait' is true, then it does not return
// until the transaction is finished.
XidStatus DtmGlobalGetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, bool wait) {
	bool result;
	char statuschar;

	if (!dtm_query(dtm, 's', 3, nodeid, xid, wait)) return -1;

	if (!dtm_read_bool(dtm, &result)) return -1;
	if (!result) return -1;
	if (!dtm_read_char(dtm, &statuschar)) return -1;

	switch (statuschar) {
		case '0':
			return TRANSACTION_STATUS_UNKNOWN;
		case 'c':
			return TRANSACTION_STATUS_COMMITTED;
		case 'a':
			return TRANSACTION_STATUS_ABORTED;
		case '?':
			return TRANSACTION_STATUS_IN_PROGRESS;
		default:
			return -1;
	}
}
