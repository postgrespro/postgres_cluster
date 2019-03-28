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
#include "catalog/pg_class.h"
#include "catalog/pg_opclass.h"
#include "commands/defrem.h"
#include "dmq.h"
#include "nodes/makefuncs.h"
#include "nodes/nodes.h"
#include "nodes/plannodes.h"
#include "nodes/readfuncs.h"
#include "nodes/relation.h"
#include "nodeDistPlanExec.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "partitioning/partbounds.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"

#include "common.h"
#include "exchange.h"
#include "partutils.h"
#include "stream.h"


static CustomPathMethods		exchange_path_methods;
static CustomScanMethods		exchange_plan_methods;
static CustomExecMethods		exchange_exec_methods;
static ExtensibleNodeMethods	exchange_plan_private;

/* XXX: Need to be placed in shared memory */
uint32 exchange_counter = 0;

static void nodeOutExchangePrivate(struct StringInfoData *str,
								   const struct ExtensibleNode *node);
static void nodeReadExchangePrivate(struct ExtensibleNode *node);
static struct Plan * ExchangePlanCustomPath(PlannerInfo *root,
					   RelOptInfo *rel,
					   struct CustomPath *best_path,
					   List *tlist,
					   List *clauses,
					   List *custom_plans);

static Node *EXCHANGE_Create_state(CustomScan *node);
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

static void create_gather_dfn(EPPNode *epp, RelOptInfo *rel);
static void create_stealth_dfn(EPPNode *epp, RelOptInfo *rel, PlannerInfo *root);
static void create_shuffle_dfn(EPPNode *epp, RelOptInfo *rel, PlannerInfo *root);


/*
 * Serialize extensible node with EXCHANGE distribution data.
 */
static void
nodeOutExchangePrivate(struct StringInfoData *str,
					   const struct ExtensibleNode *node)
{
	EPPNode *planNode = (EPPNode *) node;
	int i;

	Assert(str && node);

	appendStringInfo(str, " :nnodes %d", planNode->nnodes);
	appendStringInfoString(str, " :nodenames");
	for (i = 0; i < planNode->nnodes; i++)
		appendStringInfo(str, " %s", planNode->nodes[i]);

	appendStringInfo(str, " :natts %d", planNode->natts);

	appendStringInfoString(str, " :funcids");
	for (i = 0; i < planNode->natts; i++)
		appendStringInfo(str, " %u", planNode->funcid[i]);

	appendStringInfoString(str, " :att_exprs ");
	outNode(str, planNode->att_exprs);

	appendStringInfo(str, " :mode %d", planNode->mode);
}

/*
 * Deserialize extensible node with EXCHANGE distribution data.
 */
static void
nodeReadExchangePrivate(struct ExtensibleNode *node)
{
	EPPNode *planNode = (EPPNode *) node;
	char *token;
	int length;
	int i;

	Assert(planNode);

	/* nnodes field */
	token = pg_strtok(&length);		/* skip :fldname */
	token = pg_strtok(&length);
	planNode->nnodes = atoi(token);

	/* array of node names */
	planNode->nodes = palloc0(sizeof(NodeName) * planNode->nnodes);
	token = pg_strtok(&length);
	for (i = 0; i < planNode->nnodes; i++)
	{
		char *str;

		token = pg_strtok(&length);
		str = debackslash(token, length);
		memcpy(planNode->nodes[i], str, strlen(str));
	}

	/* natts field */
	token = pg_strtok(&length);		/* skip :fldname */
	token = pg_strtok(&length);
	planNode->natts = atoi(token);

	/* funcid array */
	token = pg_strtok(&length);		/* skip :fldname */
	planNode->funcid = readOidCols(planNode->natts);

	/* att_exprs field */
	token = pg_strtok(&length);		/* skip :fldname */
	planNode->att_exprs = nodeRead(NULL, 0);

	/* mode field */
	token = pg_strtok(&length);		/* skip :fldname */
	token = pg_strtok(&length);
	planNode->mode = atoi(token);
}

