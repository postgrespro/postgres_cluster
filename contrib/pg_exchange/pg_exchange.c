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

#include "catalog/pg_class.h"
#include "common.h"
#include "common/base64.h"
#include "dmq.h"
#include "exchange.h"
#include "hooks.h"
#include "libpq/libpq-be.h"
#include "miscadmin.h"
#include "nodeDistPlanExec.h"
#include "nodeDummyscan.h"
#include "nodes/nodes.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"


PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(init_pg_exchange);
PG_FUNCTION_INFO_V1(pg_exec_plan);


#define DMQ_CONNSTR_MAX_LEN 150

dmq_receiver_hook_type old_dmq_receiver_stop_hook;

void _PG_init(void);
static void deserialize_plan(char **squery, char **splan, char **sparams);
static void exec_plan(char *squery, PlannedStmt *pstmt, ParamListInfo paramLI, const char *serverName);
static void OnNodeDisconnect(const char *node_name);

static Size
shmem_size(void)
{
	Size	size = 0;

	size = add_size(size, sizeof(ExchangeSharedState));
	size = add_size(size, hash_estimate_size(1024,
											 sizeof(DMQDestinations)));
	return MAXALIGN(size);
}
#include "common/ip.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include "libpq/pqcomm.h"

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomBoolVariable("enable_distributed_execution",
							 "Use distributed execution.",
							 NULL,
							 &enable_distributed_execution,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	EXCHANGE_Init_methods();
	DUMMYSCAN_Init_methods();
	EXEC_Hooks_init();
	dmq_init("pg_exchange");

	RequestAddinShmemSpace(shmem_size());
	RequestNamedLWLockTranche("pg_exchange", 1);

	old_dmq_receiver_stop_hook = dmq_receiver_stop_hook;
	dmq_receiver_stop_hook = OnNodeDisconnect;
/*	{
		char host[1024];
		FILE *f = fopen("/home/andrey/PostgresPro/pgcluster/hosts.txt", "rt");
		struct addrinfo hintp;
		struct addrinfo *result;
		MemSet(&hintp, 0, sizeof(hintp));
//		hintp.ai_socktype = SOCK_STREAM;
		hintp.ai_family = AF_UNSPEC;
		hintp.ai_flags = AI_ALL;

		Assert(f != NULL);
		while (!feof(f))
		{
			struct addrinfo *next;
			int i=0;
			int res1;

			fscanf(f, "%s", host);
			res1 = pg_getaddrinfo_all(host, NULL, &hintp, &result);
			next = result;

			while (next != NULL)
			{
				SockAddr a1;
				int res2;
				char node[NI_MAXHOST];
				char service[NI_MAXSERV];
				char *res;

				res = inet_ntoa(((struct sockaddr_in *)next->ai_addr)->sin_addr);
				memcpy(&a1.addr, next->ai_addr, next->ai_addrlen);
				res2 = pg_getnameinfo_all(&a1.addr, next->ai_addrlen, node, NI_MAXHOST,
						service, NI_MAXSERV, 0);
				elog(LOG, "[%d] srchost: %s, res: [%d, %d] IP: %s, host: %s, service: %s.",
						i++, host, res1, res2, res, node, service);
				next = next->ai_next;
			}
			pg_freeaddrinfo_all(hintp.ai_family, result);
		}
		fclose(f);
	} */
}

Datum
init_pg_exchange(PG_FUNCTION_ARGS)
{
	PG_RETURN_VOID();
}

Datum
pg_exec_plan(PG_FUNCTION_ARGS)
{
	char	*squery = TextDatumGetCString(PG_GETARG_DATUM(0)),
			*splan = TextDatumGetCString(PG_GETARG_DATUM(1)),
			*sparams = TextDatumGetCString(PG_GETARG_DATUM(2)),
			*serverName = TextDatumGetCString(PG_GETARG_DATUM(3)),
			*start_address;
	PlannedStmt *pstmt;
	ParamListInfo paramLI;

	deserialize_plan(&squery, &splan, &sparams);

	pstmt = (PlannedStmt *) stringToNode(splan);

	/* Deserialize parameters of the query */
	start_address = sparams;
	paramLI = RestoreParamList(&start_address);

	exec_plan(squery, pstmt, paramLI, serverName);
	PG_RETURN_VOID();
}

/*
 * Decode base64 string into C-string and return it at same pointer
 */
