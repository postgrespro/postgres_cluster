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
#include "foreign/foreign.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "miscadmin.h"
#include "optimizer/planner.h"
#include "pgstat.h"
#include "postgres_fdw.h"
#include "storage/latch.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/memutils.h"


PG_MODULE_MAGIC;

void _PG_init(void);

static ProcessUtility_hook_type 	next_ProcessUtility_hook = NULL;
static ExecutorStart_hook_type		prev_ExecutorStart = NULL;
static ExecutorEnd_hook_type		prev_ExecutorEnd = NULL;

static void HOOK_Utility_injection(PlannedStmt *pstmt, const char *queryString,
						ProcessUtilityContext context, ParamListInfo params,
						DestReceiver *dest, char *completionTag);
static void HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags);
static void HOOK_ExecEnd_injection(QueryDesc *queryDesc);

/* Remote instance parameters. */
char	*remote_server_fdwname;

static bool		ExtensionIsActivated = false;
static PGconn	*conn = NULL;

static Oid			serverid = InvalidOid;
static UserMapping	*user = NULL;


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
		!OidIsValid(get_extension_oid("pg_repeater", true))
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
		{
			PGresult *res;
			if (nodeTag(parsetree) == T_TransactionStmt)
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;

				if (
//					(stmt->kind != TRANS_STMT_ROLLBACK_TO) &&
					(stmt->kind != TRANS_STMT_SAVEPOINT)
					)
					break;
			}
			conn = GetConnection(user, true);
			Assert(conn != NULL);

			res = PQexec(conn, queryString);
			PQclear(res);
		}
			break;
		}
	}

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, context, params,
									 dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString,
											context, params,
											dest, completionTag);
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
			char		*query,
						*query_container,
						*plan,
						*plan_container;
			int			qlen, qlen1,
						plen, plen1;
			PGresult	*res;

			serverid = get_foreign_server_oid(remote_server_fdwname, true);
			Assert(OidIsValid(serverid));

			user = GetUserMapping(GetUserId(), serverid);
			conn = GetConnection(user, true);

			set_portable_output(true);
			plan = nodeToString(queryDesc->plannedstmt);
			set_portable_output(false);
			plen = b64_enc_len(plan, strlen(plan) + 1);
			plan_container = (char *) palloc0(plen+1);
			plen1 = b64_encode(plan, strlen(plan), plan_container);
			Assert(plen > plen1);

			qlen = b64_enc_len(queryDesc->sourceText, strlen(queryDesc->sourceText) + 1);
			query_container = (char *) palloc0(qlen+1);
			qlen1 = b64_encode(queryDesc->sourceText, strlen(queryDesc->sourceText), query_container);
			Assert(qlen > qlen1);

			query = palloc0(qlen + plen + 100);
			sprintf(query, "SELECT public.pg_exec_plan('%s', '%s');", query_container, plan_container);

			res = PQexec(conn, query);
			PQclear(res);
			pfree(query);
			pfree(query_container);
			pfree(plan_container);
		}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}
