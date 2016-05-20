/*-------------------------------------------------------------------------
 *
 * pg_variables.c
 *	  Functions, which get or set variables values
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "parser/scansup.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"
#include "utils/typcache.h"

#include "pg_variables.h"

PG_MODULE_MAGIC;

/* Scalar variables functions */
PG_FUNCTION_INFO_V1(variable_set_int);
PG_FUNCTION_INFO_V1(variable_get_int);
PG_FUNCTION_INFO_V1(variable_set_text);
PG_FUNCTION_INFO_V1(variable_get_text);
PG_FUNCTION_INFO_V1(variable_set_numeric);
PG_FUNCTION_INFO_V1(variable_get_numeric);
PG_FUNCTION_INFO_V1(variable_set_timestamp);
PG_FUNCTION_INFO_V1(variable_get_timestamp);
PG_FUNCTION_INFO_V1(variable_set_timestamptz);
PG_FUNCTION_INFO_V1(variable_get_timestamptz);
PG_FUNCTION_INFO_V1(variable_set_date);
PG_FUNCTION_INFO_V1(variable_get_date);
PG_FUNCTION_INFO_V1(variable_set_jsonb);
PG_FUNCTION_INFO_V1(variable_get_jsonb);

/* Functions to work with records */
PG_FUNCTION_INFO_V1(variable_insert);
PG_FUNCTION_INFO_V1(variable_update);
PG_FUNCTION_INFO_V1(variable_delete);

PG_FUNCTION_INFO_V1(variable_select);
PG_FUNCTION_INFO_V1(variable_select_by_value);
PG_FUNCTION_INFO_V1(variable_select_by_values);

/* Functions to work with packages */
PG_FUNCTION_INFO_V1(variable_exists);
PG_FUNCTION_INFO_V1(remove_variable);
PG_FUNCTION_INFO_V1(remove_package);
PG_FUNCTION_INFO_V1(remove_packages);
PG_FUNCTION_INFO_V1(get_packages_and_variables);
PG_FUNCTION_INFO_V1(get_packages_stats);

static void getKeyFromName(text *name, char *key);
static void ensurePackagesHashExists();
static HashPackageEntry *getPackageByName(text *name, bool create, bool strict);

static HashVariableEntry *getVariableByNameWithType(HTAB *variables,
													text *name,
													Oid typid,
													bool create,
													bool strict);

#define CHECK_ARGS_FOR_NULL() \
do { \
	if (fcinfo->argnull[0]) \
		ereport(ERROR, \
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE), \
				 errmsg("package name can not be NULL"))); \
	if (fcinfo->argnull[1]) \
		ereport(ERROR, \
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE), \
				 errmsg("variable name can not be NULL"))); \
} while(0)

static HTAB *packagesHash = NULL;
static MemoryContext ModuleContext = NULL;

/* Recent package */
static HashPackageEntry *LastPackage = NULL;
/* Recent variable */
static HashVariableEntry *LastVariable = NULL;

/*
 * Set value of variable, typlen could be 0 if typbyval == true
 */
static void
variable_set(text *package_name, text *var_name,
			 Oid typid, Datum value, bool is_null)
{
	HashPackageEntry   *package;
	HashVariableEntry  *variable;
	ScalarVar		   *scalar;

	package = getPackageByName(package_name, true, false);
	variable = getVariableByNameWithType(package->variablesHash,
										 var_name, typid, true, false);

	scalar = &variable->value.scalar;

	/* Release memory for variable */
	if (scalar->typbyval == false && scalar->is_null == false)
		pfree(DatumGetPointer(scalar->value));

	scalar->is_null = is_null;
	if (!scalar->is_null)
	{
		MemoryContext	oldcxt;
		oldcxt = MemoryContextSwitchTo(package->hctx);

		scalar->value = datumCopy(value, scalar->typbyval, scalar->typlen);

		MemoryContextSwitchTo(oldcxt);
	}
	else
		scalar->value = 0;
}

