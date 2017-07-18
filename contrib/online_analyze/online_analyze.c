/*
 * Copyright (c) 2011 Teodor Sigaev <teodor@sigaev.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *		may be used to endorse or promote products derived from this software
 *		without specific prior written permission.
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
#include "access/transam.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "commands/vacuum.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "storage/bufmgr.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
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
#if PG_VERSION_NUM >= 100000
#include "utils/varlena.h"
#include "utils/regproc.h"
#endif
#endif
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

static bool online_analyze_enable = true;
static bool online_analyze_local_tracking = false;
static bool online_analyze_verbose = true;
static double online_analyze_scale_factor = 0.1;
static int online_analyze_threshold = 50;
static int online_analyze_capacity_threshold = 100000;
static double online_analyze_min_interval = 10000;
static int online_analyze_lower_limit = 0;

static ExecutorEnd_hook_type oldExecutorEndHook = NULL;
#if PG_VERSION_NUM >= 90200
static ProcessUtility_hook_type	oldProcessUtilityHook = NULL;
#endif

typedef enum CmdKind
{
	CK_SELECT = CMD_SELECT,
	CK_UPDATE = CMD_UPDATE,
	CK_INSERT = CMD_INSERT,
	CK_DELETE = CMD_DELETE,
	CK_TRUNCATE,
	CK_FASTTRUNCATE,
	CK_CREATE,
	CK_ANALYZE,
	CK_VACUUM
} CmdKind;


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

typedef struct OnlineAnalyzeTableStat {
	Oid				tableid;
	bool			rereadStat;
	PgStat_Counter	n_tuples;
	PgStat_Counter	changes_since_analyze;
	TimestampTz		autovac_analyze_timestamp;
	TimestampTz		analyze_timestamp;
} OnlineAnalyzeTableStat;

static	MemoryContext	onlineAnalyzeMemoryContext = NULL;
static	HTAB	*relstats = NULL;

static void relstatsInit(void);

#if PG_VERSION_NUM < 100000
static int
oid_cmp(const void *a, const void *b)
{
	if (*(Oid*)a == *(Oid*)b)
		return 0;
	return (*(Oid*)a > *(Oid*)b) ? 1 : -1;
}
#endif

static const char *
tableListAssign(const char * newval, bool doit, TableList *tbl)
{
	char		*rawname;
	List		*namelist;
	ListCell	*l;
	Oid			*newOids = NULL;
	int			nOids = 0,
				i = 0;

	rawname = pstrdup(newval);

	if (!SplitIdentifierString(rawname, ',', &namelist))
		goto cleanup;

	if (doit)
	{
		nOids = list_length(namelist);
		newOids = malloc(sizeof(Oid) * (nOids+1));
		if (!newOids)
			elog(ERROR,"could not allocate %d bytes",
				 (int)(sizeof(Oid) * (nOids+1)));
	}

	foreach(l, namelist)
	{
		char	*curname = (char *) lfirst(l);
#if PG_VERSION_NUM >= 90200
		Oid		relOid = RangeVarGetRelid(makeRangeVarFromNameList(
							stringToQualifiedNameList(curname)), NoLock, true);
#else
		Oid		relOid = RangeVarGetRelid(makeRangeVarFromNameList(
							stringToQualifiedNameList(curname)), true);
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
	char	*val, *ptr;
	int		i,
			len;

	len = 1 /* \0 */ + tbl->nTables * (2 * NAMEDATALEN + 2 /* ', ' */ + 1 /* . */);
	ptr = val = palloc(len);
	*ptr ='\0';
	for(i=0; i<tbl->nTables; i++)
	{
		char	*relname = get_rel_name(tbl->tables[i]);
		Oid		nspOid = get_rel_namespace(tbl->tables[i]);
		char	*nspname = get_namespace_name(nspOid);

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
makeAnalyze(Oid relOid, CmdKind operation, int64 naffected)
{
	TimestampTz				now = GetCurrentTimestamp();
	Relation				rel;
	OnlineAnalyzeTableType	reltype;
	bool					found = false,
							newTable = false;
	OnlineAnalyzeTableStat	*rstat,
							dummyrstat;
	PgStat_StatTabEntry		*tabentry = NULL;

	if (relOid == InvalidOid)
		return;

	if (naffected == 0)
		/* return if there is no changes */
		return;
	else if (naffected < 0)
		/* number if affected rows is unknown */
		naffected = 0;

	rel = RelationIdGetRelation(relOid);
	if (rel->rd_rel->relkind != RELKIND_RELATION)
	{
		RelationClose(rel);
		return;
	}

	reltype =
#if PG_VERSION_NUM >= 90100
		(rel->rd_rel->relpersistence == RELPERSISTENCE_TEMP)
#else
		(rel->rd_istemp || rel->rd_islocaltemp)
#endif
			? OATT_TEMPORARY : OATT_PERSISTENT;

	RelationClose(rel);

	/*
	 * includeTables overwrites excludeTables
	 */
	switch(online_analyze_table_type)
	{
		case OATT_ALL:
			if (get_rel_relkind(relOid) != RELKIND_RELATION ||
				(matchOid(&excludeTables, relOid) == true &&
				matchOid(&includeTables, relOid) == false))
				return;
			break;
		case OATT_NONE:
			if (get_rel_relkind(relOid) != RELKIND_RELATION ||
				matchOid(&includeTables, relOid) == false)
				return;
			break;
		case OATT_TEMPORARY:
		case OATT_PERSISTENT:
		default:
			/*
			 * skip analyze if relation's type doesn't not match
			 * online_analyze_table_type
			 */
			if ((online_analyze_table_type & reltype) == 0 ||
				matchOid(&excludeTables, relOid) == true)
			{
				if (matchOid(&includeTables, relOid) == false)
					return;
			}
			break;
	}

	/*
	 * Do not store data about persistent table in local memory because we
	 * could not track changes of them: they could be changed by another
	 * backends. So always get a pgstat table entry.
	 */
	if (reltype == OATT_TEMPORARY)
		rstat = hash_search(relstats, &relOid, HASH_ENTER, &found);
	else
		rstat = &dummyrstat; /* found == false for following if */

	if (!found)
	{
		MemSet(rstat, 0, sizeof(*rstat));
		rstat->tableid = relOid;
		newTable = true;
	}
	else if (operation == CK_VACUUM)
	{
		/* force reread becouse vacuum could change n_tuples */
		rstat->rereadStat = true;
		return;
	}
	else if (operation == CK_ANALYZE)
	{
		/* only analyze */
		rstat->changes_since_analyze = 0;
		rstat->analyze_timestamp = now;
		return;
	}

	Assert(rstat->tableid == relOid);

	if (
		/* do not reread data if it was a truncation */
		operation != CK_TRUNCATE && operation != CK_FASTTRUNCATE &&
		/* read  for persistent table and for temp teble if it allowed */
		(reltype == OATT_PERSISTENT || online_analyze_local_tracking == false) &&
		/* read only for new table or we know that it's needed */
		(newTable == true || rstat->rereadStat == true)
	   )
	{
		rstat->rereadStat = false;

		tabentry = pgstat_fetch_stat_tabentry(relOid);

		if (tabentry)
		{
			rstat->n_tuples = tabentry->n_dead_tuples + tabentry->n_live_tuples;
			rstat->changes_since_analyze =
#if PG_VERSION_NUM >= 90000
				tabentry->changes_since_analyze;
#else
				tabentry->n_live_tuples + tabentry->n_dead_tuples -
					tabentry->last_anl_tuples;
#endif
			rstat->autovac_analyze_timestamp =
				tabentry->autovac_analyze_timestamp;
			rstat->analyze_timestamp = tabentry->analyze_timestamp;
		}
	}

	if (newTable ||
		/* force analyze after truncate, fasttruncate already did analyze */
		operation == CK_TRUNCATE || (
		/* do not analyze too often, if both stamps are exceeded the go */
		TimestampDifferenceExceeds(rstat->analyze_timestamp, now, online_analyze_min_interval) &&
		TimestampDifferenceExceeds(rstat->autovac_analyze_timestamp, now, online_analyze_min_interval) &&
		/* do not analyze too small tables */
		rstat->n_tuples + rstat->changes_since_analyze + naffected > online_analyze_lower_limit &&
		/* be in sync with relation_needs_vacanalyze */
		((double)(rstat->changes_since_analyze + naffected)) >=
			 online_analyze_scale_factor * ((double)rstat->n_tuples) +
			 (double)online_analyze_threshold))
	{
#if PG_VERSION_NUM < 90500
		VacuumStmt				vacstmt;
#else
		VacuumParams			vacstmt;
#endif
		TimestampTz				startStamp, endStamp;


		memset(&startStamp, 0, sizeof(startStamp)); /* keep compiler quiet */

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
			makeRangeVarFromOid(relOid),
			VACOPT_ANALYZE | ((online_analyze_verbose) ? VACOPT_VERBOSE : 0),
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
				get_rel_name(relOid),
				((double)secs) + ((double)microsecs)/1.0e6);
		}

		rstat->autovac_analyze_timestamp = now;
		rstat->changes_since_analyze = 0;

		switch(operation)
		{
			case CK_CREATE:
			case CK_INSERT:
			case CK_UPDATE:
				rstat->n_tuples += naffected;
			case CK_DELETE:
				rstat->rereadStat = (reltype == OATT_PERSISTENT);
				break;
			case CK_TRUNCATE:
			case CK_FASTTRUNCATE:
				rstat->rereadStat = false;
				rstat->n_tuples = 0;
				break;
			default:
				break;
		}

		/* update last analyze timestamp in local memory of backend */
		if (tabentry)
		{
			tabentry->analyze_timestamp = now;
			tabentry->changes_since_analyze = 0;
		}
#if 0
		/* force reload stat for new table */
		if (newTable)
			pgstat_clear_snapshot();
#endif
	}
	else
	{
#if PG_VERSION_NUM >= 90000
		if (tabentry)
			tabentry->changes_since_analyze += naffected;
#endif
		switch(operation)
		{
			case CK_CREATE:
			case CK_INSERT:
				rstat->changes_since_analyze += naffected;
				rstat->n_tuples += naffected;
				break;
			case CK_UPDATE:
				rstat->changes_since_analyze += 2 * naffected;
				rstat->n_tuples += naffected;
			case CK_DELETE:
				rstat->changes_since_analyze += naffected;
				break;
			case CK_TRUNCATE:
			case CK_FASTTRUNCATE:
				rstat->changes_since_analyze = 0;
				rstat->n_tuples = 0;
				break;
			default:
				break;
		}
	}

	/* Reset local cache if we are over limit */
	if (hash_get_num_entries(relstats) > online_analyze_capacity_threshold)
		relstatsInit();
}

