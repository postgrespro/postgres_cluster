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
#include <arpa/inet.h>

#include "util.h"

#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

#define STATUS_UNBORN 0
#define STATUS_ALIVE  1
#define STATUS_DEAD   2

#define MAX_HEARTS 10
#define BUFLEN 2048
#define MAX_EVENTS 1024
#define BEAT_TIMEOUT_MS 500
#define RECV_TIMEOUT_MS 100
#define MAX_BEATS_MISSED 10

typedef struct heart_t {
	char *host;
	int port;

	int seqno;
	int recved_at;
	int status;

	struct sockaddr_in addr;
} heart_t;

typedef struct heart_beat_t {
	int from;
	int seqno;
} heart_beat_t;

void die(int signum) {
	shout("terminated\n");
	exit(signum);
}

static void usage(char *prog) {
	printf(
		"Usage: %s -i ID -r HOST:PORT [-r HOST:PORT ...] [-l LOGFILE]\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n"
		"   -k : Just kill the other arbiter and exit.\n",
		prog
	);
}

int heartnum = 0;
heart_t hearts[MAX_HEARTS];
int myid = -1;

static void socket_set_recv_timeout(int sock, int ms) {
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = ((ms % 1000) * 1000);
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
		shout("failed to set socket recv timeout: %s\n", strerror(errno));
		die(EXIT_FAILURE);
	}
}

static void socket_set_reuseaddr(int sock) {
	int optval = 1;
	if (setsockopt(
		sock, SOL_SOCKET, SO_REUSEADDR,
		(char const*)&optval, sizeof(optval)
	) == -1) {
		shout("failed to set socket to reuseaddr: %s\n", strerror(errno));
		die(EXIT_FAILURE);
	}

}

// Returns the created socket, or -1 if failed.
static int create_udp_socket(heart_t *h) {
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1) {
		shout("cannot create the listening socket: %s\n", strerror(errno));
		return -1;
	}

	socket_set_reuseaddr(s);
	socket_set_recv_timeout(s, RECV_TIMEOUT_MS);

	// zero out the structure
	memset((char*)&h->addr, 0, sizeof(h->addr));

	h->addr.sin_family = AF_INET;
	if (inet_aton(h->host, &h->addr.sin_addr) == 0) {
		shout("cannot convert the host string '%s' to a valid address\n", h->host);
		return -1;
	}
	h->addr.sin_port = htons(h->port);
	debug("binding %s:%d\n", h->host, h->port);
	if (bind(s, (struct sockaddr*)&h->addr, sizeof(h->addr)) == -1) {
		shout("cannot bind the socket: %s\n", strerror(errno));
		return -1;
	}

	return s;
}

static void send_a_beat(int sock) {
	heart_t *me = hearts + myid;
	me->seqno++;
	heart_beat_t beat = {myid, me->seqno};

	int i;
	for (i = 0; i < heartnum; i++) {
		if (i == myid) continue;

		heart_t *h = hearts + i;
		if (h->status == STATUS_DEAD) {
			continue;
		}

		unsigned int addrlen = sizeof(h->addr);

		int sent = sendto(
			sock, &beat, sizeof(beat), 0,
			(struct sockaddr*)&h->addr, addrlen
		);
		if (sent == -1) {
			shout(
				"failed to send a beat to [%d]: %s\n",
				i, strerror(errno)
			);
		}
	}
}

