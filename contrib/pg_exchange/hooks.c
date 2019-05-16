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
#include "unistd.h"

#include "common.h"
#include "exchange.h"
#include "hooks.h"
#include "partutils.h"

#include "miscadmin.h" /* for check_stack_depth() */
#include "nodeDistPlanExec.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "storage/ipc.h"


static set_rel_pathlist_hook_type	prev_set_rel_pathlist_hook = NULL;
static shmem_startup_hook_type PreviousShmemStartupHook = NULL;
static set_join_pathlist_hook_type prev_set_join_pathlist_hook = NULL;

static void HOOK_Baserel_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
															RangeTblEntry *rte);
static void arrange_partitions(RelOptInfo *rel1, RelOptInfo *rel2,
															List *restrictlist);

static void
HOOK_Baserel_paths(PlannerInfo *root, RelOptInfo *rel, Index rti,
															RangeTblEntry *rte)
{
	if (prev_set_rel_pathlist_hook)
		(*prev_set_rel_pathlist_hook) (root, rel, rti, rte);

	add_exchange_paths(root, rel, rti, rte);
}

static bool
join_path_contains_distexec(Path *path)
{
	JoinPath *jp;

	if (IsDistExecNode(path))
		return true;

	if (IsExchangeNode(path))
		jp = (JoinPath *) linitial(((CustomPath *) path)->custom_paths);
	else
		jp = (JoinPath *) path;

	Assert(jp->path.pathtype == T_HashJoin || jp->path.pathtype == T_NestLoop ||
			jp->path.pathtype == T_MergeJoin);

	if (IsDistExecNode(jp->innerjoinpath) || IsDistExecNode(jp->outerjoinpath))
		return true;

	if (jp->innerjoinpath->pathtype == T_Material)
	{
		MaterialPath *mp = (MaterialPath *) jp->innerjoinpath;

		if (IsDistExecNode(mp->subpath))
			return true;
	}
	if (jp->outerjoinpath->pathtype == T_Material)
		{
			MaterialPath *mp = (MaterialPath *) jp->outerjoinpath;

			if (IsDistExecNode(mp->subpath))
				return true;
		}
	return false;
}

static void
reset_cheapest(RelOptInfo *rel)
{
	rel->cheapest_parameterized_paths = NIL;
	rel->cheapest_startup_path = rel->cheapest_total_path =
	rel->cheapest_unique_path = NULL;
	if (rel->pathlist != NIL)
		set_cheapest(rel);
}

static bool
contain_distributed_paths(List *pathlist)
{
	ListCell *lc;

	foreach(lc, pathlist)
	{
		Path *path = lfirst(lc);

		if (IsDistExecNode(path))
			return true;
	}
	return false;
}

/*
 * TODO: We need routine cost_recalculate() that will be walk across path
 * and call cost function at each node from leaf to the root of path tree.
 */
