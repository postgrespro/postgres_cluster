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
#define EXCHANGEPATHNAME	"Exchange"
#define EXCHANGE_PRIVATE_NAME "ExchangePlanPrivate"

#define IsExchangeNode(pathnode) (((pathnode)->pathtype == T_CustomScan) && \
	(strcmp(((CustomPath *)(pathnode))->methods->CustomName, EXCHANGEPATHNAME) == 0))

/* Exchange Private Partitioning data */
typedef struct EPPNode
{
	ExtensibleNode node;

	int nnodes;
	NodeName *nodes;

	int16 natts;
	Oid *funcid;
	List *att_exprs;
	int8 mode;
} EPPNode;

typedef struct ExchangeState
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

	/* Partitioning info */
	int greatest_modulus;
	int16 partnatts;
	FmgrInfo *partsupfunc;
	List *partexprs;
	List *keystate;
	int nnodes;
	NodeName *nodes;
	int *indexes;
	int8 mode;
} ExchangeState;

extern uint32 exchange_counter;
/*
 * Structure for private path data. It is used at paths generating step only.
 */
typedef struct ExchangePath
{
	CustomPath cp;

	RelOptInfo	altrel;
	uint32 exchange_counter; // Debug purposes only
	int8 mode; /* It will send all tuples to a coordinator only. */
} ExchangePath;

#define GATHER_MODE		(1)
#define STEALTH_MODE	(2)
#define SHUFFLE_MODE	(3)

extern void EXCHANGE_Init_methods(void);
extern void add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
							   RangeTblEntry *rte);
extern CustomScan *make_exchange(List *custom_plans, List *tlist);
extern ExchangePath *create_exchange_path(PlannerInfo *root, RelOptInfo *rel,
		  Path *children, int8 mode);
extern void createNodeName(char *nodeName, const char *hostname, int port);

#endif /* EXCHANGE_H_ */
