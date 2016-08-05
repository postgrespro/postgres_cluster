#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>

#define BUFLEN 1024

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

typedef struct Message {
	size_t len;
	char data[1];
} Message;

typedef struct Client {
	ClientState state;
	int socket;
	size_t msglen;
	size_t cursor;
	Message *msg; /* the message that is currently being sent or received */
	int expect;
} Client;

void client_recv(Client *client);
void client_send(Client *client);
void client_switch(Client *client, ClientState state);

#endif
