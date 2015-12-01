#ifndef __BYTEBUF_H__
#define __BYTEBUF_H__

typedef struct
{
    char* data;
    int size;
    int used;
} ByteBuffer;

extern void ByteBufferAlloc(ByteBuffer* buf);
extern void ByteBufferAppend(ByteBuffer* buf, void* data, int len);
extern void ByteBufferAppendInt32(ByteBuffer* buf, int data);
extern void ByteBufferFree(ByteBuffer* buf);
extern void ByteBufferReset(ByteBuffer* buf);

#endif
