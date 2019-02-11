/*
 * pg_execplan.c
 *
 */

#include "postgres.h"

#include "access/printtup.h"
#include "commands/extension.h"
#include "commands/prepare.h"
#include "common/base64.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
#include "nodes/params.h"
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
	char			*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
					*squery = TextDatumGetCString(PG_GETARG_DATUM(1)),
					*sparams = NULL,
					*start_address,
					*splan;
	int				nstmts,
					sparams_len;
	FILE			*fout;
	MemoryContext	oldcontext;
	List	  		*parsetree_list;
	RawStmt			*parsetree;
	List			*querytree_list,
					*plantree_list;
	QueryDesc		*queryDesc;
	size_t			string_len;

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(LOG, "Store into %s plan of the query %s.", filename, squery);

	oldcontext = MemoryContextSwitchTo(MessageContext);

	parsetree_list = pg_parse_query(squery);
	nstmts = list_length(parsetree_list);
	if (nstmts != 1)
		elog(ERROR, "Query contains %d elements, but must contain only one.", nstmts);

	parsetree = (RawStmt *) linitial(parsetree_list);
	querytree_list = pg_analyze_and_rewrite(parsetree, squery, NULL, 0, NULL);
	plantree_list = pg_plan_queries(querytree_list, CURSOR_OPT_PARALLEL_OK, NULL);

	queryDesc = CreateQueryDesc((PlannedStmt *) linitial(plantree_list),
												squery,
												InvalidSnapshot,
												InvalidSnapshot,
												None_Receiver,
												NULL, NULL,
												0);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "BEFORE writing %s ...", filename);

	fout = fopen(filename, "wb");
	Assert(fout != NULL);
	string_len = strlen(squery);
	fwrite(&string_len, sizeof(size_t), 1, fout);
	fwrite(squery, sizeof(char), string_len, fout);

	set_portable_output(true);
	splan = nodeToString(queryDesc->plannedstmt);
	set_portable_output(false);
	string_len = strlen(splan);
	fwrite(&string_len, sizeof(size_t), 1, fout);
	fwrite(splan, sizeof(char), string_len, fout);

	/*
	 * Serialize parameters list. In this case we have no parameters and will
	 * serialize NULL list.
	 */
	sparams_len = EstimateParamListSpace(NULL);
	sparams = palloc0(sparams_len);
	start_address = sparams;
	SerializeParamList(NULL, &start_address);
	string_len = sparams_len;
	fwrite(&string_len, sizeof(size_t), 1, fout);
	fwrite(sparams, sizeof(char), string_len, fout);
	fclose(fout);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_VOID();
}

