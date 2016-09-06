#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "postmaster/bgworker.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/pg_sema.h"
#include "storage/shmem.h"

#include "bgwpool.h"

bool MtmIsLogicalReceiver;

typedef struct
{
    BgwPoolConstructor constructor;
    int id;
} BgwPoolExecutorCtx;

static void BgwPoolMainLoop(Datum arg)
{
    BgwPoolExecutorCtx* ctx = (BgwPoolExecutorCtx*)arg;
    int id = ctx->id;
    BgwPool* pool = ctx->constructor(id);
    int size;
    void* work;

	MtmIsLogicalReceiver = true;

    BackgroundWorkerUnblockSignals();
	BackgroundWorkerInitializeConnection(pool->dbname, pool->dbuser);

    while(true) { 
        PGSemaphoreLock(&pool->available);
        SpinLockAcquire(&pool->lock);
        size = *(int*)&pool->queue[pool->head];
        Assert(size < pool->size);
        work = malloc(size);
        pool->pending -= 1;
        pool->active += 1;
		if (pool->lastPeakTime == 0 && pool->active == pool->nWorkers && pool->pending != 0) {
			pool->lastPeakTime = MtmGetSystemTime();
		}
        if (pool->head + size + 4 > pool->size) { 
            memcpy(work, pool->queue, size);
            pool->head = INTALIGN(size);
        } else { 
            memcpy(work, &pool->queue[pool->head+4], size);
            pool->head += 4 + INTALIGN(size);
        }
        if (pool->size == pool->head) { 
            pool->head = 0;
        }
        if (pool->producerBlocked) {
            pool->producerBlocked = false;
            PGSemaphoreUnlock(&pool->overflow);
			pool->lastPeakTime = 0;
        }
        SpinLockRelease(&pool->lock);
        pool->executor(id, work, size);
        free(work);
        SpinLockAcquire(&pool->lock);
        pool->active -= 1;
		pool->lastPeakTime = 0;
        SpinLockRelease(&pool->lock);
    }
}

void BgwPoolInit(BgwPool* pool, BgwPoolExecutor executor, char const* dbname,  char const* dbuser, size_t queueSize, size_t nWorkers)
{
    pool->queue = (char*)ShmemAlloc(queueSize);
    pool->executor = executor;
    PGSemaphoreCreate(&pool->available);
    PGSemaphoreCreate(&pool->overflow);
    PGSemaphoreReset(&pool->available);
    PGSemaphoreReset(&pool->overflow);
    SpinLockInit(&pool->lock);
    pool->producerBlocked = false;
    pool->head = 0;
    pool->tail = 0;
    pool->size = queueSize;
    pool->active = 0;
    pool->pending = 0;
	pool->nWorkers = nWorkers;
	pool->lastPeakTime = 0;
	strncpy(pool->dbname, dbname, MAX_DBNAME_LEN);
	strncpy(pool->dbuser, dbuser, MAX_DBUSER_LEN);
}
 
timestamp_t BgwGetLastPeekTime(BgwPool* pool)
{
	return pool->lastPeakTime;
}

void BgwPoolStart(int nWorkers, BgwPoolConstructor constructor)
{
    int i;
	BackgroundWorker worker;

	MemSet(&worker, 0, sizeof(BackgroundWorker));
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_main = BgwPoolMainLoop;
	worker.bgw_restart_time = MULTIMASTER_BGW_RESTART_TIMEOUT;

    for (i = 0; i < nWorkers; i++) { 
        BgwPoolExecutorCtx* ctx = (BgwPoolExecutorCtx*)malloc(sizeof(BgwPoolExecutorCtx));
        snprintf(worker.bgw_name, BGW_MAXLEN, "bgw_pool_worker_%d", i+1);
        ctx->id = i;
        ctx->constructor = constructor;
        worker.bgw_main_arg = (Datum)ctx;
        RegisterBackgroundWorker(&worker);
    }
}

size_t BgwPoolGetQueueSize(BgwPool* pool)
{
	size_t used;
    SpinLockAcquire(&pool->lock);
	used = pool->head <= pool->tail ? pool->tail - pool->head : pool->size - pool->head + pool->tail;
    SpinLockRelease(&pool->lock);            
	return used;
}


void BgwPoolExecute(BgwPool* pool, void* work, size_t size)
{
    if (size+4 > pool->size) {
		/* 
		 * Size of work is larger than size of shared buffer: 
		 * run it immediately
		 */
		pool->executor(0, work, size);
		return;
	}
 
    SpinLockAcquire(&pool->lock);
    while (true) { 
        if ((pool->head <= pool->tail && pool->size - pool->tail < size + 4 && pool->head < size) 
            || (pool->head > pool->tail && pool->head - pool->tail < size + 4))
        {
            if (pool->lastPeakTime == 0) {
				pool->lastPeakTime = MtmGetSystemTime();
			}
			pool->producerBlocked = true;
            SpinLockRelease(&pool->lock);
            PGSemaphoreLock(&pool->overflow);
            SpinLockAcquire(&pool->lock);
        } else {
            pool->pending += 1;
			if (pool->lastPeakTime == 0 && pool->active == pool->nWorkers && pool->pending != 0) {
				pool->lastPeakTime = MtmGetSystemTime();
			}
            *(int*)&pool->queue[pool->tail] = size;
            if (pool->size - pool->tail >= size + 4) { 
                memcpy(&pool->queue[pool->tail+4], work, size);
                pool->tail += 4 + INTALIGN(size);
            } else { 
                memcpy(pool->queue, work, size);
                pool->tail = INTALIGN(size);
            }
            if (pool->tail == pool->size) {
                pool->tail = 0;
            }
            PGSemaphoreUnlock(&pool->available);
            break;
        }
    }
    SpinLockRelease(&pool->lock);            
}