static Const*
isFastTruncateCall(QueryDesc *queryDesc)
{
	TargetEntry	*te;
	FuncExpr	*fe;
	Const		*constval;

	if (!(
		  queryDesc->plannedstmt &&
		  queryDesc->operation == CMD_SELECT &&
		  queryDesc->plannedstmt->planTree &&
		  queryDesc->plannedstmt->planTree->targetlist &&
		  list_length(queryDesc->plannedstmt->planTree->targetlist) == 1
		 ))
		return NULL;

	te = linitial(queryDesc->plannedstmt->planTree->targetlist);

	if (!IsA(te, TargetEntry))
		return NULL;

	fe = (FuncExpr*)te->expr;

	if (!(
		  fe && IsA(fe, FuncExpr) &&
		  fe->funcid >= FirstNormalObjectId &&
		  fe->funcretset == false &&
		  fe->funcresulttype == VOIDOID &&
		  fe->funcvariadic == false &&
		  list_length(fe->args) == 1
		 ))
		return NULL;

	constval = linitial(fe->args);

	if (!(
		  IsA(constval,Const) &&
		  constval->consttype == TEXTOID &&
		  strcmp(get_func_name(fe->funcid), "fasttruncate") == 0
		 ))
		return NULL;

	return constval;
}


extern PGDLLIMPORT void onlineAnalyzeHooker(QueryDesc *queryDesc);
void
onlineAnalyzeHooker(QueryDesc *queryDesc)
{
	int64	naffected = -1;
	Const	*constval;

	if (queryDesc->estate)
		naffected = queryDesc->estate->es_processed;

#if PG_VERSION_NUM >= 90200
	if (online_analyze_enable &&
		(constval = isFastTruncateCall(queryDesc)) != NULL)
	{
		Datum		tblnamed = constval->constvalue;
		char		*tblname = text_to_cstring(DatumGetTextP(tblnamed));
		RangeVar	*tblvar =
			makeRangeVarFromNameList(stringToQualifiedNameList(tblname));

		makeAnalyze(RangeVarGetRelid(tblvar,
									 NoLock,
									 false),
					CK_FASTTRUNCATE, -1);
	}
#endif

	if (online_analyze_enable && queryDesc->plannedstmt &&
			(queryDesc->operation == CMD_INSERT ||
			 queryDesc->operation == CMD_UPDATE ||
			 queryDesc->operation == CMD_DELETE
#if PG_VERSION_NUM < 90200
			 || (queryDesc->operation == CMD_SELECT &&
				 queryDesc->plannedstmt->intoClause)
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
				int				n = lfirst_int(l);
				RangeTblEntry	*rte = list_nth(queryDesc->plannedstmt->rtable, n-1);

				if (rte->rtekind == RTE_RELATION)
					makeAnalyze(rte->relid, (CmdKind)queryDesc->operation, naffected);
			}
		}
	}

	if (oldExecutorEndHook)
		oldExecutorEndHook(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

static List		*toremove = NIL;

/*
 * removeTable called on transaction end, see call RegisterXactCallback() below
 */
static void
removeTable(XactEvent event, void *arg)
{
	ListCell	*cell;

	switch(event)
	{
		case XACT_EVENT_COMMIT:
			break;
		case XACT_EVENT_ABORT:
			toremove = NIL;
		default:
			return;
	}

	foreach(cell, toremove)
	{
		Oid	relOid = lfirst_oid(cell);

		hash_search(relstats, &relOid, HASH_REMOVE, NULL);
	}

	toremove = NIL;
}


#if PG_VERSION_NUM >= 90200
static void
onlineAnalyzeHookerUtility(
#if PG_VERSION_NUM >= 100000
						   PlannedStmt *pstmt,
#else
						   Node *parsetree,
#endif
						   const char *queryString,
#if PG_VERSION_NUM >= 90300
							ProcessUtilityContext context, ParamListInfo params,
#if PG_VERSION_NUM >= 100000
							QueryEnvironment *queryEnv,
#endif
#else
							ParamListInfo params, bool isTopLevel,
#endif
							DestReceiver *dest, char *completionTag) {
	List		*tblnames = NIL;
	CmdKind		op = CK_INSERT;
#if PG_VERSION_NUM >= 100000
	Node		*parsetree = NULL;

	if (pstmt->commandType == CMD_UTILITY)
		parsetree = pstmt->utilityStmt;
#endif

	if (parsetree && online_analyze_enable)
	{
		if (IsA(parsetree, CreateTableAsStmt) &&
			((CreateTableAsStmt*)parsetree)->into)
		{
			tblnames =
				list_make1((RangeVar*)copyObject(((CreateTableAsStmt*)parsetree)->into->rel));
			op = CK_CREATE;
		}
		else if (IsA(parsetree, TruncateStmt))
		{
			tblnames = list_copy(((TruncateStmt*)parsetree)->relations);
			op = CK_TRUNCATE;
		}
		else if (IsA(parsetree, DropStmt) &&
				 ((DropStmt*)parsetree)->removeType == OBJECT_TABLE)
		{
			ListCell	*cell;

			foreach(cell, ((DropStmt*)parsetree)->objects)
			{
				List		*relname = (List *) lfirst(cell);
				RangeVar	*rel = makeRangeVarFromNameList(relname);
				Oid			relOid = RangeVarGetRelid(rel, NoLock, true);

				if (OidIsValid(relOid))
				{
					MemoryContext	ctx;

					ctx = MemoryContextSwitchTo(TopTransactionContext);
					toremove = lappend_oid(toremove, relOid);
					MemoryContextSwitchTo(ctx);
				}
			}
		}
		else if (IsA(parsetree, VacuumStmt))
		{
			VacuumStmt	*vac = (VacuumStmt*)parsetree;

			tblnames = list_make1(vac->relation);

			if (vac->options & (VACOPT_VACUUM | VACOPT_FULL | VACOPT_FREEZE))
				/* optionally with analyze */
				op = CK_VACUUM;
			else if (vac->options & VACOPT_ANALYZE)
				op = CK_ANALYZE;
			else
				tblnames = NIL;
		}
	}

#if PG_VERSION_NUM >= 100000
#define parsetree pstmt
#endif

	if (oldProcessUtilityHook)
		oldProcessUtilityHook(parsetree, queryString,
#if PG_VERSION_NUM >= 90300
							  context, params,
#if PG_VERSION_NUM >= 100000
							  queryEnv,
#endif
#else
							  params, isTopLevel,
#endif
							  dest, completionTag);
	else
		standard_ProcessUtility(parsetree, queryString,
#if PG_VERSION_NUM >= 90300
								context, params,
#if PG_VERSION_NUM >= 100000
								queryEnv,
#endif
#else
								params, isTopLevel,
#endif
								dest, completionTag);

#if PG_VERSION_NUM >= 100000
#undef parsetree
#endif

	if (tblnames) {
		ListCell	*l;

		foreach(l, tblnames)
		{
			RangeVar	*tblname = (RangeVar*)lfirst(l);
			Oid	tblOid = RangeVarGetRelid(tblname, NoLock, true);

			makeAnalyze(tblOid, op, -1);
		}
	}
}
#endif

static void
relstatsInit(void)
{
	HASHCTL	hash_ctl;
	int		flags = 0;

	MemSet(&hash_ctl, 0, sizeof(hash_ctl));

	hash_ctl.hash = oid_hash;
	flags |= HASH_FUNCTION;

	if (onlineAnalyzeMemoryContext)
	{
		Assert(relstats != NULL);
		MemoryContextReset(onlineAnalyzeMemoryContext);
	}
	else
	{
		Assert(relstats == NULL);
		onlineAnalyzeMemoryContext =
			AllocSetContextCreate(CacheMemoryContext,
								  "online_analyze storage context",
#if PG_VERSION_NUM < 90600
								  ALLOCSET_DEFAULT_MINSIZE,
								  ALLOCSET_DEFAULT_INITSIZE,
								  ALLOCSET_DEFAULT_MAXSIZE
#else
								  ALLOCSET_DEFAULT_SIZES
#endif
								 );
	}

	hash_ctl.hcxt = onlineAnalyzeMemoryContext;
	flags |= HASH_CONTEXT;

	hash_ctl.keysize = sizeof(Oid);

	hash_ctl.entrysize = sizeof(OnlineAnalyzeTableStat);
	flags |= HASH_ELEM;

	relstats = hash_create("online_analyze storage", 1024, &hash_ctl, flags);
}

void _PG_init(void);
void
_PG_init(void)
{
	relstatsInit();

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
		"online_analyze.local_tracking",
		"Per backend tracking",
		"Per backend tracking for temp tables (do not use system statistic)",
		&online_analyze_local_tracking,
#if PG_VERSION_NUM >= 80400
		online_analyze_local_tracking,
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

	DefineCustomIntVariable(
		"online_analyze.capacity_threshold",
		"Max local cache table capacity",
		"Max local cache table capacity",
		&online_analyze_capacity_threshold,
#if PG_VERSION_NUM >= 80400
		online_analyze_capacity_threshold,
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

	DefineCustomIntVariable(
		"online_analyze.lower_limit",
		"min number of rows in table to analyze",
		"min number of rows in table to analyze",
		&online_analyze_lower_limit,
#if PG_VERSION_NUM >= 80400
		online_analyze_lower_limit,
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

	RegisterXactCallback(removeTable, NULL);
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
