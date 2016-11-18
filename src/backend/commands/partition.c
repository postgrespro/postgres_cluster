/*-------------------------------------------------------------------------
 *
 * partitions.c
 *	  partitioning utilities
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "commands/partition.h"
#include "executor/spi.h"
#include "catalog/pg_type.h"
#include "nodes/value.h"
#include "utils/builtins.h"


void
create_partitions(CreateStmt *stmt, Oid relid)
{
	PartitionInfo *pinfo = (PartitionInfo *) stmt->partition_info;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect using SPI");

	switch (pinfo->partition_type)
	{
		case PT_HASH:
			{
				// char *attname = ((ColumnRef *) pinfo->key)->fields;
				Value  *attname = (Value *) linitial(((ColumnRef *) pinfo->key)->fields);
				int		nargs = 3;
				Oid		types[3] = {OIDOID, TEXTOID, INT4OID};
				Datum	values[3] = {
					ObjectIdGetDatum(relid),
					CStringGetTextDatum(strVal(attname)),
					UInt32GetDatum(pinfo->partitions_count)};

				SPI_execute_with_args(
					"SELECT create_hash_partitions($1, $2, $3)",
					nargs, types, values, NULL, false, 0);
				break;
			}
		case PT_RANGE:
			// SPI_exec("SELECT create_range_partitions()")
			elog(ERROR, "RANGE doesn't implemented yet");
			break;
	}

	SPI_finish(); /* close SPI connection */
}