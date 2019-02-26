/*-------------------------------------------------------------------------
 *
 * common.h
 *	Common code for ParGRES extension
 *
 * Copyright (c) 2018, PostgreSQL Global Development Group
 * Author: Andrey Lepikhov <a.lepikhov@postgrespro.ru>
 *
 * IDENTIFICATION
 *	contrib/pargres/common.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "nodes/pg_list.h"
#include "storage/lock.h"


typedef struct
{
	LWLock	lock;
	int 	size;
	int 	index;
	int		values[FLEXIBLE_ARRAY_MEMBER];
} PortStack;


/* GUC variables */
extern int		node_number;
extern int		nodes_at_cluster;
extern char		*pargres_hosts_string;
extern char		*pargres_ports_string;
extern int		eports_pool_size;

extern PortStack *PORTS;
extern int CoordNode;
extern bool PargresInitialized;


extern Oid get_pargres_schema(void);
extern void STACK_Init(PortStack *stack, int range_min, int size);
int STACK_Pop(PortStack *stack);
void STACK_Push(PortStack *stack, int value);

#endif /* COMMON_H_ */
