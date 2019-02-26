/* ------------------------------------------------------------------------
 *
 * connection.c
 *		This module contains implementation of EXCHANGE message passing
 *		interface.
 *
 * Copyright (c) 2018, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "postgres.h"

#include "common/ip.h"
#include "libpq/libpq.h"
#include "libpq-fe.h"
#include "utils/memutils.h"
#include "utils/varlena.h"

#include "common.h"
#include "connection.h"

#include "stdio.h"
#include "unistd.h"

typedef PGconn* ppgconn;


MemoryContext	ParGRES_context;

/* Parameters of coordinator for current query */
int			CoordinatorPort;
pgsocket	CoordSock = PGINVALID_SOCKET;
in_addr_t	pargres_hosts[NODES_MAX_NUM];
in_addr_t	pargres_ports[NODES_MAX_NUM];
List		*pargres_host_names = NULL;
ConnInfoPool ProcessSharedConnInfoPool;

/*
 * Connection descriptors of Coordinator node for a system message passing.
 * ServiceSock[CoordinatorNode] setup into listening mode.
 * Other ServiceSock[] values contains fd of opened socket or PGINVALID_SOCKET.
 */
pgsocket	ServiceSock[NODES_MAX_NUM];

static ppgconn *conn = NULL;

ConnInfo	*BackendConnInfo = NULL;


static int _select(int nfds, fd_set *readfds, fd_set *writefds,
				   struct timeval *timeout);
static int _accept(pgsocket socket, struct sockaddr *addr,
				   socklen_t *length_ptr);
static int _send(int socket, void *buffer, size_t size, int flags);
static int _recv(int socket, void *buffer, size_t size, int flags);

#define HOST_NAME(node)	((char *)(list_nth(pargres_host_names, node)))
#define PORT_NUM(node)	(pargres_ports[node])

void
CONN_Init_module(void)
{
	int			node = 0;
	char		*rawstr;
	ListCell	*l;
	MemoryContext	oldCxt;
	List		*pargres_port_names;

	ParGRES_context = AllocSetContextCreate(TopMemoryContext,
												"PargresMemoryContext",
												ALLOCSET_DEFAULT_SIZES);
	oldCxt = MemoryContextSwitchTo(ParGRES_context);

	Assert(nodes_at_cluster <= NODES_MAX_NUM);

	/* Now pargres_hosts_string need to be parse into array */
	rawstr = pstrdup(pargres_hosts_string);
	if(!SplitIdentifierString(rawstr, ',', &pargres_host_names))
		elog(ERROR, "Bogus hosts string format: %s", rawstr);

	for ((l) = list_head(pargres_host_names); (l) != NULL; (l) = lnext(l))
	{
		char	*address = (char *) lfirst(l);
		struct hostent* server;

		Assert(node < nodes_at_cluster);

		if ((server = gethostbyname(address)) == NULL)
			elog(ERROR,"Unknown host: %s\n", address);

		pargres_hosts[node] =
							((struct in_addr *)server->h_addr_list[0])->s_addr;
		node++;
	}

	/* Parse ports numbers to an array */
	node = 0;
	rawstr = pstrdup(pargres_ports_string);
	if(!SplitIdentifierString(rawstr, ',', &pargres_port_names))
		elog(ERROR, "Bogus ports string format: %s", rawstr);

	for ((l) = list_head(pargres_port_names); (l) != NULL; (l) = lnext(l))
	{
		Assert(node < nodes_at_cluster);
		sscanf((char *) lfirst(l), "%d", &pargres_ports[node]);
		node++;
	}

	MemoryContextSwitchTo(oldCxt);
}

/*
 * Create, bind and listen socket for the port number.
 * If port == 0, allocate it by system-based mechanism.
 * Returns the port, binded with socket.
 */
