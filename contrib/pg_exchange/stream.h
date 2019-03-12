/*
 * stream.h
 *
 */

#ifndef CONTRIB_PG_EXCHANGE_STREAM_H_
#define CONTRIB_PG_EXCHANGE_STREAM_H_

#include "postgres.h"

#include "dmq.h"

#include "utils/tuplestore.h"

#define STREAM_NAME_MAX_LEN	(56)

typedef struct SendBuf
{
	uint32 index;
	uint32 tot_len;
	char data[FLEXIBLE_ARRAY_MEMBER];
} SendBuf;

#define MinSizeOfSendBuf offsetof(SendBuf, data)

typedef struct
{
	char name[STREAM_NAME_MAX_LEN];
	uint64 index;
} Stream;

typedef struct
{
	Stream stream;
	SendBuf *buf;
	DmqDestinationId dest_id;
} OStream;

typedef struct
{
	Stream stream;
	List *msgs;
} IStream;

List *istreams;
List *ostreams;


extern bool Stream_subscribe(const char *streamName);
extern bool Stream_unsubscribe(const char *streamName);

extern void SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot);
extern TupleTableSlot *RecvTuple(TupleDesc tupdesc, char *streamName, int *status);
#endif /* CONTRIB_PG_EXCHANGE_STREAM_H_ */
