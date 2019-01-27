/*
 * pg_execplan.c
 *
 */

#include "postgres.h"

#include "access/printtup.h"
#include "commands/extension.h"
#include "commands/prepare.h"
#include "executor/executor.h"
#include "nodes/plannodes.h"
#include "tcop/pquery.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"


#define EXPLAN_DEBUG_LEVEL 0

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_store_query_plan);
PG_FUNCTION_INFO_V1(pg_exec_query_plan);

void _PG_init(void);

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	return;
}

Datum
pg_store_query_plan(PG_FUNCTION_ARGS)
{
	char			*query_string = TextDatumGetCString(PG_GETARG_DATUM(1)),
					*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
					*plan_string;
	int				nstmts;
	FILE			*fout;
	MemoryContext	oldcontext;
	List	  		*parsetree_list;
	RawStmt			*parsetree;
	List			*querytree_list,
					*plantree_list;
	QueryDesc		*queryDesc;
	size_t			string_len;

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(LOG, "Store into %s plan of the query %s.", filename, query_string);

	oldcontext = MemoryContextSwitchTo(MessageContext);

	parsetree_list = pg_parse_query(query_string);
	nstmts = list_length(parsetree_list);
	if (nstmts != 1)
		elog(ERROR, "Query contains %d elements, but must contain only one.", nstmts);

	parsetree = (RawStmt *) linitial(parsetree_list);
	querytree_list = pg_analyze_and_rewrite(parsetree, query_string, NULL, 0);
	plantree_list = pg_plan_queries(querytree_list, CURSOR_OPT_PARALLEL_OK, NULL);

	queryDesc = CreateQueryDesc((PlannedStmt *) linitial(plantree_list),
												query_string,
												InvalidSnapshot,
												InvalidSnapshot,
												None_Receiver,
												0,
												0);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "BEFORE writing %s ...", filename);

	fout = fopen(filename, "wb");
	Assert(fout != NULL);
	string_len = strlen(query_string);
	fwrite(&string_len, sizeof(size_t), 1, fout);
	fwrite(query_string, sizeof(char), string_len, fout);

	set_portable_output(true);
	plan_string = nodeToString(queryDesc->plannedstmt);
	set_portable_output(false);
	string_len = strlen(plan_string);
	fwrite(&string_len, sizeof(size_t), 1, fout);
	fwrite(plan_string, sizeof(char), string_len, fout);

	fclose(fout);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_VOID();
}

static void
LoadPlanFromFile(const char *filename, char **query_string, char **plan_string)
{
	FILE	*fin;
	size_t	string_len;
	int		nelems;

	fin = fopen(filename, "rb");
	Assert(fin != NULL);

	nelems = fread(&string_len, sizeof(size_t), 1, fin);
	Assert(nelems == 1);
	*query_string = palloc0(string_len + 1);
	nelems = fread(*query_string, sizeof(char), string_len, fin);
	Assert(nelems == string_len);

	nelems = fread(&string_len, sizeof(size_t), 1, fin);
	Assert(nelems == 1);
	*plan_string = palloc0(string_len + 1);
	nelems = fread(*plan_string, sizeof(char), string_len, fin);
	Assert(nelems == string_len);

	fclose(fin);

}

Datum
pg_exec_query_plan(PG_FUNCTION_ARGS)
{
	char				*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
						*query_string = NULL,
						*plan_string = NULL;
	PlannedStmt			*pstmt;
	ParamListInfo 		paramLI = NULL;
	CachedPlanSource	*psrc;
	CachedPlan			*cplan;
	Portal				portal;
	DestReceiver		*receiver;
	int16				format = 0;
	int					eflags = 0;

	LoadPlanFromFile(filename, &query_string, &plan_string);

	PG_TRY();
	{
		set_portable_input(true);
		pstmt = (PlannedStmt *) stringToNode(plan_string);
		set_portable_input(false);
	}
	PG_CATCH();
	{
		elog(INFO, "!!!BAD PLAN: %s", plan_string);
		PG_RE_THROW();
	}
	PG_END_TRY();

	psrc = CreateCachedPlan(NULL, query_string, query_string);
	CompleteCachedPlan(psrc, NIL, NULL, NULL, 0, NULL, NULL,
							   CURSOR_OPT_GENERIC_PLAN, false);
	StorePreparedStatement(query_string, psrc, false);
	SetRemoteSubplan(psrc, pstmt);
	cplan = GetCachedPlan(psrc, paramLI, false);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query: %s\n", query_string);
	if (EXPLAN_DEBUG_LEVEL > 1)
		elog(INFO, "\nplan: %s\n", plan_string);

	receiver = CreateDestReceiver(DestDebug);
	portal = CreateNewPortal();
	portal->visible = false;
	PortalDefineQuery(portal,
					  NULL,
					  query_string,
					  query_string,
					  cplan->stmt_list,
					  cplan);
	PortalStart(portal, paramLI, eflags, InvalidSnapshot);
	PortalSetResultFormat(portal, 0, &format);
	(void) PortalRun(portal,
					 FETCH_ALL,
					 true,
					 receiver,
					 receiver,
					 query_string);
	receiver->rDestroy(receiver);
	PortalDrop(portal, false);
	DropPreparedStatement(query_string, false);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query execution finished.\n");

	PG_RETURN_VOID();
}