void
EXCHANGE_Init_methods(void)
{
	/* Initialize path generator methods */
	exchange_path_methods.CustomName = EXCHANGEPATHNAME;
	exchange_path_methods.PlanCustomPath = ExchangePlanCustomPath;
	exchange_path_methods.ReparameterizeCustomPathByChild	= NULL;

	exchange_plan_methods.CustomName = "ExchangePlan";
	exchange_plan_methods.CreateCustomScanState	= EXCHANGE_Create_state;
	RegisterCustomScanMethods(&exchange_plan_methods);

	exchange_plan_private.extnodename = EXCHANGE_PRIVATE_NAME;
	exchange_plan_private.node_size = sizeof(EPPNode);
	exchange_plan_private.nodeCopy = NULL;
	exchange_plan_private.nodeEqual = NULL;
	exchange_plan_private.nodeOut = nodeOutExchangePrivate;
	exchange_plan_private.nodeRead = nodeReadExchangePrivate;
	RegisterExtensibleNodeMethods(&exchange_plan_private);

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

		/*
		 * Traverse all APPEND subpaths, check for scan-type and search for
		 * foreign scans
		 */
		foreach(lc1, ((AppendPath *) path)->subpaths)
		{
			Path	*subpath = (Path *) lfirst(lc1);
			Path	*tmpPath;
			Oid		serverid = InvalidOid;

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

			if (OidIsValid(serverid) && !bms_is_member((int)serverid, servers))
				servers = bms_add_member(servers, serverid);
		}

		if (servers == NULL)
			continue;

		appendPath = create_append_path(root, rel, subpaths, NIL,
								PATH_REQ_OUTER(tmpLocalScanPath), 0, false,
								((AppendPath *) path)->partitioned_rels, -1);
		path = (Path *) create_exchange_path(root, rel, (Path *) appendPath, true);
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
//	set_baserel_size_estimates(root, baserel);

	/* Now I do not want to think about cost estimations. */
	path->rows = baserel->tuples;
	path->startup_cost = 10000.0001;
	path->total_cost = path->startup_cost + 100000.0001 * path->rows;
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
	char *host;
	int port;
	char *streamName = palloc(256);
	ExchangePath *path = (ExchangePath *) best_path;
	RelOptInfo *altrel = &path->altrel;
	EPPNode *private = (EPPNode *) newNode(sizeof(EPPNode), T_ExtensibleNode);

	exchange = make_exchange(custom_plans, tlist);
	private->node.extnodename = EXCHANGE_PRIVATE_NAME;

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
	Assert(altrel->nparts > 0);

	/*
	 * Compute and fill private data structure. It is used connection
	 * info and a tuple distribution rules.
	 */
	if (path->mode == GATHER_MODE)
		create_gather_dfn(private, altrel);
	else if (path->mode == STEALTH_MODE)
		create_stealth_dfn(private, altrel, root);
	else if (path->mode == SHUFFLE_MODE)
	{
		create_shuffle_dfn(private, altrel, root);
//		elog(INFO, "Shuffle DFN: stream=%s", streamName);
	}
	else
		Assert(0);

	exchange->custom_exprs = private->att_exprs;
	exchange->custom_private = lappend(exchange->custom_private, private);
	return &exchange->scan.plan;
}

/*
 * Create distribution data for EXCHANGE plan node. It is based on altrel
 * RelOptInfo structure of exchange path node.
 */
static void
create_gather_dfn(EPPNode *epp, RelOptInfo *rel)
{
	char *hostname;
	int port;
	int partno;

	epp->mode = GATHER_MODE;

	/*
	 * Do not create partition scheme. In the GATHER mode all tuples will
	 * transfer to the coordinator node. None distribution computations needed.
	 */
	epp->natts = 0;
	epp->funcid = NULL;
	epp->att_exprs = NIL;

	/* nparts of altrel represents real or virtual partitions (after JOIN) */
	epp->nnodes = rel->nparts;
	epp->nodes = (NodeName *) palloc(sizeof(NodeName) * epp->nnodes);

	for(partno = 0; partno < epp->nnodes; partno++)
	{
		RelOptInfo *part_rel = rel->part_rels[partno];

		if (OidIsValid(part_rel->serverid))
			/* Foreign relation */
			FSExtractServerName(part_rel->serverid, &hostname, &port);
		else
			GetMyServerName(&hostname, &port);

		createNodeName(epp->nodes[partno], hostname, port);
	}
}

/*
 * Do the distribution function of EXCHANGE node in accordance with current
 * RelOptInfo relation partitioning. In this case all tuples will go up.
 */
