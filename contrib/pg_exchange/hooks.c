/* ------------------------------------------------------------------------
 *
 * hooks_exec.c
 *		Executor-related logic of the ParGRES extension.
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "common.h"
#include "exchange.h"
#include "hooks.h"
#include "partutils.h"

#include "nodeDistPlanExec.h"
#include "optimizer/paths.h"
#include "storage/ipc.h"


static set_rel_pathlist_hook_type	prev_set_rel_pathlist_hook = NULL;
static shmem_startup_hook_type PreviousShmemStartupHook = NULL;
static set_join_pathlist_hook_type prev_set_join_pathlist_hook = NULL;
static void second_stage_paths(PlannerInfo *root, List *firstStagePaths, RelOptInfo *joinrel,
							   RelOptInfo *outerrel, RelOptInfo *innerrel,
							   JoinType jointype, JoinPathExtraData *extra);


static void
HOOK_Baserel_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
				   RangeTblEntry *rte)
{
	if (prev_set_rel_pathlist_hook)
		(*prev_set_rel_pathlist_hook) (root, rel, rti, rte);

	add_exchange_paths(root, rel, rti, rte);
}

#include "unistd.h"
#include "optimizer/pathnode.h"

static JoinPath *
copy_join_pathnode(JoinPath *jp)
{
	Path *path = &jp->path;
	JoinPath *joinpath;

	switch (path->pathtype)
	{
		case T_HashJoin:
			{
				HashPath   *hash_path = makeNode(HashPath);

				memcpy(hash_path, path, sizeof(HashPath));
				joinpath = (JoinPath *) hash_path;
			}
			break;

		case T_NestLoop:
			{
				NestPath   *nest_path = makeNode(NestPath);

				memcpy(nest_path, path, sizeof(NestPath));
				joinpath = (JoinPath *) nest_path;
			}
			break;

		case T_MergeJoin:
			{
				MergePath  *merge_path = makeNode(MergePath);

				memcpy(merge_path, path, sizeof(MergePath));
				joinpath = (JoinPath *) merge_path;
			}
			break;

		default:

			Assert(0);
			break;
	}
	return joinpath;
}

/*
 * If left and right relations are partitioned (XXX: not only) we can use an
 * exchange node as left or right son for tuples shuffling of a relation in
 * accordance with partitioning scheme of another relation.
 */
