/*
 * Copyright (c) 2011 Teodor Sigaev <teodor@sigaev.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *        may be used to endorse or promote products derived from this software
 *        without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "postgres.h"

#include "pgstat.h"
#include "catalog/namespace.h"
#include "commands/vacuum.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "storage/bufmgr.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/guc.h"
#if PG_VERSION_NUM >= 90200
#include "catalog/pg_class.h"
#include "nodes/primnodes.h"
#include "tcop/utility.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/timestamp.h"
#if PG_VERSION_NUM >= 90500
#include "nodes/makefuncs.h"
#endif
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

static bool online_analyze_enable = true;
static bool online_analyze_verbose = true;
static double online_analyze_scale_factor = 0.1;
static int online_analyze_threshold = 50;
static double online_analyze_min_interval = 10000;

static ExecutorEnd_hook_type oldExecutorEndHook = NULL;
#if PG_VERSION_NUM >= 90200
static ProcessUtility_hook_type	oldProcessUtilityHook = NULL;
#endif

typedef enum
{
	OATT_ALL		= 0x03,
	OATT_PERSISTENT = 0x01,
	OATT_TEMPORARY  = 0x02,
	OATT_NONE		= 0x00
} OnlineAnalyzeTableType;

static const struct config_enum_entry online_analyze_table_type_options[] = 
{
	{"all", OATT_ALL, false},
	{"persistent", OATT_PERSISTENT, false},
	{"temporary", OATT_TEMPORARY, false},
	{"none", OATT_NONE, false},
	{NULL, 0, false},
};

static int online_analyze_table_type = (int)OATT_ALL;

typedef struct TableList {
	int		nTables;
	Oid		*tables;
	char	*tableStr;
} TableList;

static TableList excludeTables = {0, NULL, NULL};
static TableList includeTables = {0, NULL, NULL};

static int
oid_cmp(const void *a, const void *b)
{
	if (*(Oid*)a == *(Oid*)b)
		return 0;
	return (*(Oid*)a > *(Oid*)b) ? 1 : -1;
}

static const char *
tableListAssign(const char * newval, bool doit, TableList *tbl)
{
	char       *rawname;
	List       *namelist;
	ListCell   *l;
	Oid         *newOids = NULL;
	int         nOids = 0,
				i = 0;

	rawname = pstrdup(newval);

	if (!SplitIdentifierString(rawname, ',', &namelist))
		goto cleanup;

	if (doit)
	{
		nOids = list_length(namelist);
		newOids = malloc(sizeof(Oid) * (nOids+1));
		if (!newOids)
			elog(ERROR,"could not allocate %d bytes", (int)(sizeof(Oid) * (nOids+1)));
	}

	foreach(l, namelist)
	{
		char        *curname = (char *) lfirst(l);
#if PG_VERSION_NUM >= 90200
		Oid         relOid = RangeVarGetRelid(makeRangeVarFromNameList(stringToQualifiedNameList(curname)), 
												NoLock, true);
#else
		Oid         relOid = RangeVarGetRelid(makeRangeVarFromNameList(stringToQualifiedNameList(curname)), 
												true);
#endif

		if (relOid == InvalidOid)
		{
#if PG_VERSION_NUM >= 90100
			if (doit == false)
#endif
			elog(WARNING,"'%s' does not exist", curname);
			continue;
		}
		else if ( get_rel_relkind(relOid) != RELKIND_RELATION )
		{
#if PG_VERSION_NUM >= 90100
			if (doit == false)
#endif
				elog(WARNING,"'%s' is not an table", curname);
			continue;
		}
		else if (doit)
		{
			newOids[i++] = relOid;
		}
	}

	if (doit)
	{
		tbl->nTables = i;
		if (tbl->tables)
			free(tbl->tables);
		tbl->tables = newOids;
		if (tbl->nTables > 1)
			qsort(tbl->tables, tbl->nTables, sizeof(tbl->tables[0]), oid_cmp);
	}

	pfree(rawname);
	list_free(namelist);

	return newval;

cleanup:
	if (newOids)
		free(newOids);
	pfree(rawname);
	list_free(namelist);
	return NULL;
}

#if PG_VERSION_NUM >= 90100
static bool
excludeTablesCheck(char **newval, void **extra, GucSource source)
{
	char *val;

	val = (char*)tableListAssign(*newval, false, &excludeTables);

	if (val)
	{
		*newval = val;
		return true;
	}

	return false;
}

static void
excludeTablesAssign(const char *newval, void *extra)
{
	tableListAssign(newval, true, &excludeTables);
}

static bool
includeTablesCheck(char **newval, void **extra, GucSource source)
{
	char *val;

	val = (char*)tableListAssign(*newval, false, &includeTables);

	if (val)
	{
		*newval = val;
		return true;
	}

	return false;
}

static void
includeTablesAssign(const char *newval, void *extra)
{
	tableListAssign(newval, true, &excludeTables);
}

#else /* PG_VERSION_NUM < 90100 */ 

