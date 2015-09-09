#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

int main() {
	DTMConn conn = DtmConnect("localhost", 5431);
	if (!conn) {
		fprintf(stderr, "failed to connect to dtmd\n");
		exit(1);
	}

	xid_t xid = 0xdeadbeefcafebabe;
	Snapshot s = malloc(sizeof(SnapshotData));
	
	DtmGlobalGetSnapshot(conn, 0, xid, s);
	fprintf(stdout, "snapshot is %d xids wide\n", s->xcnt);

	DtmDisconnect(conn);

	return 0;
}
