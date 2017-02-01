/*-------------------------------------------------------------------------
 *
 * partitions.h
 *	  partitioning utilities
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */

#ifndef PARTITION_H
#define PARTITION_H

#include "postgres.h"
#include "nodes/parsenodes.h"

typedef enum PartitionDataType
{
	PDT_NONE = 0,
	PDT_REGULAR,
	PDT_CONCURRENT
} PartitionDataType;

extern void create_partitions(PartitionInfo *pinfo, Oid relid, PartitionDataType partition_data);
extern void merge_range_partitions(List *partitions, PartitionNode *into);
extern void add_range_partition(Oid parent, PartitionNode *rpinfo);
extern void split_range_partition(Oid relid, AlterTableCmd *cmd);
extern void rename_partition(Oid parent, AlterTableCmd *cmd);
extern void drop_partition(Oid parent, AlterTableCmd *cmd);
extern void move_partition(Oid parent, AlterTableCmd *cmd);
extern void partition_existing_table(Oid relid, AlterTableCmd *cmd);

#endif /* PARTITION_H */
