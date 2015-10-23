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

void raft_update_apply(int action, int argument) {
	state[argument % STATELEN] = action;
}

void die(int signum) {
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
	shout("pid=%d [%d @ %d] %s(%d):%4dms:", getpid(), raft.me, raft.term, rolenames[raft.role], raft.log.acked, raft.timer);
	int i;
	for (i = 0; i < STATELEN; i++) {
		shout(" %d", state[i]);
	}
	shout("\n");
}

typedef struct mstimer_t {
	struct timeval tv;
} mstimer_t;

static int mstimer_reset(mstimer_t *t) {
	struct timeval newtime;
	gettimeofday(&newtime, NULL);

	int ms =
		(newtime.tv_sec - t->tv.tv_sec) * 1000 +
		(newtime.tv_usec - t->tv.tv_usec) / 1000;

	t->tv = newtime;

	return ms;
}

void usr1(int signum) {
	static int arg = 0;
	if (raft.role == ROLE_LEADER) {
		int action = rand() % 9 + 1;
		shout("got an USR1, state[%d] := %d\n", arg, action);
		raft_emit(&raft, action, arg);
		arg++;
	} else {
		shout("got an USR1 while not a leader, ignoring\n");
	}
}

static void main_loop() {
	mstimer_t t;
	mstimer_reset(&t);

	//create a UDP socket
	int s = raft_create_udp_socket(&raft);
	if (s == -1) {
		die(EXIT_FAILURE);
	}

	//keep listening for data
	while (true) {
		int ms = mstimer_reset(&t);
		raft_tick(&raft, ms);
		raft_msg_t *m = raft_recv_message(&raft);
		int applied = raft_apply(&raft, raft_update_apply);
		if (applied) {
			shout("applied %d updates\n", applied);
		}
		show_status();
		if (m) {
			raft_handle_message(&raft, m);
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

	raft_init(&raft);

	int opt;
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
	signal(SIGUSR1, usr1);

	main_loop();

	return EXIT_SUCCESS;
}
