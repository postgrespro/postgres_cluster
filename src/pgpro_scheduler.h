#ifndef PGPRO_SCHEDULER_H
#define PGPRO_SCHEDULER_H

#include "postgres.h"
#include "char_array.h"
#include "sched_manager_poll.h"
#include "utils/datetime.h"

#define PGPRO_SHM_TOC_MAGIC 0x50310
#define PGPRO_SCHEDULER_DBNAME_MAX 128
#define PGPRO_SCHEDULER_NODENAME_MAX 128
#define PGPRO_SCHEDULER_EXECUTOR_MESSAGE_MAX 1024
#define PPGS_NODEBUG 1

#ifdef PPGS_DEBUG 
#ifdef HAVE__VA_ARGS
#define _pdebug(...) elog(LOG, __VA_ARGS__);
#else
#define _pdebug(...)
#endif
#else
#define _pdebug(...)
#endif

extern void worker_spi_sighup(SIGNAL_ARGS);
extern void worker_spi_sigterm(SIGNAL_ARGS);

void pg_scheduler_startup(void);
char_array_t *readBasesToCheck(void);
extern Datum cron_string_to_json_text(PG_FUNCTION_ARGS);
void _PG_init(void);
extern PGDLLEXPORT void parent_scheduler_main(Datum) pg_attribute_noreturn();
int checkSchedulerNamespace(void);
pid_t registerManagerWorker(schd_manager_t *man);

void reload_db_role_config(char *dbname);
TimestampTz timestamp_add_seconds(TimestampTz to, int add);
char *make_date_from_timestamp(TimestampTz ts, bool hires);
int get_integer_from_string(char *s, int start, int len);
TimestampTz get_timestamp_from_string(char *str);
TimestampTz _round_timestamp_to_minute(TimestampTz ts);
bool is_scheduler_enabled(void);

#endif
