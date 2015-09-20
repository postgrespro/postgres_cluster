#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>
#include "int.h"
#include "clog.h"
#include "snapshot.h"
#include "limits.h"

#define MAX_SNAPSHOTS_PER_TRANS 8

typedef struct Transaction {
	// true if the transaction was started on the node
	bool active;

	int client_id;
	int node;
	int vote;

	xid_t xid;
	Snapshot snapshot[MAX_SNAPSHOTS_PER_TRANS];

	// if this is equal to seqno, we need to generate a new snapshot (for each node)
	int snapshot_no;
} Transaction;

#define CHAR_TO_INDEX(C) ((C) - 'a')
typedef struct GlobalTransaction {
	int n_snapshots;
	Transaction participants[MAX_NODES];
	void *listeners[CHAR_TO_INDEX('z')]; // we are going to use 'a' to 'z' for indexing
} GlobalTransaction;

int global_transaction_status(GlobalTransaction *gt);
bool global_transaction_mark(clog_t clg, GlobalTransaction *gt, int status);
void global_transaction_clear(GlobalTransaction *gt);
void global_transaction_push_listener(GlobalTransaction *gt, char cmd, void *listener);
void *global_transaction_pop_listener(GlobalTransaction *gt, char cmd);

#endif
