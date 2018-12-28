/*
 * repeater.c
 *
 */

#include "postgres.h"

#include "access/xact.h"
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

PG_MODULE_MAGIC;

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
static int execute_query(PGconn *dest, QueryDesc *queryDesc, int eflags);


char	*repeater_host_name;
int		repeater_port_number;

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomStringVariable("repeater.host",
							"Remote host name for plan execution",
							NULL,
							&repeater_host_name,
							"localhost",
							PGC_SIGHUP,
							GUC_NOT_IN_SAMPLE,
							NULL,
							NULL,
							NULL);

	DefineCustomIntVariable("repeater.port",
							"Port number of remote instance",
							NULL,
							&repeater_port_number,
							5432,
							1,
							65565,
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

static PGconn	*conn = NULL;

static PGconn*
EstablishConnection(void)
{
	char	conninfo[1024];

	if (conn != NULL)
		return conn;

	/* Connect to slave and send it a query plan */
	sprintf(conninfo, "host=localhost port=5433%c", '\0');
	conn = PQconnectdb(conninfo);

	if (PQstatus(conn) == CONNECTION_BAD)
		elog(LOG, "Connection error. conninfo: %s", conninfo);

	return conn;
}

static bool ExtensionIsActivated = false;

static bool
ExtensionIsActive(void)
{
	if (ExtensionIsActivated)
		return true;

	if (
		!IsTransactionState() ||
		!OidIsValid(get_extension_oid("repeater", true))
		)
		return false;

	ExtensionIsActivated = true;
	return ExtensionIsActivated;
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

	if (ExtensionIsActive() &&
		(nodeTag(parsetree) != T_CopyStmt) &&
		(nodeTag(parsetree) != T_CreateExtensionStmt) &&
		(nodeTag(parsetree) != T_ExplainStmt) &&
		(context != PROCESS_UTILITY_SUBCOMMAND)
	   )
	{
		PGresult *result;

		while ((result = PQgetResult(EstablishConnection())) != NULL);

		if (PQsendQuery(EstablishConnection(), queryString) == 0)
		{
			elog(ERROR, "Sending UTILITY query error: %s", queryString);
			PQreset(conn);
		}
	}
	else
		elog(LOG, "UTILITY query without sending: %s", queryString);

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

		while ((result = PQgetResult(conn)) != NULL);
	}
}
static int IsExecuted = 0;

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node	*parsetree = queryDesc->plannedstmt->utilityStmt;

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	IsExecuted++;

	if (IsExecuted > 1)
		return;

	if (
		ExtensionIsActive() &&
		(repeater_host_name == 0) &&
		((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)) &&
		!(eflags & EXEC_FLAG_EXPLAIN_ONLY)
		)
	{
		elog(LOG, "Send query: %s", queryDesc->sourceText);
		if (execute_query(EstablishConnection(), queryDesc, eflags) == 0)
			PQreset(conn);
	}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	IsExecuted--;
	/* Execute before hook because it destruct memory context of exchange list */
	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL);
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}


/*
 * Serialize plan and send it to the destination instance
 */
static int
execute_query(PGconn *dest, QueryDesc *queryDesc, int eflags)
{
	PGresult *result;

	Assert(dest != NULL);

	/*
	 * Before send of plan we need to check connection state.
	 * If previous query was failed, we get PGRES_FATAL_ERROR.
	 */
	while ((result = PQgetResult(dest)) != NULL);

	if (PQsendPlan(dest, serialize_plan(queryDesc, eflags)) == 0)
	{
		/*
		 * Report about remote execution error and return control to caller.
		 */
		elog(ERROR, "PLAN sending error.");
		return 0;
	}

	return 1;
}
