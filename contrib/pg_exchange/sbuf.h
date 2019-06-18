/*
 * sbuf.h
 *
 */

#ifndef SBUF_H_
#define SBUF_H_

#include "postgres.h"

#include "access/htup.h"
#include "utils/tuplestore.h"

/*
 * This is a structure which contains data that will be passed across network
 */
typedef struct
{
	uint32 index;
	uint32 datalen;
	bool needConfirm;
	char data[FLEXIBLE_ARRAY_MEMBER];
} StreamDataPackage;

#define SDPHeaderSize offsetof(StreamDataPackage, data)
#define SDP_Size(buf) (SDPHeaderSize + ((StreamDataPackage *)buf)->datalen)

#define TupSize(tup) ((int)(HEAPTUPLESIZE + tup->t_len))

#define DEFAULT_PACKAGE_SIZE	(BLCKSZ*4)

extern bool SDP_IsEmpty(const StreamDataPackage *buffer);
extern int SDP_Actual_size(const StreamDataPackage *buffer);
extern bool IsSDPBuf(const StreamDataPackage *buffer);
extern StreamDataPackage *SDP_Alloc(int size);
extern void SDP_Free(StreamDataPackage *buffer);
extern bool SDP_Store(StreamDataPackage *buffer, HeapTuple tuple);
extern void SDP_PrepareToRead(StreamDataPackage *buffer);
extern HeapTuple SDP_Get_tuple(StreamDataPackage *buffer);

#endif /* SBUF_H_ */
