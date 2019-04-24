/*-------------------------------------------------------------------------
 *
 * nodeDummyscan.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodeDummyscan.h"
#include "nodes/pg_list.h"
#include "nodes/nodes.h"


static CustomScanMethods	dummyscan_plan_methods;
static CustomExecMethods	dummyscan_exec_methods;


/*
 * Create state of exchange node.
 */
static Node *
CreateDummyScanState(CustomScan *node)
{
	DummyScanState	*state;

	state = (DummyScanState *) palloc0(sizeof(DummyScanState));
	NodeSetTag(state, T_CustomScanState);

	state->css.flags = node->flags;
	state->css.methods = &dummyscan_exec_methods;
	state->css.custom_ps = NIL;

	return (Node *) state;
}

static void
BeginDummyScan(CustomScanState *node, EState *estate, int eflags)
{
	return;
}

static TupleTableSlot *
ExecDummyScan(CustomScanState *node)
{
	return NULL;
}

static void
ExecEndDummyScan(CustomScanState *node)
{
	return;
}

static void
ExecReScanDummyScan(CustomScanState *node)
{
	return;
}

static void
ExplainDummyScan(CustomScanState *node, List *ancestors, ExplainState *es)
{
	StringInfoData		str;

	initStringInfo(&str);
	ExplainPropertyText("DummyScan", str.data, es);
}

void
DUMMYSCAN_Init_methods(void)
{
	dummyscan_plan_methods.CustomName 			= "DummyPlan";
	dummyscan_plan_methods.CreateCustomScanState	= CreateDummyScanState;
	RegisterCustomScanMethods(&dummyscan_plan_methods);

	/* setup exec methods */
	dummyscan_exec_methods.CustomName				= "Dummy";
	dummyscan_exec_methods.BeginCustomScan			= BeginDummyScan;
	dummyscan_exec_methods.ExecCustomScan			= ExecDummyScan;
	dummyscan_exec_methods.EndCustomScan				= ExecEndDummyScan;
	dummyscan_exec_methods.ReScanCustomScan			= ExecReScanDummyScan;
	dummyscan_exec_methods.MarkPosCustomScan			= NULL;
	dummyscan_exec_methods.RestrPosCustomScan		= NULL;
	dummyscan_exec_methods.EstimateDSMCustomScan  	= NULL;
	dummyscan_exec_methods.InitializeDSMCustomScan 	= NULL;
	dummyscan_exec_methods.InitializeWorkerCustomScan= NULL;
	dummyscan_exec_methods.ReInitializeDSMCustomScan = NULL;
	dummyscan_exec_methods.ShutdownCustomScan		= NULL;
	dummyscan_exec_methods.ExplainCustomScan		= ExplainDummyScan;
}

CustomScan *
make_dummyscan(Index scanrelid)
{
	CustomScan	*node = makeNode(CustomScan);
	Plan		*plan = &node->scan.plan;

	plan->startup_cost = 0;
	plan->total_cost = 0;
	plan->plan_rows = 0;
	plan->plan_width = 0;
	plan->qual = NIL;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	plan->parallel_aware = false;
	plan->parallel_safe = false;
	plan->targetlist = NIL;

	/* Setup methods and child plan */
	node->methods = &dummyscan_plan_methods;
	node->custom_scan_tlist = NIL;
	node->scan.scanrelid = scanrelid;
	node->custom_plans = NIL;
	node->custom_exprs = NIL;
	node->custom_private = NIL;

	return node;
}
