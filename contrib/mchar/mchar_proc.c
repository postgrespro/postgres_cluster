#include "mchar.h"
#include "mb/pg_wchar.h"
#include "access/hash.h"

PG_FUNCTION_INFO_V1(mchar_length);
Datum       mchar_length(PG_FUNCTION_ARGS);

Datum
mchar_length(PG_FUNCTION_ARGS) {
	MChar	*m = PG_GETARG_MCHAR(0);
	int32	l = UCHARLENGTH(m);

	while( l>0 && m_isspace( m->data[ l-1 ] ) )
		l--;

	l = u_countChar32(m->data, l);

	PG_FREE_IF_COPY(m,0);

	PG_RETURN_INT32(l);
}

PG_FUNCTION_INFO_V1(mvarchar_length);
Datum       mvarchar_length(PG_FUNCTION_ARGS);

Datum
mvarchar_length(PG_FUNCTION_ARGS) {
	MVarChar	*m = PG_GETARG_MVARCHAR(0);
	int32	l = UVARCHARLENGTH(m);

	while( l>0 && m_isspace( m->data[ l-1 ] ) )
		l--;

	l = u_countChar32(m->data, l);

	PG_FREE_IF_COPY(m,0);

	PG_RETURN_INT32(l);
}

static int32
uchar_substring( 
		UChar *str, int32 strl,
		int32 start, int32 length, bool length_not_specified,
		UChar *dst) {
	int32	S = start-1;	/* start position */
	int32	S1;			/* adjusted start position */
	int32	L1;			/* adjusted substring length */
	int32	subbegin=0, subend=0;

	S1 = Max(S, 0);
	if (length_not_specified)
		L1 = -1;
	else {
		/* end position */
		int32 E = S + length;

		/*
		 * A negative value for L is the only way for the end position to
		 * be before the start. SQL99 says to throw an error.
		 */

		if (E < S)
			ereport(ERROR,
					(errcode(ERRCODE_SUBSTRING_ERROR),
					 errmsg("negative substring length not allowed")));

		/*
		 * A zero or negative value for the end position can happen if the
		 * start was negative or one. SQL99 says to return a zero-length
		 * string.
		 */
		if (E < 0) 
			return 0;

		L1 = E - S1;
	}
		 
	U16_FWD_N( str, subbegin, strl, S1 );
	if ( subbegin >= strl ) 
		return 0;
	subend = subbegin;
	U16_FWD_N( str, subend, strl, L1 );

	memcpy( dst, str+subbegin, sizeof(UChar)*(subend-subbegin) );

	return subend-subbegin;
}

PG_FUNCTION_INFO_V1(mchar_substring);
Datum       mchar_substring(PG_FUNCTION_ARGS);
Datum
mchar_substring(PG_FUNCTION_ARGS) {
	MChar	*src = PG_GETARG_MCHAR(0);
	MChar	*dst;
	int32	length;

	dst = (MChar*)palloc( VARSIZE(src) );
	length = uchar_substring( 
		src->data, UCHARLENGTH(src),
		PG_GETARG_INT32(1), PG_GETARG_INT32(2), false,
		dst->data);

	dst->typmod = src->typmod;
	SET_VARSIZE(dst, MCHARHDRSZ + length *sizeof(UChar));
	
	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MCHAR(dst);
}

PG_FUNCTION_INFO_V1(mchar_substring_no_len);
Datum       mchar_substring_no_len(PG_FUNCTION_ARGS);
Datum
mchar_substring_no_len(PG_FUNCTION_ARGS) {
	MChar	*src = PG_GETARG_MCHAR(0);
	MChar	*dst;
	int32	length;

	dst = (MChar*)palloc( VARSIZE(src) );
	length = uchar_substring( 
		src->data, UCHARLENGTH(src),
		PG_GETARG_INT32(1), -1, true,
		dst->data);

	dst->typmod = src->typmod;
	SET_VARSIZE(dst, MCHARHDRSZ + length *sizeof(UChar));

	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MCHAR(dst);
}

PG_FUNCTION_INFO_V1(mvarchar_substring);
Datum       mvarchar_substring(PG_FUNCTION_ARGS);
Datum
mvarchar_substring(PG_FUNCTION_ARGS) {
	MVarChar	*src = PG_GETARG_MVARCHAR(0);
	MVarChar	*dst;
	int32	length;

	dst = (MVarChar*)palloc( VARSIZE(src) );
	length = uchar_substring( 
		src->data, UVARCHARLENGTH(src),
		PG_GETARG_INT32(1), PG_GETARG_INT32(2), false,
		dst->data);

	SET_VARSIZE(dst, MVARCHARHDRSZ + length *sizeof(UChar));

	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MVARCHAR(dst);
}

PG_FUNCTION_INFO_V1(mvarchar_substring_no_len);
Datum       mvarchar_substring_no_len(PG_FUNCTION_ARGS);
Datum
mvarchar_substring_no_len(PG_FUNCTION_ARGS) {
	MVarChar	*src = PG_GETARG_MVARCHAR(0);
	MVarChar	*dst;
	int32	length;

	dst = (MVarChar*)palloc( VARSIZE(src) );
	length = uchar_substring( 
		src->data, UVARCHARLENGTH(src),
		PG_GETARG_INT32(1), -1, true,
		dst->data);

	SET_VARSIZE(dst, MVARCHARHDRSZ + length *sizeof(UChar));

	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MVARCHAR(dst);
}

