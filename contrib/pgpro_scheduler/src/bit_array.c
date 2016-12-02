#include <stdlib.h>
#include "bit_array.h"
#include "postgres.h"
#include "memutils.h"

bit_array_t *create_bit_array(int length)
{
	bit_array_t	*aa = worker_alloc(sizeof(bit_array_t));
	return init_bit_array(aa, length);
}

bit_array_t *init_bit_array(bit_array_t *aa, int length)
{
	int i;

	aa->step = sizeof(unsigned char) * 8;
	aa->length = length;
	aa->alen = length/aa->step;
	if(length%aa->step) aa->alen++;
	aa->array = worker_alloc(sizeof(unsigned char) * aa->alen);
	for(i=0; i < aa->alen; i++) aa->array[i] = 0;

	return aa;
}

void bit_array_put(bit_array_t *aa, int k, int val)
{
	val ? bit_array_set(aa, k): bit_array_clear(aa, k);
}

int bit_array_test(bit_array_t *aa, int k)
{
	if(k > aa->length) return 0;
	return ( (aa->array[k/aa->step] & (1 << (k%aa->step) )) != 0 );
}

void bit_array_set(bit_array_t *aa, int k)
{
	if(k <= aa->length)
	{
		aa->array[k/aa->step] |= 1 << (k%aa->step);
	}
}

void bit_array_clear(bit_array_t *aa, int k)
{
	if(k <= aa->length)
	{
		aa->array[k/aa->step] &= ~(1 << (k%aa->step));
	}
}

char *bit_array_string(bit_array_t *aa)
{
	static char str[BA_BUFFER_SIZE];
	int len = aa->length > BA_BUFFER_SIZE - 1? BA_BUFFER_SIZE - 1: aa->length;
	int i;

	for(i=0; i < len; i++)
	{
		str[i] = bit_array_test(aa, i) ? '1': '0';
	}
	str[i] = 0;

	return str;
}

void bit_array_zero(bit_array_t *aa)
{
	int i;

	for(i=0; i < aa->alen; i++) aa->array[i] = 0;
}

void destroy_bit_array(bit_array_t *aa, int self_destroy)
{
	pfree(aa->array);
	if(self_destroy) pfree(aa);
}

