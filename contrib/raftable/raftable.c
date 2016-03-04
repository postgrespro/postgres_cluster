/*
 * raftable.c
 *
 * A key-value table replicated over Raft.
 *
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "storage/lwlock.h"
#include "storage/ipc.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "miscadmin.h"

#include "raftable.h"

#define RAFTABLE_KEY_LEN (64)
#define RAFTABLE_BLOCK_LEN (256)
#define RAFTABLE_BLOCKS (4096)
#define RAFTABLE_BLOCK_MEM (RAFTABLE_BLOCK_LEN * (RAFTABLE_BLOCKS - 1) + sizeof(RaftableBlockMem))
#define RAFTABLE_HASH_SIZE (127)
#define RAFTABLE_SHMEM_SIZE ((1024 * 1024) + RAFTABLE_BLOCK_MEM)

typedef struct RaftableBlock
{
	struct RaftableBlock *next;
	char data[RAFTABLE_BLOCK_LEN - sizeof(void*)];
} RaftableBlock;

typedef struct RaftableKey
{
	char data[RAFTABLE_KEY_LEN];
} RaftableKey;

typedef struct RaftableEntry
{
	RaftableKey key;
	int len;
	RaftableBlock *value;
} RaftableEntry;

typedef struct RaftableBlockMem
{
	RaftableBlock *free_blocks;
	RaftableBlock blocks[1];
} RaftableBlockMem;

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(raftable_sql_get);
PG_FUNCTION_INFO_V1(raftable_sql_set);
PG_FUNCTION_INFO_V1(raftable_sql_list);

static HTAB *hashtable;
static LWLockId hashlock;

static RaftableBlockMem *blockmem;
static LWLockId blocklock;

static shmem_startup_hook_type PreviousShmemStartupHook;

static RaftableBlock *block_alloc(void)
{
	RaftableBlock *result;

	LWLockAcquire(blocklock, LW_EXCLUSIVE);

	result = blockmem->free_blocks;
	if (result == NULL)
		elog(ERROR, "raftable memory limit hit");


	blockmem->free_blocks = blockmem->free_blocks->next;
	result->next = NULL;
	LWLockRelease(blocklock);
	return result;
}

static void block_free(RaftableBlock *block)
{
	RaftableBlock *new_free_head = block;
	Assert(block != NULL);
	while (block->next != NULL) {
		block = block->next;
	}
	LWLockAcquire(blocklock, LW_EXCLUSIVE);
	block->next = blockmem->free_blocks;
	blockmem->free_blocks = new_free_head;
	LWLockRelease(blocklock);
}


static text *entry_to_text(RaftableEntry *e)
{
	char *cursor, *buf;
	RaftableBlock *block;
	text *t;
	int len;

	buf = palloc(e->len + 1);
	cursor = buf;

	block = e->value;
	len = e->len;
	while (block != NULL)
	{
		int tocopy = len;
		if (tocopy > sizeof(block->data))
			tocopy = sizeof(block->data);

		memcpy(cursor, block->data, tocopy);
		cursor += tocopy;
		len -= tocopy;

		Assert(cursor - buf <= e->len);
		block = block->next;
	}
	Assert(cursor - buf == e->len);
	*cursor = '\0';
	t = cstring_to_text_with_len(buf, e->len);
	pfree(buf);
	return t;
}

static void text_to_entry(RaftableEntry *e, text *t)
{
	char *buf, *cursor;
	int len;
	RaftableBlock *block;

	buf = text_to_cstring(t);
	cursor = buf;
	len = strlen(buf);
	e->len = len;

	if (e->len > 0)
	{
		if (e->value == NULL)
			e->value = block_alloc();
		Assert(e->value != NULL);
	}
	else
	{
		if (e->value != NULL)
		{
			block_free(e->value);
			e->value = NULL;
		}
	}

	block = e->value;
	while (len > 0)
	{
		int tocopy = len;
		if (tocopy > sizeof(block->data))
		{
			tocopy = sizeof(block->data);
			if (block->next == NULL)
				block->next = block_alloc();
		}
		else
		{
			if (block->next != NULL)
			{
				block_free(block->next);
				block->next = NULL;
			}
		}

		memcpy(block->data, cursor, tocopy);
		cursor += tocopy;
		len -= tocopy;

		block = block->next;
	}

	pfree(buf);
	Assert(block == NULL);
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableEntry *entry;
	RaftableKey key;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	LWLockAcquire(hashlock, LW_SHARED);
	entry = hash_search(hashtable, &key, HASH_FIND, NULL);

	if (entry)
	{
		text *t = entry_to_text(entry);
		LWLockRelease(hashlock);
		PG_RETURN_TEXT_P(t);
	}
	else
	{
		LWLockRelease(hashlock);
		PG_RETURN_NULL();
	}
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	LWLockAcquire(hashlock, LW_EXCLUSIVE);
	if (PG_ARGISNULL(1))
	{
		RaftableEntry *entry = hash_search(hashtable, key.data, HASH_FIND, NULL);
		if ((entry != NULL) && (entry->len > 0))
			block_free(entry->value);
		hash_search(hashtable, key.data, HASH_REMOVE, NULL);
	}
	else
	{
		bool found;
		RaftableEntry *entry = hash_search(hashtable, key.data, HASH_ENTER, &found);
		if (!found)
		{
			entry->key = key;
			entry->value = NULL;
			entry->len = 0;
		}
		text_to_entry(entry, PG_GETARG_TEXT_P(1));
	}
	LWLockRelease(hashlock);

	PG_RETURN_VOID();
}

Datum
raftable_sql_list(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	HASH_SEQ_STATUS *scan;
	RaftableEntry *entry;

	if (SRF_IS_FIRSTCALL())
	{
		TypeFuncClass tfc;
		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tfc = get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc);
		if (tfc != TYPEFUNC_COMPOSITE)
		{
			elog(ERROR, "raftable listing function should be composite");
		}
		funcctx->tuple_desc = BlessTupleDesc(funcctx->tuple_desc);

		scan = (HASH_SEQ_STATUS *)palloc(sizeof(HASH_SEQ_STATUS));
		LWLockAcquire(hashlock, LW_SHARED);
		hash_seq_init(scan, hashtable);

		MemoryContextSwitchTo(oldcontext);

		funcctx->user_fctx = scan;
	}

	funcctx = SRF_PERCALL_SETUP();
	scan = funcctx->user_fctx;

	if ((entry = (RaftableEntry *)hash_seq_search(scan)))
	{
		HeapTuple tuple;
		Datum  vals[2];
		bool isnull[2];

		vals[0] = CStringGetTextDatum(entry->key.data);
		vals[1] = PointerGetDatum(entry_to_text(entry));
		isnull[0] = isnull[1] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc, vals, isnull);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		LWLockRelease(hashlock);
		SRF_RETURN_DONE(funcctx);
	}

}

static void startup_shmem(void)
{
	if (PreviousShmemStartupHook){
		PreviousShmemStartupHook();
	}

	{
		HASHCTL info;
		hashlock = LWLockAssign();

		info.keysize = sizeof(RaftableKey);
		info.entrysize = sizeof(RaftableEntry);

		hashtable = ShmemInitHash(
			"raftable_hashtable",
			RAFTABLE_HASH_SIZE, RAFTABLE_HASH_SIZE,
			&info, HASH_ELEM
		);
	}

	{
		bool found;
		int i;

		blocklock = LWLockAssign();

		blockmem = ShmemInitStruct(
			"raftable_blockmem",
			RAFTABLE_BLOCK_MEM,
			&found
		);

		for (i = 0; i < RAFTABLE_BLOCKS - 1; i++) {
			blockmem->blocks[i].next = blockmem->blocks + i + 1;
		}
		blockmem->blocks[RAFTABLE_BLOCKS - 1].next = NULL;
		blockmem->free_blocks = blockmem->blocks;
	}
}

void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
		elog(ERROR, "please add 'raftable' to shared_preload_libraries list");
	RequestAddinShmemSpace(RAFTABLE_SHMEM_SIZE);
	RequestAddinLWLocks(2);

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = startup_shmem;
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}
