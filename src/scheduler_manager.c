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

#define REALLOC_STEP	40

extern volatile sig_atomic_t got_sighup;
extern volatile sig_atomic_t got_sigterm;

int checkSchedulerNamespace(void)
{
	const char *sql = "select count(*) from pg_namespace where nspname = 'schedule'";
	int found  = 0;
	int ret;
	int ntup;
	bool isnull;

	pgstat_report_activity(STATE_RUNNING, "initialize: check namespace");
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());

	ret = SPI_execute(sql, true, 0);
	if(ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		ntup = DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0],
					SPI_tuptable->tupdesc, 1, &isnull)
		);
		if(!isnull && ntup == 1)
		{
			found =  1;
		}
		else if(isnull)
		{
			elog(LOG, "%s: cannot check namespace: count return null",
				MyBgworkerEntry->bgw_name);
		}
		else if(ntup > 1)
		{
			elog(LOG, "%s: cannot check namespace: found %d namespaces",
				MyBgworkerEntry->bgw_name, ntup);
		}
	}
	else if(ret != SPI_OK_SELECT)
	{
		elog(LOG, "%s: cannot check namespace: error code %d",
				MyBgworkerEntry->bgw_name, ret);
	}
	else if(SPI_processed != 1)
	{
		elog(LOG, "%s: cannot check namespace: count return %ud tups",
				MyBgworkerEntry->bgw_name,
				(unsigned)SPI_processed);
	}
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();

	return found;
}

int get_scheduler_maxworkers(void)
{
	const char *opt;

	opt = GetConfigOption("scheduler.max_workers", true, false);
	if(opt == NULL)
	{
		return 2;
	}
	return atoi(opt);
}

char *get_scheduler_nodename(void)
{
	char *ret;
	const char *opt;
	opt = GetConfigOption("scheduler.nodename", true, false);
	if(opt == NULL || strlen(opt) == 0)
	{
		ret = worker_alloc(sizeof(char) * 7);
		memcpy(ret, "master\0", 7);
	}
	else
	{
		ret = worker_alloc(sizeof(char) * (strlen(opt) + 1));
		memcpy(ret, opt, strlen(opt) + 1);
	}

	return ret;
}

scheduler_manager_ctx_t *initialize_scheduler_manager_context(char *dbname)
{
	int i;
	scheduler_manager_ctx_t *ctx;

	ctx = worker_alloc(sizeof(scheduler_manager_ctx_t));
	ctx->slots_len = get_scheduler_maxworkers();
	ctx->free_slots = ctx->slots_len;
	ctx->nodename = get_scheduler_nodename();
	ctx->database = _copy_string(dbname);

	ctx->slots = worker_alloc(sizeof(scheduler_manager_slot_t *) * ctx->slots_len);
	for(i=0; i < ctx->slots_len; i++)
	{
		ctx->slots[i] = NULL;
	}
	ctx->next_at_time = 0;
	ctx->next_checkjob_time = 0;
	ctx->next_expire_time = 0;

	return ctx;
}

void refresh_scheduler_manager_context(scheduler_manager_ctx_t *ctx)
{
	/* TODO  set new nodename , if changed kill all kids workers, change 
	   max-workers resize slots
	   if less then was and all slots are buisy [ ??? ]
	   kill youngest (?)
	*/
}

void destroy_scheduler_manager_context(scheduler_manager_ctx_t *ctx)
{
	int i;

	if(ctx->slots_len)
	{
		if(ctx->free_slots != ctx->slots_len)
		{
			for(i=0; i < ctx->slots_len - ctx->free_slots; i++)
			{
				destroy_job(ctx->slots[i]->job, 1);
				pfree(ctx->slots[i]);
			}
		}
		pfree(ctx->slots);
	}
	if(ctx->nodename) pfree(ctx->nodename);
	if(ctx->database) pfree(ctx->database);

	pfree(ctx); 
}

void scheduler_manager_stop(scheduler_manager_ctx_t *ctx)
{
	pgstat_report_activity(STATE_RUNNING, "stop ececutors");
	/* TODO stop worker but before stop all started kid workers */
}

