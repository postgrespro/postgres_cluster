/*
 * stream.c
 *
 */

#include "stream.h"
#include "miscadmin.h"
#include "unistd.h"
#include "utils/memutils.h" /* MemoryContexts */

#define IsDeliveryMessage(msg)	(msg->tot_len == MinSizeOfSendBuf)

static List *istreams = NIL;
static List *ostreams = NIL;

static DmqDestinationId dmq_dest_id(DmqSenderId id);
static char *get_stream(List *streams, const char *name);
static void RecvIfAny(void);
static bool checkDelivery(OStream *ostream);
static void StreamRepeatSend(OStream *ostream);
static OStream * ISendTuple(DmqDestinationId dest_id, char *stream,
		TupleTableSlot *slot, bool needConfirm);
static void wait_for_delivery(OStream *ostream);

static DmqDestinationId dmq_dest_id(DmqSenderId id)
{
	const char *name;

	name = dmq_sender_name(id);
	Assert(name != NULL);
	return dmq_remote_id(name);
}

static char *
get_stream(List *streams, const char *name)
{
	ListCell *lc;

	Assert(name);
	Assert(name[0] != '\0');
	for (lc = list_head(streams); lc != NULL; lc = lnext(lc))
	{
		char *streamName = (char *) lfirst(lc);
		if (strcmp(name, streamName) == 0)
			return streamName;
	}
	return NULL;
}

bool
Stream_subscribe(const char *streamName)
{
	IStream *istream;
	OStream *ostream;
	DmqSenderId	id;
	MemoryContext OldMemoryContext;

	/* Check for existed stream */
	istream = (IStream *) get_stream(istreams, streamName);
	ostream = (OStream *) get_stream(ostreams, streamName);
	if (istream || ostream)
		return false;

	OldMemoryContext = MemoryContextSwitchTo(TopMemoryContext);

	/* It is unique stream name */
	istream = (IStream *) palloc(sizeof(IStream));
	ostream = (OStream *) palloc(sizeof(OStream));
	strncpy(istream->streamName, streamName, STREAM_NAME_MAX_LEN);
	strncpy(ostream->streamName, streamName, STREAM_NAME_MAX_LEN);

	for (id = 0; id < DMQ_MAX_RECEIVERS; id++)
		istream->indexes[id] = 0;
	ostream->index = 0;
	istream->msgs = NIL;
	ostream->buf = NULL;

	istreams = lappend(istreams, istream);
	ostreams = lappend(ostreams, ostream);
	dmq_stream_subscribe(streamName);
	MemoryContextSwitchTo(OldMemoryContext);
	return true;
}

bool
Stream_unsubscribe(const char *streamName)
{
	IStream *istream;
	OStream *ostream;

	istream = (IStream *) get_stream(istreams, streamName);
	ostream = (OStream *) get_stream(ostreams, streamName);
	if (!istream || !ostream)
		return false;

	istreams = list_delete_ptr(istreams, istream);
	ostreams = list_delete_ptr(ostreams, ostream);
	dmq_stream_unsubscribe(streamName);
	pfree(istream);
	pfree(ostream);
	return true;
}

/*
 * Receive any message for any stream.
 */
static void
RecvIfAny(void)
{
	const char *streamName;
	SendBuf *msg;
	Size len;
	DmqSenderId sender_id;
	IStream *istream;

	/* Try to receive a message */
	streamName = dmq_pop(&sender_id, (void **)(&msg), &len, UINT64_MAX, false);
	if (!streamName)
		/* No messages arrived */
		return;

	/* Any message was received */
	Assert(len >= MinSizeOfSendBuf);
	istream = (IStream *) get_stream(istreams, streamName);

	if ((msg->index > istream->indexes[sender_id]) || IsDeliveryMessage(msg))
	{
		SendBuf *buf;

		Assert(istream != NULL);
		buf = palloc(len);
		memcpy(buf, msg, len);
		istream->msgs = lappend(istream->msgs, buf);
		if (!IsDeliveryMessage(msg))
			istream->indexes[sender_id] = buf->index;
	}

	/* Send delivery message, if needed. */
	if (!IsDeliveryMessage(msg) && msg->needConfirm)
	{
		SendBuf *dbuf;
		DmqDestinationId dest_id;

		/* If message is not delivery message, send delivery. */
		dbuf = palloc(MinSizeOfSendBuf);
		dbuf->tot_len = MinSizeOfSendBuf;
		dbuf->index = msg->index;
		dest_id = dmq_dest_id(sender_id);
		Assert(dest_id >= 0);
		dmq_push_buffer(dest_id, istream->streamName, dbuf, dbuf->tot_len);
		pfree(dbuf);
	}
}

