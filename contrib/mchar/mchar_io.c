#include "mchar.h"
#include "mb/pg_wchar.h"
#include "fmgr.h"
#include "libpq/pqformat.h"
#include <utils/array.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(mchar_in);
Datum       mchar_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(mchar_out);
Datum       mchar_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(mchar);
Datum       mchar(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(mvarchar_in);
Datum       mvarchar_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(mvarchar_out);
Datum       mvarchar_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(mvarchar);
Datum       mvarchar(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(mchartypmod_in);
Datum mchartypmod_in(PG_FUNCTION_ARGS);
Datum 
mchartypmod_in(PG_FUNCTION_ARGS) {
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32      *tl;
	int         n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	if (n != 1)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid type modifier")));
	if (*tl < 1)
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("length for type mchar/mvarchar must be at least 1")));

	return *tl;
}

PG_FUNCTION_INFO_V1(mchartypmod_out);
Datum mchartypmod_out(PG_FUNCTION_ARGS);
Datum 
mchartypmod_out(PG_FUNCTION_ARGS) {
	int32       typmod = PG_GETARG_INT32(0);
	char       *res = (char *) palloc(64);

	if (typmod >0) 
		snprintf(res, 64, "(%d)", (int) (typmod));
	else
		*res = '\0';

	PG_RETURN_CSTRING( res );
}

static void
mchar_strip( MChar * m, int atttypmod ) {
	int maxlen;
	
	if ( atttypmod<=0 ) {
		atttypmod =-1;
	} else {
		int	charlen = u_countChar32( m->data, UCHARLENGTH(m) );

		if ( charlen > atttypmod ) {
			int i=0;
			U16_FWD_N( m->data, i, UCHARLENGTH(m), atttypmod);
			SET_VARSIZE( m, sizeof(UChar) * i + MCHARHDRSZ );
		}
	}

	m->typmod = atttypmod;

	maxlen = UCHARLENGTH(m);
	while( maxlen>0 && m_isspace( m->data[ maxlen-1 ] ) )
		maxlen--;

	SET_VARSIZE(m, sizeof(UChar) * maxlen + MCHARHDRSZ);
}


Datum
mchar_in(PG_FUNCTION_ARGS) {
	char       *s = PG_GETARG_CSTRING(0);
#ifdef NOT_USED
	Oid         typelem = PG_GETARG_OID(1);
#endif
	int32       atttypmod = PG_GETARG_INT32(2);
	MChar	*result;
	int32	slen = strlen(s), rlen;

	pg_verifymbstr(s, slen, false);

	result = (MChar*)palloc( MCHARHDRSZ + slen * sizeof(UChar) * 4 /* upper limit of length */ );
	rlen = Char2UChar( s, slen, result->data );
	SET_VARSIZE(result, sizeof(UChar) * rlen + MCHARHDRSZ);

	mchar_strip(result, atttypmod);

	PG_RETURN_MCHAR(result);
}

Datum
mchar_out(PG_FUNCTION_ARGS) {
	MChar	*in = PG_GETARG_MCHAR(0);
	char	*out;
	size_t	size, inlen = UCHARLENGTH(in);
	size_t  charlen = u_countChar32(in->data, inlen);

	Assert( in->typmod < 0 || charlen<=in->typmod );
	size = ( in->typmod < 0 ) ? inlen : in->typmod;
	size *= pg_database_encoding_max_length();

	out = (char*)palloc( size+1 );
	size = UChar2Char( in->data, inlen, out );

	if ( in->typmod>0 && charlen < in->typmod ) {
		memset( out+size, ' ', in->typmod - charlen);
		size += in->typmod - charlen;
	}

	out[size] = '\0';

	PG_FREE_IF_COPY(in,0);

	PG_RETURN_CSTRING(out);
}

Datum
mchar(PG_FUNCTION_ARGS) {
	MChar	*source = PG_GETARG_MCHAR(0);
	MChar	*result;
	int32    typmod = PG_GETARG_INT32(1);
#ifdef NOT_USED
	bool     isExplicit = PG_GETARG_BOOL(2);
#endif

	result = palloc( VARSIZE(source) );
	memcpy( result, source, VARSIZE(source) );
	PG_FREE_IF_COPY(source,0);

	mchar_strip(result, typmod);

	PG_RETURN_MCHAR(result);
}

