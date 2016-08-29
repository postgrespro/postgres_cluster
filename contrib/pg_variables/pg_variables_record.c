/*-------------------------------------------------------------------------
 *
 * pg_variables_record.c
 *	  Functions to work with record types
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/memutils.h"
#include "utils/typcache.h"

#include "pg_variables.h"

/*
 * Hash function for records.
 *
 * We use the element type's default hash opclass, and the default collation
 * if the type is collation-sensitive.
 */
static uint32
record_hash(const void *key, Size keysize)
{
	HashRecordKey		k = *((const HashRecordKey *) key);
	Datum				h;

	if (k.is_null)
		return 0;

	h = FunctionCall1Coll(k.hash_proc, DEFAULT_COLLATION_OID, k.value);
	return DatumGetUInt32(h);
}

/*
 * Matching function for records, to be used in hashtable lookups.
 */
static int
record_match(const void *key1, const void *key2, Size keysize)
{
	HashRecordKey		k1 = *((const HashRecordKey *) key1);
	HashRecordKey		k2 = *((const HashRecordKey *) key2);
	Datum				c;

	if (k1.is_null)
	{
		if (k2.is_null)
			return 0;			/* NULL "=" NULL */
		else
			return 1;			/* NULL ">" not-NULL */
	}
	else if (k2.is_null)
		return -1;				/* not-NULL "<" NULL */

	c = FunctionCall2Coll(k1.cmp_proc, DEFAULT_COLLATION_OID,
						  k1.value, k2.value);
	return DatumGetInt32(c);
}

void
init_attributes(HashVariableEntry *variable, TupleDesc tupdesc,
				MemoryContext topctx)
{
	HASHCTL		ctl;
	char		hash_name[BUFSIZ];
	MemoryContext		oldcxt;
	RecordVar		   *record;
	TypeCacheEntry	   *typentry;
	Oid					keyid;

	Assert(variable->typid == RECORDOID);

	sprintf(hash_name, "Records hash for variable \"%s\"", variable->name);

	record = &(variable->value.record);
	record->hctx = AllocSetContextCreate(topctx,
										 hash_name,
										 ALLOCSET_DEFAULT_MINSIZE,
										 ALLOCSET_DEFAULT_INITSIZE,
										 ALLOCSET_DEFAULT_MAXSIZE);
	oldcxt = MemoryContextSwitchTo(record->hctx);
	record->tupdesc = CreateTupleDescCopyConstr(tupdesc);

	/* Initialize hash table. */
	ctl.keysize = sizeof(HashRecordKey);
	ctl.entrysize = sizeof(HashRecordEntry);
	ctl.hcxt = record->hctx;
	ctl.hash = record_hash;
	ctl.match = record_match;

	record->rhash = hash_create(hash_name, NUMVARIABLES, &ctl,
										HASH_ELEM | HASH_CONTEXT |
										HASH_FUNCTION | HASH_COMPARE);

	/* Get hash and match functions for key type. */
	keyid = record->tupdesc->attrs[0]->atttypid;
	typentry = lookup_type_cache(keyid,
								 TYPECACHE_HASH_PROC_FINFO |
								 TYPECACHE_CMP_PROC_FINFO);

	if (!OidIsValid(typentry->hash_proc_finfo.fn_oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("could not identify a hash function for type %s",
						format_type_be(keyid))));

	if (!OidIsValid(typentry->cmp_proc_finfo.fn_oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("could not identify a matching function for type %s",
						format_type_be(keyid))));

	fmgr_info(typentry->hash_proc_finfo.fn_oid, &record->hash_proc);
	fmgr_info(typentry->cmp_proc_finfo.fn_oid, &record->cmp_proc);

	MemoryContextSwitchTo(oldcxt);
}

/*
 * New record structure should be the same as the first record.
 */
void
check_attributes(HashVariableEntry *variable, TupleDesc tupdesc)
{
	int			i;

	Assert(variable->typid == RECORDOID);

	/* First, check columns count. */
	if (variable->value.record.tupdesc->natts != tupdesc->natts)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("new record structure differs from variable \"%s\" "
						"structure", variable->name)));

	/* Second, check columns type. */
	for (i = 0; i < tupdesc->natts; i++)
	{
		Form_pg_attribute	attr1 = variable->value.record.tupdesc->attrs[i],
							attr2 = tupdesc->attrs[i];

		if ((attr1->atttypid != attr2->atttypid)
			|| (attr1->attndims != attr2->attndims)
			|| (attr1->atttypmod != attr2->atttypmod))
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("new record structure differs from variable \"%s\" "
						"structure", variable->name)));
	}
}

/*
 * Check record key type. If not same then throw a error.
 */
void
check_record_key(HashVariableEntry *variable, Oid typid)
{
	Assert(variable->typid == RECORDOID);

	if (variable->value.record.tupdesc->attrs[0]->atttypid != typid)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("requested value type differs from variable \"%s\" "
						"key type", variable->name)));
}

