
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
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_execute_plan);

void _PG_init(void);

static planner_hook_type			prev_planner_hook = NULL;
static ExecutorEnd_hook_type		prev_ExecutorEnd = NULL;

static PlannedStmt *HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams);
static void HOOK_ExecEnd_injection(QueryDesc *queryDesc);
static char * serialize_plan(PlannedStmt *pstmt, ParamListInfo boundParams,
																	int *size);
static PGconn	*conn = NULL;

int node_number1 = 0;
//#include "utils/guc.h"
/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	elog(LOG, "_PG_Init");
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

	/* Planner hook */
	prev_planner_hook	= planner_hook;
	planner_hook		= HOOK_Planner_injection;

	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = HOOK_ExecEnd_injection;
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	PGresult *result;

	/* Execute before hook because it destruct memory context of exchange list */
	if (conn)
		while ((result = PQgetResult(conn)) != NULL)
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
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

	if (node_number1 > 0)
		return pstmt;
	else
		printf("SEND Query\n");

	/* Extension is not initialized. */
	if (OidIsValid(get_extension_oid("execplan", true)))
	{
		char	conninfo[1024];
		char	*data,
				*SQLCommand;
		int		status,
				data_size;

		/* Connect to slave and send it a query plan */
		sprintf(conninfo, "host=localhost port=5433%c", '\0');
		conn = PQconnectdb(conninfo);
		if (PQstatus(conn) == CONNECTION_BAD)
			elog(LOG, "Connection error. conninfo: %s", conninfo);

		data = serialize_plan(pstmt, boundParams, &data_size);
		SQLCommand = (char *) palloc0(strlen(data)+100);
		sprintf(SQLCommand, "SELECT pg_execute_plan('%s')", data);
elog(LOG, "query: %s", SQLCommand);
		status = PQsendQuery(conn, SQLCommand);
		if (status == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
	}
	return pstmt;
}

#include "utils/fmgrprotos.h"

static char *
serialize_plan(PlannedStmt *pstmt, ParamListInfo boundParams, int *size)
{
	int		splan_len,
			sparams_len,
			econtainer_len;
	char	*serialized_plan,
			*container,
			*start_address,
			*econtainer;

	Assert(size != NULL);

	serialized_plan = nodeToString(pstmt);

	/* We use splan_len+1 bytes for include end-of-string symbol. */
	splan_len = strlen(serialized_plan) + 1;

	sparams_len = EstimateParamListSpace(boundParams);

	container = (char *) palloc0(splan_len+sparams_len);
//elog(LOG, "Serialize sizes: plan: %d params: %d, numParams: %d", splan_len, sparams_len, boundParams->numParams);
	memcpy(container, serialized_plan, splan_len);
	start_address = container+splan_len;
	SerializeParamList(boundParams, &start_address);

	econtainer_len = esc_enc_len(container, splan_len+sparams_len);
	econtainer = (char *) palloc0(econtainer_len + 1);

	Assert(econtainer_len == esc_encode(container, splan_len+sparams_len, econtainer));
	econtainer[econtainer_len] = '\0';
	*size = econtainer_len + 1;
	elog(LOG, "Serialize sizes: econtainer: %d", *size);
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
	char			queryString[5] = "NONE";
	ParamListInfo 	paramLI = NULL;
	int				dec_tot_len;
	char			*dcontainer,
					*start_addr;

	elog(LOG, "datalen=%lu\n", strlen(data));
	/* Compute decoded size of bytea data */
	dec_tot_len = esc_dec_len(data, strlen(data));
	elog(LOG, "dec_tot_len=%d datalen=%lu\n", dec_tot_len, strlen(data));
	dcontainer = (char *) palloc0(dec_tot_len);
	Assert(dec_tot_len == esc_decode(data, strlen(data), dcontainer));

	pstmt = (PlannedStmt *) stringToNode((char *) dcontainer);
	elog(LOG, "Serialize Plan Size=%lu\n", strlen(dcontainer));
	start_addr = dcontainer + strlen(dcontainer) + 1;
	paramLI = RestoreParamList((char **) &start_addr);
	elog(LOG, "Decoded params. numParams: %d\n", paramLI->numParams);
//	printf("INCOMING: %s\n", data);
//	PG_RETURN_BOOL(true);
	/* Execute query plan. Based on execParallel.c ParallelQueryMain() */
//
//	ptr += strlen((const char *) ptr);
//

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