Datum
mvarchar_in(PG_FUNCTION_ARGS) {
	char       *s = PG_GETARG_CSTRING(0);
#ifdef NOT_USED
	Oid         typelem = PG_GETARG_OID(1);
#endif
	int32       atttypmod = PG_GETARG_INT32(2);
	MVarChar	*result;
	int32		slen = strlen(s), rlen;

	pg_verifymbstr(s, slen, false);

	result = (MVarChar*)palloc( MVARCHARHDRSZ + slen * sizeof(UChar) * 2 /* upper limit of length */ );
	rlen = Char2UChar( s, slen, result->data );
	SET_VARSIZE(result, sizeof(UChar) * rlen + MVARCHARHDRSZ);

	if ( atttypmod > 0 && atttypmod < u_countChar32(result->data, UVARCHARLENGTH(result)) )
		elog(ERROR,"value too long for type mvarchar(%d)", atttypmod);

	PG_RETURN_MVARCHAR(result);
}

Datum
mvarchar_out(PG_FUNCTION_ARGS) {
	MVarChar	*in = PG_GETARG_MVARCHAR(0);
	char	*out;
	size_t	size = UVARCHARLENGTH(in);

	size *= pg_database_encoding_max_length();

	out = (char*)palloc( size+1 );
	size = UChar2Char( in->data, UVARCHARLENGTH(in), out );

	out[size] = '\0';

	PG_FREE_IF_COPY(in,0);

	PG_RETURN_CSTRING(out);
}

static void
mvarchar_strip(MVarChar *m, int atttypmod) {
	int     charlen = u_countChar32(m->data, UVARCHARLENGTH(m));

	if ( atttypmod>=0 && atttypmod < charlen ) {
		int i=0;
		U16_FWD_N( m->data, i, charlen, atttypmod);
		SET_VARSIZE(m, sizeof(UChar) * i + MVARCHARHDRSZ);
	}
}

Datum
mvarchar(PG_FUNCTION_ARGS) {
	MVarChar	*source = PG_GETARG_MVARCHAR(0);
	MVarChar	*result;
	int32    typmod = PG_GETARG_INT32(1);
	bool     isExplicit = PG_GETARG_BOOL(2);
	int		charlen = u_countChar32(source->data, UVARCHARLENGTH(source)); 

	result = palloc( VARSIZE(source) );
	memcpy( result, source, VARSIZE(source) );
	PG_FREE_IF_COPY(source,0);

	if ( typmod>=0 && typmod < charlen ) {
		if ( isExplicit )
			mvarchar_strip(result, typmod);
		else
			elog(ERROR,"value too long for type mvarchar(%d)", typmod);
	}

	PG_RETURN_MVARCHAR(result);
}

PG_FUNCTION_INFO_V1(mvarchar_mchar);
Datum       mvarchar_mchar(PG_FUNCTION_ARGS);
Datum       
mvarchar_mchar(PG_FUNCTION_ARGS) {
	MVarChar	*source = PG_GETARG_MVARCHAR(0);
	MChar	*result;
	int32    typmod = PG_GETARG_INT32(1);
#ifdef NOT_USED
	bool     isExplicit = PG_GETARG_BOOL(2);
#endif

	result = palloc( MVARCHARLENGTH(source) +  MCHARHDRSZ );
	SET_VARSIZE(result, MVARCHARLENGTH(source) +  MCHARHDRSZ);
	memcpy( result->data, source->data, MVARCHARLENGTH(source));

	PG_FREE_IF_COPY(source,0);
	
	mchar_strip( result, typmod );

	PG_RETURN_MCHAR(result);
}

