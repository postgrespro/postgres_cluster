#include <stdio.h>
#include <stdlib.h>
#include "postgres.h"
#include "char_array.h"
#include "string.h"
#include "memutils.h"

int __sort_char_string(const void *a, const void *b)
{
	const char *A = *(char * const *)a;
	const char *B = *(char * const *)b;

	return strcmp(B, A);
}


char_array_t *makeCharArray(void)
{
	char_array_t *a = worker_alloc(sizeof(char_array_t));
	a->n = 0;
	a->data = NULL;

	return a;
}

char_array_t *sortCharArray(char_array_t *a)
{
	qsort(a->data, a->n, sizeof(char *), __sort_char_string);

	return a;
}

char_array_t *pushCharArray(char_array_t *a, const char *str)
{
	int len = strlen(str);

	a->data = a->data ? (char **)repalloc(a->data, sizeof(char *) * (a->n+1)): worker_alloc(sizeof(char *));
	a->data[a->n] = worker_alloc(sizeof(char) * (len + 1));
	memcpy(a->data[a->n], str, len + 1);

	a->n++;
	return a;
}

void destroyCharArray(char_array_t *a)
{
	int i;

	if(a->n && a->data)
	{
		for(i=0; i < a->n; i++)
		{
			pfree(a->data[i]);
		}
	}
	if(a->data) pfree(a->data);
    pfree(a);
}
