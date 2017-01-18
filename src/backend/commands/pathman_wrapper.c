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


static Datum construct_text_array(char **arr, int nelem);

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

static bool
pathman_invoke_return_value(const char *func,
							const FuncArgs *args,
							Datum *value,
							bool *isnull)
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

	if (SPI_processed != 1)
		elog(ERROR, "%s function failed", func);

	if (ret > 0 && SPI_tuptable != NULL)
	{
		TupleDesc	   tupdesc = SPI_tuptable->tupdesc;
		SPITupleTable *tuptable = SPI_tuptable;
		HeapTuple tuple = tuptable->vals[0];

		*value = SPI_getbinval(tuple, tupdesc, 1, isnull);
	}

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
	Oid schema_oid = get_pathman_schema();

	if (schema_oid == InvalidOid)
		elog(ERROR,
			 "pg_pathman module isn't installed");

	return get_namespace_name(schema_oid);
}

/* TODO: Probably remove this */
Oid
pm_get_attribute_type(Oid relid, const char *attname)
{
	FuncArgs		args;
	bool			isnull;
	Datum			atttype;
	bool			ret;

	InitFuncArgs(&args, 3);
	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));
	PG_SETARG_DATUM(&args, 1, TEXTOID, CStringGetTextDatum(attname));
	ret = pathman_invoke_return_value("get_attribute_type($1, $2)",
									  &args,
									  &atttype,
									  &isnull);

	if (!ret)
		elog(ERROR, "Cannot retrieve attribute type");

	FreeFuncArgs(&args);

	return !isnull ? DatumGetObjectId(atttype) : InvalidOid;
}

char *
pm_get_partition_key(Oid relid)
{
	FuncArgs		args;
	bool			isnull;
	Datum			attname;
	bool			ret;

	InitFuncArgs(&args, 1);
	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));
	ret = pathman_invoke_return_value("get_partition_key($1)",
									  &args,
									  &attname,
									  &isnull);

	if (!ret)
		elog(ERROR, "Cannot retrieve attribute type");

	if (isnull)
		elog(ERROR,
			 "Table '%s' isn't partitioned by pg_pathman",
			 get_rel_name(relid));

	FreeFuncArgs(&args);

	return TextDatumGetCString(attname);
}

Oid
pm_get_partition_key_type(Oid relid)
{
	FuncArgs		args;
	bool			isnull;
	Datum			atttype;
	bool			ret;

	InitFuncArgs(&args, 1);
	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));
	ret = pathman_invoke_return_value("get_partition_key_type($1)",
									  &args,
									  &atttype,
									  &isnull);

	if (!ret)
		elog(ERROR, "Cannot retrieve partition key type");

	FreeFuncArgs(&args);

	return !isnull ? DatumGetObjectId(atttype) : InvalidOid;
}

/*
 * TODO: add null parameters
 */
void pm_get_part_range(Oid relid, int partnum, Oid atttype, Datum *min, Datum *max)
{
	FuncArgs		args;
	Datum			arr_datum;
	bool			isnull;
	ArrayType	   *arr;
	bool			ret;

	/* deconstruct_array params */
	Datum		   *elems;
	int				nelems;
	bool		   *nulls;
	int16			typlen;
	bool			typbyval;
	char			typalign;

	InitFuncArgs(&args, 3);
	PG_SETARG_DATUM(&args, 0, OIDOID, relid);
	PG_SETARG_DATUM(&args, 1, INT4OID, partnum);
	PG_SETARG_NULL(&args, 2, atttype);
	ret = pathman_invoke_return_value("get_part_range($1, $2, $3)",
									  &args,
									  &arr_datum,
									  &isnull);

	if (!ret)
		elog(ERROR, "Cannot retrieve partition range");
	FreeFuncArgs(&args);

	/* Now we have datum. Let's extract array from it */
	arr = DatumGetArrayTypeP(arr_datum);
	get_typlenbyvalalign(atttype, &typlen, &typbyval, &typalign);
	deconstruct_array(arr, atttype, 
					  typlen, typbyval, typalign,
					  &elems, &nulls, &nelems);

	Assert(nelems == 2);
	*min = elems[0];
	*max = elems[1];
}

/*
 */
