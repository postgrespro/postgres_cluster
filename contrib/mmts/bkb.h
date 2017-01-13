/*
 * Bron–Kerbosch algorithm to find maximum clique in graph
 */  
#ifndef __BKB_H__
#define __BKB_H__

#define MAX_NODES 64

typedef uint64_t nodemask_t;
#define BIT_CHECK(mask, bit) (((mask) & ((nodemask_t)1 << (bit))) != 0)
#define BIT_CLEAR(mask, bit) (mask &= ~((nodemask_t)1 << (bit)))
#define BIT_SET(mask, bit)   (mask |= ((nodemask_t)1 << (bit)))

extern nodemask_t MtmFindMaxClique(nodemask_t* matrix, int n_modes, int* clique_size);

#endif
