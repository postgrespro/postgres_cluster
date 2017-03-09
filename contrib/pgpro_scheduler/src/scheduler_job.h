#ifndef PGPRO_SCHEDULER_JOB_H
#define PGPRO_SCHEDULER_JOB_H

#include <time.h>
#include "postgres.h"
#include <stdio.h>
#include <stdarg.h>
#include "utils/timestamp.h"
#include "memutils.h"
#include "c.h"
#include "port.h"

typedef enum {
	CronJob = 1,
	AtJob = 2
} task_type_t;

typedef struct {
	task_type_t type;
	int cron_id;
	TimestampTz start_at;
	TimestampTz started;
	char *node;
	TimestampTz last_start_avail;
	bool same_transaction;
	int dosql_n;
	char **dosql;
	int sql_params_n;
	char **sql_params;
	int depends_on_n;
	int64 *depends_on;
	TimestampTz postpone;
	char *executor;
	char *owner;
	int64 timelimit;
	int max_instances;
	char *onrollback;
	char *next_time_statement;
	bool is_active;
	int64 attempt;
	int64 resubmit_limit;
	char *error;
} job_t;

job_t *init_scheduler_job(job_t *j, unsigned char type);
job_t *get_expired_cron_jobs(char *nodename, int *n, int *is_error);
job_t *get_expired_at_jobs(char *nodename, int *n, int *is_error);
job_t *_cron_get_jobs_to_do(char *nodename, int *n, int *is_error, int limit);
job_t *_at_get_jobs_to_do(char *nodename, int *n, int *is_error, int limit);
job_t *get_jobs_to_do(char *nodename, task_type_t type, int *n, int *is_error, int limit);
job_t *set_job_error(job_t *j, const char *fmt, ...) pg_attribute_printf(2, 3);
int move_job_to_log(job_t *j, bool status, bool processed);
void destroy_job(job_t *j, int selfdestroy);
job_t *get_at_job(int cron_id, char *nodename, char **perror);
job_t *get_cron_job(int cron_id, TimestampTz start_at, char *nodename, char **perror);
int _cron_move_job_to_log(job_t *j, bool status);
int _at_move_job_to_log(job_t *j, bool status, bool processed);
int resubmit_at_job(job_t *j, TimestampTz next);
job_t *get_next_at_job_with_lock(char *nodename, char **error);
int set_at_job_done(job_t *job, char *error, int64 resubmit, char **set_error);
int _v1_set_at_job_done(job_t *job, char *error, int64 resubmit);
int move_at_job_process(int job_id);
job_t *get_at_job_for_process(char *nodename, char **error);

#endif