static void
HOOK_Join_pathlist(PlannerInfo *root, RelOptInfo *joinrel, RelOptInfo *outerrel,
		 	 	   RelOptInfo *innerrel, JoinType jointype,
				   JoinPathExtraData *extra)
{
	ListCell *lc;
	List *firstStagePaths = NIL; /* Trivial paths, made with exchange */

	if (prev_set_join_pathlist_hook)
		prev_set_join_pathlist_hook(root, joinrel, outerrel, innerrel,
									jointype, extra);

	/*
	 * At first, traverse all paths and search for the case with Exchanges at
	 * the left or right subtree. We need to delete DistPlanExec nodes and
	 * insert only one at the head of join.
	 */
	foreach(lc, joinrel->pathlist)
	{
		JoinPath *jp;
		Path	*inner;
		Path	*outer;
		Bitmapset *servers = NULL;
		CustomPath *sub;
		Path *path = lfirst(lc);

		if ((path->pathtype != T_NestLoop) &&
			(path->pathtype != T_MergeJoin) &&
			(path->pathtype != T_HashJoin))
			continue;

		jp = (JoinPath *) path;
		inner = jp->innerjoinpath;
		outer = jp->outerjoinpath;

		/*
		 * If inner path contains DistExec node - save its servers list and
		 * delete it from the path.
		 */
		if ((inner->pathtype == T_CustomScan) &&
			(strcmp(((CustomPath *) inner)->methods->CustomName,
														DISTEXECPATHNAME) == 0))
		{
			ListCell *lc;

			sub = (CustomPath *) inner;
			foreach(lc, sub->custom_private)
			{
				Oid serverid = lfirst_oid(lc);

				Assert(OidIsValid(serverid));
				if (!bms_is_member((int)serverid, servers))
					servers = bms_add_member(servers, serverid);
			}
			Assert(list_length(sub->custom_paths) == 1);
			jp->innerjoinpath = (Path *) linitial(sub->custom_paths);
		}

		/*
		 * If outer path contains DistExec node - save its servers list and
		 * delete it from the path.
		 */
		if ((outer->pathtype == T_CustomScan) &&
			(strcmp(((CustomPath *) outer)->methods->CustomName,
														DISTEXECPATHNAME) == 0))
		{
			ListCell *lc;

			sub = (CustomPath *) outer;
			foreach(lc, sub->custom_private)
			{
				Oid serverid = lfirst_oid(lc);

				Assert(OidIsValid(serverid));
				if (!bms_is_member((int)serverid, servers))
					servers = bms_add_member(servers, serverid);
			}
			Assert(list_length(sub->custom_paths) == 1);
			jp->outerjoinpath = (Path *) linitial(sub->custom_paths);
		}

		if (servers == NULL)
			continue;

		/* Add DistExec node at the top of path. */
		path = create_distexec_path(root, joinrel,
									(Path *) copy_join_pathnode(jp),
									servers);
		add_path(joinrel, path);

		/*
		 * We need guarantee, that previous JOIN path was deleted. It was
		 * incorrect.
		 */
		list_delete_ptr(joinrel->pathlist, jp);

		/* Save link to the path for future works. */
		firstStagePaths = lappend(firstStagePaths, path);
	}

	second_stage_paths(root, firstStagePaths, joinrel, outerrel, innerrel, jointype,
			   extra);
}

#define IsDistExecNode(pathnode) ((pathnode->path.pathtype == T_CustomScan) && \
	(strcmp(((CustomPath *)pathnode)->methods->CustomName, DISTEXECPATHNAME) == 0))

static CustomPath *
duplicate_join_path(CustomPath	*distExecPath)
{
	JoinPath *jp;
	CustomPath	*newDistExecPath;
	ExchangePath *exPathnode;
	ExchangePath *exHeadPathnode;
	ExchangePath *newExPathnode;

	Assert(IsDistExecNode(distExecPath));
	exHeadPathnode = linitial(distExecPath->custom_paths);
	Assert(IsExchangeNode(&exHeadPathnode->cp.path));

	/* Copy JOIN path node */
	jp = (JoinPath *) linitial(exHeadPathnode->cp.custom_paths);
	jp = copy_join_pathnode(jp);

	/* Copy inner EXCHANGE path node */
	exPathnode = (ExchangePath *) jp->innerjoinpath;
	Assert(IsExchangeNode(&exPathnode->cp.path));
	newExPathnode = (ExchangePath *) newNode(sizeof(ExchangePath), T_CustomPath);
	memcpy(newExPathnode, exPathnode, sizeof(ExchangePath));
	newExPathnode->exchange_counter = exchange_counter++;
	jp->innerjoinpath = (Path *) newExPathnode;

	/* Copy outer EXCHANGE path node */
	exPathnode = (ExchangePath *) jp->outerjoinpath;
	Assert(IsExchangeNode(&exPathnode->cp.path));
	newExPathnode = (ExchangePath *) newNode(sizeof(ExchangePath), T_CustomPath);
	memcpy(newExPathnode, exPathnode, sizeof(ExchangePath));
	newExPathnode->exchange_counter = exchange_counter++;
	jp->outerjoinpath = (Path *) newExPathnode;

	/* Copy main EXCHANGE path node */
	newExPathnode = (ExchangePath *) newNode(sizeof(ExchangePath), T_CustomPath);
	memcpy(newExPathnode, exHeadPathnode, sizeof(ExchangePath));
	newExPathnode->exchange_counter = exchange_counter++;
	newExPathnode->cp.custom_paths = lappend(NIL, jp);

	/* Copy DistExec path node */
	newDistExecPath = makeNode(CustomPath);
	memcpy(newDistExecPath, distExecPath, sizeof(CustomPath));
	newDistExecPath->custom_paths = lappend(NIL, newExPathnode);

	return newDistExecPath;
}

