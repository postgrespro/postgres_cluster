#include <stdlib.h>
#include "postgres.h"
#include "string.h"
#include "char_array.h"
#include "sched_manager_poll.h"
#include "lib/stringinfo.h"
#include "pgstat.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "storage/dsm.h"
#include "storage/shm_toc.h"
#include "utils/memutils.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "pgpro_scheduler.h"
#include "utils/resowner.h"
#include "memutils.h"

#include "postmaster/bgworker.h"

int __cmp_managers(const void *a, const void *b)
{
	const schd_manager_t *A = *(schd_manager_t * const *)a;
	const schd_manager_t *B = *(schd_manager_t * const *)b;

	return strcmp(B->dbname, A->dbname);
}

void set_supervisor_pgstatus(schd_managers_poll_t *poll)
{
	char *sv_status;

	sv_status = supervisor_state(poll);
	pgstat_report_activity(STATE_IDLE, sv_status);
	pfree(sv_status);
}

char *supervisor_state(schd_managers_poll_t *poll)
{
	char *dbnames = poll_dbnames(poll);
	char *status;
	int len;

	if(!poll->enabled)
	{
		status = palloc(sizeof(char) * 9);
		memcpy(status, "disabled", 8);
		status[8] = 0;
		return status;
	}

	len = dbnames ? strlen(dbnames): 0;
	if(len == 0)
	{
		status = palloc(sizeof(char)*26);
		memcpy(status, "waiting databases to set", 25);
		status[25] = 0;
	}
	else
	{
		status = palloc(sizeof(char) * (len + 10));
		memcpy(status, "work on: ", 9);
		memcpy(status+9, dbnames, len);
		status[len+9] = 0;
	}
	if(dbnames) pfree(dbnames);

	return status;
}

char *poll_dbnames(schd_managers_poll_t *poll)
{
	int i;
	StringInfoData string;
	char *out;

	initStringInfo(&string);

	for(i=0; i < poll->n; i++)
	{
		appendStringInfo(&string, "%s", poll->workers[i]->dbname);
		if(i < (poll->n - 1))
			appendStringInfo(&string, ", ");
	}
	out = palloc(sizeof(char) * (string.len + 1));
	memcpy(out, string.data, string.len);
	out[string.len] = 0;
	pfree(string.data);

	return out;
}

void changeChildBgwState(schd_manager_share_t *s, schd_manager_status_t status)
{
	PGPROC *parent;
	s->status = status;
	s->setbychild = true;

	parent = BackendPidGetProc(MyBgworkerEntry->bgw_notify_pid);
	if(parent)
	{
		SetLatch(&parent->procLatch);
		/* elog(LOG, "set LATCH to %d - status = %d" , MyBgworkerEntry->bgw_notify_pid, status); */
	}
	else
	{
		/* elog(LOG, "unable to set LATCH to %d", MyBgworkerEntry->bgw_notify_pid); */
	}
}

int stopAllManagers(schd_managers_poll_t *poll)
{
	int i;
	PGPROC *child;
	schd_manager_share_t *shared;

	if(poll->n == 0) return 0;

	elog(LOG, "Scheduler stops all managers");

	for(i=0; i < poll->n; i++)
	{
		shared = dsm_segment_address(poll->workers[i]->shared);
		shared->setbyparent = true;
		shared->status = SchdManagerStop;
		child = BackendPidGetProc(poll->workers[i]->pid);
		if(!child)
		{
			/* elog(LOG, "cannot get PGRPOC of %s scheduler", poll->workers[i]->dbname); */
		}
		else
		{
			SetLatch(&child->procLatch);
		}
	}

	for(i=0; i < poll->n; i++)
	{
		destroyManagerRecord(poll->workers[i]);
	}

	pfree(poll->workers);
	poll->workers = NULL;
	poll->n = 0;

	return 1;
}

void destroyManagerRecord(schd_manager_t *man)
{
	pfree(man->dbname);
	dsm_detach(man->shared);
	pfree(man);
}

schd_managers_poll_t *initSchedulerManagerPool(char_array_t *names)
{
	int i;
	schd_managers_poll_t *p = worker_alloc(sizeof(schd_managers_poll_t));

	p->n = 0;
	p->workers = NULL;
	p->enabled = true;

	if(!is_scheduler_enabled())
	{
		p->enabled = false;
		return p;
	}

	if(names->n > 0)
	{
		for(i=0; i < names->n; i++)
		{
			addManagerToPoll(p, names->data[i], 0);
		}
		_sortPollManagers(p);
	}

	return p;
}

