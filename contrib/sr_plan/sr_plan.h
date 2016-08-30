#ifndef ___SR_PLAN_H__
#define ___SR_PLAN_H__

#include "postgres.h"
#include "fmgr.h"
#include "string.h"
#include "optimizer/planner.h"
#include "nodes/print.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "utils/jsonb.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/tqual.h"
#include "utils/guc.h"
#include "utils/datum.h"
#include "utils/inval.h"
#include "utils/snapmgr.h"
#include "utils/fmgroids.h"
#include "portability/instr_time.h"
#include "storage/lock.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "parser/analyze.h"
#include "parser/parse_func.h"
#include "tcop/utility.h"
#include "catalog/pg_type.h"
#include "commands/explain.h"
#include "utils/syscache.h"
#include "funcapi.h"

Jsonb *node_tree_to_jsonb(const void *obj, Oid fake_func, bool skip_location_from_node);
void *jsonb_to_node_tree(Jsonb *json, void *(*hookPtr) (void *));
void common_walker(const void *obj, void (*callback) (void *));

#endif