scheduler_task_t *scheduler_get_active_tasks(scheduler_manager_ctx_t *ctx, int *nt)
{
	scheduler_task_t *tasks = NULL;
	StringInfoData sql;
	TupleDesc tupdesc;
	Datum dat;
	bool is_null;
	int num_id, num_rule, num_pp;
	int ret;
	int processed;
	int i;

	*nt = 0;
	initStringInfo(&sql);
	appendStringInfo(&sql, "select *, EXTRACT(EPOCH from coalesce(postpone, '0'::interval))::int as pp  from schedule.cron where active and not broken and (start_date <= 'now' or start_date is null) and (end_date <= 'now' or end_date is null) and node = '%s'", ctx->nodename);

	pgstat_report_activity(STATE_RUNNING, "select 'at' tasks");

	ret = SPI_execute(sql.data, true, 0);
	pfree(sql.data);

	if(ret == SPI_OK_SELECT)
	{
		if(SPI_processed > 0)
		{
			processed = SPI_processed;
			tupdesc = SPI_tuptable->tupdesc;

			num_id = SPI_fnumber(tupdesc, "id");
			num_rule = SPI_fnumber(tupdesc, "rule");
			num_pp = SPI_fnumber(tupdesc, "postpone");
			tasks = worker_alloc(sizeof(scheduler_task_t) * processed);

			elog(LOG, "Found %d tasks", processed);

			for(i = 0; i < processed; i++)
			{
				tasks[i].id = get_int_from_spi(i, num_id, 0);
				dat = SPI_getbinval(SPI_tuptable->vals[i], tupdesc, num_rule,
						&is_null);
				tasks[i].rule = is_null ? NULL: DatumGetJsonb(dat);
				tasks[i].postpone = get_interval_seconds_from_spi(i, num_pp, 0);
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
		elog(LOG, "%s: cannot get \"at\" tasks: error code %d",
				MyBgworkerEntry->bgw_name, ret);

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
	elog(LOG, "got dates array");
		VN = v->val.binary.data->header & JB_CMASK;
		dates = worker_alloc(sizeof(char *) * VN);
		for(i=0; i < VN; i++)
		{
			ai = getIthJsonbValueFromContainer(v->val.binary.data, i);
			if(ai->type == jbvString && ai->val.string.len >= 16)
			{
				slen = ai->val.string.len > 16 ? 16: ai->val.string.len;
				dates[*num] = worker_alloc(sizeof(char) * 17);
				memcpy(dates[*num], ai->val.string.val, slen);
				dates[*num][16] = 0;
				if(dates[*num][10] == 'T') dates[*num][10] = ' ';
				elog(LOG, " ### %s", dates[*num]);


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

	if(first_time && jsonb_has_key(task->rule, "onstart"))
	{
		*ntimes  = 1;
		nextarray = worker_alloc(sizeof(TimestampTz));
		nextarray[0] = _round_timestamp_to_minute(GetCurrentTimestamp()); 

		return nextarray;
	}

	curr = start;
	nextarray = worker_alloc(sizeof(TimestampTz) * REALLOC_STEP);
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
		}
#ifdef HAVE_INT64_TIMESTAMP
		curr += USECS_PER_MINUTE;
#else
		curr += SECS_PER_MINUTE;
#endif
	}
	elog(LOG, "made: %d", *ntimes);
	for(i=0; i < 5 ; i++) destroy_bit_array(&cron[i], 0);
	if(*ntimes == 0)
	{
		pfree(nextarray);
		return NULL;
	}
	return nextarray;
}

int how_many_instances_on_work(scheduler_manager_ctx_t *ctx, int cron_id)
{
	int i;
	int found = 0;
	int N;

	N = ctx->slots_len - ctx->free_slots;
	if(N == 0) return 0;

	for(i = 0; i < N; i++)
	{
		if(ctx->slots[i]->job->cron_id == cron_id) found++;
	}

	return found;
}

int set_job_on_free_slot(scheduler_manager_ctx_t *ctx, job_t *job)
{
	scheduler_manager_slot_t *item;
	const char *sql = "update schedule.at set started = 'now'::timestamp, active = true where cron = $1 and start_at = $2";
	Datum values[2];
	Oid argtypes[2] = {INT4OID, TIMESTAMPTZOID};
	int ret;

	if(ctx->free_slots == 0)
	{
		return -1;
	}
	values[0] = Int32GetDatum(job->cron_id);
	values[1] = TimestampTzGetDatum(job->start_at);

	ret = SPI_execute_with_args(sql, 2, argtypes, values, NULL, false, 0);
	if(ret == SPI_OK_UPDATE)
	{
		item = worker_alloc(sizeof(scheduler_manager_slot_t));
		item->job = worker_alloc(sizeof(job_t));
		memcpy(item->job, job, sizeof(job_t));

		item->started  = GetCurrentTimestamp();
		item->wait_worker_to_die = false;
		item->stop_it = job->timelimit ?
						timestamp_add_seconds(0, job->timelimit): 0;

		if(launch_executor_worker(ctx, item) == 0)
		{
			pfree(item->job);
			pfree(item);
			return 0;
		}

/*		rrr = rand() % 30;
		elog(LOG, " -- set timeout in %d sec", rrr);
		item->stop_it = timestamp_add_seconds(0, rrr); */

		ctx->slots[ctx->slots_len - (ctx->free_slots--)] = item;
		job->cron_id = -1;  /* job copied to slot - no need to be destroyed */


		return 1;
	}
	return 0;
}

int launch_executor_worker(scheduler_manager_ctx_t *ctx, scheduler_manager_slot_t *item)
{
	BackgroundWorker worker;
	dsm_segment *seg;
	Size segsize;
	schd_executor_share_t *shm_data;
	BgwHandleStatus status;
	MemoryContext old;

	pgstat_report_activity(STATE_RUNNING, "register scheduler executor");

	segsize = (Size)sizeof(schd_executor_share_t);

	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");
	old = MemoryContextSwitchTo(SchedulerWorkerContext);
	seg = dsm_create(segsize, 0);

	item->shared = seg;
	shm_data = dsm_segment_address(item->shared);

	shm_data->status = SchdExecutorInit;
	memcpy(shm_data->database, ctx->database, strlen(ctx->database));
	memcpy(shm_data->nodename, ctx->nodename, strlen(ctx->nodename));
	shm_data->cron_id = item->job->cron_id;
	shm_data->start_at = item->job->start_at;
	shm_data->message[0] = 0;
	shm_data->next_time = 0;

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

	if(!RegisterDynamicBackgroundWorker(&worker, &(item->handler)))
	{
		elog(LOG, "Cannot register executor worker for db: %s, cron: %d",
									shm_data->database, item->job->cron_id);
		dsm_detach(item->shared);
		MemoryContextSwitchTo(old);
		return 0;
	}
	status = WaitForBackgroundWorkerStartup(item->handler, &(item->pid));
	if(status != BGWH_STARTED)
	{
		elog(LOG, "Cannot start executor worker for db: %s, cron: %d, status: %d",
							shm_data->database, item->job->cron_id, status);
		dsm_detach(item->shared);
		MemoryContextSwitchTo(old);
		return 0;
	}
	MemoryContextSwitchTo(old);
	return item->pid;
}

int scheduler_start_jobs(scheduler_manager_ctx_t *ctx)
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

	if(ctx->next_checkjob_time > GetCurrentTimestamp()) return -1;
	if(ctx->free_slots == 0)
	{
		ctx->next_checkjob_time = timestamp_add_seconds(0, 2);
		return -2;
	}

	START_SPI_SNAP();

	jobs = get_jobs_to_do(ctx->nodename, &njobs, &is_error);
	nwaiting = njobs;
	elog(LOG, "*** found %d jobs to start ***", njobs);
	if(is_error)
	{
		ctx->next_checkjob_time = timestamp_add_seconds(0, interval);
		STOP_SPI_SNAP();
		/* TODO rollback transaction ??? */
		elog(LOG, "Error while retrieving jobs");
		return -3;
	}
	if(nwaiting == 0)
	{
		STOP_SPI_SNAP();
		ctx->next_checkjob_time = timestamp_add_seconds(0, interval);

		return 0;
	}

	while(ctx->free_slots && nwaiting)
	{
		N = ctx->free_slots;
		if(N > nwaiting) N = nwaiting; 


		for(i = start_i; i < N + start_i; i++)
		{
			ni = how_many_instances_on_work(ctx, jobs[i].cron_id);
			if(ni >= jobs[i].max_instances)
			{
				set_job_error(&jobs[i], "Max instances limit reached");
				move_job_to_log(&jobs[i], false);
				destroy_job(&jobs[i], 0);
				jobs[i].cron_id = -1;
			}
			else
			{
				if(!set_job_on_free_slot(ctx, &jobs[i]))
				{
					ts = make_date_from_timestamp(jobs[i].start_at);
					set_job_error(&jobs[i], "Cannot set job %d@%s:00 to worker",
											jobs[i].cron_id, ts);
					pfree(ts);
					move_job_to_log(&jobs[i], false);
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
	STOP_SPI_SNAP();
	elog(LOG, "--- slots: %d, free: %d ---", ctx->slots_len, ctx->free_slots);
	for(i = 0; i < njobs; i++)
	{
		if(jobs[i].cron_id != -1) destroy_job(&jobs[i], 0);
	}
	elog(LOG, "+++ free array");
	pfree(jobs);

	elog(LOG, "+++ jobs waiting: %d", nwaiting);

	if(nwaiting > 0)
	{
		interval = 1;
	}
	else
	{
#ifdef HAVE_INT64_TIMESTAMP
		tt = GetCurrentTimestamp()/USECS_PER_SEC;
#else
		tt = GetCurrentTimestamp();
#endif
		interval = 60  - tt % 60 ;
	}

	ctx->next_checkjob_time = timestamp_add_seconds(0, interval);
	return 1;
}

void destroy_slot_item(scheduler_manager_slot_t *item)
{
	destroy_job(item->job, 1);
	dsm_detach(item->shared);
	pfree(item);
}

int scheduler_check_slots(scheduler_manager_ctx_t *ctx)
{
	int i, busy;
	scheduler_rm_item_t *toremove;
	int nremove = 0;
	scheduler_manager_slot_t *item;
	int last;
	bool removeJob;
	pid_t tmppid;
	bool job_status;
	schd_executor_share_t *shm_data;

	if(ctx->free_slots == ctx->slots_len) return 0;
	busy = ctx->slots_len - ctx->free_slots;
	toremove = worker_alloc(sizeof(scheduler_rm_item_t)*busy);

	for(i = 0; i < busy; i++)
	{
		item = ctx->slots[i];
		if(item->wait_worker_to_die)
		{
			toremove[nremove].pos = i;
			toremove[nremove].reason = RmWaitWorker;
			nremove++;
		}
		else if(item->stop_it && item->stop_it < GetCurrentTimestamp())
		{
			toremove[nremove].pos = i;
			toremove[nremove].reason = RmTimeout;
			nremove++;
			elog(LOG, "TIME OUT ON %d", i);
		}
		else
		{
			shm_data = dsm_segment_address(item->shared);
			if(shm_data->status == SchdExecutorDone || shm_data->status == SchdExecutorError)
			{
				toremove[nremove].pos = i;
				toremove[nremove].reason = shm_data->status == SchdExecutorDone ? RmDone: RmError;
				elog(LOG, "Catch %d reason", toremove[nremove].reason);
				nremove++;
			}
		}
	}
	if(nremove)
	{
		for(i=0; i < nremove; i++)
		{
			removeJob = true;
			job_status = false;
			item = ctx->slots[toremove[i].pos];

			if(toremove[i].reason == RmTimeout)  /* TIME OUT */
			{
				set_job_error(item->job, "job timeout");
				elog(LOG, "Terminate bgworker %d", item->pid);
				TerminateBackgroundWorker(item->handler);
				if(GetBackgroundWorkerPid(item->handler, &tmppid) == BGWH_STARTED)
				{
					removeJob = false;
					item->wait_worker_to_die = true;
				}
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
				job_status = true;
			}
			else if(toremove[i].reason == RmError)
			{
				/* to get error from shared && set */
			}
			else
			{
				set_job_error(item->job, "reason: %d", toremove[i].reason);
			}

			if(removeJob)
			{
				START_SPI_SNAP();
				elog(LOG, "move to log %d at %d", item->job->cron_id, toremove[i].pos);
				move_job_to_log(item->job, job_status);
				STOP_SPI_SNAP();

				last  = ctx->slots_len - ctx->free_slots - 1;
				destroy_slot_item(item);

				if(toremove[i].pos != last)
				{
					ctx->slots[toremove[i].pos] = ctx->slots[last];
				}
				ctx->free_slots++;
			}
		}
	}
	pfree(toremove);
	return 1;
}

int scheduler_vanish_expired_jobs(scheduler_manager_ctx_t *ctx)
{ 
	job_t *expired;
	int nexpired  = 0;
	int is_error  = 0;
	int i;
	int ret;
	int move_ret;
	char *ts;

	if(ctx->next_expire_time > GetCurrentTimestamp()) return -1;
	pgstat_report_activity(STATE_RUNNING, "vanish expired tasks");
	elog(LOG, "Start vanish");
	START_SPI_SNAP();
	elog(LOG, "Going to get expired");
	expired = get_expired_jobs(ctx->nodename, &nexpired, &is_error);
	elog(LOG, "Found %d expired", nexpired);

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
			ts = make_date_from_timestamp(expired[i].last_start_avail); 
			set_job_error(&expired[i], "job cron = %d start time (%s:00) expired", expired[i].cron_id, ts);
			elog(LOG, "ERROR: %s", expired[i].error);
			move_ret  = move_job_to_log(&expired[i], 0);
			if(move_ret < 0)
			{
				elog(LOG, "cannot move job %d@%s:00 to log", expired[i].cron_id, ts);
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
	ctx->next_expire_time = timestamp_add_seconds(0, 30);
	pgstat_report_activity(STATE_IDLE, "vanish expired tasks done");

	return ret;
}

int scheduler_make_at_record(scheduler_manager_ctx_t *ctx)
{
	scheduler_task_t *tasks;
	int ntasks = 0, ntimes = 0;
	TimestampTz *next_times;
	int count;
	StringInfoData sql;
	int i, j, r1, r2;
	char **exec_dates;
	int n_exec_dates = 0;
	char *date1;
	char *date2;
	TimestampTz start, stop, tt;
	bool realloced = false;
	Datum values[4];
	char  nulls[4] = { ' ', ' ', ' ', ' ' };
	Oid argtypes[4];
	char *insert_sql = "insert into schedule.at (start_at, last_start_available, node, retry, cron, active) values ($1, $2, $3, 0, $4, false)";
	char *error;

	start = GetCurrentTimestamp();
	stop = timestamp_add_seconds(0, 600);

	if(ctx->next_at_time > GetCurrentTimestamp())
	{
		return -1;
	}
	elog(LOG, "*** make at records");
	START_SPI_SNAP();
	pgstat_report_activity(STATE_RUNNING, "make 'at' tasks");
	tasks = scheduler_get_active_tasks(ctx, &ntasks);
	if(ntasks == 0)
	{
		ctx->next_at_time = timestamp_add_seconds(0, 25);
		STOP_SPI_SNAP();
		return 0;
	}
	pgstat_report_activity(STATE_RUNNING, "calc next runtime");
	elog(LOG, "*** found %d to make", ntasks);

	initStringInfo(&sql);
	for(i = 0; i < ntasks; i++)
	{
		next_times = scheduler_calc_next_task_time(&(tasks[i]),
				GetCurrentTimestamp(), timestamp_add_seconds(0, 600),
				(ctx->next_at_time > 0 ? 0: 1), &ntimes);
elog(LOG, "=== next times  = %d", ntimes);
		exec_dates = get_dates_array_from_rule(&(tasks[i]), &n_exec_dates);
		if(n_exec_dates > 0)
		{
			elog(LOG, "FOUND DATES %d", n_exec_dates);
			date1 = make_date_from_timestamp(start);
			date2 = make_date_from_timestamp(stop);
			elog(LOG, "start - %s, stop - %s", date1, date2);
	
			for(j=0; j < n_exec_dates; j++)
			{
				r1 = strcmp(date1, exec_dates[j]);
				r2 = strcmp(exec_dates[j], date2);
				if((r1 == 0 || r1 == -1) && (r2 == 0 || r2 == -1))
				{
					if(!realloced)
					{
						if(ntimes == 0)
						{
							next_times = worker_alloc(sizeof(TimestampTz)*n_exec_dates);
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
				elog(LOG, "free date on %d", j);
				pfree(exec_dates[j]);
			}
			pfree(date1);
			pfree(date2);
			elog(LOG, "free exec dates");
			pfree(exec_dates);
		}
		if(ntimes > 0)
		{
			elog(LOG, "will check %d records", ntimes);
			for(j=0; j < ntimes; j++)
			{
				argtypes[0] = INT4OID;
				argtypes[1] = TIMESTAMPTZOID;
				values[0] = Int32GetDatum(tasks[i].id);
				values[1] = TimestampTzGetDatum(next_times[j]);

				count = select_count_with_args(
					"select count(start_at) from schedule.at where cron = $1 and start_at = $2",
					2, argtypes, values, NULL);
				resetStringInfo(&sql);
				if(count == 0)
				{
					count = select_count_with_args(
						"select count(start_at) from schedule.log where cron = $1 and start_at = $2",
						2, argtypes, values, nulls);
					resetStringInfo(&sql);
				}
				if(count > 0) continue;

				argtypes[0] = TIMESTAMPTZOID;
				argtypes[1] = TIMESTAMPTZOID;
				argtypes[2] = CSTRINGOID;
				argtypes[3] = INT4OID;

				values[0] = TimestampTzGetDatum(next_times[j]);
				values[2] = CStringGetDatum(ctx->nodename);
				values[3] = Int32GetDatum(tasks[i].id);

				if(tasks[i].postpone > 0)
				{
					values[1] = TimestampTzGetDatum(timestamp_add_seconds(next_times[j], tasks[i].postpone));
				}
				else
				{
					nulls[1] = 'n';
					values[1] = 0;
				}
				execute_spi_sql_with_args(insert_sql, 4, argtypes, values, nulls, &error);
				if(error)
				{
					elog(ERROR, "Cannot insert AT task: %s", error);
				}
				resetStringInfo(&sql);
			}
			elog(LOG, "free times next");
			pfree(next_times);
		}
	}
	STOP_SPI_SNAP();
	pfree(tasks);


	ctx->next_at_time = timestamp_add_seconds(0, 25);
	return ntasks;
}

void manager_worker_main(Datum arg)
{
	char *database;
	int database_len;
	int rc = 0;
	char *aname;
	schd_manager_share_t *shared;
	dsm_segment *seg;
	scheduler_manager_ctx_t *ctx;

	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");
	seg = dsm_attach(DatumGetInt32(arg));
	if(seg == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("unable to map dynamic shared memory segment")));

	shared = dsm_segment_address(seg);

	if(shared->status != SchdManagerInit && !(shared->setbyparent))
	{
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("corrupted dynamic shared memory segment")));
	}
	shared->setbyparent = false;

	SetConfigOption("application_name", "pgp-s manager", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "initialize");

	database_len = strlen(MyBgworkerEntry->bgw_extra);
	if(BGW_EXTRALEN < database_len +1) database_len = BGW_EXTRALEN - 1;
	database  = palloc(sizeof(char) * (database_len+1));
	memcpy(database, MyBgworkerEntry->bgw_extra, database_len);
	database[database_len] = 0;

	aname = palloc(sizeof(char) * ( 16 + database_len + 1 ));
	sprintf(aname, "pgp-s manager [%s]", database);
	SetConfigOption("application_name", aname, PGC_USERSET, PGC_S_SESSION);
	pfree(aname);

	BackgroundWorkerInitializeConnection(database, NULL);

	if(!checkSchedulerNamespace())
	{
		elog(LOG, "cannot start scheduler for %s - there is no namespace", database);
		changeChildBgwState(shared, SchdManagerQuit);
		pfree(database);
		dsm_detach(seg);
		proc_exit(0); 
	}
	SetCurrentStatementStartTimestamp();
	pgstat_report_activity(STATE_RUNNING, "initialize.");

	pqsignal(SIGHUP, worker_spi_sighup);
	pqsignal(SIGTERM, worker_spi_sigterm);
	BackgroundWorkerUnblockSignals();

	pgstat_report_activity(STATE_RUNNING, "initialize context");
	changeChildBgwState(shared, SchdManagerConnected);
	init_worker_mem_ctx("WorkerMemoryContext");
	ctx = initialize_scheduler_manager_context(database);

	while(!got_sigterm)
	{
		if(rc)
		{
			if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
			if(got_sighup)
			{
				got_sighup = false;
				ProcessConfigFile(PGC_SIGHUP);
				refresh_scheduler_manager_context(ctx);
			}
			if(!got_sighup && !got_sigterm)
			{
				if(rc & WL_LATCH_SET)
				{
					scheduler_check_slots(ctx);
				}
				else if(rc & WL_TIMEOUT)
				{
					scheduler_make_at_record(ctx);
					scheduler_vanish_expired_jobs(ctx);
					scheduler_start_jobs(ctx);
					scheduler_check_slots(ctx);
					pgstat_report_activity(STATE_IDLE, "");
				}
			}
		}
		rc = WaitLatch(MyLatch,
			WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, 1000L);
		ResetLatch(MyLatch);
	}
	delete_worker_mem_ctx();
	/* destroy_scheduler_manager_context(ctx); -  no need any more */
	changeChildBgwState(shared, SchdManagerDie);
	pfree(database);
    dsm_detach(seg);
	proc_exit(0);
}


