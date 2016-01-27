/*
 * This module is used as a layer of abstraction between the main logic and the
 * event library. This should, theoretically, allow us to switch to another
 * library with less effort.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "server.h"
#include "arbiterlimits.h"
#include "util.h"
#include "sockhub.h"

typedef struct buffer_t {
	int ready; /* number of bytes that are ready to be sent/processed */
	ShubMessageHdr *curmessage;
	char *data; /* dynamically allocated buffer */
} buffer_t;

typedef struct stream_data_t *stream_t;

typedef struct client_data_t {
	stream_t stream; /* NULL: client value is empty */
	void *userdata;
	unsigned int chan;
} client_data_t;

typedef struct stream_data_t {
	int fd;
	bool good; /* 'false': stop serving this stream and disconnect when possible */
	buffer_t input;
	buffer_t output;

	/* a map: 'chan' -> client_data_t */
	/* 'chan' is expected to be < MAX_FDS which is pretty low */
	client_data_t *clients; /* dynamically allocated */
	struct stream_data_t* next;
} stream_data_t;

typedef struct server_data_t {
	char *host;
	int port;

	int listener; /* the listening socket */
#ifdef USE_EPOLL
	int epollfd;
#else
	fd_set all; /* all sockets including the listener */
	int maxfd;
#endif
	stream_t used_chain;
	stream_t free_chain;

	onmessage_callback_t onmessage;
	onconnect_callback_t onconnect;
	ondisconnect_callback_t ondisconnect;

	bool enabled;

	stream_data_t raft_stream;
} server_data_t;

/* Returns the created socket, or -1 if failed. */
static int create_listening_socket(const char *host, int port) {
	int optval;
	struct sockaddr_in addr;
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		shout("cannot create the listening socket: %s\n", strerror(errno));
		return -1;
	}

	optval = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));
	optval = SOCKET_BUFFER_SIZE;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*) &optval, sizeof(int));
	optval = SOCKET_BUFFER_SIZE;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*) &optval, sizeof(int));

	addr.sin_family = AF_INET;
	if (inet_aton(host, &addr.sin_addr) == 0) {
		shout("cannot convert the host string '%s' to a valid address\n", host);
		return -1;
	}
	addr.sin_port = htons(port);
	debug("binding tcp %s:%d\n", host, port);
	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		shout("cannot bind the listening socket: %s\n", strerror(errno));
		return -1;
	}

	if (listen(s, LISTEN_QUEUE_SIZE) == -1) {
		shout("failed to listen the socket: %s\n", strerror(errno));
		return -1;
	}    

	return s;
}

server_t server_init(
	char *host,
	int port,
	onmessage_callback_t onmessage,
	onconnect_callback_t onconnect,
	ondisconnect_callback_t ondisconnect
) {
	server_t server = malloc(sizeof(server_data_t));
	assert(server);
	server->host = host;
	server->port = port;
	server->onmessage = onmessage;
	server->onconnect = onconnect;
	server->ondisconnect = ondisconnect;

#ifdef USE_EPOLL
    server->epollfd = epoll_create(MAX_EVENTS);
    if (server->epollfd < 0) { 
		free(server);
		return NULL;
	}
#else
	FD_ZERO(&server->all);
	server->maxfd = 0;
#endif

	server->raft_stream.fd = -1;
	server->raft_stream.good = false;
	server->raft_stream.input.ready = false;
	server->raft_stream.input.data = NULL;
	server->raft_stream.output.ready = false;
	server->raft_stream.output.data = NULL;
	server->raft_stream.output.data = NULL;
	server->raft_stream.clients = NULL;
	server->raft_stream.next = NULL;
	server->enabled = false;

	return server;
}

/* Pass NULL instead of stream if the socket is not associated with any. */
static bool server_add_socket(server_t server, int sock, stream_t stream) {
#ifdef USE_EPOLL
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = (void*)stream;        
    if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        return false;
    } 
#else
	FD_SET(sock, &server->all);
	if (sock > server->maxfd) {
		server->maxfd = sock;
	}
#endif
	return true;
}

static bool server_remove_socket(server_t server, int sock) {
#ifdef USE_EPOLL
	if (epoll_ctl(server->epollfd, EPOLL_CTL_DEL, sock, NULL) < 0) {
		return false;
	}
#else
	FD_CLR(sock, &server->all);
#endif
	return true;
}

void server_set_raft_socket(server_t server, int sock) {
	server->raft_stream.fd = sock;
	bool good = server_add_socket(server, sock, &server->raft_stream);
	server->raft_stream.good = good;
}

