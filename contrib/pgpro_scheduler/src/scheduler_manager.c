#include <stdlib.h>
#include "postgres.h"

#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "storage/ipc.h"
#include "storage/dsm.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "storage/shm_toc.h"
#include "catalog/pg_type.h"

#include "pg_config.h"
#include "fmgr.h"
#include "pgstat.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "executor/spi.h"
#include "tcop/utility.h"
#include "lib/stringinfo.h"
#include "access/xact.h"
#include "utils/snapmgr.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"
#include <sys/time.h>

#include "char_array.h"
#include "sched_manager_poll.h"
#include "cron_string.h"
#include "pgpro_scheduler.h"
#include "scheduler_manager.h"
#include "scheduler_spi_utils.h"
#include "scheduler_job.h"
#include "bit_array.h"
#include "utils/memutils.h"
#include "memutils.h"
#include "scheduler_executor.h"

#include "port.h"

#define REALLOC_STEP	40

extern volatile sig_atomic_t got_sighup;
extern volatile sig_atomic_t got_sigterm;

int checkSchedulerNamespace(void)
{
	const char *sql = "select count(*) from pg_catalog.pg_namespace where nspname = $1";
	int count  = 0;
	const char *schema;
	Oid argtypes[1] = { TEXTOID };
	Datum values[1];

	SetCurrentStatementStartTimestamp();
	pgstat_report_activity(STATE_RUNNING, "initialize: check namespace");

	schema = GetConfigOption("schedule.schema", false, true);

	START_SPI_SNAP(); 

	values[0] = CStringGetTextDatum(schema);
	count = select_count_with_args(sql, 1, argtypes, values, NULL);

	if(count == -1)
	{
		STOP_SPI_SNAP();
		elog(ERROR, "Scheduler manager: %s: cannot check namespace: sql error",
													MyBgworkerEntry->bgw_name); 
	}
	else if(count > 1 || count == 0 )
	{
		elog(LOG, "Scheduler manager: %s: cannot check namespace: "
				  "found %d namespaces", MyBgworkerEntry->bgw_name, count);
	}
	else if(count == -2)
	{
		elog(LOG, "Scheduler manager: %s: cannot check namespace: "
				  "count return null", MyBgworkerEntry->bgw_name);
	}
	else if(count != 1)
	{
		STOP_SPI_SNAP();
		elog(ERROR, "Scheduler manager: %s: cannot check namespace: "
					"unknown error %d", MyBgworkerEntry->bgw_name, count); 
	}
	STOP_SPI_SNAP();

	if(count) {
		SetConfigOption("search_path", schema, PGC_USERSET, PGC_S_SESSION);
	}

	return count;
}

int get_scheduler_maxworkers(void)
{
	const char *opt;
	int var;

	opt = GetConfigOption("schedule.max_workers", false, false);
	if(opt == NULL)
	{
		return 2;
	}

	var =  atoi(opt);
	return var;
}

int get_scheduler_at_max_workers(void)
{
	const char *opt;
	int var;

	opt = GetConfigOption("schedule.max_parallel_workers", false, false);
	if(opt == NULL)
	{
		return 2;
	}

	var =  atoi(opt);
	return var;
}

char *get_scheduler_nodename(MemoryContext mem)
{
	const char *opt;
	opt = GetConfigOption("schedule.nodename", true, false);

	return _mcopy_string(mem, (char *)(opt == NULL || strlen(opt) == 0 ? "master": opt));
}

int init_manager_pool(MemoryContext mem, scheduler_manager_pool_t *p, int N)
{
	int i;

	p->len = N;
	p->free = N;
	p->slots = MemoryContextAlloc(mem, sizeof(scheduler_manager_slot_t *) * p->len);
	for(i=0; i < N; i++)
	{
		p->slots[i] = NULL;
	}

	return N;
}

scheduler_manager_ctx_t *initialize_scheduler_manager_context(MemoryContext mem, char *dbname, dsm_segment *seg)
{
	scheduler_manager_ctx_t *ctx;

	ctx = MemoryContextAlloc(mem, sizeof(scheduler_manager_ctx_t));

	ctx->mem_ctx = mem;
	ctx->nodename = get_scheduler_nodename(mem);
	ctx->database = _mcopy_string(mem, dbname);
	ctx->seg = seg; 

	/* initialize cront workers pool */
	init_manager_pool(mem, &(ctx->cron), get_scheduler_maxworkers());
	/* initialize at workers pool */
	init_manager_pool(mem, &(ctx->at), get_scheduler_at_max_workers());

	ctx->next_at_time = 0;
	ctx->next_checkjob_time = 0;
	ctx->next_expire_time = 0;
	ctx->next_check_atjob_time = 0;
	ctx->next_at_expire_time = 0;

	return ctx;
}

int refresh_manager_at_pool(scheduler_manager_ctx_t *ctx, scheduler_manager_pool_t *p, int N)
{
	int i;
	scheduler_manager_slot_t **old;
	schd_executor_share_state_t *shared;


	if(N == p->len) return 0; /* we have nothing to change */

	elog(LOG, "Scheduler Manager %s: Change available AT workers number %d => %d", ctx->database,  p->len, N);
	if(N > p->len)
	{
		pgstat_report_activity(STATE_RUNNING, "extend the number of AT workers");

		old = p->slots;
		p->slots = MemoryContextAlloc(ctx->mem_ctx, sizeof(scheduler_manager_slot_t *) * N);
		for(i=0; i < N; i++)
		{
			p->slots[i] = NULL;
		}
		for(i=0; i < p->len; i++)
		{ 
			p->slots[i] = old[i];
		}
		pfree(old);

		for(i=p->len; i < N; i++)
		{
			start_at_worker(ctx, i);
		}
		p->free = 0;
		p->len = N;
	}
	else if(N < p->len)
	{
		pgstat_report_activity(STATE_RUNNING, "shrink the number of workers");

		old = p->slots;
		p->slots = MemoryContextAlloc(ctx->mem_ctx, sizeof(scheduler_manager_slot_t *) * N);
		for(i=0; i < N; i++)
		{
			p->slots[i] = old[i];
		}
		for(;i < p->len; i++)
		{
			if(old[i])
			{
				shared = dsm_segment_address(old[i]->shared);
				shared->stop_worker = true;
				destroy_slot_item(old[i]);
			}
		}

		p->len = N;
		p->free = 0;
		pfree(old);
	}
	return 0;
}

int refresh_manager_pool(MemoryContext mem, const char *database, const char *name, scheduler_manager_pool_t *p, int N)
{
	int i, busy;
	scheduler_manager_slot_t **old;
	if(N == p->len) return 0; /* we have nothing to change */

	elog(LOG, "Scheduler Manager %s: Change available %s workers number %d => %d", database,  name, p->len, N);
	if(N > p->len)
	{
		pgstat_report_activity(STATE_RUNNING, "extend the number of workers");

		old = p->slots;
		p->slots = MemoryContextAlloc(mem, sizeof(scheduler_manager_slot_t *) * N);
		for(i=0; i < N; i++)
		{
			p->slots[i] = NULL;
		}
		for(i=0; i < p->len; i++)
		{ 
			p->slots[i] = old[i];
		}
		pfree(old);
		p->free += (N - p->len);
		p->len = N;
	}
	else if(N < p->len)
	{
		pgstat_report_activity(STATE_RUNNING, "shrink the number of workers");
		busy = p->len - p->free;
		if(N >= busy)
		{
			p->slots = repalloc(p->slots, sizeof(scheduler_manager_slot_t *) * N);
			p->len = N;
			p->free = N - busy;
		}
		else
		{
			return busy - N;
			/* we need to wait that amount of workers to finish */
		}
	}
	return 0;
}

