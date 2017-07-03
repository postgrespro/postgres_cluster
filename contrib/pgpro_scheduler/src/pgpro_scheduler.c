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
#include "executor/spi.h"
#include "tcop/utility.h"
#include "lib/stringinfo.h"
#include "catalog/pg_type.h"
#include "access/xact.h"
#include "utils/snapmgr.h"
#include "utils/datetime.h"
#include "utils/builtins.h"
#include "catalog/pg_db_role_setting.h"
#include "commands/dbcommands.h"


#include "char_array.h"
#include "sched_manager_poll.h"
#include "cron_string.h"
#include "pgpro_scheduler.h"
#include "scheduler_manager.h"
#include "memutils.h"
#include "scheduler_spi_utils.h"


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

volatile sig_atomic_t got_sighup = false;
volatile sig_atomic_t got_sigterm = false;

/* Custom GUC variables */
char *scheduler_databases = NULL;
char *scheduler_nodename = NULL;
char *scheduler_transaction_state = NULL;
int  scheduler_max_workers = 2;
int  scheduler_max_parallel_workers = 2;
int  scheduler_worker_job_limit = 1;
bool scheduler_service_enabled = false;
char *scheduler_schema = NULL;
/* Custom GUC done */

Oid scheduler_atjob_id_OID = InvalidOid;

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

/** Some utils **/

void reload_db_role_config(char *dbname)
{
	Relation    relsetting;
	Snapshot    snapshot;
	Oid databaseid;

	StartTransactionCommand();
	databaseid = get_database_oid(dbname, false);

	relsetting = heap_open(DbRoleSettingRelationId, AccessShareLock);
	snapshot = RegisterSnapshot(GetCatalogSnapshot(DbRoleSettingRelationId));
	ApplySetting(snapshot, databaseid, InvalidOid, relsetting, PGC_S_DATABASE);
	UnregisterSnapshot(snapshot);
	heap_close(relsetting, AccessShareLock);
	CommitTransactionCommand();
}

TimestampTz timestamp_add_seconds(TimestampTz to, int64 add)
{
	if(to == 0) to = GetCurrentTimestamp();
#ifdef HAVE_INT64_TIMESTAMP
	add *= USECS_PER_SEC;
#endif
	return add + to;
}

int get_integer_from_string(char *s, int start, int len)
{
	char buff[100];

	memcpy(buff, s + start, len);
	buff[len] = 0;
	return atoi(buff);
}

char *make_date_from_timestamp(TimestampTz ts, bool hires)
{
	struct pg_tm dt;
	char *str = worker_alloc(sizeof(char) * 20);
	int tz;
	fsec_t fsec;
	const char *tzn;

	timestamp2tm(ts, &tz, &dt, &fsec, &tzn, NULL ); 
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", dt.tm_year , dt.tm_mon,
			dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
	if(!hires) str[16] = 0;
	return str;
}

TimestampTz get_timestamp_from_string(char *str)
{
    struct pg_tm dt;
    int tz;
    TimestampTz ts;

    memset(&dt, 0, sizeof(struct tm));
    dt.tm_year  = get_integer_from_string(str,  0, 4);
    dt.tm_mon   = get_integer_from_string(str,  5, 2);
    dt.tm_mday  = get_integer_from_string(str,  8, 2);
    dt.tm_hour  = get_integer_from_string(str, 11, 2);
    dt.tm_min   = get_integer_from_string(str, 14, 2);

    tz = DetermineTimeZoneOffset(&dt, session_timezone);

    tm2timestamp(&dt, 0, &tz, &ts);

    return ts;
}

TimestampTz _round_timestamp_to_minute(TimestampTz ts)
{
#ifdef HAVE_INT64_TIMESTAMP
	return ts - ts % USECS_PER_MINUTE;
#else
	return ts - ts % SECS_PER_MINUTE;
#endif
}

