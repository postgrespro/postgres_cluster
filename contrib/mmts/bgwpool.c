#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "postmaster/bgworker.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/proc.h"
#include "storage/pg_sema.h"
#include "storage/shmem.h"
#include "datatype/timestamp.h"
#include "utils/portal.h"
#include "tcop/pquery.h"

#include "bgwpool.h"

bool MtmIsLogicalReceiver;
int  MtmMaxWorkers;

static BgwPool* MtmPool;

static void BgwShutdownWorker(int sig)
{
	if (MtmPool) { 
		BgwPoolStop(MtmPool);
	}
}

static void BgwPoolMainLoop(BgwPool* pool)
{
    int size;
    void* work;
	static PortalData fakePortal;

	MtmIsLogicalReceiver = true;
	MtmPool = pool;

	signal(SIGINT, BgwShutdownWorker);
	signal(SIGQUIT, BgwShutdownWorker);
	signal(SIGTERM, BgwShutdownWorker);

    BackgroundWorkerUnblockSignals();
	BackgroundWorkerInitializeConnection(pool->dbname, pool->dbuser);
	ActivePortal = &fakePortal;
	ActivePortal->status = PORTAL_ACTIVE;
	ActivePortal->sourceText = "";

    while (true) { 
        PGSemaphoreLock(&pool->available);
        SpinLockAcquire(&pool->lock);
		if (pool->shutdown) { 
			break;
		}
        size = *(int*)&pool->queue[pool->head];
        Assert(size < pool->size);
        work = palloc(size);
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
        pool->executor(work, size);
        pfree(work);
        SpinLockAcquire(&pool->lock);
        pool->active -= 1;
		pool->lastPeakTime = 0;
        SpinLockRelease(&pool->lock);
    }
	SpinLockRelease(&pool->lock);
}

void BgwPoolInit(BgwPool* pool, BgwPoolExecutor executor, char const* dbname,  char const* dbuser, size_t queueSize, size_t nWorkers)
{
	MtmPool = pool;
    pool->queue = (char*)ShmemAlloc(queueSize);
	if (pool->queue == NULL) { 
		elog(PANIC, "Failed to allocate memory for background workers pool: %lld bytes requested", (long64)queueSize);
	}
    pool->executor = executor;
    PGSemaphoreCreate(&pool->available);
    PGSemaphoreCreate(&pool->overflow);
    PGSemaphoreReset(&pool->available);
    PGSemaphoreReset(&pool->overflow);
    SpinLockInit(&pool->lock);
	pool->shutdown = false;
    pool->producerBlocked = false;
    pool->head = 0;
    pool->tail = 0;
    pool->size = queueSize;
    pool->active = 0;
    pool->pending = 0;
	pool->nWorkers = nWorkers;
	pool->lastPeakTime = 0;
	pool->lastDynamicWorkerStartTime = 0;
	strncpy(pool->dbname, dbname, MAX_DBNAME_LEN);
	strncpy(pool->dbuser, dbuser, MAX_DBUSER_LEN);
}
 
timestamp_t BgwGetLastPeekTime(BgwPool* pool)
{
	return pool->lastPeakTime;
}

static void BgwPoolStaticWorkerMainLoop(Datum arg)
{
	BgwPoolConstructor constructor = (BgwPoolConstructor)DatumGetPointer(arg);
    BgwPoolMainLoop(constructor());
}

static void BgwPoolDynamicWorkerMainLoop(Datum arg)
{
    BgwPoolMainLoop((BgwPool*)DatumGetPointer(arg));
}

void BgwPoolStart(int nWorkers, BgwPoolConstructor constructor)
{
    int i;
	BackgroundWorker worker;

	MemSet(&worker, 0, sizeof(BackgroundWorker));
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_main = BgwPoolStaticWorkerMainLoop;
	worker.bgw_restart_time = MULTIMASTER_BGW_RESTART_TIMEOUT;

    for (i = 0; i < nWorkers; i++) { 
        snprintf(worker.bgw_name, BGW_MAXLEN, "bgw_pool_worker_%d", i+1);
        worker.bgw_main_arg = PointerGetDatum(constructor);
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


static void BgwStartExtraWorker(BgwPool* pool)
{
	if (pool->nWorkers < MtmMaxWorkers) { 
		timestamp_t now = MtmGetSystemTime();
		/*if (pool->lastDynamicWorkerStartTime + MULTIMASTER_BGW_RESTART_TIMEOUT*USECS_PER_SEC < now)*/
		{ 
			BackgroundWorker worker;
			BackgroundWorkerHandle* handle;
			MemSet(&worker, 0, sizeof(BackgroundWorker));
			worker.bgw_flags = BGWORKER_SHMEM_ACCESS |  BGWORKER_BACKEND_DATABASE_CONNECTION;
			worker.bgw_start_time = BgWorkerStart_ConsistentState;
			worker.bgw_main = BgwPoolDynamicWorkerMainLoop;
			worker.bgw_restart_time = MULTIMASTER_BGW_RESTART_TIMEOUT;
			snprintf(worker.bgw_name, BGW_MAXLEN, "bgw_pool_dynworker_%d", (int)++pool->nWorkers);
			worker.bgw_main_arg = PointerGetDatum(pool);
			pool->lastDynamicWorkerStartTime = now;
			if (!RegisterDynamicBackgroundWorker(&worker, &handle)) { 
				elog(WARNING, "Failed to start dynamic background worker");
			}
		}
	}
}

void BgwPoolExecute(BgwPool* pool, void* work, size_t size)
{
    if (size+4 > pool->size) {
		/* 
		 * Size of work is larger than size of shared buffer: 
		 * run it immediately
		 */
		pool->executor(work, size);
		return;
	}
 
    SpinLockAcquire(&pool->lock);
    while (!pool->shutdown) { 
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
			if (pool->active + pool->pending > pool->nWorkers) { 
				BgwStartExtraWorker(pool);				
			}
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

void BgwPoolStop(BgwPool* pool)
{
    SpinLockAcquire(&pool->lock);
	pool->shutdown = true;
    SpinLockRelease(&pool->lock);            
	PGSemaphoreUnlock(&pool->available);
	PGSemaphoreUnlock(&pool->overflow);
}
