#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

#define unless(x) if (!(x))

int main(int argc, char **argv) {
	DtmGlobalConfig("localhost", 5002, "/tmp");

	int i;
	for (i = 0; i < transactions; i++) {
		TransactionId xid, gxmin;
		Snapshot snapshot;
		xid = DtmGlobalStartTransaction(snapshot, &gxmin);
		if (xid == INVALID_XID) {
			fprintf(stdout, "global transaction not started\n");
			exit(EXIT_FAILURE);
		}

		DtmGlobalGetSnapshot(xid, snapshot, &gxmin);

		bool wait = true;

		XidStatus s = DtmGlobalSetTransStatus(xid, TRANSACTION_STATUS_COMMITTED, wait);
		if (s != TRANSACTION_STATUS_COMMITTED) {
			fprintf(stdout, "global transaction not committed\n");
			exit(EXIT_FAILURE);
		}

		s = DtmGlobalGetTransStatus(xid, wait);
		if (s != TRANSACTION_STATUS_COMMITTED) {
			fprintf(stdout, "global transaction not committed\n");
			exit(EXIT_FAILURE);
		}

		unless (i%10) {
			printf("Commited %u txs.\n", i+1);
		}

		base++;
	}

	return EXIT_SUCCESS;
}
