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
	size_t vallen;
	char *value;
} RaftableEntry;

typedef struct RaftableField
{
	size_t keylen;
	size_t vallen;
	bool isnull;
	char data[1];
} RaftableField;

#define MEAN_FAIL '!'
#define MEAN_OK   '.'
#define MEAN_GET  '?'
#define MEAN_SET  '='

typedef struct RaftableMessage
{
	char meaning;
	int fieldnum;
	char data[1];
} RaftableMessage;

typedef struct State *StateP;

StateP state_init(void);

void state_set(StateP state, const char *key, const char *value, size_t len);
char *state_get(StateP state, const char *key, size_t *len);

void state_update(StateP state, RaftableMessage *msg, bool clear);
void *state_make_snapshot(StateP state, size_t *size);

void *state_scan(StateP state);
bool state_next(StateP state, void *scan, char **key, char **value, size_t *len);

RaftableMessage *make_single_value_message(const char *key, const char *value, size_t vallen, size_t *size);

#endif
