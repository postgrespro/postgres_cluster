#include <stdlib.h>
#include <stdarg.h>
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

#include "pgstat.h"
#include "fmgr.h"
#include "executor/spi.h"
#include "lib/stringinfo.h"
#include "utils/snapmgr.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"
#include "utils/memutils.h"
#include "utils/guc.h"

#include "pgpro_scheduler.h"
#include "scheduler_executor.h"
#include "scheduler_job.h"
#include "scheduler_spi_utils.h"
#include "memutils.h"
#include "utils/elog.h"

extern volatile sig_atomic_t got_sighup;
extern volatile sig_atomic_t got_sigterm;

static void handle_sigterm(SIGNAL_ARGS);

static void
handle_sigterm(SIGNAL_ARGS)
{
	int save_errno = errno;

	SetLatch(MyLatch);

	if (!proc_exit_inprogress)
	{
		InterruptPending = true;
		ProcDiePending = true;
	}

	errno = save_errno;
}


void executor_worker_main(Datum arg)
{
	schd_executor_share_t *shared;
	dsm_segment *seg;
	job_t *job;
	int i;
	bool success = true;
	executor_error_t EE;
	int ret;
	char *error = NULL;
	bool use_pg_vars = true;
	schd_executor_status_t status;
	/* int rc = 0; */

	EE.n = 0;
	EE.errors = NULL;

	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler_executor");
	seg = dsm_attach(DatumGetInt32(arg));
	if(seg == NULL)
		ereport(ERROR,
			(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			 errmsg("executor unable to map dynamic shared memory segment")));
	shared = dsm_segment_address(seg);

	if(shared->status != SchdExecutorInit)
	{
		ereport(ERROR,
			(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			 errmsg("executor corrupted dynamic shared memory segment")));
	}
	status = shared->status = SchdExecutorWork;
	SetConfigOption("application_name", "pgp-s executor", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "initialize");
	init_worker_mem_ctx("ExecutorMemoryContext");
	BackgroundWorkerInitializeConnection(shared->database, NULL);

	pgstat_report_activity(STATE_RUNNING, "initialize job");
	job = initializeExecutorJob(shared);
	if(!job)
	{
		snprintf(shared->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX, 
											"Cannot retrive job information");
		shared->status = SchdExecutorError;
		delete_worker_mem_ctx();
		dsm_detach(seg);
		proc_exit(0);
	}

	if(set_session_authorization(job->executor, &error) < 0)
	{
		if(error)
		{
			snprintf(shared->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"Cannot set session auth: %s", error);
			pfree(error);
		}
		else
		{
			snprintf(shared->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"Cannot set session auth: unknown error");
		}
		shared->status = SchdExecutorError;
		delete_worker_mem_ctx();
		dsm_detach(seg);
		proc_exit(0);
	}

	pqsignal(SIGTERM, handle_sigterm);
	BackgroundWorkerUnblockSignals();

	pgstat_report_activity(STATE_RUNNING, "process job");
	CHECK_FOR_INTERRUPTS();
	/* rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, 0);
	ResetLatch(MyLatch); */

	if(job->same_transaction)
	{
		START_SPI_SNAP();
	}
	for(i=0; i < job->dosql_n; i++)
	{
		pgstat_report_activity(STATE_RUNNING, job->dosql[i]);
		CHECK_FOR_INTERRUPTS();
		if(!job->same_transaction)
		{
			START_SPI_SNAP();
		}
		ret = execute_spi(job->dosql[i], &error);
		if(ret < 0)
		{
			success = false;
			status = SchdExecutorError;
			if(error)
			{
				push_executor_error(&EE, "error on %d: %s", i+1, error);
				pfree(error);
			}
			else
			{
				push_executor_error(&EE, "error on %d: code: %d", i+1, ret);
			}
			ABORT_SPI_SNAP();
			executor_onrollback(job, &EE);

			break;
		}
		else
		{
			if(!job->same_transaction)
			{
				STOP_SPI_SNAP();
			}
		}
	}
	if(status != SchdExecutorError)
	{
		if(job->same_transaction)
		{
			STOP_SPI_SNAP();
		}
		status = SchdExecutorDone;
	}
	if(job->next_time_statement)
	{
		if(use_pg_vars)  /* may be to define custom var is better */
		{
			set_pg_var(success, &EE);
		}
		shared->next_time = get_next_excution_time(job->next_time_statement, &EE);
	}
	pgstat_report_activity(STATE_RUNNING, "finish job processing");

	if(EE.n > 0)
	{
		set_shared_message(shared, &EE);
	}
	shared->status = status;

	delete_worker_mem_ctx();
	dsm_detach(seg);
	proc_exit(0);
}

