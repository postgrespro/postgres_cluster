/*-------------------------------------------------------------------------
 *
 * encode.c
 *	  Various data encoding/decoding things.
 *
 * Copyright (c) 2001-2016, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/encode.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>

#include "common/encode_utils.h"
#include "utils/builtins.h"


struct pg_encoding
{
	unsigned	(*encode_len) (const char *data, unsigned dlen);
	unsigned	(*decode_len) (const char *data, unsigned dlen);
	unsigned	(*encode) (const char *data, unsigned dlen, char *res);
	unsigned	(*decode) (const char *data, unsigned dlen, char *res);
};

static const struct pg_encoding *pg_find_encoding(const char *name);

/*
 * SQL functions.
 */

Datum
binary_encode(PG_FUNCTION_ARGS)
{
	bytea	   *data = PG_GETARG_BYTEA_P(0);
	Datum		name = PG_GETARG_DATUM(1);
	text	   *result;
	char	   *namebuf;
	int			datalen,
				resultlen,
				res;
	const struct pg_encoding *enc;

	datalen = VARSIZE(data) - VARHDRSZ;

	namebuf = TextDatumGetCString(name);

	enc = pg_find_encoding(namebuf);
	if (enc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unrecognized encoding: \"%s\"", namebuf)));

	resultlen = enc->encode_len(VARDATA(data), datalen);
	result = palloc(VARHDRSZ + resultlen);

	res = enc->encode(VARDATA(data), datalen, VARDATA(result));

	/* Make this FATAL 'cause we've trodden on memory ... */
	if (res > resultlen)
		elog(FATAL, "overflow - encode estimate too small");

	SET_VARSIZE(result, VARHDRSZ + res);

	PG_RETURN_TEXT_P(result);
}

Datum
binary_decode(PG_FUNCTION_ARGS)
{
	text	   *data = PG_GETARG_TEXT_P(0);
	Datum		name = PG_GETARG_DATUM(1);
	bytea	   *result;
	char	   *namebuf;
	int			datalen,
				resultlen,
				res;
	const struct pg_encoding *enc;

	datalen = VARSIZE(data) - VARHDRSZ;

	namebuf = TextDatumGetCString(name);

	enc = pg_find_encoding(namebuf);
	if (enc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unrecognized encoding: \"%s\"", namebuf)));

	resultlen = enc->decode_len(VARDATA(data), datalen);
	result = palloc(VARHDRSZ + resultlen);

	res = enc->decode(VARDATA(data), datalen, VARDATA(result));

	/* Make this FATAL 'cause we've trodden on memory ... */
	if (res > resultlen)
		elog(FATAL, "overflow - decode estimate too small");

	SET_VARSIZE(result, VARHDRSZ + res);

	PG_RETURN_BYTEA_P(result);
}


/*
 * Common
 */

static const struct
{
	const char *name;
	struct pg_encoding enc;
}	enclist[] =

{
	{
		"hex",
		{
			hex_enc_len, hex_dec_len, hex_encode, hex_decode
		}
	},
	{
		"base64",
		{
			b64_enc_len, b64_dec_len, b64_encode, b64_decode
		}
	},
	{
		"escape",
		{
			esc_enc_len, esc_dec_len, esc_encode, esc_decode
		}
	},
	{
		NULL,
		{
			NULL, NULL, NULL, NULL
		}
	}
};

static const struct pg_encoding *
pg_find_encoding(const char *name)
{
	int			i;

	for (i = 0; enclist[i].name; i++)
		if (pg_strcasecmp(enclist[i].name, name) == 0)
			return &enclist[i].enc;

	return NULL;
}
