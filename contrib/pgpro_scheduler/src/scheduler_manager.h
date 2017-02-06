#ifndef PGPRO_SCHEDULER_MANAGER_H
#define PGPRO_SCHEDULER_MANAGER_H

#include <time.h>
#include "postgres.h"
#include "pgpro_scheduler.h"
#include "utils/timestamp.h"
#include "utils/jsonb.h"
#include "utils/memutils.h"
#include "bit_array.h"
#include "scheduler_job.h"

#define CEO_MIN_POS	0
#define CEO_HRS_POS	1
#define CEO_DAY_POS	2
#define CEO_MON_POS	3
#define CEO_DOW_POS	4

typedef struct {
	int id;
	Jsonb *rule;
	TimestampTz postpone;
	TimestampTz next;
	bool has_next_time_statement;
} scheduler_task_t;

typedef enum {
	RmTimeout,
	RmWaitWorker,
	RmError,
	RmDone
} schd_remove_reason_t;

typedef struct {
	int pos;
	schd_remove_reason_t reason;  
} scheduler_rm_item_t;

typedef struct {  
	TimestampTz started;
	TimestampTz stop_it;

	job_t  *job;

	pid_t pid;
	BackgroundWorkerHandle *handler;
	dsm_segment *shared;
	bool wait_worker_to_die;
} scheduler_manager_slot_t;

typedef struct {
	char *database;
	char *nodename;

	TimestampTz next_at_time;
	TimestampTz next_checkjob_time;
	TimestampTz next_expire_time;
	
	int free_slots;
	int slots_len;
	scheduler_manager_slot_t **slots;

	dsm_segment *seg;
} scheduler_manager_ctx_t;

int checkSchedulerNamespace(void);
extern PGDLLEXPORT void manager_worker_main(Datum arg);
int get_scheduler_maxworkers(void);
char *get_scheduler_nodename(void);
scheduler_manager_ctx_t *initialize_scheduler_manager_context(char *dbname, dsm_segment *seg);
int refresh_scheduler_manager_context(scheduler_manager_ctx_t *ctx);
void destroy_scheduler_manager_context(scheduler_manager_ctx_t *ctx);
int scheduler_manager_stop(scheduler_manager_ctx_t *ctx);
scheduler_task_t *scheduler_get_active_tasks(scheduler_manager_ctx_t *ctx, int *nt);
bool jsonb_has_key(Jsonb *J, const char *name);
bool _is_in_rule_array(Jsonb *J, const char *name, int value);
TimestampTz *scheduler_calc_next_task_time(scheduler_task_t *task, TimestampTz start, TimestampTz stop, int first_time, int *ntimes);
int scheduler_make_at_record(scheduler_manager_ctx_t *ctx);
bit_array_t *convert_rule_to_cron(Jsonb *J, bit_array_t *cron);
void fill_cron_array_from_rule(Jsonb *J, const char *name, bit_array_t *ce, int len, int start);
bool is_cron_fit_timestamp(bit_array_t *cron, TimestampTz timestamp);
char **get_dates_array_from_rule(scheduler_task_t *task, int *num);
int get_integer_from_jsonbval(JsonbValue *ai, int def);
int scheduler_vanish_expired_jobs(scheduler_manager_ctx_t *ctx);
int how_many_instances_on_work(scheduler_manager_ctx_t *ctx, int cron_id);
int insert_at_record(char *nodename, int cron_id, TimestampTz start_at, TimestampTz postpone, char **error);
int set_job_on_free_slot(scheduler_manager_ctx_t *ctx, job_t *job);
int scheduler_start_jobs(scheduler_manager_ctx_t *ctx);
int scheduler_check_slots(scheduler_manager_ctx_t *ctx);
void destroy_slot_item(scheduler_manager_slot_t *item);
int launch_executor_worker(scheduler_manager_ctx_t *ctx, scheduler_manager_slot_t *item);
void clean_at_table(scheduler_manager_ctx_t *ctx);
int update_cron_texttime(scheduler_manager_ctx_t *ctx, int cron_id, TimestampTz next);
int mark_job_broken(scheduler_manager_ctx_t *ctx, int cron_id, char *reason);
void manager_fatal_error(scheduler_manager_ctx_t *ctx, int ecode, char *message, ...) pg_attribute_printf(3, 4);
void set_slots_stat_report(scheduler_manager_ctx_t *ctx);
bool check_parent_stop_signal(scheduler_manager_ctx_t *ctx);

#endif
