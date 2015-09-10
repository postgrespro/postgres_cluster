#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

#define unless(x) if (!(x))

#define NODES 1


int main(int argc, char **argv) {
	GlobalTransactionId gtid;
	TransactionId base = 42;
	int transactions = 10000;
	int i;
	gtid.nNodes = NODES;
	gtid.xids = malloc(sizeof(TransactionId) * gtid.nNodes);
	gtid.nodes = malloc(sizeof(NodeId) * gtid.nNodes);

	DTMConn conn = DtmConnect("localhost", 5431);
	if (!conn) {
		exit(1);
	}

	for (i = 0; i < transactions; i++) {

		int n;
		for (n = 0; n < gtid.nNodes; n++) {
			gtid.xids[n] = base + n;
			gtid.nodes[n] = n;
		}

		if (!DtmGlobalStartTransaction(conn, &gtid)) {
			fprintf(stdout, "global transaction not started\n");
			exit(EXIT_FAILURE);
		}

		if (!DtmGlobalSetTransStatus(conn, 0, base + 0, TRANSACTION_STATUS_COMMITTED)) {
			fprintf(stdout, "global transaction not committed\n");
			exit(EXIT_FAILURE);
		}

		unless (i%10) {
			printf("Commited %u txs.\n", i+1);
		}

		base++;
	}


	DtmDisconnect(conn);
	return EXIT_SUCCESS;
}
