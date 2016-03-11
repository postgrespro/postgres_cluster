#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

bool inrange(int min, int x, int max);

static inline int min(int a, int b) {
	return a < b ? a : b;
}

static inline int max(int a, int b) {
	return a > b ? a : b;
}

static inline int rand_between(int min, int max) {
	return rand() % (max - min + 1) + min;
}

// ------ timing ------

typedef struct mstimer_t {
	struct timeval tv;
} mstimer_t;

int mstimer_reset(mstimer_t *t);
struct timeval ms2tv(int ms);

// ------ logging ------

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
