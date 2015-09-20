#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "clog.h"
#include "parser.h"
#include "eventwrap.h"
#include "util.h"
#include "intset.h"
#include "transaction.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

GlobalTransaction transactions[MAX_TRANSACTIONS];
int transactions_count;
xid_t xmax[MAX_NODES];

typedef struct client_data_t {
	int id;
	parser_t parser;
} client_data_t;

clog_t clg;
static client_data_t *create_client_data(int id);
static void free_client_data(client_data_t *cd);
static void onconnect(void *stream, void **clientdata);
static void ondisconnect(void *stream, void *clientdata);
static char *onbegin(void *stream, void *clientdata, cmd_t *cmd);
static char *onvote(void *stream, void *clientdata, cmd_t *cmd, int vote);
static char *oncommit(void *stream, void *clientdata, cmd_t *cmd);
static char *onabort(void *stream, void *clientdata, cmd_t *cmd);
static void gen_snapshot(Snapshot *s, int node);
static void gen_snapshots(GlobalTransaction *gt);
static char *onsnapshot(void *stream, void *clientdata, cmd_t *cmd);
static bool queue_for_transaction_finish(void *stream, void *clientdata, int node, xid_t xid, char cmd);
static void notify_listeners(GlobalTransaction *gt, int status);
static char *onstatus(void *stream, void *clientdata, cmd_t *cmd);
static char *onnoise(void *stream, void *clientdata, cmd_t *cmd);
static char *oncmd(void *stream, void *clientdata, cmd_t *cmd);
static char *destructive_concat(char *a, char *b);
static char *ondata(void *stream, void *clientdata, size_t len, char *data);
static void usage(char *prog);

#define CLIENT_ID(X) (((client_data_t*)(X))->id)
#define CLIENT_PARSER(X) (((client_data_t*)(X))->parser)

static client_data_t *create_client_data(int id) {
	client_data_t *cd = malloc(sizeof(client_data_t));
	cd->id = id;
	cd->parser = parser_create();
	return cd;
}

static void free_client_data(client_data_t *cd) {
	parser_destroy(cd->parser);
	free(cd);
}

int next_client_id = 0;
static void onconnect(void *stream, void **clientdata) {
	*clientdata = create_client_data(next_client_id++);
	shout("[%d] connected\n", CLIENT_ID(*clientdata));
}

static void ondisconnect(void *stream, void *clientdata) {
	int client_id = CLIENT_ID(clientdata);
	shout("[%d] disconnected\n", client_id);

	int i, n;
	for (i = transactions_count - 1; i >= 0; i--) {
		GlobalTransaction *gt = transactions + i;

		for (n = 0; n < MAX_NODES; n++) {
			Transaction *t = gt->participants + n;
			if ((t->active) && (t->client_id == client_id)) {
				if (global_transaction_mark(clg, gt, NEGATIVE)) {
					notify_listeners(gt, NEGATIVE);

					transactions[i] = transactions[transactions_count - 1];
					transactions_count--;
				} else {
					shout(
						"[%d] DISCONNECT: global transaction failed"
						" to abort O_o\n",
						client_id
					);
				}
				break;
			}
		}
	}

	free_client_data(clientdata);
}

#ifdef NDEBUG
#define shout_cmd(...)
#else
static void shout_cmd(void *clientdata, cmd_t *cmd) {
	char *cmdname;
	switch (cmd->cmd) {
		case CMD_BEGIN   : cmdname =    "BEGIN"; break;
		case CMD_COMMIT  : cmdname =   "COMMIT"; break;
		case CMD_ABORT   : cmdname =    "ABORT"; break;
		case CMD_SNAPSHOT: cmdname = "SNAPSHOT"; break;
		case CMD_STATUS  : cmdname =   "STATUS"; break;
		default          : cmdname =  "unknown";
	}
	shout("[%d] %s", CLIENT_ID(clientdata), cmdname);
	int i;
	for (i = 0; i < cmd->argc; i++) {
		shout(" %#llx", cmd->argv[i]);
	}
	shout("\n");
}
#endif

