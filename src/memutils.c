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

void *worker_alloc(Size size)
{
	AssertState(SchedulerWorkerContext != NULL);
	return MemoryContextAlloc(SchedulerWorkerContext, size);
}

void delete_worker_mem_ctx(void)
{
	MemoryContextSwitchTo(TopMemoryContext);
	MemoryContextDelete(SchedulerWorkerContext);
}
