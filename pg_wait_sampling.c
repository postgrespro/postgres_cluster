/*
 * pg_wait_sampling.c
 *		Track information about wait events.
 *
 * Copyright (c) 2015-2016, Postgres Professional
 *
 * IDENTIFICATION
 *	  contrib/pg_wait_sampling/pg_wait_sampling.c
 */
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/autovacuum.h"
#include "storage/spin.h"
#include "storage/ipc.h"
#include "storage/pg_shmem.h"
#include "storage/procarray.h"
#include "storage/shm_mq.h"
#include "storage/shm_toc.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/guc.h"

#include "pg_wait_sampling.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

/* Global variables */
bool					shmem_initialized = false;

/* Shared memory variables */
shm_toc				   *toc = NULL;
CollectorShmqHeader	   *collector_hdr = NULL;
shm_mq				   *collector_mq = NULL;

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

static PGPROC * search_proc(int backendPid);
static TupleDesc get_history_item_tupledesc();
static HeapTuple get_history_item_tuple(HistoryItem *item, TupleDesc tuple_desc);


/*
 * Estimate amount of shared memory needed.
 */
static Size
pgws_shmem_size(void)
{
	shm_toc_estimator	e;
	Size				size;
	int					nkeys;

	shm_toc_initialize_estimator(&e);

	nkeys = 2;

	shm_toc_estimate_chunk(&e, sizeof(CollectorShmqHeader));
	shm_toc_estimate_chunk(&e, (Size) COLLECTOR_QUEUE_SIZE);

	shm_toc_estimate_keys(&e, nkeys);
	size = shm_toc_estimate(&e);

	return size;
}

static bool
shmem_int_guc_check_hook(int *newval, void **extra, GucSource source)
{
	if (UsedShmemSegAddr == NULL)
		return false;
	return true;
}

/*
 * Distribute shared memory.
 */
static void
pgws_shmem_startup(void)
{
	bool	found;
	Size	segsize = pgws_shmem_size();
	void   *pgws;

	pgws = ShmemInitStruct("pg_wait_sampling", segsize, &found);

	if (!found)
	{
		toc = shm_toc_create(PG_WAIT_SAMPLING_MAGIC, pgws, segsize);

		collector_hdr = shm_toc_allocate(toc, sizeof(CollectorShmqHeader));
		shm_toc_insert(toc, 0, collector_hdr);
		collector_mq = shm_toc_allocate(toc, COLLECTOR_QUEUE_SIZE);
		shm_toc_insert(toc, 1, collector_mq);
	}
	else
	{
		toc = shm_toc_attach(PG_WAIT_SAMPLING_MAGIC, pgws);

		collector_hdr = shm_toc_lookup(toc, 0);
		collector_mq = shm_toc_lookup(toc, 1);
	}

	DefineCustomIntVariable("pg_wait_sampling.history_size",
			"Sets size of waits history.", NULL,
			&collector_hdr->historySize, 5000, 100, INT_MAX,
			PGC_SUSET, 0, shmem_int_guc_check_hook, NULL, NULL);

	DefineCustomIntVariable("pg_wait_sampling.history_period",
			"Sets period of waits history sampling.", NULL,
			&collector_hdr->historyPeriod, 10, 1, INT_MAX,
			PGC_SUSET, 0, shmem_int_guc_check_hook, NULL, NULL);

	DefineCustomIntVariable("pg_wait_sampling.profile_period",
			"Sets period of waits profile sampling.", NULL,
			&collector_hdr->profilePeriod, 10, 1, INT_MAX,
			PGC_SUSET, 0, shmem_int_guc_check_hook, NULL, NULL);

	shmem_initialized = true;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();
}

/*
 * Check shared memory is initialized. Report an error otherwise.
 */
void
check_shmem(void)
{
	if (!shmem_initialized)
	{
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("pg_wait_sampling shared memory wasn't initialized yet")));
	}
}

/*
 * Module load callback
 */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
		return;

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in pgws_shmem_startup().
	 */
	RequestAddinShmemSpace(pgws_shmem_size());

	RegisterWaitsCollector();

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = pgws_shmem_startup;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	shmem_startup_hook = prev_shmem_startup_hook;
}

/*
 * Make a TupleDesc describing single item of waits history.
 */
static TupleDesc
get_history_item_tupledesc()
{
	TupleDesc	tupdesc;

	tupdesc = CreateTemplateTupleDesc(4, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "pid",
					   INT4OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "sample_ts",
					   TIMESTAMPTZOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "type",
					   TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "event",
					   TEXTOID, -1, 0);

	return BlessTupleDesc(tupdesc);
}