bool is_scheduler_enabled(void)
{
	const char *opt;

	opt = GetConfigOption("schedule.enabled", false, true);
	if(memcmp(opt, "on", 2) == 0) return true;
	return false;
}

char *set_schema(const char *name, bool get_old)
{
	char *schema_name = NULL;
	char *current = NULL;
	bool free_name = false;

	if(get_old)
		current = _mcopy_string(NULL, (char *)GetConfigOption("search_path", true, false));
	if(name)
	{
		schema_name = (char *)name;
	}
	else
	{
		schema_name = _mcopy_string(NULL, (char *)GetConfigOption("schedule.schema", true, false));	
		free_name = true;
	}
	SetConfigOption("search_path", schema_name,  PGC_USERSET, PGC_S_SESSION);
	if(free_name) pfree(schema_name);

	return current;
}


/** END of SOME UTILS **/



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

	value = GetConfigOption("schedule.database", true, false);
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
	names = makeCharArray();
	for(i=0; i < cv_len + 1; i++)
	{
		if(clean_value[i] == 0)
		{
			ptr = clean_value + start_pos;
			if(strlen(ptr)) pushCharArray(names, ptr);
			start_pos = i + 1;
		}
	}
	pfree(clean_value);
	if(names->n == 0)
	{
		destroyCharArray(names);
		return result;
	}

	initStringInfo(&sql);
	appendStringInfo(&sql, "select datname from pg_database where datname in (");
	for(i=0; i < names->n; i++)
	{
		appendStringInfo(&sql, "'%s'", names->data[i]);
		if(i + 1  != names->n) appendStringInfo(&sql, ",");
	} 
	destroyCharArray(names);
	appendStringInfo(&sql, ")");

	START_SPI_SNAP();

	ret = SPI_execute(sql.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		STOP_SPI_SNAP();
		elog(ERROR, "cannot select from pg_database");
	}
	processed  = SPI_processed;
	if(processed == 0)
	{
		STOP_SPI_SNAP();
		return result;
	}
	for(i=0; i < processed; i++)
	{
		clean_value = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
		pushCharArray(result, clean_value);
	}
	STOP_SPI_SNAP();
	sortCharArray(result);

	return result;
}

void parent_scheduler_main(Datum arg)
{
	int rc = 0, i;
	char_array_t *names = NULL;
	schd_managers_poll_t *pool;
	schd_manager_share_t *shared;
	bool refresh = false;

	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");

	init_worker_mem_ctx("Parent scheduler context");
	elog(LOG, "Start PostgresPro scheduler."); 

	SetConfigOption("application_name", "pgp-s supervisor", PGC_USERSET, PGC_S_SESSION);
	pgstat_report_activity(STATE_RUNNING, "Initialize");
	pqsignal(SIGHUP, worker_spi_sighup);
	pqsignal(SIGTERM, worker_spi_sigterm);
	BackgroundWorkerUnblockSignals();

	BackgroundWorkerInitializeConnection("postgres", NULL);
	names = readBasesToCheck();
	pool = initSchedulerManagerPool(names);
	destroyCharArray(names);

	set_supervisor_pgstatus(pool);

	while(!got_sigterm)
	{

		if(rc & WL_POSTMASTER_DEATH) proc_exit(1);
		if(got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
			refresh = false;
			names = NULL;
			if(is_scheduler_enabled() != pool->enabled)
			{
				if(pool->enabled)
				{
					pool->enabled = false;
					stopAllManagers(pool);
					set_supervisor_pgstatus(pool);
				}
				else
				{
					refresh = true;
					pool->enabled = true;
					names = readBasesToCheck();
				}
			}
			else if(pool->enabled)
			{
				names = readBasesToCheck();
				if(isBaseListChanged(names, pool)) refresh = true;
				else destroyCharArray(names);
			}

			if(refresh)
			{
				refreshManagers(names, pool);
				set_supervisor_pgstatus(pool);
				destroyCharArray(names);
			}
		}
		else 
		{
			for(i=0; i < pool->n; i++)
			{
				shared = dsm_segment_address(pool->workers[i]->shared);

				if(shared->setbychild)
				{
				/* elog(LOG, "got status change from: %s", pool->workers[i]->dbname); */
					shared->setbychild = false;
					if(shared->status == SchdManagerConnected)
					{
						pool->workers[i]->connected = true;
					}
					else if(shared->status == SchdManagerQuit)
					{
						removeManagerFromPoll(pool, pool->workers[i]->dbname, 1, true);
						set_supervisor_pgstatus(pool);
					}
					else if(shared->status == SchdManagerDie)
					{
						removeManagerFromPoll(pool, pool->workers[i]->dbname, 1, false);
						set_supervisor_pgstatus(pool);
					}
					else
					{
						elog(WARNING, "manager: %s set strange status: %d", pool->workers[i]->dbname, shared->status);
					}
				}
			}
		}
		rc = WaitLatch(MyLatch,
			WL_LATCH_SET | WL_POSTMASTER_DEATH, 0);
		CHECK_FOR_INTERRUPTS();
		ResetLatch(MyLatch);
	}
	stopAllManagers(pool);
	delete_worker_mem_ctx(NULL);

	proc_exit(0);
}

