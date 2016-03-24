#ifndef BLOCKMEM_H
#define BLOCKMEM_H

#include <stdlib.h>

/*
 * Formats the memory region of 'size' bytes starting at 'origin'. Use this
 * before any other functions related to blockmem. Returns 1 on success,
 * 0 otherwise.
 */
int blockmem_format(void *origin, size_t size);

/*
 * Allocates blocks in the formatted region starting at 'origin' and stores
 * 'len' bytes from 'src' inside those blocks. Returns the id for later _get(),
 * _len() and _forget() operations. Returns 0 if not enough free blocks.
 */
int blockmem_put(void *origin, void *src, size_t len);

/*
 * Returns the length of data identified by 'id' that was previously stored
 * with a call to _put().
 */
size_t blockmem_len(void *origin, int id);

/*
 * Retrieves into 'dst' no more than 'len' bytes of data identified by 'id'
 * that were previously stored with a call to _put(). Returns the length of data
 * actually copied.
 */
size_t blockmem_get(void *origin, int id, void *dst, size_t len);

/*
 * Frees the blocks occupied by data identified by 'id'.
 */
void blockmem_forget(void *origin, int id);

#endif
