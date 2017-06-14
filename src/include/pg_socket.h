/*-------------------------------------------------------------------------
 *
 * pg_socket.h
 *	  Definitions for socket functions.
 *
 *
 * Copyright (c) 2016, Postgres Professional
 *
 * src/include/pg_socket.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SOCKET_H
#define PG_SOCKET_H

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include "pg_config.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef WITH_RSOCKET

#include "port.h"

/* Rsocket function pointers */
typedef struct PgSocketCall
{
	pgsocket	(*socket) (int domain, int type, int protocol);
	int			(*bind) (pgsocket socket, const struct sockaddr *addr,
						 socklen_t addrlen);
	int			(*listen) (pgsocket socket, int backlog);
	int			(*accept) (pgsocket socket, struct sockaddr *addr,
						   socklen_t *addrlen);
	int			(*connect) (pgsocket socket, const struct sockaddr *addr,
							socklen_t addrlen);
	int			(*close) (pgsocket socket);

	ssize_t		(*recv) (pgsocket socket, void *buf, size_t len, int flags);
	ssize_t		(*send) (pgsocket socket, const void *buf, size_t len,
						 int flags);
	ssize_t		(*sendmsg) (pgsocket socket, const struct msghdr *msg,
							int flags);

#ifdef HAVE_POLL
	int			(*poll) (struct pollfd *fds, nfds_t nfds, int timeout);
#endif
	int			(*select) (pgsocket nfds, fd_set *readfds, fd_set *writefds,
						   fd_set *exceptfds, struct timeval *timeout);

	int			(*getsockname) (pgsocket socket, struct sockaddr *addr,
								socklen_t *addrlen);
	int			(*setsockopt) (pgsocket socket, int level, int optname,
							   const void *optval, socklen_t optlen);
	int			(*getsockopt) (pgsocket socket, int level, int optname,
							   void *optval, socklen_t *optlen);

#if !defined(WIN32)
	int			(*fcntl) (pgsocket socket, int cmd, ... /* arg */ );
#endif
} PgSocketCall;

extern PgSocketCall *rcalls;

/*
 * These macroses call socket function depending on isRsocket value
 */

#define pg_fcntl(fd, flag, value, isRsocket) \
	((isRsocket) ? rcalls->fcntl(fd, flag, value) : \
		fcntl(fd, flag, value))

#define pg_socket(domain, type, protocol, isRsocket) \
	((isRsocket) ? rcalls->socket(domain, type, protocol) : \
		socket(domain, type, protocol))

#define pg_bind(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rcalls->bind(socket, addr, addrlen) : \
		bind(socket, addr, addrlen))

#define pg_listen(socket, backlog, isRsocket) \
	((isRsocket) ? rcalls->listen(socket, backlog) : \
		listen(socket, backlog))

#define pg_accept(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rcalls->accept(socket, addr, addrlen) : \
		accept(socket, addr, addrlen))

#define pg_connect(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rcalls->connect(socket, addr, addrlen) : \
		connect(socket, addr, addrlen))

#define pg_closesocket(socket, isRsocket) \
	((isRsocket) ? rcalls->close(socket) : \
		close(socket))

#define pg_recv(socket, buf, len, flags, isRsocket) \
	((isRsocket) ? rcalls->recv(socket, buf, len, flags) : \
		recv(socket, buf, len, flags))

#define pg_send(socket, buf, len, flags, isRsocket) \
	((isRsocket) ? rcalls->send(socket, buf, len, flags) : \
		send(socket, buf, len, flags))

#define pg_sendmsg(socket, msg, flags, isRsocket) \
	((isRsocket) ? rcalls->sendmsg(socket, msg, flags) : \
		sendmsg(socket, msg, flags))

#ifdef HAVE_POLL
#define pg_poll(fds, nfds, timeout, isRsocket) \
	((isRsocket) ? rcalls->poll(fds, nfds, timeout) : \
		poll(fds, nfds, timeout))
#endif

#define pg_select(nfds, readfds, writefds, exceptfds, timeout, isRsocket) \
	((isRsocket) ? rcalls->select(nfds, readfds, writefds, exceptfds, timeout) : \
		select(nfds, readfds, writefds, exceptfds, timeout))

#define pg_getsockname(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rcalls->getsockname(socket, addr, addrlen) : \
		getsockname(socket, addr, addrlen))

#define pg_setsockopt(socket, level, optname, optval, optlen, isRsocket) \
	((isRsocket) ? rcalls->setsockopt(socket, level, optname, optval, optlen) : \
		setsockopt(socket, level, optname, optval, optlen))

#define pg_getsockopt(socket, level, optname, optval, optlen, isRsocket) \
	((isRsocket) ? rcalls->getsockopt(socket, level, optname, optval, optlen) : \
		getsockopt(socket, level, optname, optval, optlen))

#else

#define pg_fcntl(fd, flag, value, isRsocket) \
	fcntl(fd, flag, value)

#define pg_socket(domain, type, protocol, isRsocket) \
	socket(domain, type, protocol)

#define pg_bind(socket, addr, addrlen, isRsocket) \
	bind(socket, addr, addrlen)

#define pg_listen(socket, backlog, isRsocket) \
	listen(socket, backlog)

#define pg_accept(socket, addr, addrlen, isRsocket) \
	accept(socket, addr, addrlen)

#define pg_connect(socket, addr, addrlen, isRsocket) \
	connect(socket, addr, addrlen)

#define pg_closesocket(socket, isRsocket) \
	close(socket)

#define pg_recv(socket, buf, len, flags, isRsocket) \
	recv(socket, buf, len, flags)

#define pg_send(socket, buf, len, flags, isRsocket) \
	send(socket, buf, len, flags)

#define pg_sendmsg(socket, msg, flags, isRsocket) \
	sendmsg(socket, msg, flags)

#ifdef HAVE_POLL
#define pg_poll(fds, nfds, timeout, isRsocket) \
	poll(fds, nfds, timeout)
#endif

#define pg_select(nfds, readfds, writefds, exceptfds, timeout, isRsocket) \
	select(nfds, readfds, writefds, exceptfds, timeout)

#define pg_getsockname(socket, addr, addrlen, isRsocket) \
	getsockname(socket, addr, addrlen)

#define pg_setsockopt(socket, level, optname, optval, optlen, isRsocket) \
	setsockopt(socket, level, optname, optval, optlen)

#define pg_getsockopt(socket, level, optname, optval, optlen, isRsocket) \
	getsockopt(socket, level, optname, optval, optlen)

#endif   /* WITH_RSOCKET */

#endif   /* PG_SOCKET_H */
