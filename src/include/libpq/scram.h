/*-------------------------------------------------------------------------
 *
 * scram.h
 *	  Interface to libpq/scram.c
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/libpq/scram.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SCRAM_H
#define PG_SCRAM_H

/* Name of SCRAM-SHA-256 per IANA */
#define SCRAM_SHA256_NAME "SCRAM-SHA-256"

extern void *scram_init(const char *username, const char *verifier);
extern int scram_exchange(void *opaq, char *input, int inputlen,
			   char **output, int *outputlen);
extern char *scram_build_verifier(const char *username,
					 const char *password,
					 int iterations);
extern bool is_scram_verifier(const char *verifier);

#endif
