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
#include "raft.h"
#include "server.h"
#include "util.h"
#include "transaction.h"
#include "proto.h"
#include "ddd.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

static xid_t get_global_xmin();

L2List active_transactions = {&active_transactions, &active_transactions};
L2List* free_transactions;

Transaction* transaction_hash[MAX_TRANSACTIONS];

// We reserve the local xids if they fit between (prev, next) range, and
// reserve something in (next, x) range otherwise, moving 'next' after 'x'.
xid_t prev_gxid, next_gxid;

xid_t global_xmin = INVALID_XID;

static Transaction *find_transaction(xid_t xid) {    
	Transaction *t;    
	for (t = transaction_hash[xid % MAX_TRANSACTIONS]; t != NULL && t->xid != xid; t = t->collision);
	return t;
}

typedef struct client_userdata_t {
	int id;
	int snapshots_sent;

	// FIXME: use some meaningful words for these. E.g. "expectee" instead
	// of "xwait".
	Transaction *xpart; // the transaction this client is participating in
	Transaction *xwait; // the transaction this client is waiting for
} client_userdata_t;

clog_t clg;
raft_t raft;
bool use_raft;

#define CLIENT_USERDATA(CLIENT) ((client_userdata_t*)client_get_userdata(CLIENT))
#define CLIENT_ID(CLIENT) (CLIENT_USERDATA(CLIENT)->id)
#define CLIENT_SNAPSENT(CLIENT) (CLIENT_USERDATA(CLIENT)->snapshots_sent)
#define CLIENT_XPART(CLIENT) (CLIENT_USERDATA(CLIENT)->xpart)
#define CLIENT_XWAIT(CLIENT) (CLIENT_USERDATA(CLIENT)->xwait)

static client_userdata_t *create_client_userdata(int id) {
	client_userdata_t *cd = malloc(sizeof(client_userdata_t));
	cd->id = id;
	cd->snapshots_sent = 0;
	cd->xpart = NULL;
	cd->xwait = NULL;
	return cd;
}

static void free_client_userdata(client_userdata_t *cd) {
	free(cd);
}

inline static void free_transaction(Transaction* t) {
	assert(transaction_pop_listener(t, 's') == NULL);
	Transaction** tpp;
	for (tpp = &transaction_hash[t->xid % MAX_TRANSACTIONS]; *tpp != t; tpp = &(*tpp)->collision);
	*tpp = t->collision;
	l2_list_unlink(&t->elem);
	t->elem.next = free_transactions;
	free_transactions = &t->elem;
	if (t->xmin == global_xmin) { 
		global_xmin = get_global_xmin();
	}
}

static void notify_listeners(Transaction *t, int status) {
	void *listener;
	switch (status) {
		// notify 'status' listeners about the transaction status
		case BLANK:
			while ((listener = transaction_pop_listener(t, 's'))) {
				debug("[%d] notifying the client about xid=%u (unknown)\n", CLIENT_ID(listener), t->xid);
				CLIENT_XWAIT(listener) = NULL;
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_UNKNOWN
				);
			}
			break;
		case NEGATIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				debug("[%d] notifying the client about xid=%u (aborted)\n", CLIENT_ID(listener), t->xid);
				CLIENT_XWAIT(listener) = NULL;
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_ABORTED
				);
			}
			break;
		case POSITIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				debug("[%d] notifying the client about xid=%u (committed)\n", CLIENT_ID(listener), t->xid);
				CLIENT_XWAIT(listener) = NULL;
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_COMMITTED
				);
			}
			break;
		case DOUBT:
			while ((listener = transaction_pop_listener(t, 's'))) {
				debug("[%d] notifying the client about xid=%u (inprogress)\n", CLIENT_ID(listener), t->xid);
				CLIENT_XWAIT(listener) = NULL;
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_INPROGRESS
				);
			}
			break;
	}
}

