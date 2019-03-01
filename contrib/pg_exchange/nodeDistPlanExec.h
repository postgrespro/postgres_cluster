/*-------------------------------------------------------------------------
 *
 * nodeDistPlanExec.h
 *
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEDISTPLANEXEC_H
#define NODEDISTPLANEXEC_H

#include "access/parallel.h"
#include "nodes/extensible.h"

typedef Scan DistPlanExec;

extern void DistExec_Init_methods(void);
extern CustomScan *make_distplanexec(List *custom_plans, List *tlist, List *private_data);
extern Path *create_distexec_path(PlannerInfo *root, RelOptInfo *rel,
								  Path *children, List *private_data);

#endif							/* NODEDISTPLANEXEC_H */
