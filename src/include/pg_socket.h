/*-------------------------------------------------------------------------
 *
 * pg_socket.h
 *	  Definitions for socket functions.
 *
 *
 * Copyright (c) 2017, Postgres Professional
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

/*
 * API struct for socket methods.
 */
typedef struct PgSocketData
{
	/* socket descriptor */
	pgsocket	fd;
	bool		isRsocket;

	/* function pointers */
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
} PgSocketData;

typedef PgSocketData *PgSocket;

#define PGINVALID_SOCKET_EXTENDED NULL

extern PgSocket pg_socket(int domain, int type, int protocol, bool isRsocket);
extern int pg_closesocket(PgSocket socket);

#define PG_SOCK(socket) \
	(socket)->fd
#define PG_ISRSOCKET(socket) \
	(socket)->isRsocket

/*
 * Wrappers to function pointers of PgSocketData struct
 */

#define pg_bind(socket, addr, addrlen) \
	(socket)->bind((socket)->fd, addr, addrlen)
#define pg_listen(socket, backlog) \
	(socket)->listen((socket)->fd, backlog)
#define pg_accept(socket, addr, addrlen) \
	(socket)->accept((socket)->fd, addr, addrlen)
#define pg_connect(socket, addr, addrlen) \
	(socket)->connect((socket)->fd, addr, addrlen)

#define pg_recv(socket, buf, len, flags) \
	(socket)->recv((socket)->fd, buf, len, flags)
#define pg_send(socket, buf, len, flags) \
	(socket)->send((socket)->fd, buf, len, flags)
#define pg_sendmsg(socket, msg, flags) \
	(socket)->sendmsg((socket)->fd, msg, flags)

#ifdef HAVE_POLL
#define pg_poll(fds, nfds, timeout, isRsocket) \
	((isRsocket) ? rpoll(fds, nfds, timeout) : \
		poll(fds, nfds, timeout))
#endif
#define pg_select(nfds, readfds, writefds, exceptfds, timeout, isRsocket) \
	((isRsocket) ? rselect(nfds, readfds, writefds, exceptfds, timeout) : \
		select(nfds, readfds, writefds, exceptfds, timeout))

#define pg_getsockname(socket, addr, addrlen) \
	(socket)->getsockname((socket)->fd, addr, addrlen)
#define pg_setsockopt(socket, level, optname, optval, optlen) \
	(socket)->setsockopt((socket)->fd, level, optname, optval, optlen)
#define pg_getsockopt(socket, level, optname, optval, optlen) \
	(socket)->getsockopt((socket)->fd, level, optname, optval, optlen)

#if !defined(WIN32)
#define pg_fcntl(socket, flag, value) \
	(socket)->fcntl((socket)->fd, flag, value)
#endif

extern PgSocket initialize_socket(void);
extern PgSocket initialize_rsocket(void);

extern int rpoll(struct pollfd *fds, nfds_t nfds, int timeout);
extern int rselect(pgsocket nfds, fd_set *readfds, fd_set *writefds,
				   fd_set *exceptfds, struct timeval *timeout);

extern bool pg_set_noblock_extended(PgSocket sock);
extern bool pg_set_block_extended(PgSocket sock);

#define StreamCloseExtended(sock)				\
do {											\
	if ((sock) && (sock)->fd != PGINVALID_SOCKET)	\
		pg_closesocket(sock);					\
} while (0)										\

#else   /* !WITH_RSOCKET */

typedef pgsocket PgSocket;

#define PGINVALID_SOCKET_EXTENDED PGINVALID_SOCKET

#define PG_SOCK(socket) \
	socket
#define PG_ISRSOCKET(socket) \
	false

/*
 * Wrappers to socket functions
 */

#define pg_socket(domain, type, protocol, isRsocket) \
	socket(domain, type, protocol)
#define pg_bind(socket, addr, addrlen) \
	bind(socket, addr, addrlen)
#define pg_listen(socket, backlog) \
	listen(socket, backlog)
#define pg_accept(socket, addr, addrlen) \
	accept(socket, addr, addrlen)
#define pg_connect(socket, addr, addrlen) \
	connect(socket, addr, addrlen)
#define pg_closesocket(socket) \
	close(socket)

#define pg_recv(socket, buf, len, flags) \
	recv(socket, buf, len, flags)
#define pg_send(socket, buf, len, flags) \
	send(socket, buf, len, flags)
#define pg_sendmsg(socket, msg, flags) \
	sendmsg(socket, msg, flags)

#ifdef HAVE_POLL
#define pg_poll(fds, nfds, timeout, isRsocket) \
	poll(fds, nfds, timeout)
#endif
#define pg_select(nfds, readfds, writefds, exceptfds, timeout, isRsocket) \
	select(nfds, readfds, writefds, exceptfds, timeout)

#define pg_getsockname(socket, addr, addrlen) \
	getsockname(socket, addr, addrlen)
#define pg_setsockopt(socket, level, optname, optval, optlen) \
	setsockopt(socket, level, optname, optval, optlen)
#define pg_getsockopt(socket, level, optname, optval, optlen) \
	getsockopt(socket, level, optname, optval, optlen)

#if !defined(WIN32)
#define pg_fcntl(socket, flag, value) \
	fcntl(socket, flag, value)
#endif

#define pg_set_noblock_extended(sock) \
	pg_set_noblock(sock)
#define pg_set_block_extended(sock) \
	pg_set_block(sock)

#define StreamCloseExtended(sock)	\
do {								\
	if ((sock) != PGINVALID_SOCKET)	\
		StreamClose(sock);			\
} while (0)

#endif   /* WITH_RSOCKET */

#endif   /* PG_SOCKET_H */
