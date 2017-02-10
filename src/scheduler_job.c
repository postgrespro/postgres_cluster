#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "postgres.h"
#include "catalog/pg_type.h"
#include "pgpro_scheduler.h"
#include "scheduler_job.h"
#include "lib/stringinfo.h"
#include "scheduler_spi_utils.h"
#include "utils/timestamp.h"
#include "utils/builtins.h"
#include "memutils.h"
#include "port.h"

job_t *init_scheduler_job(job_t *j, unsigned char type)
{
	if(j == NULL) j = worker_alloc(sizeof(job_t));
	memset(j, 0, sizeof(job_t));
	j->is_active = false;
	j->type = type;

	return j;
}

job_t *get_at_job(int cron_id, char *nodename, char **perror)
{
	job_t *j;
	const char *sql = "select last_start_available, array_append('{}'::text[], do_sql)::text[], executor, postpone, max_run_time as time_limit, at, params, depends_on, attempt, resubmit_limit from ONLY at_jobs_process where node = $1 and id = $2";
	Oid argtypes[2] = { TEXTOID, INT4OID};
	Datum args[2];
	int ret;
	char *error = NULL;
	char buffer[PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX];

	args[0] = PointerGetDatum(cstring_to_text(nodename));
	args[1] = Int32GetDatum(cron_id);
	START_SPI_SNAP();
	ret = execute_spi_sql_with_args(sql, 2, argtypes, args, NULL, &error);
	if(error)
	{
		snprintf(buffer, 
			PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"cannot retrive at job: %s", error);
		*perror = _copy_string(buffer);
		pfree(error);
		PopActiveSnapshot();
		AbortCurrentTransaction();
		SPI_finish();
		return NULL;
	}
	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed == 0)
		{
			STOP_SPI_SNAP();
			snprintf(buffer,
				PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"cannot find at job: %d [%s]",
				cron_id,  nodename);
			*perror = _copy_string(buffer);
			return NULL;
		}

		j = init_scheduler_job(NULL, AtJob);
		j->cron_id = cron_id;
		j->node = _copy_string(nodename);
		j->dosql = get_textarray_from_spi(0, 2, &j->dosql_n);
		j->executor = get_text_from_spi(0, 3);
		j->start_at = get_timestamp_from_spi(0, 6, 0);
		j->sql_params = get_textarray_from_spi(0, 7, &j->sql_params_n);
		j->depends_on = get_int64array_from_spi(0, 8, &j->depends_on_n);
		j->attempt = get_int64_from_spi(0, 9, 0);
		j->resubmit_limit = get_int64_from_spi(0, 10, 0);

		STOP_SPI_SNAP();

		*perror = NULL;
		return j;
	}
	snprintf(buffer,
		PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
		"error while retrive at job information: %d", ret);
	*perror = _copy_string(buffer);
	PopActiveSnapshot();
	AbortCurrentTransaction();
	SPI_finish();

	return NULL;
}

job_t *get_cron_job(int cron_id, TimestampTz start_at, char *nodename, char **perror)
{
	job_t *j;
	const char *sql = "select at.last_start_available, cron.same_transaction, cron.do_sql, cron.executor, cron.postpone, cron.max_run_time as time_limit, cron.max_instances, cron.onrollback_statement , cron.next_time_statement from at, cron where start_at = $1 and  at.active and at.cron = cron.id AND cron.node = $2 AND cron.id = $3";
	Oid argtypes[3] = { TIMESTAMPTZOID, TEXTOID, INT4OID};
	Datum args[3];
	int ret;
	char *error = NULL;
	char *ts;
	char buffer[PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX];

	args[0] = TimestampTzGetDatum(start_at);
	args[1] = PointerGetDatum(cstring_to_text(nodename));
	args[2] = Int32GetDatum(cron_id);
	START_SPI_SNAP();
	ret = execute_spi_sql_with_args(sql, 3, argtypes, args, NULL, &error);
	if(error)
	{
		snprintf(buffer, 
			PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"cannot retrive job: %s", error);
		*perror = _copy_string(buffer);
		pfree(error);
		PopActiveSnapshot();
		AbortCurrentTransaction();
		SPI_finish();
		return NULL;
	}
	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed == 0)
		{
			STOP_SPI_SNAP();
			ts = make_date_from_timestamp(start_at, false);
			snprintf(buffer,
				PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"cannot find job: %d @ %s [%s]", cron_id, ts, nodename);
			*perror = _copy_string(buffer);
			pfree(ts);
			return NULL;
		}

		j = init_scheduler_job(NULL, CronJob);
		j->start_at = start_at;
		j->node = _copy_string(nodename);
		j->same_transaction = get_boolean_from_spi(0, 2, false);
		j->dosql = get_textarray_from_spi(0, 3, &j->dosql_n);
		j->executor = get_text_from_spi(0, 4);
		j->onrollback = get_text_from_spi(0, 8);
		j->next_time_statement = get_text_from_spi(0, 9);
		STOP_SPI_SNAP();

		*perror = NULL;
		return j;
	}
	snprintf(buffer,
		PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
		"error while retrive job information: %d", ret);
	*perror = _copy_string(buffer);
	PopActiveSnapshot();
	AbortCurrentTransaction();
	SPI_finish();

	return NULL;
}

