#include <stdlib.h>
#include <assert.h>

#include "intset.h"

// TODO: use tree structures instead of this uglyness

// For a given value and given range, ensure that the value is in that range,
// either already or as a result of adding or subtracting an integer amount of
// that range's widths.
static xid_t loop_inside(xid_t min, xid_t val, xid_t max) {
	xid_t width = max - min + 1;
	return ((val - min) % width + width) % width + min;
}

// Constructor
intset_t *intset_create(int capacity) {
	intset_t *this = malloc(sizeof(intset_t));
	this->data = malloc(capacity * sizeof(intset_t));
	this->size = 0;
	this->capacity = capacity;
	this->shift = 0;
}

// Destructor
void intset_destroy(intset_t *this) {
	free(this->data);
	free(this);
}

// Sets the value of the i-th element.
static void intset_set(intset_t *this, int i, xid_t value) {
	int effective_i = loop_inside(
		0,
		i + this->shift,
		this->capacity - 1
	);
	assert(effective_i >= 0);
	assert(effective_i < this->capacity);
	this->data[effective_i] = value;
}

// Appends the value at the end. The value should be greater than all the
// previously added values.
void intset_add(intset_t *this, xid_t value) {
	assert(this->size < this->capacity);
	intset_set(this, this->size, value);
	if (this->size > 0) {
		assert(intset_get(this, this->size - 1) < value);
	}
	this->size++;
}

// Returns the value of the i-th element.
xid_t intset_get(intset_t *this, int i) {
	int effective_i = loop_inside(
		0,
		i + this->shift,
		this->capacity - 1
	);
	assert(effective_i >= 0);
	assert(effective_i < this->capacity);
	return this->data[effective_i];
}

// Removes the value, shifting everything else to the beginning.
void intset_remove(intset_t *this, xid_t value) {
	int found = 0;
	int i;
	for (i = 0; i < this->size; i++) {
		xid_t candidate = intset_get(this, i);
		if (candidate == value) {
			found = 1;
			break;
		}
		if (candidate > value) {
			break;
		}
	}
	assert(found);
	if (i) {
		// Real shifting back
		for (; i < this->size - 1; i++) {
			intset_set(this, i, intset_get(this, i + 1));
		}
	} else {
		// Imaginary shifting forth
		this->shift = loop_inside(
			0,
			this->shift + 1,
			this->capacity - 1
		);
	}
	this->size--;
	// TODO: shift firth if we are closer to the beginning.
}

// Returns the size of the intset.
int intset_size(intset_t *this) {
	return this->size;
}
