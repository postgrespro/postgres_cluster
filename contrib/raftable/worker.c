#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#include "postgres.h"
#include "postmaster/bgworker.h"
#include "miscadmin.h"

#include "raft.h"
#include "util.h"

#include "state.h"
#include "worker.h"
#include "client.h"

#define MAX_CLIENTS 1024
#define LISTEN_QUEUE_SIZE 10

typedef struct Server {
	char *host;
	int port;

	int listener;
	int raftsock;
	int id;

	int clientnum;
	Client clients[MAX_CLIENTS];
} Server;

static StateP state;
static Server server;
static raft_t raft;

static void applier(void *state, raft_update_t update, raft_bool_t snapshot)
{
	Assert(state);
	state_update(state, (RaftableMessage *)update.data, snapshot);
}

static raft_update_t snapshooter(void *state)
{
	raft_update_t shot;
	size_t shotlen;
	Assert(state);
	shot.data = state_make_snapshot(state, &shotlen);
	shot.len = shotlen;
	return shot;
}

static void add_peers(WorkerConfig *cfg)
{
	int i;
	for (i = 0; i < MAX_SERVERS; i++)
	{
		HostPort *hp = cfg->peers + i;
		if (!hp->up) continue;

		if (i == cfg->id)
		{
			raft_peer_up(raft, i, hp->host, hp->port, true);
			server.host = hp->host;
			server.port = hp->port;
			server.id = i;
		}
		else
			raft_peer_up(raft, i, hp->host, hp->port, false);
	}
}

/* Returns the created socket, or -1 if failed. */
static int create_listening_socket(const char *host, int port)
{
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

	if ((rc = getaddrinfo(host, portstr, &hint, &addrs)))
	{
		elog(WARNING, "failed to resolve address '%s:%d': %s",
			 host, port, gai_strerror(rc));
		return -1;
	}

	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1)
		{
			elog(WARNING, "cannot create the listening socket: %s", strerror(errno));
			continue;
		}

		optval = 1;
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));
		fcntl(s, F_SETFL, O_NONBLOCK);

		elog(DEBUG1, "binding tcp %s:%d\n", host, port);
		if (bind(s, a->ai_addr, a->ai_addrlen) < 0)
		{
			elog(WARNING, "cannot bind the listening socket: %s", strerror(errno));
			close(s);
			continue;
		}

		if (listen(s, LISTEN_QUEUE_SIZE) == -1)
		{
			elog(WARNING, "failed to listen the socket: %s", strerror(errno));
			close(s);
			continue;
		}
		return s;
	}
	elog(WARNING, "failed to find proper protocol");
	return -1;
}

static bool add_client(int sock)
{
	int i;

	if (server.clientnum >= MAX_CLIENTS)
	{
		elog(WARNING, "client limit hit\n");
		return false;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		Client *c = server.clients + i;
		if (c->state != CLIENT_DEAD) continue;

		c->socket = sock;
		c->state = CLIENT_SENDING;
		c->msg = NULL;
		c->cursor = 0;
		c->msglen = 0;
		c->expect = -1;
		server.clientnum++;
		return true;
	}

	Assert(false); // should not happen
	return false;
}

static bool remove_client(Client *c)
{
	Assert(c->socket >= 0);
	if (c->msg) pfree(c->msg);
	c->state = CLIENT_DEAD;

	server.clientnum--;
	close(c->socket);
	return true;
}

static bool start_server(void)
{
	int i;

	server.listener = -1;
	server.raftsock = -1;
	server.clientnum = 0;

	server.listener = create_listening_socket(server.host, server.port);
	if (server.listener == -1)
	{
		return false;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		server.clients[i].state = CLIENT_DEAD;
	}

	return true;
}

static bool accept_client(void)
{
	int fd;

	elog(DEBUG1, "a new connection is queued\n");

	fd = accept(server.listener, NULL, NULL);
	if (fd == -1) {
		elog(WARNING, "failed to accept a connection: %s\n", strerror(errno));
		return false;
	}
	elog(DEBUG1, "a new connection fd=%d accepted\n", fd);
	
	if (!raft_is_leader(raft)) {
		elog(DEBUG1, "not a leader, disconnecting the accepted connection fd=%d\n", fd);
		close(fd);
		return false;
	}

	return add_client(fd);
}

static void on_message_recv(Client *c)
{
	int index;
	raft_update_t u;
	RaftableMessage *rm;

	Assert(c->state == CLIENT_SENDING);
	Assert(c->msg != NULL);
	Assert(c->cursor == c->msg->len);
	Assert(c->expect == -1);

	rm = (RaftableMessage *)c->msg->data;
	if (rm->meaning == MEAN_SET)
	{
		u.len = c->msg->len;
		u.data = (char *)rm;
		index = raft_emit(raft, u); /* raft will copy the data */
		pfree(c->msg);
		c->msg = NULL;
		if (index < 0)
		{
			elog(WARNING, "failed to emit a raft update");
			c->state = CLIENT_SICK;
		}
		else
		{
			elog(DEBUG1, "emitted raft update %d", index);
			c->expect = index;
			c->state = CLIENT_WAITING;
		}
	}
	else if (rm->meaning == MEAN_GET)
	{
		char *key;
		char *value;
		size_t vallen;
		size_t answersize;
		RaftableField *f;
		RaftableMessage *answer;
		Assert(rm->fieldnum == 1);
		f = (RaftableField *)rm->data;
		Assert(f->vallen == 0);
		key = f->data;

		value = state_get(state, key, &vallen);
		answer = make_single_value_message(key, value, vallen, &answersize);
		answer->meaning = MEAN_OK;
		if (value) pfree(value);
		pfree(c->msg);
		c->msg = palloc(sizeof(Message) + answersize);
		c->msg->len = answersize;
		memcpy(c->msg->data, answer, answersize);
		pfree(answer);
		c->cursor = 0;
		c->state = CLIENT_RECVING;
	}
	else
	{
		elog(WARNING, "unknown meaning %d (%c) of the client's message\n", rm->meaning, rm->meaning);
		c->state = CLIENT_SICK;
	}
}

