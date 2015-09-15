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

static client_data_t *create_client_data(int id) {
	client_data_t *cd = malloc(sizeof(client_data_t));
	cd->id = id;
	cd->parser = parser_create();
	return cd;
}

clog_t clg;

#define CLIENT_ID(X) (((client_data_t*)(X))->id)
#define CLIENT_PARSER(X) (((client_data_t*)(X))->parser)

static void free_client_data(client_data_t *cd) {
	parser_destroy(cd->parser);
	free(cd);
}

int next_client_id = 0;
static void onconnect(void **client) {
	*client = create_client_data(next_client_id++);
	shout("[%d] connected\n", CLIENT_ID(*client));
}

static void ondisconnect(void *client) {
	shout("[%d] disconnected\n", CLIENT_ID(client));
	free_client_data(client);
}

static void clear_global_transaction(GlobalTransaction *t) {
	int i;
	for (i = 0; i < MAX_NODES; i++) {
		t->participants[i].active = false;
	}
}

static void shout_cmd(void *client, cmd_t *cmd) {
	char *cmdname;
	switch (cmd->cmd) {
		case CMD_BEGIN   : cmdname =    "BEGIN"; break;
		case CMD_COMMIT  : cmdname =   "COMMIT"; break;
		case CMD_ABORT   : cmdname =    "ABORT"; break;
		case CMD_SNAPSHOT: cmdname = "SNAPSHOT"; break;
		case CMD_STATUS  : cmdname =   "STATUS"; break;
		default          : cmdname =  "unknown";
	}
	shout("[%d] %s", CLIENT_ID(client), cmdname);
	int i;
	for (i = 0; i < cmd->argc; i++) {
		shout(" %#llx", cmd->argv[i]);
	}
	shout("\n");
}

