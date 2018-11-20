
#include "postgres.h"

#include "commands/extension.h"
#include "executor/execdesc.h"
#include "fmgr.h"
#include "optimizer/planner.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_execute_plan);

void _PG_init(void);

static planner_hook_type			prev_planner_hook = NULL;

static PlannedStmt *HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams);

/*
 * Module load/unload callback
 */
void
_PG_init(void)
{
	/* Planner hook */
	prev_planner_hook	= planner_hook;
	planner_hook		= HOOK_Planner_injection;
}

static PlannedStmt *HOOK_Planner_injection(Query *parse, int cursorOptions,
											ParamListInfo boundParams)
{
	PlannedStmt *stmt;
	char		*serialized_plan;

	if (prev_planner_hook)
		stmt = prev_planner_hook(parse, cursorOptions, boundParams);
	else
		stmt = standard_planner(parse, cursorOptions, boundParams);

	/* Extension is not initialized. */
	if (OidIsValid(get_extension_oid("execplan", true)))
	{
		FILE *f = fopen("/home/andrey/plans.txt", "at");
		if (stmt->paramExecTypes == NIL)
		{
			elog(LOG, "commandType: %d\n", stmt->commandType);
		}
//Assert(stmt->paramExecTypes != NIL);
		serialized_plan = nodeToString(stmt);
//		fprintf(f, "\n%s\n", serialized_plan);
		fclose(f);
	}
	return stmt;
}

#include "executor/executor.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"

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

Datum
pg_execute_plan(PG_FUNCTION_ARGS)
{
	char		*data = TextDatumGetCString(PG_GETARG_DATUM(0));
	PlannedStmt	*pstmt;
	QueryDesc	*queryDesc;
	char		queryString[5] = "NONE";
	ParamListInfo paramLI = NULL;

	elog(INFO, "MESSAGE: %s", data);

	/* Execute query plan. Based on execParallel.c ParallelQueryMain() */
	pstmt = (PlannedStmt *) stringToNode(data);
//	pstmt->paramExecTypes = NIL;
	queryDesc = CreateQueryDesc(pstmt, queryString, GetActiveSnapshot(),
								InvalidSnapshot, None_Receiver, paramLI, NULL,
								0);
	AcquirePlannerLocks(pstmt, true);

	ExecutorStart(queryDesc, 0);

	ExecutorRun(queryDesc, ForwardScanDirection, 0, true);

	/* Shut down the executor */
	ExecutorFinish(queryDesc);
	ExecutorEnd(queryDesc);
	FreeQueryDesc(queryDesc);

	PG_RETURN_BOOL(true);
}