/*
 * Insert a new record. New record key should be unique in the variable.
 */
void
insert_record(HashVariableEntry *variable, HeapTupleHeader tupleHeader)
{
	TupleDesc			tupdesc;
	HeapTuple			tuple;
	int					tuple_len;
	Datum				value;
	bool				isnull;
	RecordVar		   *record;
	HashRecordKey		k;
	HashRecordEntry	   *item;
	bool				found;
	MemoryContext		oldcxt;

	Assert(variable->typid == RECORDOID);

	record = &(variable->value.record);

	oldcxt = MemoryContextSwitchTo(record->hctx);

	tupdesc = record->tupdesc;

	/* Build a HeapTuple control structure */
	tuple_len = HeapTupleHeaderGetDatumLength(tupleHeader);

	tuple = (HeapTuple) palloc(HEAPTUPLESIZE + tuple_len);
	tuple->t_len = tuple_len;
	ItemPointerSetInvalid(&(tuple->t_self));
	tuple->t_tableOid = InvalidOid;
	tuple->t_data = (HeapTupleHeader) ((char *) tuple + HEAPTUPLESIZE);
	memcpy((char *) tuple->t_data, (char *) tupleHeader, tuple_len);

	/* Inserting a new record */
	value = fastgetattr(tuple, 1, tupdesc, &isnull);
	/* First, check if there is a record with same key */
	k.value = value;
	k.is_null = isnull;
	k.hash_proc = &record->hash_proc;
	k.cmp_proc = &record->cmp_proc;

	item = (HashRecordEntry *) hash_search(record->rhash, &k,
										   HASH_ENTER, &found);
	if (found)
	{
		heap_freetuple(tuple);
		MemoryContextSwitchTo(oldcxt);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("there is a record in the variable \"%s\" with same "
						"key", variable->name)));
	}
	/* Second, insert a new record */
	item->tuple = tuple;

	MemoryContextSwitchTo(oldcxt);
}

/*
 * Insert a record. New record key should be unique in the variable.
 */
bool
update_record(HashVariableEntry* variable, HeapTupleHeader tupleHeader)
{
	TupleDesc			tupdesc;
	HeapTuple			tuple;
	int					tuple_len;
	Datum				value;
	bool				isnull;
	RecordVar		   *record;
	HashRecordKey		k;
	HashRecordEntry	   *item;
	bool				found;
	MemoryContext		oldcxt;

	Assert(variable->typid == RECORDOID);

	record = &(variable->value.record);

	oldcxt = MemoryContextSwitchTo(record->hctx);

	tupdesc = record->tupdesc;

	/* Build a HeapTuple control structure */
	tuple_len = HeapTupleHeaderGetDatumLength(tupleHeader);

	tuple = (HeapTuple) palloc(HEAPTUPLESIZE + tuple_len);
	tuple->t_len = tuple_len;
	ItemPointerSetInvalid(&(tuple->t_self));
	tuple->t_tableOid = InvalidOid;
	tuple->t_data = (HeapTupleHeader) ((char *) tuple + HEAPTUPLESIZE);
	memcpy((char *) tuple->t_data, (char *) tupleHeader, tuple_len);

	/* Update a record */
	value = fastgetattr(tuple, 1, tupdesc, &isnull);
	k.value = value;
	k.is_null = isnull;
	k.hash_proc = &record->hash_proc;
	k.cmp_proc = &record->cmp_proc;

	item = (HashRecordEntry *) hash_search(record->rhash, &k,
										   HASH_FIND, &found);
	if (!found)
	{
		heap_freetuple(tuple);
		MemoryContextSwitchTo(oldcxt);
		return false;
	}

	item->tuple = tuple;

	MemoryContextSwitchTo(oldcxt);
	return true;
}

bool
delete_record(HashVariableEntry *variable, Datum value, bool is_null)
{
	HashRecordKey		k;
	HashRecordEntry	   *item;
	bool				found;
	RecordVar		   *record;

	Assert(variable->typid == RECORDOID);

	record = &(variable->value.record);

	/* Delete a record */
	k.value = value;
	k.is_null = is_null;
	k.hash_proc = &record->hash_proc;
	k.cmp_proc = &record->cmp_proc;

	item = (HashRecordEntry *) hash_search(record->rhash, &k,
										   HASH_REMOVE, &found);
	if (found)
		heap_freetuple(item->tuple);

	return found;
}

/*
 * Free allocated space for records hash table and datums array.
 */
void
clean_records(HashVariableEntry *variable)
{
	Assert(variable->typid == RECORDOID);

	hash_destroy(variable->value.record.rhash);
	FreeTupleDesc(variable->value.record.tupdesc);

	/* All records will be freed */
	MemoryContextDelete(variable->value.record.hctx);
}
