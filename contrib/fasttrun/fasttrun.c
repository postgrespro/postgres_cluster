#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "storage/bufmgr.h"
#include "catalog/namespace.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include <fmgr.h>
#include <funcapi.h>
#include <access/heapam.h>
#include <catalog/pg_type.h>
#include <catalog/heap.h>
#include <commands/vacuum.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(fasttruncate);
Datum	fasttruncate(PG_FUNCTION_ARGS);
Datum	
fasttruncate(PG_FUNCTION_ARGS) {
	text    *name=PG_GETARG_TEXT_P(0);
	char	*relname;
	List	*relname_list;
	RangeVar	*relvar;
	Oid		relOid;
	Relation	rel;
	bool	makeanalyze = false;

	relname = palloc( VARSIZE(name) + 1);
	memcpy(relname, VARDATA(name), VARSIZE(name)-VARHDRSZ);
	relname[ VARSIZE(name)-VARHDRSZ ] = '\0';

	relname_list = stringToQualifiedNameList(relname);
	relvar = makeRangeVarFromNameList(relname_list);
	relOid = RangeVarGetRelid(relvar, AccessExclusiveLock, false);

	if ( get_rel_relkind(relOid) != RELKIND_RELATION )
		elog(ERROR,"Relation isn't a ordinary table");

	rel = heap_open(relOid, NoLock);

	if ( !isTempNamespace(get_rel_namespace(relOid)) )
		elog(ERROR,"Relation isn't a temporary table");

	heap_truncate(list_make1_oid(relOid));

	if ( rel->rd_rel->relpages > 0 || rel->rd_rel->reltuples > 0 )
		makeanalyze = true;

	/*
	 * heap_truncate doesn't unlock the table,
	 * so we should unlock it.
	 */

	heap_close(rel, AccessExclusiveLock);

	if ( makeanalyze ) {
		VacuumParams	params;

		params.freeze_min_age = -1;
		params.freeze_table_age = -1;
		params.multixact_freeze_min_age = -1;
		params.multixact_freeze_table_age = -1;
		params.is_wraparound = false;
		params.log_min_duration = -1;

		vacuum(VACOPT_ANALYZE, NULL, relOid, &params, NULL,
				GetAccessStrategy(BAS_VACUUM), false);
	}

	PG_RETURN_VOID();
}
