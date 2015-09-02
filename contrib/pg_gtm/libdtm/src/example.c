#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

int main() {
	DTMConn conn = DtmConnect("localhost", 5555);
	if (!conn) {
		fprintf(stderr, "failed to connect to dtmd\n");
		exit(1);
	}

	cid_t gcid;
	
	gcid = DtmGlobalPrepare(conn);
	if (gcid == INVALID_GCID) {
		fprintf(stderr, "failed to prepare a commit\n");
		exit(1);
	}
	fprintf(stdout, "prepared gcid = %llu\n", gcid);

	if (!DtmGlobalCommit(conn, gcid)) {
		fprintf(stderr, "failed to commit gcid = %llu\n", gcid);
		exit(1);
	}

	gcid = DtmGlobalPrepare(conn);
	if (gcid == INVALID_GCID) {
		fprintf(stderr, "failed to prepare a commit\n");
		exit(1);
	}
	fprintf(stdout, "prepared gcid = %llu\n", gcid);

	DtmGlobalRollback(conn, gcid);
	int status = DtmGlobalGetTransStatus(conn, gcid);
	fprintf(stdout, "status of %llu is %d\n", gcid, status);

	cid_t horizon = DtmGlobalGetNextCid(conn);
	fprintf(stdout, "horizon is %llu\n", horizon);

	DtmDisconnect(conn);

	return 0;
}
