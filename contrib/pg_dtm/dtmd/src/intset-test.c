#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "intset.h"

#define CHECK(VALUE, EXPECTED, RESULT) \
	do { \
		if (RESULT) { \
			if ((VALUE) != (EXPECTED)) { \
				printf( \
					"intset %s FAILED: %s is %d, but %d expected\n", \
					(__FUNCTION__), #VALUE, (VALUE), (EXPECTED)  \
				); \
				RESULT &= false; \
			} else { \
				printf( \
					"intset %s ok: %s is %d\n", \
					(__FUNCTION__), #VALUE, (VALUE) \
				); \
				RESULT &= true; \
			} \
		} \
	} while (0)

bool check_add_remove() {
	bool ok = true;
	intset_t *s = intset_create(5);

	CHECK(s->capacity, 5, ok);
	CHECK(s->size, 0, ok);
	CHECK(s->shift, 0, ok);

	intset_add(s, 10);
	intset_add(s, 20);
	intset_add(s, 30);
	intset_add(s, 40);
	intset_add(s, 50);
	intset_remove(s, 20);
	intset_remove(s, 40);
	intset_add(s, 60);
	intset_add(s, 70);
	intset_remove(s, 60);
	intset_remove(s, 10);

	CHECK(s->size, 3, ok);
	CHECK(s->shift, 1, ok);

	intset_add(s, 80);
	CHECK(s->size, 4, ok);
	intset_add(s, 90);
	CHECK(s->size, 5, ok);

	intset_destroy(s);
	return ok;
}

int main() {
	bool ok = true;
	ok &= check_add_remove();

	if (ok) {
		printf("intset-test passed\n");
		return EXIT_SUCCESS;
	} else {
		printf("intset-test FAILED\n");
		return EXIT_FAILURE;
	}
}
