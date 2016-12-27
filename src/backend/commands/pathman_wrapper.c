/*-------------------------------------------------------------------------
 *
 * pathman_wrapper.c
 *	  pg_pathman's functions wrappers
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */

#include "commands/pathman_wrapper.h"

#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "catalog/pg_extension.h"
#include "catalog/indexing.h"
#include "commands/extension.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"


/*
 * Make pg_pathman's function call
 */
static bool
pathman_invoke(const char *func, const FuncArgs *args)
{
	char   *sql;
	int		ret;

	sql = psprintf("SELECT %s.%s",
				   get_pathman_schema_name(),
				   func);

	ret = SPI_execute_with_args(sql,
								args->nargs,
								args->types,
								args->values,
								args->nulls,
								false,
								0);
	return ret == SPI_OK_SELECT;
}

void
InitFuncArgs(FuncArgs *funcargs, uint32 size)
{
	funcargs->types = palloc(sizeof(Oid) * size);
	funcargs->values = palloc(sizeof(Datum) * size);
	funcargs->nulls = palloc(sizeof(char) * size);
	funcargs->nargs = size;

	/* Set all nulls by default */
	memset(funcargs->nulls, 'n', size);
}

void
FreeFuncArgs(FuncArgs *funcargs)
{
	pfree(funcargs->types);
	pfree(funcargs->values);
	pfree(funcargs->nulls);
	funcargs->nargs = 0;
}


/*
 * Returns pg_pathman schema's Oid or InvalidOid if that's not possible.
 */
Oid
get_pathman_schema(void)
{
	Oid				result;
	Relation		rel;
	SysScanDesc		scandesc;
	HeapTuple		tuple;
	ScanKeyData		entry[1];
	Oid				ext_schema;

	/* It's impossible to fetch pg_pathman's schema now */
	if (!IsTransactionState())
		return InvalidOid;

	ext_schema = get_extension_oid("pg_pathman", true);
	if (ext_schema == InvalidOid)
		return InvalidOid; /* exit if pg_pathman does not exist */

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(ext_schema));

	rel = heap_open(ExtensionRelationId, AccessShareLock);
	scandesc = systable_beginscan(rel, ExtensionOidIndexId, true,
								  NULL, 1, entry);

	tuple = systable_getnext(scandesc);

	/* We assume that there can be at most one matching tuple */
	if (HeapTupleIsValid(tuple))
		result = ((Form_pg_extension) GETSTRUCT(tuple))->extnamespace;
	else
		result = InvalidOid;

	systable_endscan(scandesc);

	heap_close(rel, AccessShareLock);

	return result;
}


const char *
get_pathman_schema_name()
{
	return get_namespace_name(get_pathman_schema());
}

/*
 */
void
pm_create_hash_partitions(Oid relid,
						  const char *attname,
						  uint32_t partitions_count)
{
	FuncArgs		args;
	bool			ret;

	InitFuncArgs(&args, 3);
	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));
	PG_SETARG_DATUM(&args, 1, TEXTOID, CStringGetTextDatum(attname));
	PG_SETARG_DATUM(&args, 2, INT4OID, ObjectIdGetDatum(UInt32GetDatum(partitions_count)));

	ret = pathman_invoke("create_hash_partitions($1, $2, $3)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Hash partitions creation failed");
}

/*
 *
 */
void
pm_create_range_partitions(Oid relid,
						const char *attname,
						Oid atttype,
						Datum interval,
						Oid interval_type)
{
	FuncArgs		args;
	int				ret;

	InitFuncArgs(&args, 5);
	PG_SETARG_DATUM(&args, 0, OIDOID, relid);
	PG_SETARG_DATUM(&args, 1, TEXTOID, PointerGetDatum(cstring_to_text(attname)));
	/* TODO: get value from first partition */
	PG_SETARG_DATUM(&args, 2, atttype, Int32GetDatum(0));
	PG_SETARG_DATUM(&args, 3, interval_type, interval);

	/* Zero partitions */
	PG_SETARG_DATUM(&args, 4, INT4OID, Int32GetDatum(0));

	ret = pathman_invoke("create_range_partitions($1, $2, $3, $4, $5)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Range partitions creation failed");
}


/*
 * Add range partition
 */
void
pm_add_range_partition(Oid relid,
					Oid type,
					const char *partition_name,
					Datum lower,
					Datum upper,
					bool lower_null,
					bool upper_null,
					const char *tablespace)
{
	FuncArgs	args;
	bool		ret;

	InitFuncArgs(&args, 5);

	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));

	/* Set lower bound */
	if (!lower_null)
		PG_SETARG_DATUM(&args, 1, type, lower);
	else
		PG_SETARG_NULL(&args, 1, type);

	/* Set upper bound */
	if (!upper_null)
		PG_SETARG_DATUM(&args, 2, type, upper);
	else
		PG_SETARG_NULL(&args, 2, type);

	/* Set partition name */
	if (partition_name)
	{
		PG_SETARG_DATUM(&args,
						3,
						TEXTOID,
						PointerGetDatum(cstring_to_text(partition_name)));
	}
	else
		PG_SETARG_NULL(&args, 3, TEXTOID);

	/* Set tablespace parameter */
	if (tablespace)
	{
		PG_SETARG_DATUM(&args,
						4,
						TEXTOID,
						PointerGetDatum(cstring_to_text(tablespace)));
	}
	else
		PG_SETARG_NULL(&args, 4, TEXTOID);

	/* Invoke pg_pathman's function */
	ret = pathman_invoke("add_range_partition($1, $2, $3, $4, $5)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Failed to add partition '%s'", partition_name);
}


/*
 * Merge partitions
 */
void
pm_merge_range_partitions(Oid relid1, Oid relid2)
{
	FuncArgs	args;
	bool		ret;

	InitFuncArgs(&args, 2);
	PG_SETARG_DATUM(&args, 0, OIDOID, relid1);
	PG_SETARG_DATUM(&args, 1, OIDOID, relid2);

	ret = pathman_invoke("merge_range_partitions($1, $2)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Unable to merge partitions");
}
