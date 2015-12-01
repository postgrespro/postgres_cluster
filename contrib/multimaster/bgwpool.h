#ifndef __BGWPOOL_H__
#define __BGWPOOL_H__

typedef void(*BgwExecutor)(int id, void* work, size_t size);

#define MAX_DBNAME_LEN 30

typedef struct
{
    BgwExecutor executor;
    volatile slock_t lock;
    PGSemaphoreData available;
    PGSemaphoreData overflow;
    size_t head;
    size_t tail;
    size_t size;
    bool producerBlocked;
    char dbname[MAX_DBNAME_LEN];
    char queue[1];
} BgwPool;

extern BgwPool* BgwPoolCreate(BgwExecutor executor, char const* dbname, size_t queueSize, int nWorkers);

extern void BgwPoolExecute(BgwPool* pool, void* work, size_t size);

#endif
