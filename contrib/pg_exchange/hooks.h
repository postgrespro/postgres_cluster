/* ------------------------------------------------------------------------
 *
 * hooks_exec.h
 *		Executor-related logic of the ParGRES extension.
 *
 * Copyright (c) 2018, Postgres Professional
 * Author: Andrey Lepikhov <a.lepikhov@postgrespro.ru>
 *
 * IDENTIFICATION
 *	contrib/pargres/hooks_exec.h
 *
 * ------------------------------------------------------------------------
 */

#ifndef HOOKS_EXEC_H_
#define HOOKS_EXEC_H_

#include "executor/executor.h"

extern void EXEC_Hooks_init(void);

#endif /* HOOKS_EXEC_H_ */
