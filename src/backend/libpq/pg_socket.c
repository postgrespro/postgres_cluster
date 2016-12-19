/*-------------------------------------------------------------------------
 *
 * pg_socket.c
 *	  Implementations of socket functions.
 *
 *
 * Copyright (c) 2016, Postgres Professional
 *
 * src/backend/libpq/pg_socket.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <unistd.h>

#include "pg_socket.h"

int
pg_socket(int domain, int type, int protocol, bool isRdma)
{
#ifdef WITH_RSOCKET
	if (isRdma)
		return rsocket(domain, type, protocol);
	else
#endif
		return socket(domain, type, protocol);
}

int
pg_bind(int socket, const struct sockaddr *addr, socklen_t addrlen, bool isRdma)
{
#ifdef WITH_RSOCKET
	if (isRdma)
		return rbind(socket, addr, addrlen);
	else
#endif
		return bind(socket, addr, addrlen);
}

int
pg_listen(int socket, int backlog, bool isRdma)
{
#ifdef WITH_RSOCKET
	if (isRdma)
		return rlisten(socket, backlog);
	else
#endif
		return listen(socket, backlog);
}

int
pg_closesocket(int socket, bool isRdma)
{
#ifdef WITH_RSOCKET
	if (isRdma)
		return rclose(socket);
	else
#endif
		return close(socket);
}

int
pg_setsockopt(int socket, int level, int optname,
			  const void *optval, socklen_t optlen, bool isRdma)
{
#ifdef WITH_RSOCKET
	if (isRdma)
		return rsetsockopt(socket, level, optname, optval, optlen);
	else
		return setsockopt(socket, level, optname, optval, optlen);
#endif
}
