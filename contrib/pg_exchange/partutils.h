/*
 * partutils.h
 *
 */

#ifndef PARTUTILS_H_
#define PARTUTILS_H_

#include "postgres.h"

#include "nodes/relation.h"

extern bool build_joinrel_partition_info(RelOptInfo *joinrel,
										 RelOptInfo *outer_rel,
										 RelOptInfo *inner_rel,
										 List *restrictlist,
										 JoinType jointype);

#endif /* PARTUTILS_H_ */
