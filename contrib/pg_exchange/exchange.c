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
#include "dmq.h"
#include "nodes/makefuncs.h"
#include "nodes/plannodes.h"
#include "nodeDistPlanExec.h"
#include "nodeDummyscan.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "common.h"
#include "exchange.h"


typedef struct ExchangeDataNode
{
	ExtensibleNode en;
	uint	nservers;
	Oid		*servers;

	/* Corresponding EXCHANGE nodes from different servers will communicate by
	 * the named dmq-channel (see dmq.c). This name stored in dmqStreamName by
	 * the coordinator.
	 */
	char	*dmqStreamName;
} ExchangeDataNode;

static ExtensibleNodeMethods	exchange_data_methods;
static CustomPathMethods	exchange_path_methods;
static CustomScanMethods	exchange_plan_methods;
static CustomExecMethods	exchange_exec_methods;

int	shardman_instances = -1;
conninfo_t shardman_instances_info[256];
int myNodeNum = -1;
DmqDestinationId	dest_id[1024];


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

/*
 * Serialize node.
 */
static void
ExchangeDataNodeOut(struct StringInfoData *str,
					const struct ExtensibleNode *node)
{
	ExchangeDataNode *exch = (ExchangeDataNode *) node;

	appendStringInfo(str, " :" CppAsString(fldname) " %d", exch->nservers);
}

void
EXCHANGE_Init_methods(void)
{
	exchange_data_methods.extnodename = "ExchangeDataNode";
	exchange_data_methods.node_size = sizeof(ExchangeDataNode);
	exchange_data_methods.nodeOut = ExchangeDataNodeOut;
	//	exchange_data_methods.nodeCopy = ;
	exchange_data_methods.nodeEqual;
	exchange_data_methods.nodeRead;

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
	DistExec_Init_methods();
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

		path = create_exchange_path(root, rel, (Path *) appendPath);
		path = create_distexec_path(root, rel, path, private_data);
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

	exchange = make_exchange(custom_plans, tlist);

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
	char sender_name[1024];
	int i;

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
//	elog(LOG, "Attach receivers %d", shardman_instances);
	for (i = 0; i < shardman_instances; i++)
	{
		if (i == myNodeNum)
			continue;

		sprintf(sender_name, "node-%d", i);
//		elog(LOG, "Attach receiver %d", i);
		dmq_attach_receiver(sender_name, 0);
	}

	if (myNodeNum == 0)
		dmq_stream_subscribe("stream10");
	else
		dmq_stream_subscribe("stream");
}

static bool init1 = false;

static TupleTableSlot *
EXCHANGE_Execute(CustomScanState *node)
{
	ScanState	*scanState = linitial(node->custom_ps);
	DmqSenderId			sndr_id;

	if (!init1)
	{
		elog(LOG, "INITial step");
	if (myNodeNum == 0)
	{
		int *msg = NULL;
		Size len;

		Assert(dmq_pop(&sndr_id, (void **)&msg, &len, PG_UINT64_MAX));
		elog(LOG, "INIT MSG from 1: %d, len=%lu", *msg, len);
	}
	else
	{
		dmq_push_buffer(dest_id[0], "stream10", &shardman_instances, sizeof(shardman_instances));
		elog(LOG, "SEND MSG to 0: %d", shardman_instances);
	}
		init1 = true;
	}

	elog(LOG, "Message test");
	if (myNodeNum == 0)
	{
		elog(LOG, "Send msg");
		dmq_push_buffer(dest_id[1], "stream", &shardman_instances, sizeof(shardman_instances));
	}
	else
	{
		int *msg = NULL;
		Size len;

		elog(LOG, "Wait msg");
		Assert(dmq_pop(&sndr_id, (void **)&msg, &len, PG_UINT64_MAX));
		elog(LOG, "MSG: %d, len=%lu", *msg, len);
	}

	return ExecProcNode(&scanState->ps);
}

static void
EXCHANGE_End(CustomScanState *node)
{
	Assert(list_length(node->custom_ps) == 1);
	ExecEndNode(linitial(node->custom_ps));
elog(LOG, "EXCHANGE_END");
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
