#ifndef PGPRO_SCHEDULER_SPI_H
#define PGPRO_SCHEDULER_SPI_H

#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "access/xact.h"
#include "utils/snapmgr.h"

#define select_count_sql(SQL) select_oneintvalue_sql(SQL, 0);

void START_SNAP(void);
void STOP_SNAP(void);
void START_SPI_SNAP(void);
void STOP_SPI_SNAP(void);
void ABORT_SPI_SNAP(void);

char *_copy_string(char *str);
TimestampTz get_timestamp_from_spi(int row_n, int pos, TimestampTz def);
int get_int_from_spi(int row_n, int pos, int def);
int64 get_int64_from_spi(int row_n, int pos, int def);
int select_oneintvalue_sql(const char *sql, int d);
Datum select_onedatumvalue_sql(const char *sql, bool *is_null);
int select_count_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls);
long int get_interval_seconds_from_spi(int row_n, int pos, long def);
char **get_textarray_from_spi(int row_n, int pos, int *N);
int64 *get_int64array_from_spi(int row_n, int pos, int *N);
bool get_boolean_from_spi(int row_n, int pos, bool def);
char *get_text_from_spi(int row_n, int pos);
Oid get_oid_from_spi(int row_n, int pos, Oid def);
int execute_spi_sql_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls, char **error);
int execute_spi(const char *sql, char **error);
int execute_spi_params_prepared(const char *sql, int nparams, char **params, char **error);

#endif
