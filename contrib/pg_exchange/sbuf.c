/*
 * sbuf.c
 *
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "nodes/pg_list.h"
#include "utils/memutils.h" /* MemoryContexts */

#include "common.h"
#include "sbuf.h"

typedef struct
{
	StreamDataPackage header;
	int ntuples;
	char *curptr;
	char data[FLEXIBLE_ARRAY_MEMBER];
} TupleBuffer;

#define TupleBufferMinSize offsetof(TupleBuffer, data)
#define SDP_FREE_SPACE(buf) (StorageSize(buf) - (buf->curptr - &buf->data[0]))

static List *freebufs = NIL;

static int
StorageSize(const TupleBuffer *buf)
{
	return (SDP_Size(buf) - TupleBufferMinSize);
}

bool
SDP_IsEmpty(const StreamDataPackage *buffer)
{
	TupleBuffer *buf = (TupleBuffer *) buffer;

	Assert(buf->ntuples >= 0);
	Assert(SDP_FREE_SPACE(buf) >= 0);
	return buf->ntuples == 0;
}

int
SDP_Actual_size(const StreamDataPackage *buffer)
{
	TupleBuffer *buf;

	if (buffer->datalen <= 1)
		return SDP_Size(buffer);
	Assert(IsSDPBuf(buffer));

	buf = (TupleBuffer *) buffer;
	return SDP_Size(buf) - SDP_FREE_SPACE(buf);
}

/*
 * Check correctness of Stream Data Package
 */
bool
IsSDPBuf(const StreamDataPackage *buffer)
{
	TupleBuffer *buf;

	if (buffer == NULL || buffer->datalen <= 1)
		return false;

	buf = (TupleBuffer *) buffer;

	if (buf->curptr == NULL || buf->ntuples < 0)
		return false;

	return true;
}

StreamDataPackage *
SDP_Alloc(int size)
{
	ListCell *lc;
	TupleBuffer *buf = NULL;
	MemoryContext OldMemoryContext;

	OldMemoryContext = MemoryContextSwitchTo(memory_context);

	/* To avoid palloc/free overheads we can store buffers */
	for (lc = list_head(freebufs); lc != NULL; lc = lnext(lc))
	{
		TupleBuffer *freebuf = (TupleBuffer *) lfirst(lc);

		if (freebuf->header.datalen < size)
			continue;

		buf = freebuf;
		freebufs = list_delete_ptr(freebufs, freebuf);
		break;
	}

	if (buf == NULL)
	{
		size = Max((TupleBufferMinSize + size), DEFAULT_PACKAGE_SIZE);

		/* No one buffer can be found */
		buf = palloc0(size + SDPHeaderSize);
		buf->header.datalen = size;
	}

	buf->header.index = -1;
	buf->curptr = &buf->data[0];
	buf->ntuples = 0;
	MemoryContextSwitchTo(OldMemoryContext);
	return (StreamDataPackage *) buf;
}

/*
 * Return buffer to the free buffers list
 */
void
SDP_Free(StreamDataPackage *buffer)
{
	TupleBuffer *buf = (TupleBuffer *) buffer;
	MemoryContext OldMemoryContext;

	OldMemoryContext = MemoryContextSwitchTo(memory_context);

	Assert(IsSDPBuf(buffer));
	buf->curptr = NULL;
	buf->ntuples = -1;
	freebufs = lappend(freebufs, buf);

	MemoryContextSwitchTo(OldMemoryContext);
}

bool
SDP_Store(StreamDataPackage *buffer, HeapTuple tuple)
{
	TupleBuffer *buf = (TupleBuffer *) buffer;
	HeapTuple dest;

	Assert(buf != NULL && tuple != NULL);
	/* Check that user not pass pointer to non-allocated buffer */
	Assert(buf->curptr != NULL && buf->ntuples >= 0);

	if (SDP_FREE_SPACE(buf) < HEAPTUPLESIZE + tuple->t_len)
		return true;

	dest = (HeapTuple) buf->curptr;
	dest->t_len = tuple->t_len;
	dest->t_self = tuple->t_self;
	dest->t_tableOid = tuple->t_tableOid;
	dest->t_data = (HeapTupleHeader) ((char *) dest + HEAPTUPLESIZE);
	buf->curptr += HEAPTUPLESIZE;
	memcpy((char *) dest->t_data, (char *) tuple->t_data, tuple->t_len);

	buf->curptr += tuple->t_len;
	buf->ntuples++;
	Assert(SDP_FREE_SPACE(buf) >= 0);

	return false;
}

void
SDP_PrepareToRead(StreamDataPackage *buffer)
{
	TupleBuffer *buf = (TupleBuffer *) buffer;

	Assert(IsSDPBuf(buffer));
	buf->curptr = buf->data;
}

HeapTuple
SDP_Get_tuple(StreamDataPackage *buffer)
{
	TupleBuffer *buf;
	HeapTuple tuple;

	Assert(IsSDPBuf(buffer));

	if (SDP_IsEmpty(buffer))
		return NULL;

	buf = (TupleBuffer *) buffer;
	tuple = (HeapTuple) buf->curptr;
	tuple->t_data = (HeapTupleHeader) ((char *) tuple + HEAPTUPLESIZE);

	buf->curptr += TupSize(tuple);
	buf->ntuples--;

	return tuple;
}
