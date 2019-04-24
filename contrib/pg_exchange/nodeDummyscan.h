/*-------------------------------------------------------------------------
 *
 * nodeDummyscan.h
 *
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/executor/nodeDummyscan.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEDUMMYSCAN_H
#define NODEDUMMYSCAN_H

#include "access/parallel.h"
#include "nodes/extensible.h"
//#include "nodes/execnodes.h"

typedef Scan DummyScan;

typedef struct
{
	CustomScanState	css;
} DummyScanState;

extern void DUMMYSCAN_Init_methods(void);
extern CustomScan *make_dummyscan(Index scanrelid);

#endif							/* NODEDUMMYSCAN_H */
