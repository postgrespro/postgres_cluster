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

#ifndef WITH_RSOCKET
#include <sys/socket.h>
#else
#include <rdma/rsocket.h>
#endif

#ifdef WITH_RSOCKET
#define socket(d, t, p) rsocket(d, t, p)
#define bind(s, a, l) rbind(s, a, l)
#define listen(s, b) rlisten(s, b)
#define accept(s, a, l) raccept(s, a, l)
#define connect(s, a, l) rconnect(s, a, l)
#define shutdown(s, h) rshutdown(s, h)
#define close(s) rclose(s)

#define recv(s, b, l, f) rrecv(s, b, l, f)
#define recvfrom(s, b, l, f, sa, al) rrecvfrom(s, b, l, f, sa, al)
#define recvmsg(s, m, f) rrecvmsg(s, m, f)
#define send(s, b, l, f) rsend(s, b, l, f)
#define sendto(s, b, l, f, da, al) rsendto(s, b, l, f, da, al)
#define sendmsg(s, m, f) rsendmsg(s, m, f)

#define getpeername(s, a, l) rgetpeername(s, a, l)
#define getsockname(s, a, l) rgetsockname(s, a, l)

#define setsockopt(s, l, on, ov, ol) rsetsockopt(s, l, on, ov, ol)
#define getsockopt(s, l, on, ov, ol) rgetsockopt(s, l, on, ov, ol)
#endif

#endif   /* PG_SOCKET_H */
