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


extern void worker_spi_sighup(SIGNAL_ARGS);
extern void worker_spi_sigterm(SIGNAL_ARGS);

void pg_scheduler_startup(void);
char_array_t *readBasesToCheck(void);
extern Datum cron_string_to_json_text(PG_FUNCTION_ARGS);
void _PG_init(void);
void parent_scheduler_main(Datum) pg_attribute_noreturn();
int checkSchedulerNamespace(void);
void manager_worker_main(Datum arg);
pid_t registerManagerWorker(schd_manager_t *man);

TimestampTz timestamp_add_seconds(TimestampTz to, int add);
char *make_date_from_timestamp(TimestampTz ts);
int get_integer_from_string(char *s, int start, int len);
TimestampTz get_timestamp_from_string(char *str);
TimestampTz _round_timestamp_to_minute(TimestampTz ts);

#endif
