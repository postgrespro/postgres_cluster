#include "sr_plan.h"
#include "commands/event_trigger.h"

PG_MODULE_MAGIC;

void	_PG_init(void);
void	_PG_fini(void);

static bool sr_plan_write_mode = false;

PlannedStmt *sr_planner(Query *parse,
						int cursorOptions,
						ParamListInfo boundParams);
/*static void
PlanCacheRelCallback(Datum arg, Oid relid);*/

void sr_analyze(ParseState *pstate,
				Query *query);

bool sr_query_walker(Query *node, void *context);
bool sr_query_expr_walker(Node *node, void *context);
void *replace_fake(void *node);
void walker_callback(void *node);

static Oid sr_plan_fake_func = 0;
static Oid dropped_objects_func = 0;

struct QueryParams
{
	int location;
	void *node;
};

List *query_params;
const char *query_text;

void sr_analyze(ParseState *pstate, Query *query)
{
	query_text = pstate->p_sourcetext;
}

PlannedStmt *sr_planner(Query *parse,
						int cursorOptions,
						ParamListInfo boundParams)
{
	PlannedStmt *pl_stmt;
	Jsonb *out_jsonb;
	Jsonb *out_jsonb2;
	int query_hash;
	RangeVar *sr_plans_table_rv;
	Relation sr_plans_heap;
	Relation query_index_rel;
	HeapTuple tuple;
	/* For make new tuple */
	Datum		values[6];
	static bool nulls[6] = {false, false, false, false, false, false};
	/* For search tuple */
	Datum		search_values[6];
	static bool search_nulls[6] = {false, false, false, false, false, false};
	bool find_ok = false;
	LOCKMODE heap_lock = AccessShareLock;
	Oid query_index_rel_oid;
	IndexScanDesc query_index_scan;
	ScanKeyData key;

	if(sr_plan_write_mode)
		heap_lock = RowExclusiveLock;

	if (!sr_plan_fake_func)
	{
		Oid args[1] = {ANYELEMENTOID};
		sr_plan_fake_func = LookupFuncName(list_make1(makeString("_p")), 1, args, true);
	}
	
	
	out_jsonb = node_tree_to_jsonb(parse, sr_plan_fake_func, true);
	query_hash = DatumGetInt32(DirectFunctionCall1(jsonb_hash, PointerGetDatum(out_jsonb)));
	
	query_params = NULL;
	/* Make list with all _p functions and his position */
	sr_query_walker((Query *)parse, NULL);

	sr_plans_table_rv = makeRangeVar("public", "sr_plans", -1);
	sr_plans_heap = heap_openrv(sr_plans_table_rv, heap_lock);

	query_index_rel_oid = DatumGetObjectId(DirectFunctionCall1(to_regclass, CStringGetDatum("sr_plans_query_hash_idx")));
	if (query_index_rel_oid == InvalidOid)
	{
		elog(WARNING, "Not found sr_plans_query_hash_idx index");
		return standard_planner(parse, cursorOptions, boundParams);
	}

	query_index_rel = index_open(query_index_rel_oid, heap_lock);
	query_index_scan = index_beginscan(
				sr_plans_heap,
				query_index_rel,
				SnapshotSelf,
				1,
				0
	);
	ScanKeyInit(&key,
				1,
				BTEqualStrategyNumber,
				F_INT4EQ,
				Int32GetDatum(query_hash));

	index_rescan(query_index_scan,
				&key, 1,
				NULL, 0);
	for (;;)
	{
		HeapTuple local_tuple;
		local_tuple = index_getnext(query_index_scan, ForwardScanDirection);

		if (local_tuple == NULL)
			break;

		heap_deform_tuple(local_tuple, sr_plans_heap->rd_att,
						  search_values, search_nulls);

		/* Check enabled and validate field */
		if (DatumGetBool(search_values[4]) &&
			DatumGetBool(search_values[5])) {
			find_ok = true;
			break;
		}
	}
	index_endscan(query_index_scan);
	
	if (find_ok)
	{
		elog(WARNING, "Ok we find saved plan.");
		out_jsonb2 = (Jsonb *)DatumGetPointer(PG_DETOAST_DATUM(search_values[3]));
		if (query_params != NULL)
			pl_stmt = jsonb_to_node_tree(out_jsonb2, &replace_fake);
		else
			pl_stmt = jsonb_to_node_tree(out_jsonb2, NULL);
	}
	/* Ok, we supported duplicate query_hash but only if all plans with query_hash disabled.*/
	else if (sr_plan_write_mode)
	{
		bool not_have_duplicate = true;
		Datum plan_hash;
		
		pl_stmt = standard_planner(parse, cursorOptions, boundParams);
		out_jsonb2 = node_tree_to_jsonb(pl_stmt, 0, false);
		plan_hash = DirectFunctionCall1(jsonb_hash, PointerGetDatum(out_jsonb2));

		query_index_scan = index_beginscan(sr_plans_heap,
										   query_index_rel,
										   SnapshotSelf,
										   1,
										   0);
		index_rescan(query_index_scan,
					 &key, 1,
					 NULL, 0);
		for (;;)
		{
			HeapTuple local_tuple;
			ItemPointer tid = index_getnext_tid(query_index_scan, ForwardScanDirection);
			if (tid == NULL)
				break;

			local_tuple = index_fetch_heap(query_index_scan);
			heap_deform_tuple(local_tuple, sr_plans_heap->rd_att,
							  search_values, search_nulls);

			/* Detect full plan duplicate */
			if (search_values[1] == plan_hash)
			{
				not_have_duplicate = false;
				break;
			}
		}
		index_endscan(query_index_scan);

		if (not_have_duplicate)
		{
			values[0] = Int32GetDatum(query_hash);
			values[1] = plan_hash;
			values[2] = CStringGetTextDatum(query_text);
			values[3] = PointerGetDatum(out_jsonb2);
			values[4] = BoolGetDatum(false);
			values[5] = BoolGetDatum(true);
			tuple = heap_form_tuple(sr_plans_heap->rd_att, values, nulls);
			simple_heap_insert(sr_plans_heap, tuple);
			index_insert(query_index_rel,
						 values, nulls,
						 &(tuple->t_self),
						 sr_plans_heap,
						 UNIQUE_CHECK_NO);
		}
	}
	else
	{
		pl_stmt = standard_planner(parse, cursorOptions, boundParams);
	}
	
	index_close(query_index_rel, heap_lock);
	heap_close(sr_plans_heap, heap_lock);
	return pl_stmt;
}

