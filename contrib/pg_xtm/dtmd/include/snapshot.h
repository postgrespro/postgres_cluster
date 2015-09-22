#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "int.h"
#include "limits.h"

typedef struct Snapshot {
	xid_t xmin;
	xid_t xmax;
	int nactive;
	xid_t active[MAX_TRANSACTIONS];
	int times_sent;
} Snapshot;

char *snapshot_serialize(Snapshot *s);
void snapshot_sort(Snapshot *s);

#endif
