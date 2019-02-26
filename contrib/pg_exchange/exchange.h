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

//#include "commands/explain.h"
#include "connection.h"
#include "nodes/extensible.h"
#include "optimizer/planner.h"


#define EXCHANGE_NAME	"EXCHANGE"

typedef enum
{
	FR_FUNC_NINITIALIZED = 0,
	FR_FUNC_DEFAULT,
	FR_FUNC_GATHER,
	FR_FUNC_HASH
} fr_func_id;

typedef struct
{
	int			attno;
	fr_func_id	funcId;
} fr_options_t;

typedef struct
{
	CustomScanState	css;
	Oid				*sid;
	int				nsids;
} ExchangeState;

extern void EXCHANGE_Init_methods(void);
extern void add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
		RangeTblEntry *rte);
extern CustomScan *make_exchange(List *custom_plans, List *tlist, List *private_data);
extern int get_tuple_node(fr_func_id fid, Datum value, int mynode, int nnodes,
						  void *data);

#endif /* EXCHANGE_H_ */