static void apply_clog_update(int action, int argument) {
	int status = action;
	xid_t xid = argument;
	assert((status == NEGATIVE) || (status == POSITIVE));
	debug("APPLYING: xid=%u, status=%d\n", xid, status);

	if (!clog_write(clg, xid, status)) {
		shout("APPLY: failed to write to clog, xid=%u\n", xid);
	}

	if (!use_raft || (raft.role == ROLE_LEADER)) {
		Transaction *t = find_transaction(xid);
		if (t == NULL) {
			debug("APPLY: xid=%u is not active\n", xid);
			return;
		}

		notify_listeners(t, status);
		free_transaction(t);
	}
}

static int next_client_id = 0;
static void onconnect(client_t client) {
	client_userdata_t *cd = create_client_userdata(next_client_id++);
	client_set_userdata(client, cd);
	debug("[%d] connected\n", CLIENT_ID(client));
}

static void ondisconnect(client_t client) {
	Transaction *t;
	debug("[%d, %p] disconnected\n", CLIENT_ID(client), client);
	
	if ((t = CLIENT_XPART(client))) {
		transaction_remove_listener(t, 's', client);
		if (use_raft) {
			if (raft.role == ROLE_LEADER) {
				raft_emit(&raft, NEGATIVE, t->xid);
			}
		} else {
			apply_clog_update(NEGATIVE, t->xid);
		}
	}

	if ((t = CLIENT_XWAIT(client))) {
		transaction_remove_listener(t, 's', client);
	}

	free_client_userdata(CLIENT_USERDATA(client));
	client_set_userdata(client, NULL);
}

#ifdef DEBUG
static void debug_cmd(client_t client, int argc, xid_t *argv) {
	char *cmdname;
	assert(argc > 0);
	switch (argv[0]) {
		case CMD_HELLO   : cmdname =    "HELLO"; break;
		case CMD_RESERVE : cmdname =  "RESERVE"; break;
		case CMD_BEGIN   : cmdname =    "BEGIN"; break;
		case CMD_FOR     : cmdname =      "FOR"; break;
		case CMD_AGAINST : cmdname =  "AGAINST"; break;
		case CMD_SNAPSHOT: cmdname = "SNAPSHOT"; break;
		case CMD_STATUS  : cmdname =   "STATUS"; break;
		case CMD_DEADLOCK: cmdname = "DEADLOCK"; break;
		default          : cmdname =  "unknown";
	}
	debug("[%d] %s", CLIENT_ID(client), cmdname);
	int i;
	for (i = 1; i < argc; i++) {
		debug(" %u", argv[i]);
	}
	debug("\n");
}
#else
#define debug_cmd(...)
#endif

#define CHECK(COND, CLIENT, MSG) \
	do { \
		if (!(COND)) { \
			shout("[%d] %s, returning RES_FAILED\n", CLIENT_ID(CLIENT), MSG); \
			client_message_shortcut(CLIENT, RES_FAILED); \
			return; \
		} \
	} while (0)

#define CHECKLEADER(CLIENT) \
	CHECK(!use_raft || (raft.role == ROLE_LEADER), CLIENT, "not a leader")

static xid_t max_of_xids(xid_t a, xid_t b) {
	return a > b ? a : b;
}

static void gen_snapshot(Snapshot *s) {
	Transaction* t;
    int n = 0;
	s->times_sent = 0;
	for (t = (Transaction*)active_transactions.prev; t != (Transaction*)&active_transactions; t = (Transaction*)t->elem.prev) {
        /*
		if (t->xid < s->xmin) {
			s->xmin = t->xid;
		}
		if (t->xid >= s->xmax) { 
			s->xmax = t->xid + 1;
		}
        */
		s->active[n++] = t->xid;
	}
    s->nactive = n;
	if (n > 0) {
        s->xmin = s->active[0];
        s->xmax = s->active[n-1];
		assert(s->xmin <= s->xmax);
		// snapshot_sort(s);
	} else {
		s->xmin = s->xmax = 0;
	}
}

