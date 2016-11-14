#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "scheduler_spi_utils.h"
#include "utils/timestamp.h"
#include "utils/array.h"
#include "utils/elog.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include "catalog/pg_type.h"
#include "memutils.h"

char *_copy_string(char *str)
{
	int len = strlen(str);
	char *cpy;

	cpy = worker_alloc(sizeof(char) * (len+1));

	memcpy(cpy, str, len);
	cpy[len] = 0;

	return cpy;
}

Oid get_oid_from_spi(int row_n, int pos, Oid def)
{
	Datum datum;
	bool is_null;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return def;
	return DatumGetObjectId(datum);
}

bool get_boolean_from_spi(int row_n, int pos, bool def)
{
	Datum datum;
	bool is_null;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return def;
	return DatumGetBool(datum);
}

char **get_textarray_from_spi(int row_n, int pos, int *N)
{
	Datum datum;
	bool is_null;
	ArrayType *input;
	Datum *datums;
	bool i_typbyval;
	char i_typalign;
	int16 i_typlen;
	int len, i, arr_len;
	bool *nulls;
	char **result;

	*N = 0;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return NULL;

	input = DatumGetArrayTypeP(datum);
	if(ARR_ELEMTYPE(input) != TEXTOID)
	{
		return NULL;
	}
	get_typlenbyvalalign(TEXTOID, &i_typlen, &i_typbyval, &i_typalign);
	deconstruct_array(input, TEXTOID, i_typlen, i_typbyval, i_typalign, &datums, &nulls, &len);

	if(len == 0) return NULL;
	arr_len  = len;

	for(i=0; i < len; i++)
	{
		if(nulls[i]) arr_len--;
	}
	result = worker_alloc(sizeof(char *) * arr_len);
	for(i=0; i < len; i++)
	{
		if(!nulls[i]) 
		{
			result[*N] = _copy_string(TextDatumGetCString(datums[i]));
			(*N)++;
		}
	}

	return result;
}

TimestampTz get_timestamp_from_spi(int row_n, int pos, TimestampTz def)
{
	Datum datum;
	bool is_null;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return def;
	return DatumGetTimestampTz(datum);
}

char *get_text_from_spi(int row_n, int pos)
{
	Datum datum;
	bool is_null;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return NULL;
	return _copy_string(TextDatumGetCString(datum));
}

long int get_interval_seconds_from_spi(int row_n, int pos, long def)
{
	Datum datum;
	bool is_null;
	Interval *interval;
	long result = 0;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return def;
	interval = DatumGetIntervalP(datum);
#ifdef HAVE_INT64_TIMESTAMP
	result = interval->time / 1000000.0;
#else
	result = interval->time;
#endif
	result += (DAYS_PER_YEAR * SECS_PER_DAY) * (interval->month / MONTHS_PER_YEAR);
	result += (DAYS_PER_MONTH * SECS_PER_DAY) * (interval->month % MONTHS_PER_YEAR);
	result += SECS_PER_DAY * interval->day;

	return result;
}

int get_int_from_spi(int row_n, int pos, int def)
{
	Datum datum;
	bool is_null;

	datum = SPI_getbinval(SPI_tuptable->vals[row_n], SPI_tuptable->tupdesc,
	                        pos, &is_null);
	if(is_null) return def;
	return (int)DatumGetInt32(datum);
}

Datum select_onedatumvalue_sql(const char *sql, bool *is_null)
{
	int ret;
	Datum datum = 0;

	ret = SPI_execute(sql, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed == 1)
		{
			datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc,
						1, is_null);
			return datum;
		}
	}
	*is_null = true;
	return datum;
}

int select_count_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls)
{
	int ret;
	Datum datum;
	bool is_null;

	ret = SPI_execute_with_args(sql, n, argtypes, values, nulls, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed == 1)
		{
			datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc,
						1, &is_null);
			if(is_null) return -2;
			return (int)DatumGetInt64(datum);
		}
	}
	return -1;
}

int select_oneintvalue_sql(const char *sql, int def)
{
	Datum d;
	bool is_null;

	d = select_onedatumvalue_sql(sql, &is_null);
	if(is_null) return def;
	return DatumGetInt32(d);
}

int execute_spi_sql_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls, char **error)
{
	int ret = -100;
	ErrorData *edata;
	MemoryContext old;

	*error = NULL;

	PG_TRY();
	{
		ret = SPI_execute_with_args(sql, n, argtypes, values, nulls, false, 0);
	}
	PG_CATCH();
	{
		old = switch_to_worker_context();

		edata = CopyErrorData();
		if(edata->message)
		{
			*error = _copy_string(edata->message);
		}
		else if(edata->detail)
		{
			*error = _copy_string(edata->detail);
		}
		else
		{
			*error = _copy_string("unknown error");
		}
		FreeErrorData(edata);
		MemoryContextSwitchTo(old);
		FlushErrorState();
	}
	PG_END_TRY();

	return ret;
}

int execute_spi(const char *sql, char **error)
{
	return execute_spi_sql_with_args(sql, 0, NULL, NULL, NULL, error);
}


