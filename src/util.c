#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

int inrange(int min, int x, int max) {
	assert(min <= max);
	return (min <= x) && (x <= max);
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