int
ListenPort(int port, pgsocket *sock)
{
	struct sockaddr_in	addr;
	socklen_t			addrlen = sizeof(addr);
	int					actual_port_num;
	int					res;

	Assert(sock != NULL);
	Assert(port >= 0);

	addr.sin_family = AF_INET;
	addr.sin_port =  htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (*sock == PGINVALID_SOCKET)
	{
		int	optval = 1;
		int counter = 0;

		*sock = socket(addr.sin_family, SOCK_STREAM, 0);
		if (*sock < 0)
			perror("Error on socket creation");

		if(setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &optval,
															sizeof(optval)) < 0)
		    perror("Reusing ADDR failed");

		for (;;)
		{
			counter++;
			res = bind(*sock, (struct sockaddr *)&addr, sizeof(addr));

			if ((res < 0) && (errno == EADDRINUSE))
			{
				perror("Error on socket binding");
				Assert(counter < 10);
				continue;
			}
			else if (res < 0)
			{
				perror("Error on socket binding");
				return -1;
			}

			break;
		}

		if (listen(*sock, 1024) != 0)
		{
			perror("Error on socket listening");
			return -1;
		}
	}

	getsockname(*sock, (struct sockaddr *)&addr, &addrlen);
	actual_port_num = ntohs(addr.sin_port);
	Assert(actual_port_num > 0);

	if ((port > 0) && (port != actual_port_num))
		elog(ERROR,
			 "input port number %d not correspond to actual port number %d",
			 port, actual_port_num);

	if (!pg_set_noblock(*sock))
		elog(ERROR, "Nonblocking socket failed. ");

	return actual_port_num;
}

/*
 * Low-level routine to establish connection with server, defined by the pair
 * (host, port).
 * It ignore OS core signals (EINTR) and infinitely wait, while server set
 * port to listen mode and accept the connection.
 * The routine log another error messages and returns socket.
 */
pgsocket
CONN_Connect(int port, in_addr_t host)
{
	pgsocket			sock;
	struct sockaddr_in	addr;
	int					res;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
		perror("ERROR on connect");

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = host;

	do
	{
		res = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	} while (res < 0 && ((errno == EINTR) || (errno == ECONNREFUSED)));

	if (res < 0)
		perror("CONNECT");

	return sock;
}

void
InstanceConnectionsSetup(void)
{
	int i;
	for (i = 0; i < nodes_at_cluster; i++)
		ServiceSock[i] = -1;

	CoordinatorPort = ListenPort(0, &ServiceSock[node_number]);

	if (CoordinatorPort <= 0)
		elog(ERROR,
			 "Incorrect Service Port number: %d", CoordinatorPort);
	Assert(ServiceSock[node_number] > 0);

	PostmasterConnectionsSetup();
	QueryExecutionInitialize(CoordinatorPort);
}

/*
 * Initialize connections to all another instances
 */
int
PostmasterConnectionsSetup(void)
{
	int node;
	char conninfo[STRING_SIZE_MAX];

	if (!conn)
		/* Also, which set conn[node] to NULL value*/
		conn = MemoryContextAllocZero(ParGRES_context,
											sizeof(ppgconn)*nodes_at_cluster);

	Assert(conn[node_number] == NULL);

	for (node = 0; node < nodes_at_cluster; node++)
	{
		if ((node != node_number) && (conn[node] == NULL))
		{
			sprintf(conninfo, "host=%s port=%d%c", HOST_NAME(node),
														PORT_NUM(node), '\0');
			conn[node] = PQconnectdb(conninfo);
			if (PQstatus(conn[node]) == CONNECTION_BAD)
			{
				elog(LOG, "Connection error. conninfo: %s", conninfo);
				return -1;
			}
		}
	}
	return 0;
}

static void
accept_connections(pgsocket sock, int cnum, pgsocket *incoming_socks)
{
	fd_set readset;

	Assert(sock > 0);
	Assert(incoming_socks != NULL);

	for (; (cnum > 0); )
	{
		FD_ZERO(&readset);
		FD_SET(sock, &readset);

		if(_select(sock+1, &readset, NULL, NULL) <= 0)
			perror("select");

		Assert(FD_ISSET(sock, &readset));

		cnum--;
		incoming_socks[cnum] = _accept(sock, NULL, NULL);

		if (incoming_socks[cnum] < 0)
			perror("  accept() failed");

		if (!pg_set_noblock(incoming_socks[cnum]))
			elog(ERROR, "Nonblocking socket failed. ");
	}
}

