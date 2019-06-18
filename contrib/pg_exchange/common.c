/* ------------------------------------------------------------------------
 *
 * common.c
 *		Common code for ParGRES extension
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"
#include "miscadmin.h"

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/genam.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/indexing.h"
#include "catalog/pg_extension.h"
#include "commands/extension.h"
#include "utils/fmgroids.h"

#include "common.h"


MemoryContext memory_context = NULL;
ExchangeSharedState *ExchShmem = NULL;

static bool
plan_walk_members(List *plans, bool (*walker) (), void *context)
{
	ListCell *lc;

	foreach (lc, plans)
	{
		Plan *plan = lfirst(lc);
		if (walker(plan, context))
			return true;
	}

	return false;
}

bool
plan_tree_walker(Plan *plan, bool (*walker) (), void *context)
{
	ListCell   *lc;

	/* Guard against stack overflow due to overly complex plan trees */
	check_stack_depth();

	/* initPlan-s */
	if (plan_walk_members(plan->initPlan, walker, context))
		return true;

	/* lefttree */
	if (outerPlan(plan))
	{
		if (walker(outerPlan(plan), context))
			return true;
	}

	/* righttree */
	if (innerPlan(plan))
	{
		if (walker(innerPlan(plan), context))
			return true;
	}

	/* special child plans */
	switch (nodeTag(plan))
	{
		case T_ModifyTable:
			if (plan_walk_members(((ModifyTable *) plan)->plans,
									   walker, context))
				return true;
			break;
		case T_Append:
			if (plan_walk_members(((Append *) plan)->appendplans,
									   walker, context))
				return true;
			break;
		case T_MergeAppend:
			if (plan_walk_members(((MergeAppend *) plan)->mergeplans,
									   walker, context))
				return true;
			break;
		case T_BitmapAnd:
			if (plan_walk_members(((BitmapAnd *) plan)->bitmapplans,
									   walker, context))
				return true;
			break;
		case T_BitmapOr:
			if (plan_walk_members(((BitmapOr *) plan)->bitmapplans,
									   walker, context))
				return true;
			break;
		case T_SubqueryScan:
			if (walker(((SubqueryScan *) plan)->subplan, context))
				return true;
			break;
		case T_CustomScan:
			foreach(lc, ((CustomScan *) plan)->custom_plans)
			{
				if (walker((Plan *) lfirst(lc), context))
					return true;
			}
			break;
		default:
			break;
	}

	return false;
}
