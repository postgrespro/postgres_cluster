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

#include "pg_config.h"

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
#include <rdma/rsocket.h>
#endif
#include <sys/socket.h>

#ifdef WITH_RSOCKET

#define pg_socket(domain, type, protocol, isRsocket) \
	((isRsocket) ? rsocket(domain, type, protocol) : \
		socket(domain, type, protocol))

#define pg_bind(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rbind(socket, addr, addrlen) : \
		bind(socket, addr, addrlen))

#define pg_listen(socket, backlog, isRsocket) \
	((isRsocket) ? rlisten(socket, backlog) : \
		listen(socket, backlog))

#define pg_accept(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? raccept(socket, addr, addrlen) : \
		accept(socket, addr, addrlen))

#define pg_connect(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rconnect(socket, addr, addrlen) : \
		connect(socket, addr, addrlen))

#define pg_closesocket(socket, isRsocket) \
	((isRsocket) ? rclose(socket) : \
		close(socket))

#define pg_recv(socket, buf, len, flags, isRsocket) \
	((isRsocket) ? rrecv(socket, buf, len, flags) : \
		recv(socket, buf, len, flags))

#define pg_send(socket, buf, len, flags, isRsocket) \
	((isRsocket) ? rsend(socket, buf, len, flags) : \
		send(socket, buf, len, flags))

#define pg_sendmsg(socket, msg, flags, isRsocket) \
	((isRsocket) ? rsendmsg(socket, msg, flags) : \
		sendmsg(socket, msg, flags))

#define pg_poll(fds, nfds, timeout, isRsocket) \
	((isRsocket) ? rpoll(fds, nfds, timeout) : \
		poll(fds, nfds, timeout))

#define pg_select(nfds, readfds, writefds, exceptfds, timeout) \
	rselect(nfds, readfds, writefds, exceptfds, timeout)

#define pg_getsockname(socket, addr, addrlen, isRsocket) \
	((isRsocket) ? rgetsockname(socket, addr, addrlen) : \
		getsockname(socket, addr, addrlen))

#define pg_setsockopt(socket, level, optname, optval, optlen, isRsocket) \
	((isRsocket) ? rsetsockopt(socket, level, optname, optval, optlen) : \
		setsockopt(socket, level, optname, optval, optlen))

#define pg_getsockopt(socket, level, optname, optval, optlen, isRsocket) \
	((isRsocket) ? rgetsockopt(socket, level, optname, optval, optlen) : \
		getsockopt(socket, level, optname, optval, optlen))

#else

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

#define pg_poll(fds, nfds, timeout, isRsocket) \
	poll(fds, nfds, timeout)

#define pg_select(nfds, readfds, writefds, exceptfds, timeout) \
	select(nfds, readfds, writefds, exceptfds, timeout)

#define pg_getsockname(socket, addr, addrlen, isRsocket) \
	getsockname(socket, addr, addrlen)

#define pg_setsockopt(socket, level, optname, optval, optlen, isRsocket) \
	setsockopt(socket, level, optname, optval, optlen)

#define pg_getsockopt(socket, level, optname, optval, optlen, isRsocket) \
	getsockopt(socket, level, optname, optval, optlen)

#endif   /* WITH_RSOCKET */

#endif   /* PG_SOCKET_H */
