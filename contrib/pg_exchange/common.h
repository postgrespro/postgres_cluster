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
#include "dmq.h"


typedef struct
{
	Oid serverid;
	DmqDestinationId dest_id;
} DMQDestinations;

typedef struct
{
	int nservers;
	DMQDestinations *dests;
	int coordinator_num;
} DMQDestCont;

typedef struct
{
	LWLock	*lock;
	HTAB	*htab;
} ExchangeSharedState;

extern ExchangeSharedState *ExchShmem;

#endif /* COMMON_H_ */
