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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdbool.h>

#include <jansson.h>

#include "raft.h"
#include "util.h"
#include "proto.h"

#undef shout
#define shout(...) \
	do { \
		fprintf(stderr, "SERVER: "); \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr); \
	} while (0)

#define LISTEN_QUEUE_SIZE 10
#define MAX_CLIENTS 512

/* Client state machine:
 *
 *    ┏━━━━━━━━━┓   ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
 * ──>┃  Dead   ┃<──┨                Sick                 ┃
 *    ┗━━━━┯━━━━┛   ┗━━━━┯━━━━━━━━━━━━━┯━━━━━━━━━━━━━┯━━━━┛
 *         │conn         ^ fail        ^ fail        ^ fail
 *         └───────>┏━━━━┷━━━━┓fin┏━━━━┷━━━━┓fin┏━━━━┷━━━━┓fin
 *                  ┃ Sending ┠──>┃ Waiting ┠──>┃ Recving ┠─┐
 *               ┌─>┗━━━━━━━━━┛   ┗━━━━━━━━━┛   ┗━━━━━━━━━┛ │
 *               └──────────────────────────────────────────┘
 */

typedef enum ClientState {
	CLIENT_SICK = -1,
	CLIENT_DEAD = 0,
	CLIENT_SENDING,
	CLIENT_WAITING,
	CLIENT_RECVING
} ClientState;

typedef struct Client {
	ClientState state;
	int socket;
	size_t cursor;
	Message msg; /* the message that is currently being sent or received */
	int expect;
} Client;

typedef struct Server {
	char *host;
	int port;

	int listener;
	int raftsock;
	int id;

	int clientnum;
	Client clients[MAX_CLIENTS];
} Server;

static bool continue_recv(int socket, void *dst, size_t len, size_t *done) {
	while (*done < len) {
		ssize_t recved = recv(socket, ((char *)dst) + *done, len - *done, MSG_DONTWAIT);
		if (recved == 0) return false;
		if (recved < 0) {
			switch (errno) {
				case EAGAIN:
				#if EAGAIN != EWOULDBLOCK
				case EWOULDBLOCK:
				#endif
					return true; /* try again later */
				case EINTR:
					continue; /* try again now */
				default:
					return false;
			}
		}
		*done += recved;
		assert(*done <= len);
	}
	return true;
}

static bool continue_send(int socket, void *src, size_t len, size_t *done) {
	while (*done < len) {
		ssize_t sent = send(socket, ((char *)src) + *done, len - *done, MSG_DONTWAIT);
		if (sent == 0) return false;
		if (sent < 0) {
			switch (errno) {
				case EAGAIN:
				#if EAGAIN != EWOULDBLOCK
				case EWOULDBLOCK:
				#endif
					return true; /* try again later */
				case EINTR:
					continue; /* try again now */
				default:
					return false;
			}
		}
		*done += sent;
		assert(*done <= len);
	}
	return true;
}

void client_recv(Client *client) {
	assert(client->state == CLIENT_SENDING);

	if (client->cursor < sizeof(client->msg)) {
		if (!continue_recv(client->socket, &client->msg, sizeof(client->msg), &client->cursor)) {
			goto failure;
		}
	}

	return;
failure:
	client->state = CLIENT_SICK;
}

