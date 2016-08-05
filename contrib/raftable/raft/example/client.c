#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "timeout.h"
#include "proto.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif
#define MAX_SERVERS 64

#undef shout
#define shout(...) \
	do { \
		fprintf(stderr, "CLIENT: "); \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr); \
	} while (0)

#define debug(...)

typedef struct HostPort {
	bool up;
	char host[HOST_NAME_MAX + 1];
	int port;
} HostPort;

static int leader = -1;
static int leadersock = -1;
int servernum;
HostPort servers[MAX_SERVERS];

static void select_next_server(void) {
	int orig_leader = leader;
	for (int i = 0; i < MAX_SERVERS; i++) {
		int idx = (orig_leader + i + 1) % servernum;
		HostPort *hp = servers + idx;
		if (hp->up) {
			leader = idx;
			return;
		}
	}
	shout("all servers are down\n");
}

static bool poll_until_writable(int sock, timeout_t *timeout) {
	struct pollfd pfd = {sock, POLLOUT, 0};
	int r = poll(&pfd, 1, timeout_remaining_ms(timeout));
	if (r != 1) return false;
	return (pfd.revents & POLLOUT) != 0;
}

static bool poll_until_readable(int sock, timeout_t *timeout) {
	struct pollfd pfd = {sock, POLLIN, 0};
	int remain = timeout_remaining_ms(timeout);
	int r = poll(&pfd, 1, remain);
	if (r != 1) return false;
	return (pfd.revents & POLLIN) != 0;
}

static bool timed_write(int sock, void *data, size_t len, timeout_t *timeout) {
	int sent = 0;

	while (sent < len) {
		int newbytes;
		if (timeout_happened(timeout)) {
			debug("write timed out\n");
			return false;
		}

		newbytes = write(sock, (char *)data + sent, len - sent);
		if (newbytes > 0) {
			sent += newbytes;
		} else if (newbytes == 0) {
			return false;
		} else {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
				if (!poll_until_writable(sock, timeout)) {
					return false;
				}
			} else {
				debug("failed to write: error %d: %s\n", errno, strerror(errno));
				return false;
			}
		}
	}

	return true;
}

static bool timed_read(int sock, void *data, size_t len, timeout_t *timeout) {
	int recved = 0;

	while (recved < len) {
		int newbytes;
		if (timeout_happened(timeout)) {
			debug("read timed out\n");
			return false;
		}

		newbytes = read(sock, (char *)data + recved, len - recved);
		if (newbytes > 0) {
			recved += newbytes;
		} else if (newbytes == 0) {
			return false;
		} else {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
				if (!poll_until_readable(sock, timeout)) {
					return false;
				}
			} else {
				debug("failed to read: error %d: %s\n", errno, strerror(errno));
				return false;
			}
		}
	}

	return true;
}

static void wait_ms(int ms) {
	struct timespec ts = {ms / 1000, (ms % 1000) * 1000000};
	struct timespec rem;
	while (nanosleep(&ts, &rem) == -1) {
		if (errno != EINTR) break;
		ts = rem;
	}
}

static void disconnect_leader(void) {
	if (leadersock >= 0) {
		close(leadersock);
	}
	wait_ms(100);
	select_next_server();
	leadersock = -1;
}

static bool connect_leader(timeout_t *timeout) {
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;
	int rc;
	int sd;

	HostPort *leaderhp;

	if (leader == -1) select_next_server();

	leaderhp = servers + leader;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", leaderhp->port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;

	if ((rc = getaddrinfo(leaderhp->host, portstr, &hint, &addrs))) {
		disconnect_leader();
		shout(
			"failed to resolve address '%s:%d': %s\n",
			leaderhp->host, leaderhp->port,
			gai_strerror(rc)
		);
		return false;
	}

	debug("trying [%d] %s:%d\n", leader, leaderhp->host, leaderhp->port);
	for (a = addrs; a != NULL; a = a->ai_next) {
		int one = 1;

		sd = socket(a->ai_family, SOCK_STREAM, 0);
		if (sd == -1) {
			shout("failed to create a socket: %s\n", strerror(errno));
			continue;
		}
		fcntl(sd, F_SETFL, O_NONBLOCK);
		setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

		if (connect(sd, a->ai_addr, a->ai_addrlen) == -1) {
			if (errno == EINPROGRESS) {
				TIMEOUT_LOOP_START(timeout); {
					if (poll_until_writable(sd, timeout)) {
						int err;
						socklen_t optlen = sizeof(err);
						getsockopt(sd, SOL_SOCKET, SO_ERROR, &err, &optlen);
						if (err == 0) goto success;
					}
				} TIMEOUT_LOOP_END(timeout);
				shout("connect timed out\n");
				goto failure;
			}
			else
			{
				debug("failed to connect to an address: %s\n", strerror(errno));
				close(sd);
				continue;
			}
		}

		goto success;
	}
failure:
	freeaddrinfo(addrs);
	disconnect_leader();
	debug("could not connect\n");
	return false;
success:
	freeaddrinfo(addrs);
	leadersock = sd;
	return true;
}

