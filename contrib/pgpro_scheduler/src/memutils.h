#ifndef PGPRO_SCHEDULER_MEMUTILS_H
#define PGPRO_SCHEDULER_MEMUTILS_H

#include "postgres.h"
#include "utils/memutils.h"

extern MemoryContext SchedulerWorkerContext;

MemoryContext init_worker_mem_ctx(const char *name);
MemoryContext init_mem_ctx(const char *name);
MemoryContext switch_to_worker_context(void);
void *worker_alloc(Size size);
void delete_worker_mem_ctx(void);

#endif
