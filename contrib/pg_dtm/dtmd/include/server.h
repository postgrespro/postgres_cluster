#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

/*
 * You should not want to know what is inside those structures.
 */
typedef struct server_data_t *server_t;
typedef struct client_data_t *client_t;

/*
 * The server will call this function whenever it gets a message ('len' bytes
 * of 'data') from the 'client'.
 */
typedef void (*onmessage_callback_t)(client_t client, size_t len, char *data);

/*
 * The server will call this function whenever a new 'client' send the first
 * message. This callback gets called before the 'onmessage'.
 */
typedef void (*onconnect_callback_t)(client_t client);

/*
 * The server will call this function whenever it considers the 'client'
 * disconnected.
 */
typedef void (*ondisconnect_callback_t)(client_t client);

/*
 * Creates a new server that will listen on 'host:port' and call the specified
 * callbacks. Returns the server handle to use in other methods.
 */
server_t server_init(
	char *host,
	int port,
	onmessage_callback_t onmessage,
	onconnect_callback_t onconnect,
	ondisconnect_callback_t ondisconnect
);

/*
 * Starts the server. Returns 'true' on success, 'false' otherwise.
 */
bool server_start(server_t server);

/*
 * The main server loop. Does not return, so use the callbacks and signal
 * handlers to add more logic.
 */
void server_loop(server_t server);

/*
 * These two methods allow you to set and get your custom 'userdata' for the
 * 'client'. The server does not care about this data and will not free it on
 * client disconnection.
 */
void client_set_userdata(client_t client, void *userdata);
void *client_get_userdata(client_t client);

/*
 * Puts an empty message header into the output buffer of the corresponding
 * socket. The message will not be sent until you call the _finish() method.
 * A call to this function may lead to a send() call if there is not enough
 * space in the buffer.
 *
 * Returns 'true' on success, 'false' otherwise.
 *
 * NOTE: Be careful not to call the _message_ methods for other clients until
 * you _finish() this message. This limitation is due to the fact that multiple
 * clients share the same socket.
 */
bool client_message_start(client_t client);

/*
 * Appends 'len' bytes of 'data' to the buffer of the corresponding socket.
 * A call to this function may lead to a send() call if there is not enough
 * space in the buffer.
 *
 * Returns 'true' on success, 'false' otherwise.
 */
bool client_message_append(client_t client, size_t len, void *data);

/*
 * Finalizes the message. After finalizing the message becomes ready to be sent
 * over the corresponding socket, and you may _start() another message.
 *
 * Returns 'true' on success, 'false' otherwise.
 */
bool client_message_finish(client_t client);

/*
 * A shortcut to perform all three steps in one, if you only have one number in
 * the message.
 *
 * Returns 'true' on success, 'false' otherwise.
 */
bool client_message_shortcut(client_t client, long long arg);

#endif
