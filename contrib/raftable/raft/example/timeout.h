#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <sys/time.h>
#include <stdbool.h>

typedef struct timeout_t {
	/*
	 * == 0: nowait
	 *  > 0: timer
	 *  < 0: wait indefinitely
	 */
	int msec_limit;

	struct timeval start;
} timeout_t;

void timeout_start(timeout_t *t, int msec);
bool timeout_nowait(timeout_t *t);
bool timeout_indefinite(timeout_t *t);
bool timeout_happened(timeout_t *t);
int timeout_elapsed_ms(timeout_t *t);
int timeout_remaining_ms(timeout_t *t);

#define TIMEOUT_LOOP_START(T) while (!timeout_happened(T)) {
#define TIMEOUT_LOOP_END(T) if (timeout_nowait(T)) break; }

#endif
