#ifndef __BGWPOOL_H__
#define __BGWPOOL_H__

#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/pg_sema.h"

typedef void(*BgwPoolExecutor)(int id, void* work, size_t size);

#define MAX_DBNAME_LEN 30
#define MULTIMASTER_BGW_RESTART_TIMEOUT 1 /* seconds */

typedef struct
{
    BgwPoolExecutor executor;
    volatile slock_t lock;
    PGSemaphoreData available;
    PGSemaphoreData overflow;
    size_t head;
    size_t tail;
    size_t size;
    size_t active;
    bool   producerBlocked;
    char   dbname[MAX_DBNAME_LEN];
    char*  queue;
} BgwPool;

typedef BgwPool*(*BgwPoolConstructor)(void);

extern void BgwPoolStart(int nWorkers, BgwPoolConstructor constructor);

extern void BgwPoolInit(BgwPool* pool, BgwPoolExecutor executor, char const* dbname, size_t queueSize);

extern void BgwPoolExecute(BgwPool* pool, void* work, size_t size);

extern size_t BgwPoolGetQueueSize(BgwPool* pool);

#endif
