#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "clog.h"
#include "eventwrap.h"
#include "util.h"
#include "intset.h"
#include "transaction.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

Transaction transactions[MAX_TRANSACTIONS];
int transactions_count;

// We reserve the local xids if they fit between (prev, next) range, and
// reserve something in (next, x) range otherwise, moving 'next' after 'x'.
xid_t prev_gxid, next_gxid;

typedef struct client_data_t {
	int id;
	int snapshots_sent;
	xid_t xid;
} client_data_t;

clog_t clg;

#define CLIENT_ID(X) (((client_data_t*)(X))->id)
#define CLIENT_SNAPSENT(X) (((client_data_t*)(X))->snapshots_sent)
#define CLIENT_XID(X) (((client_data_t*)(X))->xid)

static client_data_t *create_client_data(int id) {
	client_data_t *cd = malloc(sizeof(client_data_t));
	cd->id = id;
	cd->snapshots_sent = 0;
	cd->xid = INVALID_XID;
	return cd;
}

static void free_client_data(client_data_t *cd) {
	free(cd);
}

static int next_client_id = 0;
static void onconnect(void *stream, void **clientdata) {
	*clientdata = create_client_data(next_client_id++);
	debug("[%d] connected\n", CLIENT_ID(*clientdata));
}

static void notify_listeners(Transaction *t, int status) {
	void *listener;
	switch (status) {
		case BLANK:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the committed status
				write_to_stream(listener, strdup("+0"));
			}
			break;
		case NEGATIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the aborted status
				write_to_stream(listener, strdup("+a"));
			}
			break;
		case POSITIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the committed status
				write_to_stream(listener, strdup("+c"));
			}
			break;
		case DOUBT:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the committed status
				write_to_stream(listener, strdup("+?"));
			}
			break;
	}
}

static void ondisconnect(void *stream, void *clientdata) {
	debug("[%d] disconnected\n", CLIENT_ID(clientdata));

	if (CLIENT_XID(clientdata) != INVALID_XID) {
		int i;

		// need to abort the transaction this client is participating in
		for (i = transactions_count - 1; i >= 0; i--) {
			Transaction *t = transactions + i;

			if (t->xid == CLIENT_XID(clientdata)) {
				if (clog_write(clg, t->xid, NEGATIVE)) {
					notify_listeners(t, NEGATIVE);

					*t = transactions[transactions_count - 1];
					transactions_count--;
				} else {
					shout(
						"[%d] DISCONNECT: transaction %llu"
						" failed to abort O_o\n",
						CLIENT_ID(clientdata), t->xid
					);
				}
				break;
			}
		}

		if (i < 0) {
			shout(
				"[%d] DISCONNECT: transaction %llu not found O_o\n",
				CLIENT_ID(clientdata), CLIENT_XID(clientdata)
			);
		}
	}

	free_client_data(clientdata);
}

#ifdef DEBUG
static void debug_cmd(void *clientdata, cmd_t *cmd) {
	char *cmdname;
	switch (cmd->cmd) {
		case CMD_RESERVE : cmdname =  "RESERVE"; break;
		case CMD_BEGIN   : cmdname =    "BEGIN"; break;
		case CMD_FOR     : cmdname =      "FOR"; break;
		case CMD_AGAINST : cmdname =  "AGAINST"; break;
		case CMD_SNAPSHOT: cmdname = "SNAPSHOT"; break;
		case CMD_STATUS  : cmdname =   "STATUS"; break;
		default          : cmdname =  "unknown";
	}
	debug("[%d] %s", CLIENT_ID(clientdata), cmdname);
	int i;
	for (i = 0; i < cmd->argc; i++) {
		debug(" %llu", cmd->argv[i]);
	}
	debug("\n");
}
#else
#define debug_cmd(...)
#endif

#define CHECK(COND, CDATA, MSG) \
	do { \
		if (!(COND)) { \
			shout("[%d] %s, returning '-'\n", CLIENT_ID(CDATA), MSG); \
			return strdup("-"); \
		} \
	} while (0)

