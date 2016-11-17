#include "mchar.h"

int
lengthWithoutSpaceVarChar(MVarChar	*m) {
	int l = UVARCHARLENGTH(m);

	while( l>0 && m_isspace( m->data[ l-1 ] ) ) 
		l--;

	return l;
}

int
lengthWithoutSpaceChar(MChar	*m) {
	int l = UCHARLENGTH(m);

	while( l>0 && m_isspace( m->data[ l-1 ] ) ) 
		l--;

	return l;
}

static inline int
mchar_icase_compare( MChar *a, MChar *b ) {
	return UCharCaseCompare( 
		a->data, lengthWithoutSpaceChar(a),
		b->data, lengthWithoutSpaceChar(b)
	);
}

static inline int
mchar_case_compare( MChar *a, MChar *b ) {
	return UCharCompare( 
		a->data, lengthWithoutSpaceChar(a),
		b->data, lengthWithoutSpaceChar(b)
	);
}

#define MCHARCMPFUNC( c, type, action, ret ) 		\
PG_FUNCTION_INFO_V1( mchar_##c##_##type ); 		\
Datum      mchar_##c##_##type(PG_FUNCTION_ARGS);\
Datum											\
mchar_##c##_##type(PG_FUNCTION_ARGS) {			\
	MChar *a = PG_GETARG_MCHAR(0);				\
	MChar *b = PG_GETARG_MCHAR(1);				\
	int 	res = mchar_##c##_compare(a,b);		\
												\
	PG_FREE_IF_COPY(a,0);						\
	PG_FREE_IF_COPY(b,1);						\
	PG_RETURN_##ret( res action 0 );			\
}


MCHARCMPFUNC( case, eq, ==, BOOL )
MCHARCMPFUNC( case, ne, !=, BOOL )
MCHARCMPFUNC( case, lt, <, BOOL )
MCHARCMPFUNC( case, le, <=, BOOL )
MCHARCMPFUNC( case, ge, >=, BOOL )
MCHARCMPFUNC( case, gt, >, BOOL )
MCHARCMPFUNC( case, cmp, +, INT32 )

MCHARCMPFUNC( icase, eq, ==, BOOL )
MCHARCMPFUNC( icase, ne, !=, BOOL )
MCHARCMPFUNC( icase, lt, <, BOOL )
MCHARCMPFUNC( icase, le, <=, BOOL )
MCHARCMPFUNC( icase, ge, >=, BOOL )
MCHARCMPFUNC( icase, gt, >, BOOL )
MCHARCMPFUNC( icase, cmp, +, INT32 )

PG_FUNCTION_INFO_V1( mchar_larger );
Datum mchar_larger( PG_FUNCTION_ARGS );
Datum
mchar_larger( PG_FUNCTION_ARGS ) {
	MChar *a = PG_GETARG_MCHAR(0);
	MChar *b = PG_GETARG_MCHAR(1);
	MChar *r;

	r = ( mchar_icase_compare(a,b) > 0 ) ? a : b;

	PG_RETURN_MCHAR(r);
}

PG_FUNCTION_INFO_V1( mchar_smaller );
Datum mchar_smaller( PG_FUNCTION_ARGS );
Datum
mchar_smaller( PG_FUNCTION_ARGS ) {
	MChar *a = PG_GETARG_MCHAR(0);
	MChar *b = PG_GETARG_MCHAR(1);
	MChar *r;

	r = ( mchar_icase_compare(a,b) < 0 ) ? a : b;

	PG_RETURN_MCHAR(r);
}


