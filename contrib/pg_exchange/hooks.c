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
#include "exchange.h"
#include "hooks.h"
#include "optimizer/paths.h"


static set_rel_pathlist_hook_type	prev_set_rel_pathlist_hook = NULL;


static void
HOOK_Baserel_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
				   RangeTblEntry *rte)
{
	if (prev_set_rel_pathlist_hook)
		(*prev_set_rel_pathlist_hook) (root, rel, rti, rte);

	add_exchange_paths(root, rel, rti, rte);
}

void
EXEC_Hooks_init(void)
{
	prev_set_rel_pathlist_hook = set_rel_pathlist_hook;
	set_rel_pathlist_hook = HOOK_Baserel_paths;
}

