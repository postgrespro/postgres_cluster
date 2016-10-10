/*-------------------------------------------------------------------------
 *
 * crypt.h
 *	  Interface to libpq/crypt.c
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/libpq/crypt.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CRYPT_H
#define PG_CRYPT_H

#include "libpq/libpq-be.h"

extern int get_role_details(const char *role, char **password,
				 TimestampTz *vuntil, bool *vuntil_null, char **logdetail);
extern int md5_crypt_verify(const Port *port, const char *role,
				 char *client_pass, char **logdetail);

#endif
