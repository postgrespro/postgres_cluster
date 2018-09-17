/*-------------------------------------------------------------------------
 *
 * logicalworker.h
 *	  Exports for logical replication workers.
 *
 * Portions Copyright (c) 2016-2018, PostgreSQL Global Development Group
 *
 * src/include/replication/logicalworker.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef LOGICALWORKER_H
#define LOGICALWORKER_H

extern void ApplyWorkerMain(Datum main_arg);

extern bool IsLogicalWorker(void);

/* GUC */
typedef enum LogicalReplication2PCType
{
	LOGICAL_REPLICATION_2PC_OFF, /* Never do 2PC */
	LOGICAL_REPLICATION_2PC_SHARDMAN, /* 2PC only xacts with shardman gid */
	LOGICAL_REPLICATION_2PC_ALWAYS /* Always do 2PC */
} LogicalReplication2PCType;
extern int logical_replication_2pc;

#endif							/* LOGICALWORKER_H */
