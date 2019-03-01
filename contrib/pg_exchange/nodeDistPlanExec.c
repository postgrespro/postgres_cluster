/*-------------------------------------------------------------------------
 *
 * nodeDistPlanExec.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "common/base64.h"
#include "exchange.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "miscadmin.h"
#include "nodeDistPlanExec.h"
#include "nodes/nodeFuncs.h"
#include "nodes/nodes.h"
#include "nodes/pg_list.h"
#include "postgres_fdw.h"


typedef struct
{
	CustomScanState	css;
	PGconn			**conn;
	int				nconns;
} DPEState;


static CustomPathMethods	distplanexec_path_methods;
static CustomScanMethods	distplanexec_plan_methods;
static CustomExecMethods	distplanexec_exec_methods;


/*
 * Create state of exchange node.
 */
static Node *
CreateDistPlanExecState(CustomScan *node)
{
	DPEState	*state;

	state = (DPEState *) palloc0(sizeof(DPEState));
	NodeSetTag(state, T_CustomScanState);

	state->css.flags = node->flags;
	state->css.methods = &distplanexec_exec_methods;
	state->css.custom_ps = NIL;
	state->conn = NULL;
	state->nconns = 0;

	return (Node *) state;
}

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

static char*
serialize_plan(Plan *plan, const char *sourceText, ParamListInfo params)
{
	char	   *query,
			   *query_container,
			   *splan,
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
	splan = nodeToString(plan);
	set_portable_output(false);
	plen = pg_b64_enc_len(strlen(splan) + 1);
	plan_container = (char *) palloc0(plen + 1);
	plen1 = pg_b64_encode(splan, strlen(splan), plan_container);
	Assert(plen > plen1);

	qlen = pg_b64_enc_len(strlen(sourceText) + 1);
	query_container = (char *) palloc0(qlen + 1);
	qlen1 = pg_b64_encode(sourceText, strlen(sourceText), query_container);
	Assert(qlen > qlen1);

	sparams_len = EstimateParamListSpace(params);
	start_address = sparams = palloc(sparams_len);
	SerializeParamList(params, &start_address);
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

static Plan *
add_pstmt_node(Plan *plan, EState *estate)
{
	PlannedStmt *pstmt;
	ListCell   *lc;

	/* We can't scribble on the original plan, so make a copy. */
	plan = copyObject(plan);

	/*
	 * The worker will start its own copy of the executor, and that copy will
	 * insert a junk filter if the toplevel node has any resjunk entries. We
	 * don't want that to happen, because while resjunk columns shouldn't be
	 * sent back to the user, here the tuples are coming back to another
	 * backend which may very well need them.  So mutate the target list
	 * accordingly.  This is sort of a hack; there might be better ways to do
	 * this...
	 */
	foreach(lc, plan->targetlist)
	{
		TargetEntry *tle = lfirst_node(TargetEntry, lc);

		tle->resjunk = false;
	}

	/*
	 * Create a dummy PlannedStmt.  Most of the fields don't need to be valid
	 * for our purposes, but the worker will need at least a minimal
	 * PlannedStmt to start the executor.
	 */
	pstmt = makeNode(PlannedStmt);
	pstmt->commandType = CMD_SELECT;
	pstmt->queryId = UINT64CONST(0);
	pstmt->hasReturning = false;
	pstmt->hasModifyingCTE = false;
	pstmt->canSetTag = true;
	pstmt->transientPlan = false;
	pstmt->dependsOnRole = false;
	pstmt->parallelModeNeeded = false;
	pstmt->planTree = plan;
	pstmt->rtable = estate->es_range_table;
	pstmt->resultRelations = NIL;
	pstmt->nonleafResultRelations = NIL;

	/*
	 * Transfer only parallel-safe subplans, leaving a NULL "hole" in the list
	 * for unsafe ones (so that the list indexes of the safe ones are
	 * preserved).  This positively ensures that the worker won't try to run,
	 * or even do ExecInitNode on, an unsafe subplan.  That's important to
	 * protect, eg, non-parallel-aware FDWs from getting into trouble.
	 */
	pstmt->subplans = NIL;
	foreach(lc, estate->es_plannedstmt->subplans)
	{
		Plan	   *subplan = (Plan *) lfirst(lc);

		if (subplan && !subplan->parallel_safe)
			subplan = NULL;
		pstmt->subplans = lappend(pstmt->subplans, subplan);
	}

	pstmt->rewindPlanIDs = NULL;
	pstmt->rowMarks = NIL;
	pstmt->relationOids = NIL;
	pstmt->invalItems = NIL;	/* workers can't replan anyway... */
	pstmt->paramExecTypes = estate->es_plannedstmt->paramExecTypes;
	pstmt->utilityStmt = NULL;
	pstmt->stmt_location = -1;
	pstmt->stmt_len = -1;

	/* Return dummy PlannedStmt. */
	return (Plan *) pstmt;
}

static void
BeginDistPlanExec(CustomScanState *node, EState *estate, int eflags)
{
	CustomScan	*cscan = (CustomScan *) node->ss.ps.plan;
	DPEState	*dpe = (DPEState *) node;
	Plan		*subplan;
	PlanState	*subPlanState;
	bool		explain_only = ((eflags & EXEC_FLAG_EXPLAIN_ONLY) != 0);

	Assert(list_length(cscan->custom_plans) == 1);
	elog(LOG, "BeginDistPlanExec");
	/* Initialize subtree */
	subplan = linitial(cscan->custom_plans);
	subPlanState = (PlanState *) ExecInitNode(subplan, estate, eflags);
	node->custom_ps = lappend(node->custom_ps, subPlanState);

	if (!explain_only)
	{
		char	*query;
		int i = 0;
		ListCell	*lc;

		/* The Plan involves foreign servers and uses exchange nodes. */
		if (cscan->custom_private == NIL)
			return;

		dpe->nconns = list_length(cscan->custom_private);
		dpe->conn = palloc(sizeof(PGconn *) * dpe->nconns);
		query = serialize_plan(add_pstmt_node(subplan, estate), estate->es_sourceText, NULL);
		for (lc = list_head(cscan->custom_private); lc != NULL; lc = lnext(lc))
		{
			UserMapping	*user;
			int			res;
			Oid serverid = intVal(lfirst(lc));

			user = GetUserMapping(GetUserId(), serverid);
			dpe->conn[i] = GetConnection(user, true);
			Assert(dpe->conn[i] != NULL);
			res = PQsendQuery(dpe->conn[i], query);
			i++;
			Assert(res == 1);
		}
	}
}

static TupleTableSlot *
ExecDistPlanExec(CustomScanState *node)
{
	PlanState  *outerNode;

	outerNode = (PlanState *) linitial(node->custom_ps);
	return ExecProcNode(outerNode);
}

static void
ExecEndDistPlanExec(CustomScanState *node)
{
	DPEState *dpe = (DPEState *) node;
	int i;

	ExecEndNode(linitial(node->custom_ps));

	for (i = 0; i < dpe->nconns; i++)
	{
		PGresult	*result;

		while ((result = PQgetResult(dpe->conn[i])) != NULL);
		elog(LOG, "ExecEndDistPlanExec: %d", PQresultStatus(result));
	}
	if (dpe->conn)
		pfree(dpe->conn);
}

static void
ExecReScanDistPlanExec(CustomScanState *node)
{
	return;
}

static void
ExplainDistPlanExec(CustomScanState *node, List *ancestors, ExplainState *es)
{
	StringInfoData str;
	List *servers = ((CustomScan *) node->ss.ps.plan)->custom_private;
	ListCell *lc;

	initStringInfo(&str);
	appendStringInfo(&str, "involved %d remote server(s): ", list_length(servers));
	foreach(lc, servers)
	{
		appendStringInfo(&str, "%u ", (Oid) intVal(lfirst(lc)));
	}

	ExplainPropertyText("DistPlanExec", str.data, es);
}

static struct Plan *
CreateDistExecPlan(PlannerInfo *root,
					   RelOptInfo *rel,
					   struct CustomPath *best_path,
					   List *tlist,
					   List *clauses,
					   List *custom_plans)
{
	CustomScan *distExecNode;

	distExecNode = make_distplanexec(custom_plans, tlist, best_path->custom_private);

	distExecNode->scan.plan.startup_cost = best_path->path.startup_cost;
	distExecNode->scan.plan.total_cost = best_path->path.total_cost;
	distExecNode->scan.plan.plan_rows = best_path->path.rows;
	distExecNode->scan.plan.plan_width = best_path->path.pathtarget->width;
	distExecNode->scan.plan.parallel_aware = best_path->path.parallel_aware;
	distExecNode->scan.plan.parallel_safe = best_path->path.parallel_safe;

	return &distExecNode->scan.plan;
}

void
DistExec_Init_methods(void)
{
	/* Initialize path generator methods */
	distplanexec_path_methods.CustomName = "DistExecPath";
	distplanexec_path_methods.PlanCustomPath = CreateDistExecPlan;
	distplanexec_path_methods.ReparameterizeCustomPathByChild	= NULL;

	distplanexec_plan_methods.CustomName 			= "DistExecPlan";
	distplanexec_plan_methods.CreateCustomScanState	= CreateDistPlanExecState;
	RegisterCustomScanMethods(&distplanexec_plan_methods);

	/* setup exec methods */
	distplanexec_exec_methods.CustomName				= "DistExec";
	distplanexec_exec_methods.BeginCustomScan			= BeginDistPlanExec;
	distplanexec_exec_methods.ExecCustomScan			= ExecDistPlanExec;
	distplanexec_exec_methods.EndCustomScan				= ExecEndDistPlanExec;
	distplanexec_exec_methods.ReScanCustomScan			= ExecReScanDistPlanExec;
	distplanexec_exec_methods.MarkPosCustomScan			= NULL;
	distplanexec_exec_methods.RestrPosCustomScan		= NULL;
	distplanexec_exec_methods.EstimateDSMCustomScan  	= NULL;
	distplanexec_exec_methods.InitializeDSMCustomScan 	= NULL;
	distplanexec_exec_methods.InitializeWorkerCustomScan= NULL;
	distplanexec_exec_methods.ReInitializeDSMCustomScan = NULL;
	distplanexec_exec_methods.ShutdownCustomScan		= NULL;
	distplanexec_exec_methods.ExplainCustomScan		= ExplainDistPlanExec;
}

CustomScan *
make_distplanexec(List *custom_plans, List *tlist, List *private_data)
{
	CustomScan	*node = makeNode(CustomScan);
	Plan		*plan = &node->scan.plan;
	ListCell	*lc;

	plan->startup_cost = 0;
	plan->total_cost = 0;
	plan->plan_rows = 0;
	plan->plan_width =0;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	plan->parallel_aware = false;
	plan->parallel_safe = false;
	plan->targetlist = tlist;

	/* Setup methods and child plan */
	node->methods = &distplanexec_plan_methods;
	node->custom_scan_tlist = tlist;
	node->scan.scanrelid = 0;
	node->custom_plans = custom_plans;
	node->custom_exprs = NIL;
	node->custom_private = NIL;

	/* Make Private data list of the plan node */
	foreach(lc, private_data)
	{
		Oid	serverid = *(Oid *)lfirst(lc);

		node->custom_private = lappend(node->custom_private, makeInteger(serverid));
//		elog(INFO, "make serv: %d", serverid);
	}


	return node;
}

Path *
create_distexec_path(PlannerInfo *root, RelOptInfo *rel, Path *children,
					 List *private_data)
{
	CustomPath	*path = makeNode(CustomPath);
	Path		*pathnode = &path->path;

	pathnode->pathtype = T_CustomScan;
	pathnode->parent = rel;
	pathnode->pathtarget = rel->reltarget;
	pathnode->param_info = NULL;

	pathnode->parallel_aware = false; /* permanently */
	pathnode->parallel_safe = false; /* permanently */
	pathnode->parallel_workers = 0; /* permanently */
	pathnode->pathkeys = NIL;

	pathnode->rows = rel->tuples;
	pathnode->startup_cost = 0.0001;
	pathnode->total_cost = 0.0;

	path->flags = 0;
	/* Contains only one path */
	path->custom_paths = lappend(path->custom_paths, children);

	path->custom_private = private_data;
	path->methods = &distplanexec_path_methods;

	return pathnode;
}
