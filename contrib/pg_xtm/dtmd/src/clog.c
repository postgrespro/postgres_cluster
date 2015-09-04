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

#define FRESHFILE "fresh.gxid"
#define MAX_CLOG_FILES 10 // FIXME: Enforce this limit.

typedef struct clogfile_chain_t {
	struct clogfile_chain_t *prev;
	clogfile_t file;
} clogfile_chain_t;

typedef struct clog_data_t {
	char *datadir;
	int fresh_fd; // the file to keep the next fresh gxid
	xid_t fresh_gxid;

	// First commit with unknown status (used as a snapshot).
	// This value is supposed to be initialized from 'fresh_gxid' on
	// startup, and then be updated on each commit/abort.
	xid_t horizon;

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

	int res = falloc(fd, sizeof(xid_t));
	if (res) {
		shout("cannot allocate freshfile '%s': %s\n", path, strerror(errno));
		goto cleanup_fd;
	}

	xid_t fresh_gxid;
	ssize_t r = pread(fd, &fresh_gxid, sizeof(xid_t), 0);
	if (r == -1) {
		shout("cannot read fresh gxid from freshfile: %s\n", strerror(errno));
		goto cleanup_fd;
	}
	if (r != sizeof(xid_t)) {
		// FIXME: It is not an error if read returns less than requested.
		shout(
			"cannot read fresh gxid from freshfile, "
			"read %ld bytes instead of %lu\n",
			r, sizeof(xid_t)
		);
		goto cleanup_fd;
	}
	if (fresh_gxid < MIN_GXID) {
		fresh_gxid = MIN_GXID;
	}

	// Load the clog files starting from the most recent.
	clogfile_chain_t *lastfile = load_clogfile_chain(
		datadir, GXID_TO_FILEID(fresh_gxid)
	);
	if (lastfile == NULL) {
		goto cleanup_fd;
	}

	clog = malloc(sizeof(clog_data_t));
	clog->datadir = datadir;
	clog->fresh_fd = fd;
	clog->fresh_gxid = fresh_gxid;
	clog->horizon = fresh_gxid;
	clog->lastfile = lastfile;
	goto cleanup_path;

cleanup_fd:
	close(fd);
cleanup_path:
	free(path);
	return clog;
}

// Find a file containing info about the given 'gxid'. Return the clogfile
// pointer, or NULL if not found.
static clogfile_t *clog_gxid_to_file(clog_t clog, xid_t gxid) {
	clogfile_chain_t *cur;
	for (cur = clog->lastfile; cur; cur = cur->prev) {
		if (inrange(cur->file.min, gxid, cur->file.max)) {
			return &cur->file;
		}
	}
	return NULL;
}

// Get the status of the specified global commit.
int clog_read(clog_t clog, xid_t gxid) {
	clogfile_t *file = clog_gxid_to_file(clog, gxid);
	if (file) {
		int status = clogfile_get_status(file, gxid);
		if ((status == COMMIT_UNKNOWN) && (gxid < clog->horizon)) {
			// An unknown status that should be known. That means
			// we have crashed between prepare and the
			// corresponding commit/abort. Consider it aborted.
			if (clogfile_set_status(file, gxid, COMMIT_NO)) {
				shout("marked crashed prepare %016llx as aborted\n", gxid);
			} else {
				shout("couldn't mark crashed prepare %016llx as aborted!\n", gxid);
			}
			return COMMIT_NO;
		}
		return status;
	} else {
		shout(
			"gxid %016llx status is out of range, "
			"you might be experiencing a bug in backend\n",
			gxid
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
bool clog_write(clog_t clog, xid_t gxid, int status) {
	clogfile_t *file = clog_gxid_to_file(clog, gxid);
	if (file) {
		bool ok = clogfile_set_status(file, gxid, status);
		clog_move_horizon(clog);
		return ok;
	} else {
		shout(
			"cannot set gxid %016llx status, out of range, "
			"you might be experiencing a bug in backend\n",
			gxid
		);
		return false;
	}
}

// Allocate a fresh unused gxid. Return INVALID_GXID on error.
xid_t clog_advance(clog_t clog) {
	xid_t old_gxid = clog->fresh_gxid++;
	ssize_t written = pwrite(
		clog->fresh_fd, &clog->fresh_gxid,
		sizeof(xid_t), 0
	);
	if (written != sizeof(xid_t)) {
		shout("failed to store the fresh gxid value\n");
		return INVALID_GXID;
	}
	fsync(clog->fresh_fd);

	int oldf = GXID_TO_FILEID(old_gxid);
	int newf = GXID_TO_FILEID(clog->fresh_gxid);
	if (oldf != newf) {
		// create new clogfile
		clogfile_t file;
		if (!clogfile_open_by_id(&file, clog->datadir, newf, true)) {
			shout(
				"failed to create new clogfile "
				"while advancing fresh gxid\n"
			);
			return INVALID_GXID;
		}

		clogfile_chain_t *lastfile = new_clogfile_chain(&file);
		lastfile->prev = clog->lastfile;
		clog->lastfile = lastfile;
	}

	return old_gxid;
}

// Get the first unknown commit id (used as xmin). Return INVALID_GXID on
// error.
xid_t clog_horizon(clog_t clog) {
	return clog->horizon;
}

// Forget about the commits before the given one ('until'), and free the
// occupied space if possible. Return 'true' on success, 'false' otherwise.
bool clog_forget(clog_t clog, xid_t until) {
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
