/* ------------------------------------------------------------------------
 *
 * common.c
 *		Common code for ParGRES extension
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/genam.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/indexing.h"
#include "catalog/pg_extension.h"
#include "commands/extension.h"
#include "utils/fmgroids.h"

#include "common.h"


/* GUC variables */
int node_number;
int nodes_at_cluster;
char	*pargres_hosts_string = NULL;
char	*pargres_ports_string = NULL;
int		eports_pool_size = 100;

int CoordNode = -1;
bool PargresInitialized = false;
PortStack *PORTS;


Oid
get_pargres_schema(void)
{
	Oid				result;
	Relation		rel;
	SysScanDesc		scandesc;
	HeapTuple		tuple;
	ScanKeyData		entry[1];
	Oid				ext_oid;

	/* It's impossible to fetch pg_pathman's schema now */
	if (!IsTransactionState())
		return InvalidOid;

	ext_oid = get_extension_oid("pargres", true);
	if (ext_oid == InvalidOid)
		return InvalidOid; /* exit if pg_pathman does not exist */

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(ext_oid));

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

void
STACK_Init(PortStack *stack, int range_min, int size)
{
	int i;

	LWLockAcquire(&stack->lock, LW_EXCLUSIVE);

	stack->size = size;
	stack->index = 0;
	for (i = 0; i < stack->size; i++)
		stack->values[i] = range_min + i;

	LWLockRelease(&stack->lock);
}

int
STACK_Pop(PortStack *stack)
{
	int value;

	LWLockAcquire(&stack->lock, LW_EXCLUSIVE);

	Assert(stack->index < stack->size);
	value = stack->values[stack->index++];

	LWLockRelease(&stack->lock);
	return value;
}

void
STACK_Push(PortStack *stack, int value)
{
	LWLockAcquire(&stack->lock, LW_EXCLUSIVE);

	Assert(stack->index > 0);
	stack->values[--stack->index] = value;

	LWLockRelease(&stack->lock);
}