job_t *get_jobs_to_do(char *nodename, task_type_t type, int *n, int *is_error, int limit)
{
	if(type == CronJob) return _cron_get_jobs_to_do(nodename, n, is_error, limit);
	return _at_get_jobs_to_do(nodename, n, is_error, limit);
}

job_t *_at_get_jobs_to_do(char *nodename, int *n, int *is_error, int limit)
{
	job_t *jobs = NULL;
	int ret, got, i;
	Oid argtypes[2] = { TEXTOID, INT4OID };
	Datum values[2];
	const char *get_job_sql = "select id, at, last_start_available, max_run_time,  executor from ONLY at_jobs_submitted where at <= 'now' and (last_start_available is NULL OR last_start_available > 'now') AND node = $1 order by at,  submit_time limit $2";

	*is_error = *n = 0;
	START_SPI_SNAP();
	values[0] = CStringGetTextDatum(nodename);
	values[1] = Int32GetDatum(limit+1);
	ret = SPI_execute_with_args(get_job_sql, 2, argtypes, values, NULL, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = worker_alloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(&(jobs[i]), AtJob);
				jobs[i].cron_id = get_int_from_spi(i, 1, 0);
				jobs[i].start_at = get_timestamp_from_spi(i, 2, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(i, 3, 0);
				jobs[i].timelimit = get_interval_seconds_from_spi(i, 4, 0);
				jobs[i].node = _copy_string(nodename);
				jobs[i].executor = get_text_from_spi(i, 5);
			}
		}
	}
	else
	{
		*is_error = 1;
	}
	STOP_SPI_SNAP();
	return jobs;
}

job_t *_cron_get_jobs_to_do(char *nodename, int *n, int *is_error, int limit)
{
	job_t *jobs = NULL;
	int ret, got, i;
	Oid argtypes[2] = { TEXTOID, INT4OID };
	Datum values[2];
	const char *get_job_sql = "select at.start_at, at.last_start_available, at.cron, max_run_time, cron.max_instances, cron.executor, cron.next_time_statement from at at, cron cron where start_at <= 'now' and not at.active and (last_start_available is NULL OR last_start_available > 'now') and at.cron = cron.id AND cron.node = $1 order by at.start_at limit $2";

	*is_error = *n = 0;
	START_SPI_SNAP();
	values[0] = CStringGetTextDatum(nodename);
	values[1] = Int32GetDatum(limit + 1);
	ret = SPI_execute_with_args(get_job_sql, 2, argtypes, values, NULL, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = worker_alloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(&(jobs[i]), CronJob);
				jobs[i].start_at = get_timestamp_from_spi(i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(i, 3, 0);
				jobs[i].timelimit = get_interval_seconds_from_spi(i, 4, 0);
				jobs[i].max_instances = get_int_from_spi(i, 5, 1);
				jobs[i].node = _copy_string(nodename);
				jobs[i].executor = get_text_from_spi(i, 6);
				jobs[i].next_time_statement = get_text_from_spi(i, 7);
			}
		}
	}
	else
	{
		*is_error = 1;
	}
	STOP_SPI_SNAP();
	return jobs;
}

job_t *get_expired_at_jobs(char *nodename, int *n, int *is_error)
{
	StringInfoData sql;
	job_t *jobs = NULL;
	int ret, got, i;
	
	*n = *is_error = 0;
	initStringInfo(&sql);
	appendStringInfo(&sql, "select at, last_start_available, id from ONLY at_jobs_submitted where last_start_available < 'now' and node = '%s'", nodename);
	ret = SPI_execute(sql.data, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = worker_alloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(&(jobs[i]), 2);
				jobs[i].start_at = get_timestamp_from_spi(i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(i, 3, 0);
				jobs[i].node = _copy_string(nodename);
			}
		}
	}
	else
	{
		*is_error = 1;
	}
	return jobs;
}

job_t *get_expired_cron_jobs(char *nodename, int *n, int *is_error)
{
	StringInfoData sql;
	job_t *jobs = NULL;
	int ret, got, i;
	
	*n = *is_error = 0;
	initStringInfo(&sql);
	appendStringInfo(&sql, "select start_at, last_start_available, cron, started, active from at where last_start_available < 'now' and not active and node = '%s'", nodename);
	ret = SPI_execute(sql.data, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = worker_alloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(&(jobs[i]), 1);
				jobs[i].start_at = get_timestamp_from_spi(i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(i, 3, 0);
				jobs[i].node = _copy_string(nodename);
			}
		}
	}
	else
	{
		*is_error = 1;
	}
	return jobs;
}

