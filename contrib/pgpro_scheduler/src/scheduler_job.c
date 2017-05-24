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

job_t *init_scheduler_job(MemoryContext mem, job_t *j, unsigned char type)
{
	if(j == NULL) j = MemoryContextAlloc(mem, sizeof(job_t));
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
	spi_response_t *r;
	char buffer[PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX];
	MemoryContext mem = SchedulerWorkerContext;


	args[0] = PointerGetDatum(cstring_to_text(nodename));
	args[1] = Int32GetDatum(cron_id);
	START_SPI_SNAP();
	r = execute_spi_sql_with_args(mem, sql, 2, argtypes, args, NULL);
	if(r->error)
	{
		snprintf(buffer, 
			PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"cannot retrive at job: %s", r->error);
		*perror = _mcopy_string(mem, buffer);
		ABORT_SPI_SNAP();
		destroy_spi_data(r);
		return NULL;
	}
	if(r->retval == SPI_OK_SELECT)
	{
		if(r->n_rows == 0)
		{
			STOP_SPI_SNAP();
			snprintf(buffer,
				PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"cannot find at job: %d [%s]",
				cron_id,  nodename);
			*perror = _mcopy_string(mem, buffer);
			destroy_spi_data(r);
			return NULL;
		}

		j = init_scheduler_job(mem, NULL, AtJob);
		j->cron_id = cron_id;
		j->node = _mcopy_string(mem, nodename);
		j->dosql = get_textarray_from_spi(mem, r, 0, 2, &j->dosql_n);
		j->executor = get_text_from_spi(mem, r, 0, 3);
		j->start_at = get_timestamp_from_spi(r, 0, 6, 0);
		j->sql_params = get_textarray_from_spi(mem, r, 0, 7, &j->sql_params_n);
		j->depends_on = get_int64array_from_spi(mem, r, 0, 8, &j->depends_on_n);
		j->attempt = get_int64_from_spi(r, 0, 9, 0);
		j->resubmit_limit = get_int64_from_spi(r, 0, 10, 0);

		STOP_SPI_SNAP();

		*perror = NULL;
		destroy_spi_data(r);
		return j;
	}
	snprintf(buffer,
		PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
		"error while retrive at job information: %d", r->retval);
	*perror = _mcopy_string(mem, buffer);
	ABORT_SPI_SNAP();
	destroy_spi_data(r);

	return NULL;
}

job_t *get_cron_job(int cron_id, TimestampTz start_at, char *nodename, char **perror)
{
	job_t *j;
	const char *sql = "select at.last_start_available, cron.same_transaction, cron.do_sql, cron.executor, cron.postpone, cron.max_run_time as time_limit, cron.max_instances, cron.onrollback_statement , cron.next_time_statement from at, cron where start_at = $1 and  at.active and at.cron = cron.id AND cron.node = $2 AND cron.id = $3";
	Oid argtypes[3] = { TIMESTAMPTZOID, TEXTOID, INT4OID};
	Datum args[3];
	char *ts;
	char buffer[PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX];
	spi_response_t *r;
	MemoryContext mem = SchedulerWorkerContext;

	args[0] = TimestampTzGetDatum(start_at);
	args[1] = PointerGetDatum(cstring_to_text(nodename));
	args[2] = Int32GetDatum(cron_id);
	START_SPI_SNAP();
	r = execute_spi_sql_with_args(mem, sql, 3, argtypes, args, NULL);
	if(r->error)
	{
		snprintf(buffer, 
			PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"cannot retrive job: %s", r->error);
		*perror = _mcopy_string(mem, buffer);
		ABORT_SPI_SNAP();
		destroy_spi_data(r);
		return NULL;
	}
	if(r->retval == SPI_OK_SELECT)
	{
		if(r->n_rows == 0)
		{
			STOP_SPI_SNAP();
			ts = make_date_from_timestamp(start_at, false);
			snprintf(buffer,
				PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"cannot find job: %d @ %s [%s]", cron_id, ts, nodename);
			*perror = _mcopy_string(mem, buffer);
			pfree(ts);
			destroy_spi_data(r);
			return NULL;
		}

		j = init_scheduler_job(mem, NULL, CronJob);
		j->start_at = start_at;
		j->node = _mcopy_string(mem, nodename);
		j->same_transaction = get_boolean_from_spi(r, 0, 2, false);
		j->dosql = get_textarray_from_spi(mem, r, 0, 3, &j->dosql_n);
		j->executor = get_text_from_spi(mem, r, 0, 4);
		j->onrollback = get_text_from_spi(mem, r, 0, 8);
		j->next_time_statement = get_text_from_spi(mem, r, 0, 9);
		STOP_SPI_SNAP();
		destroy_spi_data(r);

		*perror = NULL;
		return j;
	}
	snprintf(buffer,
		PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
		"error while retrive job information: %d", r->retval);
	*perror = _mcopy_string(mem, buffer);

	ABORT_SPI_SNAP();
	destroy_spi_data(r);

	return NULL;
}