static void
create_stealth_dfn(EPPNode *epp, RelOptInfo *rel, PlannerInfo *root)
{
	int partno;
	char *hostname;
	int port;

	epp->mode = STEALTH_MODE;

	/*
	 * Do not create partition scheme. In the STEALTH mode all tuples will
	 * transfer up to next node. None distribution computations needed.
	 */
	epp->natts = 0;
	epp->funcid = NULL;
	epp->att_exprs = NIL;

	epp->nnodes = 0;
	epp->nodes = NULL;

	/* nparts of altrel represents real or virtual partitions (after JOIN) */
	epp->nnodes = rel->nparts;
	epp->nodes = (NodeName *) palloc(sizeof(NodeName) * epp->nnodes);

	for(partno = 0; partno < epp->nnodes; partno++)
	{
		RelOptInfo *part_rel = rel->part_rels[partno];

		if (OidIsValid(part_rel->serverid))
			/* Foreign relation */
			FSExtractServerName(part_rel->serverid, &hostname, &port);
		else
			GetMyServerName(&hostname, &port);

		createNodeName(epp->nodes[partno], hostname, port);
	}
}

static void
create_shuffle_dfn(EPPNode *epp, RelOptInfo *rel, PlannerInfo *root)
{
	int partno;
	int attno;
	char *hostname;
	int port;

	Assert(rel->part_scheme->strategy == PARTITION_STRATEGY_HASH);

	epp->mode = SHUFFLE_MODE;
	epp->nnodes = rel->nparts;
	elog(LOG, "SHUFFLE. epp->nnodes=%d", epp->nnodes);
	epp->nodes = (NodeName *) palloc(sizeof(NodeName) * epp->nnodes);

	for(partno = 0; partno < epp->nnodes; partno++)
	{
		RelOptInfo *part_rel = rel->part_rels[partno];

		if (OidIsValid(part_rel->serverid))
			/* Foreign relation */
			FSExtractServerName(part_rel->serverid, &hostname, &port);
		else
			GetMyServerName(&hostname, &port);

		createNodeName(epp->nodes[partno], hostname, port);
	}

	epp->natts = rel->part_scheme->partnatts;
	epp->funcid = (Oid *) palloc(sizeof(Oid) * epp->natts);
	for (attno = 0; attno < epp->natts; attno++)
	{
		Oid funcid;

		funcid = get_opfamily_proc(rel->part_scheme->partopfamily[attno],
								   rel->part_scheme->partopcintype[attno],
								   rel->part_scheme->partopcintype[attno],
								   HASHEXTENDED_PROC);
		epp->funcid[attno] = funcid;
		epp->att_exprs = lappend(epp->att_exprs, linitial(rel->partexprs[attno]));
	}
}

/*
 * Create EXCHANGE path.
 * custom_private != NIL - exchange node is a leaf and it is needed to create
 * subtree for scanning of base relations.
 */
ExchangePath *
create_exchange_path(PlannerInfo *root, RelOptInfo *rel, Path *children,
					 int8 mode)
{
	ExchangePath *epath = (ExchangePath *) newNode(sizeof(ExchangePath), T_CustomPath);
	CustomPath *path = &epath->cp;
	Path *pathnode = &path->path;

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

	memcpy(&epath->altrel, rel, sizeof(RelOptInfo));
	epath->exchange_counter = exchange_counter++;
	epath->mode = mode;

	return epath;
}

/*
 * Compute foreign server ID and return its dmq representation.
 */
static DmqDestinationId
computeTupleDestination(TupleTableSlot *slot, ExchangeState *state)
{
	EState *estate = state->estate;
	int		greatest_modulus;
	uint64	rowHash;
	int16	part_index;
	Datum	values[PARTITION_MAX_KEYS];
	bool	isnull[PARTITION_MAX_KEYS];

	ListCell   *partexpr_item;
	int			i;
	ExprContext *econtext;
	int index;

	ExecMaterializeSlot(slot);
	econtext = GetPerTupleExprContext(estate);
	econtext->ecxt_scantuple = slot;

	partexpr_item = list_head(state->keystate);
	for (i = 0; i < state->partnatts; i++)
	{
		Datum	datum;
		bool	isNull;

		if (partexpr_item == NULL)
			elog(ERROR, "wrong number of partition key expressions");

		datum = ExecEvalExprSwitchContext((ExprState *) lfirst(partexpr_item),
										  GetPerTupleExprContext(estate),
										  &isNull);
		partexpr_item = lnext(partexpr_item);
		values[i] = datum;
		isnull[i] = isNull;
	}

	if (partexpr_item != NULL)
		elog(ERROR, "wrong number of partition key expressions. keystate list size: %d, partnatts: %d",
				list_length(state->keystate), state->partnatts);

	greatest_modulus = state->greatest_modulus;
	rowHash = compute_partition_hash_value(state->partnatts, state->partsupfunc,
			values, isnull);
	part_index = rowHash % greatest_modulus;
	index = state->indexes[part_index];

	if (index < 0)
		return -1;

	return state->dests->dests[index].dest_id;
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
	int16 partno;
	EPPNode *epp;

	state = (ExchangeState *) palloc0(sizeof(ExchangeState));
	NodeSetTag(state, T_CustomScanState);

	state->css.flags = node->flags;
	state->css.methods = &exchange_exec_methods;
	state->estate = NULL;

	private_data = node->custom_private;
	lc = list_head(private_data);

	/* Name of DMQ-connection stream */
	strcpy(state->stream, strVal(lfirst(lc)));

	/* Load partition info */
	Assert(list_length(private_data) == 2);
	epp = lsecond(private_data);
	state->mode = epp->mode;
	if (state->mode == SHUFFLE_MODE)
	{
		state->partnatts = epp->natts;

		foreach(lc, node->custom_exprs)
			state->partexprs = lappend(state->partexprs, copyObject(lfirst(lc)));

		state->partsupfunc = (FmgrInfo *) palloc(sizeof(FmgrInfo) * state->partnatts);
		state->greatest_modulus = epp->nnodes;
		for (partno = 0; partno < state->partnatts; partno++)
			fmgr_info(epp->funcid[partno], &state->partsupfunc[partno]);
		state->keystate = NIL;
	}
	Assert(epp->nnodes > 0);
	state->nodes = epp->nodes;
	state->nnodes = epp->nnodes;
	state->indexes = (int *) palloc(sizeof(int) * state->nnodes);
	return (Node *) state;
}

