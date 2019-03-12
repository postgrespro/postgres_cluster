/*
 * stream.c
 *
 */

#include "stream.h"

#include "miscadmin.h"
#include "unistd.h"

static DmqDestinationId dmq_dest_id(DmqSenderId id)
{
	const char *name;

	name = dmq_sender_name(id);
	Assert(name != NULL);
	return dmq_remote_id(name);
}

static Stream *
get_stream(List *streams, const char *name)
{
	ListCell *lc;

	Assert(name);
	Assert(name[0] != '\0');
	for (lc = list_head(streams); lc != NULL; lc = lnext(lc))
	{
		Stream *stream = (Stream *) lfirst(lc);
		if (strcmp(name, stream->name) == 0)
			return stream;
	}
	return NULL;
}

bool
Stream_subscribe(const char *streamName)
{
	IStream *istream;
	OStream *ostream;

elog(LOG, "Subscribe on %s.", streamName);
	/* Check for existed stream */
	istream = (IStream *) get_stream(istreams, streamName);
	ostream = (OStream *) get_stream(ostreams, streamName);
	if (istream || ostream)
		return false;

	/* It is unique stream name */
	istream = palloc(sizeof(IStream));
	ostream = palloc(sizeof(OStream));
	strncpy(istream->stream.name, streamName, STREAM_NAME_MAX_LEN);
	strncpy(ostream->stream.name, streamName, STREAM_NAME_MAX_LEN);
	istream->stream.index = 0;
	istream->msgs = NIL;
	ostream->stream.index = 0;
	ostream->buf = NULL;
	istreams = lappend(istreams, istream);
	ostreams = lappend(ostreams, ostream);
	dmq_stream_subscribe(streamName);
	return true;
}

bool
Stream_unsubscribe(const char *streamName)
{
	IStream *istream;
	OStream *ostream;

//	elog(LOG, "unSubscribe on %s.", streamName);
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
	const char *streamName = NULL;
	SendBuf *msg;
	Size len;
	DmqSenderId sender_id;
	DmqDestinationId dest_id;
	IStream *istream;

	/* Try to receive a message */
	streamName = dmq_pop(&sender_id, (void **)(&msg), &len, UINT64_MAX, false);
	if (!streamName)
		/* No messages was arrived */
		return;

	/* Any message was received */
	Assert(len >= MinSizeOfSendBuf);
	istream = (IStream *) get_stream(istreams, streamName);

	if ((msg->index > istream->stream.index) || (msg->tot_len == MinSizeOfSendBuf))
	{
		SendBuf *buf;
//		elog(LOG, "RecvIfAny, pid=%d streamName=%s, len=%lu, msg->len=%u", getpid(), streamName, len, msg->tot_len);
		Assert(istream != NULL);
		buf = palloc(len);
		memcpy(buf, msg, len);
		istream->msgs = lappend(istream->msgs, buf);
		if (msg->tot_len > MinSizeOfSendBuf)
			istream->stream.index = buf->index;
	}

	if (msg->tot_len > MinSizeOfSendBuf)
	{
		SendBuf *dbuf;

		/* If message is not delivery message, send delivery. */
		dbuf = palloc(MinSizeOfSendBuf);
		dbuf->tot_len = MinSizeOfSendBuf;
		dbuf->index = msg->index;
		dest_id = dmq_dest_id(sender_id);
		Assert(dest_id >= 0);
//		elog(LOG, "[%d] Send delivery: dest_id=%d (%d, %u %lu)", getpid(),
//				dest_id, dbuf->index, dbuf->tot_len, MinSizeOfSendBuf);
		dmq_push_buffer(dest_id, istream->stream.name, dbuf, dbuf->tot_len);
	}
}

