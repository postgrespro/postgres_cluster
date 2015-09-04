#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clog.h"
#include "parser.h"
#include "eventwrap.h"
#include "util.h"
#include "intset.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

#define MAX_TRANSACTIONS 8192
#define MAX_TRANSACTIONS_PER_CLIENT 1024

static intset_t *active_transactions;

typedef struct client_data_t {
	int id;
	parser_t parser;
	int begins_num;
	xid_t begins[MAX_TRANSACTIONS_PER_CLIENT];
} client_data_t;

static client_data_t *create_client_data(int id) {
	client_data_t *cd = malloc(sizeof(client_data_t));
	cd->id = id;
	cd->parser = parser_create();
	cd->begins_num = 0;
	return cd;
}

clog_t clg;

#define CLIENT_ID(X) (((client_data_t*)(X))->id)
#define CLIENT_PARSER(X) (((client_data_t*)(X))->parser)
#define CLIENT_TRANSACTIONS_NUM(X) (((client_data_t*)(X))->begins_num)
#define CLIENT_TRANSACTIONS(X, Y) (((client_data_t*)(X))->begins[(Y)])

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
	shout(
		"[%d] disconnected, %d begins to abort\n",
		CLIENT_ID(client), CLIENT_TRANSACTIONS_NUM(client)
	);
	int i;
	for (i = 0; i < CLIENT_TRANSACTIONS_NUM(client); i++) {
		xid_t gxid = CLIENT_TRANSACTIONS(client, i);
		if (!clog_write(clg, gxid, COMMIT_NO)) {
			shout(
				"[%d] failed to abort gxid %016llx on disconnect\n",
				CLIENT_ID(client), gxid
			);
		}
		intset_remove(active_transactions, gxid);
	}
	free_client_data(client);
}

static void client_add_transaction(void *client, xid_t gxid) {
	CLIENT_TRANSACTIONS(client, CLIENT_TRANSACTIONS_NUM(client)++) = gxid;
}

static bool client_remove_transaction(void *client, xid_t gxid) {
	int i;
	for (i = 0; i < CLIENT_TRANSACTIONS_NUM(client); i++) {
		if (CLIENT_TRANSACTIONS(client, i) == gxid) {
			CLIENT_TRANSACTIONS(client, i) = CLIENT_TRANSACTIONS(
				client, --CLIENT_TRANSACTIONS_NUM(client)
			);
			return true;
		}
	}
	return false;
}

static char *onbegin(void *client, cmd_t *cmd) {
	if (CLIENT_TRANSACTIONS_NUM(client) >= MAX_TRANSACTIONS_PER_CLIENT) {
		shout(
			"[%d] cannot begin any more transactions (for this client)\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}

	if (intset_size(active_transactions) >= MAX_TRANSACTIONS) {
		shout(
			"[%d] cannot begin any more transactions (at all)\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}

	xid_t gxid = clog_advance(clg);
	if (gxid == INVALID_GXID) {
		return strdup("-");
	}

	client_add_transaction(client, gxid);

	char buf[18];
	sprintf(buf, "+%016llx", gxid);

	shout(
		"[%d] begin gxid %llx\n",
		CLIENT_ID(client), gxid
	);

	intset_add(active_transactions, gxid);
	return strdup(buf);
}

static char *oncommit(void *client, cmd_t *cmd) {
	shout(
		"[%d] commit %016llx\n",
		CLIENT_ID(client),
		cmd->arg
	);
	if (clog_write(clg, cmd->arg, COMMIT_YES)) {
		if (client_remove_transaction(client, cmd->arg)) {
			intset_remove(active_transactions, cmd->arg);
			return strdup("+");
		}
		shout(
			"[%d] tried to commit an unbeginned gxid %llu\n",
			CLIENT_ID(client),
			cmd->arg
		);
	}
	return strdup("-");
}

static char *onabort(void *client, cmd_t *cmd) {
	shout(
		"[%d] abort %016llx\n",
		CLIENT_ID(client),
		cmd->arg
	);
	if (clog_write(clg, cmd->arg, COMMIT_NO)) {
		if (client_remove_transaction(client, cmd->arg)) {
			intset_remove(active_transactions, cmd->arg);
			return strdup("+");
		}
		shout(
			"[%d] tried to abort an unbeginned gxid %016llx\n",
			CLIENT_ID(client),
			cmd->arg
		);
	}
	return strdup("-");
}

static char *onsnapshot(void *client, cmd_t *cmd) {
	shout(
		"[%d] snapshot\n",
		CLIENT_ID(client)
	);
	xid_t xmin, xmax;
	int active = intset_size(active_transactions);
	if (active > 0) {
		xmin = intset_get(active_transactions, 0);
		xmax = intset_get(active_transactions, active - 1);
	} else {
		xmin = xmax = clog_horizon(clg);
	}

	int chars_per_number = 16;
	int numbers = 3 + active;
	int eol_size = 2;
	char *buf = malloc(chars_per_number * numbers + eol_size);
	char *cursor = buf;
	*cursor = '+'; cursor++;
	sprintf(cursor, "%016llx", xmin); cursor += chars_per_number;
	sprintf(cursor, "%016llx", xmax); cursor += chars_per_number;
	sprintf(cursor, "%016llx", active); cursor += chars_per_number;
	int i;
	for (i = 0; i < active; i++) {
		sprintf(cursor, "%016llx", intset_get(active_transactions, i));
		cursor += chars_per_number;
	}
	return strdup(buf);
}

static char *onstatus(void *client, cmd_t *cmd) {
	shout(
		"[%d] status %016llx\n",
		CLIENT_ID(client),
		cmd->arg
	);
	int status = clog_read(clg, cmd->arg);
	switch (status) {
		case COMMIT_YES:
			return strdup("+c");
		case COMMIT_NO:
			return strdup("+a");
		case COMMIT_UNKNOWN:
			return strdup("+?");
		default:
			return strdup("-");
	}
}

static char *onnoise(void *client, cmd_t *cmd) {
	shout("unknown command '%c'\n", cmd->cmd);
	return strdup("-");
}

static char *oncmd(void *client, cmd_t *cmd) {
	switch (cmd->cmd) {
		case CMD_BEGIN:
			return onbegin(client, cmd);
		case CMD_COMMIT:
			return oncommit(client, cmd);
		case CMD_ABORT:
			return onabort(client, cmd);
		case CMD_SNAPSHOT:
			return onsnapshot(client, cmd);
		case CMD_STATUS:
			return onstatus(client, cmd);
		default:
			return onnoise(client, cmd);
	}

	return NULL;
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
				"[%d] parser failed: %s\n",
				CLIENT_ID(client),
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

	active_transactions = intset_create(MAX_TRANSACTIONS);

	int retcode = eventwrap(
		listenhost, listenport,
		ondata, onconnect, ondisconnect
	);

	intset_destroy(active_transactions);
	clog_close(clg);
	return retcode;
}
