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
#include "server.h"
#include "util.h"
#include "transaction.h"
#include "proto.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

L2List active_transactions = {&active_transactions, &active_transactions};
L2List* free_transactions;

// We reserve the local xids if they fit between (prev, next) range, and
// reserve something in (next, x) range otherwise, moving 'next' after 'x'.
xid_t prev_gxid, next_gxid;

typedef struct client_userdata_t {
	int id;
	int snapshots_sent;
	xid_t xid;
} client_userdata_t;

clog_t clg;

#define CLIENT_USERDATA(CLIENT) ((client_userdata_t*)client_get_userdata(CLIENT))
#define CLIENT_ID(CLIENT) (CLIENT_USERDATA(CLIENT)->id)
#define CLIENT_SNAPSENT(CLIENT) (CLIENT_USERDATA(CLIENT)->snapshots_sent)
#define CLIENT_XID(CLIENT) (CLIENT_USERDATA(CLIENT)->xid)

static client_userdata_t *create_client_userdata(int id) {
	client_userdata_t *cd = malloc(sizeof(client_userdata_t));
	cd->id = id;
	cd->snapshots_sent = 0;
	cd->xid = INVALID_XID;
	return cd;
}

static void free_client_userdata(client_userdata_t *cd) {
	free(cd);
}

inline static void free_transaction(Transaction* t)
{
    l2_list_unlink(&t->elem);
    t->elem.next = free_transactions;
    free_transactions = &t->elem;
}


static int next_client_id = 0;
static void onconnect(client_t client) {
	client_userdata_t *cd = create_client_userdata(next_client_id++);
	client_set_userdata(client, cd);
	debug("[%d] connected\n", CLIENT_ID(client));
}

static void notify_listeners(Transaction *t, int status) {
	void *listener;
	switch (status) {
		// notify 'status' listeners about the transaction status
		case BLANK:
			while ((listener = transaction_pop_listener(t, 's'))) {
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_UNKNOWN
				);
			}
			break;
		case NEGATIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the transaction status
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_ABORTED
				);
			}
			break;
		case POSITIVE:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the transaction status
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_COMMITTED
				);
			}
			break;
		case DOUBT:
			while ((listener = transaction_pop_listener(t, 's'))) {
				// notify 'status' listeners about the transaction status
				client_message_shortcut(
					(client_t)listener,
					RES_TRANSACTION_INPROGRESS
				);
			}
			break;
	}
}

static void ondisconnect(client_t client) {
	debug("[%d] disconnected\n", CLIENT_ID(client));

	if (CLIENT_XID(client) != INVALID_XID) {
        Transaction* t;

		// need to abort the transaction this client is participating in
        for (t = (Transaction*)active_transactions.next; t != (Transaction*)&active_transactions; t = (Transaction*)t->elem.next) 
        { 
			if (t->xid == CLIENT_XID(client)) {
				if (clog_write(clg, t->xid, NEGATIVE)) {
					notify_listeners(t, NEGATIVE);
                    free_transaction(t);
				} else {
					shout(
						"[%d] DISCONNECT: transaction %u"
						" failed to abort O_o\n",
						CLIENT_ID(client), t->xid
					);
				}
				break;
			}
		}

		if (t == (Transaction*)&active_transactions) {
			shout(
				"[%d] DISCONNECT: transaction %u not found O_o\n",
				CLIENT_ID(client), CLIENT_XID(client)
			);
		}
	}

	free_client_userdata(CLIENT_USERDATA(client));
	client_set_userdata(client, NULL);
}

#ifdef DEBUG
static void debug_cmd(client_t client, int argc, xid_t *argv) {
	char *cmdname;
	assert(argc > 0);
	switch (argv[0]) {
		case CMD_RESERVE : cmdname =  "RESERVE"; break;
		case CMD_BEGIN   : cmdname =    "BEGIN"; break;
		case CMD_FOR     : cmdname =      "FOR"; break;
		case CMD_AGAINST : cmdname =  "AGAINST"; break;
		case CMD_SNAPSHOT: cmdname = "SNAPSHOT"; break;
		case CMD_STATUS  : cmdname =   "STATUS"; break;
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

static xid_t max(xid_t a, xid_t b) {
	return a > b ? a : b;
}

static void gen_snapshot(Snapshot *s) {
    Transaction* t;
	s->times_sent = 0;
	s->nactive = 0;
	s->xmin = MAX_XID;
	s->xmax = MIN_XID;
    for (t = (Transaction*)active_transactions.next; t != (Transaction*)&active_transactions; t = (Transaction*)t->elem.next) 
    {
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
			"[%d] RESERVE: local range %u-%u is not between global range %u-%u\n",
			CLIENT_ID(client),
			minxid, maxxid,
			prev_gxid, next_gxid
		);

		minxid = max(minxid, next_gxid);
		maxxid = max(maxxid, minxid + minsize - 1);
		next_gxid = maxxid + 1;
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
		CLIENT_XID(client) == INVALID_XID,
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

	prev_gxid = t->xid = t->xmin = next_gxid++;
	t->snapshots_count = 0;
	t->size = 1;

	CLIENT_SNAPSENT(client) = 0;
	CLIENT_XID(client) = t->xid;

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
    l2_list_link(&active_transactions, &t->elem);

	xid_t gxmin = get_global_xmin();
	Snapshot *snap = transaction_next_snapshot(t);
	gen_snapshot(snap); 	// FIXME: increase 'times_sent' here? see also 4765234987
    t->xmin = snap->xmin;

	xid_t ok = RES_OK;
	client_message_start(client); {
		client_message_append(client, sizeof(xid_t), &ok);
		client_message_append(client, sizeof(xid_t), &t->xid);
		client_message_append(client, sizeof(xid_t), &gxmin);
		client_message_append(client, sizeof(xid_t), &snap->xmin);
		client_message_append(client, sizeof(xid_t), &snap->xmax);
		client_message_append(client, sizeof(xid_t) * snap->nactive, snap->active);
	} client_message_finish(client);
}

static Transaction *find_transaction(xid_t xid) {
	Transaction *t;

    for (t = (Transaction*)active_transactions.next; t != (Transaction*)&active_transactions; t = (Transaction*)t->elem.next)  {
		if (t->xid == xid) {
			return t;
		}
	}
	return NULL;
}

static bool queue_for_transaction_finish(client_t client, xid_t xid, char cmd) {
	assert((cmd >= 'a') && (cmd <= 'z'));

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] QUEUE: xid %u not found\n",
			CLIENT_ID(client), xid
		);
		client_message_shortcut(client, RES_FAILED);
		return false;
	}

	// TODO: Implement deadlock detection here. We have
	// CLIENT_XID(client) and 'xid', i.e. we are able to tell which
	// transaction waits which transaction.

	transaction_push_listener(t, cmd, client);
	return true;
}