static List *
create_distributed_join_paths(PlannerInfo *root, RelOptInfo *joinrel,
						 RelOptInfo *outerrel, RelOptInfo *innerrel,
						 JoinType jointype, JoinPathExtraData *extra,
						 ExchangeMode outer_mode, ExchangeMode inner_mode)
{
	ListCell *lc;
	List *prev_inner_pathlist;
	List *prev_outer_pathlist;
	List *prev_join_pathlist;
	List *dist_paths = NIL;
	bool distributedOuter;
	bool distributedInner;

	/* Save old pathlists. */
	prev_inner_pathlist = innerrel->pathlist;
	prev_outer_pathlist = outerrel->pathlist;
	prev_join_pathlist = joinrel->pathlist;
	innerrel->pathlist = outerrel->pathlist = joinrel->pathlist = NIL;

	distributedOuter = contain_distributed_paths(prev_outer_pathlist);
	distributedInner = contain_distributed_paths(prev_inner_pathlist);

	foreach(lc, prev_inner_pathlist)
	{
		Path *innpath = lfirst(lc);
		ExchangePath *inn_child;
		Bitmapset *inner_servers = NULL;
		ListCell *olc;
		ExchangePath *gather;

		if (IsDistExecNode(innpath))
		{
			inner_servers = extractForeignServers((CustomPath *) innpath);
			inn_child = (ExchangePath *) cstmSubPath1(innpath);
			Assert(inn_child->mode == EXCH_GATHER);
			if (IsExchangeNode(inn_child))
				inn_child = create_exchange_path(root, innerrel,
										cstmSubPath1(inn_child), inner_mode);
			else
				inn_child = create_exchange_path(root, innerrel,
												(Path *) inn_child, inner_mode);
		}
		else if (!distributedInner && distributedOuter)
		{
			/* The case of JOIN partitioned and simple relation */
			inn_child = create_exchange_path(root, innerrel, innpath, EXCH_GATHER);
		}
		else
			/* Use only distributed paths */
			continue;

		innerrel->pathlist = lappend(innerrel->pathlist, inn_child);
		Assert(list_length(innerrel->pathlist) == 1);
		foreach(olc, prev_outer_pathlist)
		{
			Path *outpath = lfirst(olc);
			ExchangePath *out_child;
			Path *path;
			Bitmapset *outer_servers = NULL;
			RelOptInfo *rel1, *rel2;
			bool res;

			/* Use only distributed paths */
			if (IsDistExecNode(outpath))
			{
				outer_servers = extractForeignServers((CustomPath *) outpath);
				if (!outer_servers && !outer_servers &&
					inner_mode ==EXCH_SHUFFLE && outer_mode ==EXCH_SHUFFLE)
					continue;

				out_child = (ExchangePath *) cstmSubPath1(outpath);
				Assert(out_child->mode == EXCH_GATHER);
				if (IsExchangeNode(out_child))
					out_child = create_exchange_path(root, outerrel,
										   cstmSubPath1(out_child), outer_mode);
				else
					out_child = create_exchange_path(root, outerrel,
												(Path *) out_child, inner_mode);
				if (!distributedInner)
					set_exchange_altrel(EXCH_GATHER, inn_child, innerrel, NULL,
														NULL, outer_servers);
			}
			else if (distributedInner && !distributedOuter)
			{
				out_child = create_exchange_path(root, outerrel, outpath, EXCH_GATHER);
				set_exchange_altrel(EXCH_GATHER, out_child, outerrel, NULL,
														NULL, inner_servers);
			}
			else
				continue;

			if (inner_mode == EXCH_SHUFFLE)
			{
				Assert(outer_mode == EXCH_SHUFFLE);
				rel1 = palloc(sizeof(RelOptInfo));
				memcpy(rel1, &out_child->altrel, sizeof(RelOptInfo));
				rel2 = palloc(sizeof(RelOptInfo));
				memcpy(rel2, &inn_child->altrel, sizeof(RelOptInfo));
			}
			else
			{
				rel1 = outerrel;
				rel2 = innerrel;
			}

			outerrel->pathlist = lappend(outerrel->pathlist, out_child);
			Assert(list_length(outerrel->pathlist) == 1);

			/* Try to add base path tree for distributed paths */
			reset_cheapest(joinrel);
			reset_cheapest(outerrel);
			reset_cheapest(innerrel);
			add_paths_to_joinrel(root, joinrel, outerrel, innerrel, jointype,
											extra->sjinfo, extra->restrictlist);

			set_cheapest(joinrel);
			Assert(!join_path_contains_distexec(joinrel->cheapest_total_path));
			Assert(joinrel->cheapest_total_path);
			list_free(joinrel->pathlist);
			joinrel->pathlist = NIL;

			/*Add distribution nodes pair into the head of the path */
			path = joinrel->cheapest_total_path;
			Assert(path->pathtype != T_MergeJoin);
			joinrel->cheapest_total_path = NULL;

			gather = create_exchange_path(root, joinrel, path, EXCH_GATHER);
			set_exchange_altrel(outer_mode, out_child, rel1, NULL, NULL,
									bms_union(inner_servers, outer_servers));
			set_exchange_altrel(inner_mode, inn_child, rel2, NULL, NULL,
									bms_union(inner_servers, outer_servers));
			cost_exchange(root, outerrel, out_child);
			cost_exchange(root, innerrel, inn_child);

			/* Check for special case */
			if (path->pathtype == T_NestLoop)
			{
				JoinPath *jp = (JoinPath *) path;

				set_exchange_altrel(EXCH_BROADCAST, out_child, NULL,
						&((ExchangePath *)cstmSubPath1(out_child))->altrel,
						NIL, bms_union(inner_servers, outer_servers));
				inn_child->mode = EXCH_STEALTH;
				cost_exchange(root, outerrel, out_child);
				cost_exchange(root, innerrel, inn_child);

				if (jp->innerjoinpath->pathtype != T_Material)
					jp->innerjoinpath = (Path *) create_material_path(innerrel,
															jp->innerjoinpath);
				Assert(jp->innerjoinpath->pathtype == T_Material);
				cost_exchange(root, joinrel, out_child);
			}
			else if (inner_mode == EXCH_SHUFFLE)
			{
				arrange_partitions(&out_child->altrel, &inn_child->altrel, extra->restrictlist);

				res = build_joinrel_partition_info(&gather->altrel, &out_child->altrel, &inn_child->altrel,
														   extra->restrictlist,
														   jointype);
				Assert(res);
			}

			set_exchange_altrel(EXCH_GATHER, gather, &out_child->altrel,
							&inn_child->altrel, extra->restrictlist,
							bms_union(inner_servers, outer_servers));
			cost_exchange(root, joinrel, gather);
			Assert(gather->altrel.part_scheme != NULL);
			path = (Path *) create_distexec_path(root, joinrel, (Path *) gather,
									bms_union(inner_servers, outer_servers));
			dist_paths = lappend(dist_paths, path);
			list_free(outerrel->pathlist);
			outerrel->pathlist = NIL;
		}
		list_free(innerrel->pathlist);
		innerrel->pathlist = NIL;
	}

	innerrel->pathlist = prev_inner_pathlist;
	outerrel->pathlist = prev_outer_pathlist;
	joinrel->pathlist = prev_join_pathlist;
	reset_cheapest(joinrel);
	reset_cheapest(outerrel);
	reset_cheapest(innerrel);

	return dist_paths;
}