bool sr_query_walker(Query *node, void *context)
{
		if (node == NULL)
			return false;
		// check for nodes that special work is required for, eg:
		if (IsA(node, FromExpr))
		{
			return sr_query_expr_walker((Node *)node, node);
		}
		
		// for any node type not specially processed, do:
		if (IsA(node, Query))
		{
			return query_tree_walker(node, sr_query_walker, node, 0);
		}
		else
			return false;
}

bool sr_query_expr_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;

	if (IsA(node, FuncExpr) && ((FuncExpr *)node)->funcid == sr_plan_fake_func)
	{
		struct QueryParams *param = (struct QueryParams *) palloc(sizeof(struct QueryParams));
		param->location = ((FuncExpr *)node)->location;
		param->node = ((FuncExpr *)node)->args->head->data.ptr_value;
		query_params = lappend(query_params, param);

		return false;
	}

	return expression_tree_walker(node, sr_query_expr_walker, node);
}

/* Replaced params from query_params by positions*/
void *replace_fake(void *node)
{
	if (node == NULL)
		return NULL;

	if (IsA(node, FuncExpr) && ((FuncExpr *)node)->funcid == sr_plan_fake_func)
	{
		ListCell *cell_params;
		foreach(cell_params, query_params)
		{
			if (((struct QueryParams *)lfirst(cell_params))->location == ((FuncExpr *)node)->location)
			{
				((FuncExpr *)node)->args->head->data.ptr_value = ((struct QueryParams *)lfirst(cell_params))->node;
				break;
			}
		}
	}
	return node;
}

void _PG_init(void) {
	DefineCustomBoolVariable("sr_plan.write_mode",
							 "Save all plans for all query.",
							 NULL,
							 &sr_plan_write_mode,
							 false,
							 PGC_SUSET,
							 0,
							 NULL,
							 NULL,
							 NULL);

	planner_hook = &sr_planner;
	post_parse_analyze_hook = &sr_analyze;
}

void _PG_fini(void) {
	planner_hook = NULL;
	post_parse_analyze_hook = NULL;
}

PG_FUNCTION_INFO_V1(_p);