static void
exec_plan(char *squery, char *splan, char *sparams)
{
	PlannedStmt			*pstmt;
	ParamListInfo 		paramLI = NULL;
	CachedPlanSource	*psrc;
	CachedPlan			*cplan;
	QueryDesc			*queryDesc;
	DestReceiver		*receiver;
	int					eflags = 0;
	Oid					*param_types = NULL;
	char				*start_address = sparams;

	Assert(squery && splan && sparams);

	PG_TRY();
	{
		pstmt = (PlannedStmt *) stringToNode(splan);

		/* Deserialize parameters of the query */
		paramLI = RestoreParamList(&start_address);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD PLAN: %s. Query: %s", splan, squery);
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query: %s\n", squery);
	if (EXPLAN_DEBUG_LEVEL > 1)
		elog(INFO, "\nplan: %s\n", splan);

	psrc = CreateCachedPlan(NULL, squery, NULL);

	if (paramLI->numParams > 0)
	{
		int i;

		param_types = palloc(sizeof(Oid) * paramLI->numParams);
		for (i = 0; i < paramLI->numParams; i++)
			param_types[i] = paramLI->params[i].ptype;
	}
	CompleteCachedPlan(psrc, NIL, NULL, param_types, paramLI->numParams, NULL,
								NULL, CURSOR_OPT_GENERIC_PLAN, false);

	SetRemoteSubplan(psrc, pstmt);
	cplan = GetCachedPlan(psrc, paramLI, false, NULL);

	receiver = CreateDestReceiver(DestLog);

	PG_TRY();
	{
		queryDesc = CreateQueryDesc(pstmt,
									squery,
									GetActiveSnapshot(),
									InvalidSnapshot,
									receiver,
									paramLI, NULL,
									0);
		ExecutorStart(queryDesc, eflags);
		PushActiveSnapshot(queryDesc->snapshot);
		ExecutorRun(queryDesc, ForwardScanDirection, 0, true);
		PopActiveSnapshot();
		ExecutorFinish(queryDesc);
		ExecutorEnd(queryDesc);
		FreeQueryDesc(queryDesc);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD QUERY: '%s'.", squery);
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
	char	*squery = TextDatumGetCString(PG_GETARG_DATUM(0));
	char	*splan = TextDatumGetCString(PG_GETARG_DATUM(1));
	char	*sparams = TextDatumGetCString(PG_GETARG_DATUM(2));

	char	*dec_query,
			*dec_plan,
			*dec_params;
	int		dec_query_len,
			dec_query_len1,
			dec_plan_len,
			dec_plan_len1,
			dec_params_len,
			dec_params_len1;

	Assert(squery != NULL);
	Assert(splan != NULL);
	Assert(sparams != NULL);

	dec_query_len = pg_b64_dec_len(strlen(squery));
	dec_query = palloc0(dec_query_len + 1);
	dec_query_len1 = pg_b64_decode(squery, strlen(squery), dec_query);
	Assert(dec_query_len >= dec_query_len1);

	dec_plan_len = pg_b64_dec_len(strlen(splan));
	dec_plan = palloc0(dec_plan_len + 1);
	dec_plan_len1 = pg_b64_decode(splan, strlen(splan), dec_plan);
	Assert(dec_plan_len >= dec_plan_len1);

	dec_params_len = pg_b64_dec_len(strlen(sparams));
	dec_params = palloc0(dec_params_len + 1);
	dec_params_len1 = pg_b64_decode(sparams, strlen(sparams), dec_params);
	Assert(dec_params_len >= dec_params_len1);

	exec_plan(dec_query, dec_plan, dec_params);
	pfree(dec_query);
	pfree(dec_plan);
	pfree(dec_params);
	PG_RETURN_BOOL(true);
}

static void
LoadPlanFromFile(const char *filename, char **squery, char **splan,
				 char **sparams)
{
	FILE	*fin;
	size_t	string_len;
	int		nelems;

	fin = fopen(filename, "rb");
	Assert(fin != NULL);

	/* Read query string size, allocate memory and read query from the file. */
	nelems = fread(&string_len, sizeof(size_t), 1, fin);
	Assert(nelems == 1);
	*squery = palloc0(string_len + 1);
	nelems = fread(*squery, sizeof(char), string_len, fin);
	Assert(nelems == string_len);

	/* Read plan size, allocate memory and read plan from the file. */
	nelems = fread(&string_len, sizeof(size_t), 1, fin);
	Assert(nelems == 1);
	*splan = palloc0(string_len + 1);
	nelems = fread(*splan, sizeof(char), string_len, fin);
	Assert(nelems == string_len);

	/*
	 * Read serialized query parameters string length, allocate memory and
	 * read it from the file.
	 */
	nelems = fread(&string_len, sizeof(size_t), 1, fin);
	Assert(nelems == 1);
	*sparams = palloc0(string_len + 1);
	nelems = fread(*sparams, sizeof(char), string_len, fin);
	Assert(nelems == string_len);

	fclose(fin);

}

Datum
pg_exec_stored_plan(PG_FUNCTION_ARGS)
{
	char	*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
			*squery = NULL,
			*splan = NULL,
			*sparams = NULL;

	LoadPlanFromFile(filename, &squery, &splan, &sparams);

	Assert(squery && splan && sparams);
	exec_plan(squery, splan, sparams);
	pfree(squery);
	pfree(splan);
	pfree(sparams);
	PG_RETURN_BOOL(true);
}
