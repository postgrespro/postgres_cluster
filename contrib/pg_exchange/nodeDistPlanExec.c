/*-------------------------------------------------------------------------
 *
 * nodeDistPlanExec.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "arpa/inet.h"
#include "commands/defrem.h"
#include "common.h"
#include "common/base64.h"
#include "exchange.h"
#include "foreign/fdwapi.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "miscadmin.h"
#include "nodeDistPlanExec.h"
#include "nodeDummyscan.h"
#include "nodes/nodeFuncs.h"
#include "nodes/nodes.h"
#include "nodes/pg_list.h"
#include "postgres_fdw.h"
#include "postmaster/postmaster.h"
#include "utils/queryenvironment.h"
#include "utils/rel.h"

#include "stream.h"


typedef struct
{
	CustomScanState	css;
	PGconn			**conn;
	int				nconns;
} DPEState;


static CustomPathMethods	distplanexec_path_methods;
static CustomScanMethods	distplanexec_plan_methods;
static CustomExecMethods	distplanexec_exec_methods;

char destsName[10] = "DMQ_DESTS";


static Node *CreateDistPlanExecState(CustomScan *node);
static char *serialize_plan(Plan *plan, const char *sourceText,
														ParamListInfo params);
static Plan *add_pstmt_node(Plan *plan, EState *estate);
static void BeginDistPlanExec(CustomScanState *node, EState *estate, int eflags);
static TupleTableSlot *ExecDistPlanExec(CustomScanState *node);
static void ExecEndDistPlanExec(CustomScanState *node);
static void ExecReScanDistPlanExec(CustomScanState *node);
static void ExplainDistPlanExec(CustomScanState *node, List *ancestors,
															ExplainState *es);
static struct Plan *
CreateDistExecPlan(PlannerInfo *root, RelOptInfo *rel,struct CustomPath *best_path,
								List *tlist, List *clauses, List *custom_plans);
static void dmq_init_barrier(DMQDestCont *dmq_data, PlanState *child);
static bool init_exchange_channel(PlanState *node, void *context);

Bitmapset *
extractForeignServers(CustomPath *path)
{
	ListCell *lc;
	Bitmapset *servers = NULL;

	foreach(lc, path->custom_private)
	{
		Oid serverid = lfirst_oid(lc);

		Assert(OidIsValid(serverid));
		if (!bms_is_member((int) serverid, servers))
			servers = bms_add_member(servers, serverid);
	}
	return servers;
}

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
	char *host;
	int port;
	char *serverName;

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

	GetMyServerName(&host, &port);
	serverName = serializeServer(host, port);
	query = palloc0(qlen + plen + rlen + strlen(serverName) + 100);
	sprintf(query, "SELECT public.pg_exec_plan('%s', '%s', '%s', '%s');",
						query_container, plan_container, params_container,
						serverName);

	pfree(serverName);
	pfree(host);
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

void
EstablishDMQConnections(const lcontext *context, const char *serverName, PlanState *substate)
{
	int nservers = bms_num_members(context->servers);
	DMQDestCont *dmq_data = palloc(sizeof(DMQDestCont));
	int i = 0;
	EphemeralNamedRelation enr = palloc(sizeof(EphemeralNamedRelationData));
	int coordinator_num = -1;
	int sid = -1;

	dmq_data->nservers = nservers;
	dmq_data->dests = palloc(nservers * sizeof(DMQDestinations));

	LWLockAcquire(ExchShmem->lock, LW_EXCLUSIVE);
	while ((sid = bms_next_member(context->servers, sid)) >= 0)
	{
		bool found;
		DMQDestinations	*sub;
		char senderName[256];
		char receiverName[256];
		char *host;
		int port;

		GetMyServerName(&host, &port);
		sprintf(senderName, "%s-%d", host, port);
		FSExtractServerName((Oid)sid, &host, &port);
		sprintf(receiverName, "%s-%d", host, port);

		/* This foreign server is a coordinator? */
		if (strcmp(serverName, receiverName) == 0)
			coordinator_num = i;

		sub = (DMQDestinations *) hash_search(ExchShmem->htab, &sid,
														HASH_ENTER, &found);
		if (!found)
		{
			char connstr[1024];

			/* Establish new DMQ channel with foreign server */
			sprintf(connstr, "host=%s port=%d "
							 "fallback_application_name=%s",
							 host, port, senderName);

			sub->dest_id = dmq_destination_add(connstr, senderName, receiverName, 10);
			memcpy(sub->node, receiverName, strlen(receiverName) + 1);
		}
		dmq_attach_receiver(receiverName, 0);
		memcpy(&dmq_data->dests[i++], sub, sizeof(DMQDestinations));
	}
	LWLockRelease(ExchShmem->lock);

	/* if coordinator_num == -1 - I'm the Coordinator */
	dmq_data->coordinator_num = coordinator_num;
	dmq_init_barrier(dmq_data, substate);
	/* Add list of destinations in queryEnv */
	if (!context->estate->es_queryEnv)
		context->estate->es_queryEnv = create_queryEnv();
	enr->md.name = destsName;
	enr->reldata = (void *) dmq_data;
	register_ENR(context->estate->es_queryEnv, enr);
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

	/* Initialize subtree */
	subplan = linitial(cscan->custom_plans);
	subPlanState = (PlanState *) ExecInitNode(subplan, estate, eflags);
	node->custom_ps = lappend(node->custom_ps, subPlanState);

	if (!explain_only)
	{
		char	*query;
		int i = 0;
		ListCell	*lc;
		lcontext context;

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
			Oid serverid = lfirst_oid(lc);

			user = GetUserMapping(GetUserId(), serverid);
			dpe->conn[i] = GetConnection(user, true);
			Assert(dpe->conn[i] != NULL);
			res = PQsendQuery(dpe->conn[i], query);
			if (!res)
				pgfdw_report_error(ERROR, NULL, dpe->conn[i], false, query);
			i++;
			Assert(res == 1);
		}
		Assert(i > 0);
		context.estate = estate;
		context.eflags = eflags;
		context.servers = NULL;

		localize_plan(subPlanState, &context);
		Assert(bms_num_members(context.servers) > 0);

		EstablishDMQConnections(&context, " ", subPlanState);
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
		appendStringInfo(&str, "%u ", lfirst_oid(lc));
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
	distplanexec_path_methods.CustomName = DISTEXECPATHNAME;
	distplanexec_path_methods.PlanCustomPath = CreateDistExecPlan;
	distplanexec_path_methods.ReparameterizeCustomPathByChild = NULL;

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
	List *child_tlist;

	plan->startup_cost = 10;
	plan->total_cost = 10;
	plan->plan_rows = 1000;
	plan->plan_width =10;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	plan->parallel_aware = false;
	plan->parallel_safe = false;
	plan->targetlist = tlist;

	/* Setup methods and child plan */
	node->methods = &distplanexec_plan_methods;
	node->scan.scanrelid = 0;
	node->custom_plans = custom_plans;

	child_tlist = ((Plan *)linitial(node->custom_plans))->targetlist;
	node->custom_scan_tlist = child_tlist;
	node->custom_exprs = NIL;
	node->custom_private = NIL;

	/* Make Private data list of the plan node */
	foreach(lc, private_data)
	{
		Oid	serverid = lfirst_oid(lc);
		node->custom_private = lappend_oid(node->custom_private, serverid);
	}


	return node;
}