static Datum
variable_get(text *package_name, text *var_name,
			 Oid typid, bool *is_null, bool strict)
{
	HashPackageEntry   *package;
	HashVariableEntry  *variable;
	ScalarVar		   *scalar;

	package = getPackageByName(package_name, false, strict);
	if (package == NULL)
	{
		*is_null = true;
		return 0;
	}

	variable = getVariableByNameWithType(package->variablesHash,
										 var_name, typid, false, strict);

	if (variable == NULL)
	{
		*is_null = true;
		return 0;
	}

	scalar = &(variable->value.scalar);

	*is_null = scalar->is_null;
	return scalar->value;
}

Datum
variable_set_int(PG_FUNCTION_ARGS)
{
	text	*package_name;
	text	*var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, INT4OID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_int(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 INT4OID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_text(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, TEXTOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_text(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 TEXTOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_numeric(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, NUMERICOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_numeric(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 NUMERICOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_timestamp(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, TIMESTAMPOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_timestamp(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 TIMESTAMPOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_timestamptz(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, TIMESTAMPTZOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_timestamptz(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 TIMESTAMPTZOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_date(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, DATEOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_date(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 DATEOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_set_jsonb(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	variable_set(package_name, var_name, JSONBOID,
				 PG_ARGISNULL(2) ? 0 : PG_GETARG_DATUM(2),
				 PG_ARGISNULL(2));

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	PG_RETURN_VOID();
}

Datum
variable_get_jsonb(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	bool		strict;
	bool		is_null;
	Datum		value;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	strict = PG_GETARG_BOOL(2);

	value = variable_get(package_name, var_name,
						 JSONBOID, &is_null, strict);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);
	if (!is_null)
		PG_RETURN_DATUM(value);
	else
		PG_RETURN_NULL();
}

Datum
variable_insert(PG_FUNCTION_ARGS)
{
	text			   *package_name;
	text			   *var_name;
	HeapTupleHeader		rec;
	HashPackageEntry   *package;
	HashVariableEntry  *variable;

	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;

	/* Checks */
	CHECK_ARGS_FOR_NULL();

	if (PG_ARGISNULL(2))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("record argument can not be NULL")));

	/* Get arguments */
	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	rec = PG_GETARG_HEAPTUPLEHEADER(2);

	/* Get cached package */
	if (LastPackage == NULL ||
		VARSIZE_ANY_EXHDR(package_name) != strlen(LastPackage->name) ||
		strncmp(VARDATA_ANY(package_name), LastPackage->name,
				VARSIZE_ANY_EXHDR(package_name)) != 0)
	{
		package = getPackageByName(package_name, true, false);
		LastPackage = package;
		LastVariable = NULL;
	}
	else
		package = LastPackage;

	/* Get cached variable */
	if (LastVariable == NULL ||
		VARSIZE_ANY_EXHDR(var_name) != strlen(LastVariable->name) ||
		strncmp(VARDATA_ANY(var_name), LastVariable->name,
				VARSIZE_ANY_EXHDR(var_name)) != 0)
	{
		variable = getVariableByNameWithType(package->variablesHash,
											 var_name, RECORDOID, true, false);
		LastVariable = variable;
	}
	else
		variable = LastVariable;

	/* Insert a record */
	tupType = HeapTupleHeaderGetTypeId(rec);
	tupTypmod = HeapTupleHeaderGetTypMod(rec);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	if (!variable->value.record.tupdesc)
	{
		/*
		 * This is the first record for the var_name. Initialize attributes.
		 */
		init_attributes(variable, tupdesc, package->hctx);
	}
	else
		check_attributes(variable, tupdesc);

	insert_record(variable, rec);

	/* Release resources */
	ReleaseTupleDesc(tupdesc);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	PG_RETURN_VOID();
}

Datum
variable_update(PG_FUNCTION_ARGS)
{
	text			   *package_name;
	text			   *var_name;
	HeapTupleHeader		rec;
	HashPackageEntry   *package;
	HashVariableEntry  *variable;
	bool				res;

	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;

	/* Checks */
	CHECK_ARGS_FOR_NULL();

	if (PG_ARGISNULL(2))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("record argument can not be NULL")));

	/* Get arguments */
	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);
	rec = PG_GETARG_HEAPTUPLEHEADER(2);

	/* Get cached package */
	if (LastPackage == NULL ||
		VARSIZE_ANY_EXHDR(package_name) != strlen(LastPackage->name) ||
		strncmp(VARDATA_ANY(package_name), LastPackage->name,
				VARSIZE_ANY_EXHDR(package_name)) != 0)
	{
		package = getPackageByName(package_name, false, true);
		LastPackage = package;
		LastVariable = NULL;
	}
	else
		package = LastPackage;

	/* Get cached variable */
	if (LastVariable == NULL ||
		VARSIZE_ANY_EXHDR(var_name) != strlen(LastVariable->name) ||
		strncmp(VARDATA_ANY(var_name), LastVariable->name,
				VARSIZE_ANY_EXHDR(var_name)) != 0)
	{
		variable = getVariableByNameWithType(package->variablesHash,
											 var_name, RECORDOID, false, true);
		LastVariable = variable;
	}
	else
		variable = LastVariable;

	/* Update a record */
	tupType = HeapTupleHeaderGetTypeId(rec);
	tupTypmod = HeapTupleHeaderGetTypMod(rec);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	check_attributes(variable, tupdesc);
	res = update_record(variable, rec);

	/* Release resources */
	ReleaseTupleDesc(tupdesc);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	PG_RETURN_BOOL(res);
}

Datum
variable_delete(PG_FUNCTION_ARGS)
{
	text			   *package_name;
	text			   *var_name;
	Oid					value_type;
	Datum				value;
	bool				value_is_null = PG_ARGISNULL(2);
	HashPackageEntry   *package;
	HashVariableEntry  *variable;
	bool				res;

	CHECK_ARGS_FOR_NULL();

	/* Get arguments */
	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	if (!value_is_null)
	{
		value_type = get_fn_expr_argtype(fcinfo->flinfo, 2);
		value = PG_GETARG_DATUM(2);
	}
	else
	{
		value_type = InvalidOid;
		value = 0;
	}

	/* Get cached package */
	if (LastPackage == NULL ||
		VARSIZE_ANY_EXHDR(package_name) != strlen(LastPackage->name) ||
		strncmp(VARDATA_ANY(package_name), LastPackage->name,
				VARSIZE_ANY_EXHDR(package_name)) != 0)
	{
		package = getPackageByName(package_name, false, true);
		LastPackage = package;
		LastVariable = NULL;
	}
	else
		package = LastPackage;

	/* Get cached variable */
	if (LastVariable == NULL ||
		VARSIZE_ANY_EXHDR(var_name) != strlen(LastVariable->name) ||
		strncmp(VARDATA_ANY(var_name), LastVariable->name,
				VARSIZE_ANY_EXHDR(var_name)) != 0)
	{
		variable = getVariableByNameWithType(package->variablesHash, var_name,
											 RECORDOID, false, true);
		LastVariable = variable;
	}
	else
		variable = LastVariable;

	/* Delete a record */
	if (!value_is_null)
		check_record_key(variable, value_type);
	res = delete_record(variable, value, value_is_null);

	/* Release resources */
	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	PG_RETURN_BOOL(res);
}

Datum
variable_select(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	HASH_SEQ_STATUS	   *rstat;
	HashRecordEntry	   *item;

	if (SRF_IS_FIRSTCALL())
	{
		text		   *package_name;
		text		   *var_name;
		HashPackageEntry   *package;
		HashVariableEntry  *variable;
		MemoryContext		oldcontext;
		RecordVar		   *record;

		CHECK_ARGS_FOR_NULL();

		/* Get arguments */
		package_name = PG_GETARG_TEXT_PP(0);
		var_name = PG_GETARG_TEXT_PP(1);

		package = getPackageByName(package_name, false, true);
		variable = getVariableByNameWithType(package->variablesHash,
											 var_name, RECORDOID, false, true);

		record = &(variable->value.record);

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		funcctx->tuple_desc = CreateTupleDescCopy(record->tupdesc);

		rstat = (HASH_SEQ_STATUS *) palloc0(sizeof(HASH_SEQ_STATUS));
		hash_seq_init(rstat, record->rhash);
		funcctx->user_fctx = rstat;

		MemoryContextSwitchTo(oldcontext);
		PG_FREE_IF_COPY(package_name, 0);
		PG_FREE_IF_COPY(var_name, 1);
	}

	funcctx = SRF_PERCALL_SETUP();

	/* Get next hash record */
	rstat = (HASH_SEQ_STATUS *) funcctx->user_fctx;
	item = (HashRecordEntry *) hash_seq_search(rstat);
	if (item != NULL)
	{
		Datum		result;

		result = HeapTupleGetDatum(item->tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		pfree(rstat);
		SRF_RETURN_DONE(funcctx);
	}
}

Datum
variable_select_by_value(PG_FUNCTION_ARGS)
{
	text		   *package_name;
	text		   *var_name;
	Oid				value_type;
	Datum			value;
	bool			value_is_null = PG_ARGISNULL(2);
	HashPackageEntry   *package;
	HashVariableEntry  *variable;

	HashRecordEntry	   *item;
	RecordVar		   *record;
	HashRecordKey		k;
	bool				found;

	CHECK_ARGS_FOR_NULL();

	/* Get arguments */
	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	if (!value_is_null)
	{
		value_type = get_fn_expr_argtype(fcinfo->flinfo, 2);
		value = PG_GETARG_DATUM(2);
	}
	else
	{
		value_type = InvalidOid;
		value = 0;
	}

	package = getPackageByName(package_name, false, true);
	variable = getVariableByNameWithType(package->variablesHash,
										 var_name, RECORDOID, false, true);

	if (!value_is_null)
		check_record_key(variable, value_type);

	record = &(variable->value.record);

	/* Search a record */
	k.value = value;
	k.is_null = value_is_null;
	k.hash_proc = &record->hash_proc;
	k.cmp_proc = &record->cmp_proc;

	item = (HashRecordEntry *) hash_search(record->rhash, &k,
											HASH_FIND, &found);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	if (found)
		PG_RETURN_DATUM(HeapTupleGetDatum(item->tuple));
	else
		PG_RETURN_NULL();
}

/* Structure for variable_select_by_values() */
typedef struct
{
	HashVariableEntry  *variable;
	ArrayIterator		iterator;
} VariableIteratorRec;

Datum
variable_select_by_values(PG_FUNCTION_ARGS)
{
	FuncCallContext		   *funcctx;
	VariableIteratorRec	   *var;
	Datum					value;
	HashRecordEntry		   *item;
	bool					isnull;

	if (SRF_IS_FIRSTCALL())
	{
		text		   *package_name;
		text		   *var_name;
		ArrayType	   *values;
		HashPackageEntry   *package;
		HashVariableEntry  *variable;
		MemoryContext		oldcontext;

		/* Checks */
		CHECK_ARGS_FOR_NULL();

		if (PG_ARGISNULL(2))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("array argument can not be NULL")));

		values = PG_GETARG_ARRAYTYPE_P(2);
		if (ARR_NDIM(values) > 1)
			ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("searching for elements in multidimensional arrays is not supported")));

		/* Get arguments */
		package_name = PG_GETARG_TEXT_PP(0);
		var_name = PG_GETARG_TEXT_PP(1);

		package = getPackageByName(package_name, false, true);
		variable = getVariableByNameWithType(package->variablesHash,
											 var_name, RECORDOID, false, true);

		check_record_key(variable, ARR_ELEMTYPE(values));

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		funcctx->tuple_desc = CreateTupleDescCopy(variable->value.record.tupdesc);

		var = (VariableIteratorRec *) palloc(sizeof(VariableIteratorRec));
		var->iterator = array_create_iterator(values, 0, NULL);
		var->variable = variable;
		funcctx->user_fctx = var;

		MemoryContextSwitchTo(oldcontext);
		PG_FREE_IF_COPY(package_name, 0);
		PG_FREE_IF_COPY(var_name, 1);
	}

	funcctx = SRF_PERCALL_SETUP();
	var = (VariableIteratorRec *) funcctx->user_fctx;

	/* Get next array element */
	while (array_iterate(var->iterator, &value, &isnull))
	{
		HashRecordKey	k;
		bool			found;
		RecordVar	   *record;

		record = &(var->variable->value.record);
		/* Search a record */
		k.value = value;
		k.is_null = isnull;
		k.hash_proc = &record->hash_proc;
		k.cmp_proc = &record->cmp_proc;

		item = (HashRecordEntry *) hash_search(record->rhash, &k,
											  HASH_FIND, &found);
		if (found)
		{
			Datum		result;

			result = HeapTupleGetDatum(item->tuple);

			SRF_RETURN_NEXT(funcctx, result);
		}
	}

	array_free_iterator(var->iterator);
	pfree(var);
	SRF_RETURN_DONE(funcctx);
}

static void
clean_variable(HashVariableEntry *variable)
{
	if (variable->typid == RECORDOID)
		clean_records(variable);
	else if (variable->value.scalar.typbyval == false &&
			 variable->value.scalar.is_null == false)
		pfree(DatumGetPointer(variable->value.scalar.value));
}

/*
 * Check if variable exists.
 */
Datum
variable_exists(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	HashPackageEntry   *package;
	char				key[NAMEDATALEN];
	bool				found;

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	package = getPackageByName(package_name, false, false);
	if (package == NULL)
	{
		PG_FREE_IF_COPY(package_name, 0);
		PG_FREE_IF_COPY(var_name, 1);

		PG_RETURN_BOOL(false);
	}

	getKeyFromName(var_name, key);

	hash_search(package->variablesHash, key, HASH_FIND, &found);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	PG_RETURN_BOOL(found);
}

/*
 * Remove variable from package by name.
 */
Datum
remove_variable(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	text	   *var_name;
	HashPackageEntry   *package;
	HashVariableEntry  *variable;
	bool				found;
	char				key[NAMEDATALEN];

	CHECK_ARGS_FOR_NULL();

	package_name = PG_GETARG_TEXT_PP(0);
	var_name = PG_GETARG_TEXT_PP(1);

	package = getPackageByName(package_name, false, true);
	getKeyFromName(var_name, key);

	variable = (HashVariableEntry *) hash_search(package->variablesHash,
												 key, HASH_REMOVE, &found);
	if (!found)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unrecognized variable \"%s\"", key)));

	/* Remove variable from cache */
	LastVariable = NULL;

	clean_variable(variable);

	PG_FREE_IF_COPY(package_name, 0);
	PG_FREE_IF_COPY(var_name, 1);

	PG_RETURN_VOID();
}

