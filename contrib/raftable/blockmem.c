#include "blockmem.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define BLOCK_LEN (256)
#define BLOCK_DATA_LEN ((BLOCK_LEN) - offsetof(block_t, data))
#define META(ORIGIN) ((meta_t *)(ORIGIN))
#define BLOCK(ORIGIN, ID) ((block_t *)((char *)(ORIGIN) + BLOCK_LEN * ID))
#define TAIL(ORIGIN, ID) (BLOCK(ORIGIN, ID)->next)
#define USED(ORIGIN, ID) (BLOCK(ORIGIN, ID)->used)
#define LEN(ORIGIN, ID) (BLOCK(ORIGIN, ID)->len)
#define DATA(ORIGIN, ID) (BLOCK(ORIGIN, ID)->data)
#define FREE(ORIGIN) (META(ORIGIN)->freeblock)

typedef struct meta_t
{
	int freeblock; /* the id of first free block, or -1 if none */
	char data[1];
} meta_t;

typedef struct block_t
{
	bool used;
	int next;
	int len;
	char data[1];
} block_t;

int
blockmem_format(void *origin, size_t size)
{
	int id;
	int blocks = (size - 1) / BLOCK_LEN;
	if (blocks <= 0) return 0;

	FREE(origin) = 1;

	for (id = 1; id <= blocks; id++)
	{
		USED(origin, id) = 0;
		TAIL(origin, id) = id + 1;
	}
	TAIL(origin, blocks) = 0; /* the last block has no tail */

	return 1;
}

static size_t
block_fill(void *origin, int id, void *src, size_t len)
{
	if (len > BLOCK_DATA_LEN)
		len = BLOCK_DATA_LEN;
	LEN(origin, id) = len;
	memcpy(DATA(origin, id), src, len);
	return len;
}

static void
block_clear(void *origin, int id)
{
	TAIL(origin, id) = 0;
	USED(origin, id) = true;
	LEN(origin, id) = 0;
}

static int
block_alloc(void *origin)
{
	int newborn = FREE(origin);
	if (!newborn) return 0;

	FREE(origin) = TAIL(origin, newborn);
	block_clear(origin, newborn);
	return newborn;
}

static void
block_free(void *origin, int id)
{
	/* behead the victim, and repeat for the tail */
	while (id > 0)
	{
		int tail;
		assert(USED(origin, id));

		USED(origin, id) = false;
		tail = TAIL(origin, id);
		TAIL(origin, id) = FREE(origin);
		FREE(origin) = id;
		id = tail;
	}
}

int
blockmem_put(void *origin, void *src, size_t len)
{
	int head = 0;
	int id = 0;
	char *cursor = src;
	size_t bytesleft = len;

	while (bytesleft > 0)
	{
		int copied;
		int newid = block_alloc(origin);
		if (!newid) goto failure;

		copied = block_fill(origin, newid, cursor, bytesleft);

		cursor += copied;
		bytesleft -= copied;
		if (id)
			TAIL(origin, id) = newid;
		else
			head = newid;
		id = newid;
	}

	return head;
failure:
	block_free(origin, head);
	return -1;
}

size_t
blockmem_len(void *origin, int id)
{
	size_t len = 0;
	while (id > 0)
	{
		assert(USED(origin, id));
		len += LEN(origin, id);
		id = TAIL(origin, id);
	}
	assert(len > 0);
	return len;
}

size_t
blockmem_get(void *origin, int id, void *dst, size_t len)
{
	size_t copied = 0;
	char *cursor = dst;
	while ((id > 0) && (copied < len))
	{
		size_t tocopy = LEN(origin, id);
		if (tocopy > len - copied)
			tocopy = len - copied;
		assert(tocopy > 0);
		assert(USED(origin, id));
		memcpy(cursor, DATA(origin, id), tocopy);
		copied += tocopy;
		cursor += tocopy;
		id = TAIL(origin, id);
	}
	return len;
}

void
blockmem_forget(void *origin, int id)
{
	block_free(origin, id);
}
