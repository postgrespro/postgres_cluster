#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

int main() {
	DTMConn conn = DtmConnect("localhost", 5431);
	if (!conn) {
		fprintf(stderr, "failed to connect to dtmd\n");
		exit(1);
	}

	xid_t gxid;
	Snapshot s;
	
	gxid = DtmGlobalBegin(conn);
	if (gxid == INVALID_GXID) {
		fprintf(stderr, "failed to begin a transaction\n");
		exit(1);
	}
	fprintf(stdout, "began gxid = %llu\n", gxid);

	s = DtmGlobalGetSnapshot(conn);
	fprintf(stdout, "snapshot is %d xids wide\n", s->xcnt);

	if (!DtmGlobalCommit(conn, gxid)) {
		fprintf(stderr, "failed to commit gxid = %llu\n", gxid);
		exit(1);
	}

	gxid = DtmGlobalBegin(conn);
	if (gxid == INVALID_GXID) {
		fprintf(stderr, "failed to begin a transaction\n");
		exit(1);
	}
	fprintf(stdout, "began gxid = %llu\n", gxid);

	DtmGlobalRollback(conn, gxid);
	int status = DtmGlobalGetTransStatus(conn, gxid);
	fprintf(stdout, "status of %llu is %d\n", gxid, status);

	s = DtmGlobalGetSnapshot(conn);
	fprintf(stdout, "snapshot is %d xids wide\n", s->xcnt);

	DtmDisconnect(conn);

	return 0;
}
