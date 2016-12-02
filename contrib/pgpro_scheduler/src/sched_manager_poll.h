#ifndef PGPRO_SCHEDULER_MANAGER_POLL_H
#define PGPRO_SCHEDULER_MANAGER_POLL_H

#include <stdlib.h>
#include "postgres.h"
#include "char_array.h"
#include "postmaster/bgworker.h"
#include "storage/dsm.h"

typedef enum {
	SchdManagerInit,
	SchdManagerConnected,
	SchdManagerQuit,
	SchdManagerStop,
	SchdManagerDie
} schd_manager_status_t;

typedef struct {
	schd_manager_status_t status;
	bool setbyparent;
	bool setbychild;
} schd_manager_share_t;

typedef struct {
	char *dbname;
	bool connected;
	pid_t pid;
	BackgroundWorkerHandle *handler;
	dsm_segment *shared;
} schd_manager_t;

typedef struct {
	int n;
	schd_manager_t **workers;
	bool enabled;
} schd_managers_poll_t;

void changeChildBgwState(schd_manager_share_t *, schd_manager_status_t);
int __cmp_managers(const void *a, const void *b);
schd_managers_poll_t *initSchedulerManagerPool(char_array_t *names);
void destroyManagerRecord(schd_manager_t *man);
int stopAllManagers(schd_managers_poll_t *poll);
int isBaseListChanged(char_array_t *names, schd_managers_poll_t *pool);
void _sortPollManagers(schd_managers_poll_t *poll);
int removeManagerFromPoll(schd_managers_poll_t *poll, char *name, char sort, bool stop_worker);
int addManagerToPoll(schd_managers_poll_t *poll, char *name, int sort);
int refreshManagers(char_array_t *names, schd_managers_poll_t *poll);
char *poll_dbnames(schd_managers_poll_t *poll);
pid_t registerManagerWorker(schd_manager_t *man);
void set_supervisor_pgstatus(schd_managers_poll_t *poll);
char *supervisor_state(schd_managers_poll_t *poll);

#endif



