#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

#define NODES 5

void start_transaction(DTMConn conn, TransactionId base) {
	GlobalTransactionId gtid;
	gtid.nNodes = NODES;
	gtid.xids = malloc(sizeof(TransactionId) * gtid.nNodes);
	gtid.nodes = malloc(sizeof(NodeId) * gtid.nNodes);

	int n;
	for (n = 0; n < gtid.nNodes; n++) {
		gtid.xids[n] = base + n;
		gtid.nodes[n] = n;
	}

	if (!DtmGlobalStartTransaction(conn, &gtid)) {
		fprintf(stdout, "global transaction not started\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "global transaction started\n");

	free(gtid.xids);
	free(gtid.nodes);
}

void commit_transaction(DTMConn conn, TransactionId base) {
	int n;
	for (n = 0; n < NODES; n++) {
		if (!DtmGlobalSetTransStatus(conn, n, base + n, TRANSACTION_STATUS_COMMITTED)) {
			fprintf(stdout, "global transaction not committed\n");
			exit(EXIT_FAILURE);
		}
	}
	fprintf(stdout, "global transaction committed\n");
}

void abort_transaction(DTMConn conn, TransactionId base) {
	if (!DtmGlobalSetTransStatus(conn, 0, base + 0, TRANSACTION_STATUS_ABORTED)) {
		fprintf(stdout, "global transaction not aborted\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "global transaction aborted\n");
}

void show_snapshots(DTMConn conn, TransactionId base) {
	int i, n;
	for (n = 0; n < NODES; n++) {
		Snapshot s = malloc(sizeof(SnapshotData));
		s->xip = NULL;

		if (!DtmGlobalGetSnapshot(conn, n, base + n, s)) {
			fprintf(stdout, "failed to get a snapshot[%d]\n", n);
			exit(EXIT_FAILURE);
		}
		fprintf(stdout, "snapshot[%d, %#x]: xmin = %#x, xmax = %#x, active =", n, base + n, s->xmin, s->xmax);
		for (i = 0; i < s->xcnt; i++) {
			fprintf(stdout, " %#x", s->xip[i]);
		}
		fprintf(stdout, "\n");

		free(s->xip);
		free(s);
	}
}

void show_status(DTMConn conn, TransactionId base) {
	int n;
	for (n = 0; n < NODES; n++) {
		XidStatus s = DtmGlobalGetTransStatus(conn, n, base + n);
		if (s == -1) {
			fprintf(stdout, "failed to get transaction status [%d, %#x]\n", n, base + n);
			exit(EXIT_FAILURE);
		}
		fprintf(stdout, "status[%d, %#x]: ", n, base + n);
		switch (s) {
			case TRANSACTION_STATUS_COMMITTED:
				fprintf(stdout, "committed\n");
				break;
			case TRANSACTION_STATUS_ABORTED:
				fprintf(stdout, "aborted\n");
				break;
			case TRANSACTION_STATUS_IN_PROGRESS:
				fprintf(stdout, "in progress\n");
				break;
			default:
				fprintf(stdout, "(error)\n");
				break;
		}
	}
}

int main() {
	DTMConn conn = DtmConnect("localhost", 5431);
	if (!conn) {
		fprintf(stderr, "failed to connect to dtmd\n");
		exit(1);
	}

	TransactionId base0 = 0xdeadbeef;
	TransactionId base1 = base0 + NODES;
	TransactionId base2 = base1 + NODES;

	start_transaction(conn, base0);
	show_snapshots(conn, base0);

	start_transaction(conn, base1);
	show_snapshots(conn, base0);
	show_snapshots(conn, base1);

	start_transaction(conn, base2);
	show_snapshots(conn, base0);
	show_snapshots(conn, base1);
	show_snapshots(conn, base2);

	commit_transaction(conn, base0);
	show_snapshots(conn, base1);
	show_snapshots(conn, base2);

	abort_transaction(conn, base1);
	show_snapshots(conn, base2);

	show_status(conn, base0);
	show_status(conn, base1);
	show_status(conn, base2);

	DtmDisconnect(conn);
	return EXIT_SUCCESS;
}