void client_send(Client *client) {
	assert(client->state == CLIENT_RECVING);

	if (client->cursor < sizeof(client->msg)) {
		if (!continue_send(client->socket, &client->msg, sizeof(client->msg), &client->cursor)) {
			goto failure;
		}
	}

	return;
failure:
	client->state = CLIENT_SICK;
}

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
			"applied %s: the new state is %s\n",
			snapshot ? "a snapshot" : "an update",
			encoded
		);
	} else {
		shout(
			"applied %s, but the new state could not be encoded\n",
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

static int stop = 0;
static void die(int sig)
{
	stop = 1;
}

static void usage(char *prog) {
	printf(
		"Usage: %s -i ID -r ID:HOST:PORT [-r ID:HOST:PORT ...] [-l LOGFILE]\n"
		"   -l : Run as a daemon and write output to LOGFILE.\n",
		prog
	);
}

json_t *state;
Server server;
raft_t raft;

static bool add_client(int sock) {
	if (server.clientnum >= MAX_CLIENTS) {
		shout("client limit hit\n");
		return false;
	}

	for (int i = 0; i < MAX_CLIENTS; i++) {
		Client *c = server.clients + i;
		if (c->state != CLIENT_DEAD) continue;

		c->socket = sock;
		c->state = CLIENT_SENDING;
		c->cursor = 0;
		c->expect = -1;
		server.clientnum++;
		return true;
	}

	assert(false); // should not happen
	return false;
}

static bool remove_client(Client *c) {
	assert(c->socket >= 0);
	c->state = CLIENT_DEAD;

	server.clientnum--;
	close(c->socket);
	return true;
}

/* Returns the created socket, or -1 if failed. */
static int create_listening_socket(const char *host, int port) {
	int optval;
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	struct addrinfo *a;
	char portstr[6];
	int rc;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;

	if ((rc = getaddrinfo(host, portstr, &hint, &addrs))) {
		shout("failed to resolve address '%s:%d': %s\n",
			 host, port, gai_strerror(rc));
		return -1;
	}

	for (a = addrs; a != NULL; a = a->ai_next) {
		int s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			shout("cannot create the listening socket: %s\n", strerror(errno));
			continue;
		}

		optval = 1;
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));

		shout("binding tcp %s:%d\n", host, port);
		if (bind(s, a->ai_addr, a->ai_addrlen) < 0) {
			shout("cannot bind the listening socket: %s\n", strerror(errno));
			close(s);
			continue;
		}

		if (listen(s, LISTEN_QUEUE_SIZE) == -1)
		{
			shout("failed to listen the socket: %s\n", strerror(errno));
			close(s);
			continue;
		}
		return s;
	}
	shout("failed to find proper protocol\n");
	return -1;
}


static bool start_server(void) {
	server.listener = -1;
	server.raftsock = -1;
	server.clientnum = 0;

	server.listener = create_listening_socket(server.host, server.port);
	if (server.listener == -1) {
		return false;
	}

	for (int i = 0; i < MAX_CLIENTS; i++) {
		server.clients[i].state = CLIENT_DEAD;
	}

	return true;
}

static bool accept_client(void) {
	debug("a new connection is queued\n");

	int fd = accept(server.listener, NULL, NULL);
	if (fd == -1) {
		shout("failed to accept a connection: %s\n", strerror(errno));
		return false;
	}
	debug("a new connection fd=%d accepted\n", fd);

	if (!raft_is_leader(raft)) {
		debug("not a leader, disconnecting the accepted connection fd=%d\n", fd);
		close(fd);
		return false;
	}

	return add_client(fd);
}

static void on_message_from(Client *c) {
	int index;

	assert(c->state == CLIENT_SENDING);
	assert(c->cursor == sizeof(c->msg));
	assert(c->expect == -1);

	c->cursor = 0;

	if (c->msg.meaning == MEAN_SET) {
		char buf[sizeof(c->msg) + 10];
		snprintf(buf, sizeof(buf), "{\"%s\": \"%s\"}", c->msg.key.data, c->msg.value.data);
		debug("emit update: %s\n", buf);
		raft_update_t update = {strlen(buf), buf, NULL};
		index = raft_emit(raft, update); /* raft will copy the data */
		if (index < 0) {
			shout("failed to emit a raft update\n");
			c->state = CLIENT_SICK;
		} else {
			debug("client is waiting for %d\n", index);
			c->expect = index;
			c->state = CLIENT_WAITING;
		}
	} else if (c->msg.meaning == MEAN_GET) {
		json_t *jval = json_object_get(state, c->msg.key.data);
		if (jval == NULL) {
			c->msg.meaning = MEAN_FAIL;
		} else {
			c->msg.meaning = MEAN_OK;
			strncpy(c->msg.value.data, json_string_value(jval), sizeof(c->msg.value));
		}
		c->state = CLIENT_RECVING;
	} else {
		shout("unknown meaning %d of the client's message\n", c->msg.meaning);
		c->state = CLIENT_SICK;
	}
}

static void on_message_to(Client *c) {
	assert(c->state == CLIENT_RECVING);
	assert(c->cursor == sizeof(c->msg));
	c->cursor = 0;
	c->state = CLIENT_SENDING;
}

static void attend(Client *c) {
	assert(c->state != CLIENT_DEAD);
	assert(c->state != CLIENT_SICK);
	assert(c->state != CLIENT_WAITING);

	switch (c->state) {
		case CLIENT_SENDING:
			client_recv(c);
			if (c->state == CLIENT_SICK) return;
			if (c->cursor < sizeof(Message)) return;
			on_message_from(c);
			break;
		case CLIENT_RECVING:
			client_send(c);
			if (c->state == CLIENT_SICK) return;
			if (c->cursor <  sizeof(Message)) return;
			on_message_to(c);
			break;
		default:
			assert(false); // should not happen
	}
}