void
pg_scheduler_startup(void)
{
	BackgroundWorker worker;

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = 10;
	worker.bgw_main = NULL;
	worker.bgw_notify_pid = 0;
	worker.bgw_main_arg = Int32GetDatum(0);
	worker.bgw_extra[0] = 0;
	memcpy(worker.bgw_function_name, "parent_scheduler_main", 22);
	memcpy(worker.bgw_library_name, "pgpro_scheduler", 16);
	memcpy(worker.bgw_name, "pgpro scheduler", 16);

	RegisterBackgroundWorker(&worker); 
}

void _PG_init(void)
{
	if(!process_shared_preload_libraries_in_progress)
	{
		elog(ERROR, "pgpro_scheduler module must be initialized by Postmaster. "
					"Put the following line to configuration file: "
					"shared_preload_libraries='pgpro_scheduler'");
	}
	DefineCustomStringVariable(
		"schedule.schema",
		"The name of scheduler schema",
		NULL,
		&scheduler_schema,
		"schedule",
		PGC_POSTMASTER,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomStringVariable(
		"schedule.database",
		"On which databases scheduler could be run",
		NULL,
		&scheduler_databases,
		"",
		PGC_SIGHUP,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomStringVariable(
		"schedule.nodename",
		"The name of scheduler node",
		NULL,
		&scheduler_nodename,
		"master",
		PGC_SIGHUP,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomStringVariable(
		"schedule.transaction_state",
		"State of scheduler executor transaction",
		"If not under scheduler executor process the variable has no mean and has a value = 'undefined', possible values: progress, success, failure",
		&scheduler_transaction_state , 
		"undefined",
		PGC_INTERNAL,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomIntVariable(
		"schedule.max_workers",
		"How much workers can serve scheduled jobs on one database",
		NULL,
		&scheduler_max_workers,
		2,
		1,
		1000,
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomIntVariable(
		"schedule.max_parallel_workers",
		"How much workers can serve at jobs on one database",
		NULL,
		&scheduler_max_parallel_workers,
		2,
		1,
		1000,
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomBoolVariable(
		"schedule.enabled",
		"Enable schedule service",
		NULL,
		&scheduler_service_enabled,
		false,
		PGC_SIGHUP,
		0,
		NULL,
		NULL,
		NULL
	);
	DefineCustomIntVariable(
		"schedule.worker_job_limit",
		"How much job can worker serve before shutdown",
		NULL,
		&scheduler_worker_job_limit,
		1,
		1,
		20000,
		PGC_SUSET,
		0,
		NULL,
		NULL,
		NULL
	);
	pg_scheduler_startup();
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
	PG_RETURN_NULL();
}


