#include <assert.h>

#include "util.h"
#include "transaction.h"

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
			if (!clog_write(clg, MUX_XID(node, t->xid), status)) {
				shout("clog write failed");
				return false;
			}
		}
	}
	return true;
}