Datum
_p(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

PG_FUNCTION_INFO_V1(explain_jsonb_plan);

Datum
explain_jsonb_plan(PG_FUNCTION_ARGS)
{
	Jsonb *jsonb_plan = PG_GETARG_JSONB(0);
	Node *plan;

	if (jsonb_plan == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Not found jsonb arg"));
	plan = jsonb_to_node_tree(jsonb_plan, NULL);
	if (plan == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Not found right jsonb plan"));

	if (IsA(plan, PlannedStmt))
	{
		ExplainState *es = NewExplainState();
		es->costs = false;
		ExplainBeginOutput(es);
		PG_TRY();
		{
			ExplainOnePlan((PlannedStmt *)plan, NULL,
					   es, NULL,
					   NULL, NULL);
			PG_RETURN_TEXT_P(cstring_to_text(es->str->data));
		}
		PG_CATCH();
		{
			/* Magic hack but work. In ExplainOnePlan we twice touched snapshot before die.*/
			UnregisterSnapshot(GetActiveSnapshot());
			UnregisterSnapshot(GetActiveSnapshot());
			PopActiveSnapshot();
			ExplainEndOutput(es);
			PG_RETURN_TEXT_P(cstring_to_text("Invalid plan"));
		}
		PG_END_TRY();
		ExplainEndOutput(es);
	}
	else
	{
		PG_RETURN_TEXT_P(cstring_to_text("Not found plan"));
	}
}


PG_FUNCTION_INFO_V1(sr_plan_invalid_table);

Datum
sr_plan_invalid_table(PG_FUNCTION_ARGS)
{
	FunctionCallInfoData fcinfo_new;
	ReturnSetInfo rsinfo;
	FmgrInfo	flinfo;
	ExprContext econtext;
	TupleTableSlot *slot = NULL;
	RangeVar *sr_plans_table_rv;
	Relation sr_plans_heap;
	Datum		search_values[6];
	static bool search_nulls[6];
	static bool search_replaces[6];
	HeapScanDesc heapScan;
	Jsonb *jsonb;
	JsonbValue relation_key;

	econtext.ecxt_per_query_memory = CurrentMemoryContext;

	if (!CALLED_AS_EVENT_TRIGGER(fcinfo))  /* internal error */
		elog(ERROR, "not fired by event trigger manager");

	sr_plans_table_rv = makeRangeVar("public", "sr_plans", -1);
	sr_plans_heap = heap_openrv(sr_plans_table_rv, RowExclusiveLock);
	
	relation_key.type = jbvString;
	relation_key.val.string.len = strlen("relationOids");
	relation_key.val.string.val = "relationOids";

	rsinfo.type = T_ReturnSetInfo;
	rsinfo.econtext = &econtext;
	//rsinfo.expectedDesc = fcache->funcResultDesc;
	rsinfo.allowedModes = (int) (SFRM_ValuePerCall | SFRM_Materialize);
	/* note we do not set SFRM_Materialize_Random or _Preferred */
	rsinfo.returnMode = SFRM_Materialize;
	/* isDone is filled below */
	rsinfo.setResult = NULL;
	rsinfo.setDesc = NULL;

	if (!dropped_objects_func)
	{
		Oid args[1];
		dropped_objects_func = LookupFuncName(list_make1(makeString("pg_event_trigger_dropped_objects")), 0, args, true);
	}
	
	/* Look up the function */
	fmgr_info(dropped_objects_func, &flinfo);

	InitFunctionCallInfoData(fcinfo_new, &flinfo, 0, InvalidOid, NULL, (fmNodePtr)&rsinfo);
	(*pg_event_trigger_dropped_objects) (&fcinfo_new);

	/* Check for null result, since caller is clearly not expecting one */
	if (fcinfo_new.isnull)
		elog(ERROR, "function %p returned NULL", (void *) pg_event_trigger_dropped_objects);

	slot = MakeTupleTableSlot();
	ExecSetSlotDescriptor(slot, rsinfo.setDesc);

	while(tuplestore_gettupleslot(rsinfo.setResult, true,
						false, slot))
	{
		bool isnull = false;
		bool find_plan = false;
		int droped_relation_oid = DatumGetInt32(slot_getattr(slot, 2, &isnull));
		char *type_name = TextDatumGetCString(slot_getattr(slot, 7, &isnull));
		heapScan = heap_beginscan(sr_plans_heap, SnapshotSelf, 0, (ScanKey) NULL);
		for (;;)
		{
			HeapTuple local_tuple;
			local_tuple = heap_getnext(heapScan, ForwardScanDirection);
			if (local_tuple == NULL)
				break;

			heap_deform_tuple(local_tuple, sr_plans_heap->rd_att,
							  search_values, search_nulls);

			if (DatumGetBool(search_values[5])) {
				int type;
				JsonbValue v;
				JsonbIterator *it;
				JsonbValue *node_relation;
				HeapTuple newtuple;

				jsonb = (Jsonb *)DatumGetPointer(PG_DETOAST_DATUM(search_values[3]));

				/*TODO: need move to function*/
				if (strcmp(type_name, "table") == 0)
				{
					node_relation = findJsonbValueFromContainer(&jsonb->root,
												JB_FOBJECT,
												&relation_key);
					it = JsonbIteratorInit(node_relation->val.binary.data);
					while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
					{
						if (type == WJB_ELEM)
						{
							int oid = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));
							if (oid == droped_relation_oid)
							{
								find_plan = true;
								break;
							}
							
						}
					}
				}
				else if (strcmp(type_name, "index") == 0)
				{
					it = JsonbIteratorInit(&jsonb->root);
					while ((type = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
					{
						if (type == WJB_KEY &&
							v.type == jbvString &&
							strncmp(v.val.string.val, "indexid", v.val.string.len) == 0)
						{
							type = JsonbIteratorNext(&it, &v, false);
							if (type == WJB_DONE)
								break;
							if (type == WJB_VALUE)
							{
								int oid = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));
								if (oid == droped_relation_oid)
								{
									find_plan = true;
									break;
								}
							}	
						}
					}
				}
				if (find_plan)
				{
					elog(WARNING, "Invalidate saved plan with query:\n\t%s", TextDatumGetCString(search_values[2]));
					/* update existing entry */
					search_values[5] = BoolGetDatum(false);
					search_replaces[5] = true;

					newtuple = heap_modify_tuple(local_tuple, RelationGetDescr(sr_plans_heap),
										 search_values, search_nulls, search_replaces);
					simple_heap_update(sr_plans_heap, &newtuple->t_self, newtuple);
				}
			}
		}
		heap_endscan(heapScan);
	}
	heap_close(sr_plans_heap, RowExclusiveLock);

	PG_RETURN_NULL();
}
