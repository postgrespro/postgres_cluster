#ifndef UTIL_H
#define UTIL_H

#if defined(__APPLE__)
#define off64_t off_t
#endif

#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "int.h"

char *join_path(const char *dir, const char *file);
bool inrange(xid_t min, xid_t x, xid_t max);
int falloc(int fd, off64_t size);

static inline int min(int a, int b) {
	return a < b ? a : b;
}

static inline int max(int a, int b) {
	return a > b ? a : b;
}

static inline int rand_between(int min, int max) {
	return rand() % (max - min + 1) + min;
}

#ifndef DEBUG
#define debug(...)
#else
#define debug(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr); \
	} while (0)
#endif

#define shout(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr); \
	} while (0)

#endif
