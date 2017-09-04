/*-------------------------------------------------------------------------
 *
 * noblock.c
 *	  set a file descriptor as blocking or non-blocking
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/port/noblock.c
 *
 *-------------------------------------------------------------------------
 */

#include "c.h"

#include <fcntl.h>

#include "pg_socket.h"


/*
 * Put socket into nonblock mode.
 * Returns true on success, false on failure.
 */
bool
pg_set_noblock(pgsocket sock, bool isRsocket)
{
#if !defined(WIN32)
	int			flags;

	flags = pg_fcntl(sock, F_GETFL, 0, isRsocket);
	if (flags < 0)
		return false;
	if (pg_fcntl(sock, F_SETFL, (flags | O_NONBLOCK), isRsocket) == -1)
		return false;
	return true;
#else
	unsigned long ioctlsocket_ret = 1;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}

/*
 * Put socket into blocking mode.
 * Returns true on success, false on failure.
 */
bool
pg_set_block(pgsocket sock, bool isRsocket)
{
#if !defined(WIN32)
	int			flags;

	flags = pg_fcntl(sock, F_GETFL, 0, isRsocket);
	if (flags < 0)
		return false;
	if (pg_fcntl(sock, F_SETFL, (flags & ~O_NONBLOCK), isRsocket) == -1)
		return false;
	return true;
#else
	unsigned long ioctlsocket_ret = 0;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}
