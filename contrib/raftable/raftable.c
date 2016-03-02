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

#define RAFTABLE_SHMEM_SIZE (16 * 1024)
#define RAFTABLE_HASH_SIZE (127)
#define RAFTABLE_VALUE_LEN 64

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(raftable_sql_get);
PG_FUNCTION_INFO_V1(raftable_sql_set);
PG_FUNCTION_INFO_V1(raftable_sql_list);

static HTAB *data;
static LWLockId datalock;
static shmem_startup_hook_type PreviousShmemStartupHook;

typedef struct RaftableEntry
{
	int key;
	char value[RAFTABLE_VALUE_LEN];
} RaftableEntry;

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableEntry *entry;
	int key = PG_GETARG_INT32(0);

	LWLockAcquire(datalock, LW_SHARED);
	entry = hash_search(data, &key, HASH_FIND, NULL);

	if (entry)
	{
		text *t = cstring_to_text(entry->value);
		LWLockRelease(datalock);
		PG_RETURN_TEXT_P(t);
	}
	else
	{
		LWLockRelease(datalock);
		PG_RETURN_NULL();
	}
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	int key = PG_GETARG_INT32(0);

	LWLockAcquire(datalock, LW_EXCLUSIVE);
	if (PG_ARGISNULL(1))
		hash_search(data, &key, HASH_REMOVE, NULL);
	else
	{
		RaftableEntry *entry = hash_search(data, &key, HASH_ENTER, NULL);
		entry->key = key;
		text_to_cstring_buffer(PG_GETARG_TEXT_P(1), entry->value, sizeof(entry->value));
	}
	LWLockRelease(datalock);

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
		Assert(tfc == TYPEFUNC_COMPOSITE);
		funcctx->tuple_desc = BlessTupleDesc(funcctx->tuple_desc);

		scan = (HASH_SEQ_STATUS *)palloc(sizeof(HASH_SEQ_STATUS));
		LWLockAcquire(datalock, LW_SHARED);
		hash_seq_init(scan, data);

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

		vals[0] = Int32GetDatum(entry->key);
		vals[1] = CStringGetTextDatum(entry->value);
		isnull[0] = isnull[1] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc, vals, isnull);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		LWLockRelease(datalock);
		SRF_RETURN_DONE(funcctx);
	}

}

static uint32 raftable_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(int*)key;
}

static int raftable_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(int*)key1 != *(int*)key2;
}

static void startup_shmem(void)
{
	HASHCTL info;

	if (PreviousShmemStartupHook){
		PreviousShmemStartupHook();
	}

	datalock = LWLockAssign();

	info.keysize = sizeof(int);
	info.entrysize = sizeof(RaftableEntry);
	info.hash = raftable_hash_fn;
	info.match = raftable_match_fn;

	data = ShmemInitHash(
		"raftable",
		RAFTABLE_HASH_SIZE, RAFTABLE_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);
}

void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
		elog(ERROR, "please add 'raftable' to shared_preload_libraries list");
	RequestAddinShmemSpace(RAFTABLE_SHMEM_SIZE);
	RequestAddinLWLocks(1);

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = startup_shmem;
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}
