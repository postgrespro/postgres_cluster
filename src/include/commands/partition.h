/*-------------------------------------------------------------------------
 *
 * partitions.c
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

// #endif /* PARTITION_H */
