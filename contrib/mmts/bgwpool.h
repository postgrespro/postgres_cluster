#ifndef __BGWPOOL_H__
#define __BGWPOOL_H__

#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/pg_sema.h"
#include "bkb.h"

typedef void(*BgwPoolExecutor)(void* work, size_t size);

typedef ulong64 timestamp_t;


#define MAX_DBNAME_LEN 30
#define MAX_DBUSER_LEN 30
#define MULTIMASTER_BGW_RESTART_TIMEOUT 1 /* seconds */

extern timestamp_t MtmGetSystemTime(void);   /* non-adjusted current system time */
extern timestamp_t MtmGetCurrentTime(void);  /* adjusted current system time */

extern bool MtmIsLogicalReceiver;
extern int  MtmMaxWorkers;

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
    size_t pending;
	size_t nWorkers;
	time_t lastPeakTime;
	timestamp_t lastDynamicWorkerStartTime;
    bool   producerBlocked;
	bool   shutdown;
    char   dbname[MAX_DBNAME_LEN];
	char   dbuser[MAX_DBUSER_LEN];
    char*  queue;
} BgwPool;

typedef BgwPool*(*BgwPoolConstructor)(void);

extern void BgwPoolStart(int nWorkers, BgwPoolConstructor constructor);

extern void BgwPoolInit(BgwPool* pool, BgwPoolExecutor executor, char const* dbname, char const* dbuser, size_t queueSize, size_t nWorkers);

extern void BgwPoolExecute(BgwPool* pool, void* work, size_t size);

extern size_t BgwPoolGetQueueSize(BgwPool* pool);

extern timestamp_t BgwGetLastPeekTime(BgwPool* pool);

extern void BgwPoolStop(BgwPool* pool);
#endif
