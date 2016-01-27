#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

// FIXME: different OSes use different separators
#define PATHSEP '/'

char *join_path(const char *dir, const char *file) {
	size_t dirlen, filelen, pathlen;
	char *path;

	assert(dir != NULL);
	assert(file != NULL);

	dirlen = strlen(dir);
	if (dirlen > 0) {
		if (dir[dirlen - 1] == PATHSEP) {
			// do not copy the separator
			dirlen -= 1;
		}
	} else {
		// 'dir' is empty
		return strdup(file);
	}

	filelen = strlen(file);
	if (filelen > 0) {
		if (file[0] == PATHSEP) {
			// 'file' is an absolute path
			return strdup(file);
		}
	} else {
		// 'file' is empty
		return strndup(dir, dirlen);
	}

	pathlen = dirlen + 1 + filelen;

	path = malloc(pathlen + 1);

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

#define MAX_ELAPSED 30
int mstimer_reset(mstimer_t *t) {
	int ms;
	struct timeval newtime;
	gettimeofday(&newtime, NULL);

	ms =
		(newtime.tv_sec - t->tv.tv_sec) * 1000 +
		(newtime.tv_usec - t->tv.tv_usec) / 1000;

	t->tv = newtime;

	if (ms > MAX_ELAPSED) {
		return MAX_ELAPSED;
	}
	return ms;
}

struct timeval ms2tv(int ms) {
	struct timeval result;
	result.tv_sec = ms / 1000;
	result.tv_usec = ((ms % 1000) * 1000);
	return result;
}
