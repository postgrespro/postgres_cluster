#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>
#include <pthread.h>
#include <libpq-fe.h>

typedef unsigned long long ulong64; /* we are not using uint64 here because we want to use %lld format for this type */

typedef ulong64 nodemask_t;

/*
#define referee_message(msg, ...) elog(LOG, msg,  ## __VA_ARGS__)
*/
#define referee_message(msg, ...) fprintf(stderr, msg "\n",  ## __VA_ARGS__)

#define BIT_CHECK(mask, bit) (((mask) & ((nodemask_t)1 << (bit))) != 0)
#define BIT_CLEAR(mask, bit) (mask &= ~((nodemask_t)1 << (bit)))
#define BIT_SET(mask, bit)   (mask |= ((nodemask_t)1 << (bit)))
#define ALL_BITS ((nodemask_t)~0)
#define MAX_NODES 64

int referee_loop(char const** connections, int nConns, time_t timeout)
{
	PGconn* conns[MAX_NODES];
	int i;
	PGresult *res;
	ConnStatusType status;
	nodemask_t disabledMask = 0;
	nodemask_t newEnabledMask = 0;
	nodemask_t oldEnabledMask = 0;

	
	for (i = 0; i < nConns; i++) {
		conns[i] = PQconnectdb(connections[i]);
		status = PQstatus(conns[i]);
		if (status != CONNECTION_OK)
		{
			referee_message("Could not establish connection to node %d, error = %s",
							   i+1, PQerrorMessage(conns[i]));
			return 1;
		}
		PQsetnonblocking(conns[i], 1);
	}

	while (1) { 
		char sql[128];
		sprintf(sql, "select mtm.referee_poll(%lld)", disabledMask);

		/* Initiate queries to all live nodes */
        for (i = 0; i < nConns; i++) {        
			/* Some of live node reestablished connection with dead node, so referee should also try to connect to this node */
			if (conns[i] == NULL) { 
				if (BIT_CHECK(newEnabledMask, i)) { 
					conns[i] = PQconnectdb(connections[i]);
					status = PQstatus(conns[i]);
					if (status == CONNECTION_OK) {
						BIT_CLEAR(disabledMask, i);
						referee_message("Reestablish connection with node %d", i+1);
						PQsetnonblocking(conns[i], 1);
					} else {
						PQfinish(conns[i]);
						conns[i] = NULL;
					}
				}
			} else { 
				if (!PQsendQuery(conns[i], sql)) { 
					referee_message("Failed to send query to node %d, error = %s",
									   i+1, PQerrorMessage(conns[i]));
					PQfinish(conns[i]);
					conns[i] = NULL;
					BIT_SET(disabledMask, i);
				}
			}
		}
		/* Wait some time */
		usleep(timeout);
		oldEnabledMask = newEnabledMask;
		newEnabledMask = ALL_BITS;
		for (i = 0; i < nConns; i++) {        
			if (conns[i] != NULL) {
				if (!PQconsumeInput(conns[i]) || PQisBusy(conns[i])) { 
					referee_message("Doesn't receive response from node %d within %ld microseconds", i+1, timeout);
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
					referee_message("Failed to retrieve result from node %d: %s", i+1, PQresultErrorMessage(res));
					PQclear(res);
				}
				PQfinish(conns[i]);
				conns[i] = NULL;
				BIT_SET(disabledMask, i);
			}
		}
		if (newEnabledMask == ALL_BITS) { 
			if (oldEnabledMask != ALL_BITS) { 
				referee_message("There are no more live nodes");
			}				
			/* No live nodes: referee should not alter quorum in this case */
			disabledMask = 0;
		} else { 
			if (newEnabledMask != oldEnabledMask) { 
				for (i = 0; i < nConns; i++) {        
					if (BIT_CHECK(newEnabledMask ^ oldEnabledMask, i)) { 					
						referee_message("Node %d is %s\n", i+1, BIT_CHECK(newEnabledMask, i) ? "enabled" : "disabled");
					}
				}
			}	   
		}
	}
}

int main (int argc, char* argv[])
{
	char const* connections[MAX_NODES];
	time_t timeout = 1000000;
	int nConns = 0;
	int i;

	if (argc == 1) {
        fprintf(stderr, "Use -h to show usage options\n");
        return 1;
    }

    for (i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) {
			  case 't':
				timeout = atol(argv[++i]);
				continue;
			  case 'c':
                connections[nConns++] = argv[++i];
                continue;
			}
        }
		printf("Options:\n"
			   "\t-t TIMEOUT\ttimeout in microseconds of waiting database connection string (default: 1 second)\n"
			   "\t-c STR\tdatabase connection string\n");
        return 1;
    }
	return referee_loop(connections, nConns, timeout);
}