static const char *
excludeTablesAssign(const char * newval, bool doit, GucSource source)
{
	return tableListAssign(newval, doit, &excludeTables);
}

static const char *
includeTablesAssign(const char * newval, bool doit, GucSource source)
{
	return tableListAssign(newval, doit, &includeTables);
}

#endif

static const char*
tableListShow(TableList *tbl)
{
	char    *val, *ptr;
	int     i,
			len;

	len = 1 /* \0 */ + tbl->nTables * (2 * NAMEDATALEN + 2 /* ', ' */ + 1 /* . */);
	ptr = val = palloc(len);
	*ptr ='\0';
	for(i=0; i<tbl->nTables; i++)
	{
		char    *relname = get_rel_name(tbl->tables[i]);
		Oid     nspOid = get_rel_namespace(tbl->tables[i]);
		char    *nspname = get_namespace_name(nspOid);

		if ( relname == NULL || nspOid == InvalidOid || nspname == NULL )
			continue;

		ptr += snprintf(ptr, len - (ptr - val), "%s%s.%s",
													(i==0) ? "" : ", ",
													nspname, relname);
	}

	return val;
}

static const char*
excludeTablesShow(void)
{
	return tableListShow(&excludeTables);
}

static const char*
includeTablesShow(void)
{
	return tableListShow(&includeTables);
}

static bool
matchOid(TableList *tbl, Oid oid)
{
	Oid	*StopLow = tbl->tables,
		*StopHigh = tbl->tables + tbl->nTables,
		*StopMiddle;

	/* Loop invariant: StopLow <= val < StopHigh */
	while (StopLow < StopHigh)
	{
		StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);

		if (*StopMiddle == oid)
			return true;
		else  if (*StopMiddle < oid)
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	return false;
}

#if PG_VERSION_NUM >= 90500
static RangeVar*
makeRangeVarFromOid(Oid relOid)
{
	return makeRangeVar(
				get_namespace_name(get_rel_namespace(relOid)),
				get_rel_name(relOid),
				-1
			);

}
#endif

