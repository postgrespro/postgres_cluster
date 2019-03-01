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

#include "dmq.h"
#include "exchange.h"
#include "hooks.h"
#include "libpq/libpq-be.h"
#include "miscadmin.h"
#include "utils/guc.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(init_pg_exchange);

void _PG_init(void);

/*
 * This is permanent code for initialization of shardman servers connection data
 */
static void
INIT_Distribution_conf()
{
	FILE *dcFile;
	int i;

	dcFile = fopen("../distribution.conf", "rt");
	Assert(dcFile);
	fscanf(dcFile, "%d\n", &shardman_instances);
//	elog(LOG, "shardman_instances: %d", shardman_instances);
	Assert((shardman_instances > 0) && (shardman_instances < 10));

	for (i = 0; i < shardman_instances; i++)
	{
		fscanf(dcFile, "%s\t%d\n", shardman_instances_info[i].host, &shardman_instances_info[i].port);
//		elog(LOG, "(%s, %d)", shardman_instances_info[i].host, shardman_instances_info[i].port);
	}

	fclose(dcFile);
}

#define DMQ_CONNSTR_MAX_LEN 150

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomIntVariable(
		"pg_exchange.node_number1",
		"Node number",
		NULL,
		&myNodeNum,
		1,
		0,
		1024,
		PGC_SIGHUP,
		GUC_NOT_IN_SAMPLE,
		NULL,
		NULL,
		NULL
	);

	EXCHANGE_Init_methods();
	EXEC_Hooks_init();
	dmq_init("pg_exchange");
	INIT_Distribution_conf();
//	elog(LOG, "END Initialization");
}

Datum
init_pg_exchange(PG_FUNCTION_ARGS)
{
	int i;
	char	connstr[1024],
			sender_name[DMQ_CONNSTR_MAX_LEN],
			receiver_name[DMQ_CONNSTR_MAX_LEN];

//	elog(LOG, "Add destinations");
	sprintf(sender_name, "node-%d", myNodeNum);

	for (i = 0; i < shardman_instances; i++)
	{
		if (i == myNodeNum)
			continue;

		sprintf(connstr, "host=%s "
						 "port=%d "
						 "application_name='%s' "
						 "fallback_application_name=%s",
						 shardman_instances_info[i].host,
						 shardman_instances_info[i].port,
						 sender_name,
						 sender_name);

		sprintf(receiver_name, "node-%d", i);

		dest_id[i] = dmq_destination_add(connstr, sender_name, receiver_name, 10);
		elog(LOG, "MyNodeNum: %d, sender_name: %s, receiver_name: %s, conninfo: %s, dest_id=%d.", myNodeNum, sender_name, receiver_name, connstr, dest_id[i]);
	}

	elog(LOG, "END OF INIT");
	PG_RETURN_VOID();
}
