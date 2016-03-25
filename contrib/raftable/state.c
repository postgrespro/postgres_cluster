#include "postgres.h"
#include "utils/hsearch.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"

#include "blockmem.h"
#include "state.h"

#define RAFTABLE_BLOCK_MEM (1024 * 1024)
#define RAFTABLE_HASH_SIZE (127)

typedef struct State {
	LWLockId lock;

	HTAB *hashtable;
	void *blockmem;
} State;

static char *state_get_string(StateP state, RaftableEntry *e)
{
	size_t len;
	char *s;
	Assert(state);
	Assert(LWLockHeldByMe(state->lock));

	len = blockmem_len(state->blockmem, e->block);
	Assert(len > 0);
	s = palloc(len);
	len -= blockmem_get(state->blockmem, e->block, s, len);
	Assert(len == 0);

	return s;
}

static void state_put_string(StateP state, RaftableEntry *e, char *value)
{
	Assert(state);
	Assert(LWLockHeldByMe(state->lock));
	size_t len = strlen(value) + 1;
	if (e->block)
		blockmem_forget(state->blockmem, e->block);
	e->block = blockmem_put(state->blockmem, value, len);
	if (!e->block)
		elog(ERROR, "raftable memory limit hit");
}

static void state_clear(StateP state)
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *e;

	Assert(state);
	Assert(LWLockHeldByMe(state->lock));

	hash_seq_init(&scan, state->hashtable);
	while ((e = (RaftableEntry *)hash_seq_search(&scan))) {
		Assert(e->block);
		blockmem_forget(state->blockmem, e->block);
		hash_search(state->hashtable, e->key.data, HASH_REMOVE, NULL);
	}
	hash_seq_term(&scan);
}

void state_set(StateP state, char *key, char *value)
{
	Assert(state);
	Assert(LWLockHeldByMe(state->lock));

	if (value == NULL)
	{
		RaftableEntry *e = hash_search(state->hashtable, key, HASH_FIND, NULL);
		if (e)
		{
			Assert(e->block);
			blockmem_forget(state->blockmem, e->block);
		}
		hash_search(state->hashtable, key, HASH_REMOVE, NULL);
	}
	else
	{
		bool found;
		RaftableEntry *e = hash_search(state->hashtable, key, HASH_ENTER, &found);
		if (!found)
		{
			strncpy(e->key.data, key, RAFTABLE_KEY_LEN);
			e->key.data[RAFTABLE_KEY_LEN - 1] = '\0';
			e->block = 0;
		}
		state_put_string(state, e, value);
	}
}

char *state_get(StateP state, char *key)
{
	RaftableEntry *e;
	RaftableKey rkey;

	Assert(state);
	LWLockAcquire(state->lock, LW_SHARED);

	strncpy(rkey.data, key, sizeof(rkey.data));
	e = hash_search(state->hashtable, &rkey, HASH_FIND, NULL);

	if (e)
	{
		char *s = state_get_string(state, e);
		LWLockRelease(state->lock);
		return s;
	}
	else
	{
		LWLockRelease(state->lock);
		return NULL;
	}
}

void state_update(StateP state, RaftableUpdate *update, bool clear)
{
	RaftableField *f;
	int i;
	char *cursor = update->data;

	Assert(state);
	LWLockAcquire(state->lock, LW_EXCLUSIVE);

	if (clear) state_clear(state);

	for (i = 0; i < update->fieldnum; i++) {
		f = (RaftableField *)cursor;
		cursor = f->data;
		char *key = cursor; cursor += f->keylen;
		char *value = cursor; cursor += f->vallen;
		state_set(state, key, value);
	}

	LWLockRelease(state->lock);
}

static void state_foreach_entry(StateP state, void (*agg)(StateP, RaftableEntry *, void *), void *arg)
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *e;

	Assert(state);
	Assert(LWLockHeldByMe(state->lock));

	hash_seq_init(&scan, state->hashtable);
	while ((e = (RaftableEntry *)hash_seq_search(&scan))) {
		agg(state, e, arg);
	}
	hash_seq_term(&scan);
}