job_t *get_jobs_to_do(MemoryContext mem, char *nodename, task_type_t type, int *n, int *is_error, int limit)
{
	if(type == CronJob) return _cron_get_jobs_to_do(mem, nodename, n, is_error, limit);
	return _at_get_jobs_to_do(mem, nodename, n, is_error, limit);
}

job_t *get_at_job_for_process(MemoryContext mem, char *nodename, char **error)
{
	job_t *job = NULL;
	Oid argtypes[17] = { TEXTOID };
	Datum values[17];
	bool nulls[17];
	int i;
	char *oldpath;

	const char *get_job_sql = "select * from at_jobs_submitted s where ((not exists ( select * from at_jobs_submitted s2 where s2.id = any(s.depends_on)) AND not exists ( select * from at_jobs_process p where p.id = any(s.depends_on)) AND s.depends_on is NOT NULL and s.at IS NULL) OR ( s.at IS NOT NULL AND  at <= 'now' and (last_start_available is NULL OR last_start_available > 'now'))) and node = $1 and not canceled order by at,  submit_time limit 1 FOR UPDATE SKIP LOCKED";  
	const char *insert_sql = "insert into at_jobs_process values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17)";
	spi_response_t *r;
	spi_response_t *r2;

	oldpath = set_schema(NULL, true); 
	*error = NULL;
	values[0] = CStringGetTextDatum(nodename);


	r = execute_spi_sql_with_args(mem, get_job_sql, 1, 
										argtypes, values, NULL);
	if(r->retval != SPI_OK_SELECT)
	{
		set_schema(oldpath, false);
		pfree(oldpath); 
		*error = _mcopy_string(mem, r->error);
		destroy_spi_data(r);
		return NULL;
	}
	if(r->n_rows == 0)
	{
		set_schema(oldpath, false);
		pfree(oldpath); 
		destroy_spi_data(r);
		return NULL;
	}
	job = MemoryContextAlloc(mem, sizeof(job_t));
	init_scheduler_job(mem, job, AtJob);
	job->dosql = MemoryContextAlloc(mem, sizeof(char *) * 1);

	job->cron_id = get_int_from_spi(r, 0, 1, 0);
	job->start_at = get_timestamp_from_spi(r, 0, 5, 0);
	job->dosql[0] = get_text_from_spi(mem, r, 0, 6);
	job->sql_params = get_textarray_from_spi(mem, r, 0, 7, &job->sql_params_n);
	job->executor = get_text_from_spi(mem, r, 0, 9);
	job->attempt = get_int64_from_spi(r, 0, 12, 0);
	job->resubmit_limit = get_int64_from_spi(r, 0, 13, 0);
	job->timelimit = get_interval_seconds_from_spi(r, 0, 15, 0);
	job->node = _mcopy_string(mem, nodename);

	for(i=0; i < 17; i++)
	{
		argtypes[i] = r->types[i];
		values[i] = r->rows[0][i].dat;
		nulls[i] = r->rows[0][i].null  ? 'n': ' ';
	}
	*error = NULL;
	r2 = execute_spi_sql_with_args(mem, insert_sql, 17, 
										argtypes, values, nulls);
	if(r2->retval != SPI_OK_INSERT)
	{
		set_schema(oldpath, false);
		pfree(oldpath); 
		destroy_job(job, 1);
		*error = _mcopy_string(mem, r2->error);
		destroy_spi_data(r);
		destroy_spi_data(r2);
		return NULL;
	}
	*error = NULL;
	destroy_spi_data(r);
	destroy_spi_data(r2);

	argtypes[0] = INT4OID;
	values[0] = Int32GetDatum(job->cron_id);
	r = execute_spi_sql_with_args(
				mem,
				"delete from at_jobs_submitted where id = $1", 1,
				argtypes, values, NULL);

	set_schema(oldpath, false);
	pfree(oldpath); 

	if(r->retval != SPI_OK_DELETE)
	{
		destroy_job(job, 1);
		*error = _mcopy_string(mem, r->error);
		destroy_spi_data(r);
		return NULL;
	}
	destroy_spi_data(r);
	
	return job;
}