PG_FUNCTION_INFO_V1(mchar_mvarchar);
Datum       mchar_mvarchar(PG_FUNCTION_ARGS);
Datum       
mchar_mvarchar(PG_FUNCTION_ARGS) {
	MChar	*source = PG_GETARG_MCHAR(0);
	MVarChar	*result;
	int32    typmod = PG_GETARG_INT32(1);
	int32	 scharlen = u_countChar32(source->data, UCHARLENGTH(source));
	int32	 curlen = 0, maxcharlen;
#ifdef NOT_USED
	bool     isExplicit = PG_GETARG_BOOL(2);
#endif

	maxcharlen = (source->typmod > 0) ? source->typmod : scharlen;

	result = palloc( MVARCHARHDRSZ + sizeof(UChar) * 2 * maxcharlen );

	curlen = UCHARLENGTH( source );
	if ( curlen > 0 )
		memcpy( result->data, source->data, MCHARLENGTH(source) );
	if ( source->typmod > 0 && scharlen < source->typmod  ) {
		FillWhiteSpace( result->data + curlen, source->typmod-scharlen );
		curlen += source->typmod-scharlen;
	}
	SET_VARSIZE(result, MVARCHARHDRSZ + curlen *sizeof(UChar));

	PG_FREE_IF_COPY(source,0);
	
	mvarchar_strip( result, typmod );

	PG_RETURN_MCHAR(result);
}

PG_FUNCTION_INFO_V1(mchar_send);
Datum       mchar_send(PG_FUNCTION_ARGS);
Datum       
mchar_send(PG_FUNCTION_ARGS) {
	MChar	*in = PG_GETARG_MCHAR(0);
	size_t	inlen = UCHARLENGTH(in);
	size_t  charlen = u_countChar32(in->data, inlen);
	StringInfoData buf;

	Assert( in->typmod < 0 || charlen<=in->typmod );

	pq_begintypsend(&buf);
	pq_sendbytes(&buf, (char*)in->data, inlen * sizeof(UChar) );

	if ( in->typmod>0 && charlen < in->typmod ) {
		int		nw = in->typmod - charlen;
		UChar	*white = palloc( sizeof(UChar) * nw );

		FillWhiteSpace( white, nw );
		pq_sendbytes(&buf, (char*)white, sizeof(UChar) * nw);
		pfree(white);
	}

	PG_FREE_IF_COPY(in,0);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));	
}

PG_FUNCTION_INFO_V1(mchar_recv);
Datum       mchar_recv(PG_FUNCTION_ARGS);
Datum       
mchar_recv(PG_FUNCTION_ARGS) {
	StringInfo  buf = (StringInfo) PG_GETARG_POINTER(0);
	MChar		*res; 
	int			nbytes;
#ifdef NOT_USED
	Oid         typelem = PG_GETARG_OID(1);
#endif
	int32       atttypmod = PG_GETARG_INT32(2);

	nbytes = buf->len - buf->cursor;
	res = (MChar*)palloc( nbytes + MCHARHDRSZ );
	res->len = nbytes + MCHARHDRSZ;
	res->typmod = -1;
	SET_VARSIZE(res, res->len);
	pq_copymsgbytes(buf, (char*)res->data, nbytes);

	mchar_strip( res, atttypmod );

	PG_RETURN_MCHAR(res);	
}

PG_FUNCTION_INFO_V1(mvarchar_send);
Datum       mvarchar_send(PG_FUNCTION_ARGS);
Datum       
mvarchar_send(PG_FUNCTION_ARGS) {
	MVarChar	*in = PG_GETARG_MVARCHAR(0);
	size_t	inlen = UVARCHARLENGTH(in);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendbytes(&buf, (char*)in->data, inlen * sizeof(UChar) );

	PG_FREE_IF_COPY(in,0);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));	
}

PG_FUNCTION_INFO_V1(mvarchar_recv);
Datum       mvarchar_recv(PG_FUNCTION_ARGS);
Datum       
mvarchar_recv(PG_FUNCTION_ARGS) {
	StringInfo  buf = (StringInfo) PG_GETARG_POINTER(0);
	MVarChar	*res; 
	int			nbytes;
#ifdef NOT_USED
	Oid         typelem = PG_GETARG_OID(1);
#endif
	int32       atttypmod = PG_GETARG_INT32(2);

	nbytes = buf->len - buf->cursor;
	res = (MVarChar*)palloc( nbytes + MVARCHARHDRSZ );
	res->len = nbytes + MVARCHARHDRSZ;
	SET_VARSIZE(res, res->len);
	pq_copymsgbytes(buf, (char*)res->data, nbytes);

	mvarchar_strip( res, atttypmod );

	PG_RETURN_MVARCHAR(res);	
}


