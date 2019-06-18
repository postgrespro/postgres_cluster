/*
 * stream.c
 *
 */

#include "postgres.h"

#include "miscadmin.h"
#include "nodes/pg_list.h"
#include "unistd.h"
#include "utils/memutils.h" /* MemoryContexts */

#include "common.h"
#include "sbuf.h"
#include "stream.h"

#define IsDeliveryMessage(msg)	(SDP_Size((StreamDataPackage *) msg) == SDPHeaderSize)

static List *istreams = NIL;
static List *ostreams = NIL;

static DmqDestinationId dmq_dest_id(DmqSenderId id);
static char *get_stream(List *streams, const char *name);
static void RecvIfAny(void);
static bool checkDelivery(OStream *ostream, SendBuf *sendbuf);
static void StreamRepeatSend(OStream *ostream, SendBuf *sendbuf);
static void wait_for_delivery(OStream *ostream, SendBuf *sendbuf);

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

		/* streamName is a first field of IStream and OStream structures. */
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

	OldMemoryContext = MemoryContextSwitchTo(memory_context);

	/* It is unique stream name */
	istream = (IStream *) palloc(sizeof(IStream));
	ostream = (OStream *) palloc(sizeof(OStream));
	strncpy(istream->streamName, streamName, STREAM_NAME_MAX_LEN);
	strncpy(ostream->streamName, streamName, STREAM_NAME_MAX_LEN);

	for (id = 0; id < DMQ_MAX_RECEIVERS; id++)
		istream->indexes[id] = 0;
	ostream->index = 0;
	ostream->bufs = NIL;
	istream->msgs = NIL;
	istream->deliveries = NIL;
	istream->RecvStore = tuplestore_begin_heap(false, false,
														MEM_STORAGE_MAX_SIZE);
	tuplestore_set_eflags(istream->RecvStore, 0);

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
	list_free(istream->deliveries);
	list_free(istream->msgs);
	tuplestore_end(istream->RecvStore);
	pfree(istream);
	list_free(ostream->bufs);
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
	StreamDataPackage *msg;
	Size len;
	DmqSenderId sender_id;
	IStream *istream;
	MemoryContext OldMemoryContext;

	/* Try to receive a message */
	for (;;)
	{
		streamName = dmq_pop(&sender_id, (const void **) &msg, &len,
															UINT64_MAX, false);

		if (!streamName)
			/* No messages */
			return;

		/* Message has been received */
		Assert(len >= SDPHeaderSize);
		istream = (IStream *) get_stream(istreams, streamName);

		if (istream == NULL)
		{
			/* We can't lose any data except resended byte messages. */
			Assert(msg->datalen <= 1);
			return;
		}

		OldMemoryContext = MemoryContextSwitchTo(memory_context);

		if ((msg->index > istream->indexes[sender_id]) || IsDeliveryMessage(msg))
		{
			if (IsDeliveryMessage(msg))
			{
				RecvBuf *buf = palloc(sizeof(RecvBuf));

				buf->sid = sender_id;
				buf->ptr = palloc(SDP_Size(msg));
				memcpy(buf->ptr, msg, SDP_Size(msg));
				istream->deliveries = lappend(istream->deliveries, buf);
			}
			else if (msg->datalen == 1)
			/* System one-byte message */
			{
				int size = SDP_Size(msg);
				RecvBuf *buf = palloc(sizeof(RecvBuf));

				buf->sid = sender_id;
				buf->ptr = palloc(size);
				memcpy(buf->ptr, (char *) msg, size);
				istream->msgs = lappend(istream->msgs, buf);
				istream->indexes[sender_id] = buf->ptr->index;
			}
			else
			/* A package of tuples has been received */
			{
				if (tuplestore_ateof(istream->RecvStore))
					tuplestore_clear(istream->RecvStore);
				else
					tuplestore_trim(istream->RecvStore);

				SDP_PrepareToRead(msg);
				while (!SDP_IsEmpty(msg))
					tuplestore_puttuple(istream->RecvStore, SDP_Get_tuple(msg));
				istream->indexes[sender_id] = msg->index;
			}
		}
		else
			elog(WARNING, "STEALED MSG: %u (from %lu) %d stream: %s, sender=%d",
					msg->index, istream->indexes[sender_id], msg->datalen,
					streamName, sender_id);

		/* Send delivery message, if needed. */
		if (!IsDeliveryMessage(msg) && msg->needConfirm)
		{
			StreamDataPackage dbuf;
			DmqDestinationId dest_id;

			/* If message is not delivery message, send delivery. */
			dbuf.datalen = 0;
			dbuf.index = msg->index;
			dest_id = dmq_dest_id(sender_id);
			Assert(dest_id >= 0);

			dmq_push_buffer(dest_id, istream->streamName, &dbuf,
													SDPHeaderSize, false);
		}
		MemoryContextSwitchTo(OldMemoryContext);
		return;
	}
}

