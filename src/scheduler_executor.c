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

#include "port.h"

extern volatile sig_atomic_t got_sighup;
extern volatile sig_atomic_t got_sigterm;

static int64 current_job_id = -1;
static int resubmit_current_job = 0;

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
	executor_error_t EE;
	int ret;
	char *error = NULL;
	/* bool use_pg_vars = true; */
	/* bool success = true; */
	schd_executor_status_t status;

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
	shared->message[0] = 0;

	SetConfigOption("application_name", "pgp-s executor", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "initialize");
	init_worker_mem_ctx("ExecutorMemoryContext");
	BackgroundWorkerInitializeConnection(shared->database, NULL);

	pgstat_report_activity(STATE_RUNNING, "initialize job");
	job = initializeExecutorJob(shared);
	if(!job)
	{
		if(shared->message[0] == 0)
			snprintf(shared->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX, 
											"Cannot retrive job information");
		shared->status = SchdExecutorError;
		delete_worker_mem_ctx();
		dsm_detach(seg);
		proc_exit(0);
	}
	current_job_id = job->cron_id;
	pgstat_report_activity(STATE_RUNNING, "job initialized");

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
	SetConfigOption("schedule.transaction_state", "running", PGC_INTERNAL, PGC_S_SESSION);

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
			/* success = false; */
			status = SchdExecutorError;
			if(error)
			{
				push_executor_error(&EE, "error in command #%d: %s",
															i+1, error);
				pfree(error);
			}
			else
			{
				push_executor_error(&EE, "error in command #%d: code: %d",
															i+1, ret);
			}
			ABORT_SPI_SNAP();
			SetConfigOption("schedule.transaction_state", "failure", PGC_INTERNAL, PGC_S_SESSION);
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
		SetConfigOption("schedule.transaction_state", "success", PGC_INTERNAL, PGC_S_SESSION);
	}
	if(job->next_time_statement)
	{
/*		if(use_pg_vars)  
		{
			set_pg_var(success, &EE);
		}
*/
		shared->next_time = get_next_excution_time(job->next_time_statement, &EE);
		if(shared->next_time == 0)
		{
			shared->set_invalid = true;
			sprintf(shared->set_invalid_reason, "unable to execute next time statement");
		}
	}
	current_job_id = -1;
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
	char *sql = "select oid, rolsuper from pg_catalog.pg_roles where rolname = $1";
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
			memcpy(ptr, "; ", 2);
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
	pgstat_report_activity(STATE_RUNNING, "culc next time execution time");
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
			push_executor_error(ee, "set variable error code: %d", ret);
		}
	}
}

job_t *initializeExecutorJob(schd_executor_share_t *data)
{
	job_t *J;
	char *error = NULL;
	const char *schema;
	const char *old_path;

	old_path = GetConfigOption("search_path", false, true);
	schema = GetConfigOption("schedule.schema", false, true);
	SetConfigOption("search_path", schema, PGC_USERSET, PGC_S_SESSION);

	J = data->type == CronJob ?
		get_cron_job(data->cron_id, data->start_at, data->nodename, &error):
		get_at_job(data->cron_id, data->nodename, &error);

	SetConfigOption("search_path", old_path, PGC_USERSET, PGC_S_SESSION);

	if(error)
	{
		snprintf(data->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"%s", error);
		elog(LOG, "EXECUTOR: %s", data->message);
		pfree(error);
		return NULL;
	}
	if(!J)
	{
		snprintf(data->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX,
			"unknown error get job");
		elog(LOG, "EXECUTOR: %s", data->message);
		return NULL;
	}

	return J;
}

int push_executor_error(executor_error_t *e, char *fmt, ...)
{
	va_list arglist;
	char buf[1024];
	int len;

	va_start(arglist, fmt);
	len = pvsnprintf(buf, 1024, fmt, arglist);
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

PG_FUNCTION_INFO_V1(get_self_id);
Datum 
get_self_id(PG_FUNCTION_ARGS)
{
	if(current_job_id == -1)
	{
		elog(ERROR, "There is no active job in progress");	
	}
	PG_RETURN_INT64(current_job_id);
}

PG_FUNCTION_INFO_V1(resubmit);
Datum 
resubmit(PG_FUNCTION_ARGS)
{
	if(current_job_id == -1)
	{
		elog(ERROR, "There is no active job in progress");	
	}
	PG_RETURN_BOOL(true);
}
