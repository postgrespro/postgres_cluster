/*
 * support Append plan for ORed clauses
 * Teodor Sigaev <teodor@sigaev.ru>
 */
#include "postgres.h"

#include "access/skey.h"
#include "catalog/pg_am.h"
#include "optimizer/cost.h"
#include "optimizer/clauses.h"
#include "optimizer/paths.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/predtest.h"
#include "optimizer/restrictinfo.h"
#include "utils/lsyscache.h"

typedef struct CKey {
	RestrictInfo	*rinfo;		/* original rinfo */
	int				n;			/* IndexPath's number in bitmapquals */
	OpExpr			*normalizedexpr; /* expression with Var on left */
	Var				*var;	
	Node			*value;
	Oid				opfamily;
	int				strategy;
	uint8			strategyMask;
} CKey;
#define	BTMASK(x)	( 1<<(x) )

static 	List* find_common_quals( BitmapOrPath *path );
static 	RestrictInfo* unionOperation(CKey	*key);
static	BitmapOrPath* cleanup_nested_quals( PlannerInfo *root, RelOptInfo *rel, BitmapOrPath *path );
static  List* sortIndexScans( List* ipaths );
static	List* reverseScanDirIdxPaths(List *indexPaths);
static	IndexPath* reverseScanDirIdxPath(IndexPath *ipath);

#define IS_LESS(a)	( (a) == BTLessStrategyNumber || (a)== BTLessEqualStrategyNumber )
#define IS_GREATER(a)	( (a) == BTGreaterStrategyNumber || (a) == BTGreaterEqualStrategyNumber )
#define	IS_ONE_DIRECTION(a,b)	( 		\
	( IS_LESS(a) && IS_LESS(b) )		\
	|| 									\
	( IS_GREATER(a) && IS_GREATER(b) )	\
)

typedef struct ExExpr {
	OpExpr		*expr;
	Oid			opfamily;
	Oid			lefttype;
	Oid			righttype;
	int			strategy;
	int			attno;
} ExExpr;


typedef struct IndexPathEx {
	IndexPath	*path;
	List		*preparedquals; /* list of ExExpr */
} IndexPathEx;


/*----------
 * keybased_rewrite_or_index_quals
 *	  Examine join OR-of-AND quals to see if any useful common restriction 
 *	  clauses can be extracted.  If so, try to use for creating new index paths.
 *
 * For example consider
 *		WHERE ( a.x=5 and a.y>10 ) OR a.x>5
 *	and there is an index on a.x or (a.x, a.y). So, plan
 *	will be seqscan or BitmapOr(IndexPath,IndexPath)
 *  So, we can add some restriction:
 *		WHERE (( a.x=5 and a.y>10 ) OR a.x>5) AND a.x>=5
 *	and plan may be so
 *		Index Scan (a.x>=5)
 *		Filter( (( a.x=5 and a.y>10 ) OR a.x>5) )
 *
 * We don't want to add new clauses to baserestrictinfo, just
 * use it as index quals.
 *
 * Next thing which it possible to test is use append of
 * searches instead of OR.
 * For example consider
 *	WHERE ( a.x=5 and a.y>10 ) OR a.x>6
 * and there is an index on (a.x) (a.x, a.y)
 * So, we can suggest follow plan:
 *	Append
 *	Filter ( a.x=5 and a.y>10 ) OR (a.x>6)
 *		Index Scan (a.x=5)	--in case of index on (a.x)
 *		Index Scan (a.x>6)
 * For that we should proof that index quals isn't overlapped,
 * also, some index quals may be containedi in other, so it can be eliminated  
 */

