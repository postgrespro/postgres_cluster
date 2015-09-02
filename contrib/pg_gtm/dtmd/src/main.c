#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clog.h"
#include "parser.h"
#include "eventwrap.h"
#include "util.h"

#define DEFAULT_DATADIR "/tmp/clog"
#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

#define MAX_PREPARES_PER_CLIENT 1024

typedef struct client_data_t {
	int id;
	parser_t parser;
	int prepares_num;
	cid_t prepares[MAX_PREPARES_PER_CLIENT];
} client_data_t;

static client_data_t *create_client_data(int id) {
	client_data_t *cd = malloc(sizeof(client_data_t));
	cd->id = id;
	cd->parser = parser_create();
	cd->prepares_num = 0;
	return cd;
}

clog_t clg;

#define CLIENT_ID(X) (((client_data_t*)(X))->id)
#define CLIENT_PARSER(X) (((client_data_t*)(X))->parser)
#define CLIENT_PREPARES_NUM(X) (((client_data_t*)(X))->prepares_num)
#define CLIENT_PREPARES(X, Y) (((client_data_t*)(X))->prepares[(Y)])

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
		"[%d] disconnected, %d prepares to abort\n",
		CLIENT_ID(client), CLIENT_PREPARES_NUM(client)
	);
	int i;
	for (i = 0; i < CLIENT_PREPARES_NUM(client); i++) {
		cid_t gcid = CLIENT_PREPARES(client, i);
		if (!clog_write(clg, gcid, COMMIT_NO)) {
			shout(
				"[%d] failed to abort gcid %016llx on disconnect\n",
				CLIENT_ID(client), gcid
			);
		}
	}
	free_client_data(client);
}

static void client_add_prepare(void *client, cid_t gcid) {
	CLIENT_PREPARES(client, CLIENT_PREPARES_NUM(client)++) = gcid;
}

static bool client_remove_prepare(void *client, cid_t gcid) {
	int i;
	for (i = 0; i < CLIENT_PREPARES_NUM(client); i++) {
		if (CLIENT_PREPARES(client, i) == gcid) {
			CLIENT_PREPARES(client, i) = CLIENT_PREPARES(
				client, --CLIENT_PREPARES_NUM(client)
			);
			return true;
		}
	}
	return false;
}

static char *onprepare(void *client, cmd_t *cmd) {
	if (CLIENT_PREPARES_NUM(client) >= MAX_PREPARES_PER_CLIENT) {
		shout(
			"[%d] cannot prepare any more commits\n",
			CLIENT_ID(client)
		);
		return strdup("-");
	}

	cid_t gcid = clog_advance(clg);
	if (gcid == INVALID_GCID) {
		return strdup("-");
	}

	client_add_prepare(client, gcid);

	char buf[18];
	sprintf(buf, "+%016llx", gcid);

	shout(
		"[%d] prepare gcid %llx\n",
		CLIENT_ID(client), gcid
	);

	return strdup(buf);
}

static char *oncommit(void *client, cmd_t *cmd) {
	shout(
		"[%d] commit %016llx\n",
		CLIENT_ID(client),
		cmd->arg
	);
	if (clog_write(clg, cmd->arg, COMMIT_YES)) {
		if (client_remove_prepare(client, cmd->arg)) {
			return strdup("+");
		}
		shout(
			"[%d] tried to commit an unprepared gcid %llu\n",
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
		if (client_remove_prepare(client, cmd->arg)) {
			return strdup("+");
		}
		shout(
			"[%d] tried to abort an unprepared gcid %016llx\n",
			CLIENT_ID(client),
			cmd->arg
		);
	}
	return strdup("-");
}

static char *onhorizon(void *client, cmd_t *cmd) {
	shout(
		"[%d] horizon\n",
		CLIENT_ID(client)
	);
	cid_t horizon = clog_horizon(clg);
	if (horizon == INVALID_GCID) {
		return strdup("-");
	}
	char buf[18];
	sprintf(buf, "+%016llx", horizon);
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
		case CMD_PREPARE:
			return onprepare(client, cmd);
		case CMD_COMMIT:
			return oncommit(client, cmd);
		case CMD_ABORT:
			return onabort(client, cmd);
		case CMD_HORIZON:
			return onhorizon(client, cmd);
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

	int retcode = eventwrap(
		listenhost, listenport,
		ondata, onconnect, ondisconnect
	);

	clog_close(clg);
	return retcode;
}
