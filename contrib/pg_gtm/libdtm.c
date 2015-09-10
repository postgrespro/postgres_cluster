#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <netinet/tcp.h>
#endif

#include "libdtm.h"

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
static bool dtm_write_hex16(DTMConn dtm, cid_t i) {
	char buf[17];
	if (snprintf(buf, 17, "%016llx", i) != 16) {
		return false;
	}
	return write(dtm->sock, buf, 16) == 16;
}

// Returns true if the read was successful.
static bool dtm_read_hex16(DTMConn dtm, cid_t *i) {
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

		int one = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

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

// Asks DTM for the first 'gcid' with unknown status. This 'gcid' is used as a
// kind of snapshot. Returns the 'gcid' on success, or INVALID_GCID otherwise.
cid_t DtmGlobalGetNextCid(DTMConn dtm) {
	bool ok;
	cid_t gcid;

	if (!dtm_write_char(dtm, 'h')) {
		return INVALID_GCID;
	}

	if (!dtm_read_bool(dtm, &ok)) {
		return INVALID_GCID;
	}
	if (!ok) {
		return INVALID_GCID;
	}

	if (!dtm_read_hex16(dtm, &gcid)) {
		return INVALID_GCID;
	}

	return gcid;
}

// Prepares a commit. Returns the 'gcid' on success, or INVALID_GCID otherwise.
cid_t DtmGlobalPrepare(DTMConn dtm) {
	bool ok;
	cid_t gcid;

	if (!dtm_write_char(dtm, 'p')) {
		return INVALID_GCID;
	}

	if (!dtm_read_bool(dtm, &ok)) {
		return INVALID_GCID;
	}
	if (!ok) {
		return INVALID_GCID;
	}

	if (!dtm_read_hex16(dtm, &gcid)) {
		return INVALID_GCID;
	}

	return gcid;
}

// Finishes a given commit with 'committed' status. Returns 'true' on success,
// 'false' otherwise.
bool DtmGlobalCommit(DTMConn dtm, cid_t gcid) {
	bool result;

	if (!dtm_write_char(dtm, 'c')) {
		return false;
	}

	if (!dtm_write_hex16(dtm, gcid)) {
		return false;
	}

	if (!dtm_read_bool(dtm, &result)) {
		return false;
	}

	return result;
}

// Finishes a given commit with 'aborted' status.
void DtmGlobalRollback(DTMConn dtm, cid_t gcid) {
	bool result;

	if (!dtm_write_char(dtm, 'a')) {
		return;
	}

	if (!dtm_write_hex16(dtm, gcid)) {
		return;
	}

	if (!dtm_read_bool(dtm, &result)) {
		return;
	}
}

// Gets the status of the commit identified by 'gcid'. Returns the status on
// success, or -1 otherwise.
int DtmGlobalGetTransStatus(DTMConn dtm, cid_t gcid) {
	bool result;
	char statuschar;

	if (!dtm_write_char(dtm, 's')) {
		return -1;
	}

	if (!dtm_write_hex16(dtm, gcid)) {
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
