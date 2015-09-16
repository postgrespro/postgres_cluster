/*
 * This module is used as a layer of abstraction between the main logic and the
 * event library. This should, theoretically, allow us to switch to another
 * library with less effort.
 */

#ifndef EVENTWRAP_H
#define EVENTWRAP_H

int eventwrap(
	const char *host,
	int port,
	char *(*ondata)(void *stream, void *clientdata, size_t len, char *data),
	void (*onconnect)(void *stream, void **clientdata),
	void (*ondisconnect)(void *stream, void *clientdata)
);

void write_to_stream(void *stream, char *data);

#endif
