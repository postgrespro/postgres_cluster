/*-------------------------------------------------------------------------
 *
 * pg_rsocket.c
 *	  Functions to dynamically loading of librdmacm
 *
 *
 * Copyright (c) 2017, Postgres Professional
 *
 * src/port/pg_rsocket.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif
#include "pg_socket.h"

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

#define RDMACM_NAME "librdmacm.so.1"

/* Declarations of rsocket functions */
PgSocketCall *rcalls = NULL;

static void *rdmacm_handle = NULL;

/*
 * Returns pointer to the function of the library
 */
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

/*
 * Loads librdmacm and loads pointers to rsocket functions
 */
void
initialize_rsocket(void)
{
	/* librdmacm was loaded already */
	if (rdmacm_handle != NULL)
		return;

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

	rcalls = (PgSocketCall *) malloc(sizeof(PgSocketCall));

	if (rcalls == NULL)
#ifndef FRONTEND
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
#else
	{
		fprintf(stderr, "out of memory");
		return;
	}
#endif

	/*
	 * If an error will happened in get_function() library will be closed
	 * by pg_dlclose() and rdmacm_handle will be assigned to NULL
	 */

	rcalls->socket = get_function("rsocket");
	rcalls->bind = get_function("rbind");
	rcalls->listen = get_function("rlisten");
	rcalls->accept = get_function("raccept");
	rcalls->connect = get_function("rconnect");
	rcalls->close = get_function("rclose");

	rcalls->recv = get_function("rrecv");
	rcalls->send = get_function("rsend");
	rcalls->sendmsg = get_function("rsendmsg");

#ifdef HAVE_POLL
	rcalls->poll = get_function("rpoll");
#endif
	rcalls->select = get_function("rselect");

	rcalls->getsockname = get_function("rgetsockname");
	rcalls->setsockopt = get_function("rsetsockopt");
	rcalls->getsockopt = get_function("rgetsockopt");

#if !defined(WIN32)
	rcalls->fcntl = get_function("rfcntl");
#endif
}

#endif   /* WITH_RSOCKET */