job_t *get_next_at_job_with_lock(char *nodename, char **error)
{
	job_t *job = NULL;
	Oid argtypes[1] = { TEXTOID };
	Datum values[1];
	char *oldpath;
	spi_response_t *r;
	MemoryContext mem = SchedulerWorkerContext;

	const char *get_job_sql = "select id, at, array_append('{}'::text[], do_sql)::text[], params, executor, attempt, resubmit_limit, max_run_time from ONLY at_jobs_submitted s where ((not exists ( select * from ONLY at_jobs_submitted s2 where s2.id = any(s.depends_on)) AND not exists ( select * from ONLY at_jobs_process p where p.id = any(s.depends_on)) AND s.depends_on is NOT NULL and s.at IS NULL) OR ( s.at IS NOT NULL AND  at <= 'now' and (last_start_available is NULL OR last_start_available > 'now'))) and node = $1 and not canceled order by at,  submit_time limit 1 FOR UPDATE SKIP LOCKED";  
	oldpath = set_schema(NULL, true); 
	*error = NULL;
	values[0] = CStringGetTextDatum(nodename);

	r = execute_spi_sql_with_args(mem, get_job_sql, 1, 
										argtypes, values, NULL);
	set_schema(oldpath, false);
	pfree(oldpath); 
	if(r->retval == SPI_OK_SELECT)
	{
		if(r->n_rows > 0)
		{
			job = MemoryContextAlloc(mem, sizeof(job_t));
			init_scheduler_job(mem, job, AtJob);

			job->cron_id = get_int_from_spi(r, 0, 1, 0);
			job->start_at = get_timestamp_from_spi(r, 0, 2, 0);
			job->dosql = get_textarray_from_spi(mem, r, 0, 3, &job->dosql_n);
			job->sql_params = get_textarray_from_spi(mem, r, 0, 4, &job->sql_params_n);
			job->executor = get_text_from_spi(mem, r, 0, 5);
			job->attempt = get_int64_from_spi(r, 0, 6, 0);
			job->resubmit_limit = get_int64_from_spi(r, 0, 7, 0);
			job->timelimit = get_interval_seconds_from_spi(r, 0, 8, 0);
			job->node = _mcopy_string(mem, nodename);
			destroy_spi_data(r);
			return job;
		}
	}
	*error = _mcopy_string(mem, r->error);
	destroy_spi_data(r);
	return NULL;
}

