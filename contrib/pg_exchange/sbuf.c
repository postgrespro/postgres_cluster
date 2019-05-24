/*
 * sbuf.c
 *
 */

#include "sbuf.h"

void
initTupleBuffer(TupleBuffer *tbuf, size_t mem_size)
{
	tbuf->curptr = &tbuf->data;
	/* Will corrected before send to DMQ for 'trim tails' purpose. */
	tbuf->size = mem_size;
}
