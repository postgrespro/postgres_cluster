/*
 * This module provides a high-level API to access clog files.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "clog.h"
#include "clogfile.h"
#include "util.h"

#define FRESHFILE "fresh.gcid"
#define MAX_CLOG_FILES 10 // FIXME: Enforce this limit.

typedef struct clogfile_chain_t {
	struct clogfile_chain_t *prev;
	clogfile_t file;
} clogfile_chain_t;

typedef struct clog_data_t {
	char *datadir;
	int fresh_fd; // the file to keep the next fresh gcid
	cid_t fresh_gcid;

	// First commit with unknown status (used as a snapshot).
	// This value is supposed to be initialized from 'fresh_gcid' on
	// startup, and then be updated on each commit/abort.
	cid_t horizon;

	clogfile_chain_t *lastfile;
} clog_data_t;

static clogfile_chain_t *new_clogfile_chain(clogfile_t* file) {
	clogfile_chain_t *chain = malloc(sizeof(clogfile_chain_t));
	chain->prev = NULL;
	chain->file = *file;
	return chain;
}

static clogfile_chain_t *load_clogfile_chain(char *datadir, int fileid) {
	clogfile_t file;
	if (!clogfile_open_by_id(&file, datadir, fileid, false)) {
		// This may be the first launch, so try to create the file.
		if (!clogfile_open_by_id(&file, datadir, fileid, true)) {
			return NULL;
		}
	}
	clogfile_chain_t *head = new_clogfile_chain(&file);

	clogfile_chain_t *tail = head;
	while (fileid-- > 0) {
		if (!clogfile_open_by_id(&file, datadir, fileid, false)) {
			break;
		}
		tail->prev = new_clogfile_chain(&file);
		tail = tail->prev;
	}

	return head;
}

// Open the clog at the specified path. Try not to open the same datadir twice
// or in two different processes. Return a clog object on success, NULL
// otherwise.
clog_t clog_open(char *datadir) {
	clog_t clog = NULL;
	char *path = join_path(datadir, FRESHFILE);
	int fd = open(path, O_RDWR | O_CREAT, 0660);
	if (fd == -1) {
		shout("cannot open/create freshfile '%s': %s\n", path, strerror(errno));
		goto cleanup_path;
	}

	int res;
#if defined(__APPLE__)
	fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, sizeof(cid_t), 0};
	res = fcntl(fd, F_PREALLOCATE, &store);

	if (res == -1) {
		// try and allocate space with fragments
		store.fst_flags = F_ALLOCATEALL;
		res = fcntl(fd, F_PREALLOCATE, &store);
	}

	if (res != -1) {
		res = ftruncate(fd, sizeof(cid_t));
	}
#else
	res = posix_fallocate(fd, 0, sizeof(cid_t));
#endif

	if (res) {
		shout("cannot allocate freshfile '%s': %s\n", path, strerror(errno));
		goto cleanup_fd;
	}

	cid_t fresh_gcid;
	ssize_t r = pread(fd, &fresh_gcid, sizeof(cid_t), 0);
	if (r == -1) {
		shout("cannot read fresh gcid from freshfile: %s\n", strerror(errno));
		goto cleanup_fd;
	}
	if (r != sizeof(cid_t)) {
		// FIXME: It is not an error if read returns less than requested.
		shout(
			"cannot read fresh gcid from freshfile, "
			"read %ld bytes instead of %lu\n",
			r, sizeof(cid_t)
		);
		goto cleanup_fd;
	}
	if (fresh_gcid < MIN_GCID) {
		fresh_gcid = MIN_GCID;
	}

	// Load the clog files starting from the most recent.
	clogfile_chain_t *lastfile = load_clogfile_chain(
		datadir, GCID_TO_FILEID(fresh_gcid)
	);
	if (lastfile == NULL) {
		goto cleanup_fd;
	}

	clog = malloc(sizeof(clog_data_t));
	clog->datadir = datadir;
	clog->fresh_fd = fd;
	clog->fresh_gcid = fresh_gcid;
	clog->horizon = fresh_gcid;
	clog->lastfile = lastfile;
	goto cleanup_path;

cleanup_fd:
	close(fd);
cleanup_path:
	free(path);
	return clog;
}

// Find a file containing info about the given 'gcid'. Return the clogfile
// pointer, or NULL if not found.
static clogfile_t *clog_gcid_to_file(clog_t clog, cid_t gcid) {
	clogfile_chain_t *cur;
	for (cur = clog->lastfile; cur; cur = cur->prev) {
		if (inrange(cur->file.min, gcid, cur->file.max)) {
			return &cur->file;
		}
	}
	return NULL;
}

// Get the status of the specified global commit.
int clog_read(clog_t clog, cid_t gcid) {
	clogfile_t *file = clog_gcid_to_file(clog, gcid);
	if (file) {
		int status = clogfile_get_status(file, gcid);
		if ((status == COMMIT_UNKNOWN) && (gcid < clog->horizon)) {
			// An unknown status that should be known. That means
			// we have crashed between prepare and the
			// corresponding commit/abort. Consider it aborted.
			if (clogfile_set_status(file, gcid, COMMIT_NO)) {
				shout("marked crashed prepare %016llx as aborted\n", gcid);
			} else {
				shout("couldn't mark crashed prepare %016llx as aborted!\n", gcid);
			}
			return COMMIT_NO;
		}
		return status;
	} else {
		shout(
			"gcid %016llx status is out of range, "
			"you might be experiencing a bug in backend\n",
			gcid
		);
		return COMMIT_UNKNOWN;
	}
}

static void clog_move_horizon(clog_t clog) {
	while (clog_read(clog, clog->horizon) != COMMIT_UNKNOWN) {
		clog->horizon++;
	}
}

// Set the status of the specified global commit. Return 'true' on success,
// 'false' otherwise.
bool clog_write(clog_t clog, cid_t gcid, int status) {
	clogfile_t *file = clog_gcid_to_file(clog, gcid);
	if (file) {
		bool ok = clogfile_set_status(file, gcid, status);
		clog_move_horizon(clog);
		return ok;
	} else {
		shout(
			"cannot set gcid %016llx status, out of range, "
			"you might be experiencing a bug in backend\n",
			gcid
		);
		return false;
	}
}

// Allocate a fresh unused gcid. Return INVALID_GCID on error.
cid_t clog_advance(clog_t clog) {
	cid_t old_gcid = clog->fresh_gcid++;
	ssize_t written = pwrite(
		clog->fresh_fd, &clog->fresh_gcid,
		sizeof(cid_t), 0
	);
	if (written != sizeof(cid_t)) {
		shout("failed to store the fresh gcid value\n");
		return INVALID_GCID;
	}
	fsync(clog->fresh_fd);

	int oldf = GCID_TO_FILEID(old_gcid);
	int newf = GCID_TO_FILEID(clog->fresh_gcid);
	if (oldf != newf) {
		// create new clogfile
		clogfile_t file;
		if (!clogfile_open_by_id(&file, clog->datadir, newf, true)) {
			shout(
				"failed to create new clogfile "
				"while advancing fresh gcid\n"
			);
			return INVALID_GCID;
		}

		clogfile_chain_t *lastfile = new_clogfile_chain(&file);
		lastfile->prev = clog->lastfile;
		clog->lastfile = lastfile;
	}

	return old_gcid;
}

// Get the first unknown commit id (used as a snapshot). Return INVALID_GCID on
// error.
cid_t clog_horizon(clog_t clog) {
    return clog->fresh_gcid;
//	return clog->horizon;
}

// Forget about the commits before the given one ('until'), and free the
// occupied space if possible. Return 'true' on success, 'false' otherwise.
bool clog_forget(clog_t clog, cid_t until) {
	clogfile_chain_t *cur = clog->lastfile;
	while (cur->prev) {
		if (cur->prev->file.max < until) {
			clogfile_chain_t *victim = cur->prev;
			cur->prev = victim->prev;

			if (!clogfile_remove(&victim->file)) {
				shout(
					"couldn't remove clogfile '%s'\n",
					victim->file.path
				);
				free(victim);
				return false;
			}
			free(victim);
		} else {
			cur = cur->prev;
		}
	}

	return true;
}

// Close the specified clog. Do not use the clog object after closing. Return
// 'true' on success, 'false' otherwise.
bool clog_close(clog_t clog) {
	while (clog->lastfile) {
		clogfile_chain_t *f = clog->lastfile;
		clog->lastfile = f->prev;

		clogfile_close(&f->file);
		free(f);
	}
	free(clog);
	return true;
}