/*
 * Remove package by name.
 */
Datum
remove_package(PG_FUNCTION_ARGS)
{
	text	   *package_name;
	HashPackageEntry   *package;
	bool				found;
	char				key[NAMEDATALEN];

	if (PG_ARGISNULL(0))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("package name can not be NULL")));

	package_name = PG_GETARG_TEXT_PP(0);
	getKeyFromName(package_name, key);

	if (!packagesHash)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unrecognized package \"%s\"", key)));

	package = (HashPackageEntry *) hash_search(packagesHash, key,
											   HASH_REMOVE, &found);
	if (!found)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unrecognized package \"%s\"", key)));

	/* Remove package and variable from cache */
	LastPackage = NULL;
	LastVariable = NULL;

	hash_destroy(package->variablesHash);
	/* All variables will be freed */
	MemoryContextDelete(package->hctx);

	PG_FREE_IF_COPY(package_name, 0);

	PG_RETURN_VOID();
}

/*
 * Remove all packages and variables.
 */
Datum
remove_packages(PG_FUNCTION_ARGS)
{
	/* There is not any packages and variables */
	if (packagesHash == NULL)
		PG_RETURN_VOID();

	/* Remove package and variable from cache */
	LastPackage = NULL;
	LastVariable = NULL;

	hash_destroy(packagesHash);
	/* All packages and variables will be freed */
	MemoryContextDelete(ModuleContext);

	packagesHash = NULL;
	ModuleContext = NULL;

	PG_RETURN_VOID();
}

