/* ------------------------------------------------------------------------
 *
 * exchange.c
 *		This module contains the EXHCANGE custom node implementation
 *
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"
#include "unistd.h"

#include "access/hash.h"
#include "access/htup_details.h"
#include "catalog/pg_am.h"
#include "catalog/pg_opclass.h"
#include "commands/defrem.h"
#include "dmq.h"
#include "nodes/makefuncs.h"
#include "nodes/nodes.h"
#include "nodes/plannodes.h"
#include "nodeDistPlanExec.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "common.h"
#include "exchange.h"
#include "stream.h"


//static ExtensibleNodeMethods	exchange_data_methods;
static CustomPathMethods	exchange_path_methods;
static CustomScanMethods	exchange_plan_methods;
static CustomExecMethods	exchange_exec_methods;


static Path *create_exchange_path(PlannerInfo *root, RelOptInfo *rel,
								  Path *children);
static struct Plan * ExchangePlanCustomPath(PlannerInfo *root,
					   RelOptInfo *rel,
					   struct CustomPath *best_path,
					   List *tlist,
					   List *clauses,
					   List *custom_plans);

static void EXCHANGE_Begin(CustomScanState *node, EState *estate, int eflags);
static TupleTableSlot *EXCHANGE_Execute(CustomScanState *node);
static void EXCHANGE_End(CustomScanState *node);
static void EXCHANGE_Rescan(CustomScanState *node);
static void EXCHANGE_ReInitializeDSM(CustomScanState *node,
									 ParallelContext *pcxt,
									 void *coordinate);
static void EXCHANGE_Explain(CustomScanState *node, List *ancestors,
							 ExplainState *es);
static Size EXCHANGE_EstimateDSM(CustomScanState *node, ParallelContext *pcxt);
static void EXCHANGE_InitializeDSM(CustomScanState *node, ParallelContext *pcxt,
								   void *coordinate);
static void EXCHANGE_InitializeWorker(CustomScanState *node,
									  shm_toc *toc,
									  void *coordinate);
static Node *EXCHANGE_Create_state(CustomScan *node);


#define END_OF_TUPLES	'E'
#define END_OF_EXCHANGE 'Q'
void
EXCHANGE_Init_methods(void)
{
	/* Initialize path generator methods */
	exchange_path_methods.CustomName = "Exchange";
	exchange_path_methods.PlanCustomPath = ExchangePlanCustomPath;
	exchange_path_methods.ReparameterizeCustomPathByChild	= NULL;

	exchange_plan_methods.CustomName 			= "ExchangePlan";
	exchange_plan_methods.CreateCustomScanState	= EXCHANGE_Create_state;
	RegisterCustomScanMethods(&exchange_plan_methods);

	/* setup exec methods */
	exchange_exec_methods.CustomName				= EXCHANGE_NAME;
	exchange_exec_methods.BeginCustomScan			= EXCHANGE_Begin;
	exchange_exec_methods.ExecCustomScan			= EXCHANGE_Execute;
	exchange_exec_methods.EndCustomScan				= EXCHANGE_End;
	exchange_exec_methods.ReScanCustomScan			= EXCHANGE_Rescan;
	exchange_exec_methods.MarkPosCustomScan			= NULL;
	exchange_exec_methods.RestrPosCustomScan		= NULL;
	exchange_exec_methods.EstimateDSMCustomScan  	= EXCHANGE_EstimateDSM;
	exchange_exec_methods.InitializeDSMCustomScan 	= EXCHANGE_InitializeDSM;
	exchange_exec_methods.InitializeWorkerCustomScan= EXCHANGE_InitializeWorker;
	exchange_exec_methods.ReInitializeDSMCustomScan = EXCHANGE_ReInitializeDSM;
	exchange_exec_methods.ShutdownCustomScan		= NULL;
	exchange_exec_methods.ExplainCustomScan			= EXCHANGE_Explain;

	DistExec_Init_methods();
}

#include "nodes/relation.h"
/*
 * Add one path for a base relation target:  replace all ForeignScan nodes by
 * local Scan nodes.
 * Assumptions:
 * 1. If the planner chooses this type of scan for one partition of the relation,
 * then the same type of scan must be chosen for any other partition of this
 * relation.
 * 2. Type of scan chosen for local partition of a relation will be correct and
 * optimal for any foreign partition of the same relation.
 */
