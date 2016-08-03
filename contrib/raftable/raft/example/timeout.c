#include "timeout.h"

#include <stdio.h>

void timeout_start(timeout_t *t, int msec) {
	t->msec_limit = msec;
	gettimeofday(&t->start, NULL);
}

static long msec(struct timeval *tv) {
	return tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

bool timeout_nowait(timeout_t *t) {
	return t->msec_limit == 0;
}

bool timeout_indefinite(timeout_t *t) {
	return t->msec_limit < 0;
}

bool timeout_happened(timeout_t *t) {
	if (timeout_nowait(t)) return false;
	if (timeout_indefinite(t)) return false;

	return timeout_elapsed_ms(t) > t->msec_limit;
}

int timeout_elapsed_ms(timeout_t *t) {
	struct timeval now, diff;
	gettimeofday(&now, NULL);
	timersub(&now, &t->start, &diff);
	return msec(&diff);
}

int timeout_remaining_ms(timeout_t *t) {
	int remaining_ms;
	if (timeout_nowait(t)) return 0;
	if (timeout_indefinite(t)) return -1;

	remaining_ms = t->msec_limit - timeout_elapsed_ms(t);
	if (remaining_ms > 0) {
		return remaining_ms;
	} else {
		return 0;
	}
}