static xid_t max(xid_t a, xid_t b) {
	return a > b ? a : b;
}

static void gen_snapshot(Snapshot *s) {
	s->times_sent = 0;
	s->nactive = 0;
	s->xmin = MAX_XID;
	s->xmax = MIN_XID;
	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions + i;
		if (t->xid < s->xmin) {
			s->xmin = t->xid;
		}
		if (t->xid >= s->xmax) { 
			s->xmax = t->xid + 1;
		}
		s->active[s->nactive++] = t->xid;
	}
	if (s->nactive > 0) {
		assert(s->xmin < MAX_XID);
		assert(s->xmax > MIN_XID);
		assert(s->xmin <= s->xmax);
		snapshot_sort(s);
	} else {
		s->xmin = s->xmax = 0;
	}
}

static char *onreserve(void *stream, void *clientdata, cmd_t *cmd) {
	CHECK(
		cmd->argc == 2,
		clientdata,
		"RESERVE: wrong number of arguments"
	);

	xid_t minxid = cmd->argv[0];
	int minsize = cmd->argv[1];
	xid_t maxxid = minxid + minsize - 1;

	debug(
		"[%d] RESERVE: asked for range %llu-%llu\n",
		CLIENT_ID(clientdata),
		minxid, maxxid
	);

	if ((prev_gxid >= minxid) || (maxxid >= next_gxid)) {
		debug(
			"[%d] RESERVE: local range %llu-%llu is not between global range %llu-%llu\n",
			CLIENT_ID(clientdata),
			minxid, maxxid,
			prev_gxid, next_gxid
		);

		minxid = max(minxid, next_gxid);
		maxxid = max(maxxid, minxid + minsize - 1);
		next_gxid = maxxid + 1;
	}
	debug(
		"[%d] RESERVE: allocating range %llu-%llu\n",
		CLIENT_ID(clientdata),
		minxid, maxxid
	);

	char head[1+16+16+1];
	sprintf(head, "+%016llx%016llx", minxid, maxxid);

	return strdup(head);
}

static xid_t get_global_xmin() {
	int i, j;
	xid_t xmin = next_gxid;
	Transaction *t;
	for (i = 0; i < transactions_count; i++) {
		t = transactions + i;
		j = t->snapshots_count > MAX_SNAPSHOTS_PER_TRANS ? MAX_SNAPSHOTS_PER_TRANS : t->snapshots_count; 
		while (--j >= 0) { 
			Snapshot* s = transaction_snapshot(t, j);
			if (s->xmin < xmin) {
				xmin = s->xmin;
			}
			// minor TODO: Use 'times_sent' to generate a bit greater xmin?
		}
	}
	return xmin;
}

static char *onbegin(void *stream, void *clientdata, cmd_t *cmd) {
	CHECK(
		transactions_count < MAX_TRANSACTIONS,
		clientdata,
		"BEGIN: transaction limit hit"
	);

	CHECK(
		cmd->argc == 0,
		clientdata,
		"BEGIN: wrong number of arguments"
	);

	CHECK(
		CLIENT_XID(clientdata) == INVALID_XID,
		clientdata,
		"BEGIN: already participating in another transaction"
	);

	Transaction *t = transactions + transactions_count;
	transaction_clear(t);

	prev_gxid = t->xid = next_gxid++;
	t->snapshots_count = 0;
    t->size = 1;

	CLIENT_SNAPSENT(clientdata) = 0;
	CLIENT_XID(clientdata) = t->xid;

	if (!clog_write(clg, t->xid, DOUBT)) {
		shout(
			"[%d] BEGIN: transaction %llu failed"
			" to initialize clog bits O_o\n",
			CLIENT_ID(clientdata), t->xid
		);
		return strdup("-");
	}

	char head[1+16+16+1];
	sprintf(head, "+%016llx%016llx", t->xid, get_global_xmin());

	transactions_count++;

	gen_snapshot(transaction_next_snapshot(t));
	// will wrap around if exceeded max snapshots
	Snapshot *snap = transaction_latest_snapshot(t);
	char *snapser = snapshot_serialize(snap);

	return destructive_concat(strdup(head), snapser);
}