void
keybased_rewrite_index_paths(PlannerInfo *root, RelOptInfo *rel)
{
	BitmapOrPath *bestpath = NULL;
	ListCell   *i;
	List		*commonquals;
	AppendPath  *appendidxpath;
	List		*indexPaths;
	IndexOptInfo *index;

	foreach(i, rel->baserestrictinfo)
	{
		RestrictInfo *rinfo = (RestrictInfo *) lfirst(i);

		if (restriction_is_or_clause(rinfo) &&
			!rinfo->outerjoin_delayed)
		{
			/*
			 * Use the generate_bitmap_or_paths() machinery to estimate the
			 * value of each OR clause.  We can use regular restriction
			 * clauses along with the OR clause contents to generate
			 * indexquals.	We pass outer_rel = NULL so that sub-clauses
			 * that are actually joins will be ignored.
			 */
			List	   *orpaths;
			ListCell   *k;

			orpaths = generate_bitmap_or_paths(root, rel,
											   list_make1(rinfo),
											   rel->baserestrictinfo);

			/* Locate the cheapest OR path */
			foreach(k, orpaths)
			{
				BitmapOrPath *path = (BitmapOrPath *) lfirst(k);

				Assert(IsA(path, BitmapOrPath));
				if (bestpath == NULL ||
					path->path.total_cost < bestpath->path.total_cost)
				{
					bestpath = path;
				}
			}
		}
	}

	/* Fail if no suitable clauses found */
	if (bestpath == NULL)
		return;

	commonquals = find_common_quals(bestpath);
	/* Found quals with the same args, but with, may be, different
		operations */
	if ( commonquals != NULL ) {
		List		*addon=NIL;

		foreach(i, commonquals) {
			CKey	*key = (CKey*)lfirst(i);
			RestrictInfo	*rinfo;

			/*
			 * get 'union' of operation for key
			 */
			rinfo = unionOperation(key);
			if ( rinfo ) 
				addon = lappend(addon, rinfo);
		}

		/*
		 * Ok, we found common quals and union it, so we will try to
		 * create new possible index paths
		 */
		if ( addon ) {
			List	*origbaserestrictinfo = list_copy(rel->baserestrictinfo);

			rel->baserestrictinfo = list_concat(rel->baserestrictinfo, addon);

			create_index_paths(root, rel);
		
			rel->baserestrictinfo = origbaserestrictinfo;
		}
	}

	/*
	 * Check if indexquals isn't overlapped and all index scan
	 * are on the same index.
	 */
	if ( (bestpath = cleanup_nested_quals( root, rel, bestpath )) == NULL )
		return;
	
	if (IsA(bestpath, IndexPath)) {
		IndexPath	*ipath = (IndexPath*)bestpath;

		Assert(list_length(ipath->indexquals) == list_length(ipath->indexqualcols));
		/*
		 * It's possible to do only one index scan :)
		 */
		index = ipath->indexinfo;

		if ( root->query_pathkeys != NIL && index->sortopfamily && OidIsValid(index->sortopfamily[0]) )
		{
			List	*pathkeys;

			pathkeys = build_index_pathkeys(root, index,
													ForwardScanDirection);
			pathkeys = truncate_useless_pathkeys(root, rel,
													pathkeys);

			ipath->path.pathkeys = pathkeys;
			add_path(rel, (Path *) ipath);

			/*
			 * add path ordered in backward direction if our pathkeys
			 * is still unusable...
			 */
			if ( pathkeys == NULL || pathkeys_useful_for_ordering(root, pathkeys) == 0 ) 
			{
				pathkeys = build_index_pathkeys(root, index,
													BackwardScanDirection);
				pathkeys = truncate_useless_pathkeys(root, rel,
														pathkeys);

				ipath = reverseScanDirIdxPath( ipath );

				ipath->path.pathkeys = pathkeys;
				add_path(rel, (Path *) ipath);
			}
		} else
			add_path(rel, (Path *) ipath);
		return;
	}

	/* recount costs */
	foreach(i, bestpath->bitmapquals ) {
		IndexPath	*ipath = (IndexPath*)lfirst(i);

		Assert( IsA(ipath, IndexPath) );
		Assert(list_length(ipath->indexquals) == list_length(ipath->indexqualcols));
		ipath->path.rows = rel->tuples * clauselist_selectivity(root,
															ipath->indexquals,
															rel->relid,
															JOIN_INNER, 
															NULL);
		ipath->path.rows = clamp_row_est(ipath->path.rows);
		cost_index(ipath, root, 1);
	}

	/*
	 * Check if append index can suggest ordering of result
	 *
	 * Also, we should say to AppendPath about targetlist:
	 * target list will be taked from indexscan
	 */
	index = ((IndexPath*)linitial(bestpath->bitmapquals))->indexinfo;
	if ( root->query_pathkeys != NIL && index->sortopfamily && OidIsValid(index->sortopfamily[0]) && 
				(indexPaths = sortIndexScans( bestpath->bitmapquals )) !=NULL ) {
		List	*pathkeys;

		pathkeys = build_index_pathkeys(root, index, 
										ForwardScanDirection);
		pathkeys = truncate_useless_pathkeys(root, rel,
											 pathkeys);

		appendidxpath = create_append_path(rel, indexPaths, NULL, true,
										   pathkeys, 0);
		add_path(rel, (Path *) appendidxpath);

		/*
		 * add path ordered in backward direction if our pathkeys
		 * is still unusable...
		 */
		if ( pathkeys == NULL || pathkeys_useful_for_ordering(root, pathkeys) == 0 ) {
			
			pathkeys = build_index_pathkeys(root, index, 
										BackwardScanDirection);
			pathkeys = truncate_useless_pathkeys(root, rel,
											 pathkeys);

			indexPaths = reverseScanDirIdxPaths(indexPaths);
			appendidxpath = create_append_path(rel, indexPaths, NULL, true,
											   pathkeys, 0);
			add_path(rel, (Path *) appendidxpath);
		}
	} else {
		appendidxpath = create_append_path(rel, bestpath->bitmapquals, NULL,
										   true, NIL, 0);
		add_path(rel, (Path *) appendidxpath);
	}
}

