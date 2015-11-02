#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "int.h"
#include "dtmdlimits.h"

typedef struct Snapshot {
	xid_t xmin;
	xid_t xmax;
	int nactive;
	xid_t active[MAX_TRANSACTIONS];
	int times_sent;
} Snapshot;

void snapshot_sort(Snapshot *s);

#endif
