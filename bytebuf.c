#include "postgres.h"
#include "bytebuf.h"

#define INIT_BUF_SIZE 1024

void ByteBufferAlloc(ByteBuffer* buf)
{
    buf->size = INIT_BUF_SIZE;
    buf->data = palloc(buf->size);
    buf->used = 0;
}

void ByteBufferAppend(ByteBuffer* buf, void* data, int len)
{
    if (buf->used + len > buf->size) { 
        buf->size = buf->used + len > buf->size*2 ? buf->used + len : buf->size*2;
        buf->data = (char*)repalloc(buf->data, buf->size);
    }
    memcpy(&buf->data[buf->used], data, len);
    buf->used += len;
}

void ByteBufferAppendInt32(ByteBuffer* buf, int data)
{
    ByteBufferAppend(buf, &data, sizeof data);
}

void ByteBufferFree(ByteBuffer* buf)
{
    pfree(buf->data);
}

void ByteBufferReset(ByteBuffer* buf)
{
    buf->used = 0;
}
