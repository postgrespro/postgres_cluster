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
	char host[256];
	int port;
} conninfo_t;

extern int	shardman_instances;
extern conninfo_t shardman_instances_info[256];
extern int myNodeNum;
extern DmqDestinationId	dest_id[1024];

typedef struct
{
	CustomScanState	css;
	Oid				*sid;
	int				nsids;
} ExchangeState;

extern void EXCHANGE_Init_methods(void);
extern void add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
							   RangeTblEntry *rte);
extern CustomScan *make_exchange(List *custom_plans, List *tlist);


#endif /* EXCHANGE_H_ */
