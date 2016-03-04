/*-------------------------------------------------------------------------
 *
 * pglogical_proto.h
 *		pglogical protocol
 *
 * Copyright (c) 2015, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  pglogical_proto.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PGLOGICAL_PROTO_H
#define PGLOGICAL_PROTO_H

#include "utils/timestamp.h"

#include "pglogical_relcache.h"

typedef struct PGLogicalTupleData
{
	Datum	values[MaxTupleAttributeNumber];
	bool	nulls[MaxTupleAttributeNumber];
	bool	changed[MaxTupleAttributeNumber];
} PGLogicalTupleData;

#define PGLOGICAL_COMMIT			0x00
#define PGLOGICAL_PREPARE			0x01
#define PGLOGICAL_COMMIT_PREPARED	0x02
#define PGLOGICAL_ABORT_PREPARED	0x03

#define PGLOGICAL_XACT_EVENT(flags)	(flags & 0x3)

extern void pglogical_read_begin(StringInfo in, XLogRecPtr *remote_lsn,
					  TimestampTz *committime, TransactionId *remote_xid);
extern void pglogical_read_commit(StringInfo in, XLogRecPtr *commit_lsn,
					   XLogRecPtr *end_lsn, TimestampTz *committime,
					   uint8 *flags, const char **gid);
extern char *pglogical_read_origin(StringInfo in, XLogRecPtr *origin_lsn);

extern uint32 pglogical_read_rel(StringInfo in);

extern PGLogicalRelation *pglogical_read_insert(StringInfo in, LOCKMODE lockmode,
					   PGLogicalTupleData *newtup);
extern PGLogicalRelation *pglogical_read_update(StringInfo in, LOCKMODE lockmode, bool *hasoldtup,
					   PGLogicalTupleData *oldtup, PGLogicalTupleData *newtup);
extern PGLogicalRelation *pglogical_read_delete(StringInfo in, LOCKMODE lockmode,
												 PGLogicalTupleData *oldtup);

#endif /* PGLOGICAL_PROTO_H */