void
pm_create_hash_partitions(Oid relid,
						  const char *attname,
						  uint32_t partitions_count,
						  char **relnames,
						  char **tablespaces)
{
	FuncArgs		args;
	bool			ret;

	InitFuncArgs(&args, 5);
	PG_SETARG_DATUM(&args, 0, OIDOID, ObjectIdGetDatum(relid));
	PG_SETARG_DATUM(&args, 1, TEXTOID, CStringGetTextDatum(attname));
	PG_SETARG_DATUM(&args, 2, INT4OID, ObjectIdGetDatum(UInt32GetDatum(partitions_count)));
	PG_SETARG_DATUM(&args, 3, BOOLOID, BoolGetDatum(false));

	if (relnames != NULL)
		PG_SETARG_DATUM(&args, 4, TEXTARRAYOID,
						construct_text_array(relnames, partitions_count));
	else
		PG_SETARG_NULL(&args, 4, TEXTARRAYOID);

	ret = pathman_invoke("create_hash_partitions($1, $2, $3, $4, $5)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Hash partitions creation failed");
}


static Datum
construct_text_array(char **arr, int nelem)
{
	Datum	   *datums = palloc(sizeof(Datum) * nelem);
	int			i;
	int16		elemlen;
	bool		elembyval;
	char		elemalign;
	ArrayType  *at;

	for (i = 0; i < nelem; i++)
		datums[i] = CStringGetTextDatum(arr[i]);

	get_typlenbyvalalign(TEXTOID, &elemlen, &elembyval, &elemalign);

	at = construct_array(datums, nelem, TEXTOID,
						 elemlen, elembyval, elemalign);

	return PointerGetDatum(at);
}


/*
 *
 */
void
pm_create_range_partitions(Oid relid,
						const char *attname,
						Oid atttype,
						Datum interval,
						Oid interval_type,
						bool interval_isnull)
{
	FuncArgs		args;
	int				ret;

	InitFuncArgs(&args, 5);
	PG_SETARG_DATUM(&args, 0, OIDOID, relid);
	PG_SETARG_DATUM(&args, 1, TEXTOID, PointerGetDatum(cstring_to_text(attname)));
	/* TODO: get value from first partition */
	PG_SETARG_DATUM(&args, 2, atttype, Int32GetDatum(0));

	if (!interval_isnull)
		PG_SETARG_DATUM(&args, 3, interval_type, interval);
	else
		PG_SETARG_NULL(&args, 3, interval_type);

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

/*
 * Split partition
 */
void
pm_split_range_partition(Oid part_relid,
						 Datum split_value,
						 Oid split_value_type,
						 const char *relname,
						 const char *tablespace)
{
	FuncArgs	args;
	bool		ret;

	InitFuncArgs(&args, 4);
	PG_SETARG_DATUM(&args, 0, OIDOID, part_relid);
	PG_SETARG_DATUM(&args, 1, split_value_type, split_value);

	/* Set partition name if provided */
	if (relname)
		PG_SETARG_DATUM(&args, 2, TEXTOID, CStringGetTextDatum(relname));
	else
		PG_SETARG_NULL(&args, 2, TEXTOID);

	/* Set tablespace if provided */
	if (tablespace)
		PG_SETARG_DATUM(&args, 3, TEXTOID, CStringGetTextDatum(tablespace));
	else
		PG_SETARG_NULL(&args, 3, TEXTOID);

	ret = pathman_invoke("split_range_partition($1, $2, $3, $4)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Unable to split partition '%s'", get_rel_name(part_relid));
}

void pm_alter_partition(Oid relid,
						const char *new_relname,
						Oid new_namespace,
						const char *new_tablespace)
{
	FuncArgs	args;
	bool		ret;

	InitFuncArgs(&args, 4);
	PG_SETARG_DATUM(&args, 0, OIDOID, relid);

	/* Set name if needed */
	if (new_relname)
		PG_SETARG_DATUM(&args, 1, TEXTOID, CStringGetTextDatum(new_relname));
	else
		PG_SETARG_NULL(&args, 1, TEXTOID);

	/* Set schema */
	if (new_namespace != InvalidOid)
		PG_SETARG_DATUM(&args, 2, OIDOID, new_namespace);
	else
		PG_SETARG_NULL(&args, 2, OIDOID);

	/* Set tablespace */
	if (new_tablespace != NULL)
		PG_SETARG_DATUM(&args, 3, TEXTOID, CStringGetTextDatum(new_tablespace));
	else
		PG_SETARG_NULL(&args, 3, TEXTOID);

	ret = pathman_invoke("alter_partition($1, $2, $3, $4)", &args);
	FreeFuncArgs(&args);

	if (!ret)
		elog(ERROR, "Unable to alter partition '%s'", get_rel_name(relid));
}
