/* ------------------------------------------------------------------------
 *
 * common.c
 *		Common code for ParGRES extension
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/genam.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/indexing.h"
#include "catalog/pg_extension.h"
#include "commands/extension.h"
#include "utils/fmgroids.h"

#include "common.h"


ExchangeSharedState *ExchShmem = NULL;
