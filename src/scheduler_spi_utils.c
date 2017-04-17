#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "scheduler_spi_utils.h"
#include "utils/timestamp.h"
#include "utils/array.h"
#include "utils/elog.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "catalog/pg_type.h"
#include "memutils.h"

void START_SNAP(void)
{
	SetCurrentStatementStartTimestamp(); 
	StartTransactionCommand(); 
	PushActiveSnapshot(GetTransactionSnapshot());
}

void STOP_SNAP(void) 
{
	PopActiveSnapshot(); 
	CommitTransactionCommand();
}

void START_SPI_SNAP(void)
{
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot()); 
}

void STOP_SPI_SNAP(void)
{
	SPI_finish();
	PopActiveSnapshot(); 
	CommitTransactionCommand();
}

void ABORT_SPI_SNAP(void) 
{
	PopActiveSnapshot(); 
	AbortCurrentTransaction();
	SPI_finish();
}

void destroy_spi_data(spi_response_t *d)
{
	int i,j;

	if(d->error) pfree(d->error);
	if(d->n_rows > 0)
	{
		for(i=0; i < d->n_rows; i++)
		{
			for(j=0; j < d->n_attrs; j++)
			{
				if(d->ref[j] && !(d->rows[i][j].null))
							pfree(DatumGetPointer(d->rows[i][j].dat));
			}
			pfree(d->rows[i]);
		}
	}
	if(d->types) pfree(d->types);
	if(d->ref) pfree(d->ref);
	pfree(d);
}

spi_response_t *__error_spi_resp(MemoryContext ctx, int ret, char *error)
{
	spi_response_t *r;

	r = MemoryContextAlloc(ctx, sizeof(spi_response_t));
	r->n_rows = 0;
	r->n_attrs = 0;
	r->retval = ret;
	r->types = NULL;
	r->rows = NULL;
	r->ref = NULL;
	r->error = _mcopy_string(ctx, error);

	return r;
}

spi_response_t *__copy_spi_data(MemoryContext ctx, int ret, int  n)
{
	spi_response_t *r;
	int i, j;
	Datum dat;
	bool is_null;


	r = MemoryContextAlloc(ctx, sizeof(spi_response_t));
	r->retval = ret;
	r->error = NULL;

	if(n == 0 || !SPI_tuptable)
	{
		r->types = NULL;
		r->rows = NULL;
		r->ref = NULL;
		r->n_rows = 0;
		r->n_attrs = 0;

		return r;
	}
	r->n_rows = n;
	r->n_attrs = SPI_tuptable->tupdesc->natts;
	r->types = MemoryContextAlloc(ctx, sizeof(Oid) * r->n_attrs);
	r->rows = MemoryContextAlloc(ctx, sizeof(spi_val_t *) * n);
	r->ref = MemoryContextAlloc(ctx, sizeof(bool) * r->n_attrs);


	for(i=0; i < r->n_attrs; i++)
	{
		r->types[i] = SPI_gettypeid(SPI_tuptable->tupdesc, i+1);
		r->ref[i] = SPI_tuptable->tupdesc->attrs[i]->attbyval ? false: true;
	}

	for(i=0; i < n; i++)
	{
		r->rows[i] = MemoryContextAlloc(ctx, sizeof(spi_val_t) * r->n_attrs);
		for(j=0; j < r->n_attrs; j++)
		{
			dat = SPI_getbinval(SPI_tuptable->vals[i],
									SPI_tuptable->tupdesc, j+1, &is_null);
			if(is_null)
			{
				r->rows[i][j].dat =  0;
				r->rows[i][j].null =  true;
			}	
			else
			{
				r->rows[i][j].dat = datumCopy(dat,
						SPI_tuptable->tupdesc->attrs[j]->attbyval,
						SPI_tuptable->tupdesc->attrs[j]->attlen);
				r->rows[i][j].null = false;
			}
		}
	}

	return r;
}

char *_mcopy_string(MemoryContext ctx, char *str)
{
	int len = strlen(str);
	char *cpy;

	if(!ctx) ctx = SchedulerWorkerContext;

	cpy = MemoryContextAlloc(ctx, sizeof(char) * (len+1));

	memcpy(cpy, str, len);
	cpy[len] = 0;

	return cpy;
}

char *my_copy_string(char *str)
{
	int len = strlen(str);
	char *cpy;

	cpy = palloc(sizeof(char) * (len+1));

	memcpy(cpy, str, len);
	cpy[len] = 0;

	return cpy;
}

