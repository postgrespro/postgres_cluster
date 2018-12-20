
#include "postgres.h"

#include "catalog/namespace.h"
#include "commands/extension.h"
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "nodes/params.h"
#include "planwalker.h"
#include "optimizer/planner.h"
#include "storage/lmgr.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_execute_plan);

void _PG_init(void);

static ProcessUtility_hook_type 	next_ProcessUtility_hook = NULL;
static planner_hook_type			prev_planner_hook = NULL;
static ExecutorStart_hook_type		prev_ExecutorStart = NULL;
static ExecutorEnd_hook_type		prev_ExecutorEnd = NULL;

static void HOOK_Utility_injection(PlannedStmt *pstmt, const char *queryString,
						ProcessUtilityContext context, ParamListInfo params,
						QueryEnvironment *queryEnv, DestReceiver *dest,
						char *completionTag);
static PlannedStmt *HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams);
static void HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags);
static void HOOK_ExecEnd_injection(QueryDesc *queryDesc);
static char *serialize_plan(QueryDesc *queryDesc, int eflags);
static void execute_query(char *planString);
static bool store_irel_name(Plan *plan, char *buffer);

static PGconn	*conn = NULL;

int node_number1 = 0;

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	DefineCustomIntVariable("pargres.node",
							"Node number in instances collaboration",
							NULL,
							&node_number1,
							0,
							0,
							1023,
							PGC_SIGHUP,
							GUC_NOT_IN_SAMPLE,
							NULL,
							NULL,
							NULL);

	/* ProcessUtility hook */
	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = HOOK_Utility_injection;

	/* Planner hook */
	prev_planner_hook	= planner_hook;
	planner_hook		= HOOK_Planner_injection;

	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = HOOK_ExecStart_injection;

	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = HOOK_ExecEnd_injection;
}

static void
HOOK_Utility_injection(PlannedStmt *pstmt,
						const char *queryString,
						ProcessUtilityContext context,
						ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest,
						char *completionTag)
{
	Node	*parsetree = pstmt->utilityStmt;

	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		(nodeTag(parsetree) != T_CopyStmt) &&
		(nodeTag(parsetree) != T_CreateExtensionStmt) &&
		(context != PROCESS_UTILITY_SUBCOMMAND))
	{
		char	conninfo[1024];
		int		status;

//		elog(LOG, "Send UTILITY query %d: %s", nodeTag(parsetree), queryString);

		/* Connect to slave and send it a query plan */
		sprintf(conninfo, "host=localhost port=5433%c", '\0');
		conn = PQconnectdb(conninfo);
		if (PQstatus(conn) == CONNECTION_BAD)
			elog(LOG, "Connection error. conninfo: %s", conninfo);

		status = PQsendQuery(conn, queryString);
		if (status == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
	}
	else if (node_number1 == 0)
		elog(LOG, "UTILITY query without sending: %s", queryString);

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, context, params,
									 queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString,
											context, params, queryEnv,
											dest, completionTag);

	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL)
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
		PQfinish(conn);
		conn = NULL;
	}
}

/*
 * INPUT: a base64-encoded serialized plan
 */
static void
execute_query(char *planString)
{
	char	conninfo[1024];
	char	*SQLCommand;
	int		status;

	/* Connect to slave and send it a query plan */
	sprintf(conninfo, "host=localhost port=5433%c", '\0');
	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) == CONNECTION_BAD)
		elog(LOG, "Connection error. conninfo: %s", conninfo);

	SQLCommand = (char *) palloc0(strlen(planString)+100);
	sprintf(SQLCommand, "SELECT pg_execute_plan('%s');", planString);
//elog(LOG, "query: %s", SQLCommand);
	status = PQsendQuery(conn, SQLCommand);
	if (status == 0)
		elog(ERROR, "Query sending error: %s", PQerrorMessage(conn));
}