static void
makeAnalyze(Oid relOid, CmdType operation, uint32 naffected)
{
	PgStat_StatTabEntry		*tabentry;
	TimestampTz 			now = GetCurrentTimestamp();

	if (relOid == InvalidOid)
		return;

	if (get_rel_relkind(relOid) != RELKIND_RELATION)
		return;

	tabentry = pgstat_fetch_stat_tabentry(relOid);

#if PG_VERSION_NUM >= 90000
#define changes_since_analyze(t)	((t)->changes_since_analyze)
#else
#define changes_since_analyze(t)	((t)->n_live_tuples + (t)->n_dead_tuples - (t)->last_anl_tuples)
#endif

	if (
		tabentry == NULL /* a new table */ ||
		(
			/* do not analyze too often, if both stamps are exceeded the go */
			TimestampDifferenceExceeds(tabentry->analyze_timestamp, now, online_analyze_min_interval) && 
			TimestampDifferenceExceeds(tabentry->autovac_analyze_timestamp, now, online_analyze_min_interval) &&
			/* be in sync with relation_needs_vacanalyze */
			((double)(changes_since_analyze(tabentry) + naffected)) >=
				online_analyze_scale_factor * ((double)(tabentry->n_dead_tuples + tabentry->n_live_tuples)) + 
					(double)online_analyze_threshold
		)
	)
	{
#if PG_VERSION_NUM < 90500
		VacuumStmt				vacstmt;
#else
		VacuumParams			vacstmt;
#endif
		TimestampTz				startStamp, endStamp;

		memset(&startStamp, 0, sizeof(startStamp)); /* keep compiler quiet */

		/*
		 * includeTables overwrites excludeTables
		 */
		switch(online_analyze_table_type)
		{
			case OATT_ALL:
				if (matchOid(&excludeTables, relOid) == true && matchOid(&includeTables, relOid) == false)
					return;
				break;
			case OATT_NONE:
				if (matchOid(&includeTables, relOid) == false)
					return;
				break;
			case OATT_TEMPORARY:
			case OATT_PERSISTENT:
			default:
				{
					Relation				rel;
					OnlineAnalyzeTableType	reltype;

					rel = RelationIdGetRelation(relOid);
					reltype = 
#if PG_VERSION_NUM >= 90100
						(rel->rd_rel->relpersistence == RELPERSISTENCE_TEMP)
#else
						(rel->rd_istemp || rel->rd_islocaltemp)
#endif
							? OATT_TEMPORARY : OATT_PERSISTENT;
					RelationClose(rel);

					/*
					 * skip analyze if relation's type doesn't not match online_analyze_table_type
					 */
					if ((online_analyze_table_type & reltype) == 0 || matchOid(&excludeTables, relOid) == true)
					{
						if (matchOid(&includeTables, relOid) == false)
							return;
					}
				}
				break;
		}

		memset(&vacstmt, 0, sizeof(vacstmt));

		vacstmt.freeze_min_age = -1;
		vacstmt.freeze_table_age = -1; /* ??? */

#if PG_VERSION_NUM < 90500
		vacstmt.type = T_VacuumStmt;
		vacstmt.relation = NULL;
		vacstmt.va_cols = NIL;
#if PG_VERSION_NUM >= 90000
		vacstmt.options = VACOPT_ANALYZE;
		if (online_analyze_verbose)
			vacstmt.options |= VACOPT_VERBOSE;
#else
		vacstmt.vacuum = vacstmt.full = false;
		vacstmt.analyze = true;
		vacstmt.verbose = online_analyze_verbose;
#endif
#else
		vacstmt.multixact_freeze_min_age = -1;
		vacstmt.multixact_freeze_table_age = -1;
		vacstmt.log_min_duration = -1;
#endif

		if (online_analyze_verbose)
			startStamp = GetCurrentTimestamp();

		analyze_rel(relOid,
#if PG_VERSION_NUM < 90500
			&vacstmt
#if PG_VERSION_NUM >= 90018
			, true
#endif
			, GetAccessStrategy(BAS_VACUUM)
#if (PG_VERSION_NUM >= 90000) && (PG_VERSION_NUM < 90004)
			, true
#endif
#else
			makeRangeVarFromOid(relOid), VACOPT_ANALYZE | ((online_analyze_verbose) ? VACOPT_VERBOSE : 0),
			&vacstmt, NULL, true, GetAccessStrategy(BAS_VACUUM)
#endif
		);

		if (online_analyze_verbose)
		{
			long	secs;
			int		microsecs;

			endStamp = GetCurrentTimestamp();
			TimestampDifference(startStamp, endStamp, &secs, &microsecs);
			elog(INFO, "analyze \"%s\" took %.02f seconds", 
				get_rel_name(relOid), ((double)secs) + ((double)microsecs)/1.0e6);
		}


		if (tabentry == NULL)
		{
			/* new table */
			pgstat_clear_snapshot();
		}
		else
		{
			/* update last analyze timestamp in local memory of backend */
			tabentry->analyze_timestamp = now;
		}
	}
#if PG_VERSION_NUM >= 90000
	else if (tabentry != NULL)
	{
		tabentry->changes_since_analyze += naffected;
	}
#endif
}