void
add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti, RangeTblEntry *rte)
{
	ListCell   *lc;

	if (!rte->inh)
		/*
		 * Relation is not contain any partitions.
		 */
		return;

	elog(INFO, "INSERT EXCHANGE. paths: %d", list_length(rel->pathlist));

	/* Traverse all possible paths and search for APPEND */
	foreach(lc, rel->pathlist)
	{
		Path		*path = (Path *) lfirst(lc);
		Path		*tmpLocalScanPath = NULL;
		AppendPath	*appendPath = NULL;
		ListCell	*lc1;
		Bitmapset	*servers = NULL;
		List	*subpaths = NIL;

		if (path->pathtype != T_Append)
			continue;

//		elog(INFO, "-> IE. path params: %hhu, ptype: %d, tcost: %f, scost: %f",
//				path->param_info != NULL, path->pathtype,
//				path->total_cost, path->startup_cost);

		/*
		 * Traverse all APPEND subpaths, check for scan-type and search for
		 * foreign scans
		 */
		foreach(lc1, ((AppendPath *)path)->subpaths)
		{
			Path	*subpath = (Path *) lfirst(lc1);
			Path	*tmpPath;
			Oid		serverid = InvalidOid;
			elog(INFO, "--> IE. subpath params: %hhu, ptype: %d, tcost: %f, scost: %f",
					subpath->param_info != NULL, subpath->pathtype,
					subpath->total_cost, subpath->startup_cost);
			if ((subpath->pathtype != T_ForeignScan) && (tmpLocalScanPath))
				/* Check assumption No.1 */
				Assert(tmpLocalScanPath->pathtype == subpath->pathtype);

			switch (subpath->pathtype)
			{
			case T_SeqScan:
				tmpPath = (Path *) makeNode(SeqScan);
				memcpy(tmpPath, subpath, sizeof(SeqScan));
				break;

			case T_ForeignScan:
				serverid = subpath->parent->serverid;
				if (PATH_REQ_OUTER(subpath) != NULL)
					continue;
				tmpPath = (Path *) makeNode(SeqScan);
				tmpPath = create_seqscan_path(root, subpath->parent,
						PATH_REQ_OUTER(subpath), 0);
				break;

			default:
				elog(FATAL, "Can't process relpath for pathtype=%d", path->pathtype);
			}

			if (!tmpLocalScanPath)
				tmpLocalScanPath = tmpPath;

			subpaths = lappend(subpaths, tmpPath);
//			appendPath->subpaths = lappend(appendPath->subpaths, tmpPath);
			if (OidIsValid(serverid) && !bms_is_member((int)serverid, servers))
				servers = bms_add_member(servers, serverid);
		}

		if (servers == NULL)
		{
			elog(INFO, "NO one foreign source found");
			continue;
		}
		else
			elog(INFO, "Source found: %d", bms_num_members(servers));

		appendPath = create_append_path(root, rel, subpaths, NIL,
								PATH_REQ_OUTER(tmpLocalScanPath), 0, false,
								((AppendPath *)path)->partitioned_rels, -1);
		path = create_exchange_path(root, rel, (Path *) appendPath);
		path = create_distexec_path(root, rel, path, servers);
		add_path(rel, path);
	}
}

static void
cost_exchange(PlannerInfo *root, RelOptInfo *baserel, Path *path)
{
	if (baserel->pages == 0 && baserel->tuples == 0)
	{
		baserel->pages = 10;
		baserel->tuples =
				(10 * BLCKSZ) / (baserel->reltarget->width +
									 MAXALIGN(SizeofHeapTupleHeader));
	}

	/* Estimate baserel size as best we can with local statistics. */
	set_baserel_size_estimates(root, baserel);

	/* Now I do not want to think about cost estimations. */
	path->rows = baserel->tuples;
	path->startup_cost = 10000.0001;
	path->total_cost = path->startup_cost + 100000.0001 * path->rows;
}

/* XXX: Need to be placed in shared memory */
static uint32 exchange_counter = 0;

/*
 * Create and Initialize plan structure of EXCHANGE-node. It will serialized
 * and deserialized at some instances and convert to an exchange state.
 */