static char *onbegin(void *client, cmd_t *cmd) {
	if (transactions_count >= MAX_TRANSACTIONS) {
		shout(
			"[%d] BEGIN: transaction limit hit\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}

	if (cmd->argc < 3) {
		shout(
			"[%d] BEGIN: wrong number of arguments\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}
	int size = cmd->argv[0];
	if (cmd->argc - 1 != size * 2) {
		shout(
			"[%d] BEGIN: wrong 'size'\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}
	if (size > MAX_NODES) {
		shout(
			"[%d] BEGIN: 'size' > MAX_NODES (%d > %d)\n",
			CLIENT_ID(client), size, MAX_NODES
		);
		return strdup("-");
	}

	GlobalTransaction *gt = transactions + transactions_count;
	clear_global_transaction(gt);
	int i;
	for (i = 0; i < size; i++) {
		int node = cmd->argv[i * 2 + 1];
		xid_t xid = cmd->argv[i * 2 + 2];

		if (node > MAX_NODES) {
			shout(
				"[%d] BEGIN: wrong 'node'\n",
				CLIENT_ID(client)
			);
			return strdup("-");
		}

		Transaction *t = gt->participants + node;
		if (t->active) {
			shout(
				"[%d] BEGIN: node %d mentioned twice\n",
				CLIENT_ID(client), node
			);
			return strdup("-");
		}
		t->active = true;
		t->node = node;
		t->vote = NEUTRAL;
		t->xid = xid;
		t->snapshot.seqno = 0;
		t->sent_seqno = 0;

		if (xid > xmax[node]) {
			xmax[node] = xid;
		}
	}
	transactions_count++;
	return strdup("+");
}

static char *onvote(void *client, cmd_t *cmd, int vote) {
	if (cmd->argc != 2) {
		shout(
			"[%d] VOTE: wrong number of arguments\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}
	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	if (node > MAX_NODES) {
		shout(
			"[%d] VOTE: voted about a wrong 'node' (%d)\n",
			CLIENT_ID(client), node
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
			"[%d] VOTE: node %d xid %llu not found\n",
			CLIENT_ID(client), node, xid
		);
		return strdup("-");
	}

	if (transactions[i].participants[node].vote != NEUTRAL) {
		shout(
			"[%d] VOTE: node %d voting on xid %llu again\n",
			CLIENT_ID(client), node, xid
		);
		return strdup("-");
	}
	transactions[i].participants[node].vote = vote;

	switch (global_transaction_status(transactions + i)) {
		case NEGATIVE:
			if (global_transaction_mark(clg, transactions + i, NEGATIVE)) {
				shout(
					"[%d] VOTE: global transaction aborted\n",
					CLIENT_ID(client)
				);
				transactions[i] = transactions[transactions_count - 1];
				transactions_count--;
				return strdup("+");
			} else {
				shout(
					"[%d] VOTE: global transaction failed"
					" to abort O_o\n",
					CLIENT_ID(client)
				);
				return strdup("-");
			}
		case NEUTRAL:
			shout("[%d] VOTE: vote counted\n", CLIENT_ID(client));
			return strdup("+");
		case POSITIVE:
			if (global_transaction_mark(clg, transactions + i, POSITIVE)) {
				shout(
					"[%d] VOTE: global transaction committed\n",
					CLIENT_ID(client)
				);
				transactions[i] = transactions[transactions_count - 1];
				transactions_count--;
				return strdup("+");
			} else {
				shout(
					"[%d] VOTE: global transaction failed"
					" to commit\n",
					CLIENT_ID(client)
				);
				return strdup("-");
			}
	}

	assert(false); // a case missed in the switch?
	return strdup("-");
}

static char *oncommit(void *client, cmd_t *cmd) {
	return onvote(client, cmd, POSITIVE);
}

static char *onabort(void *client, cmd_t *cmd) {
	return onvote(client, cmd, NEGATIVE);
}

static void gen_snapshot(Snapshot *s, int node) {
	s->nactive = 0;
	s->xmin = xmax[node];
    s->xmax = s->xmin + 1;
	int i;
	for (i = 0; i < transactions_count; i++) {
		Transaction *t = transactions[i].participants + node;
		if (t->active) {
			if (t->xid < s->xmin) {
				s->xmin = t->xid;
			}
			s->active[s->nactive++] = t->xid;
		}
	}
	snapshot_sort(s);
	s->seqno++;
}

static void gen_snapshots(GlobalTransaction *gt) {
	int n;
	for (n = 0; n < MAX_NODES; n++) {
		gen_snapshot(&gt->participants[n].snapshot, n);
	}
}

static char *onsnapshot(void *client, cmd_t *cmd) {
	if (cmd->argc != 2) {
		shout(
			"[%d] SNAPSHOT: wrong number of arguments\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}
	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	if (node > MAX_NODES) {
		shout(
			"[%d] SNAPSHOT: wrong 'node' (%d)\n",
			CLIENT_ID(client), node
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
			CLIENT_ID(client), node, xid
		);
		return strdup("-");
	}

	GlobalTransaction *gt = transactions + i;
	Transaction *t = gt->participants + node;
	if (t->sent_seqno == t->snapshot.seqno) {
		gen_snapshots(gt);
	}
	assert(t->sent_seqno < t->snapshot.seqno);

	t->sent_seqno++;
	return snapshot_serialize(&t->snapshot);
}

static char *onstatus(void *client, cmd_t *cmd) {
	if (cmd->argc != 2) {
		shout(
			"[%d] STATUS: wrong number of arguments\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}
	int node = cmd->argv[0];
	xid_t xid = cmd->argv[1];
	if (node > MAX_NODES) {
		shout(
			"[%d] STATUS: wrong 'node' (%d)\n",
			CLIENT_ID(client), node
		);
		return strdup("-");
	}

	int status = clog_read(clg, MUX_XID(node, xid));
	switch (status) {
		case POSITIVE:
			return strdup("+c");
		case NEGATIVE:
			return strdup("+a");
		case NEUTRAL:
			return strdup("+?");
		default:
			return strdup("-");
	}
}

static char *onnoise(void *client, cmd_t *cmd) {
	shout(
		"[%d] NOISE: unknown command '%c'\n",
		CLIENT_ID(client),
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

static char *oncmd(void *client, cmd_t *cmd) {
	shout_cmd(client, cmd);

	char *result = NULL;
	switch (cmd->cmd) {
		case CMD_BEGIN:
			result = onbegin(client, cmd);
			break;
		case CMD_COMMIT:
			result = oncommit(client, cmd);
			break;
		case CMD_ABORT:
			result = onabort(client, cmd);
			break;
		case CMD_SNAPSHOT:
			result = onsnapshot(client, cmd);
			break;
		case CMD_STATUS:
			result = onstatus(client, cmd);
			break;
		default:
			return onnoise(client, cmd);
	}
	return result;
}

char *destructive_concat(char *a, char *b) {
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

char *ondata(void *client, size_t len, char *data) {
	int i;
	parser_t parser = CLIENT_PARSER(client);
	char *response = NULL;

	shout(
		"[%d] got some data[%lu] %s\n",
		CLIENT_ID(client),
		len, data
	);

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
				CLIENT_ID(client),
				data[i], data[i],
				parser_errormsg(parser)
			);
			parser_init(parser);
			response = strdup("-");
			break;
		}
		if (cmd) {
			char *newresponse = oncmd(client, cmd);
			response = destructive_concat(response, newresponse);
			free(cmd);
		}
	}

	return response;
}

void usage(char *prog) {
	shout("Usage: %s [-d DATADIR] [-a HOST] [-p PORT]\n", prog);
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