/*
 * Structure for get_packages_and_variables().
 */
typedef struct
{
	char	*package;
	char	*variable;
} VariableRec;

/*
 * Get list of assigned packages and variables.
 */
Datum
get_packages_and_variables(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	VariableRec		   *recs;
	MemoryContext		oldcontext;

	if (SRF_IS_FIRSTCALL())
	{
		TupleDesc		tupdesc;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("function returning record called in context "
							"that cannot accept type record")));

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		/*
		 * Get all packages and variables names and save them into
		 * funcctx->user_fctx.
		 */
		if (packagesHash)
		{
			HashPackageEntry   *package;
			HASH_SEQ_STATUS		pstat;
			int					mRecs = NUMVARIABLES,
								nRecs = 0;

			recs = (VariableRec *) palloc0(sizeof(VariableRec) * mRecs);

			/* Get packages list */
			hash_seq_init(&pstat, packagesHash);
			while ((package =
				(HashPackageEntry *) hash_seq_search(&pstat)) != NULL)
			{
				HashVariableEntry  *variable;
				HASH_SEQ_STATUS		vstat;

				/* Get variables list for package */
				hash_seq_init(&vstat, package->variablesHash);
				while ((variable =
					(HashVariableEntry *) hash_seq_search(&vstat)) != NULL)
				{
					/* Resize recs if necessary */
					if (nRecs >= mRecs)
					{
						mRecs *= 2;
						recs = (VariableRec *) repalloc(recs,
												sizeof(VariableRec) * mRecs);
					}

					recs[nRecs].package = package->name;
					recs[nRecs].variable = variable->name;
					nRecs++;
				}
			}

			funcctx->user_fctx = recs;
			funcctx->max_calls = nRecs;
		}
		else
			funcctx->max_calls = 0;

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();

	/* Get the saved recs */
	recs = (VariableRec *) funcctx->user_fctx;

	if (funcctx->call_cntr < funcctx->max_calls)
	{
		Datum		values[2];
		bool		nulls[2];
		HeapTuple	tuple;
		Datum		result;
		int			i = funcctx->call_cntr;

		memset(nulls, 0, sizeof(nulls));

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		values[0] = PointerGetDatum(cstring_to_text(recs[i].package));
		values[1] = PointerGetDatum(cstring_to_text(recs[i].variable));

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
		result = HeapTupleGetDatum(tuple);

		MemoryContextSwitchTo(oldcontext);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else
		SRF_RETURN_DONE(funcctx);
}

