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
extern void add_range_partition(Oid parent, RangePartitionInfo *rpinfo);
extern void split_range_partition(Oid relid, AlterTableCmd *cmd);
extern void rename_partition(Oid parent, AlterTableCmd *cmd);

// #endif /* PARTITION_H */
