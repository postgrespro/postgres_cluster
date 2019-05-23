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

/* System messages definition */
#define END_OF_TUPLES	'E'
#define END_OF_EXCHANGE 'Q'

typedef struct SendBuf
{
	uint32 index;
	uint32 tot_len;
	bool needConfirm;
	char data[FLEXIBLE_ARRAY_MEMBER];
} SendBuf;

typedef struct RecvBuf
{
	DmqSenderId sid; /* Sender ID */
	uint32 index;
	uint32 datalen;
	char data[FLEXIBLE_ARRAY_MEMBER];
} RecvBuf;

#define MinSizeOfSendBuf offsetof(SendBuf, data)
#define MinSizeOfRecvBuf offsetof(RecvBuf, data)

typedef struct
{
	char streamName[STREAM_NAME_MAX_LEN];
	uint64 index;
	SendBuf *buf;
	DmqDestinationId dest_id;
} OStream;

typedef struct
{
	char streamName[STREAM_NAME_MAX_LEN];
	uint64 indexes[DMQ_MAX_RECEIVERS];
	List *msgs;
	List *deliveries;
} IStream;


extern bool Stream_subscribe(const char *streamName);
extern bool Stream_unsubscribe(const char *streamName);

extern void SendByteMessage(DmqDestinationId dest_id, char *stream, char tag);
extern char RecvByteMessage(const char *streamName, const char *sender);
extern void SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot,
		bool needConfirm);
extern TupleTableSlot *RecvTuple(TupleDesc tupdesc, char *streamName, int *status);
#endif /* CONTRIB_PG_EXCHANGE_STREAM_H_ */
