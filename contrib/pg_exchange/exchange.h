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

/*
 * Структура для хранения информации о распределении кортежей между инстансами.
 * Распределение обеспечивается нодой EXCHANGE.
 */
typedef struct PState
{
	int16 greatest_modulus;
	int16 partnatts;
	FmgrInfo *partsupfunc;
	List *keystate;
} PState;

typedef struct EPPNode /* Exchange Private Partitioning data */
{
	ExtensibleNode node;
	int nparts;
	int16 partnatts;
	Oid *funcid;
//	PartitionBoundInfoData *boundinfo;
	List *partexprs;
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

	PState pstate;
} ExchangeState;
extern uint32 exchange_counter;
/*
 * Structure for private path data. It is used at paths generating step only.
 */
typedef struct ExchangePath
{
	CustomPath cp;

	/*
	 * alternate partitioning scheme.
	 * */
	RelOptInfo	altrel;
	uint32 exchange_counter; // Debug purposes only
} ExchangePath;

extern void EXCHANGE_Init_methods(void);
extern void add_exchange_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
							   RangeTblEntry *rte);
extern CustomScan *make_exchange(List *custom_plans, List *tlist);
extern ExchangePath *create_exchange_path(PlannerInfo *root, RelOptInfo *rel,
		  Path *children);


#endif /* EXCHANGE_H_ */