static struct Plan *
ExchangePlanCustomPath(PlannerInfo *root,
					   RelOptInfo *rel,
					   struct CustomPath *best_path,
					   List *tlist,
					   List *clauses,
					   List *custom_plans)
{
	CustomScan *exchange;
	char *host;
	int port;
	char *streamName = palloc(256);

	exchange = make_exchange(custom_plans, tlist);

	exchange->scan.plan.startup_cost = best_path->path.startup_cost;
	exchange->scan.plan.total_cost = best_path->path.total_cost;
	exchange->scan.plan.plan_rows = best_path->path.rows;
	exchange->scan.plan.plan_width = best_path->path.pathtarget->width;
	exchange->scan.plan.parallel_aware = best_path->path.parallel_aware;
	exchange->scan.plan.parallel_safe = best_path->path.parallel_safe;

	/* Add stream name into private field*/
	GetMyServerName(&host, &port);
	sprintf(streamName, "%s-%d-%d", host, port, exchange_counter++);
	exchange->custom_private = lappend(exchange->custom_private, makeString(streamName));

	return &exchange->scan.plan;
}

/*
 * Create EXCHANGE path.
 * custom_private != NIL - exchange node is a leaf and it is needed to create
 * subtree for scanning of base relations.
 */
static Path *
create_exchange_path(PlannerInfo *root, RelOptInfo *rel, Path *children)
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

	cost_exchange(root, rel, pathnode); /* Change at next step*/

	path->flags = 0;
	/* Contains only one path */
	path->custom_paths = lappend(path->custom_paths, children);

	path->custom_private = NIL;
	path->methods = &exchange_path_methods;

	return pathnode;
}

/*
 * Make Exchange plan node from path, generated by create_exchange_path() routine
 */
CustomScan *
make_exchange(List *custom_plans, List *tlist)
{
	CustomScan	*node = makeNode(CustomScan);
	Plan		*plan = &node->scan.plan;

	plan->startup_cost = 1;
	plan->total_cost = 1;
	plan->plan_rows = 1;
	plan->plan_width =1;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	plan->parallel_aware = false; /* Use Shared Memory in parallel worker */
	plan->parallel_safe = false;
	plan->targetlist = tlist;

	/* Setup methods and child plan */
	node->methods = &exchange_plan_methods;
	node->custom_scan_tlist = tlist;
	node->scan.scanrelid = 0;
	node->custom_plans = custom_plans;
	node->custom_exprs = NIL;
	node->custom_private = NIL;

	return node;
}

/*
 * Create state of exchange node.
 */
static Node *
EXCHANGE_Create_state(CustomScan *node)
{
	ExchangeState	*state;
	ListCell		*lc;
	List			*private_data;

	state = (ExchangeState *) palloc0(sizeof(ExchangeState));
	NodeSetTag(state, T_CustomScanState);

	state->css.flags = node->flags;
	state->css.methods = &exchange_exec_methods;
	state->estate = NULL;

	private_data = node->custom_private;
	lc = list_head(private_data);
	Assert(list_length(private_data) == 1);
	strcpy(state->stream, strVal(lfirst(lc)));
	return (Node *) state;
}

#include "utils/rel.h"

static void
EXCHANGE_Begin(CustomScanState *node, EState *estate, int eflags)
{
	CustomScan	*cscan = (CustomScan *) node->ss.ps.plan;
	Plan		*scan_plan;
//	bool		explain_only = ((eflags & EXEC_FLAG_EXPLAIN_ONLY) != 0);
	PlanState	*planState;
	ExchangeState *state = (ExchangeState *) node;
	TupleDesc	scan_tupdesc;

	Assert(list_length(cscan->custom_plans) == 1);

	scan_plan = linitial(cscan->custom_plans);
	planState = (PlanState *) ExecInitNode(scan_plan, estate, eflags);
	node->custom_ps = lappend(node->custom_ps, planState);

	Stream_subscribe(state->stream);

	state->init = false;
	state->ltuples = 0;
	state->rtuples = 0;

	/* Need to access to QueryEnv which will initialized later. */
	state->estate = estate;

	scan_tupdesc = ExecTypeFromTL(scan_plan->targetlist, false);
	ExecInitScanTupleSlot(estate, &node->ss, scan_tupdesc);
}

static DmqDestinationId
distribution_fn_gather(TupleTableSlot *slot, DMQDestCont *dcont)
{
	if (dcont->coordinator_num >= 0)
		return dcont->dests[dcont->coordinator_num].dest_id;
	else
		return -1;
}

static void
init_state_ifany(ExchangeState *state)
{
	if (!state->init)
	{
		EphemeralNamedRelation enr = get_ENR(state->estate->es_queryEnv, destsName);

		Assert(enr != NULL && enr->reldata != NULL);
		state->dests = (DMQDestCont *) enr->reldata;
		state->hasLocal = true;
		state->activeRemotes = state->dests->nservers;
		state->init = true;
//		elog(INFO, "[%d] EXCHANGE Init", getpid());
	}

}