static int get_connection(timeout_t *timeout) {
	if (leadersock < 0) {
		if (connect_leader(timeout)) return leadersock;
		debug("update: connect_leader() failed\n");
	}
	return leadersock;
}

static bool try_query(Message *msg, Message *answer, timeout_t *timeout) {
	int s = get_connection(timeout);
	if (s < 0) return false;

	if (timeout_happened(timeout)) {
		debug("try_query: get_connection() timed out\n");
		return false;
	}

	if (!timed_write(s, msg, sizeof(Message), timeout)) {
		debug("try_query: failed to send the query to the leader\n");
		return false;
	}

	if (!timed_read(s, answer, sizeof(Message), timeout)) {
		debug("try_query: failed to recv the answer from the leader\n");
		return false;
	}

	return true;
}

static bool query(Message *msg, Message *answer, int timeout_ms) {
	timeout_t timeout;
	if (timeout_ms < 0) {
		while (true) {
			timeout_start(&timeout, 100);

			if (try_query(msg, answer, &timeout)) {
				return true;
			} else {
				disconnect_leader();
			}
		}
	} else {
		timeout_start(&timeout, timeout_ms);

		TIMEOUT_LOOP_START(&timeout); {
			if (try_query(msg, answer, &timeout)) {
				return true;
			} else {
				disconnect_leader();
			}
		} TIMEOUT_LOOP_END(&timeout);
	}

	shout("query failed after %d ms\n", timeout_elapsed_ms(&timeout));
	return false;
}

static void msg_fill(Message *msg, char meaning, char *key, char *value) {
	msg->meaning = meaning;
	strncpy(msg->key.data, key, sizeof(msg->key));
	strncpy(msg->value.data, value, sizeof(msg->value));
}

static char *get(char *key, int timeout_ms) {
	Message msg, answer;

	msg_fill(&msg, MEAN_GET, key, "");

	if (query(&msg, &answer, timeout_ms)) {
		if (answer.meaning == MEAN_OK) {
			return strndup(answer.value.data, sizeof(answer.value));
		} else {
			assert(answer.meaning == MEAN_FAIL);
		}
	}
	return NULL;
}

static bool set(char *key, char *value, int timeout_ms) {
	Message msg, answer;

	msg_fill(&msg, MEAN_SET, key, value);

	if (query(&msg, &answer, timeout_ms)) {
		if (answer.meaning == MEAN_OK) {
			return true;
		} else {
			assert(answer.meaning == MEAN_FAIL);
		}
	}
	return false;
}

static void usage(char *prog) {
	printf(
		"Usage: %s -k KEY -r ID:HOST:PORT [-r ID:HOST:PORT ...]\n",
		prog
	);
}

char *key;
bool setup(int argc, char **argv) {
	servernum = 0;
	int opt;
	while ((opt = getopt(argc, argv, "hk:r:")) != -1) {
		int id;
		char *host;
		char *str;
		int port;

		switch (opt) {
			case 'k':
				key = optarg;
				break;
			case 'r':
				str = strtok(optarg, ":");
				if (!str) return false;
				id = atoi(str);

				host = strtok(NULL, ":");

				str = strtok(NULL, ":");
				if (!str) return false;
				port = atoi(str);

				HostPort *hp = servers + id;
				if (hp->up) return false;
				hp->up = true;
				strncpy(hp->host, host, sizeof(hp->host));
				hp->port = port;
				servernum++;
				break;
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			default:
				return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	signal(SIGPIPE, SIG_IGN);

	if (!setup(argc, argv)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	srand(getpid());

	while (true) {

		wait_ms(1000);
		char value[20];
		snprintf(value, sizeof(value), "%d", rand());
		shout("set(%s, %s)\n", key, value);
		if (set(key, value, 1000)) {
			char *reply = get(key, 1000);
			if (reply) {
				shout("%s = %s\n", key, reply);
				free(reply);
			}
		} else {
			shout("set() failed\n");
		}
	}

	return EXIT_SUCCESS;
}
