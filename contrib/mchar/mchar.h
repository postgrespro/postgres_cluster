#ifndef __MCHAR_H__
#define __MCHAR_H__

#include "postgres.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"

typedef struct {
	int32	len;
	int32 	typmod;
	UChar	data[1];
} MChar;

#define MCHARHDRSZ	offsetof(MChar, data)	
#define MCHARLENGTH(m)	( VARSIZE(m)-MCHARHDRSZ )
#define UCHARLENGTH(m)  ( MCHARLENGTH(m)/sizeof(UChar) )	

#define DatumGetMChar(m)	((MChar*)DatumGetPointer(m))
#define MCharGetDatum(m)	PointerGetDatum(m)

#define	PG_GETARG_MCHAR(n)	DatumGetMChar(PG_DETOAST_DATUM(PG_GETARG_DATUM(n)))
#define	PG_GETARG_MCHAR_COPY(n)	DatumGetMChar(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(n)))

#define PG_RETURN_MCHAR(m)	PG_RETURN_POINTER(m)

typedef struct {
	int32	len;
	UChar	data[1];
} MVarChar;

#define MVARCHARHDRSZ 	offsetof(MVarChar, data)
#define MVARCHARLENGTH(m)  ( VARSIZE(m)-MVARCHARHDRSZ )
#define UVARCHARLENGTH(m)  ( MVARCHARLENGTH(m)/sizeof(UChar) )

#define DatumGetMVarChar(m)	((MVarChar*)DatumGetPointer(m))
#define MVarCharGetDatum(m)	PointerGetDatum(m)

#define	PG_GETARG_MVARCHAR(n)	DatumGetMVarChar(PG_DETOAST_DATUM(PG_GETARG_DATUM(n)))
#define	PG_GETARG_MVARCHAR_COPY(n)	DatumGetMVarChar(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(n)))

#define PG_RETURN_MVARCHAR(m)	PG_RETURN_POINTER(m)


int Char2UChar(const char * src, int srclen, UChar *dst); 
int UChar2Char(const UChar * src, int srclen, char *dst); 
int UChar2Wchar(UChar * src, int srclen, pg_wchar *dst);
int UCharCompare(UChar * a, int alen, UChar *b, int blen); 
int UCharCaseCompare(UChar * a, int alen, UChar *b, int blen); 

void FillWhiteSpace( UChar *dst, int n );

int lengthWithoutSpaceVarChar(MVarChar *m);
int lengthWithoutSpaceChar(MChar *m);

extern Datum       mchar_hash(PG_FUNCTION_ARGS);
extern Datum       mvarchar_hash(PG_FUNCTION_ARGS);

int m_isspace(UChar c); /* is == ' ' */

#endif