job_t *_at_get_jobs_to_do(MemoryContext mem, char *nodename, int *n, int *is_error, int limit)
{
	job_t *jobs = NULL;
	int ret, got, i;
	Oid argtypes[2] = { TEXTOID, INT4OID };
	Datum values[2];
	const char *get_job_sql = "select id, at, last_start_available, max_run_time,  executor from ONLY at_jobs_submitted s where ((not exists ( select * from ONLY at_jobs_submitted s2 where s2.id = any(s.depends_on)) AND not exists ( select * from ONLY at_jobs_process p where p.id = any(s.depends_on)) AND s.depends_on is NOT NULL and s.at IS NULL) OR ( s.at IS NOT NULL AND  at <= 'now' and (last_start_available is NULL OR last_start_available > 'now'))) and node = $1 and not canceled order by at,  submit_time limit $2";

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
			jobs = MemoryContextAlloc(mem, sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(mem, &(jobs[i]), AtJob);
				jobs[i].cron_id = get_int_from_spi(NULL, i, 1, 0);
				jobs[i].start_at = get_timestamp_from_spi(NULL, i, 2, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(NULL, i, 3, 0);
				jobs[i].timelimit = get_interval_seconds_from_spi(NULL, i, 4, 0);
				jobs[i].node = _mcopy_string(mem, nodename);
				jobs[i].executor = get_text_from_spi(mem, NULL, i, 5);
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

job_t *_cron_get_jobs_to_do(MemoryContext mem, char *nodename, int *n, int *is_error, int limit)
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
			jobs = MemoryContextAlloc(mem, sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(mem, &(jobs[i]), CronJob);
				jobs[i].start_at = get_timestamp_from_spi(NULL, i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(NULL, i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(NULL, i, 3, 0);
				jobs[i].timelimit = get_interval_seconds_from_spi(NULL, i, 4, 0);
				jobs[i].max_instances = get_int_from_spi(NULL, i, 5, 1);
				jobs[i].node = _mcopy_string(mem, nodename);
				jobs[i].executor = get_text_from_spi(mem, NULL, i, 6);
				jobs[i].next_time_statement = get_text_from_spi(mem, NULL, i, 7);
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
	pfree(sql.data);

	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = palloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(CurrentMemoryContext, &(jobs[i]), 2);
				jobs[i].start_at = get_timestamp_from_spi(NULL, i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(NULL, i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(NULL, i, 3, 0);
				jobs[i].node = my_copy_string(nodename);
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
	pfree(sql.data);

	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = palloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(CurrentMemoryContext, &(jobs[i]), 1);
				jobs[i].start_at = get_timestamp_from_spi(NULL, i, 1, 0);
				jobs[i].last_start_avail = get_timestamp_from_spi(NULL, i, 2, 0);
				jobs[i].cron_id = get_int_from_spi(NULL, i, 3, 0);
				jobs[i].node = my_copy_string(nodename);
			}
		}
	}
	else
	{
		*is_error = 1;
	}
	return jobs;
}

job_t *set_job_error(MemoryContext mem, job_t *j, const char *fmt, ...)
{
	va_list arglist;
	char buf[1024];

	va_start(arglist, fmt);
	pvsnprintf(buf, 1024, fmt, arglist);
	va_end(arglist);

	if(j->error) pfree(j->error);
	j->error = _mcopy_string(mem, buf); 

	return j;
}

int move_job_to_log(job_t *j, bool status, bool process)
{
	if(j->type == CronJob) return _cron_move_job_to_log(j, status);
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

int move_at_job_process(int job_id)
{
	const char *sql = "WITH moved_rows AS (DELETE from ONLY at_jobs_submitted a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_process SELECT * FROM moved_rows";
	Datum values[1];	
	Oid argtypes[1] = { INT4OID };
	int ret;
	char *oldpath;
	
	values[0] = Int32GetDatum(job_id);
	oldpath = set_schema(NULL, true);
	ret = SPI_execute_with_args(sql, 1, argtypes, values, NULL, false, 0);
	set_schema(oldpath, false);
	pfree(oldpath);
	return ret > 0 ? 1: ret;
}


int set_at_job_done(job_t *job, char *error, int64 resubmit, char **set_error)
{
	char *this_error = NULL;
	Datum values[21];	
	char  nulls[21];
	Oid argtypes[21] = { INT4OID };
	bool canceled = false;
	int i;
	char *oldpath;
	const char *sql;
	char buff[200];
	int n = 1;
	spi_response_t *r;
	spi_response_t *r2;

	const char *get_sql = "select * from at_jobs_process where id = $1";
	const char *insert_sql = "insert into at_jobs_done values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21)";
	const char *delete_sql = "delete from at_jobs_process where id = $1";
	const char *resubmit_sql = "insert into at_jobs_submitted values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17)";

	oldpath = set_schema(NULL, true);

	values[0] = Int32GetDatum(job->cron_id);
	r = execute_spi_sql_with_args(CurrentMemoryContext, get_sql, 1, argtypes, values, NULL);
	if(r->retval != SPI_OK_SELECT)
	{
		set_schema(oldpath, false);
		pfree(oldpath);
		*set_error = _mcopy_string(CurrentMemoryContext, r->error);
		destroy_spi_data(r);
		return -1;
	}
	if(r->n_rows <= 0)
	{
		snprintf(buff, 200, "Cannot find job to move: %d", job->cron_id);
		*set_error = _mcopy_string(CurrentMemoryContext, buff);
		set_schema(oldpath, false);
		pfree(oldpath);
		destroy_spi_data(r);
		return -1;
	}
	for(i=0; i < 18; i++)
	{
		argtypes[i] = r->types[i];
		values[i] = r->rows[0][i].dat;
		nulls[i] = r->rows[0][i].null ? 'n': ' ';
	}
	argtypes[18] = BOOLOID;
	argtypes[19] = TEXTOID;
	argtypes[20] = TIMESTAMPTZOID;
	nulls[18] = ' ';
	nulls[19] = ' ';
	nulls[20] = ' ';

	canceled = nulls[15] == 'n' ? false: DatumGetBool(values[15]);

	if(resubmit)
	{
		if(canceled)
		{
			this_error = _mcopy_string(CurrentMemoryContext, "job was canceled while processing: cannot resubmit");
			sql = insert_sql;
			n = 21;

			values[18] = BoolGetDatum(false);
			values[19] = CStringGetTextDatum(this_error);
			values[20] = TimestampTzGetDatum(GetCurrentTimestamp());
		}
		else if(job->attempt + 1 < job->resubmit_limit)
		{
			sql = resubmit_sql;
			values[4] = TimestampTzGetDatum(timestamp_add_seconds(0, resubmit));
			values[11] = Int64GetDatum(job->attempt + 1);
			n = 17;
		}
		else
		{
			this_error = _mcopy_string(CurrentMemoryContext, "resubmit limit reached");
			sql = insert_sql;
			n = 21;

			values[18] = BoolGetDatum(false);
			values[19] = CStringGetTextDatum(this_error);
			values[20] = TimestampTzGetDatum(GetCurrentTimestamp());
		}
	}
	else
	{
		n = 21;
		sql = insert_sql;
		if(error)
		{
			values[18] = BoolGetDatum(false);
			values[19] = CStringGetTextDatum(error);
		}
		else
		{
			values[18] = BoolGetDatum(true);
			nulls[19] = 'n'; 
		}
		values[20] = TimestampTzGetDatum(GetCurrentTimestamp());
	}
	r2 = execute_spi_sql_with_args(CurrentMemoryContext, sql, n, argtypes, values, nulls);
	if(this_error) pfree(this_error);
	if(r2->retval != SPI_OK_INSERT)
	{
		set_schema(oldpath, false);
		pfree(oldpath);
		*set_error = _mcopy_string(CurrentMemoryContext, r2->error);
		destroy_spi_data(r);
		destroy_spi_data(r2);
		return -1;
	}
	destroy_spi_data(r);
	destroy_spi_data(r2);

	r = execute_spi_sql_with_args(CurrentMemoryContext, delete_sql, 1, argtypes, values, NULL);
	set_schema(oldpath, false);
	pfree(oldpath);

	if(r->retval != SPI_OK_DELETE)
	{
		*set_error = _mcopy_string(CurrentMemoryContext, r->error);
		destroy_spi_data(r);
		return -1;
	}
	destroy_spi_data(r);

	return 1;
}

int _v1_set_at_job_done(job_t *job, char *error, int64 resubmit)
{
	char *this_error = NULL;
	Datum values[3];	
	char  nulls[3] = { ' ', ' ', ' ' };	
	Oid argtypes[3] = { INT4OID, BOOLOID, TEXTOID };
	int ret;
	char *oldpath;
	const char *sql;
	int n = 3;

	const char *sql_submitted = "WITH moved_rows AS (DELETE from ONLY at_jobs_process a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_done SELECT *, $2 as status, $3 as reason FROM moved_rows";
	/* const char *resubmit_sql = "update ONLY at_jobs_submitted SET attempt = attempt + 1, at = $2 WHERE id = $1"; */
	const char *resubmit_sql = "WITH moved_rows AS (DELETE from ONLY at_jobs_process a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_submitted SELECT id, node, name, comments, $2, do_sql, params, depends_on, executor, owner, last_start_available, attempt +1 , resubmit_limit, postpone, max_run_time, canceled, submit_time FROM moved_rows";

	values[0] = Int32GetDatum(job->cron_id);

	if(resubmit)
	{
		if(job->attempt + 1 < job->resubmit_limit)
		{
			sql = resubmit_sql;
			values[1] = TimestampTzGetDatum(timestamp_add_seconds(0, resubmit));
			argtypes[1] = TIMESTAMPTZOID;
			n = 2;
		}
		else
		{
			this_error = _mcopy_string(NULL, "resubmit limit reached");
			sql = sql_submitted;
			values[1] = BoolGetDatum(false);
			values[2] = CStringGetTextDatum(this_error);
		}
	}
	else
	{
		sql = sql_submitted;
		if(error)
		{
			values[1] = BoolGetDatum(false);
			values[2] = CStringGetTextDatum(error);
		}
		else
		{
			values[1] = BoolGetDatum(true);
			nulls[2] = 'n'; 
		}
	}


	oldpath = set_schema(NULL, true);

	ret = SPI_execute_with_args(sql, n, argtypes, values, nulls, false, 0);


	set_schema(oldpath, false);
	pfree(oldpath);
	if(this_error) pfree(this_error);

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
	if(select_count_with_args("SELECT count(*) FROM at_jobs_process WHERE NOT canceled and id = $1", 1, argtypes, values, NULL))
	{
		ret = SPI_execute_with_args(sql, 2, argtypes, values, NULL, false, 0);
	}
	else
	{
		return -2;
	}

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

void copy_job(MemoryContext mem, job_t *dst, job_t *src)
{
	int i;

	memcpy(dst, src, sizeof(job_t));
	if(src->node) dst->node = _mcopy_string(mem, src->node);
	if(src->executor) dst->executor = _mcopy_string(mem, src->executor);
	if(src->owner) dst->owner = _mcopy_string(mem, src->owner);
	if(src->onrollback) dst->onrollback = _mcopy_string(mem, src->onrollback);
	if(src->next_time_statement) dst->next_time_statement = _mcopy_string(mem, src->next_time_statement);
	if(src->error) dst->error = _mcopy_string(mem, src->error);
	if(src->dosql_n && src->dosql)
	{
		src->dosql = MemoryContextAlloc(mem, sizeof(char *) * src->dosql_n);
		for(i=0; i < src->dosql_n; i++)
		{
			if(src->dosql[i])
				dst->dosql[i] = _mcopy_string(mem, src->dosql[i]);
			else
				dst->dosql[i] = NULL;
		}
	}
	if(src->sql_params_n && src->sql_params)
	{
		src->sql_params = MemoryContextAlloc(mem, sizeof(char *) * src->sql_params_n);
		for(i=0; i < src->sql_params_n; i++)
		{
			if(src->sql_params[i])
				dst->sql_params[i] = _mcopy_string(mem, src->sql_params[i]);
			else
				dst->sql_params[i] = NULL;
		}
	}
	if(src->depends_on_n > 0)
	{
		dst->depends_on = MemoryContextAlloc(mem, sizeof(int64) * src->depends_on_n);
		memcpy(dst->depends_on, src->depends_on, sizeof(int64) * src->depends_on_n);
	}
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