PG_FUNCTION_INFO_V1( mchar_concat );
Datum mchar_concat( PG_FUNCTION_ARGS );
Datum
mchar_concat( PG_FUNCTION_ARGS ) {
	MChar *a = PG_GETARG_MCHAR(0);
	MChar *b = PG_GETARG_MCHAR(1);
	MChar *result;
	int	maxcharlen, curlen;
	int	acharlen = u_countChar32(a->data, UCHARLENGTH(a)),
		bcharlen = u_countChar32(b->data, UCHARLENGTH(b));


	maxcharlen = ((a->typmod<=0) ? acharlen : a->typmod) + 
				 ((b->typmod<=0) ? bcharlen : b->typmod);

	result = (MChar*)palloc( MCHARHDRSZ + sizeof(UChar) * 2 * maxcharlen );

	curlen = UCHARLENGTH( a );
	if ( curlen > 0 )
		memcpy( result->data, a->data, MCHARLENGTH(a) );
	if ( a->typmod > 0 && acharlen < a->typmod  ) {
		FillWhiteSpace( result->data + curlen, a->typmod-acharlen );
		curlen += a->typmod-acharlen;
	}

	if ( UCHARLENGTH(b) > 0 ) {
		memcpy( result->data + curlen, b->data, MCHARLENGTH( b ) );
		curlen += UCHARLENGTH( b );
	}
	if ( b->typmod > 0 && bcharlen < b->typmod  ) {
		FillWhiteSpace( result->data + curlen, b->typmod-bcharlen );
		curlen += b->typmod-bcharlen;
	}


	result->typmod = -1;
	SET_VARSIZE(result, sizeof(UChar) * curlen + MCHARHDRSZ);

	PG_FREE_IF_COPY(a,0);
	PG_FREE_IF_COPY(b,1);

	PG_RETURN_MCHAR(result);
}

static inline int
mvarchar_icase_compare( MVarChar *a, MVarChar *b ) {
	
	return UCharCaseCompare( 
		a->data, lengthWithoutSpaceVarChar(a),
		b->data, lengthWithoutSpaceVarChar(b)
	);
}

static inline int
mvarchar_case_compare( MVarChar *a, MVarChar *b ) {
	return UCharCompare( 
		a->data, lengthWithoutSpaceVarChar(a),
		b->data, lengthWithoutSpaceVarChar(b)
	);
}

#define MVARCHARCMPFUNC( c, type, action, ret ) 	\
PG_FUNCTION_INFO_V1( mvarchar_##c##_##type ); 		\
Datum      mvarchar_##c##_##type(PG_FUNCTION_ARGS);	\
Datum												\
mvarchar_##c##_##type(PG_FUNCTION_ARGS) {			\
	MVarChar *a = PG_GETARG_MVARCHAR(0);			\
	MVarChar *b = PG_GETARG_MVARCHAR(1);			\
	int 	res = mvarchar_##c##_compare(a,b);		\
													\
	PG_FREE_IF_COPY(a,0);							\
	PG_FREE_IF_COPY(b,1);							\
	PG_RETURN_##ret( res action	0 );				\
}


MVARCHARCMPFUNC( case, eq, ==, BOOL )
MVARCHARCMPFUNC( case, ne, !=, BOOL )
MVARCHARCMPFUNC( case, lt, <, BOOL )
MVARCHARCMPFUNC( case, le, <=, BOOL )
MVARCHARCMPFUNC( case, ge, >=, BOOL )
MVARCHARCMPFUNC( case, gt, >, BOOL )
MVARCHARCMPFUNC( case, cmp, +, INT32 )

MVARCHARCMPFUNC( icase, eq, ==, BOOL )
MVARCHARCMPFUNC( icase, ne, !=, BOOL )
MVARCHARCMPFUNC( icase, lt, <, BOOL )
MVARCHARCMPFUNC( icase, le, <=, BOOL )
MVARCHARCMPFUNC( icase, ge, >=, BOOL )
MVARCHARCMPFUNC( icase, gt, >, BOOL )
MVARCHARCMPFUNC( icase, cmp, +, INT32 )

