/* ------------------------------------------------------------------------
 *
 * pg_exchange.c
 *		This module Adds custom node called EXCHANGE and rules for planner.
 *		Now planner can made some operations uver the partitioned relations
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "exchange.h"
#include "hooks.h"

PG_MODULE_MAGIC;

void _PG_init(void);

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
//	EXCHANGE_Init_methods();

	EXEC_Hooks_init();

//	CONN_Init_module();
}
