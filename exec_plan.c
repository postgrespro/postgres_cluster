
#include "postgres.h"

//#include "access/xact.h"
#include "commands/extension.h"
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "nodes/params.h"
#include "optimizer/planner.h"
#include "storage/lmgr.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_execute_plan);

void _PG_init(void);

static ProcessUtility_hook_type 	next_ProcessUtility_hook = NULL;
static planner_hook_type			prev_planner_hook = NULL;
static ExecutorStart_hook_type		prev_ExecutorStart = NULL;
static ExecutorEnd_hook_type		prev_ExecutorEnd = NULL;

static void HOOK_Utility_injection(PlannedStmt *pstmt, const char *queryString,
						ProcessUtilityContext context, ParamListInfo params,
						QueryEnvironment *queryEnv, DestReceiver *dest,
						char *completionTag);
static PlannedStmt *HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams);
static void HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags);
static void HOOK_ExecEnd_injection(QueryDesc *queryDesc);
static char * serialize_plan(PlannedStmt *pstmt, ParamListInfo boundParams,
							 const char *querySourceText);
static void execute_query(char *planString);

static PGconn	*conn = NULL;

int node_number1 = 0;

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomIntVariable("pargres.node",
							"Node number in instances collaboration",
							NULL,
							&node_number1,
							0,
							0,
							1023,
							PGC_SIGHUP,
							GUC_NOT_IN_SAMPLE,
							NULL,
							NULL,
							NULL);

	/* ProcessUtility hook */
	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = HOOK_Utility_injection;

	/* Planner hook */
	prev_planner_hook	= planner_hook;
	planner_hook		= HOOK_Planner_injection;

	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = HOOK_ExecStart_injection;

	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = HOOK_ExecEnd_injection;
}

static void
HOOK_Utility_injection(PlannedStmt *pstmt,
						const char *queryString,
						ProcessUtilityContext context,
						ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest,
						char *completionTag)
{
	Node	*parsetree = pstmt->utilityStmt;

	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		(nodeTag(parsetree) == T_CreateStmt))
	{
		char	conninfo[1024];
		int		status;

		elog(LOG, "Send UTILITY query: %s", queryString);

		/* Connect to slave and send it a query plan */
		sprintf(conninfo, "host=localhost port=5433%c", '\0');
		conn = PQconnectdb(conninfo);
		if (PQstatus(conn) == CONNECTION_BAD)
			elog(LOG, "Connection error. conninfo: %s", conninfo);

		status = PQsendQuery(conn, queryString);
		if (status == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
	}

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, context, params,
									 queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString,
											context, params, queryEnv,
											dest, completionTag);

	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL)
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
		PQfinish(conn);
		conn = NULL;
	}
}

static void
execute_query(char *planString)
{
	char	conninfo[1024];
	char	*SQLCommand;
	int		status;

	/* Connect to slave and send it a query plan */
	sprintf(conninfo, "host=localhost port=5433%c", '\0');
	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) == CONNECTION_BAD)
		elog(LOG, "Connection error. conninfo: %s", conninfo);

	SQLCommand = (char *) palloc0(strlen(planString)+100);
	sprintf(SQLCommand, "SELECT pg_execute_plan('%s')", planString);
//elog(LOG, "query: %s", SQLCommand);
	status = PQsendQuery(conn, SQLCommand);
	if (status == 0)
		elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
}

static PlannedStmt *
HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams)
{
	PlannedStmt *pstmt;

	conn = NULL;

	if (prev_planner_hook)
		pstmt = prev_planner_hook(parse, cursorOptions, boundParams);
	else
		pstmt = standard_planner(parse, cursorOptions, boundParams);

	if ((node_number1 > 0) || (parse->utilityStmt != NULL))
		return pstmt;

	/* Extension is not initialized. */
	if (OidIsValid(get_extension_oid("execplan", true)))
	{

	}
	return pstmt;
}

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node	*parsetree = queryDesc->plannedstmt->utilityStmt;

	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)))
	{
		elog(LOG, "Send query: %s", queryDesc->sourceText);
		execute_query(serialize_plan(queryDesc->plannedstmt, queryDesc->params,
									 queryDesc->sourceText));
	}
	else
	{
//		elog(LOG, "EXECUTOR Process query without sending. IsParsetree=%hhu node_number1=%d IsExt=%hhu", parsetree != NULL, node_number1, OidIsValid(get_extension_oid("execplan", true)));
	}

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	/* Execute before hook because it destruct memory context of exchange list */
	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL)
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
		PQfinish(conn);
		conn = NULL;
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