static bool
checkDelivery(OStream *ostream, SendBuf *sendbuf)
{
	IStream *istream;
	ListCell *lc;
	bool found = false;
	List *temp = NIL;
	MemoryContext OldMemoryContext;

	RecvIfAny();

	istream = (IStream *) get_stream(istreams, ostream->streamName);
	Assert(istream);
	OldMemoryContext = MemoryContextSwitchTo(memory_context);
	/* Search for the delivery message and all duplicates. */
	foreach(lc, istream->deliveries)
	{
		RecvBuf *buf = lfirst(lc);

		if (buf->ptr->index == sendbuf->buf->index)
		{
			temp = lappend(temp, buf);
			found = true;
		}
	}

	/* Delete all duplicates of the delivery message. */
	foreach(lc, temp)
	{
		RecvBuf *buf = lfirst(lc);

		istream->deliveries = list_delete_ptr(istream->deliveries, buf);
		pfree(buf->ptr);
		pfree(buf);
	}
	list_free(temp);
	MemoryContextSwitchTo(OldMemoryContext);

	return found;
}

static void
StreamRepeatSend(OStream *ostream, SendBuf *sendbuf)
{
	while (!dmq_push_buffer(sendbuf->dest_id, ostream->streamName, sendbuf->buf,
												SDP_Size(sendbuf->buf), true))
			RecvIfAny();
}

static SendBuf *
getOutBuffer(OStream *ostream, DmqDestinationId dest_id)
{
	ListCell *lc;
	SendBuf *buf;
	MemoryContext OldMemoryContext;

	foreach(lc, ostream->bufs)
	{
		buf = lfirst(lc);
		if (buf->dest_id == dest_id)
			return buf;
	}

	OldMemoryContext = MemoryContextSwitchTo(memory_context);

	buf = palloc(sizeof(SendBuf));
	buf->dest_id = dest_id;
	buf->buf = NULL;
	ostream->bufs = lappend(ostream->bufs, buf);

	MemoryContextSwitchTo(OldMemoryContext);
	return buf;
}

void
SendTuple(DmqDestinationId dest_id, char *stream, TupleTableSlot *slot)
{
	OStream	*ostream;
	bool needFlushBuf;
	SendBuf *buf;

	ostream = (OStream *) get_stream(ostreams, stream);
	Assert(ostream);

	Assert(!TupIsNull(slot));

	if (!TTS_HAS_PHYSICAL_TUPLE(slot))
//	if (slot->tts_tuple == NULL)
		ExecMaterializeSlot(slot);

	buf = getOutBuffer(ostream, dest_id);
	if (!IsSDPBuf(buf->buf))
	{
		/* Previously it has been sent a system message */
		buf->buf = SDP_Alloc(TupSize(slot->tts_tuple) * 5);
		buf->buf->needConfirm = false;
		buf->dest_id = dest_id;
	}

	/* Try to store into the buffer */
	needFlushBuf = SDP_Store(buf->buf, slot->tts_tuple);
	if (!needFlushBuf)
	{
		return;
	}

	/* If buffer is full - send it by DMQ */
	buf->buf->index = ++(ostream->index);
	while (!dmq_push_buffer(dest_id, stream, buf->buf,
											SDP_Actual_size(buf->buf), true))
		RecvIfAny();

	SDP_Free(buf->buf);

	/* Store into the newly allocated buffer */
	buf->buf = SDP_Alloc(TupSize(slot->tts_tuple) * 5);
	needFlushBuf = SDP_Store(buf->buf, slot->tts_tuple);
	Assert(!needFlushBuf);
}

static void
wait_for_delivery(OStream *ostream, SendBuf *sendbuf)
{
	for (;;)
	{
		int waits;

		pg_usleep(10);

		for (waits = 0; waits < 100000; waits++)
		{
			if (checkDelivery(ostream, sendbuf))
				return;
		}
//		elog(LOG, "BEFORE StreamRepeatSend");
		StreamRepeatSend(ostream, sendbuf);
	}

	/* TODO: Insert FATAL report */
	Assert(0);
}