/*
 * transformToCkey - transform RestrictionInfo
 * to CKey struct. Fucntion checks possibility and correctness of
 * RestrictionInfo to use it as common key, normalizes 
 * expression and "caches" some information. Note,
 * original RestrictInfo isn't touched
 */

static CKey*
transformToCkey( IndexOptInfo *index, RestrictInfo* rinfo, int indexcol) {
	CKey	*key;
	OpExpr  *expr = (OpExpr*)rinfo->clause;

	if ( rinfo->outerjoin_delayed )
		return NULL;

	if ( !IsA(expr, OpExpr) )
		return NULL;

	if ( contain_mutable_functions((Node*)expr) )
		return NULL;
	
	if ( list_length( expr->args ) != 2 )
		return NULL;

	key = (CKey*)palloc(sizeof(CKey));
	key->rinfo = rinfo;

	key->normalizedexpr = (OpExpr*)copyObject( expr ); 
	if (!bms_equal(rinfo->left_relids, index->rel->relids))
		CommuteOpExpr(key->normalizedexpr);

	/*
	 * fix_indexqual_operand returns copy of object
	 */
	key->var = (Var*)fix_indexqual_operand(linitial(key->normalizedexpr->args), index, indexcol);
	Assert( IsA(key->var, Var) );

	key->opfamily = index->opfamily[ key->var->varattno - 1 ];

	/* restore varattno, because it may be different in different index */
	key->var->varattno = key->var->varoattno;

	key->value = (Node*)lsecond(key->normalizedexpr->args);

	key->strategy = get_op_opfamily_strategy( key->normalizedexpr->opno, key->opfamily);
	Assert( key->strategy != InvalidStrategy );

	key->strategyMask = BTMASK(key->strategy);

	return key;
}

/*
 * get_index_quals - get list of quals in
 * CKeys form
 */

