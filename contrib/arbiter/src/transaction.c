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

	#if 0
	/*
	 * Commented out in order to report ABORTED status immediately:
	 * not waiting for all responses
	 */
	if (t->votes_for + t->votes_against < t->size) {
		return DOUBT;
	}
	#endif

	if (t->votes_against) {
		return NEGATIVE;
	}
	
	if (t->votes_for == t->size) {
		return POSITIVE;
	}

	return DOUBT;
}

void transaction_clear(Transaction *t) {
	int i;

	t->xid = INVALID_XID;
	t->xmin = INVALID_XID;
	t->size = 0;
	t->fixed_size = false;
	t->votes_for = 0;
	t->votes_against = 0;
	t->snapshots_count = 0;

	for (i = 'a'; i <= 'z'; i++) {
		t->listeners[CHAR_TO_INDEX(i)] = NULL;
	}
}

void transaction_push_listener(Transaction *t, char cmd, void *listener) {
	list_node_t *n;

	assert((cmd >= 'a') && (cmd <= 'z'));
	n = malloc(sizeof(list_node_t));
	n->value = listener;
	n->next = t->listeners[CHAR_TO_INDEX(cmd)];
	t->listeners[CHAR_TO_INDEX(cmd)] = n;
}

void *transaction_pop_listener(Transaction *t, char cmd) {
	list_node_t *n;
	void *value;

	assert((cmd >= 'a') && (cmd <= 'z'));
	if (!t->listeners[CHAR_TO_INDEX(cmd)]) {
		return NULL;
	}
	n = t->listeners[CHAR_TO_INDEX(cmd)];
	t->listeners[CHAR_TO_INDEX(cmd)] = n->next;
	value = n->value;
	free(n);
	return value;
}

bool transaction_remove_listener(Transaction *t, char cmd, void *listener) {
	list_node_t *prev, *victim;

	assert((cmd >= 'a') && (cmd <= 'z'));
	prev = NULL;
	victim = t->listeners[CHAR_TO_INDEX(cmd)];

	// find the victim
	while (victim) {
		if (victim->value == listener) {
			break;
		}
		prev = victim;
		victim = victim->next;
	}

	if (victim) {
		// victim found
		if (prev) {
			prev->next = victim->next;
		} else {
			t->listeners[CHAR_TO_INDEX(cmd)] = victim->next;
		}
		free(victim);
		return true;
	}

	return false;
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