bool server_start(server_t server) {
	debug("starting the server\n");
	server->free_chain = NULL;
	server->used_chain = NULL;
	
	server->listener = create_listening_socket(server->host, server->port);
	if (server->listener == -1) {
		return false;
	}

	return server_add_socket(server, server->listener, NULL);
}

static bool stream_flush(stream_t stream) {
	char *cursor;
	ShubMessageHdr *msg;
	int tosend = stream->output.ready;
	if (tosend == 0) {
		/* nothing to do */
		return true;
	}

	cursor = stream->output.data;
	while (tosend > 0) {
		/* repeat sending until we send everything */
		int sent = send(stream->fd, cursor, tosend, 0);
		if (sent == -1) {
			shout("failed to flush the stream\n");
			stream->good = false;
			return false;
		}
		cursor += sent;
		tosend -= sent;
		assert(tosend >= 0);
	}

	stream->output.ready = 0;
	msg = stream->output.curmessage;
	if (msg) {
		/* move the unfinished message to the start of the buffer */
		memmove(stream->output.data, msg, msg->size + sizeof(ShubMessageHdr));
		stream->output.curmessage = (ShubMessageHdr*)stream->output.data;
	}

	return true;
}

static void server_flush(server_t server) {
	stream_t s;
	debug("flushing the streams\n");
	for (s = server->used_chain; s != NULL; s = s->next) { 
		stream_flush(s);
	}
}

static void stream_init(stream_t stream, int fd) {
	int i;
	stream->input.data = malloc(BUFFER_SIZE);
	assert(stream->input.data);
	stream->input.curmessage = NULL;
	stream->input.ready = 0;

	stream->output.data = malloc(BUFFER_SIZE);
	assert(stream->output.data);
	stream->output.curmessage = NULL;
	stream->output.ready = 0;

	stream->fd = fd;
	stream->good = true;

	stream->clients = malloc(MAX_TRANSACTIONS * sizeof(client_data_t));
	assert(stream->clients);
	/* mark all clients as empty */
	for (i = 0; i < MAX_TRANSACTIONS; i++) {
		stream->clients[i].stream = NULL;
	}
}

static void server_stream_destroy(server_t server, stream_t stream) {
	int c;
	for (c = 0; c < MAX_TRANSACTIONS; c++) {
		client_t client = stream->clients + c;
		if (client->stream) {
			server->ondisconnect(client);
			if (client->userdata) {
				shout(
					"client still has userdata after 'ondisconnect' call,\n"
					"please set it to NULL in 'ondisconnect' callback\n"
				);
			}
		}
	}

	server_remove_socket(server, stream->fd);
	close(stream->fd);
	free(stream->clients);
	free(stream->input.data);
	free(stream->output.data);
}

static void server_close_bad_streams(server_t server) {
	stream_t s, next, *spp;
	for (spp = &server->used_chain; (s = *spp) != NULL; s = next) { 
		next = s->next;
		if (!s->good) {
			server_stream_destroy(server, s);
			*spp = next;
			s->next = server->free_chain;
			server->free_chain = s;
		} else {
			spp = &s->next;
		}
	}
}

static bool stream_message_start(stream_t stream, unsigned int chan) {
	ShubMessageHdr *msg;

	if (!stream->good) {
		return false;
	}

	if (stream->output.curmessage) {
		shout("cannot start new message while the old one is unfinished\n");
		stream->good = false;
		return false;
	}

	if (BUFFER_SIZE - stream->output.ready < sizeof(ShubMessageHdr)) {
		if (!stream_flush(stream)) {
			shout("failed to flush before starting new message\n");
			stream->good = false;
			return false;
		}
	}

	msg = stream->output.curmessage = (ShubMessageHdr*)(stream->output.data + stream->output.ready);
	msg->size = 0;
	msg->code = 'r';
	msg->chan = chan;

	return true;
}

static bool stream_message_append(stream_t stream, size_t len, void *data) {
	ShubMessageHdr *msg;
	int newsize;

	debug("appending %d\n", *(int*)data);

	if (stream->output.curmessage == NULL) {
		shout("cannot append, the message was not started\n");
		stream->good = false;
		return false;
	}

	newsize = stream->output.curmessage->size + sizeof(ShubMessageHdr) + len;
	if (newsize > BUFFER_SIZE) {
		/* the flushing will not help here */
		shout("the message cannot be bigger than the buffer size\n");
		stream->good = false;
		return false;
	}

	if (stream->output.ready + newsize > BUFFER_SIZE) {
		if (!stream_flush(stream)) {
			shout("failed to flush before extending the message\n");
			stream->good = false;
			return false;
		}
	}

	msg = stream->output.curmessage;
	memcpy((char*)msg + msg->size + sizeof(ShubMessageHdr), data, len);
	msg->size += len;

	return true;
}

