/*
 * partutils.c
 *
 */
#include "partutils.h"

#include "optimizer/paths.h"
#include "partitioning/partbounds.h"


bool
build_joinrel_partition_info(RelOptInfo *joinrel, RelOptInfo *outer_rel,
							 RelOptInfo *inner_rel, List *restrictlist,
							 JoinType jointype)
{
	int			partnatts;
	int			cnt;
	PartitionScheme part_scheme;

	/*
	 * We can only consider this join as an input to further partitionwise
	 * joins if (a) the input relations are partitioned and have
	 * consider_partitionwise_join=true, (b) the partition schemes match, and
	 * (c) we can identify an equi-join between the partition keys.  Note that
	 * if it were possible for have_partkey_equi_join to return different
	 * answers for the same joinrel depending on which join ordering we try
	 * first, this logic would break.  That shouldn't happen, though, because
	 * of the way the query planner deduces implied equalities and reorders
	 * the joins.  Please see optimizer/README for details.
	 */
	if (!IS_PARTITIONED_REL(outer_rel) || !IS_PARTITIONED_REL(inner_rel) ||
		outer_rel->part_scheme != inner_rel->part_scheme ||
		!have_partkey_equi_join(joinrel, outer_rel, inner_rel,
								jointype, restrictlist))
		return false;

	part_scheme = outer_rel->part_scheme;

	Assert(REL_HAS_ALL_PART_PROPS(outer_rel) &&
		   REL_HAS_ALL_PART_PROPS(inner_rel));

	/*
	 * For now, our partition matching algorithm can match partitions only
	 * when the partition bounds of the joining relations are exactly same.
	 * So, bail out otherwise.
	 */
	if (outer_rel->nparts != inner_rel->nparts ||
		!partition_bounds_equal(part_scheme->partnatts,
								part_scheme->parttyplen,
								part_scheme->parttypbyval,
								outer_rel->boundinfo, inner_rel->boundinfo))
	{
		Assert(0);
		Assert(!IS_PARTITIONED_REL(joinrel));
		return false;
	}

	/*
	 * This function will be called only once for each joinrel, hence it
	 * should not have partition scheme, partition bounds, partition key
	 * expressions and array for storing child relations set.
	 */
	Assert(!joinrel->part_scheme && !joinrel->partexprs &&
		   !joinrel->nullable_partexprs && !joinrel->part_rels &&
		   !joinrel->boundinfo);

	/*
	 * Join relation is partitioned using the same partitioning scheme as the
	 * joining relations and has same bounds.
	 */
	joinrel->part_scheme = part_scheme;
	joinrel->boundinfo = outer_rel->boundinfo;
	partnatts = joinrel->part_scheme->partnatts;
	joinrel->partexprs = (List **) palloc0(sizeof(List *) * partnatts);
	joinrel->nullable_partexprs =
		(List **) palloc0(sizeof(List *) * partnatts);
	joinrel->nparts = outer_rel->nparts;
	joinrel->part_rels =
		(RelOptInfo **) palloc0(sizeof(RelOptInfo *) * joinrel->nparts);

	/*
	 * Construct partition keys for the join.
	 *
	 * An INNER join between two partitioned relations can be regarded as
	 * partitioned by either key expression.  For example, A INNER JOIN B ON
	 * A.a = B.b can be regarded as partitioned on A.a or on B.b; they are
	 * equivalent.
	 *
	 * For a SEMI or ANTI join, the result can only be regarded as being
	 * partitioned in the same manner as the outer side, since the inner
	 * columns are not retained.
	 *
	 * An OUTER join like (A LEFT JOIN B ON A.a = B.b) may produce rows with
	 * B.b NULL. These rows may not fit the partitioning conditions imposed on
	 * B.b. Hence, strictly speaking, the join is not partitioned by B.b and
	 * thus partition keys of an OUTER join should include partition key
	 * expressions from the OUTER side only.  However, because all
	 * commonly-used comparison operators are strict, the presence of nulls on
	 * the outer side doesn't cause any problem; they can't match anything at
	 * future join levels anyway.  Therefore, we track two sets of
	 * expressions: those that authentically partition the relation
	 * (partexprs) and those that partition the relation with the exception
	 * that extra nulls may be present (nullable_partexprs).  When the
	 * comparison operator is strict, the latter is just as good as the
	 * former.
	 */
	for (cnt = 0; cnt < partnatts; cnt++)
	{
		List	   *outer_expr;
		List	   *outer_null_expr;
		List	   *inner_expr;
		List	   *inner_null_expr;
		List	   *partexpr = NIL;
		List	   *nullable_partexpr = NIL;

		outer_expr = list_copy(outer_rel->partexprs[cnt]);
		outer_null_expr = list_copy(outer_rel->nullable_partexprs[cnt]);
		inner_expr = list_copy(inner_rel->partexprs[cnt]);
		inner_null_expr = list_copy(inner_rel->nullable_partexprs[cnt]);

		switch (jointype)
		{
			case JOIN_INNER:
				partexpr = list_concat(outer_expr, inner_expr);
				nullable_partexpr = list_concat(outer_null_expr,
												inner_null_expr);
				break;

			case JOIN_SEMI:
			case JOIN_ANTI:
				partexpr = outer_expr;
				nullable_partexpr = outer_null_expr;
				break;

			case JOIN_LEFT:
				partexpr = outer_expr;
				nullable_partexpr = list_concat(inner_expr,
												outer_null_expr);
				nullable_partexpr = list_concat(nullable_partexpr,
												inner_null_expr);
				break;

			case JOIN_FULL:
				nullable_partexpr = list_concat(outer_expr,
												inner_expr);
				nullable_partexpr = list_concat(nullable_partexpr,
												outer_null_expr);
				nullable_partexpr = list_concat(nullable_partexpr,
												inner_null_expr);
				break;

			default:
				elog(ERROR, "unrecognized join type: %d", (int) jointype);

		}

		joinrel->partexprs[cnt] = partexpr;
		joinrel->nullable_partexprs[cnt] = nullable_partexpr;
	}
	return true;
}