static void onhello(client_t client, int argc, xid_t *argv) {
	CHECK(argc == 1, client, "HELLO: wrong number of arguments");

	debug("[%d] HELLO\n", CLIENT_ID(client));
	if (raft.role == ROLE_LEADER) {
		client_message_shortcut(client, RES_OK);
	} else {
		client_message_shortcut(client, RES_FAILED);
	}
}

// the greatest gxid we can provide on BEGIN or RESERVE
static xid_t last_xid_in_term() {
	return raft.term * XIDS_PER_TERM - 1;
}

static xid_t first_xid_in_term() {
	return (raft.term - 1) * XIDS_PER_TERM;
}

static int xid2term(xid_t xid) {
	int term = xid / XIDS_PER_TERM + 1;
	return term;
}

// when to start worrying about starting a new term
static xid_t get_threshold_xid() {
	return last_xid_in_term() - NEW_TERM_THRESHOLD;
}

static bool xid_is_safe(xid_t xid) {
	return xid <= last_xid_in_term();
}

static bool xid_is_disturbing(xid_t xid) {
	return inrange(next_gxid + 1, get_threshold_xid(), xid);
}

static void set_next_gxid(xid_t value) {
        assert(next_gxid < value); // The value should only grow.

	if (use_raft && raft.role == ROLE_LEADER) {
		assert(xid_is_safe(value));
		if (xid_is_disturbing(value)) {
			// Time to worry has come.
			raft_ensure_term(&raft, xid2term(value));
		} else {
			// It is either too early to worry,
			// or we have already increased the term.
		}
	}

	// Check that old position is 'dirty'. It is used when dtmd restarts,
	// to find out a correct value for 'next_gxid'. If we do not remember
	// 'next_gxid' it will lead to reuse of xids, which is bad.
	assert((next_gxid == MIN_XID) || (clog_read(clg, next_gxid) == NEGATIVE));
	assert(clog_read(clg, value) == BLANK); // New position should be clean.
	if (!clog_write(clg, value, NEGATIVE)) { // Marked the new position as dirty.
		shout("could not mark xid = %u dirty\n", value);
		assert(false); // should not happen
	}
	if (!clog_write(clg, next_gxid, BLANK)) { // Cleaned the old position.
		shout("could not clean clean xid = %u from dirty state\n", next_gxid);
		assert(false); // should not happen
	}

        next_gxid = value;
}

static bool use_xid(xid_t xid) {
	if (!xid_is_safe(xid)) {
		return false;
	}
	shout("setting next_gxid to %u\n", xid + 1);
	set_next_gxid(xid + 1);
	return true;
}

static void onreserve(client_t client, int argc, xid_t *argv) {
	CHECK(argc == 3, client, "RESERVE: wrong number of arguments");

	xid_t minxid = argv[1];
	int minsize = argv[2];
	xid_t maxxid = minxid + minsize - 1;

	debug(
		"[%d] RESERVE: asked for range %u-%u\n",
		CLIENT_ID(client),
		minxid, maxxid
	);

	if ((prev_gxid >= minxid) || (maxxid >= next_gxid)) {
		debug(
			"[%d] RESERVE: local range %u-%u is not inside global range %u-%u\n",
			CLIENT_ID(client),
			minxid, maxxid,
			prev_gxid, next_gxid
		);

		minxid = max_of_xids(minxid, next_gxid);
		maxxid = max_of_xids(maxxid, minxid + minsize - 1);
		CHECK(
			use_xid(maxxid),
			client,
			"not enough xids left in this term"
		);
	}
	debug(
		"[%d] RESERVE: allocating range %u-%u\n",
		CLIENT_ID(client),
		minxid, maxxid
	);

	xid_t ok = RES_OK;
	client_message_start(client);
	client_message_append(client, sizeof(xid_t), &ok);
	client_message_append(client, sizeof(minxid), &minxid);
	client_message_append(client, sizeof(maxxid), &maxxid);
	client_message_finish(client);
}

