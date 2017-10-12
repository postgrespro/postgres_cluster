#include "postgres.h"
#include "utils/memutils.h"
#include "memutils.h"

MemoryContext SchedulerWorkerContext = NULL;

MemoryContext init_mem_ctx(const char *name)
{
	return AllocSetContextCreate(TopMemoryContext, name,
								 ALLOCSET_DEFAULT_MINSIZE,
								 ALLOCSET_DEFAULT_INITSIZE,
								 ALLOCSET_DEFAULT_MAXSIZE);
}

bool is_worker_context_initialized(void)
{
	return SchedulerWorkerContext == NULL ? false: true;
}

MemoryContext init_worker_mem_ctx(const char *name)
{
	AssertState(SchedulerWorkerContext == NULL);
	SchedulerWorkerContext = AllocSetContextCreate(TopMemoryContext, name,
								 ALLOCSET_DEFAULT_MINSIZE,
								 ALLOCSET_DEFAULT_INITSIZE,
								 ALLOCSET_DEFAULT_MAXSIZE);
	if(SchedulerWorkerContext == NULL) elog(ERROR, "NULL context");
	return SchedulerWorkerContext;
}

MemoryContext switch_to_worker_context(void)
{
	AssertState(SchedulerWorkerContext != NULL);
	return MemoryContextSwitchTo(SchedulerWorkerContext);
}

void check_scheduler_context(void)
{
	if(MemoryContextIsValid(SchedulerWorkerContext))
	{
		elog(LOG, "scheduler context: ok");
	}
	else
	{
		elog(LOG, "scheduler context: broken");
	}
}

void *worker_alloc(Size size)
{
	AssertState(SchedulerWorkerContext != NULL);
	return MemoryContextAlloc(SchedulerWorkerContext, size);
}

void drop_worker_context(void)
{
	if(SchedulerWorkerContext)
	{
		MemoryContextDelete(SchedulerWorkerContext);
		SchedulerWorkerContext = NULL;
	}
}

void delete_worker_mem_ctx(MemoryContext old)
{
	if(!old) old = TopMemoryContext;

	MemoryContextSwitchTo(old);
	MemoryContextDelete(SchedulerWorkerContext);
	SchedulerWorkerContext = NULL;
}

char *_mcopy_string(MemoryContext ctx, char *str)
{
	int len = strlen(str);
	char *cpy;

	if(!ctx) ctx = SchedulerWorkerContext;

	cpy = MemoryContextAlloc(ctx, sizeof(char) * (len+1));
	if(!cpy) return NULL;

	memcpy(cpy, str, len);
	cpy[len] = 0;

	return cpy;
}

char *my_copy_string(char *str)
{
	int len = strlen(str);
	char *cpy;

	cpy = palloc(sizeof(char) * (len+1));
	if(!cpy) return NULL;

	memcpy(cpy, str, len);
	cpy[len] = 0;

	return cpy;
}


