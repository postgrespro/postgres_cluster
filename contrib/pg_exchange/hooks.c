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
#include "nodeDistPlanExec.h"
#include "optimizer/paths.h"
#include "storage/ipc.h"


static set_rel_pathlist_hook_type	prev_set_rel_pathlist_hook = NULL;
static shmem_startup_hook_type PreviousShmemStartupHook = NULL;
static set_join_pathlist_hook_type prev_set_join_pathlist_hook = NULL;
static void second_stage_paths(List *firstStagePaths);


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
			(strcmp(((CustomPath *)inner)->methods->CustomName, DISTEXECPATHNAME) == 0))
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
			pfree(inner);
		}

		/*
		 * If outer path contains DistExec node - save its servers list and
		 * delete it from the path.
		 */
		if ((outer->pathtype == T_CustomScan) &&
			(strcmp(((CustomPath *)outer)->methods->CustomName, DISTEXECPATHNAME) == 0))
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
			pfree(outer);
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

	second_stage_paths(firstStagePaths);
}

/*
 * Add Paths same as the case of partitionwise join.
 */
static void
second_stage_paths(List *firstStagePaths)
{
	if (list_length(firstStagePaths) == 0)
		return;
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