static Transaction *find_transaction(xid_t xid) {
	int i;
	Transaction *t;
	for (i = 0; i < transactions_count; i++) {
		t = transactions + i;
		if (t->xid == xid) {
			return t;
		}
	}
	return NULL;
}

static bool queue_for_transaction_finish(void *stream, void *clientdata, xid_t xid, char cmd) {
	assert((cmd >= 'a') && (cmd <= 'z'));

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] QUEUE: xid %llu not found\n",
			CLIENT_ID(clientdata), xid
		);
		return strdup("-");
	}

	// TODO: Implement deadlock detection here. We have
	// CLIENT_XID(clientdata) and 'xid', i.e. we are able to tell which
	// transaction waits which transaction.

	transaction_push_listener(t, cmd, stream);
	return true;
}

static char *onvote(void *stream, void *clientdata, cmd_t *cmd, int vote) {
	assert((vote == POSITIVE) || (vote == NEGATIVE));

	// Check the arguments
	xid_t xid = cmd->argv[0];
	bool wait = cmd->argv[1];

	CHECK(
//		CLIENT_XID(clientdata) == INVALID_XID ||
		CLIENT_XID(clientdata) == xid,
		clientdata,
		"VOTE: voting for a transaction not participated in"
	);

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] VOTE: xid %llu not found\n",
			CLIENT_ID(clientdata), xid
		);
		return strdup("-");
	}

	if (vote == POSITIVE) {
		t->votes_for += 1;
	} else if (vote == NEGATIVE) {
		t->votes_against += 1;
	} else {
		assert(false); // should not happen
	}
	assert(t->votes_for + t->votes_against <= t->size);

	CLIENT_XID(clientdata) = INVALID_XID; // not participating any more

	switch (transaction_status(t)) {
		case NEGATIVE:
			CHECK(
				clog_write(clg, t->xid, NEGATIVE),
				clientdata,
				"VOTE: transaction failed to abort O_o"
			);

			notify_listeners(t, NEGATIVE);

			*t = transactions[transactions_count - 1];
			transactions_count--;
			return strdup("+a");
		case DOUBT:
			if (wait) {
				CHECK(
					queue_for_transaction_finish(stream, clientdata, xid, 's'),
					clientdata,
					"VOTE: couldn't queue for transaction finish"
				);
				return NULL;
			} else {
				return strdup("+?");
			}
		case POSITIVE:
			CHECK(
				clog_write(clg, t->xid, POSITIVE),
				clientdata,
				"VOTE: transaction failed to commit"
			);

			notify_listeners(t, POSITIVE);

			*t = transactions[transactions_count - 1];
			transactions_count--;
			return strdup("+c");
	}

	assert(false); // a case missed in the switch?
	return strdup("-");
}

static char *onsnapshot(void *stream, void *clientdata, cmd_t *cmd) {
	CHECK(
		cmd->argc == 1,
		clientdata,
		"SNAPSHOT: wrong number of arguments"
	);

	xid_t xid = cmd->argv[0];

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] SNAPSHOT: xid %llu not found\n",
			CLIENT_ID(clientdata), xid
		);
		return strdup("-");
	}

	if (CLIENT_XID(clientdata) == INVALID_XID) {
		CLIENT_SNAPSENT(clientdata) = 0;
		CLIENT_XID(clientdata) = t->xid;
        t->size += 1;
	}

	CHECK(
		CLIENT_XID(clientdata) == t->xid,
		clientdata,
		"SNAPSHOT: getting snapshot for a transaction not participated in"
	);

	assert(CLIENT_SNAPSENT(clientdata) <= t->snapshots_count); // who sent an inexistent snapshot?!

	if (CLIENT_SNAPSENT(clientdata) == t->snapshots_count) {
		// a fresh snapshot is needed
		gen_snapshot(transaction_next_snapshot(t));
	}

	char head[1+16+1];
	sprintf(head, "+%016llx", get_global_xmin());

	Snapshot *snap = transaction_snapshot(t, CLIENT_SNAPSENT(clientdata)++);
	snap->times_sent += 1;
	char *snapser = snapshot_serialize(snap);

	// FIXME: Remote this assert if you do not have a barrier upon getting
	// snapshot in backends. The assert should indicate that situation :)
	// assert(CLIENT_SNAPSENT(clientdata) == t->snapshots_count);

	return destructive_concat(strdup(head), snapser);
}