int isBaseListChanged(char_array_t *names, schd_managers_poll_t *poll)
{
	int i,j;

	if(names->n != poll->n) return 1;

	for(i=0; i < names->n; i++)
	{
		for(j=0; j < poll->n; j++)
		{
			if(strcmp(names->data[i], poll->workers[j]->dbname) != 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

pid_t registerManagerWorker(schd_manager_t *man)
{
	BackgroundWorker worker;
	BgwHandleStatus status;

	pgstat_report_activity(STATE_RUNNING, "register scheduler manager");


	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	worker.bgw_main_arg = UInt32GetDatum(dsm_segment_handle(man->shared));
	sprintf(worker.bgw_library_name, "pgpro_scheduler");
	sprintf(worker.bgw_function_name, "manager_worker_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "scheduler %s", man->dbname);
	memcpy(worker.bgw_extra, man->dbname,
		BGW_EXTRALEN > strlen(man->dbname)+1 ?
			strlen(man->dbname)+1 : BGW_EXTRALEN);
	worker.bgw_notify_pid = MyProcPid;

	if (!RegisterDynamicBackgroundWorker(&worker, &(man->handler)))
	{
		elog(LOG, "Cannot register manager worker for db: %s", man->dbname);
		return 0;
	}
	status = WaitForBackgroundWorkerStartup(man->handler, &(man->pid));
	if (status != BGWH_STARTED)
	{
		elog(LOG, "Cannot start manager worker for db: %s (%d)", man->dbname, status);
		return 0;
	}

	return man->pid;
}

void _sortPollManagers(schd_managers_poll_t *poll)
{
	if(poll->n > 1) 
		qsort(poll->workers, poll->n, sizeof(schd_manager_t *), __cmp_managers);
}

int removeManagerFromPoll(schd_managers_poll_t *poll, char *name, char sort, bool stop_worker)
{
	int found  = 0;
	int i;
	schd_manager_t *mng;

	for(i=0; i < poll->n; i++)
	{
		if(strcmp(name, poll->workers[i]->dbname) == 0)
		{
			found = 1;
			break;
		}
	}

	if(found == 0) return 0;
	mng = poll->workers[i];

	if(stop_worker)
	{
		elog(LOG, "Stop scheduler manager for %s", mng->dbname);
		TerminateBackgroundWorker(mng->handler);
	}

	if(poll->n == 1)
	{
		poll->n = 0;
		pfree(poll->workers[0]->dbname);
		pfree(poll->workers[0]);
		pfree(poll->workers);
		poll->workers = NULL;
	}
	else
	{
		pfree(poll->workers[i]->dbname);
		pfree(poll->workers[i]);
		if(i != (poll->n-1))
		{
			poll->workers[i] = poll->workers[poll->n-1];
		}
		poll->n--;
		poll->workers = repalloc(poll->workers, sizeof(schd_manager_t *) * poll->n);
	}

	if(sort) _sortPollManagers(poll);
	return 1;
}


int addManagerToPoll(schd_managers_poll_t *poll, char *name, int sort)
{
	int pos = -1;
	schd_manager_t *man;
	schd_manager_share_t *shm_data;
	Size segsize;
/*	shm_toc_estimator e;
	shm_toc *toc; */
	dsm_segment *seg;

/*	shm_toc_initialize_estimator(&e);
	shm_toc_estimate_chunk(&e, sizeof(schd_manager_share_t));
	shm_toc_estimate_keys(&e, 1);
	segsize = shm_toc_estimate(&e); */
	segsize = (Size)sizeof(schd_manager_share_t);

	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pgpro_scheduler");
	seg = dsm_create(segsize, 0);

	man = palloc(sizeof(schd_manager_t));
	man->dbname = palloc(sizeof(char *) * (strlen(name) + 1));
	man->connected = false;
	memcpy(man->dbname, name, strlen(name) + 1);
	man->shared = seg;
/*	toc = shm_toc_create(PGPRO_SHM_TOC_MAGIC, dsm_segment_address(man->shared),
	                         segsize);

	shm_data = shm_toc_allocate(toc, sizeof(schd_manager_share_t));
	shm_toc_insert(toc, 0, shm_data); */
	shm_data = dsm_segment_address(man->shared);

	shm_data->setbyparent = true;
	shm_data->setbychild = false;
	shm_data->status = SchdManagerInit;

	if(registerManagerWorker(man) > 0)
	{
		pos = poll->n++;
		poll->workers = poll->workers ?
			repalloc(poll->workers, sizeof(schd_manager_t *) * poll->n):
			palloc(sizeof(schd_manager_t *));
		poll->workers[pos] = man;
		if(sort) _sortPollManagers(poll);

		return 1;
	}

	dsm_detach(man->shared);
	pfree(man->dbname);
	pfree(man);

	return 0;
}

int refreshManagers(char_array_t *names, schd_managers_poll_t *poll)
{
	int i,j;
	int found = 0;
	char_array_t *new = makeCharArray();
	char_array_t *same = makeCharArray();
	char_array_t *delete = makeCharArray();

	for(i=0; i < names->n; i++)
	{
		found = 0;
		for(j=0; j < poll->n; j++)
		{
			if(strcmp(names->data[i], poll->workers[j]->dbname) == 0)
			{
				found = 1;
				pushCharArray(same, names->data[i]);
			}
		}
		if(found == 0) pushCharArray(new, names->data[i]);
	}

	if(poll->n != same->n)
	{
		for(i=0; i < poll->n; i++)
		{
			found = 0;
			for(j=0; j < same->n; j++)
			{
				if(strcmp(poll->workers[i]->dbname, same->data[j]) == 0)
				{
					found = 1;
					break;
				}
			}
			if(found == 0) pushCharArray(delete, poll->workers[i]->dbname);
		}
	}
	/* elog(LOG, "WORK RESULT: same: %d, new: %d, delete: %d", same->n, new->n, delete->n); */
	if(delete->n)
	{
		for(i = 0; i < delete->n; i++)
		{
			removeManagerFromPoll(poll, delete->data[i], 0, true);
		}
	}
	if(new->n)
	{
		for(i = 0; i < new->n; i++)
		{
			addManagerToPoll(poll, new->data[i], 0);
		}
	}

	if(delete->n && new->n) _sortPollManagers(poll);

	return 1;
}