static TupleTableSlot *
EXCHANGE_Execute(CustomScanState *node)
{
	ScanState	*ss = &node->ss;
	ScanState	*subPlanState = linitial(node->custom_ps);
	ExchangeState *state = (ExchangeState *) node;
	bool readRemote = true;

	init_state_ifany(state);

	for(;;)
	{
		TupleTableSlot *slot = NULL;
		DmqDestinationId dest;

		readRemote = !readRemote;

		if ((state->activeRemotes > 0) && readRemote)
		{
			int status;

			slot = RecvTuple(ss->ss_ScanTupleSlot->tts_tupleDescriptor,
							 state->stream, &status);
			switch (status)
			{
			case -1:
				/* No tuples currently */
				break;
			case 0:
				Assert(!TupIsNull(slot));
				state->rtuples++;
				return slot;
			case 1:
				state->activeRemotes--;
				break;
			case 2: /* Close EXCHANGE channel */
				break;
			default:
				Assert(0);
			}
		}

		if ((state->hasLocal) && (!readRemote))
		{
			slot = ExecProcNode(&subPlanState->ps);
			if (TupIsNull(slot))
			{
				int i;
//				elog(LOG, "[%s] FINISH Local store: l=%d, r=%d", state->stream, state->ltuples, state->rtuples);
				for (i = 0; i < state->dests->nservers; i++)
					SendByteMessage(state->dests->dests[i].dest_id, state->stream, END_OF_TUPLES);
				state->hasLocal = false;
				continue;
			}
			else
			{
//				elog(LOG, "Got from Local store: l=%d.", state->ltuples);
				state->ltuples++;
			}
		}

		if ((state->activeRemotes == 0) && (!state->hasLocal))
		{
			elog(LOG, "[%s] Exchange returns NULL: %d %d", state->stream,
					state->ltuples, state->rtuples);
			return NULL;
		}

		if (TupIsNull(slot))
			continue;

		dest = distribution_fn_gather(slot, state->dests);
//		elog(LOG, "Distribute: %d", dest);
		if (dest < 0)
			return slot;
		else
		{
			SendTuple(dest, state->stream, slot);
		}
	}
	return NULL;
}

static void
EXCHANGE_End(CustomScanState *node)
{
	ExchangeState *state = (ExchangeState *) node;

	Assert(list_length(node->custom_ps) == 1);
	ExecEndNode(linitial(node->custom_ps));
	Stream_unsubscribe(state->stream);

	elog(INFO, "EXCHANGE_END");
}

static void
EXCHANGE_Rescan(CustomScanState *node)
{
	ExchangeState *state = (ExchangeState *) node;
	PlanState *subPlan = (PlanState *) linitial(node->custom_ps);

	init_state_ifany(state);
elog(INFO, "Rescan exchange! %d", getpid());
	if (subPlan->chgParam == NULL)
		ExecReScan(subPlan);
	state->activeRemotes = state->dests->nservers;
	state->ltuples = 0;
	state->rtuples = 0;
	state->hasLocal = true;
}

static void
EXCHANGE_ReInitializeDSM(CustomScanState *node, ParallelContext *pcxt,
		  	  	  	  	 void *coordinate)
{
	/* ToDo */
	elog(LOG, "I am in ReInitializeDSM()!");
	Assert(0);
}

static void
EXCHANGE_Explain(CustomScanState *node, List *ancestors, ExplainState *es)
{
	StringInfoData		str;
	ExchangeState *state = (ExchangeState *) node;

	initStringInfo(&str);
	appendStringInfo(&str, "stream: %s. ", state->stream);
	ExplainPropertyText("Exchange", str.data, es);
}

static Size
EXCHANGE_EstimateDSM(CustomScanState *node, ParallelContext *pcxt)
{
	return 0;
}

static void
EXCHANGE_InitializeDSM(CustomScanState *node, ParallelContext *pcxt,
					   void *coordinate)
{
	/*
	 * coordinate - pointer to shared memory segment.
	 * node->pscan_len - size of the coordinate - is defined by
	 * EstimateDSMCustomScan() function.
	 */
}

static void
EXCHANGE_InitializeWorker(CustomScanState *node,
						  shm_toc *toc,
						  void *coordinate)
{

}