PG_FUNCTION_INFO_V1( mvarchar_larger );
Datum mvarchar_larger( PG_FUNCTION_ARGS );
Datum
mvarchar_larger( PG_FUNCTION_ARGS ) {
	MVarChar *a = PG_GETARG_MVARCHAR(0);
	MVarChar *b = PG_GETARG_MVARCHAR(1);
	MVarChar *r;

	r = ( mvarchar_icase_compare(a,b) > 0 ) ? a : b;

	PG_RETURN_MVARCHAR(r);
}

PG_FUNCTION_INFO_V1( mvarchar_smaller );
Datum mvarchar_smaller( PG_FUNCTION_ARGS );
Datum
mvarchar_smaller( PG_FUNCTION_ARGS ) {
	MVarChar *a = PG_GETARG_MVARCHAR(0);
	MVarChar *b = PG_GETARG_MVARCHAR(1);
	MVarChar *r;

	r = ( mvarchar_icase_compare(a,b) < 0 ) ? a : b;

	PG_RETURN_MVARCHAR(r);
}

PG_FUNCTION_INFO_V1( mvarchar_concat );
Datum mvarchar_concat( PG_FUNCTION_ARGS );
Datum
mvarchar_concat( PG_FUNCTION_ARGS ) {
	MVarChar *a = PG_GETARG_MVARCHAR(0);
	MVarChar *b = PG_GETARG_MVARCHAR(1);
	MVarChar *result;
	int	curlen;
	int	acharlen = u_countChar32(a->data, UVARCHARLENGTH(a)),
		bcharlen = u_countChar32(b->data, UVARCHARLENGTH(b));

	result = (MVarChar*)palloc( MVARCHARHDRSZ + sizeof(UChar) * 2 * (acharlen + bcharlen) );

	curlen = UVARCHARLENGTH( a );
	if ( curlen > 0 )
		memcpy( result->data, a->data, MVARCHARLENGTH(a) );

	if ( UVARCHARLENGTH(b) > 0 ) {
		memcpy( result->data + curlen, b->data, MVARCHARLENGTH( b ) );
		curlen += UVARCHARLENGTH( b );
	}

	SET_VARSIZE(result, sizeof(UChar) * curlen + MVARCHARHDRSZ);

	PG_FREE_IF_COPY(a,0);
	PG_FREE_IF_COPY(b,1);

	PG_RETURN_MVARCHAR(result);
}

PG_FUNCTION_INFO_V1( mchar_mvarchar_concat );
Datum mchar_mvarchar_concat( PG_FUNCTION_ARGS );
Datum                                            
mchar_mvarchar_concat( PG_FUNCTION_ARGS ) {
	MChar *a = PG_GETARG_MCHAR(0);
	MVarChar *b = PG_GETARG_MVARCHAR(1);
	MVarChar *result;
	int	curlen, maxcharlen;
	int	acharlen = u_countChar32(a->data, UCHARLENGTH(a)),
		bcharlen = u_countChar32(b->data, UVARCHARLENGTH(b));

	maxcharlen = ((a->typmod<=0) ? acharlen : a->typmod) + bcharlen;

	result = (MVarChar*)palloc( MVARCHARHDRSZ + sizeof(UChar) * 2 * maxcharlen );

	curlen = UCHARLENGTH( a );
	if ( curlen > 0 )
		memcpy( result->data, a->data, MCHARLENGTH(a) );
	if ( a->typmod > 0 && acharlen < a->typmod  ) {
		FillWhiteSpace( result->data + curlen, a->typmod-acharlen );
		curlen += a->typmod-acharlen;
	}

	if ( UVARCHARLENGTH(b) > 0 ) {
		memcpy( result->data + curlen, b->data, MVARCHARLENGTH( b ) );
		curlen += UVARCHARLENGTH( b );
	}

	SET_VARSIZE(result, sizeof(UChar) * curlen + MVARCHARHDRSZ);

	PG_FREE_IF_COPY(a,0);
	PG_FREE_IF_COPY(b,1);

	PG_RETURN_MVARCHAR(result);
}

