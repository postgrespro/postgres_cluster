#include "postgres.h"
#include "utils/hsearch.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"

#include "state.h"

#define RAFTABLE_BLOCK_MEM (8*1024 * 1024)
#define RAFTABLE_HASH_SIZE (127)

typedef struct State {
	HTAB *hashtable;
} State;

//static void state_put_string(StateP state, RaftableEntry *e, const char *value, size_t len)
//{
//	Assert(state);
//	if (e->value) pfree(e->value);
//	e->value = palloc(len);
//	memcpy(e->value, value, len);
//}

static void state_clear(StateP state)
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *e;

	Assert(state);

	hash_seq_init(&scan, state->hashtable);
	while ((e = (RaftableEntry *)hash_seq_search(&scan)))
	{
		Assert(e->value);
		pfree(e->value);
		hash_search(state->hashtable, e->key.data, HASH_REMOVE, NULL);
	}
}

void state_set(StateP state, const char *key, const char *value, size_t vallen)
{
	Assert(state);
	fprintf(stderr, "setting state[%s] = %.*s\n", key, (int)vallen, value);

	if (value == NULL)
	{
		RaftableEntry *e = hash_search(state->hashtable, key, HASH_FIND, NULL);
		if (e)
		{
			Assert(e->value);
			pfree(e->value);
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
			e->value = NULL;
		}
		else
		{
			Assert(e->value != NULL);
			pfree(e->value);
		}
		e->vallen = vallen;
		e->value = memcpy(palloc(vallen), value, vallen);
		fprintf(stderr, "value set to %.*s\n", (int)e->vallen, e->value);
	}
}

char *state_get(StateP state, const char *key, size_t *len)
{
	RaftableEntry *e;
	RaftableKey rkey;

	Assert(state);

	strncpy(rkey.data, key, sizeof(rkey.data));
	e = hash_search(state->hashtable, &rkey, HASH_FIND, NULL);

	if (e)
	{
		*len = e->vallen;
		return memcpy(palloc(e->vallen), e->value, e->vallen);
	}
	else
	{
		*len = 0;
		return NULL;
	}
}

void state_update(StateP state, RaftableMessage *msg, bool clear)
{
	RaftableField *f;
	int i;
	char *cursor = msg->data;

	Assert(state);

	if (clear) state_clear(state);

	for (i = 0; i < msg->fieldnum; i++) {
		char *key, *value;
		f = (RaftableField *)cursor;
		cursor = f->data;
		key = cursor; cursor += f->keylen;
		value = cursor; cursor += f->vallen;
		state_set(state, key, value, f->vallen);
	}
}

static void state_foreach_entry(StateP state, void (*agg)(StateP, RaftableEntry *, void *), void *arg)
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *e;

	Assert(state);

	hash_seq_init(&scan, state->hashtable);
	while ((e = (RaftableEntry *)hash_seq_search(&scan))) {
		agg(state, e, arg);
	}
}

static void agg_size(StateP state, RaftableEntry *e, void *arg)
{
	size_t *size = arg;
	Assert(state);
	Assert(e->value);
	*size += sizeof(RaftableField) - 1;
	*size += strlen(e->key.data) + 1;
	*size += e->vallen;
}

static void agg_snapshot(StateP state, RaftableEntry *e, void *arg)
{
	char **cursor = arg;
	RaftableField *f;
	Assert(state);
	Assert(e->value);

	f = (RaftableField *)(*cursor);
	(*cursor) = f->data;

	f->keylen = strlen(e->key.data) + 1;
	memcpy(*cursor, e->key.data, f->keylen - 1);
	(*cursor)[f->keylen - 1] = '\0';
	(*cursor) += f->keylen;

	if (e->value)
	{
		f->isnull = false;

		memcpy((*cursor), e->value, e->vallen);

		(*cursor) += e->vallen;
		f->vallen = e->vallen;
	}
	else
		f->isnull = true;
}

static size_t state_estimate_size(StateP state)
{
	size_t size = sizeof(RaftableMessage);
	state_foreach_entry(state, agg_size, &size);
	return size;
}

void *state_make_snapshot(StateP state, size_t *size)
{
	RaftableMessage *message;
	char *cursor;
	Assert(state);

	*size = state_estimate_size(state);
	message = malloc(*size); /* this is later freed by raft with a call to plain free() */
	cursor = (char *)message;

	state_foreach_entry(state, agg_snapshot, &cursor);

	return message;
}

void *state_scan(StateP state)
{
	HASH_SEQ_STATUS *scan = palloc(sizeof(HASH_SEQ_STATUS));
	Assert(state);

	hash_seq_init(scan, state->hashtable);
	return scan;
}

bool state_next(StateP state, void *scan, char **key, char **value, size_t *len)
{
	RaftableEntry *e;
	Assert(state);
	Assert(scan);
	e = (RaftableEntry *)hash_seq_search((HASH_SEQ_STATUS *)scan);
	if (e)
	{
		*key = pstrdup(e->key.data);
		*len = e->vallen;
		*value = memcpy(palloc(e->vallen), e->value, e->vallen);
		return true;
	}
	else
	{
		pfree(scan);
		return false;
	}
}

StateP state_init(void)
{
	State *state = palloc(sizeof(State));

	HASHCTL info;
	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(RaftableKey);
	info.entrysize = sizeof(RaftableEntry);

	state->hashtable = hash_create("raftable_hashtable",
								   128, &info,
								   HASH_ELEM);

	return state;
}

RaftableMessage *make_single_value_message(const char *key, const char *value, size_t vallen, size_t *size)
{
	RaftableField *f;
	RaftableMessage *msg;
	size_t keylen = 0;
	*size = sizeof(RaftableMessage);

	keylen = strlen(key) + 1;

	*size += sizeof(RaftableField) - 1;
	*size += keylen;
	*size += vallen;
	msg = palloc(*size);

	msg->fieldnum = 1;

	f = (RaftableField *)msg->data;
	f->keylen = keylen;
	f->vallen = vallen;
	memcpy(f->data, key, keylen);
	if (vallen > 0)
		memcpy(f->data + keylen, value, vallen);

	return msg;
}