static Datum
hash_uchar( UChar *s, int len ) {
	int32	length;
	UErrorCode err = 0;
	UChar	*d;
	Datum res;

	if ( len == 0 )
		return hash_any( NULL, 0 );

	err = 0;
	d = (UChar*) palloc( sizeof(UChar) * len * 2 );
	length = u_strFoldCase(d, len*2, s, len, U_FOLD_CASE_DEFAULT, &err);

	if ( U_FAILURE(err) )
		elog(ERROR,"ICU u_strFoldCase fails and returns %d (%s)", err,  u_errorName(err));

	res = hash_any( (unsigned char*) d,  length * sizeof(UChar) );

	pfree(d);
	return res;
}

PG_FUNCTION_INFO_V1(mvarchar_hash);
Datum
mvarchar_hash(PG_FUNCTION_ARGS) {
	MVarChar	*src = PG_GETARG_MVARCHAR(0);
	Datum		res;

	res = hash_uchar( src->data, lengthWithoutSpaceVarChar(src) );

	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_DATUM( res );
}

PG_FUNCTION_INFO_V1(mchar_hash);
Datum
mchar_hash(PG_FUNCTION_ARGS) {
	MChar	*src = PG_GETARG_MCHAR(0);
	Datum	res;

	res = hash_uchar( src->data, lengthWithoutSpaceChar(src) );

	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_DATUM( res );
}

PG_FUNCTION_INFO_V1(mchar_upper);
Datum       mchar_upper(PG_FUNCTION_ARGS);
Datum
mchar_upper(PG_FUNCTION_ARGS) {
	MChar	*src = PG_GETARG_MCHAR(0);
	MChar	*dst = (MChar*)palloc( VARSIZE(src) * 2 );

	dst->len = MCHARHDRSZ;
	dst->typmod = src->typmod;
	if ( UCHARLENGTH(src) != 0 ) {
		int 		length;
		UErrorCode	err=0;

		length = u_strToUpper( dst->data, VARSIZE(src) * 2 - MCHARHDRSZ,
								src->data, UCHARLENGTH(src),
								NULL, &err );

		Assert( length <= VARSIZE(src) * 2 - MCHARHDRSZ );

		if ( U_FAILURE(err) )
			elog(ERROR,"ICU u_strToUpper fails and returns %d (%s)", err,  u_errorName(err));

		dst->len += sizeof(UChar) * length;	
	}

	SET_VARSIZE( dst, dst->len );
	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MCHAR( dst );
}

PG_FUNCTION_INFO_V1(mchar_lower);
Datum       mchar_lower(PG_FUNCTION_ARGS);
Datum
mchar_lower(PG_FUNCTION_ARGS) {
	MChar	*src = PG_GETARG_MCHAR(0);
	MChar	*dst = (MChar*)palloc( VARSIZE(src) * 2 );

	dst->len = MCHARHDRSZ;
	dst->typmod = src->typmod;
	if ( UCHARLENGTH(src) != 0 ) {
		int 		length;
		UErrorCode	err=0;

		length = u_strToLower( dst->data, VARSIZE(src) * 2 - MCHARHDRSZ,
								src->data, UCHARLENGTH(src),
								NULL, &err );

		Assert( length <= VARSIZE(src) * 2 - MCHARHDRSZ );

		if ( U_FAILURE(err) )
			elog(ERROR,"ICU u_strToLower fails and returns %d (%s)", err,  u_errorName(err));

		dst->len += sizeof(UChar) * length;	
	}

	SET_VARSIZE( dst, dst->len );
	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MCHAR( dst );
}

PG_FUNCTION_INFO_V1(mvarchar_upper);
Datum       mvarchar_upper(PG_FUNCTION_ARGS);
Datum
mvarchar_upper(PG_FUNCTION_ARGS) {
	MVarChar	*src = PG_GETARG_MVARCHAR(0);
	MVarChar	*dst = (MVarChar*)palloc( VARSIZE(src) * 2 );

	dst->len = MVARCHARHDRSZ;

	if ( UVARCHARLENGTH(src) != 0 ) {
		int 		length;
		UErrorCode	err=0;

		length = u_strToUpper( dst->data, VARSIZE(src) * 2 - MVARCHARHDRSZ,
								src->data, UVARCHARLENGTH(src),
								NULL, &err );

		Assert( length <= VARSIZE(src) * 2 - MVARCHARHDRSZ );

		if ( U_FAILURE(err) )
			elog(ERROR,"ICU u_strToUpper fails and returns %d (%s)", err,  u_errorName(err));

		dst->len += sizeof(UChar) * length;	
	}

	SET_VARSIZE( dst, dst->len );
	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MVARCHAR( dst );
}

PG_FUNCTION_INFO_V1(mvarchar_lower);
Datum       mvarchar_lower(PG_FUNCTION_ARGS);
Datum
mvarchar_lower(PG_FUNCTION_ARGS) {
	MVarChar	*src = PG_GETARG_MVARCHAR(0);
	MVarChar	*dst = (MVarChar*)palloc( VARSIZE(src) * 2 );

	dst->len = MVARCHARHDRSZ;

	if ( UVARCHARLENGTH(src) != 0 ) {
		int 		length;
		UErrorCode	err=0;

		length = u_strToLower( dst->data, VARSIZE(src) * 2 - MVARCHARHDRSZ,
								src->data, UVARCHARLENGTH(src),
								NULL, &err );

		Assert( length <= VARSIZE(src) * 2 - MVARCHARHDRSZ );

		if ( U_FAILURE(err) )
			elog(ERROR,"ICU u_strToLower fails and returns %d (%s)", err,  u_errorName(err));

		dst->len += sizeof(UChar) * length;	
	}

	SET_VARSIZE( dst, dst->len );
	PG_FREE_IF_COPY(src, 0);
	PG_RETURN_MVARCHAR( dst );
}