static bool stream_message_finish(stream_t stream) {
	if (stream->output.curmessage == NULL) {
		shout("cannot finish, the message was not started\n");
		stream->good = false;
		return false;
	}

	stream->output.ready += stream->output.curmessage->size;
	stream->output.ready += sizeof(ShubMessageHdr);
	stream->output.curmessage = NULL;
	assert(stream->output.ready <= BUFFER_SIZE);
	return true;
}

bool client_message_start(client_t client) {
	return stream_message_start(client->stream, client->chan);
}

bool client_message_append(client_t client, size_t len, void *data) {
	return stream_message_append(client->stream, len, data);
}

bool client_message_finish(client_t client) {
	return stream_message_finish(client->stream);
}

bool client_message_shortcut(client_t client, xid_t arg) {
	if (!stream_message_start(client->stream, client->chan)) {
		return false;
	}
	if (!stream_message_append(client->stream, sizeof(arg), &arg)) {
		return false;
	}
	if (!stream_message_finish(client->stream)) {
		return false;
	}
	return true;
}

bool client_redirect(client_t client, unsigned addr, int port) {
	assert(false); // FIXME: implement
	return true;
}

static bool server_accept(server_t server) {
	int fd;
	stream_t s;

	debug("a new connection is queued\n");

	fd = accept(server->listener, NULL, NULL);
	if (fd == -1) {
		shout("failed to accept a connection: %s\n", strerror(errno));
		return false;
	}
	debug("a new connection accepted\n");
	
	s = server->free_chain;
	if (s == NULL) { 
		s = malloc(sizeof(stream_data_t));
	} else { 
		server->free_chain = s->next;
	}
	/* add new stream */
	s->next = server->used_chain;
	server->used_chain = s;

	if (!server->enabled) {
		shout("server disabled, disconnecting the accepted connection\n");
		// FIXME: redirect instead of disconnecting
		close(fd);
		return false;
	}

	stream_init(s, fd);

	return server_add_socket(server, fd, s);
}

static client_t stream_get_client(stream_t stream, unsigned int chan, bool *isnew) {
	client_t client;

	assert(chan < MAX_TRANSACTIONS);
	client = stream->clients + chan;
	if (client->stream == NULL) {
		/* client is new */
		client->stream = stream;
		client->chan = chan;
		*isnew = true;
		client->userdata = NULL;
	} else {
		/* collisions should not happen */
		assert(client->chan == chan);
		*isnew = false;
	}
	return client;
}

static bool server_stream_handle(server_t server, stream_t stream) {
	char *cursor;
	int avail;
	int recved;
	int toprocess;

	debug("a stream ready to recv\n");

	cursor = stream->input.data + stream->input.ready;
	avail = BUFFER_SIZE - stream->input.ready;
	assert(avail > 0);

	recved = recv(stream->fd, cursor, avail, 0);
	if (recved == -1) {
		shout("failed to recv from a stream: %s\n", strerror(errno));
		stream->good = false;
		return false;
	}
	if (recved == 0) {
		debug("eof from a stream\n");
		stream->good = false;
		return false;
	}

	debug("recved %d bytes\n", recved);
	stream->input.ready += recved;

	cursor = stream->input.data;
	toprocess = stream->input.ready;
	while (toprocess >= sizeof(ShubMessageHdr)) {
		ShubMessageHdr *msg = (ShubMessageHdr*)cursor;
		int header_and_data = sizeof(ShubMessageHdr) + msg->size;
		if (header_and_data <= toprocess) {
			/* handle message */
			bool isnew;
			client_t client = stream_get_client(stream, msg->chan, &isnew);
			if (isnew) {
				server->onconnect(client);
			}
			if (msg->code == MSG_DISCONNECT) {
				server->ondisconnect(client);
				if (client->userdata) {
					shout(
						"client still has userdata after 'ondisconnect' call,\n"
						"please set it to NULL in 'ondisconnect' callback\n"
					);
				}
				client->stream = NULL;
			} else {
				server->onmessage(client, msg->size, (char*)msg + sizeof(ShubMessageHdr));
			}
			cursor += header_and_data;
			toprocess -= header_and_data;
		} else {
			debug("message is still not ready: need %d more bytes\n", header_and_data - toprocess);
			if (header_and_data > BUFFER_SIZE) {
				shout(
					"the message of size %d will never fit into recv buffer of size %d\n",
					header_and_data, BUFFER_SIZE
				);
				stream->good = false;
				return false;
			}
			break;
		}
	}

	assert(toprocess >= 0);
	if (toprocess > 0) {
		memmove(stream->input.data, cursor, toprocess);
	}
	stream->input.ready = toprocess;

	return true;
}