static xid_t get_global_xmin() {
	xid_t xmin = next_gxid;
	Transaction *t;
	for (t = (Transaction*)active_transactions.next; t != (Transaction*)&active_transactions; t = (Transaction*)t->elem.next) {
		if (t->xmin < xmin) { 
			xmin = t->xmin;
		}
	}
	return xmin;
}

static void onbegin(client_t client, int argc, xid_t *argv) {
	Transaction *t;
	CHECK(
		argc == 1,
		client,
		"BEGIN: wrong number of arguments"
	);

	CHECK(
		CLIENT_XPART(client) == NULL,
		client,
		"BEGIN: already participating in another transaction"
	);

	t = (Transaction*)free_transactions;
	if (t == NULL) { 
		t = (Transaction*)malloc(sizeof(Transaction));
	} else { 
		free_transactions = t->elem.next;
	}
	transaction_clear(t);
	l2_list_link(&active_transactions, &t->elem);

	t->xid = next_gxid;
	CHECK(
		use_xid(next_gxid),
		client,
		"not enought xids left in this term"
	);
	prev_gxid = t->xid;
	t->snapshots_count = 0;
	t->size = 1;

	t->collision = transaction_hash[t->xid % MAX_TRANSACTIONS];
	transaction_hash[t->xid % MAX_TRANSACTIONS] = t;

	CLIENT_SNAPSENT(client) = 0;
	CLIENT_XPART(client) = t;

	if (!clog_write(clg, t->xid, DOUBT)) {
		shout(
			"[%d] BEGIN: transaction %u failed"
			" to initialize clog bits O_o\n",
			CLIENT_ID(client), t->xid
		);
		client_message_shortcut(client, RES_FAILED);
		free_transaction(t);
		return;
	}

	Snapshot *snap = transaction_next_snapshot(t);
	gen_snapshot(snap); // FIXME: increase 'times_sent' here? see also 4765234987
	t->xmin = snap->xmin;
	if (global_xmin == INVALID_XID) { 
		global_xmin = snap->xmin;
	}

	xid_t ok = RES_OK;
	client_message_start(client); {
		client_message_append(client, sizeof(xid_t), &ok);
		client_message_append(client, sizeof(xid_t), &t->xid);
		client_message_append(client, sizeof(xid_t), &global_xmin);
		client_message_append(client, sizeof(xid_t), &snap->xmin);
		client_message_append(client, sizeof(xid_t), &snap->xmax);
		client_message_append(client, sizeof(xid_t) * snap->nactive, snap->active);
	} client_message_finish(client);
}

static bool queue_for_transaction_finish(client_t client, xid_t xid, char cmd) {
	assert((cmd >= 'a') && (cmd <= 'z'));

	debug("[%d] QUEUE for xid=%u status\n", CLIENT_ID(client), xid);

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] QUEUE: xid=%u not found\n",
			CLIENT_ID(client), xid
		);
		client_message_shortcut(client, RES_FAILED);
		return false;
	}

	// TODO: Implement deadlock detection here. We have
	// CLIENT_XID(client) and 'xid', i.e. we are able to tell which
	// transaction waits which transaction.

	CLIENT_XWAIT(client) = t;
	transaction_push_listener(t, cmd, client);
	return true;
}