char BytePackage[SDPHeaderSize + 1];

void
SendByteMessage(DmqDestinationId dest_id, char *stream, char tag,
				bool needConfirm)
{
	OStream	*ostream;
	StreamDataPackage *buf = (StreamDataPackage *) BytePackage;
	SendBuf *sendbuf;

	ostream = (OStream *) get_stream(ostreams, stream);
	Assert(ostream);

	sendbuf = getOutBuffer(ostream, dest_id);
	if (IsSDPBuf(sendbuf->buf))
	{
		if (!SDP_IsEmpty(sendbuf->buf))
		{
			sendbuf->buf->index = ++(ostream->index);

			/* Flush tuple buffer before sending byte message */
			while (!dmq_push_buffer(dest_id, stream, sendbuf->buf,
										SDP_Actual_size(sendbuf->buf), true))
				RecvIfAny();
		}
		SDP_Free(sendbuf->buf);
	}

	buf->datalen = 1;
	buf->data[0] = tag;
	buf->index = ++(ostream->index);
	buf->needConfirm = needConfirm;

	sendbuf->buf = buf;
	sendbuf->dest_id = dest_id;

elog(LOG, "-> [%s] SendByteMessage: dest_id=%d, tag=%c sz: %lu index=%u, stream=%s",
		stream, dest_id, tag, SDP_Size(buf), buf->index, stream);
	while (!dmq_push_buffer(dest_id, stream, buf, SDP_Size(buf), true))
		RecvIfAny();
//	elog(LOG, "-> SendByteMessage: 1");
	if (buf->needConfirm)
		wait_for_delivery(ostream, sendbuf);

	elog(LOG, "-> SendByteMessage: CONFIRMED");
}

char
RecvByteMessage(const char *streamName, const char *sender)
{
	IStream *istream;
	ListCell *lc;
	MemoryContext OldMemoryContext;

	OldMemoryContext = MemoryContextSwitchTo(memory_context);

	RecvIfAny();

	istream = (IStream *) get_stream(istreams, streamName);
	Assert(istream);

	foreach(lc, istream->msgs)
	{
		RecvBuf *buf = lfirst(lc);
		const char *senderName = dmq_sender_name(buf->sid);

		if (buf->ptr->datalen == 1 && strcmp(senderName, sender) == 0)
		{
			char tag = buf->ptr->data[0];

			istream->msgs = list_delete_ptr(istream->msgs, buf);
			Assert(!IsSDPBuf(buf->ptr));
			pfree(buf);
			MemoryContextSwitchTo(OldMemoryContext);
			return tag;
		}
	}
	MemoryContextSwitchTo(OldMemoryContext);
	return 0;
}

int ntuples = 0;
/*
 * Receive tuple from any remote instance.
 * Returns NULL, if end-of-transfer received from a instance.
 */
int
RecvTuple(char *streamName, TupleTableSlot *slot)
{
	IStream *istream;
	ListCell *lc;
	List *temp = NIL;
	int status = -1; /* No tuples from network */
	bool found;

	istream = (IStream *) get_stream(istreams, streamName);
	Assert(istream);

	RecvIfAny();

	found = tuplestore_gettupleslot(istream->RecvStore, true, true, slot);

	if (found)
		return 0;

	for (lc = list_head(istream->msgs); lc != NULL; lc = lnext(lc))
	{
		RecvBuf *buf = lfirst(lc);

		Assert(buf->ptr->datalen == 1);

		temp = lappend(temp, buf);
		switch (buf->ptr->data[0])
		{
		case END_OF_TUPLES:
			/* No tuples from network */
			elog(LOG, "--- END_OF_TUPLES ---, [%s] sid=%d, index=%u, store: %ld EOF: %hhu",
					streamName, buf->sid, buf->ptr->index,
					tuplestore_tuple_count(istream->RecvStore),
					tuplestore_ateof(istream->RecvStore));
			status = 1;
			break;
		default:
			Assert(0);
		}
		break;
	}

	foreach(lc, temp)
	{
		RecvBuf *buf = lfirst(lc);

		istream->msgs = list_delete_ptr(istream->msgs, buf);
		pfree(buf);
	}
	list_free(temp);

	return status;
}
