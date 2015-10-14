#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "libdtm.h"
#include "dtmd/include/proto.h"
#include "sockhub/sockhub.h"

#ifdef TEST
// standalone test without postgres functions
#define palloc malloc
#define pfree free
#endif

#define COMMAND_BUFFER_SIZE 1024
#define RESULTS_SIZE 1024 // size in 64-bit numbers

typedef struct DTMConnData *DTMConn;

typedef struct DTMConnData
{
	int sock;
} DTMConnData;

static char *dtmhost = NULL;
static int dtmport = 0;
static char* dtm_unix_sock_dir;

typedef unsigned xid_t;

// Connects to the specified DTM.
static DTMConn DtmConnect(char *host, int port)
{
	DTMConn dtm;
	int sd;

	if (host == NULL)
	{
		// use a UNIX socket
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
		// use an IP socket
		struct addrinfo *addrs = NULL;
		struct addrinfo hint;
		char portstr[6];
		struct addrinfo *a;

		memset(&hint, 0, sizeof(hint));
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_family = AF_INET;
		snprintf(portstr, 6, "%d", port);
		hint.ai_protocol = getprotobyname("tcp")->p_proto;

        while (1) {
            char* sep = strchr(host, ',');
            if (sep != NULL) {
                *sep = '\0';
            }
            if (getaddrinfo(host, portstr, &hint, &addrs))
            {
                perror("failed to resolve address");
                if (sep == NULL) { 
                    return NULL;
                } else { 
                    goto TryNextHost;
                }
            }

            for (a = addrs; a != NULL; a = a->ai_next)
            {
                int one = 1;
                sd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
                if (sd == -1)
                {
                    perror("failed to create a socket");
                    goto TryNextHost;
                }
                setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
                
                if (connect(sd, a->ai_addr, a->ai_addrlen) == -1)
                {
                    perror("failed to connect to an address");
                    close(sd);
                    goto TryNextHost;
                }
                
                // success
                freeaddrinfo(addrs);
                dtm = malloc(sizeof(DTMConnData));
                dtm->sock = sd;
                if (sep != NULL) {
                    *sep = ',';
                }
                return dtm;
            }
            freeaddrinfo(addrs);
          TryNextHost:
            if (sep == NULL) { 
                break;
            }
            *sep = ',';
            host = sep + 1;
        }
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

static int dtm_recv_results(DTMConn dtm, int maxlen, xid_t *results) {
	ShubMessageHdr msg;
	int recved;
	int needed;

	recved = 0;
	needed = sizeof(ShubMessageHdr);
	while (recved < needed) {
		int newbytes = read(dtm->sock, (char*)&msg + recved, needed - recved);
		if (newbytes == -1) {
			elog(ERROR, "Failed to recv results header from arbiter");
			return 0;
		}
		if (newbytes == 0) {
			elog(ERROR, "Arbiter closed connection during recv");
			return 0;
		}
		recved += newbytes;
	}

	recved = 0;
	needed = msg.size;
	assert(needed % sizeof(xid_t) == 0);
	if (needed > maxlen * sizeof(xid_t)) {
		elog(ERROR, "The message body will not fit into the results array");
		return 0;
	}
	while (recved < needed) {
		int newbytes = read(dtm->sock, (char*)results + recved, needed - recved);
		if (newbytes == -1) {
			elog(ERROR, "Failed to recv results body from arbiter");
			return 0;
		}
		if (newbytes == 0) {
			elog(ERROR, "Arbiter closed connection during recv");
			return 0;
		}
		recved += newbytes;
	}
	return needed / sizeof(xid_t);
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
	while (sent < datasize) {
		int newbytes = write(dtm->sock, buf + sent, datasize - sent);
		if (newbytes == -1) {
			elog(ERROR, "Failed to send a command to arbiter");
			return false;
		}
		sent += newbytes;
	}
	return true;
}

void DtmGlobalConfig(char *host, int port, char* sock_dir) {
	if (dtmhost) {
		free(dtmhost);
		dtmhost = NULL;
	}
	if (host) {
		dtmhost = strdup(host);
	}
	dtmport = port;
	dtm_unix_sock_dir = sock_dir;
}

static DTMConn GetConnection()
{
	static DTMConn dtm = NULL;
	if (dtm == NULL)
	{
		dtm = DtmConnect(dtmhost, dtmport);
		if (dtm == NULL)
		{
			if (dtmhost) {
				elog(ERROR, "Failed to connect to DTMD at tcp %s:%d", dtmhost, dtmport);
			} else {
				elog(ERROR, "Failed to connect to DTMD at unix %d", dtmport);
			}
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
	int i;
	xid_t xid;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

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
	fprintf(stderr, "DtmGlobalStartTransaction: transaction failed to start\n");
	return INVALID_XID;
}

// Asks the DTM for a fresh snapshot. Fills the 'snapshot' and 'gxmin' on
// success. 'gxmin' is the smallest xmin among all snapshots known to DTM.
void DtmGlobalGetSnapshot(TransactionId xid, Snapshot snapshot, TransactionId *gxmin)
{
	int i;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

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
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

	switch (status)
	{
		case TRANSACTION_STATUS_COMMITTED:
			// command
			if (!dtm_send_command(dtm, CMD_FOR, 2, xid, wait)) goto failure;
			break;
		case TRANSACTION_STATUS_ABORTED:
			// command
			if (!dtm_send_command(dtm, CMD_AGAINST, 2, xid, wait)) goto failure;
			break;
		default:
			assert(false); // should not happen
			goto failure;
	}

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 1) goto failure;
	switch (results[0]) {
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
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

	// command
	if (!dtm_send_command(dtm, CMD_STATUS, 2, xid, wait)) goto failure;

	// response
	reslen = dtm_recv_results(dtm, RESULTS_SIZE, results);
	if (reslen != 1) goto failure;
	switch (results[0]) {
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
	xid_t xmin, xmax;
	int count;
	int reslen;
	xid_t results[RESULTS_SIZE];
	DTMConn dtm = GetConnection();

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
	fprintf(
		stderr,
		"DtmGlobalReserve: failed to reserve"
		" %d transaction starting from = %d\n",
		nXids, xid
	);
	return 0;
}