static bool recursion = false;
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
	List *delpaths = NIL;
	List *pathlist = NIL;

	if (recursion)
		return;
	recursion = true;

	if (prev_set_join_pathlist_hook)
		(*prev_set_join_pathlist_hook)(root, joinrel, outerrel, innerrel,
															jointype, extra);

	/*
	 * Approach: we give to the planner only one path from outer and
	 * inner pathlists. After this we try to create paths by the
	 * add_paths_to_joinrel() routine. At last, we append exchange (GATHER)
	 * and DistExec nodes into the head of each obtained path.
	 */

	/* Delete all incorrect paths */
	foreach(lc, joinrel->pathlist)
	{
		Path *path = lfirst(lc);

		if (IsDistExecNode(path) || !join_path_contains_distexec(path))
			continue;
		delpaths = lappend(delpaths, path);
	}
	foreach(lc, delpaths)
		joinrel->pathlist = list_delete_ptr(joinrel->pathlist, lfirst(lc));

	/* Add distributed paths */
	pathlist = list_concat(pathlist, create_distributed_join_paths(root, joinrel,
					   outerrel, innerrel, jointype, extra, EXCH_GATHER,
					   EXCH_GATHER));

	pathlist = list_concat(pathlist, create_distributed_join_paths(root, joinrel,
					   outerrel, innerrel, jointype, extra, EXCH_BROADCAST,
					   EXCH_STEALTH));

	pathlist = list_concat(pathlist, create_distributed_join_paths(root, joinrel,
					   outerrel, innerrel, jointype, extra, EXCH_SHUFFLE,
					   EXCH_SHUFFLE));

	foreach(lc, pathlist)
		add_path(joinrel, lfirst(lc));

	recursion = false;
}

#include "access/hash.h"
#include "access/htup_details.h"
#include "catalog/pg_am.h"
#include "catalog/pg_opclass.h"
#include "commands/defrem.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "partitioning/partbounds.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

/* Arrange partitions */
static void
arrange_partitions(RelOptInfo *rel1, RelOptInfo *rel2, List *restrictlist)
{
	struct PartitionBoundInfoData *boundinfo = palloc(sizeof(PartitionBoundInfoData));
	PartitionScheme part_scheme = palloc(sizeof(PartitionSchemeData));
	Bitmapset *servers = NULL;
	ListCell *lc;
	int16 len = list_length(restrictlist);
	int i;
	int sid = -1;

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
	rel1->nullable_partexprs =
					(List **) palloc0(sizeof(List *) * part_scheme->partnatts);
	rel2->nullable_partexprs =
						(List **) palloc0(sizeof(List *) * part_scheme->partnatts);
	part_scheme->strategy = PARTITION_STRATEGY_HASH;
	rel1->part_scheme = rel2->part_scheme = part_scheme;

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

	boundinfo->strategy = rel1->part_scheme->strategy;
	boundinfo->default_index = -1;
	boundinfo->null_index = -1;
	boundinfo->kind = NULL;
	boundinfo->ndatums = rel1->nparts;
	boundinfo->datums = (Datum **) palloc0(boundinfo->ndatums * sizeof(Datum *));
	boundinfo->indexes = (int *) palloc(rel1->nparts * sizeof(int));
	for (i = 0; i < rel1->nparts; i++)
	{
		int modulus = rel1->nparts;
		int remainder = i;

		boundinfo->datums[i] = (Datum *) palloc(2 * sizeof(Datum));
		boundinfo->datums[i][0] = Int32GetDatum(modulus);
		boundinfo->datums[i][1] = Int32GetDatum(remainder);
		boundinfo->indexes[i] = i;
	}

	rel1->boundinfo = rel2->boundinfo = boundinfo;
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
