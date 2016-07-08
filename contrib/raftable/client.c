#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <postgres.h>

#include "client.h"

static bool continue_recv(int socket, void *dst, size_t len, size_t *done)
{
	while (*done < len)
	{
		ssize_t recved = recv(socket, ((char *)dst) + *done, len - *done, MSG_DONTWAIT);
		if (recved == 0) return false;
		if (recved < 0)
		{
			switch (errno)
			{
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
		Assert(*done <= len);
	}
	return true;
}

static bool continue_send(int socket, void *src, size_t len, size_t *done)
{
	while (*done < len)
	{
		ssize_t sent = send(socket, ((char *)src) + *done, len - *done, MSG_DONTWAIT);
		if (sent == 0) return false;
		if (sent < 0)
		{
			switch (errno)
			{
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
		Assert(*done <= len);
	}
	return true;
}

void client_recv(Client *client)
{
	Assert(client->state == CLIENT_SENDING);

	if (!client->msg)
	{
		if (client->cursor < sizeof(client->msglen))
		{
			if (!continue_recv(client->socket, &client->msglen, sizeof(client->msglen), &client->cursor))
				goto failure;
		}
		if (client->cursor < sizeof(client->msglen)) return; /* continue later */

		client->msg = palloc(sizeof(Message) + client->msglen);
		client->msg->len = client->msglen;
		client->cursor = 0;
	}

	if (client->cursor < client->msg->len)
	{
		if (!continue_recv(client->socket, client->msg->data, client->msg->len, &client->cursor))
			goto failure;
	}

	return;
failure:
	client->state = CLIENT_SICK;
}

void client_send(Client *client)
{
	size_t totallen;
	Assert(client->state == CLIENT_RECVING);
	Assert(client->msg != NULL);

	totallen = client->msg->len + sizeof(client->msg->len);
	if (client->cursor < totallen)
	{
		if (!continue_send(client->socket, client->msg, totallen, &client->cursor))
			goto failure;
	}

	return;
failure:
	client->state = CLIENT_SICK;
}
