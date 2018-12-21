/*
 * repeater.c
 *
 */

#include "postgres.h"

#include "commands/extension.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "nodes/params.h"
#include "optimizer/planner.h"
#include "tcop/utility.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/plancache.h"

#include "exec_plan.h"

void _PG_init(void);

static ProcessUtility_hook_type 	next_ProcessUtility_hook = NULL;
static ExecutorStart_hook_type		prev_ExecutorStart = NULL;
static ExecutorEnd_hook_type		prev_ExecutorEnd = NULL;

static void HOOK_Utility_injection(PlannedStmt *pstmt, const char *queryString,
						ProcessUtilityContext context, ParamListInfo params,
						QueryEnvironment *queryEnv, DestReceiver *dest,
						char *completionTag);
static void HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags);
static void HOOK_ExecEnd_injection(QueryDesc *queryDesc);


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

	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = HOOK_ExecStart_injection;

	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = HOOK_ExecEnd_injection;
}

#include "access/xact.h"

static PGconn	*conn = NULL;

static PGconn*
EstablishConnection(void)
{
	char	conninfo[1024];

	if (conn != NULL)
		return conn;

elog(LOG, "Create new connection ---");
	/* Connect to slave and send it a query plan */
	sprintf(conninfo, "host=localhost port=5433%c", '\0');
	conn = PQconnectdb(conninfo);

	if (PQstatus(conn) == CONNECTION_BAD)
		elog(LOG, "Connection error. conninfo: %s", conninfo);

	return conn;
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

	//elog(LOG, "queryString: %s", queryString);
	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		(nodeTag(parsetree) != T_CopyStmt) &&
		(nodeTag(parsetree) != T_CreateExtensionStmt)// &&
//		(context != PROCESS_UTILITY_SUBCOMMAND)
	   )
	{
		int		status;

		status = PQsendQuery(EstablishConnection(), queryString);

		if (status == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
	}
	else if (node_number1 == 0)
		elog(LOG, "UTILITY query without sending: %s, isExt=%d node_number1=%d", queryString, (OidIsValid(get_extension_oid("execplan", true))), node_number1);

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
	}
}

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node	*parsetree = queryDesc->plannedstmt->utilityStmt;

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)))
	{
//		elog(LOG, "Send query: %s", queryDesc->sourceText);
		execute_query(EstablishConnection(), queryDesc, eflags);
	}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	/* Execute before hook because it destruct memory context of exchange list */
	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL)
		{
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
//			elog(LOG, "PQresultStatus(result)=%d", PQresultStatus(result));
		}
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}
