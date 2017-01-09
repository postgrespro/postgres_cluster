/*-------------------------------------------------------------------------
 *
 * noblock.c
 *	  set a file descriptor as non-blocking
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
#ifdef WITH_RSOCKET
#include <rdma/rsocket.h>
#endif


bool
pg_set_noblock(pgsocket sock, bool isRsocket)
{
#if !defined(WIN32)
#ifdef WITH_RSOCKET
	if (isRsocket)
		return (rfcntl(sock, F_SETFL, O_NONBLOCK) != -1);
	else
#endif
		return (fcntl(sock, F_SETFL, O_NONBLOCK) != -1);
#else
	unsigned long ioctlsocket_ret = 1;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}


bool
pg_set_block(pgsocket sock, bool isRsocket)
{
#if !defined(WIN32)
	int			flags;

#ifdef WITH_RSOCKET
	if (isRsocket)
	{
		flags = rfcntl(sock, F_GETFL);
		if (flags < 0 || rfcntl(sock, F_SETFL, (long) (flags & ~O_NONBLOCK)))
			return false;
	}
	else
#endif
	{
		flags = fcntl(sock, F_GETFL);
		if (flags < 0 || fcntl(sock, F_SETFL, (long) (flags & ~O_NONBLOCK)))
			return false;
	}
	return true;
#else
	unsigned long ioctlsocket_ret = 0;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}
