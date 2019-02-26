/*-------------------------------------------------------------------------
 *
 * connection.h
 *	Network layer functions
 *
 * Copyright (c) 2018, PostgreSQL Global Development Group
 * Author: Andrey Lepikhov <a.lepikhov@postgrespro.ru>
 *
 * IDENTIFICATION
 *	contrib/pargres/connection.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "access/htup.h"
#include "port/atomics.h"


#define NODES_MAX_NUM	(1024)
#define STRING_SIZE_MAX	(1024)



typedef struct
{
	int	port[NODES_MAX_NUM];
} ConnInfo;

#define POOL_MAX_SIZE		(10)

typedef struct
{
	int					size;
	pg_atomic_uint32	current;
	int					CoordinatorNode;
	ConnInfo			info[POOL_MAX_SIZE];
} ConnInfoPool;

typedef struct
{
	pgsocket	*rsock; /* incoming messages */
	bool		*rsIsOpened;
	pgsocket	*wsock; /* outcoming messages */
	bool		*wsIsOpened;
} ex_conn_t;

extern ConnInfo	*BackendConnInfo;
extern int		CoordinatorPort;
extern pgsocket	CoordSock;
extern pgsocket	ServiceSock[NODES_MAX_NUM];
extern ConnInfoPool ProcessSharedConnInfoPool;


extern void CONN_Init_module(void);
extern void InstanceConnectionsSetup(void);
extern int ListenPort(int port, pgsocket *sock);
extern pgsocket CONN_Connect(int port, in_addr_t host);
extern int PostmasterConnectionsSetup(void);
extern int QueryExecutionInitialize(int port);
extern int CONN_Launch_query(const char *query);
extern void CONN_Check_query_result(void);
extern void CONN_Init_exchange(ConnInfo *pool, ex_conn_t *exconn, int mynum,
																  int nnodes);
extern void CONN_Exchange_close(ex_conn_t *conn);
extern int CONN_Send(pgsocket sock, void *buf, int size);
extern int CONN_Recv(pgsocket *socks, int nsocks, void *buf, int expected_size);
extern HeapTuple CONN_Recv_tuple(pgsocket *socks, bool *isopened, int *res);
extern void ServiceConnectionSetup(void);
extern void OnExecutionEnd(void);
extern ConnInfo* GetConnInfo(ConnInfoPool *pool);
extern void CreateConnectionPool(ConnInfoPool *pool, int nconns, int nnodes, int mynode);

#endif /* CONNECTION_H_ */
