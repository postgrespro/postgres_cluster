/* ------------------------------------------------------------------------
 *
 * exchange.c
 *		This module contains the EXHCANGE custom node implementation
 *
 *		The EXCHANGE node implement intra- plan node tuples shuffling
 *		between instances by a socket interface implemented by connection.c
 *		module.
 *		Connections each-by-each are established by EXCHANGE_Begin() and in
 *		parallel worker initializer routine.
 *		Connections are closed by EXCHANGE_End().
 *		After receiving NULL slot from local storage EXCHANGE node sends char
 *		'C' to the another. It is not closed connection immediately for
 *		possible rescan() calling.
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
#include "nodes/makefuncs.h"
#include "nodes/plannodes.h"
#include "nodeDistPlanExec.h"
#include "nodeDummyscan.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "common.h"
#include "connection.h"
#include "exchange.h"


static CustomPathMethods	exchange_path_methods;
static CustomScanMethods	exchange_plan_methods;
static CustomExecMethods	exchange_exec_methods;


static Path *create_exchange_path(PlannerInfo *root, RelOptInfo *rel,
								  Path *children, List *private_data);
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

	DUMMYSCAN_Init_methods();
	DistPlanExec_Init_methods();
}



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

	elog(INFO, "INSERT EXCHANGE");

	/* Traverse all possible paths and search for APPEND */
	foreach(lc, rel->pathlist)
	{
		Path		*path = (Path *) lfirst(lc);
		Path		*tmpLocalScanPath = NULL;
		AppendPath	*appendPath = NULL;
		ListCell	*lc1;
		List		*private_data = NIL;

		Assert(path->pathtype != T_MergeAppend); /* Do it later */

		if (path->pathtype != T_Append)
			continue;

		appendPath = makeNode(AppendPath);
		memcpy(appendPath, path, sizeof(AppendPath));
		appendPath->subpaths = NIL;

		/*
		 * Traverse all APPEND subpaths, check for scan-type and search for
		 * foreign scans
		 */
		foreach(lc1, ((AppendPath *)path)->subpaths)
		{
			Path	*subpath = (Path *) lfirst(lc1);
			Path	*tmpPath;
			Oid		*serverid = NULL;

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
				serverid = &subpath->parent->serverid;
				tmpPath = (Path *) makeNode(SeqScan);
				tmpPath = create_seqscan_path(root, subpath->parent, subpath->parent->lateral_relids, 0);
				break;

			default:
				elog(FATAL, "Can't process relpath for pathtype=%d", path->pathtype);
			}

			if (!tmpLocalScanPath)
				tmpLocalScanPath = tmpPath;

			appendPath->subpaths = lappend(appendPath->subpaths, tmpPath);
			if (serverid)
				private_data = lappend(private_data, serverid);
		}

		if (private_data == NIL)
		{
			pfree(appendPath);
			elog(INFO, "NO one foreign source found");
			continue;
		}

		path = create_exchange_path(root, rel, (Path *) appendPath, private_data);
		path = create_distexec_path(root, rel, path);
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
	path->startup_cost = 0.0001;
	path->total_cost = path->startup_cost + 0.0001 * path->rows;
}

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

	exchange = make_exchange(custom_plans, tlist, best_path->custom_private);

	exchange->scan.plan.startup_cost = best_path->path.startup_cost;
	exchange->scan.plan.total_cost = best_path->path.total_cost;
	exchange->scan.plan.plan_rows = best_path->path.rows;
	exchange->scan.plan.plan_width = best_path->path.pathtarget->width;
	exchange->scan.plan.parallel_aware = best_path->path.parallel_aware;
	exchange->scan.plan.parallel_safe = best_path->path.parallel_safe;

	return &exchange->scan.plan;
}

/*
 * Create EXCHANGE path.
 * custom_private != NIL - exchange node is a leaf and it is needed to create
 * subtree for scanning of base relations.
 */
static Path *
create_exchange_path(PlannerInfo *root, RelOptInfo *rel, Path *children,
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

	cost_exchange(root, rel, pathnode); /* Change at next step*/

	path->flags = 0;
	/* Contains only one path */
	path->custom_paths = lappend(path->custom_paths, children);

	path->custom_private = private_data;
	path->methods = &exchange_path_methods;

	return pathnode;
}