static void onvote(client_t client, int argc, xid_t *argv, int vote) {
	assert((vote == POSITIVE) || (vote == NEGATIVE));

	// Check the arguments
	xid_t xid = argv[1];
	bool wait = argv[2];

	CHECK(
		CLIENT_XID(client) == xid,
		client,
		"VOTE: voting for a transaction not participated in"
	);

	Transaction *t = find_transaction(xid);
	if (t == NULL) {
		shout(
			"[%d] VOTE: xid %u not found\n",
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

	CLIENT_XID(client) = INVALID_XID; // not participating any more

	switch (transaction_status(t)) {
		case NEGATIVE:
			CHECK(
				clog_write(clg, t->xid, NEGATIVE),
				client,
				"VOTE: transaction failed to abort O_o"
			);

			notify_listeners(t, NEGATIVE);
            free_transaction(t);
			client_message_shortcut(client, RES_TRANSACTION_ABORTED);
			return;
		case DOUBT:
			if (wait) {
				CHECK(
					queue_for_transaction_finish(client, xid, 's'),
					client,
					"VOTE: couldn't queue for transaction finish"
				);
				return;
			} else {
				client_message_shortcut(client, RES_TRANSACTION_INPROGRESS);
				return;
			}
		case POSITIVE:
			CHECK(
				clog_write(clg, t->xid, POSITIVE),
				client,
				"VOTE: transaction failed to commit"
			);

			notify_listeners(t, POSITIVE);
            free_transaction(t);
			client_message_shortcut(client, RES_TRANSACTION_COMMITTED);
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
			"[%d] SNAPSHOT: xid %u not found\n",
			CLIENT_ID(client), xid
		);
		client_message_shortcut(client, RES_FAILED);
		return;
	}

	if (CLIENT_XID(client) == INVALID_XID) {
		CLIENT_SNAPSENT(client) = 0;
		CLIENT_XID(client) = t->xid;
		t->size += 1;
	}

	CHECK(
		CLIENT_XID(client) == t->xid,
		client,
		"SNAPSHOT: getting snapshot for a transaction not participated in"
	);

	assert(CLIENT_SNAPSENT(client) <= t->snapshots_count); // who sent an inexistent snapshot?!

	if (CLIENT_SNAPSENT(client) == t->snapshots_count) {
		// a fresh snapshot is needed
        Snapshot* snap = transaction_next_snapshot(t);
		gen_snapshot(snap);
        if (snap->xmin < t->xmin) { 
            t->xmin = snap->xmin;
        }
	}

	xid_t gxmin = get_global_xmin();

	Snapshot *snap = transaction_snapshot(t, CLIENT_SNAPSENT(client)++);
	snap->times_sent += 1; // FIXME: does times_sent get used anywhere? see also 4765234987

	xid_t ok = RES_OK;
	client_message_start(client); {
		client_message_append(client, sizeof(xid_t), &ok);
		client_message_append(client, sizeof(xid_t), &gxmin);
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

static void oncmd(client_t client, int argc, xid_t *argv) {
	debug_cmd(client, argc, argv);

	assert(argc > 0);
	switch (argv[0]) {
		case CMD_RESERVE:
			onreserve(client, argc, argv);
			break;
		case CMD_BEGIN:
			onbegin(client, argc, argv);
			break;
		case CMD_FOR:
			onvote(client, argc, argv, POSITIVE);
			break;
		case CMD_AGAINST:
			onvote(client, argc, argv, NEGATIVE);
			break;
		case CMD_SNAPSHOT:
			onsnapshot(client, argc, argv);
			break;
		case CMD_STATUS:
			onstatus(client, argc, argv);
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
		"Usage: %s [-d DATADIR] [-k] [-a HOST] [-p PORT] [-l LOGFILE]\n"
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
		if (!freopen(logfilename, "a", stdout)) {
			// nowhere to report this failure
			return EXIT_FAILURE;
		}
		if (!freopen(logfilename, "a", stderr)) {
			// nowhere to report this failure
			return EXIT_FAILURE;
		}
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

	pidpath = join_path(datadir, "arbiter.pid");
	signal(SIGTERM, die);
	signal(SIGINT, die);

	write_pid(pidpath, getpid());

	prev_gxid = MIN_XID;
	next_gxid = MIN_XID;

	server_t server = server_init(
		listenhost, listenport,
		onmessage, onconnect, ondisconnect
	);
	if (!server_start(server)) {
		return EXIT_FAILURE;
	}
	server_loop(server);

	clog_close(clg);

	return EXIT_SUCCESS;
}
