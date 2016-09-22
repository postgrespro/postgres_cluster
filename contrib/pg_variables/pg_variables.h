/*-------------------------------------------------------------------------
 *
 * pg_variables.c
 *	  exported definitions for pg_variables.c
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */
#ifndef __PG_VARIABLES_H__
#define __PG_VARIABLES_H__

#include "access/htup.h"
#include "access/tupdesc.h"
#include "datatype/timestamp.h"
#include "utils/date.h"
#include "utils/hsearch.h"
#include "utils/numeric.h"
#include "utils/jsonb.h"

/* initial number of packages hashes */
#define NUMPACKAGES 8
#define NUMVARIABLES 16

typedef struct HashPackageEntry
{
	char			name[NAMEDATALEN];
	HTAB		   *variablesHash;
	/* Memory context for package variables for easy memory release */
	MemoryContext	hctx;
} HashPackageEntry;

typedef struct RecordVar
{
	HTAB		   *rhash;
	TupleDesc		tupdesc;
	/* Memory context for records hash table for easy memory release */
	MemoryContext	hctx;
	/* Hash function info */
	FmgrInfo		hash_proc;
	/* Match function info */
	FmgrInfo		cmp_proc;
} RecordVar;

typedef struct ScalarVar
{
	Datum			value;
	bool			is_null;
	bool			typbyval;
	int16			typlen;
} ScalarVar;

typedef struct HashVariableEntry
{
	char			name[NAMEDATALEN];
	union
	{
		ScalarVar	scalar;
		RecordVar	record;
	}				value;

	Oid				typid;
} HashVariableEntry;

typedef struct HashRecordKey
{
	Datum		value;
	bool		is_null;
	/* Hash function info */
	FmgrInfo   *hash_proc;
	/* Match function info */
	FmgrInfo   *cmp_proc;
} HashRecordKey;

typedef struct HashRecordEntry
{
	HashRecordKey	key;
	HeapTuple		tuple;
} HashRecordEntry;

extern void init_attributes(HashVariableEntry* variable, TupleDesc tupdesc,
							MemoryContext topctx);
extern void check_attributes(HashVariableEntry *variable, TupleDesc tupdesc);
extern void check_record_key(HashVariableEntry *variable, Oid typid);

extern void insert_record(HashVariableEntry* variable,
						  HeapTupleHeader tupleHeader);
extern bool update_record(HashVariableEntry *variable,
						  HeapTupleHeader tupleHeader);
extern bool delete_record(HashVariableEntry* variable, Datum value,
						  bool is_null);
extern void clean_records(HashVariableEntry *variable);

#endif   /* __PG_VARIABLES_H__ */
