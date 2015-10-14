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

#define MAX_CLOG_FILES 10 // FIXME: Enforce this limit.

typedef struct clogfile_chain_t {
	struct clogfile_chain_t *prev;
	clogfile_t file;
} clogfile_chain_t;

typedef struct clog_data_t {
	char *datadir;

	clogfile_chain_t *lastfile;
} clog_data_t;

static clogfile_chain_t *new_clogfile_chain(clogfile_t* file) {
	clogfile_chain_t *chain = malloc(sizeof(clogfile_chain_t));
	chain->prev = NULL;
	chain->file = *file;
	return chain;
}

static int get_latest_fileid(char *datadir) {
	DIR *d = opendir(datadir);

	if (!d) {
		shout("cannot open datadir\n");
		return -1;
	}

	int latest = 0;
	struct dirent *e;
	while ((e = readdir(d)) != NULL) {
		int len = strlen(e->d_name);
		if (len != 20) continue;
		int fileid;
		char ext[4];
		int r = sscanf(e->d_name, "%016x.%3s", &fileid, ext);
		if (r != 2) continue;
		if (strcmp(ext, "dat")) continue;
		if (fileid > latest) latest = fileid;
	}

	closedir(d);
	return latest;
}

static clogfile_chain_t *load_clogfile_chain(char *datadir) {
	clogfile_t file;
	int fileid = get_latest_fileid(datadir);
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

	clogfile_chain_t *lastfile = load_clogfile_chain(datadir);
	if (lastfile == NULL) {
		return clog;
	}

	clog = malloc(sizeof(clog_data_t));
	clog->datadir = datadir;
	clog->lastfile = lastfile;

	return clog;
}

// Find a file containing info about the given 'xid'. Return the clogfile
// pointer, or NULL if not found.
static clogfile_t *clog_xid_to_file(clog_t clog, xid_t xid) {
	clogfile_chain_t *cur;
	for (cur = clog->lastfile; cur; cur = cur->prev) {
		if (inrange(cur->file.min, xid, cur->file.max)) {
			return &cur->file;
		}
	}
	return NULL;
}

// Get the status of the specified global commit.
int clog_read(clog_t clog, xid_t xid) {
	clogfile_t *file = clog_xid_to_file(clog, xid);
	if (file) {
		int status = clogfile_get_status(file, xid);
		return status;
	} else {
		shout(
			"xid %016x status is out of range, "
			"you might be experiencing a bug in backend\n",
			xid
		);
		return BLANK;
	}
}

// Set the status of the specified global commit. Return 'true' on success,
// 'false' otherwise.
bool clog_write(clog_t clog, xid_t xid, int status) {
	clogfile_t *file = clog_xid_to_file(clog, xid);
	if (!file) {
		debug("xid %016llx out of range, creating the file\n", xid);
		clogfile_t newfile;
		if (!clogfile_open_by_id(&newfile, clog->datadir, XID_TO_FILEID(xid), true)) {
			shout(
				"failed to create new clogfile "
				"while saving transaction status\n"
			);
			return false;
		}

		clogfile_chain_t *lastfile = new_clogfile_chain(&newfile);
		lastfile->prev = clog->lastfile;
		clog->lastfile = lastfile;
	}
	file = clog_xid_to_file(clog, xid);
	if (!file) {
		shout("the file is absent despite our efforts\n");
		return false;
	}
	bool ok = clogfile_set_status(file, xid, status);
	return ok;
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