static List*
get_index_quals(IndexPath *path, int cnt) {
	ListCell	*i, *c;
	List	*quals = NIL;

	Assert(list_length(path->indexquals) == list_length(path->indexqualcols));
	forboth(i, path->indexquals, c, path->indexqualcols) {
		CKey	*k = transformToCkey( path->indexinfo, (RestrictInfo*)lfirst(i), lfirst_int(c) );
		if ( k ) {
			k->n = cnt;
			quals = lappend(quals, k);
		}
	}
	return quals;
}

/*
 * extract all quals from bitmapquals->indexquals for
 */
static List*
find_all_quals( BitmapOrPath *path, int *counter ) {
	ListCell   *i,*j;
	List	*allquals = NIL;

	*counter = 0;

	foreach(i, path->bitmapquals )
	{
		Path *subpath = (Path *) lfirst(i);

		if ( IsA(subpath, BitmapAndPath) ) {
			foreach(j, ((BitmapAndPath*)subpath)->bitmapquals) {
				Path *subsubpath = (Path *) lfirst(i);

				if ( IsA(subsubpath, IndexPath) ) {
					if ( ((IndexPath*)subsubpath)->indexinfo->relam != BTREE_AM_OID )
						return NIL;
					allquals = list_concat(allquals, get_index_quals( (IndexPath*)subsubpath, *counter ));
				} else
					return NIL;
			}
		} else if ( IsA(subpath, IndexPath) ) {
			if ( ((IndexPath*)subpath)->indexinfo->relam != BTREE_AM_OID )
				return NIL;
			allquals = list_concat(allquals, get_index_quals( (IndexPath*)subpath, *counter ));
		} else
			return NIL;

		(*counter)++;
	}

	return allquals;
}

/*
 * Compares aruments of operation
 */
static bool
iseqCKeyArgs( CKey	*a, CKey *b ) {
	if ( a->opfamily != b->opfamily )
		return false;

	if ( !equal( a->value, b->value ) )
		return false;

	if ( !equal( a->var, b->var ) )
		return false;

	return true;
}

/*
 * Count entries of CKey with the same arguments
 */
static int
count_entry( List *allquals, CKey *tocmp ) {
	ListCell	*i;
	int 		curcnt=0;

	foreach(i, allquals) {
		CKey    *key = lfirst(i);

		if ( key->n == curcnt ) {
			continue;
		} else if ( key->n == curcnt+1 ) {
			if ( iseqCKeyArgs( key, tocmp ) ) {
				tocmp->strategyMask |= key->strategyMask;
				curcnt++;
			}
		} else
			return -1;
	}

	return curcnt+1;
}

/*
 * Finds all CKey with the same arguments
 */
static List*
find_common_quals( BitmapOrPath *path ) {
	List *allquals;
	List *commonquals = NIL;
	ListCell	*i;
	int counter;

	if ( (allquals = find_all_quals( path, &counter ))==NIL )
		return NIL;

	foreach(i, allquals) {
		CKey	*key = lfirst(i);

		if ( key->n != 0 )
			break;

		if ( counter == count_entry(allquals, key) ) 
			commonquals = lappend( commonquals, key );
	}

	return commonquals;
}

/*
 * unionOperation - make RestrictInfo with combined operation
 */

