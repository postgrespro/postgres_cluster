#include "timeout.h"

void timeout_start(timeout_t *t, int msec)
{
	t->msec_limit = msec;
	t->start = GetCurrentTimestamp();
}

static long msec(TimestampTz timer)
{
	long sec;
	int usec;
	TimestampDifference(0, timer, &sec, &usec);
	return sec * 1000 + usec / 1000;
}

bool timeout_nowait(timeout_t *t)
{
	return t->msec_limit == 0;
}

bool timeout_indefinite(timeout_t *t)
{
	return t->msec_limit < 0;
}

bool timeout_happened(timeout_t *t)
{
	if (timeout_nowait(t)) return false;
	if (timeout_indefinite(t)) return false;

	return timeout_elapsed_ms(t) > t->msec_limit;
}

int timeout_elapsed_ms(timeout_t *t)
{
	TimestampTz now = GetCurrentTimestamp();
	return msec(now - t->start);
}

int timeout_remaining_ms(timeout_t *t)
{
	int remaining_ms;
	if (timeout_nowait(t)) return 0;
	if (timeout_indefinite(t)) return -1;

	remaining_ms = t->msec_limit - timeout_elapsed_ms(t);
	if (remaining_ms > 0)
		return remaining_ms;
	else
		return 0;
}
