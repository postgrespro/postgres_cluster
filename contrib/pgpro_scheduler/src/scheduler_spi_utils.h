#ifndef PGPRO_SCHEDULER_SPI_H
#define PGPRO_SCHEDULER_SPI_H

#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "access/xact.h"
#include "utils/snapmgr.h"

#define select_count_sql(SQL) select_oneintvalue_sql(SQL, 0);

typedef struct {
	bool null;
	Datum dat;
} spi_val_t;

typedef struct {
	int n_rows;
	int n_attrs;
	Oid *types;
	bool *ref;
	spi_val_t **rows;
	int retval;
	char *error;
} spi_response_t;

void START_SNAP(void);
void STOP_SNAP(void);
void START_SPI_SNAP(void);
void STOP_SPI_SNAP(void);
void ABORT_SPI_SNAP(void);

char *_mcopy_string(MemoryContext ctx, char *str);
char *my_copy_string(char *str);

int select_oneintvalue_sql(const char *sql, int d);
Datum select_onedatumvalue_sql(const char *sql, bool *is_null);
int select_count_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls);

TimestampTz get_timestamp_from_spi(spi_response_t *r, int row_n, int pos, TimestampTz def);
int get_int_from_spi(spi_response_t *r, int row_n, int pos, int def);
int64 get_int64_from_spi(spi_response_t *r, int row_n, int pos, int def);
int64 get_interval_seconds_from_spi(spi_response_t *r, int row_n, int pos, long def);
char **get_textarray_from_spi(MemoryContext ctx, spi_response_t *r, int row_n, int pos, int *N);
int64 *get_int64array_from_spi(MemoryContext ctx, spi_response_t *r, int row_n, int pos, int *N);
bool get_boolean_from_spi(spi_response_t *r, int row_n, int pos, bool def);
char *get_text_from_spi(MemoryContext ctx, spi_response_t *r, int row_n, int pos);
Oid get_oid_from_spi(spi_response_t *r, int row_n, int pos, Oid def);

spi_response_t *execute_spi_sql_with_args(MemoryContext ctx, const char *sql, int n, Oid *argtypes, Datum *values, char *nulls);
spi_response_t *execute_spi(MemoryContext ctx, const char *sql);
spi_response_t *execute_spi_params_prepared(MemoryContext ctx, const char *sql, int nparams, char **params);


spi_response_t *__error_spi_resp(MemoryContext ctx, int ret, char *error);
spi_response_t *__copy_spi_data(MemoryContext ctx, int ret, int  n);
void destroy_spi_data(spi_response_t *d);

#endif