PG_FUNCTION_INFO_V1( mvarchar_mchar_concat );
Datum mvarchar_mchar_concat( PG_FUNCTION_ARGS );
Datum                                            
mvarchar_mchar_concat( PG_FUNCTION_ARGS ) {
	MVarChar *a = PG_GETARG_MVARCHAR(0);
	MChar *b = PG_GETARG_MCHAR(1);
	MVarChar *result;
	int	curlen, maxcharlen;
	int	acharlen = u_countChar32(a->data, UVARCHARLENGTH(a)),
		bcharlen = u_countChar32(b->data, UCHARLENGTH(b));

	maxcharlen = acharlen + ((b->typmod<=0) ? bcharlen : b->typmod);

	result = (MVarChar*)palloc( MVARCHARHDRSZ + sizeof(UChar) * 2 * maxcharlen );

	curlen = UVARCHARLENGTH( a );
	if ( curlen > 0 )
		memcpy( result->data, a->data, MVARCHARLENGTH(a) );

	if ( UCHARLENGTH(b) > 0 ) {
		memcpy( result->data + curlen, b->data, MCHARLENGTH( b ) );
		curlen += UCHARLENGTH( b );
	}
	if ( b->typmod > 0 && bcharlen < b->typmod  ) {
		FillWhiteSpace( result->data + curlen, b->typmod-bcharlen );
		curlen += b->typmod-bcharlen;
	}

	SET_VARSIZE(result, sizeof(UChar) * curlen + MVARCHARHDRSZ);

	PG_FREE_IF_COPY(a,0);
	PG_FREE_IF_COPY(b,1);

	PG_RETURN_MVARCHAR(result);
}

/*
 * mchar <> mvarchar 
 */
static inline int
mc_mv_icase_compare( MChar *a, MVarChar *b ) {
	return UCharCaseCompare( 
		a->data, lengthWithoutSpaceChar(a),
		b->data, lengthWithoutSpaceVarChar(b)
	);
}

static inline int
mc_mv_case_compare( MChar *a, MVarChar *b ) {
	return UCharCompare( 
		a->data, lengthWithoutSpaceChar(a),
		b->data, lengthWithoutSpaceVarChar(b)
	);
}

#define MC_MV_CHARCMPFUNC( c, type, action, ret ) 		\
PG_FUNCTION_INFO_V1( mc_mv_##c##_##type ); 		\
Datum      mc_mv_##c##_##type(PG_FUNCTION_ARGS);\
Datum											\
mc_mv_##c##_##type(PG_FUNCTION_ARGS) {			\
	MChar *a = PG_GETARG_MCHAR(0);				\
	MVarChar *b = PG_GETARG_MVARCHAR(1);		\
	int 	res = mc_mv_##c##_compare(a,b);		\
												\
	PG_FREE_IF_COPY(a,0);						\
	PG_FREE_IF_COPY(b,1);						\
	PG_RETURN_##ret( res action 0 );			\
}


MC_MV_CHARCMPFUNC( case, eq, ==, BOOL )
MC_MV_CHARCMPFUNC( case, ne, !=, BOOL )
MC_MV_CHARCMPFUNC( case, lt, <, BOOL )
MC_MV_CHARCMPFUNC( case, le, <=, BOOL )
MC_MV_CHARCMPFUNC( case, ge, >=, BOOL )
MC_MV_CHARCMPFUNC( case, gt, >, BOOL )
MC_MV_CHARCMPFUNC( case, cmp, +, INT32 )

MC_MV_CHARCMPFUNC( icase, eq, ==, BOOL )
MC_MV_CHARCMPFUNC( icase, ne, !=, BOOL )
MC_MV_CHARCMPFUNC( icase, lt, <, BOOL )
MC_MV_CHARCMPFUNC( icase, le, <=, BOOL )
MC_MV_CHARCMPFUNC( icase, ge, >=, BOOL )
MC_MV_CHARCMPFUNC( icase, gt, >, BOOL )
MC_MV_CHARCMPFUNC( icase, cmp, +, INT32 )

