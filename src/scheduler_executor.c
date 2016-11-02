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
#include  "utils/elog.h"

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
	int rc = 0;
	job_t *job;
	int i;

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
	shared->status = SchdExecutorWork;
	SetConfigOption("application_name", "pgp-s executor", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "initialize");
	init_worker_mem_ctx("ExecutorMemoryContext");
	BackgroundWorkerInitializeConnection(shared->database, NULL);

	pgstat_report_activity(STATE_RUNNING, "initialize job");
	job = initializeExecutorJob(shared);
	if(!job)
	{
		shared->status = SchdExecutorError;
		snprintf(shared->message, PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX, 
											"Cannot retrive job information");
		delete_worker_mem_ctx();
		dsm_detach(seg);
		proc_exit(1);
	}

	pqsignal(SIGTERM, handle_sigterm);
	BackgroundWorkerUnblockSignals();

	pgstat_report_activity(STATE_RUNNING, "process job");
	while(!got_sigterm)
	{
		CHECK_FOR_INTERRUPTS();
		if(rc)
		{
			if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
		}
		rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, 0);
		ResetLatch(MyLatch);
		for(i=0; i < job->dosql_n; i++)
		{
			pgstat_report_activity(STATE_RUNNING, job->dosql[i]);
			CHECK_FOR_INTERRUPTS();
			START_SPI_SNAP();

			SPI_execute(job->dosql[i], true, 0);

			STOP_SPI_SNAP();
		}
		shared->status = SchdExecutorDone;
		pgstat_report_activity(STATE_RUNNING, "finish job processing");
		break;
	}

	delete_worker_mem_ctx();
	dsm_detach(seg);
	proc_exit(1);
}

job_t *initializeExecutorJob(schd_executor_share_t *data)
{
	const char *sql = "select at.last_start_available, cron.same_transaction, cron.do_sql, cron.executor, cron.postpone, cron.max_run_time as time_limit, cron.max_instances, cron.onrollback_statement , cron.next_time_statement from schedule.at at, schedule.cron cron where start_at = $1 and  at.active and at.cron = cron.id AND cron.node = $2 AND cron.id = $3";
	Oid argtypes[3] = { TIMESTAMPTZOID, CSTRINGOID, INT4OID};
	Datum args[3];
	job_t *J;
	int ret;

	args[0] = TimestampTzGetDatum(data->start_at);
	args[1] = CStringGetTextDatum(data->nodename);
	args[2] = Int32GetDatum(data->cron_id);

	START_SPI_SNAP();
	PG_TRY();
	{
		ret = SPI_execute_with_args(sql, 3, argtypes, args, NULL, true, 1);
	}
	PG_CATCH();
	{
		elog(LOG, "Executor error while retrive job information, ret: %d", data->cron_id);
		PopActiveSnapshot();
		AbortCurrentTransaction();
		SPI_finish();
		return NULL;
	}
	PG_END_TRY();

	if(ret == SPI_OK_INSERT)
	{
		if(SPI_processed == 0)
		{
			STOP_SPI_SNAP();
			elog(LOG, "Executor cannot find job");
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
	elog(LOG, "Executor error while retrive job information, ret: %d", data->cron_id);
	PopActiveSnapshot();
	AbortCurrentTransaction();
	SPI_finish();
	return NULL;
}