void
ServiceConnectionSetup(void)
{
	if (CoordNode == node_number)
	{
		pgsocket	*isocks = palloc((nodes_at_cluster-1)*sizeof(pgsocket));
		int			i;

		accept_connections(ServiceSock[node_number], nodes_at_cluster-1, isocks);
		for (i = 0; i < nodes_at_cluster-1; i++)
		{
			int nodenum;

			CONN_Recv(isocks, nodes_at_cluster-1, &nodenum, sizeof(int));
			Assert(nodenum != node_number);
			ServiceSock[nodenum] = isocks[i];
		}

		pfree(isocks);

		CoordSock = PGINVALID_SOCKET;
		CoordinatorPort = -1;
	}
	else
	{
		CoordSock = CONN_Connect(CoordinatorPort, pargres_hosts[CoordNode]);
		Assert(CoordSock > 0);
		CONN_Send(CoordSock, &node_number, sizeof(int));
	}
}

int
QueryExecutionInitialize(int port)
{
	int node;
	char command[STRING_SIZE_MAX];

	Assert(conn != NULL);

	sprintf(command, "SELECT set_query_id(%d, %d);%c", node_number, port, '\0');

	for (node = 0; node < nodes_at_cluster; node++)
	{
		int status;

		if (node == node_number)
			continue;

		Assert(conn[node] != NULL);
		status = PQsendQuery(conn[node], command);

		if (status == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn[node]));
		Assert(status > 0);
	}

	ServiceConnectionSetup();
	CONN_Check_query_result();

	return 0;
}

int
CONN_Launch_query(const char *query)
{
	int node;

	for (node = 0; node < nodes_at_cluster; node++)
	{
		int	result;

		if (node == node_number)
			continue;

		result = PQsendQuery(conn[node], query);

		if (result == 0)
			elog(ERROR, "Query sending error: %s", PQerrorMessage(conn[node]));

		Assert(result >= 0);
	}

	return 0;
}

void
CONN_Check_query_result(void)
{
	int node;
	PGresult *result;

	if (!conn)
		return;

	do
	{
		for (node = 0; node < nodes_at_cluster; node++)
		{
			if (conn[node] == NULL)
				continue;

			if ((result = PQgetResult(conn[node])) != NULL)
			{
				elog(LOG, "[%d]: %s", node, PQcmdStatus(result));
				Assert(PQresultStatus(result) != PGRES_FATAL_ERROR);
				break;
			}
		}
	} while (result);
}

static int
sendall(int s, char *buf, int len, int flags)
{
    int total = 0;
    int n;

    while(total < len)
    {
        n = _send(s, buf+total, len-total, flags);
        if(n == -1) { break; }
        total += n;
    }

    return (n==-1 ? -1 : total);
}

static pgsocket BackendExchangeListenSock = PGINVALID_SOCKET;

void
CONN_Init_exchange(ConnInfo *pool, ex_conn_t *exconn, int mynum, int nnodes)
{
	int			node;
	pgsocket	*incoming_socks = palloc((nnodes - 1) * sizeof(pgsocket));

	Assert(pool != NULL);
//	elog(LOG, "Start CONN_Init_exchange");
	exconn->rsock = palloc(sizeof(pgsocket) * nnodes);
	exconn->rsIsOpened = palloc(sizeof(pgsocket) * nnodes);
	exconn->wsock = palloc(sizeof(pgsocket) * nnodes);
	exconn->wsIsOpened = palloc(sizeof(pgsocket) * nnodes);
	exconn->rsock[mynum] = PGINVALID_SOCKET;
	exconn->rsIsOpened[mynum] = false;
	exconn->wsock[mynum] = PGINVALID_SOCKET;
	exconn->wsIsOpened[mynum] = false;

	if (BackendExchangeListenSock == PGINVALID_SOCKET)
		ListenPort(pool->port[mynum], &BackendExchangeListenSock);

	/* Init sockets for connection for foreign servers */
	for (node = 0; node < nnodes; node++)
	{
		if (node == node_number)
			continue;
		exconn->wsock[node] = CONN_Connect(pool->port[node], pargres_hosts[node]);
		Assert(exconn->wsock[node] > 0);
	}

	accept_connections(BackendExchangeListenSock, nnodes-1, incoming_socks);
	for (node = 0; node < nnodes; node++)
	{
		if (node == node_number)
			continue;

		exconn->wsIsOpened[node] = true;
		CONN_Send(exconn->wsock[node], &node_number, sizeof(int));
	}

	for (node = 0; node < nnodes-1; node++)
	{
		int nodenum;

		CONN_Recv(&incoming_socks[node], 1, &nodenum, sizeof(int));

		Assert(nodenum != node_number);
		exconn->rsock[nodenum] = incoming_socks[node];
		exconn->rsIsOpened[nodenum] = true;
	}

	pfree(incoming_socks);
}