static void onvote(client_t client, int argc, xid_t *argv, int vote) {
	assert((vote == POSITIVE) || (vote == NEGATIVE));

	// Check the arguments
	xid_t xid = argv[1];
	bool wait = argv[2];

	CHECK(
		CLIENT_XPART(client) && (CLIENT_XPART(client)->xid == xid),
		client,
		"VOTE: voting for a transaction not participated in"
	);

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] VOTE: xid=%u not found\n",
			CLIENT_ID(client), xid
		);
		client_message_shortcut(client, RES_FAILED);
		return;
	}

	if (vote == POSITIVE) {
		t->votes_for += 1;
	} else if (vote == NEGATIVE) {
		t->votes_against += 1;
	} else {
		assert(false); // should not happen
	}
	assert(t->votes_for + t->votes_against <= t->size);

	CLIENT_XPART(client) = NULL; // not participating any more

	int s = transaction_status(t);
	switch (s) {
		case NEGATIVE:
		case POSITIVE:
			if (use_raft) {
				CHECK(
					queue_for_transaction_finish(client, xid, 's'),
					client,
					"VOTE: couldn't queue for transaction finish"
				);
				raft_emit(&raft, s, t->xid);
			} else {
				apply_clog_update(s, t->xid);
				if (s == POSITIVE) {
					client_message_shortcut(client, RES_TRANSACTION_COMMITTED);
				} else {
					client_message_shortcut(client, RES_TRANSACTION_ABORTED);
				}
			}
			return;
		case DOUBT:
			if (wait) {
				CHECK(
					queue_for_transaction_finish(client, xid, 's'),
					client,
					"VOTE: couldn't queue for transaction finish"
				);
			} else {
				client_message_shortcut(client, RES_TRANSACTION_INPROGRESS);
			}
			return;
	}

	assert(false); // a case missed in the switch?
	client_message_shortcut(client, RES_FAILED);
}

static void onsnapshot(client_t client, int argc, xid_t *argv) {
	CHECK(
		argc == 2,
		client,
		"SNAPSHOT: wrong number of arguments"
	);

	xid_t xid = argv[1];

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] SNAPSHOT: xid=%u not found\n",
			CLIENT_ID(client), xid
		);
		client_message_shortcut(client, RES_FAILED);
		return;
	}

	if (CLIENT_XPART(client) == NULL) {
		CLIENT_SNAPSENT(client) = 0;
		CLIENT_XPART(client) = t;
		t->size += 1;
	}

	CHECK(
		CLIENT_XPART(client) && (CLIENT_XPART(client)->xid == xid),
		client,
		"SNAPSHOT: getting snapshot for a transaction not participated in"
	);

	assert(CLIENT_SNAPSENT(client) <= t->snapshots_count); // who sent an inexistent snapshot?!

	if (CLIENT_SNAPSENT(client) == t->snapshots_count) {
		// a fresh snapshot is needed
		gen_snapshot(transaction_next_snapshot(t));
	}

	Snapshot *snap = transaction_snapshot(t, CLIENT_SNAPSENT(client)++);
	snap->times_sent += 1; // FIXME: does times_sent get used anywhere? see also 4765234987

	xid_t ok = RES_OK;
	client_message_start(client); {
		client_message_append(client, sizeof(xid_t), &ok);
		client_message_append(client, sizeof(xid_t), &global_xmin);
		client_message_append(client, sizeof(xid_t), &snap->xmin);
		client_message_append(client, sizeof(xid_t), &snap->xmax);
		client_message_append(client, sizeof(xid_t) * snap->nactive, snap->active);
	} client_message_finish(client);
}

static void onstatus(client_t client, int argc, xid_t *argv) {
	if (argc != 3) {
		shout(
			"[%d] STATUS: wrong number of arguments %d, expected %d\n",
			CLIENT_ID(client), argc, 3
		);
		client_message_shortcut(client, RES_FAILED);
		return;
	}
	xid_t xid = argv[1];
	bool wait = argv[2];

	int status = clog_read(clg, xid);
	switch (status) {
		case BLANK:
			client_message_shortcut(client, RES_TRANSACTION_UNKNOWN);
			return;
		case POSITIVE:
			client_message_shortcut(client, RES_TRANSACTION_COMMITTED);
			return;
		case NEGATIVE:
			client_message_shortcut(client, RES_TRANSACTION_ABORTED);
			return;
		case DOUBT:
			if (wait) {
				if (!queue_for_transaction_finish(client, xid, 's')) {
					shout(
						"[%d] STATUS: couldn't queue for transaction finish\n",
						CLIENT_ID(client)
					);
					client_message_shortcut(client, RES_FAILED);
					return;
				}
				return;
			} else {
				client_message_shortcut(client, RES_TRANSACTION_INPROGRESS);
				return;
			}
		default:
			assert(false); // should not happen
			client_message_shortcut(client, RES_FAILED);
			return;
	}
}