static char *onbegin(void *stream, void *clientdata, cmd_t *cmd) {
	if (transactions_count >= MAX_TRANSACTIONS) {
		shout(
			"[%d] BEGIN: transaction limit hit\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}

	if (cmd->argc < 3) {
		shout(
			"[%d] BEGIN: wrong number of arguments\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}
	int size = cmd->argv[0];
	if (cmd->argc - 1 != size * 2) {
		shout(
			"[%d] BEGIN: wrong 'size'\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}
	if (size > MAX_NODES) {
		shout(
			"[%d] BEGIN: 'size' > MAX_NODES (%d > %d)\n",
			CLIENT_ID(clientdata), size, MAX_NODES
		);
		return strdup("-");
	}

	GlobalTransaction *gt = transactions + transactions_count;
	global_transaction_clear(gt);
	int i;
	for (i = 0; i < size; i++) {
		int node = cmd->argv[i * 2 + 1];
		xid_t xid = cmd->argv[i * 2 + 2];

		if (node >= MAX_NODES) {
			shout(
				"[%d] BEGIN: wrong 'node'\n",
				CLIENT_ID(clientdata)
			);
			return strdup("-");
		}

		Transaction *t = gt->participants + node;
		if (t->active) {
			shout(
				"[%d] BEGIN: node %d mentioned twice\n",
				CLIENT_ID(clientdata), node
			);
			return strdup("-");
		}
		t->client_id = CLIENT_ID(clientdata);
		t->active = true;
		t->node = node;
		t->vote = DOUBT;
		t->xid = xid;
		t->snapshot_no = 0;

		if (xid > xmax[node]) {
			xmax[node] = xid;
		}
	}
	if (!global_transaction_mark(clg, gt, DOUBT)) {
		shout(
			"[%d] BEGIN: global transaction failed"
			" to initialize clog bits O_o\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}

	transactions_count++;
	return strdup("+");
}

static void notify_listeners(GlobalTransaction *gt, int status) {
	void *listener;
	switch (status) {
		case NEGATIVE:
			while ((listener = global_transaction_pop_listener(gt, 's'))) {
				// notify 'status' listeners about the aborted status
				write_to_stream(listener, strdup("+a"));
			}
			while ((listener = global_transaction_pop_listener(gt, 'c'))) {
				// notify 'commit' listeners about the failure
				write_to_stream(listener, strdup("-"));
			}
			break;
		case POSITIVE:
			while ((listener = global_transaction_pop_listener(gt, 's'))) {
				// notify 'status' listeners about the committed status
				write_to_stream(listener, strdup("+c"));
			}
			while ((listener = global_transaction_pop_listener(gt, 'c'))) {
				// notify 'commit' listeners about the success
				write_to_stream(listener, strdup("+"));
			}
			break;
	}
}

static char *onvote(void *stream, void *clientdata, cmd_t *cmd, int vote) {
	assert((vote == POSITIVE) || (vote == NEGATIVE));

	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	bool wait = (vote == POSITIVE) ? cmd->argv[2] : false;
	if (node >= MAX_NODES) {
		shout(
			"[%d] VOTE: voted about a wrong 'node' (%d)\n",
			CLIENT_ID(clientdata), node
		);
		return strdup("-");
	}

	if ((vote == NEGATIVE) && wait) {
		shout(
			"[%d] VOTE: 'wait' is ignored for NEGATIVE votes\n",
			CLIENT_ID(clientdata)
		);
	}

	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions[i].participants + node;
		if ((t->active) && (t->node == node) && (t->xid == xid)) {
			break;
		}
	}

	if (i == transactions_count) {
		shout(
			"[%d] VOTE: node %d xid %llu not found\n",
			CLIENT_ID(clientdata), node, xid
		);
		return strdup("-");
	}

	if (transactions[i].participants[node].vote != DOUBT) {
		shout(
			"[%d] VOTE: node %d voting on xid %llu again\n",
			CLIENT_ID(clientdata), node, xid
		);
		return strdup("-");
	}
	transactions[i].participants[node].vote = vote;

	GlobalTransaction *gt = transactions + i;
	switch (global_transaction_status(gt)) {
		case NEGATIVE:
			if (global_transaction_mark(clg, gt, NEGATIVE)) {
				notify_listeners(gt, NEGATIVE);

				transactions[i] = transactions[transactions_count - 1];
				transactions_count--;
				return strdup("+");
			} else {
				shout(
					"[%d] VOTE: global transaction failed"
					" to abort O_o\n",
					CLIENT_ID(clientdata)
				);
				return strdup("-");
			}
		case DOUBT:
			//shout("[%d] VOTE: vote counted\n", CLIENT_ID(clientdata));
			if (wait) {
				if (!queue_for_transaction_finish(stream, clientdata, node, xid, 'c')) {
					shout(
						"[%d] VOTE: couldn't queue for transaction finish\n",
						CLIENT_ID(clientdata)
					);
					return strdup("-");
				}
				return NULL;
			} else {
				return strdup("+");
			}
		case POSITIVE:
			if (global_transaction_mark(clg, transactions + i, POSITIVE)) {
				notify_listeners(gt, POSITIVE);

				transactions[i] = transactions[transactions_count - 1];
				transactions_count--;
				return strdup("+");
			} else {
				shout(
					"[%d] VOTE: global transaction failed"
					" to commit\n",
					CLIENT_ID(clientdata)
				);
				return strdup("-");
			}
	}

	assert(false); // a case missed in the switch?
	return strdup("-");
}

static char *oncommit(void *stream, void *clientdata, cmd_t *cmd) {
	if (cmd->argc != 3) {
		shout(
			"[%d] COMMIT: wrong number of arguments\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}

	return onvote(stream, clientdata, cmd, POSITIVE);
}

static char *onabort(void *stream, void *clientdata, cmd_t *cmd) {
	if (cmd->argc != 2) {
		shout(
			"[%d] ABORT: wrong number of arguments\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}

	return onvote(stream, clientdata, cmd, NEGATIVE);
}

static void gen_snapshot(Snapshot *s, int node) {
	s->nactive = 0;
	s->xmin = xmax[node];
    s->xmax = 0;
	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions[i].participants + node;
		if (t->active) {
			if (t->xid < s->xmin) {
				s->xmin = t->xid;
			}
            if (t->xid >= s->xmax) { 
                s->xmax = t->xid + 1;
            }
			s->active[s->nactive++] = t->xid;
		}
	}
	snapshot_sort(s);
}

static void gen_snapshots(GlobalTransaction *gt) {
	int n;
	for (n = 0; n < MAX_NODES; n++) {
		gen_snapshot(&gt->participants[n].snapshot[gt->n_snapshots % MAX_SNAPSHOTS_PER_TRANS], n);
	}
    gt->n_snapshots += 1;
}

static char *onsnapshot(void *stream, void *clientdata, cmd_t *cmd) {
	if (cmd->argc != 2) {
		shout(
			"[%d] SNAPSHOT: wrong number of arguments\n",
			CLIENT_ID(clientdata)
		);
		return strdup("-");
	}
	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	if (node > MAX_NODES) {
		shout(
			"[%d] SNAPSHOT: wrong 'node' (%d)\n",
			CLIENT_ID(clientdata), node
		);
		return strdup("-");
	}

	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions[i].participants + node;
		if ((t->active) && (t->node == node) && (t->xid == xid)) {
			break;
		}
	}

	if (i == transactions_count) {
		shout(
			"[%d] SNAPSHOT: node %d xid %llu not found\n",
			CLIENT_ID(clientdata), node, xid
		);
		return strdup("-");
	}

	GlobalTransaction *gt = &transactions[i];
	Transaction *t = &gt->participants[node];
	if (t->snapshot_no == gt->n_snapshots) {
		gen_snapshots(gt);
	}
	assert(t->snapshot_no < gt->n_snapshots);

	return snapshot_serialize(&t->snapshot[t->snapshot_no++ % MAX_SNAPSHOTS_PER_TRANS]);
}

static bool queue_for_transaction_finish(void *stream, void *clientdata, int node, xid_t xid, char cmd) {
	assert((cmd >= 'a') && (cmd <= 'z'));
	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions[i].participants + node;
		if ((t->active) && (t->node == node) && (t->xid == xid)) {
			break;
		}
	}

	if (i == transactions_count) {
		shout(
			"[%d] QUEUE: node %d xid %llu not found\n",
			CLIENT_ID(clientdata), node, xid
		);
		return false;
	}

	global_transaction_push_listener(&transactions[i], cmd, stream);
	return true;
}

static char *onstatus(void *stream, void *clientdata, cmd_t *cmd) {
	if (cmd->argc != 3) {
		shout(
			"[%d] STATUS: wrong number of arguments %d, expected %d\n",
			CLIENT_ID(clientdata), cmd->argc, 3
		);
		return strdup("-");
	}
	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	bool wait = cmd->argv[2];
	if (node > MAX_NODES) {
		shout(
			"[%d] STATUS: wrong 'node' (%d)\n",
			CLIENT_ID(clientdata), node
		);
		return strdup("-");
	}

	int status = clog_read(clg, MUX_XID(node, xid));
	switch (status) {
		case BLANK:
			return strdup("+0");
		case POSITIVE:
			return strdup("+c");
		case NEGATIVE:
			return strdup("+a");
		case DOUBT:
			if (wait) {
				if (!queue_for_transaction_finish(stream, clientdata, node, xid, 's')) {
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
	shout_cmd(clientdata, cmd);

	char *result = NULL;
	switch (cmd->cmd) {
		case CMD_BEGIN:
			result = onbegin(stream, clientdata, cmd);
			break;
		case CMD_COMMIT:
			result = oncommit(stream, clientdata, cmd);
			break;
		case CMD_ABORT:
			result = onabort(stream, clientdata, cmd);
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

static char *destructive_concat(char *a, char *b) {
	if ((a == NULL) && (b == NULL)) {
		return NULL;
	}

	size_t lena = a ? strlen(a) : 0;
	size_t lenb = b ? strlen(b) : 0;
	size_t lenc = lena + lenb + 1;
	char *c = malloc(lenc);

	if (a) {
		strcpy(c, a);
		free(a);
	}
	if (b) {
		strcpy(c + lena, b);
		free(b);
	}

	return c;
}

static char *ondata(void *stream, void *clientdata, size_t len, char *data) {
	int i;
	parser_t parser = CLIENT_PARSER(clientdata);
	char *response = NULL;

//	shout(
//		"[%d] got some data[%lu] %s\n",
//		CLIENT_ID(clientdata),
//		len, data
//	);

	// The idea is to feed each character through
	// the parser, which will return a cmd from
	// time to time.
	for (i = 0; i < len; i++) {
		if (data[i] == '\n') {
			// ignore newlines (TODO: should we ignore them?)
			continue;
		}

		cmd_t *cmd = parser_feed(parser, data[i]);
		if (parser_failed(parser)) {
			shout(
				"[%d] parser failed on character '%c' (%d): %s\n",
				CLIENT_ID(clientdata),
				data[i], data[i],
				parser_errormsg(parser)
			);
			parser_init(parser);
			response = strdup("-");
			break;
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
	printf("Usage: %s [-d DATADIR] [-a HOST] [-p PORT]\n", prog);
}

int main(int argc, char **argv) {
	char *datadir = DEFAULT_DATADIR;
	char *listenhost = DEFAULT_LISTENHOST;
	int listenport = DEFAULT_LISTENPORT;

	int opt;
	while ((opt = getopt(argc, argv, "hd:a:p:")) != -1) {
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
			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	clg = clog_open(datadir);
	if (!clg) {
		shout("could not open clog at '%s'\n", datadir);
		return EXIT_FAILURE;
	}

	transactions_count = 0;
	int i;
	for (i = 0; i < MAX_NODES; i++) {
		xmax[i] = 0;
	}

	int retcode = eventwrap(
		listenhost, listenport,
		ondata, onconnect, ondisconnect
	);

	clog_close(clg);
	return retcode;
}