static bool
checkDelivery(OStream *ostream)
{
	IStream *istream;
	ListCell *lc;

	RecvIfAny();
	istream = (IStream *) get_stream(istreams, ostream->streamName);
	Assert(istream);

	foreach(lc, istream->msgs)
	{
		SendBuf *buf = lfirst(lc);
		if ((buf->index == ostream->index) &&
			(buf->tot_len == MinSizeOfSendBuf))
		{
			istream->msgs = list_delete_ptr(istream->msgs, buf);
			return true;
		}
	}
	return false;
}
static void
StreamRepeatSend(OStream *ostream)
{
	dmq_push_buffer(ostream->dest_id, ostream->streamName, ostream->buf,
					ostream->buf->tot_len);
}

static OStream *
ISendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot,
		bool needConfirm)
{
	HeapTuple tuple;
	int tupsize;
	SendBuf *buf;
	OStream	*ostream;

	ostream = (OStream *) get_stream(ostreams, stream);
	Assert(ostream && !ostream->buf);

	if (!TupIsNull(slot))
	{
		int tot_len;

		if (slot->tts_tuple == NULL)
			ExecMaterializeSlot(slot);

		tuple = slot->tts_tuple;
		tupsize = offsetof(HeapTupleData, t_data);

		tot_len = MinSizeOfSendBuf + tupsize + tuple->t_len;
		buf = palloc(tot_len);
		buf->tot_len = tot_len;
		memcpy(buf->data, tuple, tupsize);
		memcpy(buf->data + tupsize, tuple->t_data, tuple->t_len);
	}
	else
		Assert(0);

	buf->index = ++(ostream->index);
	buf->needConfirm = needConfirm;
	ostream->dest_id = dest_id;
	dmq_push_buffer(dest_id, stream, buf, buf->tot_len);
	if (buf->needConfirm)
		ostream->buf = buf;
	else
		ostream->buf = NULL;

	return ostream;
}

static void
wait_for_delivery(OStream *ostream)
{
	for (;;)
	{
		int waits;

		pg_usleep(10);

		for (waits = 0; waits < 100000; waits++)
		{
			if (checkDelivery(ostream))
			{
				pfree(ostream->buf);
				ostream->buf = NULL;
				return;
			}
		}
		StreamRepeatSend(ostream);
	}

	/* TODO: Insert FATAL report */
	Assert(0);
}

void
SendByteMessage(DmqDestinationId dest_id, char *stream, char msg)
{
	OStream	*ostream;
	SendBuf *buf;

	ostream = (OStream *) get_stream(ostreams, stream);
	Assert(ostream && !ostream->buf);

	buf = palloc(MinSizeOfSendBuf + 1);
	buf->tot_len = MinSizeOfSendBuf + 1;
	buf->data[0] = msg;

	buf->index = ++(ostream->index);
	buf->needConfirm = true;
	ostream->buf = buf;
	ostream->dest_id = dest_id;
	dmq_push_buffer(dest_id, stream, buf, buf->tot_len);
	wait_for_delivery(ostream);
}

/*
 * Send tuple to instance, identified by dest_id. Stream is some abstraction
 * of named channel.
 */
void
SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot,
		bool needConfirm)
{
	OStream *ostream;

	ostream = ISendTuple(dest_id, stream, slot, needConfirm);
	if (needConfirm)
		wait_for_delivery(ostream);
}

/*
 * Receive tuple or message from any remote instance.
 * Returns NULL, if end-of-transfer received from a instance.
 */
TupleTableSlot *
RecvTuple(TupleDesc tupdesc, char *streamName, int *status)
{
	IStream *istream;
	ListCell *lc;
	TupleTableSlot *slot;

	RecvIfAny();

	istream = (IStream *) get_stream(istreams, streamName);
	Assert(istream);

	foreach(lc, istream->msgs)
	{
		SendBuf *buf = lfirst(lc);
		if (buf->tot_len > MinSizeOfSendBuf)
		{
			HeapTuple tup;
			int tupsize = offsetof(HeapTupleData, t_data);

			istream->msgs = list_delete_ptr(istream->msgs, buf);
			if (buf->tot_len == MinSizeOfSendBuf + 1) /* System message */
			{
				switch (buf->data[0])
				{
				case END_OF_TUPLES:
					/* No tuples from network */
					*status = 1;
					break;
				case 'Q':
					*status = 2;
					break;
				default:
					*status = 3;
					break;
				}
				pfree(buf);
				return (TupleTableSlot *) NULL;
			}

			*status = 0;
			tup = (HeapTuple) buf->data;
			tup->t_data = (HeapTupleHeader) (buf->data + tupsize);
			slot = MakeSingleTupleTableSlot(tupdesc);
			return ExecStoreTuple((HeapTuple) buf->data, slot, InvalidBuffer, true);
		}
	}

	*status = -1; /* No tuples from network */
	return (TupleTableSlot *) NULL;
}