#include "access/hash.h"
#include "access/htup_details.h"
#include "catalog/pg_am.h"
#include "catalog/pg_opclass.h"
#include "commands/defrem.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

/* After the JOIN the distribution will be defined by the join clause. */
static void
arrange_partitioning_attrs(RelOptInfo *rel1,
						   RelOptInfo *rel2,
						   List *restrictlist)
{
	ListCell *lc;
	PartitionScheme part_scheme = palloc(sizeof(PartitionSchemeData));
	int16 len = list_length(restrictlist);

	rel1->partexprs = (List **) palloc0(sizeof(List *) * len);
	rel2->partexprs = (List **) palloc0(sizeof(List *) * len);
	part_scheme->partnatts = 0;
	part_scheme->partopfamily = (Oid *) palloc(sizeof(Oid) * len);
	part_scheme->partopcintype = (Oid *) palloc(sizeof(Oid) * len);
	part_scheme->parttypbyval = (bool *) palloc(sizeof(bool) * len);
	part_scheme->parttyplen = (int16 *) palloc(sizeof(int16) * len);
	part_scheme->partsupfunc = (FmgrInfo *) palloc(sizeof(FmgrInfo) * len);
	part_scheme->partcollation = (Oid *) palloc(sizeof(Oid) * len);

	foreach(lc, restrictlist)
	{
		RestrictInfo *rinfo = lfirst_node(RestrictInfo, lc);
		OpExpr *opexpr = (OpExpr *) rinfo->clause;
		Expr *expr1;
		Expr *expr2;
		int16 partno = part_scheme->partnatts;
		Oid partopclass;
		HeapTuple opclasstup;
		Form_pg_opclass opclassform;

		Assert(is_opclause(opexpr));
		/* Skip clauses which can not be used for a join. */
		if (!rinfo->can_join)
			continue;

		/* Match the operands to the relation. */
		if (bms_is_subset(rinfo->left_relids, rel1->relids) &&
			bms_is_subset(rinfo->right_relids, rel2->relids))
		{
			expr1 = linitial(opexpr->args);
			expr2 = lsecond(opexpr->args);
		}
		else if (bms_is_subset(rinfo->left_relids, rel2->relids) &&
				 bms_is_subset(rinfo->right_relids, rel1->relids))
		{
			expr1 = lsecond(opexpr->args);
			expr2 = linitial(opexpr->args);
		}
		else
			Assert(0);

		rel1->partexprs[partno] = lappend(rel1->partexprs[partno], copyObject(expr1));
		rel2->partexprs[partno] = lappend(rel2->partexprs[partno], copyObject(expr2));

		part_scheme->partcollation[partno] = exprCollation((Node *) expr1);
		Assert(exprCollation((Node *) expr1) == exprCollation((Node *) expr2));
		Assert(exprType((Node *) expr1) == exprType((Node *) expr2));
		partopclass = GetDefaultOpClass(exprType((Node *) expr1), HASH_AM_OID);
		opclasstup = SearchSysCache1(CLAOID, ObjectIdGetDatum(partopclass));
		if (!HeapTupleIsValid(opclasstup))
			elog(ERROR, "cache lookup failed for partclass %u", partopclass);
		opclassform = (Form_pg_opclass) GETSTRUCT(opclasstup);
		part_scheme->partopfamily[partno] = opclassform->opcfamily;
		part_scheme->partopcintype[partno] = opclassform->opcintype;
		part_scheme->partnatts++;
		ReleaseSysCache(opclasstup);
	}
//elog(INFO, "arrange_partitioning_attrs: ");
//elog(INFO, "->1: %s ", nodeToString(rel1->partexprs[0]));
//elog(INFO, "->2: %s ", nodeToString(rel2->partexprs[0]));
//elog(INFO, "restrictlist: %s", nodeToString(restrictlist));
	/* Now we use hash partition only */
	Assert((rel1->part_scheme->strategy == PARTITION_STRATEGY_HASH) &&
			(rel1->part_scheme->strategy == rel2->part_scheme->strategy));

	part_scheme->strategy = PARTITION_STRATEGY_HASH;
	rel1->part_scheme = rel2->part_scheme = part_scheme;
}