static PlannedStmt *
HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams)
{
	PlannedStmt *pstmt;

	conn = NULL;

	if (prev_planner_hook)
		pstmt = prev_planner_hook(parse, cursorOptions, boundParams);
	else
		pstmt = standard_planner(parse, cursorOptions, boundParams);

	if ((node_number1 > 0) || (parse->utilityStmt != NULL))
		return pstmt;

	/* Extension is not initialized. */
	if (OidIsValid(get_extension_oid("execplan", true)))
	{

	}
	return pstmt;
}

static void
HOOK_ExecStart_injection(QueryDesc *queryDesc, int eflags)
{
	Node	*parsetree = queryDesc->plannedstmt->utilityStmt;

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	if ((OidIsValid(get_extension_oid("execplan", true))) &&
		(node_number1 == 0) &&
		((parsetree == NULL) || (nodeTag(parsetree) != T_CreatedbStmt)))
	{
//		elog(LOG, "Send query: %s", queryDesc->sourceText);
		execute_query(serialize_plan(queryDesc, eflags));
	}
}

static void
HOOK_ExecEnd_injection(QueryDesc *queryDesc)
{
	/* Execute before hook because it destruct memory context of exchange list */
	if (conn)
	{
		PGresult *result;

		while ((result = PQgetResult(conn)) != NULL)
			Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
		PQfinish(conn);
		conn = NULL;
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

#include "common/base64.h"
#include "nodes/nodeFuncs.h"

static bool
compute_irels_buffer_len(Plan *plan, int *length)
{
	Oid		indexid = InvalidOid;

	switch (nodeTag(plan))
	{
		case T_IndexScan:
		{
			indexid = ((IndexScan *) plan)->indexid;
			break;
		}
		case T_IndexOnlyScan:
		{
			indexid = ((IndexOnlyScan *) plan)->indexid;
			break;
		}
		case T_BitmapIndexScan:
		{
			indexid = ((BitmapIndexScan *) plan)->indexid;
			break;
		}
		default:
			break;
	}

	if (indexid != InvalidOid)
	{
		*length += strlen(get_rel_name(indexid)) + 1;
	}

	return plan_tree_walker(plan, compute_irels_buffer_len, length);
}

//#include "nodes/pg_list.h"

static char *
serialize_plan(QueryDesc *queryDesc, int eflags)
{
	int		splan_len,
			sparams_len,
			qtext_len,
			relnames_len = 0,
			inames_len = 0,
			tot_len,
			econtainer_len;
	char	*serialized_plan,
			*container,
			*econtainer,
			*start_address;
	ListCell		*lc;

	serialized_plan = nodeToString(queryDesc->plannedstmt);

	/*
	 * Traverse the rtable list, get elements with relkind == RTE_RELATION, get
	 * relation name by rte->relid. This step is needed for serialized plan raw
	 * data memory estimation
	 */
	foreach(lc, queryDesc->plannedstmt->rtable)
	{
		RangeTblEntry	*rte = (RangeTblEntry *) lfirst(lc);
		char			*relname;

		if (rte->rtekind == RTE_RELATION)
		{
			/* Use the current actual name of the relation */
			relname = get_rel_name(rte->relid);
			relnames_len += strlen(relname) + 1;
		}
	}

	/*
	 * Traverse the query plan and search for the node types: T_BitmapIndexScan,
	 * T_IndexOnlyScan, T_IndexScan. Its contains 'Oid indexid' field. We need
	 * to save the relation names in serialized plan.
	 */
	compute_irels_buffer_len(queryDesc->plannedstmt->planTree, &inames_len);
//	plan_tree_walker(queryDesc->plannedstmt->planTree,
//					 compute_irels_buffer_len,
//					 &inames_len);
//	planstate_tree_walker((PlanState *) (queryDesc->planstate), compute_irels_buffer_len, &inames_len);
//elog(LOG, "inames_len=%d", inames_len);
	/* We use len+1 bytes for include end-of-string symbol. */
	splan_len = strlen(serialized_plan) + 1;
	qtext_len = strlen(queryDesc->sourceText) + 1;

	sparams_len = EstimateParamListSpace(queryDesc->params);
	tot_len = splan_len + sparams_len + qtext_len + 2*sizeof(int) + relnames_len + inames_len;

	container = (char *) palloc0(tot_len);
	start_address = container;

	memcpy(start_address, serialized_plan, splan_len);
	start_address += splan_len;
	SerializeParamList(queryDesc->params, &start_address);
	Assert(start_address == container + splan_len + sparams_len);
	memcpy(start_address, queryDesc->sourceText, qtext_len);

	/* Add instrument_options */
	start_address += qtext_len;
	memcpy(start_address, &queryDesc->instrument_options, sizeof(int));

	/* Add flags */
	start_address += sizeof(int);
	memcpy(start_address, &eflags, sizeof(int));
	start_address += sizeof(int);

	/*
	 * Second pass of the rtable list, get elements with
	 * relkind == RTE_RELATION, get relation name by rte->relid. Save relation
	 * names into the serialized plan buffer.
	 */
	foreach(lc, queryDesc->plannedstmt->rtable)
	{
		RangeTblEntry	*rte = (RangeTblEntry *) lfirst(lc);
		char			*relname;
		int				len;

		if (rte->rtekind == RTE_RELATION)
		{
			/* Use the current actual name of the relation */
			relname = get_rel_name(rte->relid);
			len = strlen(relname) + 1;
			memcpy(start_address, relname, len);
			start_address += len;
		}
	}
	store_irel_name((Plan *) (queryDesc->plannedstmt->planTree), start_address);
//	plan_tree_walker((Plan *) (queryDesc->plannedstmt->planTree),
//					 store_irel_name,
//					 start_address);

	start_address += inames_len;
	Assert((start_address - container) == tot_len);

	econtainer_len = pg_b64_enc_len(tot_len);
	econtainer = (char *) palloc0(econtainer_len+1);
	Assert(pg_b64_encode(container, tot_len, econtainer) <= econtainer_len);

	/* In accordance with example from fe-auth-scram.c */
	econtainer[econtainer_len] = '\0';

	return econtainer;
}

static void
ScanQueryForLocks(PlannedStmt *pstmt, bool acquire)
{
	ListCell   *lc;

	/* Shouldn't get called on utility commands */
	Assert(pstmt->commandType != CMD_UTILITY);

	/*
	 * First, process RTEs of the current query level.
	 */
	foreach(lc, pstmt->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(lc);

		switch (rte->rtekind)
		{
			case RTE_RELATION:
				/* Acquire or release the appropriate type of lock */
				if (acquire)
					LockRelationOid(rte->relid, rte->rellockmode);
				else
					UnlockRelationOid(rte->relid, rte->rellockmode);
				break;

			default:
				/* ignore other types of RTEs */
				break;
		}
	}

}
static void
AcquirePlannerLocks(PlannedStmt *pstmt, bool acquire)
{
	if (pstmt->commandType == CMD_UTILITY)
	{
		/* Ignore utility statements, unless they contain a Query */
	}

	ScanQueryForLocks(pstmt, acquire);
}

static bool
store_irel_name(Plan *plan, char *buffer)
{
	Oid		indexid = InvalidOid;

	switch (nodeTag(plan))
	{
		case T_IndexScan:
		{
			indexid = ((IndexScan *) plan)->indexid;
			break;
		}
		case T_IndexOnlyScan:
		{
			indexid = ((IndexOnlyScan *) plan)->indexid;
			break;
		}
		case T_BitmapIndexScan:
		{
			indexid = ((BitmapIndexScan *) plan)->indexid;
			break;
		}
		default:
			break;
	}

	if (indexid != InvalidOid)
	{
		char	*relname;
		int		relname_len;
//elog(LOG, "INDEXID %d", indexid);
		relname = get_rel_name(indexid);
		relname_len = strlen(relname) + 1;
		memcpy(buffer, relname, relname_len);
		buffer += relname_len;
	}

	return plan_tree_walker(plan, store_irel_name, buffer);
}

static bool
extract_relname(Plan *plan, char *buffer)
{
	Oid		*indexid = NULL;

	switch (nodeTag(plan))
	{
		case T_IndexScan:
		{
			indexid = &((IndexScan *) plan)->indexid;
//			elog(LOG, "INDEX SCAN indexid=%d", *indexid);
			break;
		}
		case T_IndexOnlyScan:
		{
			indexid = &((IndexOnlyScan *) plan)->indexid;
			break;
		}
		case T_BitmapIndexScan:
		{
			indexid = &((BitmapIndexScan *) plan)->indexid;
			break;
		}
		default:
			break;
	}

	if (indexid != NULL)
	{
		*indexid = RelnameGetRelid(buffer);
		buffer += strlen(buffer) + 1;
//		elog(LOG, "RR Relation: %s, indexid=%d", buffer, *indexid);
	}

	return plan_tree_walker(plan, extract_relname, buffer);
}

/*
 * pg_execute_plan() -- execute a query plan by an instance
 *
 * Restore an query plan from base64-encoded string.
 */
Datum
pg_execute_plan(PG_FUNCTION_ARGS)
{
	PlannedStmt		*pstmt;
	QueryDesc		*queryDesc;
	ParamListInfo 	paramLI = NULL;
	ListCell		*lc;
	char			*data = TextDatumGetCString(PG_GETARG_DATUM(0)),
					*decdata,
					*queryString,
					*start_addr;
	int				decdata_len,
					*instrument_options,
					*eflags;

	/* Decode base64 string into a byte sequence */
	Assert(data != NULL);
	decdata_len = pg_b64_dec_len(strlen(data));
	decdata = (char *) palloc(decdata_len);
	decdata_len = pg_b64_decode(data, strlen(data), decdata);

	/* Create query plan */
	pstmt = (PlannedStmt *) stringToNode((char *) decdata);

	/* Restore parameters list. */
	start_addr = decdata + strlen(decdata) + 1;
	paramLI = RestoreParamList((char **) &start_addr);

	/* Restore query source text string */
	queryString = start_addr;
//elog(LOG, "queryString: %s", queryString);
	/* Restore instrument and flags */
	start_addr += strlen(queryString) + 1;
	instrument_options = (int *) start_addr;
	start_addr += sizeof(int);
	eflags = (int *) start_addr;
	start_addr += sizeof(int);

	/*
	 * Get relation oid by relation name, stored at the serialized plan.
	 */
	foreach(lc, pstmt->rtable)
	{
		RangeTblEntry	*rte = (RangeTblEntry *) lfirst(lc);

		if (rte->rtekind == RTE_RELATION)
		{
			rte->relid = RelnameGetRelid(start_addr);
			Assert(rte->relid != InvalidOid);
//			elog(LOG, "Relation from decoded plan. relid=%d relname=%s", rte->relid, start_addr);
			start_addr += strlen(start_addr) + 1;
		}
	}

	extract_relname(pstmt->planTree, start_addr);

	queryDesc = CreateQueryDesc(pstmt, queryString, GetActiveSnapshot(),
								InvalidSnapshot, None_Receiver, paramLI, NULL,
								*instrument_options);

	AcquirePlannerLocks(pstmt, true);

	ExecutorStart(queryDesc, *eflags);

	ExecutorRun(queryDesc, ForwardScanDirection, 0, true);

	/* Shut down the executor */
	ExecutorFinish(queryDesc);
	ExecutorEnd(queryDesc);
	FreeQueryDesc(queryDesc);

	pfree(decdata);
	PG_RETURN_BOOL(true);
}

