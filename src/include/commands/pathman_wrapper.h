/*-------------------------------------------------------------------------
 *
 * pathman_wrapper.h
 *	  pg_pathman's functions wrappers
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "nodes/parsenodes.h"


typedef struct FuncArgs
{
	uint32	nargs;
	Oid	   *types;
	Datum  *values;
	char   *nulls;
} FuncArgs;

#define PG_SETARG_DATUM(args, n, type, value) \
	do { \
		(args)->types[(n)] = (type); \
		(args)->values[(n)] = (value); \
		(args)->nulls[(n)] = ' '; \
	} while(0)

#define PG_SETARG_NULL(args, n, type) \
	do { \
		(args)->types[(n)] = (type); \
		(args)->nulls[(n)] = 'n'; \
	} while(0)

void InitFuncArgs(FuncArgs *funcargs, uint32 size);
void FreeFuncArgs(FuncArgs *funcargs);

const char *get_pathman_schema_name();
Oid get_pathman_schema(void);

void pm_create_hash_partitions(Oid relid,
						  const char *attname,
						  uint32_t partitions_count);
void pm_create_range_partitions(Oid relid,
						const char *attname,
						Oid atttype,
						Datum interval,
						Oid interval_type);

void pm_add_range_partition(Oid relid,
					Oid type,
					const char *partition_name,
					Datum lower,
					Datum upper,
					bool lower_null,
					bool upper_null,
					const char *tablespace);
