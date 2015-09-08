/*
 * This module provides a low-level API to access clog files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>

#include "clogfile.h"
#include "util.h"

static char *clogfile_get_path(char *datadir, int fileid) {
	char fileidstr[21];
	sprintf(fileidstr, "%016llx.dat", (unsigned long long)fileid);
	return join_path(datadir, fileidstr);
}

// Open a clog file with the gived id. Create before opening if 'create' is
// true. Return 'true' on success, 'false' otherwise.
bool clogfile_open_by_id(clogfile_t *clogfile, char *datadir, int fileid, bool create) {
	clogfile->path = clogfile_get_path(datadir, fileid);
	clogfile->min = COMMITS_PER_FILE * fileid;
	clogfile->max = clogfile->min + COMMITS_PER_FILE - 1;

	int fd;
	if (create) {
		fd = open(clogfile->path, O_RDWR | O_CREAT | O_EXCL, 0660);
		if (fd == -1) {
			shout("cannot create clog file '%s': %s\n", clogfile->path, strerror(errno));
			return false;
		}
		shout("created clog file '%s'\n", clogfile->path);
	} else {
		fd = open(clogfile->path, O_RDWR);
		if (fd == -1) {
			shout("cannot open clog file '%s': %s\n", clogfile->path, strerror(errno));
			return false;
		}
		shout("opened clog file '%s'\n", clogfile->path);
	}

	if (falloc(fd, BYTES_PER_FILE)) {
		shout("cannot allocate clog file '%s': %s\n", clogfile->path, strerror(errno));
		close(fd);
		return false;
	}

	clogfile->data = mmap(
		NULL, BYTES_PER_FILE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED, fd, 0
	);
	// mmap will keep the file referenced, even after we close the fd
	close(fd);

	if (clogfile->data == MAP_FAILED) {
		shout("cannot mmap clog file '%s': %s\n", clogfile->path, strerror(errno));
		return false;
	}
	return true;
}

// Close and remove the given clog file. Return 'true' on success, 'false'
// otherwise.
bool clogfile_remove(clogfile_t *clogfile) {
	if (!clogfile_close(clogfile)) {
		return false;
	}
	if (unlink(clogfile->path)) {
		return false;
	}
	return true;
} 

// Close the specified clogfile. Return 'true' on success, 'false' otherwise.
bool clogfile_close(clogfile_t *clogfile) {
	if (munmap(clogfile->data, BYTES_PER_FILE)) {
		return false;
	}
	return true;
}

// Get the status of the specified global commit from the clog file.
int clogfile_get_status(clogfile_t *clogfile, xid_t gxid) {
	off64_t offset = GXID_TO_OFFSET(gxid);
	int suboffset = GXID_TO_SUBOFFSET(gxid);
	char *p = ((char*)clogfile->data + offset);
	return ((*p) >> (BITS_PER_COMMIT * suboffset)) & COMMIT_MASK; // AND-out all other status
}

// Set the status of the specified global commit in the clog file. Return
// 'true' on success, 'false' otherwise.
bool clogfile_set_status(clogfile_t *clogfile, xid_t gxid, int status) {
	off64_t offset = GXID_TO_OFFSET(gxid);
	int suboffset = GXID_TO_SUBOFFSET(gxid);
	char *p = ((char*)clogfile->data + offset);
	*p &= ~(COMMIT_MASK << (BITS_PER_COMMIT * suboffset));   // AND-out the old status
	*p |= status << (BITS_PER_COMMIT * suboffset); // OR-in the new status
	if (msync(clogfile->data, BYTES_PER_FILE, MS_SYNC)) {
		shout("cannot msync clog file '%s': %s\n", clogfile->path, strerror(errno));
		return false;
	}
	return true;
}