static void notify(void) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Client *c = server.clients + i;
		if (c->state != CLIENT_WAITING) continue;
		assert(c->expect >= 0);
		if (!raft_applied(raft, server.id, c->expect)) continue;

		c->msg.meaning = MEAN_OK;
		c->cursor = 0;
		c->state = CLIENT_RECVING;
		c->expect = -1;
	}
}

static void drop_bads(void) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Client *c = server.clients + i;
		if (c->state == CLIENT_DEAD) continue;
		if ((c->state == CLIENT_SICK) || !raft_is_leader(raft)) {
			remove_client(c);
		}
	}
}

static void add_to_fdset(int fd, fd_set *fdset, int *maxfd) {
	assert(fd >= 0);
	FD_SET(fd, fdset);
	if (fd > *maxfd) *maxfd = fd;
}

static bool tick(int timeout_ms) {
	drop_bads();

	bool raft_ready = false;

	fd_set readfds;
	fd_set writefds;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	int maxfd = 0;
	add_to_fdset(server.listener, &readfds, &maxfd);
	add_to_fdset(server.raftsock, &readfds, &maxfd);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Client *c = server.clients + i;
		switch (c->state) {
			case CLIENT_SENDING:
				add_to_fdset(c->socket, &readfds, &maxfd);
				break;
			case CLIENT_RECVING:
				add_to_fdset(c->socket, &writefds, &maxfd);
				break;
			default:
				continue;
		}
	}

	int numready = 0;
	mstimer_t timer;
	mstimer_reset(&timer);
	while (timeout_ms > 0) {
		struct timeval timeout = ms2tv(timeout_ms);
		numready = select(maxfd + 1, &readfds, &writefds, NULL, &timeout);
		timeout_ms -= mstimer_reset(&timer);
		if (numready >= 0) break;
		if (errno == EINTR) {
			continue;
		}
		shout("failed to select: %s\n", strerror(errno));
		return false;
	}

	if (FD_ISSET(server.listener, &readfds)) {
		numready--;
		accept_client();
	}

	if (FD_ISSET(server.raftsock, &readfds)) {
		numready--;
		raft_ready = true;
	}

	Client *c = server.clients;
	while (numready > 0) {
		assert(c - server.clients < MAX_CLIENTS);
		switch (c->state) {
			case CLIENT_SENDING:
				assert(c->socket >= 0);
				if (FD_ISSET(c->socket, &readfds)) {
					attend(c);
					numready--;
				}
				break;
			case CLIENT_RECVING:
				assert(c->socket >= 0);
				if (FD_ISSET(c->socket, &writefds)) {
					attend(c);
					numready--;
				}
				break;
			default:
				break;
		}
		c++;
	}
	assert(numready == 0);

	return raft_ready;
}

int main(int argc, char **argv) {
	char *logfilename = NULL;
	bool daemonize = false;

	int id;
	char *host;
	char *str;
	int port;
	int opt;

	server.id = NOBODY;
	server.host = NULL;

	state = json_object();

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
				server.id = atoi(optarg);
				break;
			case 'r':
				if (server.id == NOBODY) {
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

				if (!raft_peer_up(raft, id, host, port, id == server.id)) {
					usage(argv[0]);
					return EXIT_FAILURE;
				}
				if (id == server.id) {
					server.host = host;
					server.port = port;
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
	if (!server.host) {
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

	if (!start_server()) {
		shout("couldn't start the server\n");
		return EXIT_FAILURE;
	}

	signal(SIGTERM, die);
	signal(SIGQUIT, die);
	signal(SIGINT, die);
	sigset_t sset;
	sigfillset(&sset);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);

	server.raftsock = raft_create_udp_socket(raft);
	if (server.raftsock == -1) {
		shout("couldn't start raft\n");
		return EXIT_FAILURE;
	}

	mstimer_t t;
	mstimer_reset(&t);
	while (!stop)
	{
		raft_msg_t m = NULL;

		int ms = mstimer_reset(&t);
		raft_tick(raft, ms);

		if (tick(rc.heartbeat_ms))
		{
			m = raft_recv_message(raft);
			assert(m != NULL);
			raft_handle_message(raft, m);
			notify();
		}
	}

	return EXIT_SUCCESS;
}