static void on_message_send(Client *c)
{
	Assert(c->state == CLIENT_RECVING);
	Assert(c->msg != NULL);
	Assert(c->cursor == c->msg->len + sizeof(c->msg->len));
	pfree(c->msg);
	c->msg = NULL;
	c->state = CLIENT_SENDING;
	c->cursor = 0;
}

static void attend(Client *c)
{
	Assert(c->state != CLIENT_DEAD);
	Assert(c->state != CLIENT_SICK);
	Assert(c->state != CLIENT_WAITING);

	switch (c->state)
	{
		case CLIENT_SENDING:
			client_recv(c);
			if (c->state == CLIENT_SICK) return;
			if (!c->msg) return;
			if (c->cursor < c->msg->len) return;
			elog(DEBUG1, "got %d bytes from client", (int)c->cursor);
			on_message_recv(c);
			break;
		case CLIENT_RECVING:
			client_send(c);
			if (c->state == CLIENT_SICK) return;
			if (c->cursor < c->msg->len) return;
			on_message_send(c);
			break;
		default:
			Assert(false); // should not happen
	}
}

static void notify(void)
{
	int i = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		size_t answersize;
		RaftableMessage *answer;
		Client *c = server.clients + i;
		if (c->state != CLIENT_WAITING) continue;
		Assert(c->expect >= 0);
		if (!raft_applied(raft, server.id, c->expect)) continue;

		elog(DEBUG1, "notify client %d that update %d is applied", i, c->expect);
		answer = make_single_value_message("", NULL, 0, &answersize);
		answer->meaning = MEAN_OK;
		c->msg = palloc(sizeof(Message) + answersize);
		c->msg->len = answersize;
		memcpy(c->msg->data, answer, answersize);
		c->cursor = 0;
		pfree(answer);
		c->state = CLIENT_RECVING;
		c->expect = -1;
	}
}

static void drop_bads(void)
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		Client *c = server.clients + i;
		if (c->state == CLIENT_DEAD) continue;
		if ((c->state == CLIENT_SICK) || !raft_is_leader(raft))
			remove_client(c);
	}
}

static void add_to_fdset(int fd, fd_set *fdset, int *maxfd)
{
	Assert(fd >= 0);
	FD_SET(fd, fdset);
	if (fd > *maxfd) *maxfd = fd;
}

static bool tick(int timeout_ms)
{
	int i;
	int numready = 0;
	Client *c;
	bool raft_ready = false;
	int maxfd = 0;

	mstimer_t timer;

	fd_set readfds;
	fd_set writefds;

	drop_bads();

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	add_to_fdset(server.listener, &readfds, &maxfd);
	add_to_fdset(server.raftsock, &readfds, &maxfd);
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		Client *c = server.clients + i;
		switch (c->state)
		{
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

	if (FD_ISSET(server.listener, &readfds))
	{
		numready--;
		accept_client();
	}

	if (FD_ISSET(server.raftsock, &readfds))
	{
		numready--;
		raft_ready = true;
	}

	c = server.clients;
	while (numready > 0)
	{
		Assert(c - server.clients < MAX_CLIENTS);
		switch (c->state)
		{
			case CLIENT_SENDING:
				Assert(c->socket >= 0);
				if (FD_ISSET(c->socket, &readfds))
				{
					attend(c);
					numready--;
				}
				break;
			case CLIENT_RECVING:
				Assert(c->socket >= 0);
				if (FD_ISSET(c->socket, &writefds))
				{
					attend(c);
					numready--;
				}
				break;
			default:
				break;
		}
		c++;
	}

	return raft_ready;
}


static int stop = 0;
static void die(int sig)
{
    stop = 1;
}

void raftable_worker_main(Datum arg)
{
	sigset_t sset;
	mstimer_t t;
	WorkerConfig *cfg = *(WorkerConfig **)arg;
	state = state_init();

	cfg->raft_config.userdata = state;
	cfg->raft_config.applier = applier;
	cfg->raft_config.snapshooter = snapshooter;

	raft = raft_init(&cfg->raft_config);
	if (raft == NULL)
		elog(ERROR, "couldn't configure raft");

	add_peers(cfg);

	if (!start_server()) elog(ERROR, "couldn't start raftable server");

    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGTERM, die);
    sigfillset(&sset);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);

	BackgroundWorkerUnblockSignals();

	server.raftsock = raft_create_udp_socket(raft);
	if (server.raftsock == -1) elog(ERROR, "couldn't start raft");

	mstimer_reset(&t);
	while (!stop)
	{
		raft_msg_t m = NULL;

		int ms = mstimer_reset(&t);
		raft_tick(raft, ms);

		if (tick(cfg->raft_config.heartbeat_ms))
		{
			m = raft_recv_message(raft);
			Assert(m != NULL);
			raft_handle_message(raft, m);
			notify();
		}
		CHECK_FOR_INTERRUPTS();
	}
	elog(LOG, "Raftable worker stopped");
	exit(1); /* automatically restart raftable */
}
