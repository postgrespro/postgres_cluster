#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdtm.h"

#define NODES 1

#ifdef WIN32
#include <windows.h>
static float li2f(LARGE_INTEGER x) {
	float result = ((float)x.HighPart) * 4.294967296E9 + (float)((x).LowPart);
	return result;
}

static float now_s() {
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return li2f(count) / li2f(freq);
}
#else
static float now_s() {
	// current time in seconds
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
		return t.tv_sec + t.tv_nsec * 1e-9;
	} else {
		printf("Error while clock_gettime()\n");
		exit(0);
	}
}
#endif

static void start_transaction(DTMConn conn, TransactionId base) {
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
	//fprintf(stdout, "global transaction started\n");

	free(gtid.xids);
	free(gtid.nodes);
}

static void commit_transaction(DTMConn conn, TransactionId base) {
	int n;
	for (n = 0; n < NODES; n++) {
		if (!DtmGlobalSetTransStatus(conn, n, base + n, TRANSACTION_STATUS_COMMITTED)) {
			fprintf(stdout, "global transaction not committed\n");
			exit(EXIT_FAILURE);
		}
	}
	//fprintf(stdout, "global transaction committed\n");
}

static void abort_transaction(DTMConn conn, TransactionId base) {
	if (!DtmGlobalSetTransStatus(conn, 0, base + 0, TRANSACTION_STATUS_ABORTED)) {
		fprintf(stdout, "global transaction not aborted\n");
		exit(EXIT_FAILURE);
	}
	//fprintf(stdout, "global transaction aborted\n");
}

static void show_snapshots(DTMConn conn, TransactionId base) {
	int i, n;
	for (n = 0; n < NODES; n++) {
		Snapshot s = malloc(sizeof(SnapshotData));
		s->xip = NULL;

		if (!DtmGlobalGetSnapshot(conn, n, base + n, s)) {
			fprintf(stdout, "failed to get a snapshot[%d]\n", n);
			exit(EXIT_FAILURE);
		}
		//fprintf(stdout, "snapshot[%d, %#x]: xmin = %#x, xmax = %#x, active =", n, base + n, s->xmin, s->xmax);
		//for (i = 0; i < s->xcnt; i++) {
		//	fprintf(stdout, " %#x", s->xip[i]);
		//}
		//fprintf(stdout, "\n");

		free(s->xip);
		free(s);
	}
}

static void show_status(DTMConn conn, TransactionId base) {
	int n;
	for (n = 0; n < NODES; n++) {
		XidStatus s = DtmGlobalGetTransStatus(conn, n, base + n);
		if (s == -1) {
			fprintf(stdout, "failed to get transaction status [%d, %#x]\n", n, base + n);
			exit(EXIT_FAILURE);
		}
		//fprintf(stdout, "status[%d, %#x]: ", n, base + n);
		//switch (s) {
		//	case TRANSACTION_STATUS_COMMITTED:
		//		fprintf(stdout, "committed\n");
		//		break;
		//	case TRANSACTION_STATUS_ABORTED:
		//		fprintf(stdout, "aborted\n");
		//		break;
		//	case TRANSACTION_STATUS_IN_PROGRESS:
		//		fprintf(stdout, "in progress\n");
		//		break;
		//	default:
		//		fprintf(stdout, "(error)\n");
		//		break;
		//}
	}
}

int main(int argc, char **argv) {
	DTMConn conn = DtmConnect("localhost", 5431);
	if (!conn) {
		exit(1);
	}

	TransactionId base = 42;

	int transactions = atoi(argv[1]);
	int i;
	float started = now_s();
	for (i = 0; i < transactions; i++) {
		printf("-------- %d, base = %d -------- %0.6f sec \n", i, base, now_s() - started);
		start_transaction(conn, base);
		//show_snapshots(conn, base);
		//show_status(conn, base);
		commit_transaction(conn, base);
		base += NODES;
	}
	float elapsed = now_s() - started;

	printf(
		"%d transactions in %0.2f sec -> %0.2f tps\n",
		transactions,
		elapsed,
		transactions / elapsed
	);

	DtmDisconnect(conn);
	return EXIT_SUCCESS;
}
