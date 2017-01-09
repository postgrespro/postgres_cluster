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
#include "catalog/heap.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "commands/partition.h"
#include "commands/pathman_wrapper.h"
#include "executor/spi.h"
#include "nodes/value.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"
#include "utils/lsyscache.h"
#include "parser/parse_node.h"
#include "access/htup_details.h"


static void create_range_partitions(CreateStmt *stmt, Oid relid, const char *attname);


void
create_partitions(CreateStmt *stmt, Oid relid)
{
	PartitionInfo *pinfo = (PartitionInfo *) stmt->partition_info;
	Value  *attname = (Value *) linitial(((ColumnRef *) pinfo->key)->fields);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect using SPI");

	switch (pinfo->partition_type)
	{
		case P_HASH:
			{
				pm_create_hash_partitions(relid,
										  strVal(attname),
										  pinfo->partitions_count);
				break;
			}
		case P_RANGE:
			{
				create_range_partitions(stmt, relid, strVal(attname));
				break;
			}
	}

	SPI_finish(); /* close SPI connection */
}

/*
 * Extracts partitioning parameters from statement, creates partitioned table
 * and all partitions via pg_pathman's wrapper functions
 */
static void
create_range_partitions(CreateStmt *stmt, Oid relid, const char *attname)
{
	ListCell	   *lc;
	Datum			last_bound = (Datum) 0;
	bool			last_bound_is_null = true;
	PartitionInfo  *pinfo = (PartitionInfo *) stmt->partition_info;

	/* partitioning key */
	AttrNumber	attnum = get_attnum(relid, attname);
	Oid			atttype;
	int32		atttypmod;

	/* parameters */
	Datum		interval_datum;
	Oid			interval_type;

	/* for parsing interval */
	Const	   *interval_const;
	ParseState *pstate = make_parsestate(NULL);

	if (!attnum)
		elog(ERROR,
			 "Unknown attribute '%s'",
			 attname);

	/* Convert interval to Const node */
	if (IsA(pinfo->interval, A_Const))
	{
		A_Const    *con = (A_Const *) pinfo->interval;
		Value	   *val = &con->val;

		interval_const = make_const(pstate, val, con->location);
	}
	else
		elog(ERROR, "Constant interval value is expected");

	/* If attribute is of type DATE or TIMESTAMP then convert interval to Interval type */
	atttype = get_atttype(relid, attnum);
	atttypmod = get_atttypmod(relid, attnum);
	if (atttype == DATEOID || atttype == TIMESTAMPOID || atttype == TIMESTAMPTZOID)
	{
		char	   *interval_literal;

		/* We should get an UNKNOWN type here */
		if (interval_const->consttype != UNKNOWNOID)
			elog(ERROR, "Expected a literal as an interval value");

		/* Get a text representation of the interval */
		interval_literal = DatumGetCString(interval_const->constvalue);
		interval_datum = DirectFunctionCall3(interval_in,
											 CStringGetDatum(interval_literal),
											 ObjectIdGetDatum(InvalidOid),
											 Int32GetDatum(-1));
		interval_type = INTERVALOID;
	}
	else
	{
		interval_datum = interval_const->constvalue;
		interval_type = interval_const->consttype;
	}

	/* Invoke pg_pathman's wrapper */
	pm_create_range_partitions(relid,
							   attname,
							   atttype,
							   interval_datum,
							   interval_type);

	/* Add partitions */
	foreach(lc, pinfo->partitions)
	{
		RangePartitionInfo *p = (RangePartitionInfo *) lfirst(lc);
		Node *orig = (Node *) p->upper_bound;
		Node *bound_expr;

		/* Transform raw expression */
		bound_expr = cookDefault(pstate, orig, atttype, atttypmod, (char *) attname);

		if (!IsA(bound_expr, Const))
			elog(ERROR, "Constant expected");

		pm_add_range_partition(relid,
							((Const *) bound_expr)->consttype,
							p->relation ? p->relation->relname : NULL,
							last_bound,
							((Const *) bound_expr)->constvalue,
							last_bound_is_null,
							false,
							p->tablespace);
		last_bound = ((Const *) bound_expr)->constvalue;
		last_bound_is_null = false;
	}
}

void
add_range_partition(Oid parent, RangePartitionInfo *rpinfo)
{
	char	   *attname;
	AttrNumber	attnum;
	Oid			atttype;
	int32		atttypmod;
	Datum		lower,
				upper;

	/* Parsing upper bound */
	ParseState *pstate = make_parsestate(NULL);
	Node	   *bound_expr;
	// Node	   *orig_bound_expr = (Node *) p->upper_bound;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect using SPI");

	// atttype = pm_get_partition_key_type(parent);

	/* Partitioning attribute parameters */
	attname = pm_get_partition_key(parent);
	attnum = get_attnum(parent, attname);
	atttype = get_atttype(parent, attnum);
	atttypmod = get_atttypmod(parent, attnum);

	Assert(atttype != InvalidOid);

	bound_expr = cookDefault(pstate,
							 (Node *) rpinfo->upper_bound,
							 atttype,
							 atttypmod,
							 attname);

	if (!IsA(bound_expr, Const))
		elog(ERROR, "Constant expected");

	pm_get_part_range(parent, -1, atttype, &lower, &upper);
	pm_add_range_partition(parent,
						   atttype,
						   rpinfo->relation ? rpinfo->relation->relname : NULL,
						   upper,
						   ((Const *) bound_expr)->constvalue,
						   false,
						   false,
						   rpinfo->tablespace);

	SPI_finish();
}


void
merge_range_partitions(List *partitions)
{
	ListCell   *lc;
	List	   *relids = NIL;

	/* There should be exactly two partitions */
	Assert(list_length(partitions) == 2);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect using SPI");

	/* Convert rangevars to relids */
	foreach(lc, partitions)
	{
		RangeVar *rangevar = (RangeVar *) lfirst(lc);

		relids = lappend_oid(relids, RangeVarGetRelid(rangevar, NoLock, false));
	}
	pm_merge_range_partitions(linitial_oid(relids), lsecond_oid(relids));

	/* TODO: Add INTO clause */

	SPI_finish();
}