static void
set_path_pointers(CustomPath *path, JoinPath **jp, ExchangePath **exres,
		ExchangePath **exouter, ExchangePath **exinner)
{
	/* Set pointers to the EXCHANGE nodes */
	Assert(IsExchangeNode((Path *)linitial(path->custom_paths)));
	*exres = (ExchangePath *) linitial(path->custom_paths);

	*jp = (JoinPath *) linitial((*exres)->cp.custom_paths);
	Assert((*jp)->path.pathtype == T_HashJoin);
	Assert(IsExchangeNode((*jp)->innerjoinpath) && IsExchangeNode((*jp)->outerjoinpath));
	*exinner = (ExchangePath *) (*jp)->innerjoinpath;
	*exouter = (ExchangePath *) (*jp)->outerjoinpath;
}

/* Arrange partitions */
static void
arrange_partitions(RelOptInfo *rel1,
		   RelOptInfo *rel2,
		   List *restrictlist)
{
	Bitmapset *servers = NULL;
	int i;
	int sid = -1;

	for (i = 0; i < rel1->nparts; i++)
		if (OidIsValid(rel1->part_rels[i]->serverid) &&
					!bms_is_member((int) rel1->part_rels[i]->serverid, servers))
			servers = bms_add_member(servers, (int) rel1->part_rels[i]->serverid);
		else if (!bms_is_member(0, servers))
			servers = bms_add_member(servers, 0);

	for (i = 0; i < rel2->nparts; i++)
		if (OidIsValid(rel2->part_rels[i]->serverid) &&
					!bms_is_member((int) rel2->part_rels[i]->serverid, servers))
			servers = bms_add_member(servers, (int) rel2->part_rels[i]->serverid);
		else if (!bms_is_member(0, servers))
			servers = bms_add_member(servers, 0);
	rel1->nparts = rel2->nparts = bms_num_members(servers);

	/* Create virtual partitions */
	rel1->part_rels = palloc(sizeof(RelOptInfo *) * rel1->nparts);
	rel2->part_rels = palloc(sizeof(RelOptInfo *) * rel2->nparts);
	for (i = 0; i < rel1->nparts; i++)
	{
		sid = bms_next_member(servers, sid);

		rel1->part_rels[i] = palloc(sizeof(RelOptInfo));
		rel2->part_rels[i] = palloc(sizeof(RelOptInfo));
		rel1->part_rels[i]->serverid = (Oid) sid;
		rel2->part_rels[i]->serverid = (Oid) sid;
	}
	for (i = 0; i < rel1->nparts; i++)
	{
		elog(LOG, "SHUFFLE sid=%u %d", rel1->part_rels[i]->serverid, bms_num_members(servers));
	}
}
/*
 * Add Paths same as the case of partitionwise join.
 */
