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

#include "pg_config.h"
#include "fmgr.h"
#include "pgstat.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "tcop/utility.h"
#include "lib/stringinfo.h"
#include "access/xact.h"
#include "utils/snapmgr.h"
#include "utils/datetime.h"

#include "char_array.h"
#include "sched_manager_poll.h"
#include "cron_string.h"
#include "pgpro_scheduler.h"
#include "scheduler_manager.h"
#include "memutils.h"


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

volatile sig_atomic_t got_sighup = false;
volatile sig_atomic_t got_sigterm = false;


static char *sched_databases = "";
static char *sched_nodename = "master";
static int sched_max_workers = 2;

extern void
worker_spi_sighup(SIGNAL_ARGS)
{
	int save_errno = errno;

	got_sighup = true;
	SetLatch(MyLatch);

	errno = save_errno;
}

extern void
worker_spi_sigterm(SIGNAL_ARGS)
{
	int save_errno = errno;

	got_sigterm = true;
	SetLatch(MyLatch);

	errno = save_errno;
}

TimestampTz timestamp_add_seconds(TimestampTz to, int add)
{
	if(to == 0) to = GetCurrentTimestamp();
#ifdef HAVE_INT64_TIMESTAMP
	add *= USECS_PER_SEC;
#endif
	return add + to;
}

char_array_t *readBasesToCheck(void)
{
	const char *value;
	int value_len = 0;
	int nnames = 0;
	char_array_t *names;
	char_array_t *result;
	char *clean_value;
	int i;
	int cv_len = 0;
	StringInfoData sql;
	int ret;
	int start_pos = 0;
	int processed;
	char *ptr = NULL;


	pgstat_report_activity(STATE_RUNNING, "read configuration");
	result = makeCharArray();

	value = GetConfigOption("schedule.database", 1, 0);
	if(!value || strlen(value) == 0)
	{
		return result;
	}
	value_len = strlen(value);
	clean_value = worker_alloc(sizeof(char)*(value_len+1));
	nnames = 1;
	for(i=0; i < value_len; i++)
	{
		if(value[i] != ' ')
		{
			if(value[i] == ',')
			{
				nnames++;
				clean_value[cv_len++] = 0;
			}
			else
			{
				clean_value[cv_len++] = value[i];
			}
		}
	}
	clean_value[cv_len] = 0;
	if(cv_len == 0 || nnames == cv_len)
	{
		pfree(clean_value);
		return result;
	}
	elog(LOG, "clean value: %s [%d,%d]", clean_value, cv_len, nnames);
	names = makeCharArray();
	for(i=0; i < cv_len + 1; i++)
	{
		if(clean_value[i] == 0)
		{
			elog(LOG, "start position: %d", start_pos);
			ptr = clean_value + start_pos;
			if(strlen(ptr)) pushCharArray(names, ptr);
			start_pos = i + 1;
		}
	}
	pfree(clean_value);
	if(names->n == 0)
	{
		return result;
	}

	initStringInfo(&sql);
	appendStringInfo(&sql, "select datname from pg_database where datname in (");
	for(i=0; i < names->n; i++)
	{
		appendStringInfo(&sql, "'%s'", names->data[i]);
		if(i + 1  != names->n) appendStringInfo(&sql, ",");
	} 
	appendStringInfo(&sql, ")");
	elog(LOG, "SQL: %s", sql.data);
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());

	ret = SPI_execute(sql.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		SPI_finish();
		PopActiveSnapshot();
		CommitTransactionCommand();
		elog(FATAL, "Cannot get databases list: error code %d", ret);
	}
	destroyCharArray(names);
	processed  = SPI_processed;
	if(processed == 0)
	{
		SPI_finish();
		PopActiveSnapshot();
		CommitTransactionCommand();
		return result;
	}
	for(i=0; i < processed; i++)
	{
		clean_value = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
		pushCharArray(result, clean_value);
	}
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	sortCharArray(result);
	return result;
}