int refresh_scheduler_manager_context(scheduler_manager_ctx_t *ctx)
{
	int rc = 0;
	int Ncron, Nat;
	int waitCron = 1 ;

	Ncron = get_scheduler_maxworkers();
	Nat = get_scheduler_at_max_workers();

	refresh_manager_at_pool(ctx, &(ctx->at), Nat);
	while(1)
	{
		if(waitCron > 0)
			waitCron = refresh_manager_pool(ctx->mem_ctx, ctx->database, "cron", &(ctx->cron), Ncron);
		if(waitCron == 0) break;

		pgstat_report_activity(STATE_RUNNING, "wait for some workers free slots");
		CHECK_FOR_INTERRUPTS();
		if(rc)
		{
			if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
			if(got_sigterm) proc_exit(0);
			if(got_sighup) return 0;   /* need to refresh it again */
		}
		rc = WaitLatch(MyLatch,
				WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, 500L);
		ResetLatch(MyLatch);
	}

	return 1;
}

void destroy_scheduler_manager_pool(scheduler_manager_pool_t *p)
{
	int i;
	if(p->len)
	{
		if(p->free != p->len)
		{
			for(i=0; i < p->len - p->free; i++)
			{
				destroy_job(p->slots[i]->job, 1);
				pfree(p->slots[i]);
			}
		}
		pfree(p->slots);
	}
}

void destroy_scheduler_manager_context(scheduler_manager_ctx_t *ctx)
{

	destroy_scheduler_manager_pool(&(ctx->cron));
	destroy_scheduler_manager_pool(&(ctx->at));

	if(ctx->nodename) pfree(ctx->nodename);
	if(ctx->database) pfree(ctx->database);

	pfree(ctx); 
}

int scheduler_manager_stop(scheduler_manager_ctx_t *ctx)
{
	int i,pi;
	int onwork = 0;
	int working = 0;
	scheduler_manager_pool_t *pools[2];

	pools[0] = &(ctx->cron);
	pools[1] = &(ctx->at);

	for(pi = 0; pi < 2; pi++)
	{
		pgstat_report_activity(STATE_RUNNING, "stop executors");
		for(i=0; i < pools[pi]->len; i++)
		{
			if(pools[pi]->slots[i])
			{
				elog(LOG, "Schedule manager: terminate bgworker %d",
												pools[pi]->slots[i]->pid);
				TerminateBackgroundWorker(pools[pi]->slots[i]->handler);
			}
		}
		working += onwork;
	}
	return working;
}

scheduler_task_t *scheduler_get_active_tasks(scheduler_manager_ctx_t *ctx, int *nt)
{
	scheduler_task_t *tasks = NULL;
	StringInfoData sql;
	TupleDesc tupdesc;
	Datum dat;
	bool is_null;
	int ret;
	int processed;
	int i;
	char *statement = NULL;

	*nt = 0;
	initStringInfo(&sql);
	appendStringInfo(&sql, "select id, rule, postpone, _next_exec_time, next_time_statement, start_date, end_date from cron where active and not broken and (start_date <= 'now' or start_date is null) and (end_date >= 'now' or end_date is null) and node = '%s'", ctx->nodename);

	pgstat_report_activity(STATE_RUNNING, "select 'at' tasks");

	ret = SPI_execute(sql.data, true, 0);
	pfree(sql.data);

	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed > 0)
		{
			processed = SPI_processed;
			tupdesc = SPI_tuptable->tupdesc;

			tasks = palloc(sizeof(scheduler_task_t) * processed);

			for(i = 0; i < processed; i++)
			{
				tasks[i].id = get_int_from_spi(NULL, i, 1, 0);
				dat = SPI_getbinval(SPI_tuptable->vals[i], tupdesc, 2,
						&is_null);
				tasks[i].rule = is_null ? NULL: DatumGetJsonb(dat);
				tasks[i].postpone = get_interval_seconds_from_spi(NULL, i, 3, 0);
				tasks[i].next = get_timestamp_from_spi(NULL, i, 4, 0);
				statement = get_text_from_spi(CurrentMemoryContext, NULL, i, 5);
				if(statement)
				{
					tasks[i].has_next_time_statement = true;
					pfree(statement);
					statement = NULL;
				}
				else
				{
					tasks[i].has_next_time_statement = false;
				}
				tasks[i].date1 = get_timestamp_from_spi(NULL, i, 6, 0);
				tasks[i].date2 = get_timestamp_from_spi(NULL, i, 7, 0);
			}
			*nt = processed;
		}
		else
		{
			return tasks;
		}
	}
	else if(ret != SPI_OK_SELECT)
	{
		elog(LOG, "Scheduler manager %s: cannot get \"at\" tasks: error code %d",
				ctx->database, ret);

        scheduler_manager_stop(ctx);
		return NULL;
	}

	return tasks;
}

bool jsonb_has_key(Jsonb *J, const char *name)
{
	JsonbValue  kval;
	JsonbValue *v = NULL;

	kval.type = jbvString;
	kval.val.string.val = (char *)name;
	kval.val.string.len = strlen(name);

	v = findJsonbValueFromContainer(&J->root, JB_FOBJECT | JB_FARRAY, &kval);

	return v != NULL ? true: false;
}

void fill_cron_array_from_rule(Jsonb *J, const char *name, bit_array_t *ce, int len, int start)
{
	JsonbValue  kval;
	JsonbValue *v = NULL;
	int i;
	JsonbValue *ai;
	int tval;
	int VN;

	init_bit_array(ce, len);

	kval.type = jbvString;
	kval.val.string.val = (char *)name;
	kval.val.string.len = strlen(name);

	v = findJsonbValueFromContainer(&J->root, JB_FOBJECT, &kval);
	if(v &&
		(v->type == jbvArray || (v->type == jbvBinary &&
								 v->val.binary.data->header & JB_FARRAY)
		)
	)
	{
		if(v->type == jbvArray)	
		{
			VN = v->val.array.nElems;
		}
		else
		{
			VN = v->val.binary.data->header & JB_CMASK;
		}
		for(i=0; i < VN; i++)
		{
			ai = v->type == jbvArray ?
				&(v->val.array.elems[i]):
				getIthJsonbValueFromContainer(v->val.binary.data, i);
			tval = get_integer_from_jsonbval(ai, -1);
			if(tval >= 0 ) bit_array_set(ce, tval - start);
		}
	}
}

bit_array_t *convert_rule_to_cron(Jsonb *J, bit_array_t *cron)
{
	fill_cron_array_from_rule(J, "days", &cron[CEO_DAY_POS], CE_DAYS_LEN, 1); 
	fill_cron_array_from_rule(J, "wdays", &cron[CEO_DOW_POS], CE_DOWS_LEN, 0); 
	fill_cron_array_from_rule(J, "hours", &cron[CEO_HRS_POS], CE_HOURS_LEN, 0); 
	fill_cron_array_from_rule(J, "minutes", &cron[CEO_MIN_POS], CE_MINUTES_LEN, 0); 
	fill_cron_array_from_rule(J, "months", &cron[CEO_MON_POS], CE_MONTHS_LEN, 1); 
	return cron;
}