int set_session_authorization(char *username, char **error)
{
	Oid types[1] = { TEXTOID };
	Oid useroid;
	Datum values[1];
	bool is_superuser;
	int ret;
	char *sql = "select usesysid, usesuper from pg_catalog.pg_user where usename = $1";
	char buff[1024];

	values[0] = CStringGetTextDatum(username);	
	START_SPI_SNAP();
	ret = execute_spi_sql_with_args(sql, 1, types, values, NULL, error);

	if(ret < 0) return ret;
	if(SPI_processed == 0)
	{
		STOP_SPI_SNAP();
		sprintf(buff, "Cannot find user with name: %s", username);
		*error = _copy_string(buff);

		return -200;
	}
	useroid = get_oid_from_spi(0, 1, 0);
	is_superuser = get_boolean_from_spi(0, 2, false);

	STOP_SPI_SNAP();

	SetSessionAuthorization(useroid, is_superuser);

	return 1;
}

void set_shared_message(schd_executor_share_t *shared, executor_error_t *ee)
{
	int left = PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX;
	char *ptr;
	int i, elen, tlen;

	ptr = shared->message;

	for(i = 0; i < ee->n; i++)
	{
		elen = strlen(ee->errors[i]);
		tlen = left > elen ? elen: left-1;

		if(tlen <= 1)
		{
			ptr[0] = 0;
			break;
		}

		memcpy(ptr, ee->errors[i], tlen);
		ptr += tlen; left -= tlen;
		if(left < 3)
		{
			ptr[0] = 0;
			break;
		}
		if(ee->n == i+1)
		{
			ptr[0] = 0;
		}
		else
		{
			memcpy(ptr, ", ", 2);
			left -= 2;
			ptr += 2;
		}
	}
}

TimestampTz get_next_excution_time(char *sql, executor_error_t *ee)
{
	char *error;
	int ret;
	TimestampTz ts = 0;
	Datum d;
	bool isnull;

	START_SPI_SNAP();
	pgstat_report_activity(STATE_RUNNING, "finish job processing");
	ret = execute_spi(sql, &error);
	if(ret < 0)
	{
		if(error)
		{
			push_executor_error(ee, "next time error: %s", error);
			pfree(error);
		}
		else
		{
			push_executor_error(ee, "next time error: code = %d", ret);
		}
		ABORT_SPI_SNAP();
		return 0;
	}
	if(SPI_processed == 0)
	{	
		push_executor_error(ee, "next time statement returns 0 rows");
	}
	else if(SPI_gettypeid(SPI_tuptable->tupdesc, 1) != TIMESTAMPTZOID)
	{
		push_executor_error(ee, "next time statement column 1 type is not timestamp with timezone");
	}
	else
	{
		d = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc,
								1, &isnull);
		if(isnull)
		{
			push_executor_error(ee, "next time statement row 0 column 1 ihas NULL value");
		}
		else
		{
			ts = DatumGetTimestampTz(d);
		}
	}

	STOP_SPI_SNAP();
	return ts;
}

int executor_onrollback(job_t *job, executor_error_t *ee)
{
	char *error = NULL;
	int ret;

	if(!job->onrollback) return 0;
	pgstat_report_activity(STATE_RUNNING, "execure onrollback");

	START_SPI_SNAP();
	ret = execute_spi(job->onrollback, &error);
	if(ret < 0)
	{
		if(error)
		{
			elog(LOG, "EXECUTOR: onrollback error: %s", error);
			push_executor_error(ee, "onrollback error: %s", error);
			pfree(error);
		}
		else
		{
			push_executor_error(ee, "onrollback error: unknown: %d", ret);
		}
		ABORT_SPI_SNAP();
	}
	else
	{
		STOP_SPI_SNAP();
	}
	return ret;
}