extern PGDLLIMPORT void onlineAnalyzeHooker(QueryDesc *queryDesc);
void
onlineAnalyzeHooker(QueryDesc *queryDesc)
{
	uint32	naffected = 0;

	if (queryDesc->estate)
		naffected = queryDesc->estate->es_processed;

	if (online_analyze_enable && queryDesc->plannedstmt &&
			(queryDesc->operation == CMD_INSERT ||
			 queryDesc->operation == CMD_UPDATE ||
			 queryDesc->operation == CMD_DELETE
#if PG_VERSION_NUM < 90200
			 || (queryDesc->operation == CMD_SELECT && queryDesc->plannedstmt->intoClause)
#endif
			 ))
	{
#if PG_VERSION_NUM < 90200
		if (queryDesc->operation == CMD_SELECT)
		{
			Oid	relOid = RangeVarGetRelid(queryDesc->plannedstmt->intoClause->rel, true);

			makeAnalyze(relOid, queryDesc->operation, naffected);
		}
		else
#endif
		if (queryDesc->plannedstmt->resultRelations &&
				 queryDesc->plannedstmt->rtable)
		{
			ListCell	*l;

			foreach(l, queryDesc->plannedstmt->resultRelations)
			{
				int 			n = lfirst_int(l);
				RangeTblEntry	*rte = list_nth(queryDesc->plannedstmt->rtable, n-1);

				if (rte->rtekind == RTE_RELATION)
					makeAnalyze(rte->relid, queryDesc->operation, naffected);
			}
		}
	}

	if (oldExecutorEndHook)
		oldExecutorEndHook(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

#if PG_VERSION_NUM >= 90200
static void
onlineAnalyzeHookerUtility(Node *parsetree, const char *queryString,
#if PG_VERSION_NUM >= 90300
						   	ProcessUtilityContext context, ParamListInfo params,
#else
							ParamListInfo params, bool isTopLevel,
#endif
							DestReceiver *dest, char *completionTag) {
	RangeVar	*tblname = NULL;

	if (IsA(parsetree, CreateTableAsStmt) && ((CreateTableAsStmt*)parsetree)->into)
		tblname = (RangeVar*)copyObject(((CreateTableAsStmt*)parsetree)->into->rel);

	if (oldProcessUtilityHook)
		oldProcessUtilityHook(parsetree, queryString, 
#if PG_VERSION_NUM >= 90300
							  context, params,
#else
							  params, isTopLevel,
#endif
							  dest, completionTag);
	else
		standard_ProcessUtility(parsetree, queryString, 
#if PG_VERSION_NUM >= 90300
						   		context, params,
#else
								params, isTopLevel,
#endif
								dest, completionTag);

	if (tblname) {
		Oid	tblOid = RangeVarGetRelid(tblname, NoLock, true);

		makeAnalyze(tblOid, CMD_INSERT, 0); 
	}
}
#endif

void _PG_init(void);
void
_PG_init(void)
{
	oldExecutorEndHook = ExecutorEnd_hook;

	ExecutorEnd_hook = onlineAnalyzeHooker;

#if PG_VERSION_NUM >= 90200
	oldProcessUtilityHook = ProcessUtility_hook;

	ProcessUtility_hook = onlineAnalyzeHookerUtility;
#endif


	DefineCustomBoolVariable(
		"online_analyze.enable",
		"Enable on-line analyze",
		"Enables analyze of table directly after insert/update/delete/select into",
		&online_analyze_enable,
#if PG_VERSION_NUM >= 80400
		online_analyze_enable,
#endif
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
		GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

	DefineCustomBoolVariable(
		"online_analyze.verbose",
		"Verbosity of on-line analyze",
		"Make ANALYZE VERBOSE after table's changes",
		&online_analyze_verbose,
#if PG_VERSION_NUM >= 80400
		online_analyze_verbose,
#endif
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
		GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

    DefineCustomRealVariable(
		"online_analyze.scale_factor",
		"fraction of table size to start on-line analyze",
		"fraction of table size to start on-line analyze",
		&online_analyze_scale_factor,
#if PG_VERSION_NUM >= 80400
		online_analyze_scale_factor,
#endif
		0.0,
		1.0,
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
		GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

    DefineCustomIntVariable(
		"online_analyze.threshold",
		"min number of row updates before on-line analyze",
		"min number of row updates before on-line analyze",
		&online_analyze_threshold,
#if PG_VERSION_NUM >= 80400
		online_analyze_threshold,
#endif
		0,
		0x7fffffff,
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
		GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

    DefineCustomRealVariable(
		"online_analyze.min_interval",
		"minimum time interval between analyze call (in milliseconds)",
		"minimum time interval between analyze call (in milliseconds)",
		&online_analyze_min_interval,
#if PG_VERSION_NUM >= 80400
		online_analyze_min_interval,
#endif
		0.0,
		1e30,
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
		GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

	DefineCustomEnumVariable(
		"online_analyze.table_type",
		"Type(s) of table for online analyze: all(default), persistent, temporary, none",
		NULL,
		&online_analyze_table_type,
#if PG_VERSION_NUM >= 80400
		online_analyze_table_type,
#endif
		online_analyze_table_type_options,
		PGC_USERSET,
#if PG_VERSION_NUM >= 80400
        GUC_NOT_IN_SAMPLE,
#if PG_VERSION_NUM >= 90100
		NULL,
#endif
#endif
		NULL,
		NULL
	);

    DefineCustomStringVariable(
		"online_analyze.exclude_tables",
		"List of tables which will not online analyze",
		NULL,
		&excludeTables.tableStr,
#if PG_VERSION_NUM >= 80400
		"",
#endif
		PGC_USERSET,
		0,
#if PG_VERSION_NUM >= 90100
		excludeTablesCheck,
		excludeTablesAssign,
#else
		excludeTablesAssign,
#endif
		excludeTablesShow
	);

    DefineCustomStringVariable(
		"online_analyze.include_tables",
		"List of tables which will online analyze",
		NULL,
		&includeTables.tableStr,
#if PG_VERSION_NUM >= 80400
		"",
#endif
		PGC_USERSET,
		0,
#if PG_VERSION_NUM >= 90100
		includeTablesCheck,
		includeTablesAssign,
#else
		includeTablesAssign,
#endif
		includeTablesShow
	);
}

void _PG_fini(void);
void
_PG_fini(void)
{
	ExecutorEnd_hook = oldExecutorEndHook;
#if PG_VERSION_NUM >= 90200
	ProcessUtility_hook = oldProcessUtilityHook;
#endif

	if (excludeTables.tables)
		free(excludeTables.tables);
	if (includeTables.tables)
		free(includeTables.tables);

	excludeTables.tables = includeTables.tables = NULL;
	excludeTables.nTables = includeTables.nTables = 0;
}
