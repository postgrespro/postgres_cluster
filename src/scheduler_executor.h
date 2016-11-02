#ifndef PGPRO_SCHEDULER_EXECUTOR_H
#define PGPRO_SCHEDULER_EXECUTOR_H

#include "postgres.h"
#include "pgpro_scheduler.h"
#include "utils/timestamp.h"
#include "scheduler_job.h"

typedef enum {
	SchdExecutorInit,
	SchdExecutorWork,
	SchdExecutorDone,
	SchdExecutorError
} schd_executor_status_t;

typedef struct {
	char database[PGPRO_SCHEDULER_DBNAME_MAX];
	char nodename[PGPRO_SCHEDULER_NODENAME_MAX];

	int cron_id;
	TimestampTz start_at;

	schd_executor_status_t status;
	TimestampTz status_set;

	char message[PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX];
} schd_executor_share_t;

void executor_worker_main(Datum arg);
job_t *initializeExecutorJob(schd_executor_share_t *data);


#endif