static void
deserialize_plan(char **squery, char **splan, char **sparams)
{
	char	*dec_query,
			*dec_plan,
			*dec_params;
	int		dec_query_len,
			len,
			dec_plan_len,
			dec_params_len;

	dec_query_len = pg_b64_dec_len(strlen(*squery));
	dec_query = palloc0(dec_query_len + 1);
	len = pg_b64_decode(*squery, strlen(*squery), dec_query);
	Assert(dec_query_len >= len);

	dec_plan_len = pg_b64_dec_len(strlen(*splan));
	dec_plan = palloc0(dec_plan_len + 1);
	len = pg_b64_decode(*splan, strlen(*splan), dec_plan);
	Assert(dec_plan_len >= len);

	dec_params_len = pg_b64_dec_len(strlen(*sparams));
	dec_params = palloc0(dec_params_len + 1);
	len = pg_b64_decode(*sparams, strlen(*sparams), dec_params);
	Assert(dec_params_len >= len);

	*squery = dec_query;
	*splan = dec_plan;
	*sparams = dec_params;
}
#include "tcop/tcopprot.h"
static void
exec_plan(char *squery, PlannedStmt *pstmt, ParamListInfo paramLI, const char *serverName)
{
	CachedPlanSource	*psrc;
	CachedPlan			*cplan;
	QueryDesc			*queryDesc;
	DestReceiver		*receiver;
	int					eflags = 0;
	Oid					*param_types = NULL;

	Assert(squery && pstmt && paramLI);
	debug_query_string = strdup(squery);
	psrc = CreateCachedPlan(NULL, squery, NULL);

	if (paramLI->numParams > 0)
	{
		int i;

		param_types = palloc(sizeof(Oid) * paramLI->numParams);
		for (i = 0; i < paramLI->numParams; i++)
			param_types[i] = paramLI->params[i].ptype;
	}
	CompleteCachedPlan(psrc, NIL, NULL, param_types, paramLI->numParams, NULL,
								NULL, CURSOR_OPT_GENERIC_PLAN, false);

	SetRemoteSubplan(psrc, pstmt);
	cplan = GetCachedPlan(psrc, paramLI, false, NULL);

	receiver = CreateDestReceiver(DestLog);

	PG_TRY();
	{
		lcontext context;
		queryDesc = CreateQueryDesc(pstmt,
									squery,
									GetActiveSnapshot(),
									InvalidSnapshot,
									receiver,
									paramLI, NULL,
									0);

		context.pstmt = pstmt;
		context.eflags = eflags;
		context.servers = NULL;
		context.indexinfo = NULL;
		localize_plan(pstmt->planTree, &context);

		ExecutorStart(queryDesc, eflags);
		EstablishDMQConnections(&context, serverName, queryDesc->estate,
								queryDesc->planstate);
		PushActiveSnapshot(queryDesc->snapshot);
		ExecutorRun(queryDesc, ForwardScanDirection, 0, true);
		PopActiveSnapshot();
		ExecutorFinish(queryDesc);
		ExecutorEnd(queryDesc);
		FreeQueryDesc(queryDesc);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD QUERY: '%s'.", squery);
		ReleaseCachedPlan(cplan, false);
		PG_RE_THROW();
	}
	PG_END_TRY();

	receiver->rDestroy(receiver);
	ReleaseCachedPlan(cplan, false);
}

static void
OnNodeDisconnect(const char *node_name)
{
	HASH_SEQ_STATUS status;
	DMQDestinations *dest;
	Oid serverid = InvalidOid;

	elog(LOG, "Node %s: disconnected", node_name);


	LWLockAcquire(ExchShmem->lock, LW_EXCLUSIVE);

	hash_seq_init(&status, ExchShmem->htab);

	while ((dest = hash_seq_search(&status)) != NULL)
	{
		if (!(strcmp(dest->node, node_name) == 0))
			continue;

		serverid = dest->serverid;
		dmq_detach_receiver(node_name);
		dmq_destination_drop(node_name);
		break;
	}
	hash_seq_term(&status);

	if (OidIsValid(serverid))
		hash_search(ExchShmem->htab, &serverid, HASH_REMOVE, NULL);
	else
		elog(LOG, "Record on disconnected server %u with name %s not found.",
														serverid, node_name);
	LWLockRelease(ExchShmem->lock);
}
