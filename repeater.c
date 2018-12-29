/*-------------------------------------------------------------------------
 *
 * repeater.c
 * 			Simple demo for remote plan execution patch.
 *
 * Transfer query plan to a remote instance and wait for result.
 * Remote instance parameters (host, port) defines by GUCs.
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 2018-2019, Postgres Professional
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "commands/extension.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "optimizer/planner.h"
#include "tcop/utility.h"
#include "utils/guc.h"

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

/* Remote instance parameters. */
char	*repeater_host_name;
int		repeater_port_number;

static bool		ExtensionIsActivated = false;
static PGconn	*conn = NULL;

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

static PGconn*
EstablishConnection(void)
{
	char	conninfo[1024];

	if (conn != NULL)
		return conn;

	/* Connect to slave and send it a query plan */
	sprintf(conninfo, "host=%s port=%d %c", repeater_host_name, repeater_port_number, '\0');
	conn = PQconnectdb(conninfo);

	if (PQstatus(conn) == CONNECTION_BAD)
		elog(LOG, "Connection error. conninfo: %s", conninfo);
	else
		elog(LOG, "Connection established: host=%s, port=%d", repeater_host_name, repeater_port_number);

	return conn;
}

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

/*
 * We need to send some DML queries for sync database schema to a plan execution
 * at a remote instance.
 */
static void
HOOK_Utility_injection(PlannedStmt *pstmt,
						const char *queryString,
						ProcessUtilityContext context,
						ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest,
						char *completionTag)
{
	Node		*parsetree = pstmt->utilityStmt;
	PGresult	*result;

	/*
	 * Very non-trivial decision about transferring utility query to data nodes.
	 * This exception list used for demonstration and let us to execute some
	 * simple queries.
	 */
	if (ExtensionIsActive() &&
		pstmt->canSetTag &&
		(nodeTag(parsetree) != T_CopyStmt) &&
		(nodeTag(parsetree) != T_CreateExtensionStmt) &&
		(nodeTag(parsetree) != T_ExplainStmt) &&
		(nodeTag(parsetree) != T_FetchStmt) &&
		(context != PROCESS_UTILITY_SUBCOMMAND)
	   )
	{
		/*
		 * Previous query could be completed with error report at this instance.
		 * In this case, we need to prepare connection to the remote instance.
		 */
		while ((result = PQgetResult(EstablishConnection())) != NULL);

		if (PQsendQuery(EstablishConnection(), queryString) == 0)
			elog(ERROR, "Connection error: query: %s, status=%d, errmsg=%s",
					queryString,
					PQstatus(EstablishConnection()),
					PQerrorMessage(EstablishConnection()));
	}

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, context, params,
									 queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString,
											context, params, queryEnv,
											dest, completionTag);

	/*
	 * Check end of query execution at the remote instance.
	 */
	if (conn)
		while ((result = PQgetResult(conn)) != NULL);
}

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node		*parsetree = queryDesc->plannedstmt->utilityStmt;
	PGresult	*result;
	PGconn		*dest = EstablishConnection();

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
elog(LOG, "QUERY: %s", queryDesc->sourceText);
	/*
	 * This not fully correct sign for prevent passing each subquery to
	 * the remote instance. Only for demo.
	 */
		if (ExtensionIsActive() &&
			queryDesc->plannedstmt->canSetTag &&
			((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)) &&
			!(eflags & EXEC_FLAG_EXPLAIN_ONLY))
	{
		/*
		 * Prepare connection.
		 */
		while ((result = PQgetResult(dest)) != NULL);
		elog(LOG, "->QUERY: %s", queryDesc->sourceText);
		if (PQsendPlan(dest, serialize_plan(queryDesc, eflags)) == 0)
			/*
			 * Report about remote execution error.
			 */
			elog(ERROR, "Connection errors during PLAN transferring: status=%d, errmsg=%s",
										PQstatus(dest), PQerrorMessage(dest));
	}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
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