void set_pg_var(bool result, executor_error_t *ee)
{
	char *sql = "select pgv_set_text('pgpro_scheduler', 'transaction', $1)";
	Oid argtypes[1] = { TEXTOID };
	Datum vals[1];
	char *error = NULL;
	int ret;

	pgstat_report_activity(STATE_RUNNING, "set pg_valiable");

	vals[0] = PointerGetDatum(cstring_to_text(result ? "success": "failure"));

	ret = execute_spi_sql_with_args(sql, 1, argtypes, vals, NULL, &error);
	if(ret < 0)
	{
		if(error)
		{
			push_executor_error(ee, "set variable: %s", error);
		}
		else
		{
			push_executor_error(ee, "set variable: code: %d", ret);
		}
	}
}

job_t *initializeExecutorJob(schd_executor_share_t *data)
{
	const char *sql = "select at.last_start_available, cron.same_transaction, cron.do_sql, cron.executor, cron.postpone, cron.max_run_time as time_limit, cron.max_instances, cron.onrollback_statement , cron.next_time_statement from schedule.at at, schedule.cron cron where start_at = $1 and  at.active and at.cron = cron.id AND cron.node = $2 AND cron.id = $3";
	Oid argtypes[3] = { TIMESTAMPTZOID, TEXTOID, INT4OID};
	Datum args[3];
	job_t *J;
	int ret;
	char *error = NULL;
	char *ts;

	args[0] = TimestampTzGetDatum(data->start_at);
	args[1] = PointerGetDatum(cstring_to_text(data->nodename));
	args[2] = Int32GetDatum(data->cron_id);

	START_SPI_SNAP();
	ret = execute_spi_sql_with_args(sql, 3, argtypes, args, NULL, &error);

	if(error)
	{
		snprintf(data->message,
			PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"cannot retrive job: %s", error);
		elog(LOG, "EXECUTOR: %s", data->message);
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
			ts = make_date_from_timestamp(data->start_at);
			snprintf(data->message,
				PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
				"cannot find job: %d @ %s [%s]",
				data->cron_id, ts, data->nodename);
			elog(LOG, "EXECUTOR: %s", data->message);
			pfree(ts);
			return NULL;
		}
		J = worker_alloc(sizeof(job_t));

		J->cron_id = data->cron_id;
		J->start_at = data->start_at;
		J->node = _copy_string(data->nodename);
		J->same_transaction = get_boolean_from_spi(0, 2, false);
		J->dosql = get_textarray_from_spi(0, 3, &J->dosql_n);
		J->executor = get_text_from_spi(0, 4);
		J->onrollback = get_text_from_spi(0, 8);
		J->next_time_statement = get_text_from_spi(0, 9);

		STOP_SPI_SNAP();

		return J;
	}
	snprintf(data->message,
		PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
		"error while retrive job information: %d", ret);
	elog(LOG, "EXECUTOR: %s", data->message);

	PopActiveSnapshot();
	AbortCurrentTransaction();
	SPI_finish();
	return NULL;
}

int push_executor_error(executor_error_t *e, char *fmt, ...)
{
	va_list arglist;
	char buf[1024];
	int len;

	va_start(arglist, fmt);
	len = vsnprintf(buf, 1024, fmt, arglist);
	va_end(arglist);

	if(e->n == 0)
	{
		e->errors = worker_alloc(sizeof(char *));
	}
	else
	{
		e->errors = repalloc(e->errors, sizeof(char *) * (e->n+1));
	}
	e->errors[e->n] = worker_alloc(sizeof(char)*(len + 1));
	memcpy(e->errors[e->n], buf, len+1);

	elog(LOG, "EXECUTOR: %s", e->errors[e->n]);
	e->n++;

	return e->n;
}