#include "utils/fmgrprotos.h"

static char *
serialize_plan(PlannedStmt *pstmt, ParamListInfo boundParams,
			   const char *querySourceText)
{
	int		splan_len,
			sparams_len,
			qtext_len,
			econtainer_len;
	char	*serialized_plan,
			*container,
			*start_address,
			*econtainer;

	serialized_plan = nodeToString(pstmt);

	/* We use len+1 bytes for include end-of-string symbol. */
	splan_len = strlen(serialized_plan) + 1;
	qtext_len = strlen(querySourceText) + 1;

	sparams_len = EstimateParamListSpace(boundParams);

	container = (char *) palloc0(splan_len + sparams_len + qtext_len);
//elog(LOG, "Serialize sizes: plan: %d params: %d, numParams: %d", splan_len, sparams_len, boundParams->numParams);
	memcpy(container, serialized_plan, splan_len);
	start_address = container + splan_len;
	SerializeParamList(boundParams, &start_address);

	Assert(start_address == container + splan_len + sparams_len);
	memcpy(start_address, querySourceText, qtext_len);

	econtainer_len = pg_base64_enc_len(container, splan_len + sparams_len + qtext_len);
	econtainer = (char *) palloc0(econtainer_len + 1);
	if (econtainer_len != pg_base64_encode(container, splan_len + sparams_len +
			qtext_len, econtainer))
		elog(LOG, "econtainer_len: %d %d", econtainer_len, pg_base64_encode(container, splan_len + sparams_len +
				qtext_len, econtainer));
	Assert(econtainer_len == pg_base64_encode(container, splan_len + sparams_len +
										qtext_len, econtainer));
	econtainer[econtainer_len] = '\0';

	return econtainer;
}

static void
ScanQueryForLocks(PlannedStmt *pstmt, bool acquire)
{
	ListCell   *lc;

	/* Shouldn't get called on utility commands */
	Assert(pstmt->commandType != CMD_UTILITY);

	/*
	 * First, process RTEs of the current query level.
	 */
	foreach(lc, pstmt->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(lc);

		switch (rte->rtekind)
		{
			case RTE_RELATION:
				/* Acquire or release the appropriate type of lock */
				if (acquire)
					LockRelationOid(rte->relid, rte->rellockmode);
				else
					UnlockRelationOid(rte->relid, rte->rellockmode);
				break;

			default:
				/* ignore other types of RTEs */
				break;
		}
	}

}
static void
AcquirePlannerLocks(PlannedStmt *pstmt, bool acquire)
{
	if (pstmt->commandType == CMD_UTILITY)
	{
		/* Ignore utility statements, unless they contain a Query */
	}

	ScanQueryForLocks(pstmt, acquire);
}

Datum
pg_execute_plan(PG_FUNCTION_ARGS)
{
	char			*data = TextDatumGetCString(PG_GETARG_DATUM(0));
	PlannedStmt		*pstmt;
	QueryDesc		*queryDesc;
	char			*queryString;
	ParamListInfo 	paramLI = NULL;
	int				dec_tot_len;
	char			*dcontainer,
					*start_addr;

	/* Compute decoded size of bytea data */
	dec_tot_len = pg_base64_dec_len(data, strlen(data));
	dcontainer = (char *) palloc0(dec_tot_len);
	Assert(dec_tot_len == pg_base64_decode(data, strlen(data), dcontainer));

	pstmt = (PlannedStmt *) stringToNode((char *) dcontainer);
	start_addr = dcontainer + strlen(dcontainer) + 1;
	paramLI = RestoreParamList((char **) &start_addr);
	queryString = start_addr;
//	elog(LOG, "Decoded query: %s\n", start_addr);

	queryDesc = CreateQueryDesc(pstmt, queryString, GetActiveSnapshot(),
								InvalidSnapshot, None_Receiver, paramLI, NULL,
								0);
	AcquirePlannerLocks(pstmt, true);

	ExecutorStart(queryDesc, 0);

	ExecutorRun(queryDesc, ForwardScanDirection, 0, true);

	/* Shut down the executor */
	ExecutorFinish(queryDesc);
	ExecutorEnd(queryDesc);
	FreeQueryDesc(queryDesc);

	PG_RETURN_BOOL(true);
}

