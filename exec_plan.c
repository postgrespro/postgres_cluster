
#include "postgres.h"

#include "catalog/namespace.h"
#include "common/base64.h"
#include "nodes/nodeFuncs.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"

#include "exec_plan.h"
#include "planwalker.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_execute_plan);

static char *serialize_plan(QueryDesc *queryDesc, int eflags);
static bool store_irel_name(Plan *plan, char *buffer);

/*
 * INPUT: a base64-encoded serialized plan
 */
void
execute_query(PGconn *dest, QueryDesc *queryDesc, int eflags)
{
	char	*SQLCommand;
	int		status;
	char	*serializedPlan;
	PGresult *result;

	Assert(dest != NULL);

	/*
	 * Before send of plan we need to check connection state.
	 * If previous query was failed, we get PGRES_FATAL_ERROR.
	 */
	while ((result = PQgetResult(dest)) != NULL);

	serializedPlan = serialize_plan(queryDesc, eflags);
	/* Connect to slave and send it a query plan */
	SQLCommand = (char *) palloc0(strlen(serializedPlan)+100);
	sprintf(SQLCommand, "SELECT pg_execute_plan('%s');", serializedPlan);

	status = PQsendQuery(dest, SQLCommand);

	if (status == 0)
		elog(ERROR, "Query sending error: %s", PQerrorMessage(dest));
}

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
elog(LOG, "Send QUERY: %s", queryDesc->sourceText);
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
	elog(LOG, "Recv QUERY: %s", queryString);
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
//	elog(LOG, "End of QUERY: %s", queryString);
	pfree(decdata);
	PG_RETURN_BOOL(true);
}