static HeapTuple
get_history_item_tuple(HistoryItem *item, TupleDesc tuple_desc)
{
	HeapTuple	tuple;
	Datum		values[4];
	bool		nulls[4];
	const char *event_type,
			   *event;

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));

	/* Values available to all callers */
	event_type = pgstat_get_wait_event_type(item->wait_event_info);
	event = pgstat_get_wait_event(item->wait_event_info);
	values[0] = Int32GetDatum(item->pid);
	values[1] = TimestampTzGetDatum(item->ts);
	if (event_type)
		values[2] = PointerGetDatum(cstring_to_text(event_type));
	else
		nulls[2] = true;
	if (event)
		values[3] = PointerGetDatum(cstring_to_text(event));
	else
		nulls[3] = true;

	tuple = heap_form_tuple(tuple_desc, values, nulls);
	return tuple;
}

/*
 * Find PGPROC entry responsible for given pid.
 */
static PGPROC *
search_proc(int pid)
{
	int i;

	if (pid == 0)
		return MyProc;

	for (i = 0; i < ProcGlobal->allProcCount; i++)
	{
		PGPROC	*proc = &ProcGlobal->allProcs[i];
		if (proc->pid && proc->pid == pid)
		{
			return proc;
		}
	}

	ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("backend with pid=%d not found", pid)));
	return NULL;
}

typedef struct
{
	HistoryItem	   *state;
	TimestampTz		ts;
} WaitCurrentContext;

PG_FUNCTION_INFO_V1(pg_wait_sampling_get_current);
Datum
pg_wait_sampling_get_current(PG_FUNCTION_ARGS)
{
	FuncCallContext 	*funcctx;
	WaitCurrentContext 	*params;
	HistoryItem 		*currentState;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext		oldcontext;
		WaitCurrentContext 	*params;

		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		params = (WaitCurrentContext *)palloc0(sizeof(WaitCurrentContext));
		params->ts = GetCurrentTimestamp();

		funcctx->user_fctx = params;
		funcctx->tuple_desc = get_history_item_tupledesc();

		LWLockAcquire(ProcArrayLock, LW_SHARED);

		if (!PG_ARGISNULL(0))
		{
			HistoryItem		item;
			PGPROC		   *proc;

			proc = search_proc(PG_GETARG_UINT32(0));
			read_current_wait(proc, &item);
			params->state = (HistoryItem *)palloc0(sizeof(HistoryItem));
			funcctx->max_calls = 1;
			*params->state = item;
		}
		else
		{
			int					procCount = ProcGlobal->allProcCount,
								i,
								j = 0;
			Timestamp			currentTs = GetCurrentTimestamp();

			params->state = (HistoryItem *) palloc0(sizeof(HistoryItem) * procCount);
			for (i = 0; i < procCount; i++)
			{
				PGPROC *proc = &ProcGlobal->allProcs[i];

				if (proc != NULL && proc->pid != 0)
				{
					read_current_wait(proc, &params->state[j]);
					params->state[j].ts = currentTs;
					j++;
				}
			}
			funcctx->max_calls = j;
		}

		LWLockRelease(ProcArrayLock);

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	params = (WaitCurrentContext *)funcctx->user_fctx;
	currentState = NULL;

	if (funcctx->call_cntr < funcctx->max_calls)
	{
		HeapTuple tuple;

		tuple = get_history_item_tuple(&params->state[funcctx->call_cntr],
									   funcctx->tuple_desc);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		SRF_RETURN_DONE(funcctx);
	}
}

typedef struct
{
	long			count;
	ProfileItem	   *items;
} Profile;

static void
initLockTag(LOCKTAG *tag)
{
	tag->locktag_field1 = PG_WAIT_SAMPLING_MAGIC;
	tag->locktag_field2 = 0;
	tag->locktag_field3 = 0;
	tag->locktag_field4 = 0;
	tag->locktag_type = LOCKTAG_USERLOCK;
	tag->locktag_lockmethodid = USER_LOCKMETHOD;
}

static Profile *
receive_profile(shm_mq_handle *mqh)
{
	Size			len;
	void		   *data;
	Profile		   *result;
	long			count,
					i;
	shm_mq_result	res;

	res = shm_mq_receive(mqh, &len, &data, false);
	if (res != SHM_MQ_SUCCESS)
		elog(ERROR, "Error reading mq.");
	if (len != sizeof(count))
		elog(ERROR, "Invalid message length.");
	memcpy(&count, data, sizeof(count));

	result = (Profile *) palloc(sizeof(Profile));
	result->count = count;
	result->items = (ProfileItem *) palloc(result->count * sizeof(ProfileItem));

	for (i = 0; i < count; i++)
	{
		res = shm_mq_receive(mqh, &len, &data, false);
		if (res != SHM_MQ_SUCCESS)
			elog(ERROR, "Error reading mq.");
		if (len != sizeof(ProfileItem))
			elog(ERROR, "Invalid item message length %d %d.", len, sizeof(ProfileItem));
		memcpy(&result->items[i], data, sizeof(ProfileItem));
	}

	return result;
}

