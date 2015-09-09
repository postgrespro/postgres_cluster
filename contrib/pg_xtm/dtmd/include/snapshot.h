#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "int.h"
#include "limits.h"

typedef struct Snapshot {
	// initially 0, which means 'invalid snapshot'
	int seqno;

	xid_t xmin;
	xid_t xmax;
	int nactive;
	xid_t active[MAX_TRANSACTIONS];
} Snapshot;

char *snapshot_serialize(Snapshot *s);
void snapshot_sort(Snapshot *s);

#endif