void
OnExecutionEnd(void)
{
	if (BackendExchangeListenSock != PGINVALID_SOCKET)
	{
		if (closesocket(BackendExchangeListenSock) < 0)
			perror("CLOSE");
		BackendExchangeListenSock = PGINVALID_SOCKET;

		Assert((BackendConnInfo->port[node_number] > 0) &&
			   (BackendConnInfo->port[node_number] < PG_UINT16_MAX));
		STACK_Push(PORTS, BackendConnInfo->port[node_number]);
		BackendConnInfo = NULL;
	}
	else
	{
		/* For EXPLAIN operation */
	}
}

void
CONN_Exchange_close(ex_conn_t *conn)
{
	int node;

	Assert(conn != NULL);

	for (node = 0; node < nodes_at_cluster; node++)
	{
		char close_sig = 'C';

		if (conn->wsIsOpened[node] == false)
			continue;

		Assert(conn->wsock[node] > 0);
		CONN_Send(conn->wsock[node], &close_sig, 1);
		conn->wsIsOpened[node] = false;
	}
}

int
CONN_Send(pgsocket sock, void *buf, int size)
{
	Assert(sock != PGINVALID_SOCKET);
	if (sendall(sock, (char *)buf, size, 0) < 0)
	{
		elog(LOG, "sock: %d", sock);
		perror("SEND Error");
		return -1;
	}

	return 0;
}

static int
_select(int nfds, fd_set *readfds, fd_set *writefds,
				   struct timeval *timeout)
{
	int res;

	do
	{
		res = select(nfds, readfds, writefds, NULL, timeout);
	} while (res < 0 && errno == EINTR);

	return res;
}

static int
_accept(pgsocket socket, struct sockaddr *addr, socklen_t *length_ptr)
{
	int res;

	do
	{
		res = accept(socket, addr, length_ptr);
	} while (res < 0 && errno == EINTR);

	return res;
}

static int
_send(int socket, void *buffer, size_t size, int flags)
{
	int res;

	do
	{
		res = send(socket, buffer, size, flags);
	} while (res < 0 && errno == EINTR);

	return res;
}

static int
_recv(int socket, void *buffer, size_t size, int flags)
{
	int res;

	do
	{
		res = recv(socket, buffer, size, flags);
	} while (res < 0 && errno == EINTR);

	return res;
}

int
CONN_Recv(pgsocket *socks, int nsocks, void *buf, int expected_size)
{
	fd_set	readset;
	int		high_sock = 0;
	int		i;

	FD_ZERO(&readset);
	for (i = 0; i < nsocks; i++)
	{
		Assert(socks[i] > 0);
		FD_SET(socks[i], &readset);

		if (high_sock < socks[i])
			high_sock = socks[i];
	}

	if(_select(high_sock+1, &readset, NULL, NULL) <= 0)
		perror("RECV Select error");

	for (i = 0; (i < nsocks) && !FD_ISSET(socks[i], &readset); i++);
	Assert(i < nsocks);

	return _recv(socks[i], buf, expected_size, 0);
}

/*
 * Receive a tuple from any other EXCHANGE instances. One byte length message is
 * a "End of Tuples" command.
 * This function returns iff message was arrived.
 */
