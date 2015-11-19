#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ftw.h>
#include <unistd.h>

#include "clog.h"

bool test_clog(char *datadir) {
	bool ok = true;

	clog_t clog;
	if (!(clog = clog_open(datadir))) return false;

	clog_close(clog);
	if (!(clog = clog_open(datadir))) return false;

	if (!clog_write(clog, 42, NEGATIVE)) return false;
	if (!clog_write(clog, 1000, NEGATIVE)) return false;
	printf("commit %d status %d\n", 42, clog_read(clog, 42));
	printf("commit %d status %d\n", 1000, clog_read(clog, 1000));
	if (!clog_write(clog, 1000, POSITIVE)) return false;
	if (!clog_write(clog, 1500, DOUBT)) return false;

	if (!clog_close(clog)) return false;
	if (!(clog = clog_open(datadir))) return false;

	int status;

	printf("commit %d status %d (should be 2)\n", 42, status = clog_read(clog, 42));
	if (status != NEGATIVE) return false;

	printf("commit %d status %d (should be 1)\n", 1000, status = clog_read(clog, 1000));
	if (status != POSITIVE) return false;

	printf("commit %d status %d (should be 3)\n", 1500, status = clog_read(clog, 1500));
	if (status != DOUBT) return false;

	printf("commit %d status %d (should be 0)\n", 2044, status = clog_read(clog, 2044));
	if (status != BLANK) return false;

	if (!clog_close(clog)) return false;

	return ok;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	fprintf(stderr, "removing '%s'\n", fpath);
	int r = remove(fpath);
	if (r) {
		fprintf(stderr, "cannot remove '%s': %s\n", fpath, strerror(errno));
	}
	return r;
}

int rmrf(char *path) {
	return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int main() {
	bool ok = true;

	char tmpdir[] = "clog-test-XXXXXX";
	if (!mkdtemp(tmpdir)) {
		fprintf(stderr, "cannot create tmp dir\n");
		return EXIT_FAILURE;
	}
	fprintf(stderr, "created tmp dir '%s'\n", tmpdir);

	ok &= test_clog(tmpdir);

	if (rmrf(tmpdir)) {
		fprintf(stderr, "cannot remove tmp dir '%s'\n", tmpdir);
		return EXIT_FAILURE;
	}

	if (ok) {
		printf("clog-test passed\n");
		return EXIT_SUCCESS;
	} else {
		printf("clog-test FAILED\n");
		return EXIT_FAILURE;
	}
}