PG_FUNCTION_INFO_V1(pg_wait_sampling_get_profile);
Datum
pg_wait_sampling_get_profile(PG_FUNCTION_ARGS)
{
	Profile *profile;
	FuncCallContext *funcctx;

	if (SRF_IS_FIRSTCALL())
	{
		shm_mq			   *mq;
		shm_mq_handle	   *mqh;
		LOCKTAG				tag;
		MemoryContext		oldcontext;
		TupleDesc			tupdesc;

		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		initLockTag(&tag);

		LockAcquire(&tag, ExclusiveLock, false, false);

		mq = shm_mq_create(shm_toc_lookup(toc, 1), COLLECTOR_QUEUE_SIZE);
		collector_hdr->request = PROFILE_REQUEST;

		SetLatch(collector_hdr->latch);

		shm_mq_set_receiver(mq, MyProc);
		mqh = shm_mq_attach(mq, NULL, NULL);

		profile = receive_profile(mqh);
		funcctx->user_fctx = profile;
		funcctx->max_calls = profile->count;

		shm_mq_detach(mq);

		LockRelease(&tag, ExclusiveLock, false);

		tupdesc = CreateTemplateTupleDesc(4, false);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "pid",
						   INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "type",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "event",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 4, "count",
						   INT8OID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	profile = (Profile *)funcctx->user_fctx;

	if (funcctx->call_cntr < funcctx->max_calls)
	{
		/* for each row */
		Datum		values[4];
		bool		nulls[4];
		HeapTuple	tuple;
		ProfileItem *item;
		const char *event_type,
				   *event;

		item = &profile->items[funcctx->call_cntr];

		MemSet(values, 0, sizeof(values));
		MemSet(nulls, 0, sizeof(nulls));

		/* Values available to all callers */
		event_type = pgstat_get_wait_event_type(item->wait_event_info);
		event = pgstat_get_wait_event(item->wait_event_info);
		values[0] = Int32GetDatum(item->pid);
		if (event_type)
			values[1] = PointerGetDatum(cstring_to_text(event_type));
		else
			nulls[1] = true;
		if (event)
			values[2] = PointerGetDatum(cstring_to_text(event));
		else
			nulls[2] = true;
		values[3] = Int64GetDatum(item->count);

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		/* nothing left */
		SRF_RETURN_DONE(funcctx);
	}
}

PG_FUNCTION_INFO_V1(pg_wait_sampling_reset_profile);
Datum
pg_wait_sampling_reset_profile(PG_FUNCTION_ARGS)
{
	LOCKTAG		tag;

	initLockTag(&tag);

	LockAcquire(&tag, ExclusiveLock, false, false);

	collector_hdr->request = PROFILE_RESET;
	SetLatch(collector_hdr->latch);

	LockRelease(&tag, ExclusiveLock, false);

	PG_RETURN_VOID();
}

static History *
receive_observations(shm_mq_handle *mqh)
{
	Size			len;
	void		   *data;
	History		   *result;
	int				count,
					i;
	shm_mq_result	res;

	res = shm_mq_receive(mqh, &len, &data, false);
	if (res != SHM_MQ_SUCCESS)
		elog(ERROR, "Error reading mq.");
	if (len != sizeof(count))
		elog(ERROR, "Invalid message length.");
	memcpy(&count, data, sizeof(count));

	result = (History *) palloc(sizeof(History));
	AllocHistory(result, count);

	for (i = 0; i < count; i++)
	{
		res = shm_mq_receive(mqh, &len, &data, false);
		if (res != SHM_MQ_SUCCESS)
			elog(ERROR, "Error reading mq.");
		if (len != sizeof(HistoryItem))
			elog(ERROR, "Invalid message length.");
		memcpy(&result->items[i], data, sizeof(HistoryItem));
	}

	return result;
}

PG_FUNCTION_INFO_V1(pg_wait_sampling_get_history);
Datum
pg_wait_sampling_get_history(PG_FUNCTION_ARGS)
{
	History				*observations;
	FuncCallContext		*funcctx;

	check_shmem();

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext	oldcontext;
		LOCKTAG			tag;
		shm_mq		   *mq;
		shm_mq_handle  *mqh;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		initLockTag(&tag);
		LockAcquire(&tag, ExclusiveLock, false, false);

		mq = shm_mq_create(collector_mq, COLLECTOR_QUEUE_SIZE);
		collector_hdr->request = HISTORY_REQUEST;

		SetLatch(collector_hdr->latch);

		shm_mq_set_receiver(mq, MyProc);
		mqh = shm_mq_attach(mq, NULL, NULL);

		observations = receive_observations(mqh);
		funcctx->user_fctx = observations;
		funcctx->max_calls = observations->count;

		shm_mq_detach(mq);
		LockRelease(&tag, ExclusiveLock, false);

		funcctx->tuple_desc = get_history_item_tupledesc();
		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	observations = (History *)funcctx->user_fctx;

	if (observations->index < observations->count)
	{
		HeapTuple	tuple;
		HistoryItem *observation;

		observation = &observations->items[observations->index];
		tuple = get_history_item_tuple(observation, funcctx->tuple_desc);
		observations->index++;
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		/* nothing left */
		SRF_RETURN_DONE(funcctx);
	}

	PG_RETURN_VOID();
}
