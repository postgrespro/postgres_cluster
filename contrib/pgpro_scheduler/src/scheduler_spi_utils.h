#ifndef PGPRO_SCHEDULER_SPI_H
#define PGPRO_SCHEDULER_SPI_H

#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "access/xact.h"
#include "utils/snapmgr.h"

#define select_count_sql(SQL) select_oneintvalue_sql(SQL, 0);

#define START_SNAP() \
    SetCurrentStatementStartTimestamp(); \
	StartTransactionCommand(); \
	PushActiveSnapshot(GetTransactionSnapshot());

#define STOP_SNAP() \
	PopActiveSnapshot(); \
	CommitTransactionCommand(); 

#define START_SPI_SNAP() \
    SetCurrentStatementStartTimestamp(); \
	StartTransactionCommand(); \
	AssertState(SPI_connect() == SPI_OK_CONNECT); \
	PushActiveSnapshot(GetTransactionSnapshot());

#define STOP_SPI_SNAP() \
	SPI_finish(); \
	PopActiveSnapshot(); \
	CommitTransactionCommand(); 

#define ABORT_SPI_SNAP() \
	PopActiveSnapshot(); \
	AbortCurrentTransaction(); \
	SPI_finish();

char *_copy_string(char *str);
TimestampTz get_timestamp_from_spi(int row_n, int pos, TimestampTz def);
int get_int_from_spi(int row_n, int pos, int def);
int select_oneintvalue_sql(const char *sql, int d);
Datum select_onedatumvalue_sql(const char *sql, bool *is_null);
int select_count_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls);
long int get_interval_seconds_from_spi(int row_n, int pos, long def);
char **get_textarray_from_spi(int row_n, int pos, int *N);
bool get_boolean_from_spi(int row_n, int pos, bool def);
char *get_text_from_spi(int row_n, int pos);
Oid get_oid_from_spi(int row_n, int pos, Oid def);
int execute_spi_sql_with_args(const char *sql, int n, Oid *argtypes, Datum *values, char *nulls, char **error);
int execute_spi(const char *sql, char **error);

#endif
