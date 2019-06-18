/*
 * stream.h
 *
 */

#ifndef CONTRIB_PG_EXCHANGE_STREAM_H_
#define CONTRIB_PG_EXCHANGE_STREAM_H_

#include "postgres.h"
#include "executor/tuptable.h"
#include "nodes/pg_list.h"

#include "dmq.h"
#include "sbuf.h"

#define STREAM_NAME_MAX_LEN	(56)
#define MEM_STORAGE_MAX_SIZE	(8192)

/* System messages definition */
#define END_OF_TUPLES	'E'
#define END_OF_EXCHANGE 'Q'

typedef struct
{
	DmqSenderId sid; /* Sender ID */
	StreamDataPackage *ptr;
} RecvBuf;

typedef struct
{
	DmqDestinationId dest_id;
	StreamDataPackage *buf;
} SendBuf;

typedef struct
{
	char streamName[STREAM_NAME_MAX_LEN];
	uint32 index;
	List *bufs;
} OStream;

typedef struct
{
	char streamName[STREAM_NAME_MAX_LEN];
	uint64 indexes[DMQ_MAX_RECEIVERS];
	List *msgs;
	List *deliveries;
	Tuplestorestate *RecvStore;
} IStream;


extern bool Stream_subscribe(const char *streamName);
extern bool Stream_unsubscribe(const char *streamName);
extern void SendByteMessage(DmqDestinationId dest_id, char *stream, char tag,
							bool needConfirm);
extern char RecvByteMessage(const char *streamName, const char *sender);
extern void SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot);
extern int RecvTuple(char *streamName, TupleTableSlot *slot);
#endif /* CONTRIB_PG_EXCHANGE_STREAM_H_ */