HeapTuple
CONN_Recv_tuple(pgsocket *socks, bool *isopened,  int *res)
{
	int				i;
	struct timeval	timeout;
	HeapTupleData	htHeader;
	HeapTuple		tuple;
	fd_set			readset;
	int				counter = 0;

	Assert(socks != NULL);
	Assert(isopened != NULL);
	Assert(res != NULL);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	/*
	 * In this cycle we wait one from events:
	 * 1. Tuple arrived, return it to the caller immediately.
	 * 2. high_sock == 0: all connections closed. Returns -1.
	 * 3. No messages received. Returns 0.
	 */
	for (;;)
	{
		int high_sock = 0;

		FD_ZERO(&readset);

		for (i = 0; i < nodes_at_cluster; i++)
		{
			if (!isopened[i])
				continue;

			Assert(socks[i] > 0);
			if (high_sock < socks[i])
				high_sock = socks[i];

			FD_SET(socks[i], &readset);
		}

		/* We have any open incoming connections? */
		if (high_sock == 0)
		{
			*res = -2;
			return NULL;
		}

		/* Check state of all incloming sockets without timeout */
		*res = _select(high_sock+1, &readset, NULL, &timeout);

		if (*res < 0)
		{
			perror("SELECT ERROR");
		}
		else if (*res == 0)
			/* No one message was arrived */
			return NULL;

		/* Search for socket triggered */
		for (i = 0; i < nodes_at_cluster; i++)
		{
			if (!isopened[i])
				continue;

			if (FD_ISSET(socks[i], &readset))
			{
				if ((*res = _recv(socks[i], &htHeader,
								  offsetof(HeapTupleData, t_data), 0)) > 1)
				{
					int res1;

					FD_ZERO(&readset);
					FD_SET(socks[i], &readset);

					/* Wait for 'Body' of the tuple */
					if ((res1 = _select(socks[i]+1, &readset, NULL, NULL)) <= 0)
						Assert(0);

					tuple = (HeapTuple) palloc0(HEAPTUPLESIZE + htHeader.t_len);
					memcpy(tuple, &htHeader, HEAPTUPLESIZE);
					tuple->t_data = (HeapTupleHeader)
											((char *) tuple + HEAPTUPLESIZE);
					res1 = _recv(socks[i], tuple->t_data, tuple->t_len, 0);
					Assert(res1 == tuple->t_len);
					*res += res1;
					return tuple;
				}
				else if (*res == 1)
				{
//					elog(LOG, "READ SOCK Close: %d (%d)", socks[i], i);
					isopened[i] = false;
					continue;
				}
				else if (*res < 0)
					perror("RECEIVE ERROR");
				else
					Assert(0);
			}
		}
		pg_usleep(1);
		if (++counter >= 1000000)
			elog(ERROR, "Exchange receiving timeout was exceeded.");
	}

	return NULL;
}

ConnInfo*
GetConnInfo(ConnInfoPool *pool)
{
	int current;

	Assert(pool != NULL);

	current = pg_atomic_fetch_add_u32(&pool->current, 1);
	Assert(current != pool->size);
	return &pool->info[current];
}

/*
 * Call by Leader backend at initialization process of shared memory for
 * parallel workers.
 */
void
CreateConnectionPool(ConnInfoPool *pool, int nconns, int nnodes, int mynode)
{
	int i;

	Assert(pool != NULL);
	Assert(nconns > 0);
	Assert(nnodes > 0);
	Assert((mynode >= 0) && (mynode < nnodes));

	pool->CoordinatorNode = CoordNode;

	for (i = 0; i < nconns; i++)
	{
		int j;

		if (mynode == CoordNode)
		{
			pool->info[i].port[mynode] = STACK_Pop(PORTS);

			Assert((pool->info[i].port[mynode] > 0) &&
				   (pool->info[i].port[mynode] < PG_UINT16_MAX));

			for (j = 0; j < nnodes; j++)
			{
				if (j == mynode)
					continue;

				CONN_Recv(&ServiceSock[j], 1, &(pool->info[i].port[j]),
																sizeof(int));
			}

			for (j = 0; j < nnodes; j++)
			{
				if (j == node_number)
					continue;
				Assert(ServiceSock[j] > 0);
				CONN_Send(ServiceSock[j], pool->info[i].port, nnodes*sizeof(int));
			}
		}
		else
		{
			int port = STACK_Pop(PORTS);

			Assert((port > 0) && (port < PG_UINT16_MAX));
			Assert(CoordSock > 0);

			CONN_Send(CoordSock, &port, sizeof(int));
			CONN_Recv(&CoordSock, 1, pool->info[i].port, nnodes*sizeof(int));
		}
	}
	pg_atomic_write_u32(&pool->current, 0);
	pool->size = nconns;
}
