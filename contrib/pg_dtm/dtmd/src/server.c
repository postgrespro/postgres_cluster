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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "server.h"
#include "limits.h"
#include "util.h"
#include "sockhub.h"

typedef struct buffer_t {
	int ready; // number of bytes that are ready to be sent/processed
	ShubMessageHdr *curmessage;
	char *data; // dynamically allocated buffer
} buffer_t;

typedef struct stream_data_t *stream_t;

typedef struct client_data_t {
	stream_t stream; // NULL: client value is empty
	void *userdata;
	unsigned int chan;
} client_data_t;

typedef struct stream_data_t {
	int fd;
	bool good; // 'false': stop serving this stream and disconnect when possible
	buffer_t input;
	buffer_t output;

	// a map: 'chan' -> client_data_t
	// 'chan' is expected to be < MAX_FDS which is pretty low
	client_data_t *clients; // dynamically allocated
} stream_data_t;

typedef struct server_data_t {
	char *host;
	int port;

	int listener; // the listening socket
	fd_set all; // all sockets including the listener
	int maxfd;

	int streamsnum;
	stream_data_t streams[MAX_STREAMS];

	onmessage_callback_t onmessage;
	onconnect_callback_t onconnect;
	ondisconnect_callback_t ondisconnect;
} server_data_t;

// Returns the created socket, or -1 if failed.
static int create_listening_socket(const char *host, int port) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		shout("cannot create the listening socket: %s\n", strerror(errno));
		return -1;
	}

	int optval = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	if (inet_aton(host, &addr.sin_addr) == 0) {
		shout("cannot convert the host string '%s' to a valid address\n", host);
		return -1;
	}
	addr.sin_port = htons(port);
	debug("binding %s:%d\n", host, port);
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

	return server;
}

bool server_start(server_t server) {
	debug("starting the server\n");
	server->streamsnum = 0;

	server->listener = create_listening_socket(server->host, server->port);
	if (server->listener == -1) {
		return false;
	}

	FD_ZERO(&server->all);
	FD_SET(server->listener, &server->all);
	server->maxfd = server->listener;

	return true;
}

static bool stream_flush(stream_t stream) {
	int tosend = stream->output.ready;
	if (tosend == 0) {
		// nothing to do
		return true;
	}

	char *cursor = stream->output.data;
	while (tosend > 0) {
		// repeat sending until we send everything
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
	ShubMessageHdr *msg = stream->output.curmessage;
	if (msg) {
		// move the unfinished message to the start of the buffer
		memmove(stream->output.data, msg, msg->size + sizeof(ShubMessageHdr));
		stream->output.curmessage = (ShubMessageHdr*)stream->output.data;
	}

	return true;
}

static void server_flush(server_t server) {
	debug("flushing the streams\n");
	int i;
	for (i = 0; i < server->streamsnum; i++) {
		stream_t stream = server->streams + i;
		stream_flush(stream);
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
	// mark all clients as empty
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

	FD_CLR(stream->fd, &server->all);
	close(stream->fd);
	free(stream->clients);
	free(stream->input.data);
	free(stream->output.data);
}

static void server_close_bad_streams(server_t server) {
	int i;
	for (i = server->streamsnum - 1; i >= 0; i--) {
		stream_t stream = server->streams + i;
		if (!stream->good) {
			server_stream_destroy(server, stream);
			if (i != server->streamsnum - 1) {
				// move the last one here
				*stream = server->streams[server->streamsnum - 1];
			}
			server->streamsnum--;
		}
	}
}

static bool stream_message_start(stream_t stream, unsigned int chan) {
	ShubMessageHdr *msg;

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

	debug("appending %d\n", *(int*)data);

	if (stream->output.curmessage == NULL) {
		shout("cannot append, the message was not started\n");
		stream->good = false;
		return false;
	}

	int newsize = stream->output.curmessage->size + sizeof(ShubMessageHdr) + len;
	if (newsize > BUFFER_SIZE) {
		// the flushing will not help here
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

bool client_message_shortcut(client_t client, long long arg) {
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

static bool server_accept(server_t server) {
	debug("a new connection is queued\n");

	int fd = accept(server->listener, NULL, NULL);
	if (fd == -1) {
		shout("failed to accept a connection: %s\n", strerror(errno));
		return false;
	}
	debug("a new connection accepted\n");

	if (server->streamsnum >= MAX_STREAMS) {
		shout("streams limit hit, disconnecting the accepted connection\n");
		close(fd);
		return false;
	}

	// add new stream
	stream_t s = server->streams + server->streamsnum++;
	stream_init(s, fd);

	FD_SET(fd, &server->all);
	if (fd > server->maxfd) {
		server->maxfd = fd;
	}

	return true;
}

static client_t stream_get_client(stream_t stream, unsigned int chan, bool *isnew) {
	assert(chan < MAX_TRANSACTIONS);
	client_t client = stream->clients + chan;
	if (client->stream == NULL) {
		// client is new
		client->stream = stream;
		client->chan = chan;
		*isnew = true;
		client->userdata = NULL;
	} else {
		// collisions should not happen
		assert(client->chan == chan);
		*isnew = false;
	}
	return client;
}

static bool server_stream_handle(server_t server, stream_t stream) {
	debug("a stream ready to recv\n");

	char *cursor = stream->input.data + stream->input.ready;
	int avail = BUFFER_SIZE - stream->input.ready;
	assert(avail > 0);

	int recved = recv(stream->fd, cursor, avail, 0);
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
	int toprocess = stream->input.ready;
	while (toprocess >= sizeof(ShubMessageHdr)) {
		ShubMessageHdr *msg = (ShubMessageHdr*)cursor;
		int header_and_data = sizeof(ShubMessageHdr) + msg->size;
		if (header_and_data <= toprocess) {
			// handle message
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
					header_and_data, BUFFER_SIZE);
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

void server_loop(server_t server) {
	while (1) {
		int i;
		fd_set readfds = server->all;
		debug("selecting\n");
		int numready = select(server->maxfd + 1, &readfds, NULL, NULL, NULL);
		if (numready == -1) {
			shout("failed to select: %s\n", strerror(errno));
			return;
		}

		if (FD_ISSET(server->listener, &readfds)) {
			numready--;
			server_accept(server);
		}

		for (i = 0; (i < server->streamsnum) && (numready > 0); i++) {
			stream_t stream = server->streams + i;
			if (FD_ISSET(stream->fd, &readfds)) {
				server_stream_handle(server, stream);
				numready--;
			}
		}

		server_flush(server);
		server_close_bad_streams(server);
	}
}

void client_set_userdata(client_t client, void *userdata) {
	client->userdata = userdata;
}

void *client_get_userdata(client_t client) {
	return client->userdata;
}

#if 0
// usage example

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
