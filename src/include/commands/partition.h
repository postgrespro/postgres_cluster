/*-------------------------------------------------------------------------
 *
 * partitions.h
 *	  partitioning utilities
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */

// #ifndef PARTITION_H
// #define PARTITION_H

#include "postgres.h"
#include "nodes/parsenodes.h"

extern void create_partitions(CreateStmt *stmt, Oid relid);
extern void merge_range_partitions(List *partitions);

// #endif /* PARTITION_H */
