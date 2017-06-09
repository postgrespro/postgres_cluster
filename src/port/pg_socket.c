/*-------------------------------------------------------------------------
 *
 * pg_socket.c
 *	  Definitions for socket functions.
 *
 *
 * Copyright (c) 2017, Postgres Professional
 *
 * src/port/pg_socket.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include "pg_socket.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>


#ifdef WITH_RSOCKET
/*
 * Copy of defines from dynloader.h
 */
#ifdef HAVE_DLOPEN

#include <dlfcn.h>

/*
 * In some older systems, the RTLD_NOW flag isn't defined and the mode
 * argument to dlopen must always be 1.  The RTLD_GLOBAL flag is wanted
 * if available, but it doesn't exist everywhere.
 * If it doesn't exist, set it to 0 so it has no effect.
 */
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#define pg_dlopen(f)	dlopen((f), RTLD_NOW | RTLD_GLOBAL)
#define pg_dlsym		dlsym
#define pg_dlclose		dlclose
#define pg_dlerror		dlerror
#endif   /* HAVE_DLOPEN */

/* Buffer structs used to initialize PgSocket with pg_socket() */
static PgSocketData rsocket_buf = {0};
static void *rdmacm_handle = NULL;
#endif   /* WITH_RSOCKET */

/*
 * Creates socket or rsocket if isRsocket is true. You must call
 * pg_closesocket() to close created socket (or rsocket) and to free PgSocket
 * struct.
 *
 * Returns PgSocket struct. Result is malloc'ed.
 * If an error happened returns NULL.
 */
PgSocket
pg_socket(int domain, int type, int protocol, bool isRsocket)
{
	PgSocket	sock;

#ifdef WITH_RSOCKET
	if (isRsocket)
		sock = initialize_rsocket();
	else
#endif
	{
		sock = initialize_socket();
	}

	/* Something happened during initialization */
	if (sock == NULL)
		return NULL;

	sock->fd = sock->socket(domain, type, protocol);
	sock->isRsocket = isRsocket;
	return sock;
}

/*
 * Closes socket and frees PgSocket struct
 */
int
pg_closesocket(PgSocket socket)
{
	int			res = socket->close(socket->fd);

	free((char *) socket);
	return res;
}

/*
 * Just copies pointers to socket functions to the socket_buf members.
 */
PgSocket
initialize_socket(void)
{
	PgSocket	sock = (PgSocketData *) malloc(sizeof(PgSocketData));

	if (sock == NULL)
#ifndef FRONTEND
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
#else
	{
		fprintf(stderr, "out of memory");
		return NULL;
	}
#endif

	sock->fd = PGINVALID_SOCKET;
	sock->isRsocket = false;

	sock->socket = &socket;
	sock->bind = &bind;
	sock->listen = &listen;
	sock->accept = &accept;
	sock->connect = &connect;
	sock->close = &close;

	sock->recv = &recv;
	sock->send = &send;
	sock->sendmsg = &sendmsg;

#ifdef HAVE_POLL
	sock->poll = &poll;
#endif
	sock->select = &select;

	sock->getsockname = &getsockname;
	sock->setsockopt = &setsockopt;
	sock->getsockopt = &getsockopt;

#if !defined(WIN32)
	sock->fcntl = &fcntl;
#endif

	return sock;
}

#ifdef WITH_RSOCKET
#define RDMACM_NAME "librdmacm.so.1"

static void *
get_function(const char *name)
{
	void	   *func;

	/* Something happened earlier */
	if (rdmacm_handle == NULL)
		return NULL;

	func = pg_dlsym(rdmacm_handle, name);
	if (func == NULL)
	{
		/* try to unlink library */
		pg_dlclose(rdmacm_handle);
		rdmacm_handle = NULL;
#ifndef FRONTEND
		ereport(ERROR,
				(errmsg("could not find function \"%s\" in library \"%s\"",
						name, RDMACM_NAME)));
#else
		fprintf(stderr, "could not find function \"%s\" in library \"%s\"",
				name, RDMACM_NAME);
#endif
	}

	return func;
}