static void recv_a_beat(int sock) {
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(addr);

	heart_beat_t beat;

	//try to receive some data, this is a blocking call
	int recved = recvfrom(
		sock, &beat, sizeof(beat), 0,
		(struct sockaddr*)&addr, &addrlen
	);

	if (recved == -1) {
		if (
			(errno == EAGAIN) ||
			(errno == EWOULDBLOCK) ||
			(errno == EINTR)
		) {
			return;
		} else {
			shout("failed to recv: %s\n", strerror(errno));
			die(EXIT_FAILURE);
		}
	}

	if (recved != sizeof(beat)) {
		shout(
			"a corrupt beat recved from %s:%d\n",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port)
		);
		return;
	}

	debug(
		"received a beat from [%d] %s:%d\n",
		beat.from,
		inet_ntoa(addr.sin_addr),
		ntohs(addr.sin_port)
	);

	if ((beat.from < 0) || (beat.from >= heartnum)) {
		shout(
			"the 'from' is out of range (%d)\n",
			beat.from
		);
	}

	if (beat.from == myid) {
		shout("the beat is from myself O_o\n");
	}

	heart_t *h = hearts + beat.from;

	if (h->status == STATUS_DEAD) {
		shout("the beat is from a dead heart\n");
		return;
	}

	if (memcmp(&h->addr.sin_addr, &addr.sin_addr, sizeof(struct in_addr))) {
		shout(
			"the beat is from a wrong address %s = %d"
			" (expected from %s = %d)\n",
			inet_ntoa(h->addr.sin_addr),
			h->addr.sin_addr.s_addr,
			inet_ntoa(addr.sin_addr),
			addr.sin_addr.s_addr
		);
	}

	if (h->addr.sin_port != addr.sin_port) {
		shout(
			"the beat is from a wrong port %d"
			" (expected from %d)\n",
			ntohs(h->addr.sin_port),
			ntohs(addr.sin_port)
		);
	}

	if ((h->status == STATUS_UNBORN) || (beat.seqno > h->seqno)) {
		h->seqno = beat.seqno;
		h->recved_at = hearts[myid].seqno;
		h->status = STATUS_ALIVE;
	}
}

static void check_pulse() {
	heart_t *me = hearts + myid;
	int i;
	for (i = 0; i < heartnum; i++) {
		heart_t *h = hearts + i;
		int missed = me->seqno - h->recved_at;
		if (i == myid) {
			// ignore myself and non-living
			continue;
		}
		if (h->status == STATUS_ALIVE) {
			if (missed > MAX_BEATS_MISSED) {
				h->status = STATUS_DEAD;
			}
		}
	}
}

char *statusnames[] = {"unborn", "living", " dead "};

static void show_status() {
	shout("----- status[%d] -----\n", myid);
	heart_t *me = hearts + myid;

	int i;
	for (i = 0; i < heartnum; i++) {
		heart_t *h = hearts + i;
		int missed = me->seqno - h->recved_at;
		if (i == myid) {
			shout("[%d] (  me  )\n", i);
		} else {
			shout(
				"[%d] (%s) seqno %d, recved_at %d, missed %d\n",
				i,
				statusnames[h->status],
				h->seqno, h->recved_at, missed
			);
		}
	}
}

static void heart_loop() {
	int ms_from_last_beat = 0;

	struct timeval oldtime;
	gettimeofday(&oldtime, NULL);

	//create a UDP socket
	int s = create_udp_socket(hearts + myid);
	if (s == -1) {
		die(EXIT_FAILURE);
	}

	//keep listening for data
	while (true) {
		struct timeval newtime;
		gettimeofday(&newtime, NULL);
		ms_from_last_beat += (newtime.tv_sec - oldtime.tv_sec) * 1000;
		ms_from_last_beat += (newtime.tv_usec - oldtime.tv_usec) / 1000;
		oldtime = newtime;

		if (ms_from_last_beat > BEAT_TIMEOUT_MS) {
			ms_from_last_beat -= BEAT_TIMEOUT_MS;
			show_status();
			send_a_beat(s);
		}
		if (ms_from_last_beat > BEAT_TIMEOUT_MS) {
			shout("a freeze detected, fixing the timeouts\n");
			ms_from_last_beat = 0;
		}

		recv_a_beat(s);
		check_pulse();
	}

	close(s);
}

int main(int argc, char **argv) {
	char *logfilename = NULL;
	bool daemonize = false;

	char *portstr;
	heart_t *h;

	int opt;
	while ((opt = getopt(argc, argv, "hi:r:l:")) != -1) {
		switch (opt) {
			case 'i':
				myid = atoi(optarg);

				break;
			case 'r':
				h = hearts + heartnum;

				h->host = DEFAULT_LISTENHOST;
				h->port = DEFAULT_LISTENPORT;
				h->seqno = -MAX_BEATS_MISSED;
				h->recved_at = 0;

				h->host = strtok(optarg, ":");
				portstr = strtok(NULL, ":");
				if (portstr) {
					h->port = atoi(portstr);
				} else {
					h->port = DEFAULT_LISTENPORT;
				}

				if (inet_aton(h->host, &h->addr.sin_addr) == 0) {
					shout(
						"cannot convert the host string '%s'"
						" to a valid address\n", h->host
					);
					return EXIT_FAILURE;
				}
				h->addr.sin_port = htons(h->port);

				heartnum++;
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

	if (myid < 0) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (myid >= heartnum) {
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

	heart_loop();

	return EXIT_SUCCESS;
}