static void onnoise(client_t client, int argc, xid_t *argv) {
	shout(
		"[%d] NOISE: unknown command '%c' (%d)\n",
		CLIENT_ID(client),
		(char)argv[0], argv[0]
	);
	client_message_shortcut(client, RES_FAILED);
}

static Graph graph;

static void ondeadlock(client_t client, int argc, xid_t *argv) {
    int port;
    xid_t root;
    nodeid_t node_id;

	if (argc < 4) {
		shout(
			"[%d] DEADLOCK: wrong number of arguments %d, expected > 4\n",
			CLIENT_ID(client), argc
		);        
		client_message_shortcut(client, RES_FAILED);
		return;
	}
    port = argv[1];
    root = argv[2];
    node_id = ((nodeid_t)port << 32) | client_get_ip_addr(client);
    addSubgraph(&graph, node_id, argv+3, argc-3);
    bool hasDeadLock = detectDeadLock(&graph, root);
    client_message_shortcut(client, hasDeadLock ? RES_DEADLOCK : RES_OK);
}
    

static void oncmd(client_t client, int argc, xid_t *argv) {
	debug_cmd(client, argc, argv);

	assert(argc > 0);
	switch (argv[0]) {
		case CMD_HELLO:
			onhello(client, argc, argv);
			break;
		case CMD_RESERVE:
			CHECKLEADER(client);
			onreserve(client, argc, argv);
			break;
		case CMD_BEGIN:
			CHECKLEADER(client);
			onbegin(client, argc, argv);
			break;
		case CMD_FOR:
			CHECKLEADER(client);
			onvote(client, argc, argv, POSITIVE);
			break;
		case CMD_AGAINST:
			CHECKLEADER(client);
			onvote(client, argc, argv, NEGATIVE);
			break;
		case CMD_SNAPSHOT:
			CHECKLEADER(client);
			onsnapshot(client, argc, argv);
			break;
		case CMD_STATUS:
			CHECKLEADER(client);
			onstatus(client, argc, argv);
			break;
		case CMD_DEADLOCK:
			ondeadlock(client, argc, argv);
			break;
		default:
			onnoise(client, argc, argv);
	}
}

static void onmessage(client_t client, size_t len, char *data) {
	assert(len % sizeof(xid_t) == 0);

	int argc = len / sizeof(xid_t);
	xid_t *argv = (xid_t*)data;
	CHECK(argc > 0, client, "EMPTY: empty command?");

	oncmd(client, argc, argv);
}