static void agg_size(StateP state, RaftableEntry *e, void *arg)
{
	size_t *size = arg;
	Assert(state);
	Assert(e->block);
	*size += sizeof(RaftableField) - 1;
	*size += strlen(e->key.data) + 1;
	*size += blockmem_len(state->blockmem, e->block);
}

static void agg_snapshot(StateP state, RaftableEntry *e, void *arg)
{
	char **cursor = arg;
	RaftableField *f;
	Assert(state);
	Assert(e->block);

	f = (RaftableField *)(*cursor);
	(*cursor) = f->data;

	f->keylen = strlen(e->key.data) + 1;
	memcpy(cursor, e->key.data, f->keylen - 1);
	(*cursor)[f->keylen - 1] = '\0';
	(*cursor) += f->keylen;

	if (e->block)
	{
		f->isnull = false;

		char *s = state_get_string(state, e);
		f->vallen = strlen(s) + 1;
		memcpy((*cursor), s, f->vallen);
		pfree(s);

		(*cursor) += f->vallen;
	}
	else
		f->isnull = true;
}

static size_t state_estimate_size(StateP state)
{
	size_t size = sizeof(RaftableUpdate);
	state_foreach_entry(state, agg_size, &size);
	return size;
}

void *state_make_snapshot(StateP state, size_t *size)
{
	RaftableUpdate *update;
	char *cursor;
	Assert(state);
	LWLockAcquire(state->lock, LW_SHARED);

	*size = state_estimate_size(state);
	update = malloc(*size);
	cursor = (char *)update;

	state_foreach_entry(state, agg_snapshot, &cursor);

	LWLockRelease(state->lock);
	return update;
}

void *state_scan(StateP state)
{
	HASH_SEQ_STATUS *scan = palloc(sizeof(HASH_SEQ_STATUS));
	Assert(state);
	LWLockAcquire(state->lock, LW_SHARED);

	hash_seq_init(scan, state->hashtable);
	return scan;
}

bool state_next(StateP state, void *scan, char **key, char **value)
{
	Assert(state);
	Assert(scan);
	Assert(LWLockHeldByMe(state->lock));
	RaftableEntry *e = (RaftableEntry *)hash_seq_search((HASH_SEQ_STATUS *)scan);
	if (e)
	{
		*key = pstrdup(e->key.data);
		*value = state_get_string(state, e);
		return true;
	}
	else
	{
		LWLockRelease(state->lock);
		pfree(scan);
		return false;
	}
}

void state_shmem_request()
{
	int flags;
	HASHCTL info;
	info.keysize = sizeof(RaftableKey);
	info.entrysize = sizeof(RaftableEntry);
	info.dsize = info.max_dsize = hash_select_dirsize(RAFTABLE_HASH_SIZE);
	flags = HASH_SHARED_MEM | HASH_ALLOC | HASH_DIRSIZE | HASH_ELEM;
	RequestAddinShmemSpace(RAFTABLE_BLOCK_MEM + sizeof(State) + hash_get_shared_size(&info, flags));
	RequestAddinLWLocks(1);
}

StateP state_shmem_init()
{
	State *state;

	HASHCTL info;
	info.keysize = sizeof(RaftableKey);
	info.entrysize = sizeof(RaftableEntry);
	bool found;
	int i;

	state = ShmemInitStruct(
		"raftable_state",
		RAFTABLE_BLOCK_MEM,
		&found
	);
	Assert(state);

	state->lock = LWLockAssign();

	state->hashtable = ShmemInitHash(
		"raftable_hashtable",
		RAFTABLE_HASH_SIZE, RAFTABLE_HASH_SIZE,
		&info, HASH_ELEM
	);

	state->blockmem = ShmemInitStruct(
		"raftable_blockmem",
		RAFTABLE_BLOCK_MEM,
		&found
	);
	Assert(state->blockmem);
	blockmem_format(state->blockmem, RAFTABLE_BLOCK_MEM);

	return state;
}