job_t *set_job_error(job_t *j, const char *fmt, ...)
{
	va_list arglist;
	char buf[1024];

	va_start(arglist, fmt);
	pvsnprintf(buf, 1024, fmt, arglist);
	va_end(arglist);

	if(j->error) pfree(j->error);
	j->error = _copy_string(buf); 

	return j;
}

int move_job_to_log(job_t *j, bool status, bool process)
{
	if(j->type == CronJob) _cron_move_job_to_log(j, status);
	return _at_move_job_to_log(j, status, process);
}

int _at_move_job_to_log(job_t *j, bool status, bool process)
{
	Datum values[3];	
	char  nulls[3] = { ' ', ' ', ' ' };	
	Oid argtypes[3] = { INT4OID, BOOLOID, TEXTOID };
	int ret;
	const char *sql_process = "WITH moved_rows AS (DELETE from ONLY at_jobs_process a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_done SELECT *, $2 as status, $3 as reason FROM moved_rows";
	const char *sql_submitted = "WITH moved_rows AS (DELETE from ONLY at_jobs_submitted a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_done SELECT *, NULL as start_time, $2 as status, $3 as reason FROM moved_rows";
	const char *sql;

	sql = process ? sql_process: sql_submitted;

	values[0] = Int32GetDatum(j->cron_id);
	values[1] = BoolGetDatum(status);
	if(j->error)
	{
		values[2] = CStringGetTextDatum(j->error);
	}
	else
	{
		nulls[2] = 'n'; 
	}
	ret = SPI_execute_with_args(sql, 3, argtypes, values, nulls, false, 0);

	return ret > 0 ? 1: ret;
}

int resubmit_at_job(job_t *j, TimestampTz next)
{
	Datum values[2];	
	Oid argtypes[2] = { INT4OID, TIMESTAMPTZOID };
	int ret;
	const char *sql = "WITH moved_rows AS (DELETE from ONLY at_jobs_process a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_submitted SELECT id, node, name, comments, $2, do_sql, params, depends_on, executor, owner, last_start_available, attempt +1 , resubmit_limit, postpone, max_run_time, submit_time FROM moved_rows";

	values[0] = Int32GetDatum(j->cron_id);
	values[1] = TimestampTzGetDatum(next);
	ret = SPI_execute_with_args(sql, 2, argtypes, values, NULL, false, 0);

	return ret > 0 ? 1: ret;
}

int _cron_move_job_to_log(job_t *j, bool status)
{
	Datum values[4];	
	char  nulls[4] = { ' ', ' ', ' ', ' ' };	
	Oid argtypes[4] = { BOOLOID, TEXTOID, INT4OID, TIMESTAMPTZOID };
	int ret;
	const char *del_sql = "delete from at where start_at = $1 and cron = $2";
	const char *sql = "insert into log (start_at,  last_start_available, retry, cron, node, started, status, finished, message)  SELECT start_at, last_start_available, retry, cron, node, started, $1 as status, 'now'::timestamp as finished, $2 as message from at where cron = $3 and start_at = $4";

	/* in perl was this at first $status = 0 if $job->{spoiled}; skip so far */

	values[0] = BoolGetDatum(status);
	if(j->error)
	{
		values[1] = CStringGetTextDatum(j->error);
	}
	else
	{
		nulls[1] = 'n'; 
	}
	values[2] = Int32GetDatum(j->cron_id);
	values[3] = TimestampTzGetDatum(j->start_at);

	ret = SPI_execute_with_args(sql, 4, argtypes, values, nulls, false, 0);
	if(ret == SPI_OK_INSERT)
	{
		argtypes[0] = TIMESTAMPTZOID;
		argtypes[1] = INT4OID;
		values[0] = TimestampTzGetDatum(j->start_at);
		values[1] = Int32GetDatum(j->cron_id);
		ret = SPI_execute_with_args(del_sql, 2, argtypes, values, NULL, false, 0);
		if(ret == SPI_OK_DELETE)
		{
			return 1;
		}
	}
	return ret;
}

void destroy_job(job_t *j, int selfdestroy)
{
	int i;

	if(j->node) pfree(j->node);
	if(j->executor) pfree(j->executor);
	if(j->owner) pfree(j->owner);
	if(j->onrollback) pfree(j->onrollback);
	if(j->next_time_statement) pfree(j->next_time_statement);
	if(j->error) pfree(j->error);

	if(j->dosql_n && j->dosql)
	{
		for(i=0; i < j->dosql_n; i++)
		{
			if(j->dosql[i]) pfree(j->dosql[i]);
		}
		pfree(j->dosql);
	}
	if(j->sql_params_n && j->sql_params)
	{
		for(i=0; i < j->sql_params_n; i++)
		{
			if(j->sql_params[i]) pfree(j->sql_params[i]);
		}
		pfree(j->sql_params);
	}
	if(j->depends_on_n && j->depends_on) pfree(j->depends_on);

	if(selfdestroy) pfree(j);
}

