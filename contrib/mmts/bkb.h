/*
 * Bron–Kerbosch algorithm to find maximum clique in graph
 */  
#ifndef __BKB_H__
#define __BKB_H__

#define MAX_NODES 64

typedef long long long64; /* we are not using int64 here because we want to use %lld format for this type */
typedef unsigned long long ulong64; /* we are not using uint64 here because we want to use %lld format for this type */

typedef ulong64 nodemask_t;

#define BIT_CHECK(mask, bit) (((mask) & ((nodemask_t)1 << (bit))) != 0)
#define BIT_CLEAR(mask, bit) (mask &= ~((nodemask_t)1 << (bit)))
#define BIT_SET(mask, bit)   (mask |= ((nodemask_t)1 << (bit)))
#define ALL_BITS ((nodemask_t)~0)

extern nodemask_t MtmFindMaxClique(nodemask_t* matrix, int n_modes, int* clique_size);

#endif