static char *onstatus(void *stream, void *clientdata, cmd_t *cmd) {
	if (cmd->argc != 2) {
		shout(
			"[%d] STATUS: wrong number of arguments %d, expected %d\n",
			CLIENT_ID(clientdata), cmd->argc, 3
		);
		return strdup("-");
	}
	xid_t xid = cmd->argv[0];
	bool wait = cmd->argv[1];

	int status = clog_read(clg, xid);
	switch (status) {
		case BLANK:
			return strdup("+0");
		case POSITIVE:
			return strdup("+c");
		case NEGATIVE:
			return strdup("+a");
		case DOUBT:
			if (wait) {
				if (!queue_for_transaction_finish(stream, clientdata, xid, 's')) {
					shout(
						"[%d] STATUS: couldn't queue for transaction finish\n",
						CLIENT_ID(clientdata)
					);
					return strdup("-");
				}
				return NULL;
			} else {
				return strdup("+?");
			}
		default:
			assert(false); // should not happen
			return strdup("-");
	}
}

static char *onnoise(void *stream, void *clientdata, cmd_t *cmd) {
	shout(
		"[%d] NOISE: unknown command '%c'\n",
		CLIENT_ID(clientdata),
		cmd->cmd
	);
	return strdup("-");
}

// static float now_s() {
// 	// current time in seconds
// 	struct timespec t;
// 	if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
// 		return t.tv_sec + t.tv_nsec * 1e-9;
// 	} else {
// 		printf("Error while clock_gettime()\n");
// 		exit(0);
// 	}
// }

static char *oncmd(void *stream, void *clientdata, cmd_t *cmd) {
	debug_cmd(clientdata, cmd);

	char *result = NULL;
	switch (cmd->cmd) {
		case CMD_RESERVE:
			result = onreserve(stream, clientdata, cmd);
			break;
		case CMD_BEGIN:
			result = onbegin(stream, clientdata, cmd);
			break;
		case CMD_FOR:
			result = onvote(stream, clientdata, cmd, POSITIVE);
			break;
		case CMD_AGAINST:
			result = onvote(stream, clientdata, cmd, NEGATIVE);
			break;
		case CMD_SNAPSHOT:
			result = onsnapshot(stream, clientdata, cmd);
			break;
		case CMD_STATUS:
			result = onstatus(stream, clientdata, cmd);
			break;
		default:
			return onnoise(stream, clientdata, cmd);
	}
	return result;
}

static char *ondata(void *stream, void *clientdata, size_t len, char *data) {
	int i;
	char *response = NULL;

	for (i = 0; i < len; i++) {
		if (data[i] == '\n') {
			// ignore newlines (TODO: should we ignore them?)
			continue;
		}

		if (cmd) {
			char *newresponse = oncmd(stream, clientdata, cmd);
			response = destructive_concat(response, newresponse);
			free(cmd);
		}
	}

	return response;
}

static void usage(char *prog) {
	printf(
		"Usage: %s [-d DATADIR] [-k] [-a HOST] [-p PORT] [-l LOGFILE]\n"
		"   dtmd will try to kill the other one running at\n"
		"   the same DATADIR.\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n"
		"   -k : Just kill the other dtm and exit.\n",
		prog
	);
}