static RestrictInfo*
unionOperation(CKey	*key) {
	RestrictInfo	*rinfo;
	Oid		lefttype, righttype;
	int		strategy;

	switch( key->strategyMask ) {
		case	BTMASK(BTLessStrategyNumber):	
		case	BTMASK(BTLessEqualStrategyNumber):	
		case	BTMASK(BTEqualStrategyNumber):	
		case	BTMASK(BTGreaterEqualStrategyNumber):
		case	BTMASK(BTGreaterStrategyNumber):
				/* trivial case */
				break;
		case	BTMASK(BTLessStrategyNumber) | BTMASK(BTLessEqualStrategyNumber):
		case	BTMASK(BTLessStrategyNumber) | BTMASK(BTLessEqualStrategyNumber) | BTMASK(BTEqualStrategyNumber):
		case	BTMASK(BTLessStrategyNumber) | BTMASK(BTEqualStrategyNumber):
		case	BTMASK(BTLessEqualStrategyNumber) | BTMASK(BTEqualStrategyNumber):
				/* any subset of <, <=, = can be unioned with <= */ 
				key->strategy = BTLessEqualStrategyNumber;
				break;
		case	BTMASK(BTGreaterEqualStrategyNumber) | BTMASK(BTGreaterStrategyNumber):
		case	BTMASK(BTEqualStrategyNumber) | BTMASK(BTGreaterEqualStrategyNumber) | BTMASK(BTGreaterStrategyNumber):
		case	BTMASK(BTEqualStrategyNumber) | BTMASK(BTGreaterStrategyNumber):
		case	BTMASK(BTEqualStrategyNumber) | BTMASK(BTGreaterEqualStrategyNumber):
				/* any subset of >, >=, = can be unioned with >= */ 
				key->strategy = BTGreaterEqualStrategyNumber;
				break;
		default:
			/*
			 * Can't make common restrict qual
			 */
			return NULL;
	}

	get_op_opfamily_properties(key->normalizedexpr->opno, key->opfamily, false,
							  &strategy, &lefttype, &righttype);

	if ( strategy != key->strategy ) {
		/*
		 * We should check because it's possible to have "strange"
		 * opfamilies - without some strategies...
		 */
		key->normalizedexpr->opno = get_opfamily_member(key->opfamily, lefttype, righttype, key->strategy);

		if ( key->normalizedexpr->opno == InvalidOid )
			return NULL;

		key->normalizedexpr->opfuncid = get_opcode( key->normalizedexpr->opno );
		Assert ( key->normalizedexpr->opfuncid != InvalidOid );
	}

	rinfo =	make_simple_restrictinfo((Expr*)key->normalizedexpr);

	return rinfo;
}

/*
 * Remove unneeded RestrioctionInfo nodes as it
 * needed by predicate_*_by()
 */
static void 
make_predicate(List *indexquals, List *indexqualcols, List **preds, List **predcols) {
	ListCell	*i, *c;

	*preds = NIL;
	*predcols = NIL;

	forboth(i, indexquals, c, indexqualcols) 
	{
		RestrictInfo *rinfo = lfirst(i);
		OpExpr  *expr = (OpExpr*)rinfo->clause;

		if ( rinfo->outerjoin_delayed )
			continue;

		if ( !IsA(expr, OpExpr) )
			continue;

		if ( list_length( expr->args ) != 2 )
			continue;

		*preds = lappend(*preds, rinfo);
		*predcols = lappend(*predcols, lfirst(c));
	}
}

#define CELL_GET_QUALS(x)	( ((IndexPath*)lfirst(x))->indexquals )
#define CELL_GET_CLAUSES(x)	( ((IndexPath*)lfirst(x))->indexclauses )

static List*
listRInfo2OpExpr(List *listRInfo) {
    ListCell    *i;
	List        *listOpExpr=NULL;

	foreach(i, listRInfo)
	{
		RestrictInfo *rinfo = lfirst(i);
		OpExpr  *expr = (OpExpr*)rinfo->clause;

		listOpExpr = lappend(listOpExpr, expr);
	}

	return listOpExpr;
}

/*
 * returns list of all nested quals
 */
static List*
contained_quals(List *nested, List* quals, ListCell *check) {
	ListCell	*i;
	List		*checkpred;

	if ( list_member_ptr( nested, lfirst(check) ) )
		return nested;

	if (equal(CELL_GET_QUALS(check), CELL_GET_CLAUSES(check)) == false)
		return nested;

	checkpred = listRInfo2OpExpr(CELL_GET_QUALS(check));

	if ( contain_mutable_functions((Node*)checkpred) )
		return nested;

	foreach(i, quals )
	{
		if ( check == i )
			continue;

		if ( list_member_ptr( nested, lfirst(i) ) )
			continue;

		if ( equal(CELL_GET_QUALS(i), CELL_GET_CLAUSES(i)) &&
				predicate_implied_by( checkpred, CELL_GET_QUALS(i) ) ) 
			nested = lappend( nested, lfirst(i) );
	}
	return nested;
}

