#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "snapshot.h"
#include "util.h"

static int compare_xid(void const* x, void const* y) {
	xid_t xid1 = *(xid_t*)x;
	xid_t xid2 = *(xid_t*)y;
	return xid1 < xid2 ? -1 : xid1 == xid2 ? 0 : 1;
}

void snapshot_sort(Snapshot *s) {
	qsort(s->active, s->nactive, sizeof(xid_t), compare_xid);
}
