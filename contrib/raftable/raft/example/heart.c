#define _POSIX_C_SOURCE 2
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdbool.h>

#include <jansson.h>

#include "raft.h"
#include "util.h"

static void applier(void *state, raft_update_t update, raft_bool_t snapshot) {
	json_error_t error;

	json_t *patch = json_loadb(update.data, update.len, 0, &error);
	if (!patch) {
		shout(
			"error parsing json at position %d: %s\n",
			error.column,
			error.text
		);
	}

	if (snapshot) {
		json_object_clear(state);
	}

	if (json_object_update(state, patch)) {
		shout("error updating state\n");
	}

	json_decref(patch);

	char *encoded = json_dumps(state, JSON_INDENT(4) | JSON_SORT_KEYS);
	if (encoded) {
		debug(
			"applied %s: new state is %s\n",
			snapshot ? "a snapshot" : "an update",
			encoded
		);
	} else {
		shout(
			"applied %s, but new state could not be encoded\n",
			snapshot ? "a snapshot" : "an update"
		);
	}
	free(encoded);
}

static raft_update_t snapshooter(void *state) {
	raft_update_t shot;
	shot.data = json_dumps(state, JSON_SORT_KEYS);
	shot.len = strlen(shot.data);
	if (shot.data) {
		debug("snapshot taken: %.*s\n", shot.len, shot.data);
	} else {
		shout("failed to take a snapshot\n");
	}

	return shot;
}

static void die(int signum) {
	debug("dying\n");
	exit(signum);
}

static void usage(char *prog) {
	printf(
		"Usage: %s -i ID -r ID:HOST:PORT [-r ID:HOST:PORT ...] [-l LOGFILE]\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n",
		prog
	);
}

raft_t raft;

static void main_loop(char *host, int port) {
	mstimer_t t;
	mstimer_reset(&t);

	// create a UDP socket for raft
	int r = raft_create_udp_socket(raft);
	if (r == NOBODY) {
		die(EXIT_FAILURE);
	}

	#define EMIT_EVERY_MS 1000
	int emit_ms = 0;
	while (true) {
		int ms;
		raft_msg_t m;

		ms = mstimer_reset(&t);

		raft_tick(raft, ms);
		m = raft_recv_message(raft);
		if (m) {
			raft_handle_message(raft, m);
		}

		if (raft_is_leader(raft)) {
			emit_ms += ms;
			while (emit_ms > EMIT_EVERY_MS) {
				emit_ms -= EMIT_EVERY_MS;
				char buf[1000];
				char key = 'a' + rand() % 5;
				int value = rand() % 10000;
				sprintf(buf, "{\"key-%c\":%d}", key, value);
				shout("emit update: = %s\n", buf);
				raft_update_t update = {strlen(buf), buf, NULL};
				raft_emit(raft, update);
			}
		}
	}

	close(r);
}

int main(int argc, char **argv) {
	char *logfilename = NULL;
	bool daemonize = false;

	int myid = NOBODY;
	int id;
	char *host;
	char *str;
	int port;
	int opt;

	char *myhost = NULL;
	int myport;

	json_t *state = json_object();

	raft_config_t rc;
	rc.peernum_max = 64;
	rc.heartbeat_ms = 20;
	rc.election_ms_min = 150;
	rc.election_ms_max = 300;
	rc.log_len = 10;
	rc.chunk_len = 4;
	rc.msg_len_max = 500;
	rc.userdata = state;
	rc.applier = applier;
	rc.snapshooter = snapshooter;
	raft = raft_init(&rc);

	int peernum = 0;
	while ((opt = getopt(argc, argv, "hi:r:l:")) != -1) {
		switch (opt) {
			case 'i':
				myid = atoi(optarg);
				break;
			case 'r':
				if (myid == NOBODY) {
					usage(argv[0]);
					return EXIT_FAILURE;
				}

				str = strtok(optarg, ":");
				if (str) {
					id = atoi(str);
				} else {
					usage(argv[0]);
					return EXIT_FAILURE;
				}

				host = strtok(NULL, ":");

				str = strtok(NULL, ":");
				if (str) {
					port = atoi(str);
				} else {
					usage(argv[0]);
					return EXIT_FAILURE;
				}

				if (!raft_peer_up(raft, id, host, port, id == myid)) {
					usage(argv[0]);
					return EXIT_FAILURE;
				}
				if (id == myid) {
					myhost = host;
					myport = port;
				}
				peernum++;
				break;
			case 'l':
				logfilename = optarg;
				daemonize = true;
				break;
			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}
	if (!myhost) {
		usage(argv[0]);
		return EXIT_FAILURE;
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

	if (daemonize) {
		if (daemon(true, true) == -1) {
			shout("could not daemonize: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	signal(SIGTERM, die);
	signal(SIGINT, die);

	main_loop(myhost, myport);

	return EXIT_SUCCESS;
}
