/*-------------------------------------------------------------------------
 *
 * nodeDistPlanExec.h
 *
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEDISTPLANEXEC_H
#define NODEDISTPLANEXEC_H

#include "access/parallel.h"
#include "nodes/extensible.h"

typedef Scan DistPlanExec;

typedef struct
{
	EState *estate;
	int eflags;
	Bitmapset *servers;
} lcontext;


extern char destsName[10];
#define DISTEXECPATHNAME	"DistExecPath"

extern void DistExec_Init_methods(void);
extern CustomScan *make_distplanexec(List *custom_plans, List *tlist, List *private_data);
extern Path *create_distexec_path(PlannerInfo *root, RelOptInfo *rel,
								  Path *children, Bitmapset *servers);
extern bool localize_plan(PlanState *node, lcontext *context);
extern void FSExtractServerName(Oid fsid, char **host, int *port);
extern void GetMyServerName(char **host, int *port);
extern char *serializeServer(const char *host, int port);
extern void EstablishDMQConnections(const lcontext *context, const char *serverName);

#endif							/* NODEDISTPLANEXEC_H */
