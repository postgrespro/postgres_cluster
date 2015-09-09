#include <stdio.h>
#include <stdlib.h>
#include "clogfile.h"

int main() {
	clogfile_t a, b;
	if (!clogfile_open_by_id(&a, "/tmp", 0, false)) {
		if (!clogfile_open_by_id(&a, "/tmp", 0, true)) {
			return EXIT_FAILURE;
		}
	}
	if (!clogfile_open_by_id(&b, "/tmp", 1, false)) {
		if (!clogfile_open_by_id(&b, "/tmp", 1, true)) {
			return EXIT_FAILURE;
		}
	}

	uint64_t xid;
	for (xid = 0; xid < 32; xid++) {
		int status;
		if (xid < 16) {
			status = clogfile_get_status(&a, xid);
		} else {
			status = clogfile_get_status(&b, xid);
		}
		printf("before: %lu status %d\n", xid, status);
	}

	if (!clogfile_set_status(&a, 0, XSTATUS_INPROGRESS)) return EXIT_FAILURE;
	if (!clogfile_set_status(&a, 1, XSTATUS_COMMITTED)) return EXIT_FAILURE;
	if (!clogfile_set_status(&a, 2, XSTATUS_ABORTED)) return EXIT_FAILURE;
	if (!clogfile_set_status(&b, 29, XSTATUS_INPROGRESS)) return EXIT_FAILURE;
	if (!clogfile_set_status(&b, 30, XSTATUS_COMMITTED)) return EXIT_FAILURE;
	if (!clogfile_set_status(&b, 31, XSTATUS_ABORTED)) return EXIT_FAILURE;

	for (xid = 0; xid < 32; xid++) {
		int status;
		if (xid < 16) {
			status = clogfile_get_status(&a, xid);
		} else {
			status = clogfile_get_status(&b, xid);
		}
		printf(" after: %lu status %d\n", xid, status);
	}

	if (!clogfile_close(&a)) return EXIT_FAILURE;
	if (!clogfile_close(&b)) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
