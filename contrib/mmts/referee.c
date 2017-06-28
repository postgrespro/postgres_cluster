/*
 * arbitraror.c
 *
 * Referee for multimaster (make it possible to work out of quorum)
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "libpq-fe.h"
#include "postmaster/bgworker.h"
#include "storage/latch.h"
#include "storage/proc.h"
#include "storage/ipc.h"

#include "multimaster.h"

#define REFEREE_RECONNECT_TIMEOUT 10 

static bool MtmRefereeStop;

static void MtmShutdownReferee(int sig)
{
	MtmRefereeStop = true;
}

static void MtmRefereeLoop(char const** connections, int nConns)
{
	PGconn* conns[MAX_NODES];
	int i;
	PGresult *res;
	ConnStatusType status;
	nodemask_t disabledMask = 0;
	nodemask_t newEnabledMask = 0;
	nodemask_t oldEnabledMask = 0;
	int result;
	
	for (i = 0; i < nConns; i++) {
		conns[i] = PQconnectdb_safe(connections[i]);
		status = PQstatus(conns[i]);
		if (status != CONNECTION_OK)
		{
			MTM_ELOG(LOG, "Could not establish connection to node %d, error = %s",
					 i+1, PQerrorMessage(conns[i]));
			PQfinish(conns[i]);
			conns[i] = NULL;
			sleep(REFEREE_RECONNECT_TIMEOUT);
			i -= 1;
		} else {
			PQsetnonblocking(conns[i], 1);
		}
	}

	while (!MtmRefereeStop) { 
		char sql[128];
		sprintf(sql, "select mtm.referee_poll(%lld)", disabledMask);

		/* Initiate queries to all live nodes */
        for (i = 0; i < nConns; i++) {        
			/* Some of live node reestablished connection with dead node, so referee should also try to connect to this node */
			if (conns[i] == NULL) { 
				if (BIT_CHECK(newEnabledMask, i)) { 
					conns[i] = PQconnectdb_safe(connections[i]);
					status = PQstatus(conns[i]);
					if (status == CONNECTION_OK) {
						BIT_CLEAR(disabledMask, i);
						MTM_ELOG(LOG, "Reestablish connection with node %d", i+1);
						PQsetnonblocking(conns[i], 1);
					} else {
						PQfinish(conns[i]);
						conns[i] = NULL;
					}
				}
			} else { 
				if (!PQsendQuery(conns[i], sql)) { 
					MTM_ELOG(LOG, "Failed to send query to node %d, error = %s",
									   i+1, PQerrorMessage(conns[i]));
					PQfinish(conns[i]);
					conns[i] = NULL;
					BIT_SET(disabledMask, i);
				}
			}
		}
		/* Wait some time */
		result = WaitLatch(&MyProc->procLatch, WL_TIMEOUT | WL_POSTMASTER_DEATH, MtmHeartbeatRecvTimeout);
		if (result & WL_POSTMASTER_DEATH) {
			proc_exit(1);
		}

		oldEnabledMask = newEnabledMask;
		newEnabledMask = ALL_BITS;
		for (i = 0; i < nConns; i++) {        
			if (conns[i] != NULL) {
				if (!PQconsumeInput(conns[i]) || PQisBusy(conns[i])) { 
					MTM_ELOG(LOG, "Doesn't receive response from node %d within %d milleseconds", i+1, MtmHeartbeatRecvTimeout);
				} else {
					res = PQgetResult(conns[i]);
					if (PQresultStatus(res) == PGRES_TUPLES_OK)
					{
						char* mask = PQgetvalue(res, 0, 0);
						newEnabledMask &= atol(mask);
						PQclear(res);
						PQgetResult(conns[i]);
						continue;
					}
					MTM_ELOG(LOG, "Failed to retrieve result from node %d: %s", i+1, PQresultErrorMessage(res));
					PQclear(res);
				}
				PQfinish(conns[i]);
				conns[i] = NULL;
				BIT_SET(disabledMask, i);
			}
		}
		if (newEnabledMask == ALL_BITS) { 
			if (oldEnabledMask != ALL_BITS) { 
				MTM_ELOG(LOG, "There are no more live nodes");
			}				
			/* No live nodes: referee should not alter quorum in this case */
			disabledMask = 0;
		} else { 
			if (newEnabledMask != oldEnabledMask) { 
				for (i = 0; i < nConns; i++) {        
					if (BIT_CHECK(newEnabledMask ^ oldEnabledMask, i)) { 					
						MTM_ELOG(LOG, "Node %d is %s\n", i+1, BIT_CHECK(newEnabledMask, i) ? "enabled" : "disabled");
					}
				}
			}	   
		}
	}
}

static void MtmRefereeMain(Datum arg)
{
	char const* connections[MAX_NODES];
	int i;

	pqsignal(SIGINT, MtmShutdownReferee);
	pqsignal(SIGQUIT, MtmShutdownReferee);
	pqsignal(SIGTERM, MtmShutdownReferee);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	for (i = 0; i < MtmNodes; i++) { 
		connections[i] = psprintf("%s  application_name=%s", MtmConnections[i].connStr, MULTIMASTER_ADMIN);
	}
	MtmRefereeLoop(connections, MtmNodes);
}


static BackgroundWorker MtmRefereeWorker = {
	"mtm-referee",
	0,
	BgWorkerStart_ConsistentState,
	REFEREE_RECONNECT_TIMEOUT,
	MtmRefereeMain
};


void MtmRefereeInitialize(void)
{
	RegisterBackgroundWorker(&MtmRefereeWorker);
}

