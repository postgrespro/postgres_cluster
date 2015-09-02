#ifndef UTIL_H
#define UTIL_H

#if defined(__APPLE__)
#define off64_t off_t
#endif

#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "int.h"

char *join_path(const char *dir, const char *file);
bool inrange(cid_t min, cid_t x, cid_t max);
int falloc(int fd, off64_t size);

#define shout(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr); \
	} while (0)

#endif
