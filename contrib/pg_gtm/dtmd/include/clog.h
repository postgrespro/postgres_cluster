/*
 * This module provides a high-level API to access clog files.
 */

#ifndef CLOG_H
#define CLOG_H

#include <stdbool.h>
#include "int.h"

#define INVALID_GCID 0
#define MIN_GCID 42
#define MAX_GCID 0xdeadbeefcafebabe

#define COMMIT_UNKNOWN 0
#define COMMIT_YES     1
#define COMMIT_NO      2

typedef struct clog_data_t *clog_t;

// Open the clog at the specified path. Try not to open the same datadir twice
// or in two different processes. Return a clog object on success, NULL
// otherwise.
clog_t clog_open(char *datadir);

// Get the status of the specified global commit.
int clog_read(clog_t clog, cid_t gcid);

// Set the status of the specified global commit. Return 'true' on success,
// 'false' otherwise.
bool clog_write(clog_t clog, cid_t gcid, int status);

// Allocate a fresh unused gcid. Return INVALID_GCID on error.
cid_t clog_advance(clog_t clog);

// Get the first unknown commit id (used as a snapshot). Return INVALID_GCID on
// error.
cid_t clog_horizon(clog_t clog);

// Forget about the commits before the given one ('until'), and free the
// occupied space if possible. Return 'true' on success, 'false' otherwise.
bool clog_forget(clog_t clog, cid_t until);

// Close the specified clog. Do not use the clog object after closing. Return
// 'true' on success, 'false' otherwise.
bool clog_close(clog_t clog);

#endif