static void usage(char *prog) {
	printf(
		"Usage: %s -i ID -r HOST:PORT [-r HOST:PORT ...] [-d DATADIR] [-k] [-l LOGFILE]\n"
		"   arbiter will try to kill the other one running at\n"
		"   the same DATADIR.\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n"
		"   -k : Just kill the other arbiter and exit.\n",
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
	char *pidpath = join_path(datadir, "arbiter.pid");
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
		debug("SIGTERM sent to pid=%d\n", pid);
		debug("waiting for pid=%d to die\n", pid);
		waitpid(pid, NULL, 0);
		debug("pid=%d died\n", pid);
	} else {
		debug("no elder to kill\n");
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

char *datadir = DEFAULT_DATADIR;
char *logfilename = NULL;
bool daemonize = false;
bool assassin = false;

bool configure(int argc, char **argv) {
	raft_init(&raft);
	int myid = NOBODY;

    initGraph(&graph);

	int opt;
	while ((opt = getopt(argc, argv, "hd:i:r:l:k")) != -1) {
		char *host;
		char *portstr;
		int port;
		switch (opt) {
			case 'i':
				myid = atoi(optarg);
				break;
			case 'd':
				datadir = optarg;
				break;
			case 'r':
				host = strtok(optarg, ":");
				portstr = strtok(NULL, ":");
				if (portstr) {
					port = atoi(portstr);
				} else {
					port = DEFAULT_LISTENPORT;
				}
				raft_add_server(&raft, host, port);
				break;
			case 'l':
				logfilename = optarg;
				daemonize = true;
				break;
			case 'h':
				usage(argv[0]);
				return false;
			case 'k':
				assassin = true;
				break;
			default:
				usage(argv[0]);
				return false;
		}
	}

	if (raft.servernum < 1) {
		shout("please, specify -r HOST:PORT at least once\n");
		usage(argv[0]);
		return false;
	}
	use_raft = raft.servernum > 1;

	if (!raft_set_myid(&raft, myid)) {
		usage(argv[0]);
		return false;
	}

	return true;
}

bool redirect_output() {
	if (logfilename) {
		if (!freopen(logfilename, "a", stdout)) {
			// nowhere to report this failure
			return false;
		}
		if (!freopen(logfilename, "a", stderr)) {
			// nowhere to report this failure
			return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	if (!configure(argc, argv)) return EXIT_FAILURE;

	kill_the_elder(datadir);
	if (assassin) return EXIT_SUCCESS;

	if (!redirect_output()) return EXIT_FAILURE;

	next_gxid = MIN_XID;
	clg = clog_open(datadir);

	xid_t last_used_xid = clog_find_last_used(clg);
	shout("will use %u\n", last_used_xid);
	if (!use_xid(last_used_xid)) {
		shout("could not set last used xid to %u\n", last_used_xid);
		return EXIT_FAILURE;
	}
	raft.term = xid2term(next_gxid);

	prev_gxid = next_gxid - 1;
	debug("initial next_gxid = %u\n", next_gxid);
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

	pidpath = join_path(datadir, "arbiter.pid");
	signal(SIGTERM, die);
	signal(SIGINT, die);

	write_pid(pidpath, getpid());


	int raftsock = raft_create_udp_socket(&raft);
	if (raftsock == -1) {
		die(EXIT_FAILURE);
	}

	server_t server = server_init(
		raft.servers[raft.me].host, raft.servers[raft.me].port,
		onmessage, onconnect, ondisconnect
	);

	server_set_raft_socket(server, raftsock);

	if (!server_start(server)) {
		return EXIT_FAILURE;
	}

	mstimer_t t;
	mstimer_reset(&t);
	int old_term = 0;
	while (true) {
		int ms = mstimer_reset(&t);
		raft_msg_t *m = NULL;

		if (use_raft) {
			raft_tick(&raft, ms);
		}

		// The client interaction is done in server_tick.
		if (server_tick(server, HEARTBEAT_TIMEOUT_MS)) {
			m = raft_recv_message(&raft);
			assert(m); // m should not be NULL, because the message should be ready to recv
		}

		if (use_raft) {
			int applied = raft_apply(&raft, apply_clog_update);
			if (applied) {
				debug("applied %d updates\n", applied);
			}

			if (m) {
				raft_handle_message(&raft, m);
			}

			server_set_enabled(server, raft.role == ROLE_LEADER);

			// Update the gxid limits based on current term and leadership.
			if (old_term < raft.term) {
				if (raft.role == ROLE_FOLLOWER) {
					// If we become a leader, we will use
					// the range of xids after the current
					// last_gxid.
					prev_gxid = last_xid_in_term();
					set_next_gxid(prev_gxid + 1);
					shout("updated range to %u-%u\n", prev_gxid, next_gxid);
				}
				old_term = raft.term;
			}
		} else {
			server_set_enabled(server, true);
		}
	}

	clog_close(clg);

	return EXIT_SUCCESS;
}
