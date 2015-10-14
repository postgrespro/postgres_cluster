#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>
#include "int.h"
#include "clog.h"
#include "snapshot.h"
#include "limits.h"

#define MAX_SNAPSHOTS_PER_TRANS 8

#define CHAR_TO_INDEX(C) ((C) - 'a')

typedef struct L2List
{
    struct L2List* next;
    struct L2List* prev;
} L2List;

typedef struct Transaction {
    L2List elem;
	xid_t xid;

	int size; // number of paritcipants

	// for + against ≤ size
	int votes_for;
	int votes_against;

	Snapshot snapshots[MAX_SNAPSHOTS_PER_TRANS];
	int snapshots_count; // will wrap around if exceeds max snapshots

	void *listeners[CHAR_TO_INDEX('z')]; // we are going to use 'a' to 'z' for indexing
} Transaction;

static inline void l2_list_link(L2List* after, L2List* elem)
{
    elem->next = after->next;
    elem->prev = after;
    after->next->prev = elem;
    after->next = elem;
}

static inline void l2_list_unlink(L2List* elem)
{
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
}



Snapshot *transaction_latest_snapshot(Transaction *t);
Snapshot *transaction_snapshot(Transaction *t, int snapno);
Snapshot *transaction_next_snapshot(Transaction *t);
int transaction_status(Transaction *t);
void transaction_clear(Transaction *t);
void transaction_push_listener(Transaction *t, char cmd, void *listener);
void *transaction_pop_listener(Transaction *t, char cmd);
bool transaction_participate(Transaction *t, int clientid);

#endif
