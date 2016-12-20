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

#ifdef WITH_RSOCKET
#include <rdma/rsocket.h>
#endif
#include <sys/socket.h>

#include "pg_config.h"

extern int pg_socket(int domain, int type, int protocol, bool isRdma);
extern int pg_bind(int socket, const struct sockaddr *addr, socklen_t addrlen,
				   bool isRdma);
extern int pg_listen(int socket, int backlog, bool isRdma);
extern int pg_accept(int socket, struct sockaddr *addr, socklen_t *addrlen,
					 bool isRdma);
extern int pg_closesocket(int socket, bool isRdma);

extern int pg_select(int nfds, fd_set *readfds, fd_set *writefds,
				   fd_set *exceptfds, struct timeval *timeout);

extern int pg_getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen,
						  bool isRdma);

extern int pg_setsockopt(int socket, int level, int optname,
						 const void *optval, socklen_t optlen, bool isRdma);
extern int pg_getsockopt(int socket, int level, int optname,
						 void *optval, socklen_t *optlen, bool isRdma);

#endif   /* PG_SOCKET_H */