CustomPath *
create_distexec_path(PlannerInfo *root, RelOptInfo *rel, Path *children,
					 Bitmapset *servers)
{
	CustomPath	*path = makeNode(CustomPath);
	Path		*pathnode = &path->path;
	int member = -1;

	pathnode->pathtype = T_CustomScan;
	pathnode->pathtarget = rel->reltarget;
	pathnode->param_info = NULL;
	pathnode->parent = rel;

	pathnode->parallel_aware = false; /* permanently */
	pathnode->parallel_safe = false; /* permanently */
	pathnode->parallel_workers = 0; /* permanently */
	pathnode->pathkeys = NIL;

	path->flags = 0;
	path->custom_paths = lappend(path->custom_paths, children);
	path->custom_private = NIL;

	while ((member = bms_next_member(servers, member)) >= 0)
		path->custom_private = lappend_oid(path->custom_private, (Oid) member);

	path->methods = &distplanexec_path_methods;

	pathnode->rows = children->rows;
	pathnode->startup_cost = 100.;
	pathnode->total_cost = pathnode->startup_cost;

	return path;
}

bool
localize_plan(PlanState *node, lcontext *context)
{
	if (node == NULL)
		return false;

	check_stack_depth();

	planstate_tree_walker(node, localize_plan, context);

	if (nodeTag(node->plan) == T_Append)
	{
		int i;
		AppendState	*apSt = (AppendState *) node;

		/*
		 * Traverse all APPEND scans and search for foreign partitions. Scan of
		 * foreign partition is replaced by DummyScan node.
		 */
		for (i = 0; i < apSt->as_nplans; i++)
		{
			switch (nodeTag(apSt->appendplans[i]))
			{
			case T_SeqScanState:
			case T_IndexScanState:
			case T_BitmapIndexScanState:
			case T_BitmapHeapScanState:
			{
				ScanState	*ss = (ScanState *) apSt->appendplans[i];
				if (ss->ss_currentRelation->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
				{
					CustomScan *dummyScan = make_dummyscan();
					Oid serverid;

					ss->ps.plan = &dummyScan->scan.plan;
					apSt->appendplans[i] = ExecInitNode(ss->ps.plan, context->estate, context->eflags);

					serverid = GetForeignServerIdByRelId(ss->ss_currentRelation->rd_id);
					if (!bms_is_member((int)serverid, context->servers))
						context->servers = bms_add_member(context->servers, (int)serverid);
					ExecCloseScanRelation(ss->ss_currentRelation);
				}
			}
				break;
			default:
				elog(LOG, "!! Some problems here: tag=%d", nodeTag(apSt->appendplans[i]));
				break;
			}
		}
	}

	return false;
}

const char *LOCALHOST = "localhost";
/*
 * fsid - foreign server oid.
 * host - returns C-string with foreign server host name
 * port - returns foreign server port number.
 */
void
FSExtractServerName(Oid fsid, char **host, int *port)
{
	ForeignServer *server;
	ListCell   *lc;
	char *hostname = NULL;

	server = GetForeignServer(fsid);
	*port = 5432;
	foreach(lc, server->options)
	{
		DefElem    *def = (DefElem *) lfirst(lc);

		if (strcmp(def->defname, "host") == 0)
			hostname = pstrdup(defGetString(def));
		else if (strcmp(def->defname, "port") == 0)
			*port = strtol(defGetString(def), NULL, 10);
	}

	if (!hostname)
		hostname = pstrdup(LOCALHOST);
	*host = hostname;
}

void
GetMyServerName(char **host, int *port)
{
	*host = pstrdup(LOCALHOST);
	*port = PostPortNumber;
}

char*
serializeServer(const char *host, int port)
{
	char *serverName = palloc(256);

	sprintf(serverName, "%s-%d", host, port);
	return serverName;
}

/*
 * All instances call this function and get returned after:
 * 1. connections established
 * 2. subscriptions made.
 * At first, routine wait for 'active' connection status.
 * At second, it send test message by the stream and wait to confirm
 * subscription.
 * Third, it send 'start work' message and wait for confirm delivery.
 */
static void
dmq_init_barrier(DMQDestCont *dmq_data, PlanState *child)
{
	int i;

	/* Wait for dmq connection establishing */
	for (i = 0; i < dmq_data->nservers; i++)
		while (dmq_get_destination_status(dmq_data->dests[i].dest_id) != Active);

	init_exchange_channel(child, (void *) dmq_data);
}

static bool
init_exchange_channel(PlanState *node, void *context)
{
	CustomScanState *css;
	ExchangeState	*state;
	DMQDestCont *dmq_data;
	int i;
	char ib = 'I';

	if (node == NULL)
		return false;

	check_stack_depth();

	planstate_tree_walker(node, init_exchange_channel, context);

	if (nodeTag(node->plan) != T_CustomScan)
		return false;

	css = (CustomScanState *) node;
	if (strcmp(css->methods->CustomName, EXCHANGE_NAME) != 0)
		return false;

	/* It is EXCHANGE node */
	state = (ExchangeState *) css;

	if (state->mode == EXCH_STEALTH)
		/* We can't plan to send or receive any data. */
		return false;

	/*
	 * Do the mapping from current exchange-made partitioning scheme into the
	 * DMQ destinations array.
	 */
	dmq_data = (DMQDestCont *) context;
	for (i = 0; i < state->nnodes; i++)
	{
		int j;

		for (j = 0; j < dmq_data->nservers; j++)
			if (strcmp(state->nodes[i], dmq_data->dests[j].node) == 0)
				break;
		if (j >= dmq_data->nservers)
		{
			/* My node found */
			state->indexes[i] = -1;
			continue;
		}
		else
			state->indexes[i] = j;

		SendByteMessage(dmq_data->dests[j].dest_id, state->stream, ib);
	}
	return false;
}