static bool
checkDelivery(OStream *ostream)
{
	IStream *istream;
	ListCell *lc;

	RecvIfAny();
	istream = (IStream *) get_stream(istreams, ostream->stream.name);
	Assert(istream);

	foreach(lc, istream->msgs)
	{
		SendBuf *buf = lfirst(lc);
		if ((buf->index == ostream->stream.index) &&
			(buf->tot_len == MinSizeOfSendBuf))
		{
//			elog(LOG, "[%d] got delivery:", getpid());
			istream->msgs = list_delete_ptr(istream->msgs, buf);
//			elog(LOG, "[%d] got delivery2:", getpid());
			return true;
		}
//		else
//		elog(LOG, "[%d] check delivery: %s, %d %d %u %lu", getpid(), ostream->stream.name,
//				list_length(istream->msgs),
//				buf->index,
//				buf->tot_len,
//				MinSizeOfSendBuf);
	}
	return false;
}
static void
StreamRepeatSend(OStream *ostream)
{
	dmq_push_buffer(ostream->dest_id, ostream->stream.name, ostream->buf,
					ostream->buf->tot_len);
}

static OStream *
ISendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot)
{
	HeapTuple tuple;
	int tupsize;
	SendBuf *buf;
	OStream	*ostream;

	ostream = (OStream *) get_stream(ostreams, stream);

	if (ostream->buf)
	{
		pfree(ostream->buf);
		ostream->buf = NULL;
	}

	if (!TupIsNull(slot))
	{
		int tot_len;

		if (slot->tts_tuple == NULL)
			ExecMaterializeSlot(slot);

		tuple = slot->tts_tuple;
		tupsize = offsetof(HeapTupleData, t_data);

		tot_len = MinSizeOfSendBuf + tupsize + tuple->t_len;
//		elog(LOG, "PUSH BUFFER to stream %s, size=%d", stream, tot_len);
		buf = palloc(tot_len);
		buf->tot_len = tot_len;
		memcpy(buf->data, tuple, tupsize);
		memcpy(buf->data + tupsize, tuple->t_data, tuple->t_len);
	}
	else
	{
		/*
		 * NULL slot is meaning the end of tuple transfer. We don't pass tuple,
		 * but special info with one byte size.
		 */
		buf = palloc(MinSizeOfSendBuf + 1);
		buf->tot_len = MinSizeOfSendBuf + 1;
		buf->data[0] = 'E';
	}

	buf->index = ++(ostream->stream.index);
	ostream->buf = buf;
	ostream->dest_id = dest_id;
	dmq_push_buffer(dest_id, stream, buf, buf->tot_len);
	return ostream;
}

/*
 * Send tuple to instance, identified by dest_id. Stream is some abstraction
 * of named channel.
 */
void
SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot)
{
	int attempts = 0;
	OStream *ostream;

	ostream = ISendTuple(dest_id, stream, slot);
while (1==1)
//	for (attempts = 0; attempts < 50; attempts++)
	{
		int waits;

		attempts++;
		for (waits = 0; waits < 100000; waits++)
		{
			if (checkDelivery(ostream))
			{
//				elog(LOG, "Message delivery is confirmed. Index=%lu", ostream->stream.index);
				pfree(ostream->buf);
				ostream->buf = NULL;
				//ostreams = list_delete_ptr(ostreams, ostream);
				return;
			}
		}
//		elog(LOG, "[%d] Repeat send: %d, buf:(ind: %u, len: %u)", getpid(), attempts, ostream->buf->index, ostream->buf->tot_len);
		StreamRepeatSend(ostream);
//		elog(LOG, "Attempt %d failed", attempts);
	}
	Assert(0);
}

/*
 * Receive tuple from any remote instance.
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

			*status = 0;
			if (buf->tot_len == MinSizeOfSendBuf + 1) /* System message */
			{
				Assert(buf->data[0] == 'E');
				return (TupleTableSlot *) NULL;
			}
			istream->msgs = list_delete_ptr(istream->msgs, buf);
			tup = (HeapTuple) buf->data;
			tup->t_data = (HeapTupleHeader) (buf->data + tupsize);
			slot = MakeSingleTupleTableSlot(tupdesc);
			return ExecStoreTuple((HeapTuple) buf->data, slot, InvalidBuffer, false);
		}
	}
//	elog(LOG, "Recv tuple failed. listl=%d", list_length(istream->msgs));
	*status = 1;
	return (TupleTableSlot *) NULL;
}