int get_integer_from_jsonbval(JsonbValue *ai, int def)
{
	char buf[50];

	if(ai->type == jbvNumeric)
	{
		return DatumGetInt32(
		   DirectFunctionCall1(numeric_int4, NumericGetDatum(ai->val.numeric)));
	}
	else if(ai->type == jbvString)
	{
		if(ai->val.string.len > 0 && ai->val.string.len < 50)
		{
			memcpy(buf, ai->val.string.val, ai->val.string.len);
			buf[ai->val.string.len] = 0;
			return atoi(buf);
		}
	}
	return def;
}

bool _is_in_rule_array(Jsonb *J, const char *name, int value)
{
	JsonbValue  kval;
	JsonbValue *v = NULL;
	int i;
	int tval;

	kval.type = jbvString;
	kval.val.string.val = (char *)name;
	kval.val.string.len = strlen(name);

	v = findJsonbValueFromContainer(&J->root, JB_FARRAY, &kval);
	if(v == NULL)
	{
		/* this an error in rule structure, should we notice about it */
		return false;
	}
	for(i=0; i < v->val.array.nElems; i++)
	{
		tval = get_integer_from_jsonbval(&(v->val.array.elems[i]), -1);
		if(tval > 0 && tval == value) return true;
	}

	return false;
}

bool is_cron_fit_timestamp(bit_array_t *cron, TimestampTz timestamp)
{
	struct pg_tm info;
	int tz;
	fsec_t fsec;
	const char *tzn;

	timestamp2tm(timestamp, &tz, &info, &fsec, &tzn, NULL ); /* TODO ERROR */
	info.tm_wday = j2day(date2j(info.tm_year, info.tm_mon, info.tm_mday));

	if(bit_array_test(&cron[CEO_DOW_POS], info.tm_wday) && \
	 	bit_array_test(&cron[CEO_MON_POS], info.tm_mon - 1) && \
		bit_array_test(&cron[CEO_DAY_POS], info.tm_mday - 1) && \
		bit_array_test(&cron[CEO_HRS_POS], info.tm_hour) && \
		bit_array_test(&cron[CEO_MIN_POS], info.tm_min) \
	) return true;

	return false;
}

char **get_dates_array_from_rule(scheduler_task_t *task, int *num)
{
	JsonbValue  kval;
	JsonbValue *v = NULL;
	int i;
	JsonbValue *ai;
	int VN;
	char **dates;
	int slen;

	*num = 0;

	kval.type = jbvString;
	kval.val.string.val = "dates";
	kval.val.string.len = 5;

	v = findJsonbValueFromContainer(&task->rule->root, JB_FOBJECT, &kval);
	if(v && v->type == jbvBinary && v->val.binary.data->header & JB_FARRAY)
	{
		VN = v->val.binary.data->header & JB_CMASK;
		dates = palloc(sizeof(char *) * VN);
		for(i=0; i < VN; i++)
		{
			ai = getIthJsonbValueFromContainer(v->val.binary.data, i);
			if(ai->type == jbvString && ai->val.string.len >= 16)
			{
				slen = ai->val.string.len > 16 ? 16: ai->val.string.len;
				dates[*num] = palloc(sizeof(char) * 17);
				memcpy(dates[*num], ai->val.string.val, slen);
				dates[*num][16] = 0;
				if(dates[*num][10] == 'T') dates[*num][10] = ' ';
				/* elog(LOG, " ### %s", dates[*num]); */


				(*num)++;
			}
		}
		if(*num == 0)
		{
			pfree(dates);
			return NULL;
		}
		return dates;
	}

	return NULL;
}

TimestampTz *scheduler_calc_next_task_time(scheduler_task_t *task, TimestampTz start, TimestampTz stop, int first_time, int *ntimes)
{
	TimestampTz *nextarray = NULL;
	TimestampTz curr;
	bit_array_t cron[5];
	int i;

	*ntimes = 0;

	if(task->date1 > 0 && task->date1 > stop) return NULL; 
	if(task->date1 > 0 && task->date1 > start) start = task->date1;
	if(task->date2 > 0 && task->date2 < start) return NULL;
	if(task->date2 > 0 && task->date2 < stop) stop = task->date2;

	if(first_time && jsonb_has_key(task->rule, "onstart"))
	{
		*ntimes  = 1;
		nextarray = palloc(sizeof(TimestampTz));
		nextarray[0] = _round_timestamp_to_minute(GetCurrentTimestamp()); 

		return nextarray;
	}
	if(task->next > 0)
	{
		if(task->next >= start && stop >= task->next)
		{
			*ntimes  = 1;
			nextarray = palloc(sizeof(TimestampTz));
			nextarray[0] = task->next;

			return nextarray;
		}
		return NULL;
	}



/* to avoid to set job on minute has already passed  we add 1 minute */
	curr = start;
#ifdef HAVE_INT64_TIMESTAMP
	curr += USECS_PER_MINUTE;
#else
	curr += SECS_PER_MINUTE;
#endif

	nextarray = palloc(sizeof(TimestampTz) * REALLOC_STEP);
	convert_rule_to_cron(task->rule, cron);

/*	elog(LOG, "minutes: %s", bit_array_string(&cron[CEO_MIN_POS]));
	elog(LOG, "hours: %s", bit_array_string(&cron[CEO_HRS_POS]));
	elog(LOG, "days: %s", bit_array_string(&cron[CEO_DAY_POS]));
	elog(LOG, "months: %s", bit_array_string(&cron[CEO_MON_POS]));
	elog(LOG, "dows: %s", bit_array_string(&cron[CEO_DOW_POS]));  */

	while(curr <= stop)
	{
		if(is_cron_fit_timestamp(cron, curr))
		{
			nextarray[(*ntimes)++] = _round_timestamp_to_minute(curr);
			if(*ntimes % REALLOC_STEP == 0)
			{
				nextarray = repalloc(nextarray, sizeof(TimestampTz) * (*ntimes + REALLOC_STEP));
			}
			if(task->has_next_time_statement) break;
		}
#ifdef HAVE_INT64_TIMESTAMP
		curr += USECS_PER_MINUTE;
#else
		curr += SECS_PER_MINUTE;
#endif
	}
	for(i=0; i < 5 ; i++) destroy_bit_array(&cron[i], 0);
	if(*ntimes == 0)
	{
		pfree(nextarray);
		return NULL;
	}
	return nextarray;
}

int how_many_instances_on_work(scheduler_manager_ctx_t *ctx, job_t *j)
{
	int i;
	int found = 0;
	int N;
	scheduler_manager_pool_t *p;

	p = j->type == CronJob ? &(ctx->cron) : &(ctx->at);

	N = p->len - p->free;
	if(N == 0) return 0;

	for(i = 0; i < N; i++)
	{
		if(p->slots[i]->job->cron_id == j->cron_id) found++;
	}

	return found;
}