void parent_scheduler_main(Datum arg)
{
	int rc = 0, i;
	char_array_t *names;
	schd_managers_poll_t *poll;
	schd_manager_share_t *shared;

	init_worker_mem_ctx("Parent scheduler context");

	/*CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");*/
	SetConfigOption("application_name", "pgp-s supervisor", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "Initialize");
	pqsignal(SIGHUP, worker_spi_sighup);
	pqsignal(SIGTERM, worker_spi_sigterm);
	BackgroundWorkerUnblockSignals();

	BackgroundWorkerInitializeConnection("postgres", NULL);
	names = readBasesToCheck();
	elog(LOG, "GOT NAMES");
	poll = initSchedulerManagerPool(names);
	destroyCharArray(names);

	set_supervisor_pgstatus(poll);

	while(!got_sigterm)
	{

		if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
		if(got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
			names = readBasesToCheck();
			if(isBaseListChanged(names, poll))
			{
				refreshManagers(names, poll);
				set_supervisor_pgstatus(poll);
			}
			destroyCharArray(names);
		}
		else if(got_sigterm)
		{
			/* TODO STOP ALL MANAGERS */
		}
		else 
		{
			for(i=0; i < poll->n; i++)
			{
				/* toc = shm_toc_attach(PGPRO_SHM_TOC_MAGIC, dsm_segment_address(poll->workers[i]->shared));
				shared = shm_toc_lookup(toc, 0); */
				shared = dsm_segment_address(poll->workers[i]->shared);

				if(shared->setbychild)
				{
				elog(LOG, "got status change from: %s", poll->workers[i]->dbname);
					shared->setbychild = false;
					if(shared->status == SchdManagerConnected)
					{
						poll->workers[i]->connected = true;
					}
					else if(shared->status == SchdManagerQuit)
					{
						removeManagerFromPoll(poll, poll->workers[i]->dbname, 1);
						set_supervisor_pgstatus(poll);
					}
					else
					{
						elog(WARNING, "manager: %s set strange status: %d", poll->workers[i]->dbname, shared->status);
						/* MAYBE kill the creep */
					}
				}
			}
		}
		rc = WaitLatch(MyLatch,
			WL_LATCH_SET | WL_POSTMASTER_DEATH, 0);
		CHECK_FOR_INTERRUPTS();
		ResetLatch(MyLatch);
	}
	stopAllManagers(poll);
	delete_worker_mem_ctx();

	proc_exit(1);
}

void
pg_scheduler_startup(void)
{
	BackgroundWorker worker;

	elog(LOG, "start up");

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = parent_scheduler_main;
	worker.bgw_notify_pid = 0;
	worker.bgw_main_arg = 0;
	strcpy(worker.bgw_name, "pgpro scheduler");

	elog(LOG, "Register WORKER");


	RegisterBackgroundWorker(&worker); 
}

void _PG_init(void)
{
	RequestAddinShmemSpace(1000);
	DefineCustomStringVariable(
		"schedule.database",
		"On which databases scheduler could be run",
		NULL,
		&sched_databases,
		"",
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomStringVariable(
		"schedule.nodename",
		"The name of scheduler node",
		NULL,
		&sched_nodename,
		"master",
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomIntVariable(
		"schedule.max_workers",
		"How much workers can serve scheduler on one database",
		NULL,
		&sched_max_workers,
		2,
		1,
		100,
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	pg_scheduler_startup();
}

PG_FUNCTION_INFO_V1(temp_now);
Datum
temp_now(PG_FUNCTION_ARGS)
{
	TimestampTz ts;
	struct pg_tm info;
	struct pg_tm cp;
	int tz;
	fsec_t fsec;
	const char *tzn;
	long int toff = 0;

	if(!PG_ARGISNULL(0))
	{
		ts = PG_GETARG_TIMESTAMPTZ(0);
	}
	else
	{
		ts = GetCurrentTimestamp();
	}

	timestamp2tm(ts, &tz, &info, &fsec, &tzn, session_timezone );
	info.tm_wday = j2day(date2j(info.tm_year, info.tm_mon, info.tm_mday));

	elog(NOTICE, "WDAY: %d, MON: %d, MDAY: %d, HOUR: %d, MIN: %d, YEAR: %d (%ld)", 
		info.tm_wday, info.tm_mon, info.tm_mday, info.tm_hour, info.tm_min,
		info.tm_year, info.tm_gmtoff);
	elog(NOTICE, "TZP: %d, ZONE: %s", tz, tzn);

	cp.tm_mon = info.tm_mon;
	cp.tm_mday = info.tm_mday;
	cp.tm_hour = info.tm_hour;
	cp.tm_min = info.tm_min;
	cp.tm_year = info.tm_year;
	cp.tm_sec = info.tm_sec;

	toff = DetermineTimeZoneOffset(&cp, session_timezone);
	elog(NOTICE, "Detect: offset = %ld", toff);

	cp.tm_gmtoff = -toff;
	tm2timestamp(&cp, 0, &tz, &ts);


	PG_RETURN_TIMESTAMPTZ(ts);
}

PG_FUNCTION_INFO_V1(cron_string_to_json_text);
Datum
cron_string_to_json_text(PG_FUNCTION_ARGS)
{
	char *source = NULL;
	char *jsonText = NULL;
	text *text_p;
	int len;
	char *error = NULL;
	
	if(PG_ARGISNULL(0))
	{  
		PG_RETURN_NULL();
	}
	source = PG_GETARG_CSTRING(0);
	jsonText = parse_crontab_to_json_text(source);
	
	if(jsonText)
	{
		len = strlen(jsonText);
		text_p = (text *) palloc(sizeof(char)*len + VARHDRSZ);
		memcpy((void *)VARDATA(text_p), jsonText, len);
		SET_VARSIZE(text_p, sizeof(char)*len + VARHDRSZ);
		free(jsonText);
		PG_RETURN_TEXT_P(text_p);
	}
	else
	{
		error = get_cps_error();
		if(error)
		{
			elog(ERROR, "%s (%d)", error, cps_error);
		}
		else
		{
			elog(ERROR, "unknown error: %d", cps_error);
		}
	}
}