/*
 * mvarchar <> mchar 
 */
static inline int
mv_mc_icase_compare( MVarChar *a, MChar *b ) {
	return UCharCaseCompare( 
		a->data, lengthWithoutSpaceVarChar(a),
		b->data, lengthWithoutSpaceChar(b)
	);
}

static inline int
mv_mc_case_compare( MVarChar *a, MChar *b ) {
	return UCharCompare( 
		a->data, lengthWithoutSpaceVarChar(a),
		b->data, lengthWithoutSpaceChar(b)
	);
}

#define MV_MC_CHARCMPFUNC( c, type, action, ret ) 		\
PG_FUNCTION_INFO_V1( mv_mc_##c##_##type ); 		\
Datum      mv_mc_##c##_##type(PG_FUNCTION_ARGS);\
Datum											\
mv_mc_##c##_##type(PG_FUNCTION_ARGS) {			\
	MVarChar *a = PG_GETARG_MVARCHAR(0);		\
	MChar *b = PG_GETARG_MCHAR(1);				\
	int 	res = mv_mc_##c##_compare(a,b);		\
												\
	PG_FREE_IF_COPY(a,0);						\
	PG_FREE_IF_COPY(b,1);						\
	PG_RETURN_##ret( res action 0 );			\
}


MV_MC_CHARCMPFUNC( case, eq, ==, BOOL )
MV_MC_CHARCMPFUNC( case, ne, !=, BOOL )
MV_MC_CHARCMPFUNC( case, lt, <, BOOL )
MV_MC_CHARCMPFUNC( case, le, <=, BOOL )
MV_MC_CHARCMPFUNC( case, ge, >=, BOOL )
MV_MC_CHARCMPFUNC( case, gt, >, BOOL )
MV_MC_CHARCMPFUNC( case, cmp, +, INT32 )

MV_MC_CHARCMPFUNC( icase, eq, ==, BOOL )
MV_MC_CHARCMPFUNC( icase, ne, !=, BOOL )
MV_MC_CHARCMPFUNC( icase, lt, <, BOOL )
MV_MC_CHARCMPFUNC( icase, le, <=, BOOL )
MV_MC_CHARCMPFUNC( icase, ge, >=, BOOL )
MV_MC_CHARCMPFUNC( icase, gt, >, BOOL )
MV_MC_CHARCMPFUNC( icase, cmp, +, INT32 )

#define NULLHASHVALUE       (-2147483647)

#define FULLEQ_FUNC(type, cmpfunc, hashfunc)            \
PG_FUNCTION_INFO_V1( isfulleq_##type );                 \
Datum   isfulleq_##type(PG_FUNCTION_ARGS);              \
Datum                                                   \
isfulleq_##type(PG_FUNCTION_ARGS) {                     \
    if ( PG_ARGISNULL(0) && PG_ARGISNULL(1) )           \
        PG_RETURN_BOOL(true);                           \
    else if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )      \
        PG_RETURN_BOOL(false);                          \
                                                        \
    PG_RETURN_DATUM( DirectFunctionCall2( cmpfunc,      \
            PG_GETARG_DATUM(0),                         \
            PG_GETARG_DATUM(1)                          \
    ) );                                                \
}                                                       \
                                                        \
PG_FUNCTION_INFO_V1( fullhash_##type );                 \
Datum   fullhash_##type(PG_FUNCTION_ARGS);              \
Datum                                                   \
fullhash_##type(PG_FUNCTION_ARGS) {                     \
    if ( PG_ARGISNULL(0) )                              \
        PG_RETURN_INT32(NULLHASHVALUE);                 \
                                                        \
    PG_RETURN_DATUM( DirectFunctionCall1( hashfunc,     \
            PG_GETARG_DATUM(0)                          \
    ) );                                                \
}

FULLEQ_FUNC( mchar, mchar_icase_eq, mchar_hash );
FULLEQ_FUNC( mvarchar, mvarchar_icase_eq, mvarchar_hash );

