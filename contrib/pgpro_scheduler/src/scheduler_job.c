#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "postgres.h"
#include "catalog/pg_type.h"
#include "scheduler_job.h"
#include "lib/stringinfo.h"
#include "scheduler_spi_utils.h"
#include "utils/timestamp.h"
#include "utils/builtins.h"
#include "memutils.h"

job_t *init_scheduler_job(job_t *j)
{
	if(j == NULL) j = worker_alloc(sizeof(job_t));
	memset(j, 0, sizeof(job_t));
	j->is_active = false;

	return j;
}

job_t *get_jobs_to_do(char *nodename, int *n, int *is_error)
{
	job_t *jobs = NULL;
	int ret, got, i;
	Oid argtypes[1] = { TEXTOID };
	Datum values[1];
	const char *get_job_sql = "select at.start_at, at.last_start_available, at.cron, max_run_time, cron.max_instances, cron.executor, cron.next_time_statement from at at, cron cron where start_at <= 'now' and not at.active and (last_start_available is NULL OR last_start_available > 'now') and at.cron = cron.id AND cron.node = $1 order by at.start_at";

	*is_error = *n = 0;
	START_SPI_SNAP();
	values[0] = CStringGetTextDatum(nodename);
	ret = SPI_execute_with_args(get_job_sql, 1, argtypes, values, NULL, true, 0);
	if(ret == SPI_OK_SELECT)
	{
		got  = SPI_processed;
		if(got > 0)
		{
			*n = got;
			jobs = worker_alloc(sizeof(job_t) * got);
			for(i=0; i < got; i++)
			{
				init_scheduler_job(&(jobs[i]));
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

job_t *get_expired_jobs(char *nodename, int *n, int *is_error)
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
				init_scheduler_job(&(jobs[i]));
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
	vsnprintf(buf, 1024, fmt, arglist);
	va_end(arglist);

	if(j->error) pfree(j->error);
	j->error = _copy_string(buf); 

	return j;
}

int move_job_to_log(job_t *j, bool status)
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

	if(selfdestroy) pfree(j);
}