/*
 * Checks that one row can be in several quals.
 * It's guaranteed by predicate_refuted_by()
 */
static bool
is_intersect(ListCell *check) {
	ListCell	*i;
	List		*checkpred=NULL;

	checkpred=listRInfo2OpExpr(CELL_GET_QUALS(check));
	Assert( checkpred != NULL );

	for_each_cell(i, check) {
		if ( i==check )
			continue;

		if ( predicate_refuted_by( checkpred, CELL_GET_QUALS(i) ) == false )
			return true;
	}

	return false;
}

/*
 * Removes nested quals and gurantees that quals are not intersected,
 * ie one row can't satisfy to several quals. It's open a possibility of
 * Append node using instead of BitmapOr
 */
static	BitmapOrPath* 
cleanup_nested_quals( PlannerInfo *root, RelOptInfo *rel, BitmapOrPath *path ) {
	ListCell   *i;
	IndexOptInfo	*index=NULL;
	List		*nested = NULL;

	/*
	 * check all path to use only one index
	 */
	foreach(i, path->bitmapquals )
	{

		if ( IsA(lfirst(i), IndexPath) ) {
			List *preds, *predcols;
			IndexPath *subpath = (IndexPath *) lfirst(i);

			if ( subpath->indexinfo->relam != BTREE_AM_OID )
				return NULL;

			if ( index == NULL )
				index = subpath->indexinfo;
			else if ( index->indexoid != subpath->indexinfo->indexoid )
				return NULL;

			/*
			 * work only with optimizable quals
			 */
			Assert(list_length(subpath->indexquals) == list_length(subpath->indexqualcols));
			make_predicate(subpath->indexquals, subpath->indexqualcols, &preds, &predcols); 
			if (preds == NIL)
				return NULL;
			subpath->indexquals = preds;
			subpath->indexqualcols = predcols;
			Assert(list_length(subpath->indexquals) == list_length(subpath->indexqualcols));
		} else
			return NULL;
	}

	/*
	 * eliminate nested quals
	 */
	foreach(i, path->bitmapquals ) {
		nested = contained_quals(nested, path->bitmapquals, i);
	}

	if ( nested != NIL ) {
		path->bitmapquals = list_difference_ptr( path->bitmapquals, nested );

		Assert( list_length( path->bitmapquals )>0 );

		/*
		 * All quals becomes only one after eliminating nested quals 
		 */
		if (list_length( path->bitmapquals ) == 1) 
			return (BitmapOrPath*)linitial(path->bitmapquals);
	}

	/*
	 * Checks for intersection 
	 */
	foreach(i, path->bitmapquals ) {
		if ( is_intersect( i ) )
			return NULL;
	}

	return path;
}

/*
 * Checks if whole result of one simple operation is contained
 * in another
 */
static int
simpleCmpExpr( ExExpr *a, ExExpr *b ) {
	if ( predicate_implied_by((List*)a->expr, (List*)b->expr) ) 
		/*  
		 * a:( Var < 15 ) > b:( Var <= 10 ) 
		 */
		return 1; 
	else if ( predicate_implied_by((List*)b->expr, (List*)a->expr) )
		/*  
		 * a:( Var <= 10 ) < b:( Var < 15 ) 
		 */
		return -1;
	else
		return 0;
}

/*
 * Trys to define where is equation - on left or right side
 *		a(< 10)	 b(=11)   - on right
 *		a(> 10)  b(=9)    - on left
 *		a(= 10)	 b(=11)   - on right
 *		a(= 10)  b(=9)    - on left
 * Any other - result is 0;
 */
