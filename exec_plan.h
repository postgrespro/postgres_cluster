/*
 * exec_plan.h
 *
 */

#ifndef CONTRIB_EXECPLAN_EXEC_PLAN_H_
#define CONTRIB_EXECPLAN_EXEC_PLAN_H_

#include "postgres.h"

#include "executor/executor.h"
#include "libpq-fe.h"

void execute_query(PGconn *dest, QueryDesc *queryDesc, int eflags);

#endif /* CONTRIB_EXECPLAN_EXEC_PLAN_H_ */
