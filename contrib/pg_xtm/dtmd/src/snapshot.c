#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "snapshot.h"

static void append_char(char **cursorp, char c) {
	*((*cursorp)++) = c;
}

static void append_hex16(char **cursorp, xid_t value) {
	int written = sprintf(*cursorp, "%016llx", value);
	assert(written == 16);
	*cursorp += written;
}

char *snapshot_serialize(Snapshot *s) {
	assert(s->seqno > 0);

	int numberlen = 16;
	int numbers = 3 + s->nactive;
	int len = 1 + numberlen * numbers + 1;
	char *data = malloc(len);

	char *cursor = data;

	append_char(&cursor, '+');
	append_hex16(&cursor, s->xmin);
	append_hex16(&cursor, s->xmax);

	int i;
	for (i = 0; i < s->nactive; i++) {
		append_hex16(&cursor, s->active[i]);
	}

	assert(cursor - data == len);
	assert(data[len - 1] == '\0');
	return data;
}
