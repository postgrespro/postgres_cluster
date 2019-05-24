/*
 * expath.c
 *
 */

#include "postgres.h"

#include "optimizer/pathnode.h"

#include "expath.h"


/*
 * FDW paths and EXCHANGE paths are incompatible and can't be combined at a plan.
 * We need to construct two non-intersecting path branches across all plan.
 * Costs of this plans is not an indicator of path quality at intermediate
 * stages of a plan building. We need bypass add_path() path checking procedure.
 */
void
force_add_path(RelOptInfo *rel, Path *path)
{
	List *pathlist = rel->pathlist;

	rel->pathlist = NIL;
	rel->cheapest_parameterized_paths = NIL;
	rel->cheapest_startup_path = rel->cheapest_total_path =
											rel->cheapest_unique_path = NULL;
	add_path(rel, path);
	rel->pathlist = list_concat(rel->pathlist, pathlist);
	set_cheapest(rel);
}