static int
cmpEqExpr( ExExpr *a, ExExpr *b ) {
	Oid oldop = b->expr->opno;
	int res=0;

	b->expr->opno = get_opfamily_member(b->opfamily, b->lefttype, b->righttype, BTLessStrategyNumber);
	if ( b->expr->opno != InvalidOid ) {
		 b->expr->opfuncid = get_opcode( b->expr->opno );
		res = simpleCmpExpr(a,b);
	}

	if ( res == 0 ) {
		b->expr->opno = get_opfamily_member(b->opfamily, b->lefttype, b->righttype, BTGreaterStrategyNumber);
		if ( b->expr->opno != InvalidOid ) {
		 	b->expr->opfuncid = get_opcode( b->expr->opno );
			res = -simpleCmpExpr(a,b);
		}
	}

	b->expr->opno = oldop;
	b->expr->opfuncid = get_opcode( b->expr->opno );

	return res;
}

/*
 * Is result of a contained in result of b or on the contrary?
 */
static int
cmpNegCmp( ExExpr *a, ExExpr *b ) {
	Oid oldop = b->expr->opno;
	int	res = 0;

	b->expr->opno = get_negator( b->expr->opno );
	if ( b->expr->opno != InvalidOid ) { 
		b->expr->opfuncid = get_opcode( b->expr->opno );
		res = simpleCmpExpr(a,b);
	}

	b->expr->opno = oldop;
	b->expr->opfuncid = get_opcode( b->expr->opno );

	return ( IS_LESS(a->strategy) ) ? res : -res;
}

/*
 * Returns 1 if whole result of a is on left comparing with result of b
 * Returns -1 if whole result of a is on right comparing with result of b
 * Return 0 if it's impossible to define or results is overlapped
 * Expressions should use the same attribute of index and should be
 * a simple: just one operation with index.
 */
static int
cmpExpr( ExExpr *a, ExExpr *b ) {
	int res;

	/* 
	 * If a and b are overlapped, we can't decide which one is
	 * lefter or righter
	 */
	if ( IS_ONE_DIRECTION(a->strategy, b->strategy) || predicate_refuted_by((List*)a->expr, (List*)b->expr) == false )
		return 0; 

	/*
	 * In this place it's impossible to have a row which satisfies
	 * a and b expressions, so we will try to find relatiove position of that results
	 */
	if ( b->strategy == BTEqualStrategyNumber ) { 
		return -cmpEqExpr(a, b); /* Covers cases with any operations in a */
	} else if ( a->strategy == BTEqualStrategyNumber ) {
		return cmpEqExpr(b, a);
	} else if ( (res = cmpNegCmp(a, b)) == 0 ) { /* so, a(<10) b(>20) */ 
		res = -cmpNegCmp(b, a);
	}
		
	return res;
}

/*
 * Try to define positions of result which satisfy indexquals a and b per
 * one index's attribute.
 */
static int
cmpColumnQuals( List *a, List *b, int attno ) {
	int res = 0;
	ListCell *ai, *bi;

	foreach(ai, a) {
		ExExpr	*ae = (ExExpr*)lfirst(ai);

		if ( attno != ae->attno )
			continue;

		foreach(bi, b) {
			ExExpr	*be = (ExExpr*)lfirst(bi);
	
			if ( attno != be->attno )
				continue;

			if ((res=cmpExpr(ae, be))!=0)
				return res;
		}
	}

	return 0;
}

static IndexOptInfo	*sortingIndex = NULL;
static bool volatile	unableToDefine = false;

/*
 * Compare result of two indexquals. 
 * Warinig: it use PG_RE_THROW(), so any call should be wrapped with
 * PG_TRY().  Try/catch construction is used here for minimize unneeded
 * actions when sorting is impossible
 */
