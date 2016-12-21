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

/* This is intended to be used in both frontend and backend, so use c.h */
#include "c.h"

#include <unistd.h>

#include "pg_socket.h"

int
pg_socket(int domain, int type, int protocol, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rsocket(domain, type, protocol);
	else
#endif
		return socket(domain, type, protocol);
}

int
pg_bind(int socket, const struct sockaddr *addr, socklen_t addrlen, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rbind(socket, addr, addrlen);
	else
#endif
		return bind(socket, addr, addrlen);
}

int
pg_listen(int socket, int backlog, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rlisten(socket, backlog);
	else
#endif
		return listen(socket, backlog);
}

int
pg_accept(int socket, struct sockaddr *addr, socklen_t *addrlen, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return raccept(socket, addr, addrlen);
	else
#endif
		return accept(socket, addr, addrlen);
}

int
pg_connect(int socket, const struct sockaddr *addr,
		   socklen_t addrlen, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rconnect(socket, addr, addrlen);
	else
#endif
		return connect(socket, addr, addrlen);
}

int
pg_closesocket(int socket, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rclose(socket);
	else
#endif
		return close(socket);
}

ssize_t
pg_recv(int socket, void *buf, size_t len, int flags, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rrecv(socket, buf, len, flags);
	else
#endif
		return recv(socket, buf, len, flags);
}

ssize_t
pg_send(int socket, const void *buf, size_t len, int flags, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rsend(socket, buf, len, flags);
	else
#endif
		return send(socket, buf, len, flags);
}

int
pg_poll(struct pollfd *fds, nfds_t nfds, int timeout, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rpoll(fds, nfds, timeout);
	else
#endif
		return poll(fds, nfds, timeout);
}

int
pg_select(int nfds, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout)
{
#ifdef WITH_RSOCKET
	return rselect(nfds, readfds, writefds, exceptfds, timeout);
#else
	return select(nfds, readfds, writefds, exceptfds, timeout);
#endif
}

int
pg_getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen,
			   bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rgetsockname(socket, addr, addrlen);
	else
#endif
		return getsockname(socket, addr, addrlen);
}

int
pg_setsockopt(int socket, int level, int optname,
			  const void *optval, socklen_t optlen, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rsetsockopt(socket, level, optname, optval, optlen);
	else
		return setsockopt(socket, level, optname, optval, optlen);
#endif
}

int
pg_getsockopt(int socket, int level, int optname,
			  void *optval, socklen_t *optlen, bool isRsocket)
{
#ifdef WITH_RSOCKET
	if (isRsocket)
		return rgetsockopt(socket, level, optname, optval, optlen);
	else
		return getsockopt(socket, level, optname, optval, optlen);
#endif
}