bool server_tick(server_t server, int timeout_ms) {

	int i;
	int numready;
	bool raft_ready = false;
#ifdef USE_EPOLL
	struct epoll_event events[MAX_EVENTS];
	numready = epoll_wait(server->epollfd, events, MAX_EVENTS, timeout_ms);
	if (numready < 0) {
		shout("failed to epoll: %s\n", strerror(errno));
		return false;
	}
	for (i = 0; i < numready; i++) { 
		stream_t stream = (stream_t)events[i].data.ptr;

		if (stream == NULL) { 
			server_accept(server);
		} else if (stream == &server->raft_stream) {
			raft_ready = true;
		} else {
			if (events[i].events & EPOLLERR) {
				stream->good = false;
			} else if (events[i].events & EPOLLIN) {
				server_stream_handle(server, stream);
			}
		}
	}
#else
	fd_set readfds = server->all;
	struct timeval timeout = ms2tv(timeout_ms);
	numready = select(server->maxfd + 1, &readfds, NULL, NULL, &timeout);
	if (numready == -1) {
		shout("failed to select: %s\n", strerror(errno));
		return false;
	}

	if (FD_ISSET(server->listener, &readfds)) {
		numready--;
		server_accept(server);
	}

	if ((server->raft_stream.good) && FD_ISSET(server->raft_stream.fd, &readfds)) {
		numready--;
		raft_ready = true;
	}

	stream_t s;
	for (s = server_used_chain; (s != NULL) && (numready > 0); s = s->next) {
		if (FD_ISSET(s->fd, &readfds)) {
			server_stream_handle(server, s);
			numready--;
		}
	}
#endif

	server_close_bad_streams(server);
	server_flush(server);

	return raft_ready;
}

static void server_close_all_streams(server_t server) {
	stream_t s, next, *spp;
	for (spp = &server->used_chain; (s = *spp) != NULL; s = next) { 
		next = s->next;
		server_stream_destroy(server, s);
		*spp = next;
		s->next = server->free_chain;
		server->free_chain = s;
	}
}

void server_disable(server_t server) {
	if (!server->enabled) return;
	server->enabled = false;
	shout("server disabled\n");

	server_close_all_streams(server);
	shout("client connections closed\n");
}

void server_enable(server_t server) {
	if (server->enabled) return;
	server->enabled = true;
	shout("server enabled\n");
}

void server_set_enabled(server_t server, bool enable) {
	if (server->enabled != enable) {
		if (enable) {
			server_enable(server);
		} else {
			server_disable(server);
		}
	}
}

void client_set_userdata(client_t client, void *userdata) {
	client->userdata = userdata;
}

void *client_get_userdata(client_t client) {
	return client->userdata;
}

unsigned client_get_ip_addr(client_t client)
{
    struct sockaddr_in inet_addr;
    socklen_t inet_addr_len = sizeof(inet_addr);
    inet_addr.sin_addr.s_addr = 0;
    getpeername(client->stream->fd, (struct sockaddr *)&inet_addr, &inet_addr_len);
    return inet_addr.sin_addr.s_addr;
}
   
#if 0
/* usage example */

void test_onconnect(client_t client) {
	char *name = "hello";
	client_set_userdata(client, name);
	printf("===== a new client\n");
}

void test_ondisconnect(client_t client) {
	printf("===== '%s' disconnected\n", (char*)client_get_userdata(client));
	client_set_userdata(client, NULL);
}

void test_onmessage(client_t client, size_t len, char *data) {
	printf("===== a message from '%s'\n", (char*)client_get_userdata(client));
	client_message_start(client);
	while (len >= sizeof(int)) {
		int x = *(int*)data;
		data += sizeof(int);
		len -= sizeof(int);

		x++;
		client_message_append(client, sizeof(int), &x);
	}
	client_message_finish(client);
}

int main(int argc, char **argv) {
	server_t srv = server_init("0.0.0.0", 5431, test_onmessage, test_onconnect, test_ondisconnect);
	if (!server_start(srv)) {
		return EXIT_FAILURE;
	}
	server_loop(srv);
	return EXIT_SUCCESS;
}
#endif