static int
cmpIndexPathEx(const void *a, const void *b) {
	IndexPathEx	*aipe = (IndexPathEx*)a;
	IndexPathEx	*bipe = (IndexPathEx*)b;
	int attno, res = 0;

	for(attno=1; res==0 && attno<=sortingIndex->ncolumns; attno++) 
		res=cmpColumnQuals(aipe->preparedquals, bipe->preparedquals, attno);

	if ( res==0 ) {
		unableToDefine = true;
		PG_RE_THROW(); /* it should be PG_THROW(), but it's the same */
	}

	return res;
}

/*
 * Initialize lists of operation in useful form
 */
static List*
prepareQuals(IndexOptInfo *index, List *indexquals, List *indexqualcols) {
	ListCell	*i, *c;
	List		*res=NULL;
	ExExpr		*ex;

	Assert(list_length(indexquals) == list_length(indexqualcols));
	forboth(i, indexquals, c, indexqualcols)
	{
		RestrictInfo *rinfo = lfirst(i);
		OpExpr  *expr = (OpExpr*)rinfo->clause;

		if ( rinfo->outerjoin_delayed )
			return NULL;

		if ( !IsA(expr, OpExpr) )
			return NULL;

		if ( list_length( expr->args ) != 2 )
			return NULL;

		if ( contain_mutable_functions((Node*)expr) )
			return NULL;

		ex = (ExExpr*)palloc(sizeof(ExExpr));
		ex->expr = (OpExpr*)copyObject( expr ); 
		if (!bms_equal(rinfo->left_relids, index->rel->relids))
			CommuteOpExpr(ex->expr);
		linitial(ex->expr->args) = fix_indexqual_operand(linitial(ex->expr->args), index, lfirst_int(c));
		ex->attno = ((Var*)linitial(ex->expr->args))->varattno;
		ex->opfamily = index->opfamily[ ex->attno - 1 ];
		get_op_opfamily_properties( ex->expr->opno, ex->opfamily, false, 
			&ex->strategy, &ex->lefttype, &ex->righttype);


		res = lappend(res, ex);
	}

	return res;
}

/*
 * sortIndexScans - sorts index scans to get sorted results.
 * Function supposed that index is the same for all
 * index scans
 */
static List*
sortIndexScans( List* ipaths ) {
	ListCell	*i;
	int			j=0;
	IndexPathEx	*ipe = (IndexPathEx*)palloc( sizeof(IndexPathEx)*list_length(ipaths) );
	List		*orderedPaths = NIL;
	IndexOptInfo *index = ((IndexPath*)linitial(ipaths))->indexinfo;

	foreach(i, ipaths) {
		ipe[j].path = (IndexPath*)lfirst(i);
		ipe[j].preparedquals = prepareQuals( index, ipe[j].path->indexquals, ipe[j].path->indexqualcols );

		if (ipe[j].preparedquals == NULL)
			return NULL;
		j++;
	}

	sortingIndex = index;
	unableToDefine = false;
	PG_TRY(); {
		qsort(ipe, list_length(ipaths), sizeof(IndexPathEx), cmpIndexPathEx);
	} PG_CATCH(); {
		if ( unableToDefine == false ) 
			PG_RE_THROW(); /* not our problem */
	} PG_END_TRY();

	if ( unableToDefine == true )
		return NULL;

	for(j=0;j<list_length(ipaths);j++)
		orderedPaths = lappend(orderedPaths, ipe[j].path);	

	return  orderedPaths;	
}

static  IndexPath*
reverseScanDirIdxPath(IndexPath *ipath) {
	IndexPath   *n = makeNode(IndexPath);

	*n = *ipath;

	n->indexscandir = BackwardScanDirection;

	return n;
}

static List*
reverseScanDirIdxPaths(List *indexPaths) {
	List		*idxpath = NIL;
	ListCell	*i;

	foreach(i, indexPaths) {
		idxpath = lcons(reverseScanDirIdxPath( (IndexPath*)lfirst(i) ), idxpath);
	}

	return idxpath;
}
