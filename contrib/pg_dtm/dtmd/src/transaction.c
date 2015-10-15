#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include "transaction.h"

typedef struct list_node_t {
	void *value;
	struct list_node_t *next;
} list_node_t;

int transaction_status(Transaction *t) {
	assert(t->votes_for + t->votes_against <= t->size);

	if (t->votes_for + t->votes_against < t->size) {
		return DOUBT;
	}

	if (t->votes_against) {
		return NEGATIVE;
	} else {
		return POSITIVE;
	}
}

void transaction_clear(Transaction *t) {
	int i;
    
	t->xid = INVALID_XID;
	t->size = 0;
	t->votes_for = 0;
	t->votes_against = 0;
	t->snapshots_count = 0;

	for (i = 'a'; i <= 'z'; i++) {
		t->listeners[CHAR_TO_INDEX(i)] = NULL;
	}
}

void transaction_push_listener(Transaction *t, char cmd, void *stream) {
	assert((cmd >= 'a') && (cmd <= 'z'));
	list_node_t *n = malloc(sizeof(list_node_t));
	n->value = stream;
	n->next = t->listeners[CHAR_TO_INDEX(cmd)];
	t->listeners[CHAR_TO_INDEX(cmd)] = n;
}

void *transaction_pop_listener(Transaction *t, char cmd) {
	assert((cmd >= 'a') && (cmd <= 'z'));
	if (!t->listeners[CHAR_TO_INDEX(cmd)]) {
		return NULL;
	}
	list_node_t *n = t->listeners[CHAR_TO_INDEX(cmd)];
	t->listeners[CHAR_TO_INDEX(cmd)] = n->next;
	void *value = n->value;
	free(n);
	return value;
}

Snapshot *transaction_snapshot(Transaction *t, int snapno) {
	return t->snapshots + (snapno % MAX_SNAPSHOTS_PER_TRANS);
}

Snapshot *transaction_latest_snapshot(Transaction *t) {
	return transaction_snapshot(t, t->snapshots_count - 1);
}

Snapshot *transaction_next_snapshot(Transaction *t) {
	return transaction_snapshot(t, t->snapshots_count++);
}