int set_at_job_started(job_t *job)
{
	const char *sql = "WITH moved_rows AS (DELETE from ONLY at_jobs_submitted a WHERE a.id = $1 RETURNING a.*) INSERT INTO at_jobs_process SELECT * FROM moved_rows";
	Datum values[1];
	Oid argtypes[1] = {INT4OID};
	int ret;

	values[0] = Int32GetDatum(job->cron_id);
	ret = SPI_execute_with_args(sql, 1, argtypes, values, NULL, false, 0);
	return ret > 0 ? 1: 0;
}

int set_cron_job_started(job_t *job)
{
	const char *sql = "update at set started = 'now'::timestamp with time zone, active = true where cron = $1 and start_at = $2";
	Datum values[2];
	Oid argtypes[2] = {INT4OID, TIMESTAMPTZOID};
	int ret;
	int my_ret;

	values[0] = Int32GetDatum(job->cron_id);
	values[1] = TimestampTzGetDatum(job->start_at);

	ret = SPI_execute_with_args(sql, 2, argtypes, values, NULL, false, 0);
	my_ret = ret == SPI_OK_UPDATE ? 1: 0;

	return my_ret;
}

int set_job_on_free_slot(scheduler_manager_ctx_t *ctx, job_t *job)
{
	scheduler_manager_pool_t *p;
	scheduler_manager_slot_t *item;
	int ret;
	int idx;
	schd_executor_share_t *sdata;
	PGPROC *worker;
	bool started = false;

	p = job->type == CronJob ? &(ctx->cron) : &(ctx->at);
	if(p->free == 0)
	{
		return -1;
	}

	START_SPI_SNAP();

	ret = job->type == CronJob ?
		set_cron_job_started(job): set_at_job_started(job);

	STOP_SPI_SNAP();

	if(ret)
	{
		idx = p->len - p->free;  /* next free slot */
		started = false;

		if(p->slots[idx])
		{
			item = p->slots[idx];
			if(!item->is_free)
			{
				elog(LOG, "Worker on slot %d is not free", idx);
				return 0;
			}
			sdata = dsm_segment_address(item->shared);

			if(sdata->status == SchdExecutorLimitReached)
			{
				destroy_slot_item(item);
				p->slots[idx] = NULL;
			}
			else
			{
				item->job = MemoryContextAlloc(ctx->mem_ctx, sizeof(job_t));
				copy_job(ctx->mem_ctx, item->job, job);
				/* memcpy(item->job, job, sizeof(job_t)); */
				item->started  = GetCurrentTimestamp();
				item->wait_worker_to_die = false;
				item->stop_it = job->timelimit ?
					timestamp_add_seconds(0, job->timelimit): 0;

				init_executor_shared_data(sdata, ctx, item->job);
				worker = BackendPidGetProc(item->pid);
				if(worker)
				{
					item->is_free = false;
					SetLatch(&worker->procLatch);
					started = true;
				}
				else
				{
					destroy_job(item->job, 1);
					return 0;
				}
			}
		}
		if(!started)
		{
			/* need to launch new worker to process job */
			item = init_manager_slot(ctx->mem_ctx); 
			item->job = MemoryContextAlloc(ctx->mem_ctx, sizeof(job_t));
			copy_job(ctx->mem_ctx, item->job, job);
			/* memcpy(item->job, job, sizeof(job_t)); */

			item->started  = item->worker_started = GetCurrentTimestamp();
			item->wait_worker_to_die = false;
			item->stop_it = job->timelimit ?
						timestamp_add_seconds(0, job->timelimit): 0;

			if(launch_executor_worker(ctx, item) == 0)
			{
				destroy_job(item->job, 1);
				pfree(item);
				return 0;
			}
			p->slots[idx] = item;
		}
		p->free--;
		/* job->cron_id = -1;  *//* job copied to slot - no need to be destroyed */

		return 1;
	}
	return 0;
}

scheduler_manager_slot_t *init_manager_slot(MemoryContext mem)
{
	scheduler_manager_slot_t *item = MemoryContextAlloc(mem,
										sizeof(scheduler_manager_slot_t));
	memset(item, 0, sizeof(scheduler_manager_slot_t));

	return item;
}