static void
getMemoryTotalSpace(MemoryContext context, int level, Size *totalspace)
{
#if PG_VERSION_NUM >= 90600
	MemoryContext child;
	MemoryContextCounters totals;

	AssertArg(MemoryContextIsValid(context));

	/* Examine the context itself */
	memset(&totals, 0, sizeof(totals));
	(*context->methods->stats) (context, level, false, &totals);
	*totalspace += totals.totalspace;

	/*
	 * Examine children.
	 */
	for (child = context->firstchild; child != NULL; child = child->nextchild)
		getMemoryTotalSpace(child, level + 1, totalspace);
#else
	*totalspace = 0;
#endif
}

/*
 * Get list of assigned packages and used memory in bytes.
 */
Datum
get_packages_stats(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	MemoryContext		oldcontext;
	HASH_SEQ_STATUS	   *pstat;
	HashPackageEntry   *package;

	if (SRF_IS_FIRSTCALL())
	{
		TupleDesc		tupdesc;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("function returning record called in context "
							"that cannot accept type record")));

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		/*
		 * Get all packages and variables names and save them into
		 * funcctx->user_fctx.
		 */
		if (packagesHash)
		{
			pstat = (HASH_SEQ_STATUS *) palloc0(sizeof(HASH_SEQ_STATUS));
			/* Get packages list */
			hash_seq_init(pstat, packagesHash);

			funcctx->user_fctx = pstat;
		}
		else
			funcctx->user_fctx = NULL;

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	if (funcctx->user_fctx == NULL)
		SRF_RETURN_DONE(funcctx);

	/* Get packages list */
	pstat = (HASH_SEQ_STATUS *) funcctx->user_fctx;

	package = (HashPackageEntry *) hash_seq_search(pstat);
	if (package != NULL)
	{
		Datum		values[2];
		bool		nulls[2];
		HeapTuple	tuple;
		Datum		result;
		Size		totalspace = 0;

		memset(nulls, 0, sizeof(nulls));

		/* Fill data */
		values[0] = PointerGetDatum(cstring_to_text(package->name));

		getMemoryTotalSpace(package->hctx, 0, &totalspace);
		values[1] = Int64GetDatum(totalspace);

		/* Data are ready */
		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		pfree(pstat);
		SRF_RETURN_DONE(funcctx);
	}
}

