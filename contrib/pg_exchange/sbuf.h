/*
 * sbuf.h
 *
 */

#ifndef SBUF_H_
#define SBUF_H_

#include "postgres.h"

typedef struct TupleBuffer
{
	size_t size;
	void *curptr;
	char data[FLEXIBLE_ARRAY_MEMBER];
} TupleBuffer;

#define DEFAULT_TUPLEBUF_SIZE	(BLCKSZ * 2)

extern void initTupleBuffer(TupleBuffer *tbuf, size_t mem_size);

#endif /* SBUF_H_ */
