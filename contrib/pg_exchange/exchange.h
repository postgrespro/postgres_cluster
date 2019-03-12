/*-------------------------------------------------------------------------
 *
 * exchange.h
 *	Special custom node for tuples shuffling during parallel query execution.
 *
 * Copyright (c) 2018, PostgreSQL Global Development Group
 * Author: Andrey Lepikhov <a.lepikhov@postgrespro.ru>
 *
 * IDENTIFICATION
 *	contrib/pargres/exchange.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef EXCHANGE_H_
#define EXCHANGE_H_

#include "nodes/extensible.h"
#include "optimizer/planner.h"
#include "dmq.h"


#define EXCHANGE_NAME	"EXCHANGE"

typedef struct
{
	CustomScanState	css;
	char stream[256];
	DMQDestCont	*dests;
	bool init;
	EState *estate;
	bool hasLocal;
	int activeRemotes;
	int ltuples;
	int rtuples;
} ExchangeState;

extern void EXCHANGE_Init_methods(void);
extern void add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
							   RangeTblEntry *rte);
extern CustomScan *make_exchange(List *custom_plans, List *tlist);


#endif /* EXCHANGE_H_ */