static void
initialize_rsocket_buf(void)
{
	rdmacm_handle = pg_dlopen(RDMACM_NAME);
	if (rdmacm_handle == NULL)
	{
		char	   *error = (char *) pg_dlerror();
#ifndef FRONTEND
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not load library \"%s\": %s",
						RDMACM_NAME, error)));
#else
		fprintf(stderr, "could not load library \"%s\": %s",
				RDMACM_NAME, error);
		return;
#endif
	}
	rsocket_buf.fd = PGINVALID_SOCKET;
	rsocket_buf.isRsocket = true;

	/*
	 * If an error will happened in get_function() library will be closed
	 * by pg_dlclose() and rdmacm_handle will be assigned to NULL
	 */

	rsocket_buf.socket = get_function("rsocket");
	rsocket_buf.bind = get_function("rbind");
	rsocket_buf.listen = get_function("rlisten");
	rsocket_buf.accept = get_function("raccept");
	rsocket_buf.connect = get_function("rconnect");
	rsocket_buf.close = get_function("rclose");

	rsocket_buf.recv = get_function("rrecv");
	rsocket_buf.send = get_function("rsend");
	rsocket_buf.sendmsg = get_function("rsendmsg");

#ifdef HAVE_POLL
	rsocket_buf.poll = get_function("rpoll");
#endif
	rsocket_buf.select = get_function("rselect");

	rsocket_buf.getsockname = get_function("rgetsockname");
	rsocket_buf.setsockopt = get_function("rsetsockopt");
	rsocket_buf.getsockopt = get_function("rgetsockopt");

#if !defined(WIN32)
	rsocket_buf.fcntl = get_function("rfcntl");
#endif
}

/*
 * Loads rdmacm library and copies pointers to rsocket functions to the
 * rsocket_buf members.
 */
PgSocket
initialize_rsocket(void)
{
	PgSocket	sock = (PgSocketData *) malloc(sizeof(PgSocketData));

	if (sock == NULL)
#ifndef FRONTEND
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
#else
	{
		fprintf(stderr, "out of memory");
		return NULL;
	}
#endif

	if (rdmacm_handle == NULL)
		initialize_rsocket_buf();

	/* Some error was happened in initialize_rsocket_buf() */
	if (rdmacm_handle == NULL)
	{
		free(sock);
		return NULL;
	}

	memcpy(sock, &rsocket_buf, sizeof(rsocket_buf));
	return sock;
}

/*
 * Call rpoll() from dynamically loaded library rdmacm
 */
int
rpoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	if (rdmacm_handle == NULL)
		initialize_rsocket_buf();
	if (rdmacm_handle)
		return rsocket_buf.poll(fds, nfds, timeout);
	else
		/* Some error was happened in initialize_rsocket_buf() */
		return -1;
}

/*
 * Call rselect() from dynamically loaded library rdmacm
 */
int
rselect(pgsocket nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	if (rdmacm_handle == NULL)
		initialize_rsocket_buf();
	if (rdmacm_handle)
		return rsocket_buf.select(nfds, readfds, writefds, exceptfds, timeout);
	else
		/* Some error was happened in initialize_rsocket_buf() */
		return -1;
}
#endif   /* WITH_RSOCKET */

/*
 * Put socket into nonblock mode.
 * Returns true on success, false on failure.
 */
bool
pg_set_noblock_extended(PgSocket sock)
{
	int			flags;

	flags = pg_fcntl(sock, F_GETFL, 0);
	if (flags < 0)
		return false;
	if (pg_fcntl(sock, F_SETFL, (flags | O_NONBLOCK)) == -1)
		return false;
	return true;
}

/*
 * Put socket into blocking mode.
 * Returns true on success, false on failure.
 */
bool
pg_set_block_extended(PgSocket sock)
{
	int			flags;

	flags = pg_fcntl(sock, F_GETFL, 0);
	if (flags < 0)
		return false;
	if (pg_fcntl(sock, F_SETFL, (flags & ~O_NONBLOCK)) == -1)
		return false;
	return true;
}
