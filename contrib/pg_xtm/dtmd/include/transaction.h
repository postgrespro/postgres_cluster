#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>
#include "int.h"
#include "clog.h"
#include "snapshot.h"
#include "limits.h"

#define MAX_SNAPSHOTS_PER_TRANS 8

#define CHAR_TO_INDEX(C) ((C) - 'a')

typedef struct Transaction {
	xid_t xid;

	int size; // number of paritcipants
	int max_size; // maximal number of participants

	// for + against ≤ size
	int votes_for;
	int votes_against;

	Snapshot snapshots[MAX_SNAPSHOTS_PER_TRANS];
	int snapshots_count; // will wrap around if exceeds max snapshots

	void *listeners[CHAR_TO_INDEX('z')]; // we are going to use 'a' to 'z' for indexing
} Transaction;

Snapshot *transaction_latest_snapshot(Transaction *t);
Snapshot *transaction_snapshot(Transaction *t, int snapno);
Snapshot *transaction_next_snapshot(Transaction *t);
int transaction_status(Transaction *t);
void transaction_clear(Transaction *t);
void transaction_push_listener(Transaction *t, char cmd, void *listener);
void *transaction_pop_listener(Transaction *t, char cmd);
bool transaction_participate(Transaction *t, int clientid);

#endif
