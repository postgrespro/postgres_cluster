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

#ifdef WITH_RSOCKET
#include <rdma/rsocket.h>
#endif
#include <sys/socket.h>

extern int pg_socket(int domain, int type, int protocol, bool isRsocket);
extern int pg_bind(int socket, const struct sockaddr *addr, socklen_t addrlen,
				   bool isRsocket);
extern int pg_listen(int socket, int backlog, bool isRsocket);
extern int pg_accept(int socket, struct sockaddr *addr, socklen_t *addrlen,
					 bool isRsocket);
extern int pg_connect(int socket, const struct sockaddr *addr,
					  socklen_t addrlen, bool isRsocket);
extern int pg_closesocket(int socket, bool isRsocket);

extern ssize_t pg_recv(int socket, void *buf, size_t len, int flags,
					   bool isRsocket);
extern ssize_t pg_send(int socket, const void *buf, size_t len, int flags,
					   bool isRsocket);

extern int pg_poll(struct pollfd *fds, nfds_t nfds, int timeout,
				   bool isRsocket);
extern int pg_select(int nfds, fd_set *readfds, fd_set *writefds,
				   fd_set *exceptfds, struct timeval *timeout);

extern int pg_getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen,
						  bool isRsocket);

extern int pg_setsockopt(int socket, int level, int optname,
						 const void *optval, socklen_t optlen, bool isRsocket);
extern int pg_getsockopt(int socket, int level, int optname,
						 void *optval, socklen_t *optlen, bool isRsocket);

#endif   /* PG_SOCKET_H */
