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
	PlannedStmt *pstmt;
	int eflags;
	Bitmapset *servers;
	IndexOptInfo *indexinfo;
	List	*foreign_scans;
} lcontext;


extern bool enable_distributed_execution;
extern char destsName[10];
#define DISTEXECPATHNAME	"DistExecPath"

#define IsDistExecNode(pathnode) ((((Path *) pathnode)->pathtype == T_CustomScan) && \
	(strcmp(((CustomPath *)pathnode)->methods->CustomName, DISTEXECPATHNAME) == 0))

extern Bitmapset *extractForeignServers(CustomPath *path);
extern void DistExec_Init_methods(void);
extern CustomScan *make_distplanexec(List *custom_plans, List *tlist, List *private_data);
extern CustomPath *create_distexec_path(PlannerInfo *root, RelOptInfo *rel,
								  Path *children, Bitmapset *servers);
extern bool localize_plan(Plan *node, lcontext *context);
extern void FSExtractServerName(Oid fsid, char **host, int *port);
extern char *GetMyServerName(int *port);
extern char *serializeServer(const char *host, int port);
extern void EstablishDMQConnections(const lcontext *context,
									const char *serverName,
									EState *estate,
									PlanState *substate);

#endif							/* NODEDISTPLANEXEC_H */