Oid get_oid_from_spi(spi_response_t *r, int row_n, int pos, Oid def)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return def;
	return DatumGetObjectId(datum);
}

bool get_boolean_from_spi(spi_response_t *r, int row_n, int pos, bool def)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
									SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return def;
	return DatumGetBool(datum);
}

int64 *get_int64array_from_spi(MemoryContext mem, spi_response_t *r, int row_n, int pos, int *N)
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
	int64 *result;

	*N = 0;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return NULL;

	input = DatumGetArrayTypeP(datum);
	if(ARR_ELEMTYPE(input) != INT8OID)
	{
		return NULL;
	}
	get_typlenbyvalalign(INT8OID, &i_typlen, &i_typbyval, &i_typalign);
	deconstruct_array(input, INT8OID, i_typlen, i_typbyval, i_typalign, &datums, &nulls, &len);

	if(len == 0) return NULL;
	arr_len  = len;

	for(i=0; i < len; i++)
	{
		if(nulls[i]) arr_len--;
	}
	result = MemoryContextAlloc(mem, sizeof(int64) * arr_len);
	for(i=0; i < len; i++)
	{
		if(!nulls[i]) 
		{
			result[*N] = Int64GetDatum(datums[i]);
			(*N)++;
		}
	}

	return result;
}

char **get_textarray_from_spi(MemoryContext mem, spi_response_t *r, int row_n, int pos, int *N)
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

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
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
	result = MemoryContextAlloc(mem, sizeof(char *) * arr_len);
	for(i=0; i < len; i++)
	{
		if(!nulls[i]) 
		{
			result[*N] = _mcopy_string(mem, TextDatumGetCString(datums[i]));
			(*N)++;
		}
	}

	return result;
}

TimestampTz get_timestamp_from_spi(spi_response_t *r, int row_n, int pos, TimestampTz def)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
								SPI_tuptable->tupdesc, pos, &is_null);
	}

	if(is_null) return def;
	return DatumGetTimestampTz(datum);
}

char *get_text_from_spi(MemoryContext mem, spi_response_t *r, int row_n, int pos)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return NULL;
	return _mcopy_string(mem, TextDatumGetCString(datum));
}

int64 get_interval_seconds_from_spi(spi_response_t *r, int row_n, int pos, long def)
{
	Datum datum;
	bool is_null;
	Interval *interval;
	int64 result = 0;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
								SPI_tuptable->tupdesc, pos, &is_null);
	}
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

int get_int_from_spi(spi_response_t *r, int row_n, int pos, int def)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return def;
	return (int)DatumGetInt32(datum);
}

int64 get_int64_from_spi(spi_response_t *r, int row_n, int pos, int def)
{
	Datum datum;
	bool is_null;

	if(r)
	{
		datum = r->rows[row_n][pos-1].dat;
		is_null = r->rows[row_n][pos-1].null;
	}
	else
	{
		datum = SPI_getbinval(SPI_tuptable->vals[row_n],
							SPI_tuptable->tupdesc, pos, &is_null);
	}
	if(is_null) return def;
	return (int64)DatumGetInt64(datum);
}

