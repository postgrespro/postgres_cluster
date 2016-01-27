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

#include "raft.h"
#include "util.h"

#define STATELEN 10
int state[STATELEN] = {0};

static void raft_update_apply(int action, int argument) {
	state[argument % STATELEN] = action;
}

static void die(int signum) {
	shout("terminated\n");
	exit(signum);
}

static void usage(char *prog) {
	printf(
		"Usage: %s -i ID -r HOST:PORT [-r HOST:PORT ...] [-l LOGFILE]\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n",
		prog
	);
}

raft_t raft;

char *rolenames[] = {"F", "C", "L"};

static void show_status() {
	int i;
	shout("pid=%d [%d @ %d] %s(%d):%4dms:", getpid(), raft.me, raft.term, rolenames[raft.role], raft.log.acked, raft.timer);
	for (i = 0; i < STATELEN; i++) {
		shout(" %d", state[i]);
	}
	shout("\n");
}

static void main_loop() {
	int s;
	int arg;

	mstimer_t t;
	mstimer_reset(&t);

	//create a UDP socket
	s = raft_create_udp_socket(&raft);
	if (s == -1) {
		die(EXIT_FAILURE);
	}

	arg = 0;
	while (true) {
		int ms;
		raft_msg_t *m;
		int applied;

		ms = mstimer_reset(&t);
		raft_tick(&raft, ms);
		m = raft_recv_message(&raft);
		applied = raft_apply(&raft, raft_update_apply);
		if (applied) {
			shout("applied %d updates\n", applied);
		}
		show_status();
		if (m) {
			raft_handle_message(&raft, m);
		}

		if ((raft.role == ROLE_LEADER) && (rand() % 10 == 0)) {
			int action = rand() % 9 + 1;
			shout("set state[%d] = %d\n", arg, action);
			raft_emit(&raft, action, arg);
			arg++;
		}
	}

	close(s);
}

int main(int argc, char **argv) {
	char *logfilename = NULL;
	bool daemonize = false;

	int myid = NOBODY;
	char *host;
	char *portstr;
	int port;
	int opt;

	raft_init(&raft);

	while ((opt = getopt(argc, argv, "hi:r:l:")) != -1) {
		switch (opt) {
			case 'i':
				myid = atoi(optarg);
				break;
			case 'r':
				host = strtok(optarg, ":");
				portstr = strtok(NULL, ":");
				if (portstr) {
					port = atoi(portstr);
				} else {
					port = DEFAULT_LISTENPORT;
				}

				if (!raft_add_server(&raft, host, port)) {
					usage(argv[0]);
					return EXIT_FAILURE;
				}
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

	if (!raft_set_myid(&raft, myid)) {
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

	main_loop();

	return EXIT_SUCCESS;
}