int launch_executor_worker(scheduler_manager_ctx_t *ctx, scheduler_manager_slot_t *item)
{
	BackgroundWorker worker;
	dsm_segment *seg;
	Size segsize;
	schd_executor_share_t *shm_data;
	BgwHandleStatus status;
	MemoryContext old;
	ResourceOwner prev_owner;

	pgstat_report_activity(STATE_RUNNING, "register scheduler executor");

	segsize = (Size)sizeof(schd_executor_share_t);

	prev_owner = CurrentResourceOwner;
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");
	seg = dsm_create(segsize, 0);

	item->shared = seg;
	item->res_owner = CurrentResourceOwner;

	shm_data = dsm_segment_address(item->shared);

	init_executor_shared_data(shm_data, ctx, item->job);

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
					BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	worker.bgw_main_arg = UInt32GetDatum(dsm_segment_handle(seg));
	sprintf(worker.bgw_library_name, "pgpro_scheduler");
	sprintf(worker.bgw_function_name, "executor_worker_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "scheduler executor %s", shm_data->database);
	worker.bgw_notify_pid = MyProcPid;

	CurrentResourceOwner = prev_owner;

	old = MemoryContextSwitchTo(ctx->mem_ctx);
	if(!RegisterDynamicBackgroundWorker(&worker, &(item->handler)))
	{
		elog(LOG, "Cannot register executor worker for db: %s, cron: %d",
									shm_data->database, item->job->cron_id);
		dsm_detach(item->shared);
		MemoryContextSwitchTo(old);
		return 0;
	}
	MemoryContextSwitchTo(old);
	status = WaitForBackgroundWorkerStartup(item->handler, &(item->pid));
	if(status != BGWH_STARTED)
	{
		elog(LOG, "Cannot start executor worker for db: %s, cron: %d, status: %d",
							shm_data->database, item->job->cron_id, status);
		dsm_detach(item->shared);
		return 0;
	}
	return item->pid;
}

void init_executor_shared_data(schd_executor_share_t *data, scheduler_manager_ctx_t *ctx, job_t *job)
{
	data->status = SchdExecutorInit;
	memcpy(data->database, ctx->database, strlen(ctx->database));
	memcpy(data->nodename, ctx->nodename, strlen(ctx->nodename));
	data->new_job = true;

	if(job)
	{
		memcpy(data->user, job->executor, NAMEDATALEN);
		data->cron_id = job->cron_id;
		data->start_at = job->start_at;
		data->type = job->type;
	}
	else
	{
		data->cron_id = 0;
		data->start_at = 0;
		data->type = 0;
		data->user[0] = 0;
	}

	data->message[0] = 0;
	data->next_time = 0;
	data->set_invalid = false;
	data->set_invalid_reason[0] = 0;
	data->worker_exit = false;
}

int scheduler_start_jobs(scheduler_manager_ctx_t *ctx, task_type_t type)
{
	int interval = 20;
	job_t *jobs;
	int nwaiting = 0;
	int is_error = 0;
	int N = 0;
	int njobs = 0;
	int start_i = 0;
	TimestampTz tt;
	int i, ni;
	char *ts;
	scheduler_manager_pool_t *p;
	TimestampTz *check_time;

	if(type == CronJob)
	{
		p = &(ctx->cron);
		check_time = &(ctx->next_checkjob_time);
	}
	else
	{
		p = &(ctx->at);
		check_time = &(ctx->next_check_atjob_time);
		interval = 2;
	}

	if(*check_time > GetCurrentTimestamp()) return 0;
	if(p->free == 0)
	{
		if(type == CronJob) *check_time = timestamp_add_seconds(0, 1);
		return 1;
	}

	jobs = get_jobs_to_do(CurrentMemoryContext, ctx->nodename, type, &njobs, &is_error, p->free);

	nwaiting = njobs;
	if(is_error)
	{
		*check_time = timestamp_add_seconds(0, interval);
		elog(LOG, "Error while retrieving jobs");
		return 0;
	}
	if(nwaiting == 0)
	{
		*check_time = timestamp_add_seconds(0, interval);

		return 0;
	}

	while(p->free && nwaiting)
	{
		N = p->free;
		if(N > nwaiting) N = nwaiting; 

		for(i = start_i; i < N + start_i; i++)
		{
			ni = type == CronJob ?
				how_many_instances_on_work(ctx, &(jobs[i])): 100000;
			if(type == CronJob && ni >= jobs[i].max_instances)
			{
				set_job_error(ctx->mem_ctx, &jobs[i], "max instances limit reached");
				START_SPI_SNAP();
				move_job_to_log(&jobs[i], false, false);
				STOP_SPI_SNAP();
				destroy_job(&jobs[i], 0);
				jobs[i].cron_id = -1;
			}
			else
			{
				if(set_job_on_free_slot(ctx, &jobs[i]) <= 0)
				{
					if(type == CronJob)
					{
						ts = make_date_from_timestamp(jobs[i].start_at, false);
						set_job_error(ctx->mem_ctx, &jobs[i],
								"Cannot set job %d@%s:00 to worker",
								 			jobs[i].cron_id, ts);
						pfree(ts);
					}
					else
					{
						set_job_error(ctx->mem_ctx, &jobs[i],
								"Cannot set at job %d to worker",
								 			jobs[i].cron_id);
						elog(ERROR, "Cannot set job to free slot type=%d, id=%d", 
									jobs[i].type, jobs[i].cron_id);
					}
					START_SPI_SNAP();
					move_job_to_log(&jobs[i], false, false);
					STOP_SPI_SNAP();
					destroy_job(&jobs[i], 0);
					jobs[i].cron_id = -1;
				}
			}
		}

		if(N < nwaiting)
		{
			start_i += N;
			nwaiting  -= N;
		}
		else
		{
			nwaiting = 0;
		}
	}

	for(i = 0; i < njobs; i++)
	{
		if(jobs[i].cron_id != -1) destroy_job(&jobs[i], 0);
	}
	pfree(jobs);

	if(nwaiting > 0)
	{
		interval = type == CronJob ? 1: 0;
	}
	else
	{
		if(type == CronJob)
		{
#ifdef HAVE_INT64_TIMESTAMP
			tt = GetCurrentTimestamp()/USECS_PER_SEC;
#else
			tt = GetCurrentTimestamp();
#endif
			interval = 60  - tt % 60 ;
		}
		else
		{
			interval = 2;
		}
	}

	*check_time = timestamp_add_seconds(0, interval);
	return nwaiting;
}

void destroy_slot_item(scheduler_manager_slot_t *item)
{
	if(item->job) destroy_job(item->job, 1);
	dsm_detach(item->shared);
	if(item->res_owner) ResourceOwnerDelete(item->res_owner);
	if(item->handler) pfree(item->handler);
	pfree(item);
}

int scheduler_check_slots(scheduler_manager_ctx_t *ctx, scheduler_manager_pool_t *p)
{
	int i, j, busy;
	scheduler_rm_item_t *toremove;
	int nremove = 0;
	scheduler_manager_slot_t *item;
	int last;
	bool removeJob;
	pid_t tmppid;
	bool job_status;
	schd_executor_share_t *shm_data;
	TimestampTz next_time;
	char *next_time_str;
	char *error;

	if(p->free == p->len) return 0;
	busy = p->len - p->free;
	toremove = worker_alloc(sizeof(scheduler_rm_item_t)*busy);

	for(i = 0; i < busy; i++)
	{
		item = p->slots[i];
		if(item->wait_worker_to_die)
		{
			toremove[nremove].pos = i;
			toremove[nremove].reason = RmWaitWorker;
			toremove[nremove].vanish_item = true;
			nremove++;
		}
		else if(item->stop_it && item->stop_it < GetCurrentTimestamp())
		{
			toremove[nremove].pos = i;
			toremove[nremove].reason = RmTimeout;
			toremove[nremove].vanish_item = true;
			nremove++;
		}
		else
		{
			shm_data = dsm_segment_address(item->shared);
			if(shm_data->status == SchdExecutorDone || shm_data->status == SchdExecutorError)
			{
				toremove[nremove].pos = i;
				toremove[nremove].reason = shm_data->status == SchdExecutorDone ? RmDone: RmError;
				toremove[nremove].vanish_item = shm_data->worker_exit;
				nremove++;
			}
			else if(shm_data->status == SchdExecutorResubmit)
			{
				toremove[nremove].pos = i;
				toremove[nremove].reason = RmDoneResubmit;
				toremove[nremove].vanish_item = shm_data->worker_exit;
				nremove++;
			}
			else if(shm_data->status == SchdExecutorLimitReached)
			{
				toremove[nremove].pos = i;
				toremove[nremove].reason = RmFreeSlot;
				toremove[nremove].vanish_item = true;
				nremove++;
			}
		}
	}
	if(nremove)
	{
		_pdebug("do need to remove: %d", nremove);
		for(i=0; i < nremove; i++)
		{
			removeJob = true;
			job_status = false;
			_pdebug("=== remove position: %d", toremove[i].pos);
			item = p->slots[toremove[i].pos];
			_pdebug("=== remove cron_id: %d", item->job->cron_id);

			if(toremove[i].reason == RmTimeout)  /* TIME OUT */
			{
				set_job_error(ctx->mem_ctx, item->job, "job timeout");
				elog(LOG, "Terminate bgworker %d due to job timeout", item->pid);
				TerminateBackgroundWorker(item->handler);
				if(GetBackgroundWorkerPid(item->handler, &tmppid) == BGWH_STARTED)
				{
					removeJob = false;
					item->wait_worker_to_die = true;
				}
			}
			else if(toremove[i].reason == RmDoneResubmit)
			{
				removeJob = true;
				job_status = true;
			}
			else if(toremove[i].reason == RmWaitWorker) /* wait worker to die */
			{
				if(GetBackgroundWorkerPid(item->handler, &tmppid) == BGWH_STARTED)
				{
					removeJob = false;
					item->wait_worker_to_die = true;
				}
			}
			else if(toremove[i].reason == RmDone)
			{
			    shm_data = dsm_segment_address(item->shared);
				job_status = true;
				if(shm_data->message[0] != 0)
				{
					set_job_error(ctx->mem_ctx, item->job, "%s", shm_data->message);
				}
			}
			else if(toremove[i].reason == RmError)
			{
			    shm_data = dsm_segment_address(item->shared);
				if(shm_data->message[0] != 0)
				{
					set_job_error(ctx->mem_ctx, item->job, "%s", shm_data->message);
				}
				else
				{
					set_job_error(ctx->mem_ctx, item->job, "unknown error occured" );
				}
			}
			else if(toremove[i].reason == RmFreeSlot)
			{
				/* Just free slot - worker exited cause it achived max job
				   limit without job execution due to config change
				*/
				removeJob = true;
				job_status = true;
			}
			else
			{
				set_job_error(ctx->mem_ctx, item->job, "reason: %d", toremove[i].reason);
			}

			if(removeJob)
			{
				shm_data = dsm_segment_address(item->shared);
				if(toremove[i].reason != RmFreeSlot)
				{
					START_SPI_SNAP();

					if(shm_data->set_invalid)
					{
						if(item->job->type == CronJob)
						{
							mark_job_broken(ctx, item->job->cron_id,
									shm_data->set_invalid_reason);
						}
						else
						{
							elog(WARNING, "MANAGER %s: attempt to set at job broken",
									ctx->database);
						}
					}
					if(item->job->next_time_statement)
					{
						if(shm_data->next_time > 0)
						{
							next_time = _round_timestamp_to_minute(shm_data->next_time);
							next_time_str = make_date_from_timestamp(next_time, false);
							if(insert_at_record(ctx->nodename, item->job->cron_id, next_time, 0, &error) < 0)
							{
								manager_fatal_error(ctx, 0, "Cannot insert next time at record: %s", error ? error: "unknown error");
							}
							update_cron_texttime(ctx,item->job->cron_id, next_time);
							if(!item->job->error)
							{
								set_job_error(ctx->mem_ctx, item->job, "set next exec time: %s", next_time_str);
								pfree(next_time_str);
							}
						}
					}
					if(toremove[i].reason == RmDoneResubmit)
					{
						if(item->job->type == AtJob)
						{
							if(resubmit_at_job(item->job, shm_data->next_time) == -2)
							{
								set_job_error(ctx->mem_ctx, item->job, "was canceled while processing");
								move_job_to_log(item->job, false, true);
							}
						}
						else
						{
							set_job_error(ctx->mem_ctx, item->job, "cannot resubmit Cron job");
							move_job_to_log(item->job, false, true);
						}
					}
					else
					{
						move_job_to_log(item->job, job_status, true);
					}
					STOP_SPI_SNAP();
				}

				last  = p->len - p->free - 1;
				if(toremove[i].vanish_item)
				{
					destroy_slot_item(item);
				}
				else
				{
					item->is_free = true;
					if(item->handler) pfree(item->handler); /* release worker struct */
				}

				if(toremove[i].pos != last)
				{
					_pdebug("--- move from %d to %d", last, toremove[i].pos);
					p->slots[toremove[i].pos] = p->slots[last];
					if(toremove[i].vanish_item)
					{
						p->slots[last] = NULL;
					}
					else
					{
						p->slots[last] = item;
					}

					for(j=i+1; j < nremove; j++)
					{
						if(toremove[j].pos == last) 
						{
							toremove[j].pos = toremove[i].pos;
							break;
						}
					}
				}
				else
				{
					if(toremove[i].vanish_item) p->slots[last] = NULL;
				}
				p->free++;
				_pdebug("--- free slots: %d", p->free);
			}
		}
		_pdebug("done remove: %d", nremove);
	}
	pfree(toremove);
	return 1;
}

int mark_job_broken(scheduler_manager_ctx_t *ctx, int cron_id, char *reason)
{
	Oid types[2] = { INT4OID, TEXTOID };
	Datum values[2];
	char *sql = "update cron set reason = $2, broken = true where id = $1";
	spi_response_t *r;
	int ret;

	values[0] = Int32GetDatum(cron_id);
	values[1] = CStringGetTextDatum(reason);
	r = execute_spi_sql_with_args(CurrentMemoryContext, sql, 2, types, values, NULL);
	if(r->retval < 0)
	{
		manager_fatal_error(ctx, 0, "Cannot set cron %d broken: %s", cron_id, r->error);
	}
	ret = r->retval;
	destroy_spi_data(r);
	return ret;
}

int update_cron_texttime(scheduler_manager_ctx_t *ctx, int cron_id, TimestampTz next)
{
	Oid types[2] = { INT4OID, TIMESTAMPTZOID };
	Datum values[2];
	bool nulls[2] = { ' ', ' ' };
	int ret;
	char *sql = "update cron set _next_exec_time = $2 where id = $1";
	spi_response_t *r;

	values[0] = Int32GetDatum(cron_id);
	if(next > 0)
	{
		values[1] = TimestampTzGetDatum(next);
	}
	else
	{
		nulls[1] = 'n';
	}
	r = execute_spi_sql_with_args(SchedulerWorkerContext, sql, 2, types, values, nulls);
	ret = r->retval;
	if(ret < 0)
	{
		manager_fatal_error(ctx, 0, "Cannot update cron %d next time: %s", cron_id, r->error);
	}
	destroy_spi_data(r);

	return ret;
}

int scheduler_vanish_expired_jobs(scheduler_manager_ctx_t *ctx, task_type_t type)
{ 
	job_t *expired;
	int nexpired  = 0;
	int is_error  = 0;
	int i;
	int ret;
	int move_ret;
	char *ts;
	bool ts_hires = false;
	TimestampTz *check_time;
	int interval;

	check_time = type == CronJob ? &(ctx->next_expire_time): &(ctx->next_at_expire_time);
	interval = type == CronJob ? 30: 25;

	if(*check_time > GetCurrentTimestamp()) return -1;
	pgstat_report_activity(STATE_RUNNING, "vanish expired tasks");

	START_SPI_SNAP();
	expired = type == CronJob ? 
		get_expired_cron_jobs(ctx->nodename, &nexpired, &is_error):
		get_expired_at_jobs(ctx->nodename, &nexpired, &is_error);
	if(type == AtJob) ts_hires = true;

	if(is_error)
	{
		/* TODO process error */
		ret = -1;
	}
	else if(nexpired > 0)
	{
		ret = nexpired;
		for(i=0; i < nexpired; i++)
		{
			ts = make_date_from_timestamp(expired[i].last_start_avail, ts_hires); 
			if(type == CronJob)
			{
				set_job_error(ctx->mem_ctx, &expired[i],
					"job cron = %d start time (%s:00) expired",
					expired[i].cron_id, ts);
			}
			else
			{
				set_job_error(ctx->mem_ctx, &expired[i], "job expired");
			}

			move_ret  = move_job_to_log(&expired[i], 0, false);
			if(move_ret < 0)
			{
				elog(LOG, "Scheduler manager %s: cannot move %s job %d@%s%s to log",
						ctx->database, (type == CronJob ? "cron": "at"),
						expired[i].cron_id, ts, (ts_hires ? "": ":00"));
				ret--;
			}
			pfree(ts);
			destroy_job(&expired[i], 0);
		}
		pfree(expired);
		if(ret == 0) ret = -2;
	}
	else
	{
		ret = 0;
	}
	STOP_SPI_SNAP();
	*check_time = timestamp_add_seconds(0, interval);
	pgstat_report_activity(STATE_IDLE, "vanish expired tasks done");

	return ret;
}

int insert_at_record(char *nodename, int cron_id, TimestampTz start_at, TimestampTz postpone, char **error)
{
	Datum values[4];
	char  nulls[4] = { ' ', ' ', ' ', ' ' };
	Oid argtypes[4];
	char *insert_sql = "insert into at (start_at, last_start_available, node, retry, cron, active) values ($1, $2, $3, 0, $4, false)";
	char *at_sql = "select count(start_at) from at where cron = $1 and start_at = $2";
	char *log_sql = "select count(start_at) from log where cron = $1 and start_at = $2";
	int count, ret;
	spi_response_t *r;

	argtypes[0] = INT4OID;
	argtypes[1] = TIMESTAMPTZOID;
	values[0] = Int32GetDatum(cron_id);
	values[1] = TimestampTzGetDatum(start_at);

	count = select_count_with_args(at_sql, 2, argtypes, values, NULL);
	if(count == 0) count = select_count_with_args(log_sql, 2, argtypes, values, NULL);
	if(count > 0) return 0;

	argtypes[0] = TIMESTAMPTZOID;
	argtypes[1] = TIMESTAMPTZOID;
	argtypes[2] = CSTRINGOID;
	argtypes[3] = INT4OID;

	values[0] = TimestampTzGetDatum(start_at);
	values[2] = CStringGetDatum(nodename);
	values[3] = Int32GetDatum(cron_id);

	if(postpone > 0)
	{
		values[1] = TimestampTzGetDatum(timestamp_add_seconds(start_at, postpone));
		nulls[1] = ' ';
	}
	else
	{
		nulls[1] = 'n';
		values[1] = 0;
	}

	r = execute_spi_sql_with_args(SchedulerWorkerContext, insert_sql, 4, argtypes, values, nulls);
	
	ret = r->retval;
	if(r->error) *error = my_copy_string(r->error);
	destroy_spi_data(r);

	if(ret < 0) return ret;
	return 1;
}

int scheduler_make_atcron_record(scheduler_manager_ctx_t *ctx)
{
	scheduler_task_t *tasks;
	int ntasks = 0, ntimes = 0;
	TimestampTz *next_times;
	int i, j, r1, r2;
	char **exec_dates = NULL;
	int n_exec_dates = 0;
	char *date1;
	char *date2;
	TimestampTz start, stop, tt;
	bool realloced = false;
	char *error;
	MemoryContext this_ctx;
	MemoryContext old;

	start = GetCurrentTimestamp();
	stop = timestamp_add_seconds(0, 600);

	if(ctx->next_at_time > GetCurrentTimestamp())
	{
		return -1;
	}
	this_ctx = AllocSetContextCreate(TopMemoryContext, "ctx for at create",
				ALLOCSET_DEFAULT_MINSIZE,
				ALLOCSET_DEFAULT_INITSIZE,
				ALLOCSET_DEFAULT_MAXSIZE);
	if(this_ctx == NULL) elog(ERROR, "Cannot create ctx for at create");
	old = MemoryContextSwitchTo(this_ctx);

	START_SPI_SNAP();
	pgstat_report_activity(STATE_RUNNING, "make 'at' cron tasks");
	tasks = scheduler_get_active_tasks(ctx, &ntasks);
	if(ntasks == 0)
	{
		ctx->next_at_time = timestamp_add_seconds(0, 25);
		STOP_SPI_SNAP();
		MemoryContextSwitchTo(old);
		MemoryContextDelete(this_ctx);
		return 0;
	}
	pgstat_report_activity(STATE_RUNNING, "calc next runtime");

	for(i = 0; i < ntasks; i++)
	{
		n_exec_dates = 0;
		ntimes = 0;
		realloced = false;

		next_times = scheduler_calc_next_task_time(&(tasks[i]),
				GetCurrentTimestamp(), timestamp_add_seconds(0, 600),
				(ctx->next_at_time > 0 ? 0: 1), &ntimes);
		if(tasks[i].next == 0)
			exec_dates = get_dates_array_from_rule(&(tasks[i]), &n_exec_dates);
		if(n_exec_dates > 0)
		{
			date1 = make_date_from_timestamp(start, false);
			date2 = make_date_from_timestamp(stop, false);

	
			for(j=0; j < n_exec_dates; j++)
			{
				r1 = strcmp(date1, exec_dates[j]);
				r2 = strcmp(exec_dates[j], date2);
				if(r1 <= 0 && r2 <= 0)
				{
					if(!realloced)
					{
						if(ntimes == 0)
						{
							next_times = palloc(sizeof(TimestampTz)*n_exec_dates);
						}
						else
						{
							next_times = repalloc(next_times, sizeof(TimestampTz)*(ntimes + n_exec_dates));
						}
						realloced = true;
					}
					tt = get_timestamp_from_string(exec_dates[j]);
					next_times[ntimes++] = tt;
				}
				pfree(exec_dates[j]);
			}
			pfree(date1);
			pfree(date2);
			pfree(exec_dates);
		}
		if(ntimes > 0)
		{
			for(j=0; j < ntimes; j++)
			{
				if(insert_at_record(ctx->nodename, tasks[i].id, next_times[j], tasks[i].postpone, &error) < 0)
				{
					manager_fatal_error(ctx, 0, "Cannot insert AT task: %s", error ? error: "unknown error");
				}
			}
			pfree(next_times);
		}
	}
	pfree(tasks);
	STOP_SPI_SNAP();

	ctx->next_at_time = timestamp_add_seconds(0, 25);

	MemoryContextSwitchTo(old);
	MemoryContextDelete(this_ctx);

	return ntasks;
}

void clean_at_table(scheduler_manager_ctx_t *ctx)
{
	spi_response_t *r;
	MemoryContext mem = init_mem_ctx("clean ctx");

	START_SPI_SNAP();
	r = execute_spi(mem, "truncate at");
	if(r->retval < 0)
	{
		manager_fatal_error(ctx, 0, "Cannot clean 'at' table: %s", r->error);
	}
	destroy_spi_data(r);
	r = execute_spi(mem, "update cron set _next_exec_time = NULL where _next_exec_time is not NULL");
	if(r->retval  < 0)
	{
		manager_fatal_error(ctx, 0, "Cannot clean cron _next time: %s",
																	r->error);
	}
	destroy_spi_data(r);
	STOP_SPI_SNAP();
	MemoryContextDelete(mem);
}

bool check_parent_stop_signal(scheduler_manager_ctx_t *ctx, schd_manager_share_t *shared)
{
	if(shared->setbyparent)
	{
		shared->setbyparent = false;
		if(shared->status == SchdManagerStop)
		{
			elog(LOG, "Scheduler manager %s: receive stop signal from supervisor", ctx->database);
			return true;
		}
	}
	return false;
}

void set_slots_stat_report(scheduler_manager_ctx_t *ctx)
{
	char state[128];
	snprintf(state, 128, "slots(busy/free): cron(%d/%d), at(%d/%d)",
			ctx->cron.len - ctx->cron.free, ctx->cron.free,
			ctx->at.len - ctx->at.free, ctx->at.free);

	pgstat_report_activity(STATE_RUNNING, state);
}

int start_at_worker(scheduler_manager_ctx_t *ctx, int pos)
{
	BackgroundWorker worker;
	dsm_segment *seg;
	Size segsize;
	schd_executor_share_state_t *shm_data;
	BgwHandleStatus status;
	scheduler_manager_slot_t *item;
	ResourceOwner prev_owner;
	MemoryContext old;

	prev_owner = CurrentResourceOwner;
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");

	pgstat_report_activity(STATE_RUNNING, "register scheduler at executor");
	segsize = (Size)sizeof(schd_executor_share_state_t);
	seg = dsm_create(segsize, 0);

	item = MemoryContextAlloc(ctx->mem_ctx, sizeof(scheduler_manager_slot_t));
	item->job = NULL;
	item->started  = item->worker_started = GetCurrentTimestamp();
	item->wait_worker_to_die = false;
	item->stop_it = 0;
	item->shared = seg;
	item->res_owner = CurrentResourceOwner;
	shm_data = dsm_segment_address(item->shared);

	memcpy(shm_data->database, ctx->database, strlen(ctx->database));
	memcpy(shm_data->nodename, ctx->nodename, strlen(ctx->nodename));
	shm_data->stop_worker  = false;
	shm_data->status  = SchdExecutorInit;
	shm_data->start_at  = 0;

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
					BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	worker.bgw_main_arg = UInt32GetDatum(dsm_segment_handle(seg));
	sprintf(worker.bgw_library_name, "pgpro_scheduler");
	sprintf(worker.bgw_function_name, "at_executor_worker_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "scheduler at-executor %s", shm_data->database);
	worker.bgw_notify_pid = MyProcPid;

	CurrentResourceOwner = prev_owner;

	old = MemoryContextSwitchTo(ctx->mem_ctx);
	if(!RegisterDynamicBackgroundWorker(&worker, &(item->handler)))
	{
		elog(LOG, "Cannot register AT executor worker for db: %s",
									shm_data->database);
		dsm_detach(item->shared);
		pfree(item);
		ctx->at.slots[pos] = NULL;
		MemoryContextSwitchTo(old);
		return 0;
	}
	MemoryContextSwitchTo(old);
	status = WaitForBackgroundWorkerStartup(item->handler, &(item->pid));
	if(status != BGWH_STARTED)
	{
		elog(LOG, "Cannot start AT executor worker for db: %s, status: %d",
							shm_data->database,  status);
		dsm_detach(item->shared);
		pfree(item);
		ctx->at.slots[pos] = NULL;
		return 0;
	}
	ctx->at.slots[pos] = item;

	return item->pid;
}

void start_at_workers(scheduler_manager_ctx_t *ctx, schd_manager_share_t *shared)
{
	int i;

	if(ctx->at.len > 0) 
	{
		for(i=0 ; i < ctx->at.len; i++)
		{
			if(start_at_worker(ctx, i) == 0)
			{
				scheduler_manager_stop(ctx);
				delete_worker_mem_ctx(NULL);
				changeChildBgwState(shared, SchdManagerDie);
				dsm_detach(ctx->seg);
				proc_exit(0);
			}
		}
	}
}

void manager_worker_main(Datum arg)
{
	char database[NAMEDATALEN];
	char buffer[1024];
	int rc = 0;
	schd_manager_share_t *shared;
	dsm_segment *seg;
	scheduler_manager_ctx_t *ctx;
	int wait = 0;
	schd_manager_share_t *parent_shared;
	MemoryContext old = NULL;
	MemoryContext longTerm;


	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");
	seg = dsm_attach(DatumGetInt32(arg));
	if(seg == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("unable to map dynamic shared memory segment")));

	shared = dsm_segment_address(seg);

	if(shared->status != SchdManagerInit && !(shared->setbyparent))
	{
		dsm_detach(seg);
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("corrupted dynamic shared memory segment")));
	}
	init_worker_mem_ctx("manager worker context");
	shared->setbyparent = false;

	SetConfigOption("application_name", "pgp-s manager", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "initialize");

	snprintf(database, NAMEDATALEN, "%s", MyBgworkerEntry->bgw_extra);
	snprintf(buffer, 1024, "pgp-s manager [%s]", database);
	SetConfigOption("application_name", buffer, PGC_USERSET, PGC_S_SESSION);

	BackgroundWorkerInitializeConnection(database, NULL);
	elog(LOG, "Started scheduler manager for '%s'", database);

	if(!checkSchedulerNamespace())
	{
		elog(LOG, "cannot start scheduler for %s - there is no namespace", database);
		changeChildBgwState(shared, SchdManagerQuit);
		dsm_detach(seg);
		delete_worker_mem_ctx(NULL);
		proc_exit(0); 
	}
	SetCurrentStatementStartTimestamp();
	pgstat_report_activity(STATE_RUNNING, "initialize.");

	pqsignal(SIGHUP, worker_spi_sighup);
	pqsignal(SIGTERM, worker_spi_sigterm);
	BackgroundWorkerUnblockSignals();

	parent_shared = dsm_segment_address(seg);
	pgstat_report_activity(STATE_RUNNING, "initialize context");
	changeChildBgwState(shared, SchdManagerConnected);

	longTerm = init_mem_ctx("long term context for slots");
	ctx = initialize_scheduler_manager_context(longTerm, database, seg);

	start_at_workers(ctx, shared);
	clean_at_table(ctx);
	set_slots_stat_report(ctx);
	/* SetConfigOption("enable_seqscan", "off", PGC_USERSET, PGC_S_SESSION); */

	while(!got_sigterm)
	{
		if(!SchedulerWorkerContext)
				init_worker_mem_ctx("manager worker context 2");
		old = switch_to_worker_context();
		if(rc)
		{
			if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
			if(got_sighup)
			{
				got_sighup = false;
				ProcessConfigFile(PGC_SIGHUP);
				reload_db_role_config(database); 
				refresh_scheduler_manager_context(ctx); /* TODO */
				set_slots_stat_report(ctx);
			}
			if(!got_sigterm)
			{
				wait = 0;
				if(check_parent_stop_signal(ctx, parent_shared))  break; 

				/** start cron jobs **/	
				wait += scheduler_start_jobs(ctx, CronJob); 

				/** check cron slots **/	
				scheduler_check_slots(ctx, &(ctx->cron)); 

				scheduler_make_atcron_record(ctx); 
				set_slots_stat_report(ctx); 
				/* if there are any expired jobs to get rid of */

				scheduler_vanish_expired_jobs(ctx, AtJob);
				scheduler_vanish_expired_jobs(ctx, CronJob);
			}
		}

		delete_worker_mem_ctx(old);
		rc = WaitLatch(MyLatch,
			WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, 1500L);
		ResetLatch(MyLatch);
	}
	scheduler_manager_stop(ctx);
	pgstat_report_activity(STATE_RUNNING, "finalize manager");
	changeChildBgwState(shared, SchdManagerDie);
	if(SchedulerWorkerContext) delete_worker_mem_ctx(old);
	dsm_detach(seg);
	pgstat_report_activity(STATE_RUNNING, "drop context");
	MemoryContextDelete(longTerm);
	proc_exit(0);
}

void manager_fatal_error(scheduler_manager_ctx_t *ctx, int ecode, char *message, ...)
{
	va_list arglist;
	char buf[1024];

	scheduler_manager_stop(ctx);
	changeChildBgwState((schd_manager_share_t *)(dsm_segment_address(ctx->seg)), SchdManagerDie);
    dsm_detach(ctx->seg);

	va_start(arglist, message);
	pvsnprintf(buf, 1024, message, arglist);
	va_end(arglist);


	delete_worker_mem_ctx(NULL);
	if(ecode == 0)
	{
		ecode = ERRCODE_INTERNAL_ERROR;
	}

	ereport(ERROR, (errcode(ecode), errmsg("%s", buf)));
}


