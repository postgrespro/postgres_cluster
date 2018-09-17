/*-------------------------------------------------------------------------
 *
 * pgoutput.h
 *		Logical Replication output plugin
 *
 * Copyright (c) 2015-2018, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		pgoutput.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PGOUTPUT_H
#define PGOUTPUT_H

#include "nodes/pg_list.h"
#include "replication/logicalworker.h"

typedef struct PGOutputData
{
	MemoryContext context;		/* private memory context for transient
								 * allocations */

	/* client info */
	uint32		protocol_version;

	List	   *publication_names;
	List	   *publications;
	LogicalReplication2PCType twophase;
	bool		prepare_notifies;
} PGOutputData;

#endif							/* PGOUTPUT_H */
