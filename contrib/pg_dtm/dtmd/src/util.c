#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

// FIXME: different OSes use different separators
#define PATHSEP '/'

char *join_path(const char *dir, const char *file) {
	assert(dir != NULL);
	assert(file != NULL);

	size_t dirlen = strlen(dir);
	if (dirlen > 0) {
		if (dir[dirlen - 1] == PATHSEP) {
			// do not copy the separator
			dirlen -= 1;
		}
	} else {
		// 'dir' is empty
		return strdup(file);
	}

	size_t filelen = strlen(file);
	if (filelen > 0) {
		if (file[0] == PATHSEP) {
			// 'file' is an absolute path
			return strdup(file);
		}
	} else {
		// 'file' is empty
		return strndup(dir, dirlen);
	}

	size_t pathlen = dirlen + 1 + filelen;

	char *path = malloc(pathlen + 1);

	strncpy(path, dir, dirlen);
	path[dirlen] = PATHSEP;
	strncpy(path + dirlen + 1, file, filelen + 1);

	return path;
}

bool inrange(xid_t min, xid_t x, xid_t max) {
	assert(min <= max);
	return (min <= x) && (x <= max);
}

int falloc(int fd, off64_t size) {
	int res;
#if defined(__APPLE__)
	fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, size, 0};
	res = fcntl(fd, F_PREALLOCATE, &store);

	if (res == -1) {
		// try and allocate space with fragments
		store.fst_flags = F_ALLOCATEALL;
		res = fcntl(fd, F_PREALLOCATE, &store);
	}

	if (res != -1) {
		res = ftruncate(fd, size);
	}
#else
	res = posix_fallocate64(fd, 0, size);
#endif
	return res;
}
