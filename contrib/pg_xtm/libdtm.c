#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "libdtm.h"

#if 0
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

// Asks DTM for a fresh snapshot. Returns a snapshot on success, or NULL
// otherwise. Please free the snapshot memory yourself after use.
Snapshot DtmGlobalGetSnapshot(DTMConn dtm, NodeId nodeid, TransactionId xid, Snapshot s) {
	bool ok;
	int i;
	xid_t number;

	if (!dtm_write_char(dtm, 'h')) {
		return NULL;
	}

	if (!dtm_read_bool(dtm, &ok)) {
		return NULL;
	}
	if (!ok) {
		return NULL;
	}

	if (!dtm_read_hex16(dtm, &number)) {
		goto cleanup_snapshot;
	}
	s->xmin = number;
	Assert(s->xmin == number); // the number should fits into xmin field size

	if (!dtm_read_hex16(dtm, &number)) {
		goto cleanup_snapshot;
	}
	s->xmax = number;
	Assert(s->xmax == number); // the number should fit into xmax field size

	if (!dtm_read_hex16(dtm, &number)) {
		goto cleanup_snapshot;
	}
	s->xcnt = number;
	Assert(s->xcnt == number); // the number should definitely fit into xcnt field size

	s->xip = palloc(s->xcnt * sizeof(TransactionId));
	for (i = 0; i < s->xcnt; i++) {
		if (!dtm_read_hex16(dtm, &number)) {
			goto cleanup_active_list;
		}
		s->xip[i] = number;
		Assert(s->xip[i] == number); // the number should fit into xip[i] size
	}

	return s;

cleanup_active_list:
	pfree(s->xip);
cleanup_snapshot:
	pfree(s);
	return NULL;
}

#if 0
// Starts a transaction. Returns the 'gxid' on success, or INVALID_GXID otherwise.
xid_t DtmGlobalBegin(DTMConn dtm) {
	bool ok;
	xid_t gxid;

	if (!dtm_write_char(dtm, 'b')) {
		return INVALID_GXID;
	}

	if (!dtm_read_bool(dtm, &ok)) {
		return INVALID_GXID;
	}
	if (!ok) {
		return INVALID_GXID;
	}

	if (!dtm_read_hex16(dtm, &gxid)) {
		return INVALID_GXID;
	}

	return gxid;
}
#endif

void DtmGlobalSetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, XidStatus status)
{
}
#if 0
// Marks a given transaction as 'committed'. Returns 'true' on success,
// 'false' otherwise.
bool DtmGlobalCommit(DTMConn dtm, xid_t gxid) {
	bool result;

	if (!dtm_write_char(dtm, 'c')) {
		return false;
	}

	if (!dtm_write_hex16(dtm, gxid)) {
		return false;
	}

	if (!dtm_read_bool(dtm, &result)) {
		return false;
	}

	return result;
}

// Marks a given transaction as 'aborted'.
void DtmGlobalRollback(DTMConn dtm, xid_t gxid) {
	bool result;

	if (!dtm_write_char(dtm, 'a')) {
		return;
	}

	if (!dtm_write_hex16(dtm, gxid)) {
		return;
	}

	if (!dtm_read_bool(dtm, &result)) {
		return;
	}
}
#endif

// Gets the status of the transaction identified by 'gxid'. Returns the status
// on success, or -1 otherwise.
XidStatus DtmGlobalGetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId gxid) {
	bool result;
	char statuschar;

	if (!dtm_write_char(dtm, 's')) {
		return -1;
	}

	if (!dtm_write_hex16(dtm, gxid)) {
		return -1;
	}

	if (!dtm_read_bool(dtm, &result)) {
		return -1;
	}
	if (!result) {
		return -1;
	}

	if (!dtm_read_char(dtm, &statuschar)) {
		return -1;
	}

	switch (statuschar) {
		case 'c':
			return COMMIT_YES;
		case 'a':
			return COMMIT_NO;
		case '?':
			return COMMIT_UNKNOWN;
		default:
			return -1;
	}
}
