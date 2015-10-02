#ifndef INTSET_H
#define INTSET_H

#include "int.h"

// TODO: use tree structures instead of this uglyness

typedef struct intset_t {
	xid_t *data;
	int shift;
	int size;
	int capacity;
} intset_t;

// Constructor and destructor
intset_t *intset_create(int capacity);
void intset_destroy(intset_t *this);

// Appends the value at the end. The value should be greater than all the
// previously added values.
void intset_add(intset_t *this, xid_t value);

// Returns the value of the i-th element.
xid_t intset_get(intset_t *this, int i);

// Removes the value, shifting everything else to the beginning.
void intset_remove(intset_t *this, xid_t value);

// Returns the size of the intset.
int intset_size(intset_t *this);

#endif
