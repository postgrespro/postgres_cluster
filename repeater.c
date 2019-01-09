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
char	*remote_server_fdwname;

static bool		ExtensionIsActivated = false;
static PGconn	*conn = NULL;

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomStringVariable("repeater.fdwname",
							"Remote host fdw name",
							NULL,
							&remote_server_fdwname,
							"remoteserv",
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

#include "miscadmin.h"
#include "pgstat.h"
#include "storage/latch.h"

#include "foreign/foreign.h"
#include "postgres_fdw.h"

static Oid			serverid = InvalidOid;
static UserMapping	*user = NULL;

static bool
pgfdw_cancel_query(PGconn *conn)
{
	PGcancel   *cancel;
	char		errbuf[256];
	PGresult   *result = NULL;

	if ((cancel = PQgetCancel(conn)))
	{
		if (!PQcancel(cancel, errbuf, sizeof(errbuf)))
		{
			ereport(WARNING,
					(errcode(ERRCODE_CONNECTION_FAILURE),
					 errmsg("could not send cancel request: %s",
							errbuf)));
			PQfreeCancel(cancel);
			return false;
		}

		PQfreeCancel(cancel);
	}
	else
		elog(FATAL, "Can't get connection cancel descriptor");

	PQconsumeInput(conn);
	PQclear(result);

	return true;
}

static void
cancelQueryIfNeeded(PGconn *conn, const char *query)
{
	Assert(conn != NULL);
	Assert(query != NULL);

	if (PQtransactionStatus(conn) != PQTRANS_IDLE)
	{
		PGresult *res;

		printf("CONN status BEFORE EXEC: %d, txs: %d errmsg: %s\n",
										PQstatus(conn),
										PQtransactionStatus(conn),
										PQerrorMessage(conn));

		res = PQgetResult(conn);

		if (PQresultStatus(res) == PGRES_FATAL_ERROR)
			Assert(pgfdw_cancel_query(conn));
		else
			pgfdw_get_result(conn, query);
	}

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

	if (ExtensionIsActive() &&
		pstmt->canSetTag &&
		(context != PROCESS_UTILITY_SUBCOMMAND)
	   )
	{
		if (!user)
		{
			MemoryContext	oldCxt = MemoryContextSwitchTo(TopMemoryContext);

			serverid = get_foreign_server_oid(remote_server_fdwname, true);
			Assert(OidIsValid(serverid));

			user = GetUserMapping(GetUserId(), serverid);
			MemoryContextSwitchTo(oldCxt);
		}
		switch (nodeTag(parsetree))
		{
		case T_CopyStmt:
		case T_CreateExtensionStmt:
		case T_ExplainStmt:
		case T_FetchStmt:
		case T_VacuumStmt:
			break;
		default:
			if (nodeTag(parsetree) == T_TransactionStmt)
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;

				if (
//					(stmt->kind != TRANS_STMT_ROLLBACK_TO) &&
					(stmt->kind != TRANS_STMT_SAVEPOINT)
					)
					break;
			}
			if (conn)
				cancelQueryIfNeeded(conn, queryString);
			conn = GetConnection(user, true);
			cancelQueryIfNeeded(conn, queryString);
			Assert(conn != NULL);

			Assert(PQsendQuery(conn, queryString));
			break;
		};
	}

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, context, params,
									 queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString,
											context, params, queryEnv,
											dest, completionTag);
	if (conn)
		cancelQueryIfNeeded(conn, queryString);
}

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node		*parsetree = queryDesc->plannedstmt->utilityStmt;

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	/*
	 * This not fully correct sign for prevent passing each subquery to
	 * the remote instance. Only for demo.
	 */
		if (ExtensionIsActive() &&
			queryDesc->plannedstmt->canSetTag &&
			((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)) &&
			!(eflags & EXEC_FLAG_EXPLAIN_ONLY))
		{
			Oid			serverid;
			UserMapping	*user;

			serverid = get_foreign_server_oid(remote_server_fdwname, true);
			Assert(OidIsValid(serverid));

			user = GetUserMapping(GetUserId(), serverid);
			conn = GetConnection(user, true);
			cancelQueryIfNeeded(conn, queryDesc->sourceText);

			if (PQsendPlan(conn, serialize_plan(queryDesc, eflags)) == 0)
				pgfdw_report_error(ERROR, NULL, conn, false, queryDesc->sourceText);
		}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	if (conn)
		cancelQueryIfNeeded(conn, queryDesc->sourceText);

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}
