/*
 * This module provides a high-level API to access clog files.
 */

#ifndef CLOG_H
#define CLOG_H

#include <stdbool.h>
#include "int.h"

#define INVALID_GXID 0
#define MIN_GXID 42
#define MAX_GXID 0xdeadbeefcafebabe

#define NEUTRAL  0
#define POSITIVE 1
#define NEGATIVE 2

typedef struct clog_data_t *clog_t;

// Open the clog at the specified path. Try not to open the same datadir twice
// or in two different processes. Return a clog object on success, NULL
// otherwise.
clog_t clog_open(char *datadir);

// Get the status of the specified global commit.
int clog_read(clog_t clog, xid_t gxid);

// Set the status of the specified global commit. Return 'true' on success,
// 'false' otherwise.
bool clog_write(clog_t clog, xid_t gxid, int status);

// Forget about the commits before the given one ('until'), and free the
// occupied space if possible. Return 'true' on success, 'false' otherwise.
bool clog_forget(clog_t clog, xid_t until);

// Close the specified clog. Do not use the clog object after closing. Return
// 'true' on success, 'false' otherwise.
bool clog_close(clog_t clog);

#endif
