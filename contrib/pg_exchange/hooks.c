/* ------------------------------------------------------------------------
 *
 * hooks_exec.c
 *		Executor-related logic of the ParGRES extension.
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "common.h"
//#include "common/base64.h"
#include "connection.h"
#include "exchange.h"
#include "hooks.h"
//#include "libpq/libpq.h"
//#include "libpq-fe.h"
//#include "miscadmin.h"
//#include "nodes/nodeFuncs.h"
#include "optimizer/paths.h"
//#include "postgres_fdw.h"


static set_rel_pathlist_hook_type	prev_set_rel_pathlist_hook = NULL;
static ExecutorStart_hook_type		prev_ExecutorStart_hook = NULL;


static void
HOOK_Baserel_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
				   RangeTblEntry *rte)
{
	if (prev_set_rel_pathlist_hook)
		(*prev_set_rel_pathlist_hook) (root, rel, rti, rte);

	add_exchange_paths(root, rel, rti, rte);
}
/*
static bool
collect_involved_servers(PlanState *node, Bitmapset **servers)
{
	if (node == NULL)
		return false;

	check_stack_depth();

	planstate_tree_walker(node, collect_involved_servers, servers);

	if ((nodeTag(node->plan) == T_CustomScan) &&
		(strcmp(((CustomScanState *) node)->methods->CustomName,
														EXCHANGE_NAME) == 0))
	{
		int i;

		for (i = 0; i < ((ExchangeState *) node)->nsids; i++)
		{
			Oid serverid = ((ExchangeState *) node)->sid[i];

			if (!bms_is_member(serverid, *servers))
				*servers = bms_add_member(*servers, serverid);
		}
		elog(INFO, "Got exchange node!");
	}

	return false;
}
*/
/*
static char*
serialize_plan(QueryDesc *queryDesc)
{
	char	   *query,
			   *query_container,
			   *plan,
			   *plan_container,
			   *sparams,
			   *start_address,
			   *params_container;
	int			qlen,
				qlen1,
				plen,
				plen1,
				rlen,
				rlen1,
				sparams_len;

	set_portable_output(true);
	plan = nodeToString(queryDesc->plannedstmt);
	set_portable_output(false);
	plen = pg_b64_enc_len(strlen(plan) + 1);
	plan_container = (char *) palloc0(plen + 1);
	plen1 = pg_b64_encode(plan, strlen(plan), plan_container);
	Assert(plen > plen1);

	qlen = pg_b64_enc_len(strlen(queryDesc->sourceText) + 1);
	query_container = (char *) palloc0(qlen + 1);
	qlen1 = pg_b64_encode(queryDesc->sourceText, strlen(queryDesc->sourceText), query_container);
	Assert(qlen > qlen1);

	sparams_len = EstimateParamListSpace(queryDesc->params);
	start_address = sparams = palloc(sparams_len);
	SerializeParamList(queryDesc->params, &start_address);
	rlen = pg_b64_enc_len(sparams_len);
	params_container = (char *) palloc0(rlen + 1);
	rlen1 = pg_b64_encode(sparams, sparams_len, params_container);
	Assert(rlen >= rlen1);

	query = palloc0(qlen + plen + rlen + 100);
	sprintf(query, "SELECT public.pg_exec_plan('%s', '%s', '%s');",
						query_container, plan_container, params_container);

	pfree(query_container);
	pfree(plan_container);
	pfree(sparams);
	pfree(params_container);

	return query;
}
*/
/*
static void
send_query_plan(QueryDesc *queryDesc, Bitmapset *servers)
{
	int serverid = -1;
	char *query;

	query = serialize_plan(queryDesc);
	while ((serverid = bms_next_member(servers, serverid)) >= 0)
	{
		UserMapping	*user;
		PGconn		*conn;
		int			res;

		user = GetUserMapping(GetUserId(), (Oid)serverid);
		conn = GetConnection(user, true);
		Assert(conn != NULL);
		res = PQsendQuery(conn, query);
		Assert(res == 1);
	}
}
*/
/*
 * Pass serialized query plan to all involved foreign servers.
 */
static void
HOOK_Executor_start(QueryDesc *queryDesc, int eflags)
{
//	Bitmapset *servers = NULL;

	if (prev_ExecutorStart_hook)
		(*prev_ExecutorStart_hook) (queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	/*
	 * Walk across plan state, see at EXCHANGE each node and collect numbers of
	 * foreign servers involved in the query execution.
	 */
//	collect_involved_servers(queryDesc->planstate, &servers);

	/* The Plan involves foreign servers and uses exchange nodes. */
//	if (servers)
//		send_query_plan(queryDesc, servers);
}

void
EXEC_Hooks_init(void)
{
	EXCHANGE_Init_methods();

	prev_set_rel_pathlist_hook = set_rel_pathlist_hook;
	set_rel_pathlist_hook = HOOK_Baserel_paths;

	prev_ExecutorStart_hook = ExecutorStart_hook;
	ExecutorStart_hook = HOOK_Executor_start;
}

