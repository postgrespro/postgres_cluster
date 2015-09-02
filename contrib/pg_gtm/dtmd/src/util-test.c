#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

bool check_join_path(const char *dir, const char *file, const char *check) {
	char *joined = join_path(dir, file);
	bool ok = strcmp(check, joined) == 0;
	printf(
		"'%s' + '%s' = '%s' (%s)\n",
		dir, file, joined,
		ok ? "ok" : "FAILED"
	);
	free(joined);
	return ok;
}

bool check_inrange(cid_t min, cid_t x, cid_t max, bool check) {
	bool result = inrange(min, x, max);
	bool ok = result == check;
	printf(
		"%llu <= %llu <= %llu == %s (%s)\n",
		min, x, max,
		result ? "true" : "false",
		ok ? "ok" : "FAILED"
	);
	return ok;
}

int main() {
	bool ok = true;
	ok &= check_join_path("", "", "");

	ok &= check_join_path("", "bar", "bar");
	ok &= check_join_path("", "/bar", "/bar");
	ok &= check_join_path("foo", "", "foo");

	ok &= check_join_path("foo", "bar", "foo/bar");
	ok &= check_join_path("foo/", "bar", "foo/bar");
	ok &= check_join_path("foo", "/bar", "/bar");
	ok &= check_join_path("foo/", "/bar", "/bar");
	ok &= check_join_path("foo/", "", "foo");

	ok &= check_inrange(1, 2, 3, true);
	ok &= check_inrange(1, 1, 2, true);
	ok &= check_inrange(1, 1, 1, true);
	ok &= check_inrange(1, 2, 2, true);
	ok &= check_inrange(1, 2, 1, false);
	ok &= check_inrange(1, 0, 1, false);
	ok &= check_inrange(1, 3, 2, false);
	ok &= check_inrange(1, 0, 2, false);

	if (ok) {
		printf("util-test passed\n");
		return EXIT_SUCCESS;
	} else {
		printf("util-test FAILED\n");
		return EXIT_FAILURE;
	}
}