// Reads a pid from the file at 'pidpath'.
// Returns the pid, or 0 in case of error.
int read_pid(char *pidpath) {
	FILE *f = fopen(pidpath, "r");
	if (f == NULL) {
		debug("failed to open pidfile for reading: %s\n", strerror(errno));
		return 0;
	}

	int pid = 0;
	if (fscanf(f, "%d", &pid) != 1) {
		shout("failed to read pid from pidfile\n");
		pid = 0;
	}

	if (fclose(f)) {
		shout("failed to close pidfile O_o: %s\n", strerror(errno));
	}
	return pid;
}

// Returns the pid, or 0 in case of error.
int write_pid(char *pidpath, int pid) {
	FILE *f = fopen(pidpath, "w");
	if (f == NULL) {
		shout("failed to open pidfile for writing: %s\n", strerror(errno));
		return 0;
	}

	if (fprintf(f, "%d\n", pid) < 0) {
		shout("failed to write pid to pidfile\n");
		pid = 0;
	}

	if (fclose(f)) {
		shout("failed to close pidfile O_o: %s\n", strerror(errno));
	}
	return pid;
}

// If there is a pidfile in 'datadir',
// sends TERM signal to the corresponding pid.
void kill_the_elder(char *datadir) {
	char *pidpath = join_path(datadir, "dtmd.pid");
	int pid = read_pid(pidpath);
	free(pidpath);

	if (pid > 1) {
		if (kill(pid, SIGTERM)) {
			switch(errno) {
				case EPERM:
					shout("was not allowed to kill pid=%d\n", pid);
					break;
				case ESRCH:
					shout("pid=%d not found for killing\n", pid);
					break;
			}
		}
		debug("SIGTERM sent to pid=%d\n" pid);
		debug("waiting for pid=%d to die\n" pid);
		waitpid(pid, NULL, 0);
		debug("pid=%d died\n" pid);
	} else {
		debug("no elder to kill\n" pid);
	}
}

char *pidpath;
void die(int signum) {
	shout("terminated\n");
	if (unlink(pidpath) == -1) {
		shout("could not remove pidfile: %s\n", strerror(errno));
	}
	exit(signum);
}

int main(int argc, char **argv) {
	char *datadir = DEFAULT_DATADIR;
	char *listenhost = DEFAULT_LISTENHOST;
	char *logfilename = NULL;
	bool daemonize = false;
	bool assassin = false;
	int listenport = DEFAULT_LISTENPORT;

	int opt;
	while ((opt = getopt(argc, argv, "hd:a:p:l:k")) != -1) {
		switch (opt) {
			case 'd':
				datadir = optarg;
				break;
			case 'a':
				listenhost = optarg;
				break;
			case 'p':
				listenport = atoi(optarg);
				break;
			case 'l':
				logfilename = optarg;
				daemonize = true;
				break;
			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;
			case 'k':
				assassin = true;
				break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	kill_the_elder(datadir);
	if (assassin) {
		return EXIT_SUCCESS;
	}

	if (logfilename) {
		freopen(logfilename, "a", stdout);
		freopen(logfilename, "a", stderr);
	}

	clg = clog_open(datadir);
	if (!clg) {
		shout("could not open clog at '%s'\n", datadir);
		return EXIT_FAILURE;
	}

	if (daemonize) {
		if (daemon(true, true) == -1) {
			shout("could not daemonize: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	pidpath = join_path(datadir, "dtmd.pid");
	signal(SIGTERM, die);
	signal(SIGINT, die);

	write_pid(pidpath, getpid());

	prev_gxid = MIN_XID;
	next_gxid = MIN_XID;
	transactions_count = 0;

	int retcode = eventwrap(
		listenhost, listenport,
		ondata, onconnect, ondisconnect
	);

	clog_close(clg);

	return retcode;
}
