#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include "transaction.h"

typedef struct list_node_t {
	void *value;
	struct list_node_t *next;
} list_node_t;

int global_transaction_status(GlobalTransaction *gt) {
	int node;
	int forcount = 0, againstcount = 0, inprogresscount = 0;
	for (node = 0; node < MAX_NODES; node++) {
		Transaction *t = gt->participants + node;
		if (t->active) {
			assert(t->node == node);
			switch (t->vote) {
				case BLANK:
					shout("a blank vote, this should not happen");
					return BLANK;
				case NEGATIVE:
					againstcount++;
					break;
				case DOUBT:
					inprogresscount++;
					break;
				case POSITIVE:
					forcount++;
					break;
			}
		}
	}
	if (againstcount) {
		return NEGATIVE;
	} else if (inprogresscount) {
		return DOUBT;
	} else {
		return POSITIVE;
	}
}

bool global_transaction_mark(clog_t clg, GlobalTransaction *gt, int status) {
	int node;
	for (node = 0; node < MAX_NODES; node++) {
		Transaction *t = gt->participants + node;
		if (t->active) {
			assert(t->node == node);
			if (clog_write(clg, MUX_XID(node, t->xid), status)) {
				shout("clog write %llu, %d\n", MUX_XID(node, t->xid), status);
			} else {
				shout("clog write failed\n");
				return false;
			}
		} else {
			shout("node %d is not a participant\n", node);
		}
	}
	return true;
}

void global_transaction_clear(GlobalTransaction *gt) {
	int i;
	for (i = 0; i < MAX_NODES; i++) {
		gt->participants[i].active = false;
	}
	gt->listener = NULL;
}

void global_transaction_push_listener(GlobalTransaction *gt, void *stream) {
	list_node_t *n = malloc(sizeof(list_node_t));
	n->value = stream;
	n->next = gt->listener;
	gt->listener = n;
}

void *global_transaction_pop_listener(GlobalTransaction *gt) {
	if (!gt->listener) {
		return NULL;
	}
	list_node_t *n = gt->listener;
	gt->listener = n->next;
	void *value = n->value;
	free(n);
	return value;
}