Datum select_onedatumvalue_sql(const char *sql, bool *is_null)
{
	int ret;
	Datum datum = 0;

	SetCurrentStatementStartTimestamp();
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

	SetCurrentStatementStartTimestamp();
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

spi_response_t *execute_spi_sql_with_args(MemoryContext ctx, const char *sql, int n, Oid *argtypes, Datum *values, char *nulls)
{
	int ret = -100;
	ErrorData *edata;
	char other[100];
	ResourceOwner oldowner = CurrentResourceOwner;
	spi_response_t *rv = NULL;
	if(!ctx) ctx = SchedulerWorkerContext;

	SetCurrentStatementStartTimestamp();
	BeginInternalSubTransaction(NULL);
	MemoryContextSwitchTo(ctx);

	PG_TRY();
	{
		ret = SPI_execute_with_args(sql, n, argtypes, values, nulls, false, 0);
		MemoryContextSwitchTo(ctx);
		rv = __copy_spi_data(ctx, ret, SPI_processed);
		ReleaseCurrentSubTransaction();
		MemoryContextSwitchTo(ctx);
		CurrentResourceOwner = oldowner;
		SPI_restore_connection(); 
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(ctx);
		edata = CopyErrorData();
		if(edata->message)
		{
			rv = __error_spi_resp(ctx, ret, edata->message);
		}
		else if(edata->detail)
		{
			rv = __error_spi_resp(ctx, ret, edata->detail);
		}
		else
		{
			rv = __error_spi_resp(ctx, ret, "unknown error");
		}
		RollbackAndReleaseCurrentSubTransaction(); 
		CurrentResourceOwner = oldowner;
		MemoryContextSwitchTo(ctx);
		SPI_restore_connection(); 
		FreeErrorData(edata);
		FlushErrorState();
	}
	PG_END_TRY();

	if(!rv && ret < 0)
	{
		if(ret == SPI_ERROR_CONNECT)
		{
			rv = __error_spi_resp(ctx, ret, "Connection error");
		}
		else if(ret == SPI_ERROR_COPY)
		{
			rv = __error_spi_resp(ctx, ret, "COPY error");
		}
		else if(ret == SPI_ERROR_OPUNKNOWN)
		{
			rv = __error_spi_resp(ctx, ret, "SPI_ERROR_OPUNKNOWN");
		}
		else if(ret == SPI_ERROR_UNCONNECTED)
		{
			rv = __error_spi_resp(ctx, ret, "Unconnected call");
		}
		else
		{
			sprintf(other, "error number: %d", ret);
			rv = __error_spi_resp(ctx, ret, other);
		}
	}

	return rv;
}

spi_response_t *execute_spi(MemoryContext ctx, const char *sql)
{	
	return execute_spi_sql_with_args(ctx, sql, 0, NULL, NULL, NULL);
}

spi_response_t *execute_spi_params_prepared(MemoryContext ctx, const char *sql, int nparams, char **params)
{
	int ret = -100;
	ErrorData *edata;
	char other[100];
	SPIPlanPtr plan;
	Oid *paramtypes;
	Datum *values;
	int i;
	ResourceOwner oldowner = CurrentResourceOwner;
	spi_response_t *rv;

	if(!ctx) ctx = SchedulerWorkerContext;


	paramtypes = worker_alloc(sizeof(Oid) * nparams);
	values = worker_alloc(sizeof(Datum) * nparams);
	for(i=0; i < nparams; i++)
	{
		paramtypes[i] = TEXTOID;
		values[i] = CStringGetTextDatum(params[i]);
	}

	SetCurrentStatementStartTimestamp();
	BeginInternalSubTransaction(NULL);
	switch_to_worker_context();

	PG_TRY();
	{
		plan = SPI_prepare(sql, nparams, paramtypes);
		if(plan)
		{
			SetCurrentStatementStartTimestamp();
			ret = SPI_execute_plan(plan, values, NULL, false, 0);
			rv = __copy_spi_data(ctx, ret, SPI_processed);
		}
		ReleaseCurrentSubTransaction();
		switch_to_worker_context();
		CurrentResourceOwner = oldowner;
		SPI_restore_connection();
	}
	PG_CATCH();
	{
		switch_to_worker_context();

		edata = CopyErrorData();
		if(edata->message)
		{
			rv = __error_spi_resp(ctx, ret, edata->message);
		}
		else if(edata->detail)
		{
			rv = __error_spi_resp(ctx, ret, edata->detail);
		}
		else
		{
			rv = __error_spi_resp(ctx, ret, "unknown error");
		}
		FreeErrorData(edata);
		FlushErrorState();
		RollbackAndReleaseCurrentSubTransaction(); 
		CurrentResourceOwner = oldowner;
		switch_to_worker_context();
		SPI_restore_connection();
	}
	PG_END_TRY();

	pfree(values);
	pfree(paramtypes);

	if(!rv && ret < 0)
	{
		if(ret == SPI_ERROR_CONNECT)
		{
			rv = __error_spi_resp(ctx, ret, "Connection error");
		}
		else if(ret == SPI_ERROR_COPY)
		{
			rv = __error_spi_resp(ctx, ret, "COPY error");
		}
		else if(ret == SPI_ERROR_OPUNKNOWN)
		{
			rv = __error_spi_resp(ctx, ret, "SPI_ERROR_OPUNKNOWN");
		}
		else if(ret == SPI_ERROR_UNCONNECTED)
		{
			rv = __error_spi_resp(ctx, ret, "Unconnected call");
		}
		else
		{
			sprintf(other, "error number: %d", ret);
			rv = __error_spi_resp(ctx, ret, other);
		}
	}

	return rv;
}
