/*
 * This module is used as a layer of abstraction between the main logic and the
 * event library. This should, theoretically, allow us to switch to another
 * library with less effort.
 */

#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LISTENQUEUE 10
#include "eventwrap.h"
#include "util.h"

uv_loop_t *loop;

char *(*ondata_cb)(void *client, size_t len, char *data);
void (*onconnect_cb)(void **client);
void (*ondisconnect_cb)(void *client);

static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->len = suggested_size;
	buf->base = malloc(buf->len);
}

static void on_write(uv_write_t *req, int status) {
	if (status == -1) {
		shout("write failed\n");
		return;
	}
	free(req);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { 
	if (nread == -1) {
		shout("read failed\n");
        uv_close((uv_handle_t*)stream, NULL);
		return;
	}

	if (nread == UV_EOF) {
		ondisconnect_cb(stream->data);
        uv_close((uv_handle_t*)stream, NULL);
		return;
	}

	char *response = ondata_cb(stream->data, nread, buf->base);
	free(buf->base);

	if (response) {
		uv_write_t *wreq = malloc(sizeof(uv_write_t));
		uv_buf_t wbuf = uv_buf_init(response, strlen(response));
		uv_write(wreq, stream, &wbuf, 1, on_write);
		free(response);
	}
}

static void on_connect(uv_stream_t *server, int status) {
	if (status == -1) {
		shout("incoming connection failed");
		return;
	}

	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);
	if (uv_accept(server, (uv_stream_t*)client) < 0) {
		shout("failed to accept the connection\n");
		uv_close((uv_handle_t*)client, NULL);
		free(client);
		return;
	}
	uv_tcp_nodelay(client, 1);
	onconnect_cb(&client->data);
	uv_read_start((uv_stream_t*)client, on_alloc, on_read);
}

int eventwrap(
	const char *host,
	int port,
	char *(*ondata)(void *client, size_t len, char *data),
	void (*onconnect)(void **client),
	void (*ondisconnect)(void *client)
) {
	ondata_cb = ondata;
	onconnect_cb = onconnect;
	ondisconnect_cb = ondisconnect;

	shout("libuv version: %s\n", uv_version_string());

	loop = uv_default_loop();

	uv_tcp_t server;
	uv_tcp_init(loop, &server);

	struct sockaddr_in addr;
	int rc = uv_ip4_addr(host, port, &addr);
    if (rc != 0) { 
        shout("uv_ip4_addr error %s\n", uv_strerror(rc));
        return 1;
    }
	uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);

	rc = uv_listen((uv_stream_t*)&server, LISTENQUEUE, on_connect);
	if (rc) {
		shout("Listen error %s\n", uv_strerror(rc));
		return 1;
	}

	return uv_run(loop, UV_RUN_DEFAULT);
}
