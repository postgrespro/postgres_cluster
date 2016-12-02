#ifndef PGPRO_SCHEDULER_BITARRAY_H
#define PGPRO_SCHEDULER_BITARRAY_H

#include "stdio.h"

#define BA_BUFFER_SIZE 1024

typedef struct {
	unsigned length;
	unsigned alen;
	unsigned step;
	unsigned char *array;
} bit_array_t;

bit_array_t *create_bit_array(int length);
bit_array_t *init_bit_array(bit_array_t *aa, int length);
int bit_array_test(bit_array_t *aa, int k);
void bit_array_set(bit_array_t *aa, int k);
void bit_array_clear(bit_array_t *aa, int k);
void bit_array_put(bit_array_t *aa, int k, int val);
char *bit_array_string(bit_array_t *aa);
void bit_array_zero(bit_array_t *aa);
void destroy_bit_array(bit_array_t *aa, int self_destroy);

#endif
