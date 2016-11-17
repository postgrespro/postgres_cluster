#include "mchar.h"

#include "unicode/ucol.h"
#include "unicode/ucnv.h"

static UConverter *cnvDB = NULL;
static UCollator  *colCaseInsensitive = NULL;
static UCollator  *colCaseSensitive = NULL;

static void
createUObjs() {
	if ( !cnvDB ) {
		UErrorCode err = 0;

		if ( GetDatabaseEncoding() == PG_UTF8 )
			cnvDB = ucnv_open("UTF8", &err);
	 	else
			cnvDB = ucnv_open(NULL, &err);
		if ( U_FAILURE(err) || cnvDB == NULL ) 
			elog(ERROR,"ICU ucnv_open returns %d (%s)", err,  u_errorName(err));
	}

	if ( !colCaseInsensitive ) {
		UErrorCode err = 0;

		colCaseInsensitive = ucol_open("", &err);
		if ( U_FAILURE(err) || cnvDB == NULL ) { 
			if ( colCaseSensitive )
				ucol_close( colCaseSensitive );
			colCaseSensitive = NULL;
			elog(ERROR,"ICU ucol_open returns %d (%s)", err,  u_errorName(err));
		}

		ucol_setStrength( colCaseInsensitive, UCOL_SECONDARY );
	}

	if ( !colCaseSensitive ) {
		UErrorCode err = 0;

		colCaseSensitive = ucol_open("", &err);
		if ( U_FAILURE(err) || cnvDB == NULL ) { 
			if ( colCaseSensitive )
				ucol_close( colCaseSensitive );
			colCaseSensitive = NULL;
			elog(ERROR,"ICU ucol_open returns %d (%s)", err,  u_errorName(err));
		}

		ucol_setAttribute(colCaseSensitive, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &err);				
		if (U_FAILURE(err)) {
			if ( colCaseSensitive )
				ucol_close( colCaseSensitive );
			colCaseSensitive = NULL;
			elog(ERROR,"ICU ucol_setAttribute returns %d (%s)", err,  u_errorName(err));
		}
	}
}

int
Char2UChar(const char * src, int srclen, UChar *dst) {
	int dstlen=0;
	UErrorCode err = 0;

	createUObjs();
	dstlen = ucnv_toUChars( cnvDB, dst, srclen*4, src, srclen, &err ); 
	if ( U_FAILURE(err)) 
		elog(ERROR,"ICU ucnv_toUChars returns %d (%s)", err,  u_errorName(err));

	return dstlen;
}

int
UChar2Char(const UChar * src, int srclen, char *dst) {
	int dstlen=0;
	UErrorCode err = 0;

	createUObjs();
	dstlen = ucnv_fromUChars( cnvDB, dst, srclen*4, src, srclen, &err ); 
	if ( U_FAILURE(err) ) 
		elog(ERROR,"ICU ucnv_fromUChars returns %d (%s)", err,  u_errorName(err));

	return dstlen;
}

int
UChar2Wchar(UChar * src, int srclen, pg_wchar *dst) {
	int dstlen=0;
	char	*utf = palloc(sizeof(char)*srclen*4);

	dstlen = UChar2Char(src, srclen, utf);
	dstlen = pg_mb2wchar_with_len( utf, dst, dstlen );
	pfree(utf);

	return dstlen;
}

static UChar UCharWhiteSpace = 0;

void
FillWhiteSpace( UChar *dst, int n ) {
	if ( UCharWhiteSpace == 0 ) {
		int len;
		UErrorCode err = 0;

		u_strFromUTF8( &UCharWhiteSpace, 1, &len, " ", 1, &err);

		Assert( len==1 );
		Assert( !U_FAILURE(err) );
	}

	while( n-- > 0 ) 
		*dst++ = UCharWhiteSpace;
}

int 
UCharCaseCompare(UChar * a, int alen, UChar *b, int blen) {
	int len = Min(alen, blen);
	int res;

	createUObjs();

	res = (int)ucol_strcoll( colCaseInsensitive,
							  a, len,
							  b, len);
	if ( res == 0 && alen != blen )
		return (alen > blen) ? 1 : - 1;
	return res;
}

int 
UCharCompare(UChar * a, int alen, UChar *b, int blen) {
	int len = Min(alen, blen);
	int res;
	
	createUObjs();

	res =  (int)ucol_strcoll( colCaseSensitive,
							  a, len,
							  b, len);
	if ( res == 0 && alen != blen )
		return (alen > blen) ? 1 : - 1;
	return res;
}
