#ifndef RAFTABLE_STATE_H
#define RAFTABLE_STATE_H

#define RAFTABLE_KEY_LEN (64)

typedef struct RaftableKey
{
	char data[RAFTABLE_KEY_LEN];
} RaftableKey;

typedef struct RaftableEntry
{
	RaftableKey key;
	int block;
} RaftableEntry;

typedef struct RaftableField {
	int keylen; /* with NULL at the end */
	int vallen; /* with NULL at the end */
	bool isnull;
	char data[1];
} RaftableField;

typedef struct RaftableUpdate {
	int expector;
	int fieldnum;
	char data[1];
} RaftableUpdate;

typedef struct State *StateP;

void state_set(StateP state, char *key, char *value);
char *state_get(StateP state, char *key);

void state_update(StateP state, RaftableUpdate *update, bool clear);
void *state_make_snapshot(StateP state, size_t *size);

void *state_scan(StateP state);
bool state_next(StateP state, void *scan, char **key, char **value);

void state_shmem_request();
StateP state_shmem_init();

#endif
