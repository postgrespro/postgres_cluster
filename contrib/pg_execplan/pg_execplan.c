/*
 * pg_execplan.c
 *
 */

#include "postgres.h"

#include "access/printtup.h"
#include "commands/extension.h"
#include "commands/prepare.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
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
PG_FUNCTION_INFO_V1(pg_exec_plan);
PG_FUNCTION_INFO_V1(pg_exec_stored_plan);

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
exec_plan(char *query_string, char *plan_string)
{
	PlannedStmt			*pstmt;
	ParamListInfo 		paramLI = NULL;
	CachedPlanSource	*psrc;
	CachedPlan			*cplan;
	QueryDesc			*queryDesc;
	DestReceiver		*receiver;
	int					eflags = 0;

	PG_TRY();
	{
		set_portable_input(true);
		pstmt = (PlannedStmt *) stringToNode(plan_string);
		set_portable_input(false);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD PLAN: %s. Query: %s", plan_string, query_string);
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query: %s\n", query_string);
	if (EXPLAN_DEBUG_LEVEL > 1)
		elog(INFO, "\nplan: %s\n", plan_string);

	psrc = CreateCachedPlan(NULL, query_string, NULL);
	CompleteCachedPlan(psrc, NIL, NULL, NULL, 0, NULL, NULL,
							   CURSOR_OPT_GENERIC_PLAN, false);

	SetRemoteSubplan(psrc, pstmt);
	cplan = GetCachedPlan(psrc, paramLI, false);

	receiver = CreateDestReceiver(DestLog);

	PG_TRY();
	{
		queryDesc = CreateQueryDesc(pstmt,
									query_string,
									GetActiveSnapshot(),
									InvalidSnapshot,
									receiver,
									paramLI,
									0);
		ExecutorStart(queryDesc, eflags);
		PushActiveSnapshot(queryDesc->snapshot);
		ExecutorRun(queryDesc, ForwardScanDirection, 0);
		PopActiveSnapshot();
		ExecutorFinish(queryDesc);
		ExecutorEnd(queryDesc);
		FreeQueryDesc(queryDesc);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD QUERY: '%s'.", query_string);
		ReleaseCachedPlan(cplan, false);
		PG_RE_THROW();
	}
	PG_END_TRY();

	receiver->rDestroy(receiver);
	ReleaseCachedPlan(cplan, false);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query execution finished.\n");
}

Datum
pg_exec_plan(PG_FUNCTION_ARGS)
{
	char	*query_string = TextDatumGetCString(PG_GETARG_DATUM(0));
	char	*plan_string = TextDatumGetCString(PG_GETARG_DATUM(1));

	char	*dec_query,
			*dec_plan;
	int		dec_query_len,
			dec_query_len1,
			dec_plan_len,
			dec_plan_len1;

	Assert(query_string != NULL);
	Assert(plan_string != NULL);

	dec_query_len = b64_dec_len(query_string, strlen(query_string) + 1)+1;
	dec_query = palloc0(dec_query_len + 1);
	dec_query_len1 = b64_decode(query_string, strlen(query_string), dec_query);
	Assert(dec_query_len > dec_query_len1);

	dec_plan_len = b64_dec_len(plan_string, strlen(plan_string) + 1);
	dec_plan = palloc0(dec_plan_len + 1);
	dec_plan_len1 = b64_decode(plan_string, strlen(plan_string), dec_plan);
	Assert(dec_plan_len > dec_plan_len1);

	exec_plan(dec_query, dec_plan);
	pfree(dec_query);
	pfree(dec_plan);
	PG_RETURN_BOOL(true);
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
pg_exec_stored_plan(PG_FUNCTION_ARGS)
{
	char				*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
						*query_string = NULL,
						*plan_string = NULL;

	LoadPlanFromFile(filename, &query_string, &plan_string);
	exec_plan(query_string, plan_string);
	PG_RETURN_BOOL(true);
}
