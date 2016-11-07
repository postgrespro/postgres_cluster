/*-------------------------------------------------------------------------
 *
 * sha.h
 *	  Generic headers for SHA functions of PostgreSQL.
 *
 * Portions Copyright (c) 2016, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *        src/include/common/sha.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef _PG_SHA_H_
#define _PG_SHA_H_

#ifdef USE_SSL
#include <openssl/sha.h>
#endif

/*** SHA-1/224/256/384/512 Various Length Definitions ***********************/
#define PG_SHA1_BLOCK_LENGTH			64
#define PG_SHA1_DIGEST_LENGTH			20
#define PG_SHA1_DIGEST_STRING_LENGTH	(PG_SHA1_DIGEST_LENGTH * 2 + 1)
#define PG_SHA224_BLOCK_LENGTH			64
#define PG_SHA224_DIGEST_LENGTH			28
#define PG_SHA224_DIGEST_STRING_LENGTH	(PG_SHA224_DIGEST_LENGTH * 2 + 1)
#define PG_SHA256_BLOCK_LENGTH			64
#define PG_SHA256_DIGEST_LENGTH			32
#define PG_SHA256_DIGEST_STRING_LENGTH	(PG_SHA256_DIGEST_LENGTH * 2 + 1)
#define PG_SHA384_BLOCK_LENGTH			128
#define PG_SHA384_DIGEST_LENGTH			48
#define PG_SHA384_DIGEST_STRING_LENGTH	(PG_SHA384_DIGEST_LENGTH * 2 + 1)
#define PG_SHA512_BLOCK_LENGTH			128
#define PG_SHA512_DIGEST_LENGTH			64
#define PG_SHA512_DIGEST_STRING_LENGTH	(PG_SHA512_DIGEST_LENGTH * 2 + 1)

/* Context Structures for SHA-1/224/256/384/512 */
#ifdef USE_SSL
typedef SHA_CTX pg_sha1_ctx;
typedef SHA256_CTX pg_sha256_ctx;
typedef SHA512_CTX pg_sha512_ctx;
typedef SHA256_CTX pg_sha224_ctx;
typedef SHA512_CTX pg_sha384_ctx;
#else
typedef struct pg_sha1_ctx
{
	union
	{
		uint8		b8[20];
		uint32		b32[5];
	}			h;
	union
	{
		uint8		b8[8];
		uint64		b64[1];
	}			c;
	union
	{
		uint8		b8[64];
		uint32		b32[16];
	}			m;
	uint8		count;
} pg_sha1_ctx;
typedef struct pg_sha256_ctx
{
	uint32		state[8];
	uint64		bitcount;
	uint8		buffer[PG_SHA256_BLOCK_LENGTH];
} pg_sha256_ctx;
typedef struct pg_sha512_ctx
{
	uint64		state[8];
	uint64		bitcount[2];
	uint8		buffer[PG_SHA512_BLOCK_LENGTH];
} pg_sha512_ctx;
typedef struct pg_sha256_ctx pg_sha224_ctx;
typedef struct pg_sha512_ctx pg_sha384_ctx;
#endif	/* USE_SSL */

/* Interface routines for SHA-1/224/256/384/512 */
extern void pg_sha1_init(pg_sha1_ctx *ctx);
extern void pg_sha1_update(pg_sha1_ctx *ctx,
						const uint8 *input0, size_t len);
extern void pg_sha1_final(pg_sha1_ctx *ctx, uint8 *dest);

extern void pg_sha224_init(pg_sha224_ctx *ctx);
extern void	pg_sha224_update(pg_sha224_ctx *ctx,
						const uint8 *input0, size_t len);
extern void pg_sha224_final(pg_sha224_ctx *ctx, uint8 *dest);

extern void pg_sha256_init(pg_sha256_ctx *ctx);
extern void pg_sha256_update(pg_sha256_ctx *ctx,
						const uint8 *input0, size_t len);
extern void pg_sha256_final(pg_sha256_ctx *ctx, uint8 *dest);

extern void pg_sha384_init(pg_sha384_ctx *ctx);
extern void pg_sha384_update(pg_sha384_ctx *ctx,
						const uint8 *, size_t len);
extern void pg_sha384_final(pg_sha384_ctx *ctx, uint8 *dest);

extern void pg_sha512_init(pg_sha512_ctx *ctx);
extern void pg_sha512_update(pg_sha512_ctx *ctx,
						const uint8 *input0, size_t len);
extern void pg_sha512_final(pg_sha512_ctx *ctx, uint8 *dest);

#endif   /* _PG_SHA_H_ */