/*
 * Static functions
 */

static void
getKeyFromName(text *name, char *key)
{
	int		key_len = VARSIZE_ANY_EXHDR(name);

	if (key_len >= NAMEDATALEN - 1)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("name \"%.*s\" is too long",
					   key_len, VARDATA_ANY(name))));

	strncpy(key, VARDATA_ANY(name), key_len);
	key[key_len] = '\0';
}

static void
ensurePackagesHashExists()
{
	HASHCTL		ctl;

	if (packagesHash)
		return;

	ModuleContext = AllocSetContextCreate(CacheMemoryContext,
										  "pg_variables memory context",
										  ALLOCSET_DEFAULT_MINSIZE,
										  ALLOCSET_DEFAULT_INITSIZE,
										  ALLOCSET_DEFAULT_MAXSIZE);

	ctl.keysize = NAMEDATALEN;
	ctl.entrysize = sizeof(HashPackageEntry);
	ctl.hcxt = ModuleContext;

	packagesHash = hash_create("Packages hash",
							   NUMPACKAGES, &ctl, HASH_ELEM | HASH_CONTEXT);
}

static HashPackageEntry *
getPackageByName(text* name, bool create, bool strict)
{
	HashPackageEntry   *package;
	char				key[NAMEDATALEN];
	bool				found;

	getKeyFromName(name, key);

	if (create)
		ensurePackagesHashExists();
	else
	{
		if (!packagesHash)
		{
			if (strict)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("unrecognized package \"%s\"", key)));
			else
				return NULL;
		}
	}

	if (create)
		package = (HashPackageEntry *) hash_search(packagesHash,
												   key, HASH_ENTER, &found);
	else
		package = (HashPackageEntry *) hash_search(packagesHash,
												   key, HASH_FIND, &found);

	/* Package entry was created, so we need create hash table for variables. */
	if (!found)
	{
		if (create)
		{
			HASHCTL			ctl;
			char			hash_name[BUFSIZ];
			MemoryContext	oldcxt;

			sprintf(hash_name, "Variables hash for package \"%s\"", key);

			package->hctx = AllocSetContextCreate(ModuleContext,
												  hash_name,
												  ALLOCSET_DEFAULT_MINSIZE,
												  ALLOCSET_DEFAULT_INITSIZE,
												  ALLOCSET_DEFAULT_MAXSIZE);
			oldcxt = MemoryContextSwitchTo(package->hctx);

			ctl.keysize = NAMEDATALEN;
			ctl.entrysize = sizeof(HashVariableEntry);
			ctl.hcxt = package->hctx;
			package->variablesHash = hash_create(hash_name,
												 NUMVARIABLES, &ctl,
												 HASH_ELEM | HASH_CONTEXT);

			MemoryContextSwitchTo(oldcxt);
		}
		else if (strict)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("unrecognized package \"%s\"", key)));
	}

	return package;
}