static void
EXCHANGE_Begin(CustomScanState *node, EState *estate, int eflags)
{
	CustomScan	*cscan = (CustomScan *) node->ss.ps.plan;
	Plan		*scan_plan;
	PlanState	*planState;
	ExchangeState *state = (ExchangeState *) node;
	TupleDesc	scan_tupdesc;

	Assert(list_length(cscan->custom_plans) == 1);

	scan_plan = linitial(cscan->custom_plans);
	planState = (PlanState *) ExecInitNode(scan_plan, estate, eflags);
	node->custom_ps = lappend(node->custom_ps, planState);

//	Stream_subscribe(state->stream);

	state->init = false;
	state->ltuples = 0;
	state->rtuples = 0;

	/* Need to access to QueryEnv which will initialized later. */
	state->estate = estate;

	scan_tupdesc = ExecTypeFromTL(scan_plan->targetlist, false);
	ExecInitScanTupleSlot(estate, &node->ss, scan_tupdesc);

	/* First time through, set up expression evaluation state */
	if (state->mode == SHUFFLE_MODE)
		state->keystate = ExecPrepareExprList(state->partexprs, estate);
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
				/* Any system message */
				break;
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
					SendByteMessage(state->dests->dests[i].dest_id,
												state->stream, END_OF_TUPLES);

				state->hasLocal = false;
				continue;
			}
			else
				state->ltuples++;
		}

		if ((state->activeRemotes == 0) && (!state->hasLocal))
		{
//			elog(LOG, "[%s] Exchange returns NULL. Tuples: local=%d, remote=%d",
//					state->stream, state->ltuples, state->rtuples);
			return NULL;
		}

		if (TupIsNull(slot))
			continue;

		if (state->mode == GATHER_MODE)
		{
			int cnum = state->dests->coordinator_num;
			if (cnum < 0)
				dest = -1;
			else
				dest = state->dests->dests[cnum].dest_id;
		}
		else if (state->mode == STEALTH_MODE)
			dest = -1;
		else if (state->mode == SHUFFLE_MODE)
			dest = computeTupleDestination(slot, state);
		else
			Assert(0);

		if (dest < 0)
			return slot;
		else
		{
//			elog(LOG, "SEND TUPLE to stream [%s]", state->stream);
			SendTuple(dest, state->stream, slot, false);
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

//	elog(INFO, "EXCHANGE_END");
}

static void
EXCHANGE_Rescan(CustomScanState *node)
{
	ExchangeState *state = (ExchangeState *) node;
	PlanState *subPlan = (PlanState *) linitial(node->custom_ps);

	init_state_ifany(state);

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
	Assert(0);
}

static void
EXCHANGE_Explain(CustomScanState *node, List *ancestors, ExplainState *es)
{
	StringInfoData		str;
	ExchangeState *state = (ExchangeState *) node;

	initStringInfo(&str);
	appendStringInfo(&str, "mode:%d, stream: %s. ", state->mode, state->stream);
	appendStringInfo(&str, "qual: %s.", nodeToString(state->partexprs));
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

void
createNodeName(char *nodeName, const char *hostname, int port)
{
	sprintf(nodeName, "%s-%d", hostname, port);
}
