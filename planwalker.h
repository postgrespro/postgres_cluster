/*
 * planwalker.h
 *
 */

#ifndef PLANWALKER_H_
#define PLANWALKER_H_

#include "postgres.h"
#include "nodes/plannodes.h"

bool plan_tree_walker(Plan *plan, bool (*walker) (), void *context);

#endif /* PLANWALKER_H_ */