/*
 * Make Exchange plan node from path, generated by create_exchange_path() routine
 */
CustomScan *
make_exchange(List *custom_plans, List *tlist, List *private_data)
{
	CustomScan	*node = makeNode(CustomScan);
	Plan		*plan = &node->scan.plan;
	ListCell	*lc;

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

	/* Make Private data list of EXCHANGE plan node */
	foreach(lc, private_data)
	{
		Oid	serverid = *(Oid *)lfirst(lc);
		node->custom_private = lappend(node->custom_private, makeInteger(serverid));
//		elog(INFO, "make serv: %d", serverid);
	}

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
	int i;
	int ncells;

	state = (ExchangeState *) palloc0(sizeof(ExchangeState));
	NodeSetTag(state, T_CustomScanState);

	state->css.flags = node->flags;
	state->css.methods = &exchange_exec_methods;

	private_data = node->custom_private;
	lc = list_head(private_data);
	ncells = list_length(private_data);
	state->sid = palloc(sizeof(Oid) * ncells);
	state->nsids = ncells;

	for (i = 0; i < ncells; i++)
	{
		Assert(lc != NULL);
		state->sid[i] = (Oid)intVal(lfirst(lc));
		lc = lnext(lc);
//		elog(INFO, "SERVERID %d/%d: %u", i+1, ncells, state->sid[i]);
	}

	return (Node *) state;
}

#include "utils/rel.h"


static void
EXCHANGE_Begin(CustomScanState *node, EState *estate, int eflags)
{
	CustomScan	*cscan = (CustomScan *) node->ss.ps.plan;
	Plan		*scan_plan;
	bool		explain_only = ((eflags & EXEC_FLAG_EXPLAIN_ONLY) != 0);
	AppendState	*appendState;

	Assert(list_length(cscan->custom_plans) == 1);

	scan_plan = linitial(cscan->custom_plans);
	appendState = (AppendState *) ExecInitNode(scan_plan, estate, eflags);
	Assert(appendState->ps.type == T_AppendState);
	node->custom_ps = lappend(node->custom_ps, appendState);

	if (!explain_only)
	{
		/*
		 * Localize plan. Traverse all APPEND scans and search for foreign
		 * partitions. Scan of foreign parttion is replaced by NullScan node.
		 */
		int i;

		for(i = 0; i < appendState->as_nplans; i++)
		{
			ScanState	*scanState = (ScanState *) appendState->appendplans[i];

			if (scanState->ss_currentRelation->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
			{
				CustomScan *dummyScan = make_dummyscan();

				ExecCloseScanRelation(scanState->ss_currentRelation);
				scanState->ps.plan = &dummyScan->scan.plan;
				appendState->appendplans[i] = ExecInitNode(scanState->ps.plan, estate, eflags);
			}
		}
	}
//	elog(LOG, "EXCHANGE BEGIN: pid=%d", getpid());
	/*
	 * Next, initialize ExchangeState
	 */
}

static TupleTableSlot *
EXCHANGE_Execute(CustomScanState *node)
{
	ScanState	*scanState = linitial(node->custom_ps);

	return ExecProcNode(&scanState->ps);
}

static void
EXCHANGE_End(CustomScanState *node)
{
	Assert(list_length(node->custom_ps) == 1);
	ExecEndNode(linitial(node->custom_ps));
elog(INFO, "EXCHANGE_END");
	/*
	 * Clean out exchange state
	 */
}

static void
EXCHANGE_Rescan(CustomScanState *node)
{
	PlanState		*outerPlan = outerPlanState(node);

	if (outerPlan->chgParam == NULL)
		ExecReScan(outerPlan);
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

	initStringInfo(&str);
	ExplainPropertyText("Exchange", str.data, es);
}

static Size
EXCHANGE_EstimateDSM(CustomScanState *node, ParallelContext *pcxt)
{
	return sizeof(ConnInfoPool) * pcxt->nworkers;
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
