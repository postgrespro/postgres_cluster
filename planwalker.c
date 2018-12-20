/*
 * planwalker.c
 *
 */

#include "planwalker.h"


static bool plan_walk_members(List *plans, bool (*walker) (), void *context);

/*
 * plan_tree_walker --- walk plan trees
 *
 */
bool
plan_tree_walker(Plan *plan,
					  bool (*walker) (),
					  void *context)
{
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
			if (plan_walk_members(((CustomScan *) plan)->custom_plans,
												   walker, context))
				return true;
			break;
		default:
			break;
	}

	return false;
}

static bool
plan_walk_members(List *plans, bool (*walker) (), void *context)
{
	ListCell   *lc;

//	if (list_length(plans) == 0)
//		return false;

	foreach(lc, plans)
	{
		Plan *plan = lfirst(lc);

		if (walker(plan, context))
			return true;
	}

	return false;
}