static void
second_stage_paths(PlannerInfo *root, List *firstStagePaths, RelOptInfo *joinrel, RelOptInfo *outerrel,
	 	   RelOptInfo *innerrel, JoinType jointype, JoinPathExtraData *extra)
{
	ListCell *lc;

	if (list_length(firstStagePaths) == 0)
		return;

	foreach(lc, firstStagePaths)
	{
		CustomPath *path = (CustomPath *) lfirst(lc);
		JoinPath *jp;
		ExchangePath *innerex;
		ExchangePath *outerex;
		ExchangePath *expath;
		int i;

		Assert(IsDistExecNode(path));

		/*
		 * Add gather-type EXCHANGE node into the head of the path.
		 * If JOIN need to shuffle tuples than we will have virtual partitions.
		 */
		if (!IsA(((Path *) linitial(path->custom_paths)), CustomScan))
		{
			jp = (JoinPath *) linitial(path->custom_paths);
			Assert(jp->path.pathtype == T_HashJoin);
			expath = create_exchange_path(root, joinrel, (Path *) jp, GATHER_MODE);
			path->custom_paths = list_delete(path->custom_paths, jp);
			path->custom_paths = lappend(path->custom_paths, expath);
		}

		set_path_pointers(path, &jp, &expath, &outerex, &innerex);
		path->path.total_cost = 10000.;

		if (build_joinrel_partition_info(&expath->altrel, &outerex->altrel,
				&innerex->altrel, extra->restrictlist, jointype))
		{
			/* Simple case like foreign-push-join case. */
//			elog(INFO, "--- MAKE SIMPLE PATH ---");
			innerex->mode = STEALTH_MODE;
			outerex->mode = STEALTH_MODE;
		}
		else
		{
			CustomPath *newpath;
			bool res;

//			elog(INFO, "--- MAKE SHUFFLE PATH ---");
			/* Get a copy of the simple path */
			newpath = duplicate_join_path(path);
			set_path_pointers(newpath, &jp, &expath, &outerex, &innerex);
			arrange_partitioning_attrs(&outerex->altrel, &innerex->altrel,
														extra->restrictlist);
			arrange_partitions(&outerex->altrel, &innerex->altrel,
					   	   	   	   	   	   	   			extra->restrictlist);
			res = build_joinrel_partition_info(&expath->altrel,
											   &outerex->altrel,
											   &innerex->altrel,
											   extra->restrictlist,
											   jointype);
			Assert(res);
			Assert(expath->altrel.part_scheme != NULL);
			innerex->mode = SHUFFLE_MODE;
			outerex->mode = SHUFFLE_MODE;
			newpath->path.total_cost = 0.1;
			add_path(joinrel, (Path *) newpath);
		}

		Assert(expath->mode == GATHER_MODE);
		Assert(expath->altrel.nparts > 0);
		Assert(outerex->altrel.nparts > 0);
		Assert(innerex->altrel.nparts > 0);

		/*
		 * Set partitions preferences at altrel field of after-JOIN exchange
		 */
		for (i = 0; i < expath->altrel.nparts; i++)
		{
			Assert(expath->altrel.part_rels[i] == NULL);
			expath->altrel.part_rels[i] = palloc(sizeof(RelOptInfo));
			memcpy(expath->altrel.part_rels[i], innerex->altrel.part_rels[i], sizeof(RelOptInfo));
		}
	}
}

static void
HOOK_shmem_startup(void)
{
	bool found;
	HASHCTL		hash_info;

	if (PreviousShmemStartupHook)
		(*PreviousShmemStartupHook)();

	MemSet(&hash_info, 0, sizeof(hash_info));
	hash_info.keysize = sizeof(Oid);
	hash_info.entrysize = sizeof(DMQDestinations);

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	ExchShmem = ShmemInitStruct("pg_exchange",
								sizeof(ExchangeSharedState),
								&found);
	if (!found)
		ExchShmem->lock = &(GetNamedLWLockTranche("pg_exchange"))->lock;

	ExchShmem->htab = ShmemInitHash("dmq_destinations",
								10,
								1024,
								&hash_info,
								HASH_ELEM);
	LWLockRelease(AddinShmemInitLock);
}

void
EXEC_Hooks_init(void)
{
	prev_set_rel_pathlist_hook = set_rel_pathlist_hook;
	set_rel_pathlist_hook = HOOK_Baserel_paths;
	prev_set_join_pathlist_hook = set_join_pathlist_hook;
	set_join_pathlist_hook = HOOK_Join_pathlist;

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = HOOK_shmem_startup;
}