static HashVariableEntry *
getVariableByNameWithType(HTAB *variables, text *name, Oid typid,
						  bool create, bool strict)
{
	HashVariableEntry  *variable;
	char				key[NAMEDATALEN];
	bool				found;

	getKeyFromName(name, key);

	if (create)
		variable = (HashVariableEntry *) hash_search(variables,
													 key, HASH_ENTER, &found);
	else
		variable = (HashVariableEntry *) hash_search(variables,
													 key, HASH_FIND, &found);

	/* Check variable type */
	if (found)
	{
		if (variable->typid != typid)
		{
			char   *var_type = DatumGetCString(DirectFunctionCall1(
								regtypeout, ObjectIdGetDatum(variable->typid)));

			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("variable \"%s\" requires \"%s\" value",
							key, var_type)));
		}
	}
	else
	{
		/* Variable entry was created, so initialize new variable. */
		if (variable)
		{
			memset(&variable->value, 0, sizeof(variable->value));
			variable->typid = typid;
			if (typid != RECORDOID)
			{
				get_typlenbyval(variable->typid,
								&variable->value.scalar.typlen,
								&variable->value.scalar.typbyval);
				variable->value.scalar.is_null = true;
			}
		}
		else if (strict)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("unrecognized variable \"%s\"", key)));
	}

	return variable;
}
