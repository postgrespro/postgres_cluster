/*
 * This module provides a low-level API to access clog files.
 */

#include <stdbool.h>
#include "int.h"

#ifndef CLOGFILE_H
#define CLOGFILE_H

#define BITS_PER_COMMIT 2
#define COMMIT_MASK ((1 << BITS_PER_COMMIT) - 1)
#define COMMITS_PER_BYTE 4
#define COMMITS_PER_FILE 1024 // 0x100000000
#define BYTES_PER_FILE ((COMMITS_PER_FILE) / (COMMITS_PER_BYTE))
#define GCID_TO_FILEID(GCID) ((GCID) / (COMMITS_PER_FILE))
#define GCID_TO_OFFSET(GCID) (((GCID) % (COMMITS_PER_FILE)) / (COMMITS_PER_BYTE))
#define GCID_TO_SUBOFFSET(GCID) (((GCID) % (COMMITS_PER_FILE)) % (COMMITS_PER_BYTE))

typedef struct clogfile_t {
	char *path;
	cid_t min;
	cid_t max;
	void *data; // ptr for mmap
} clogfile_t;

// Open a clog file with the gived id. Create before opening if 'create' is
// true. Return 'true' on success, 'false' otherwise.
bool clogfile_open_by_id(clogfile_t *clogfile, char *datadir, int fileid, bool create);

// Close and remove the given clog file. Return 'true' on success, 'false'
// otherwise.
bool clogfile_remove(clogfile_t *clogfile);

// Close the specified clogfile. Return 'true' on success, 'false' otherwise.
bool clogfile_close(clogfile_t *clogfile);

// Get the status of the specified global commit from the clog file.
int clogfile_get_status(clogfile_t *clogfile, cid_t gcid);

// Set the status of the specified global commit in the clog file. Return
// 'true' on success, 'false' otherwise.
bool clogfile_set_status(clogfile_t *clogfile, cid_t gcid, int status);

#endif
