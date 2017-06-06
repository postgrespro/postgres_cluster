#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/subtransaction.hxx>
#include <pqxx/nontransaction>
#include <pqxx/pipeline>

typedef unsigned long long ulong64; /* we are not using uint64 here because we want to use %lld format for this type */

typedef ulong64 nodemask_t;

#define BIT_CHECK(mask, bit) (((mask) & ((nodemask_t)1 << (bit))) != 0)
#define BIT_CLEAR(mask, bit) (mask &= ~((nodemask_t)1 << (bit)))
#define BIT_SET(mask, bit)   (mask |= ((nodemask_t)1 << (bit)))

using namespace std;
using namespace pqxx;

template<class T>
class my_unique_ptr
{
    T* ptr;
    
  public:
    my_unique_ptr(T* p = NULL) : ptr(p) {}
    ~my_unique_ptr() { delete ptr; }
    T& operator*() const  { return *ptr; }
    T* operator->() const { return ptr; }
	bool operator ==(T const* p) const { return ptr == p; }
	bool operator !=(T const* p) const { return ptr != p; }
    void operator=(T* p) { 
		delete ptr;
		ptr = p; 
	}
    void operator=(my_unique_ptr& other) {
		delete ptr;
        ptr = other.ptr;
        other.ptr = NULL;
    }        
};

struct config
{
	int timeout;
	vector<string> connections;

	config() {
		timeout = 1000000; // 1 second
	}	
};

int main (int argc, char* argv[])
{
	config cfg;

	if (argc == 1){
        printf("Use -h to show usage options\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) {
			  case 't':
				cfg.timeout = atoi(argv[++i]);
				continue;
			  case 'c':
                cfg.connections.push_back(string(argv[++i]));
                continue;
			}
        }
		printf("Options:\n"
                "\t-t TIMEOUT\ttimeout in microseconds of waiting database connection string (default: 1 second)\n"
			   "\t-c STR\tdatabase connection string\n");
        return 1;
    }
	
	size_t nConns = cfg.connections.size();
    vector< my_unique_ptr<connection> > conns(nConns);
	for (size_t i = 0; i < nConns; i++) {
		conns[i] = new connection(cfg.connections[i]);
    }
	nodemask_t disabledMask = 0;
	nodemask_t newEnabledMask = 0;
	nodemask_t oldEnabledMask = 0;

	while (true) { 
        vector< my_unique_ptr<nontransaction> > txns(conns.size());
        vector< my_unique_ptr<pipeline> > pipes(nConns);
        vector<pipeline::query_id> queries(nConns);
		char sql[128];
		sprintf(sql, "select mtm.arbitrator_poll(%lld)", disabledMask);

		// Initiate queries to all live nodes
        for (size_t i = 0; i < nConns; i++) {        
			// Some of live node reestablished connection with dead node, so arbitrator should also try to connect to this node
			if (conns[i] == NULL) { 
				if (BIT_CHECK(newEnabledMask, i)) { 
					try { 
						conns[i] = new connection(cfg.connections[i]);
						BIT_CLEAR(disabledMask, i);
						fprintf(stdout, "Reestablish connection with node %d\n", (int)i+1);
					} catch (pqxx_exception const& x) { 
						if (conns[i] == NULL) { 
							conns[i] = NULL;
							fprintf(stderr, "Failed to connect to node %d: %s\n", (int)i+1, x.base().what());
						}
					}
				}
			} else { 
				txns[i] = new nontransaction(*conns[i]);
				pipes[i] = new pipeline(*txns[i]);
				queries[i] = pipes[i]->insert(sql);
			}
		}
		// Wait some time
		usleep(cfg.timeout);
		oldEnabledMask = newEnabledMask;
		newEnabledMask = ~0;
		for (size_t i = 0; i < nConns; i++) {        
			if (!BIT_CHECK(disabledMask, i)) { 
				if (!pipes[i]->is_finished(queries[i])) { 
					fprintf(stderr, "Doesn't receive response from node %d within %d microseconds\n", (int)i+1, cfg.timeout);
					BIT_SET(disabledMask, i);
					conns[i] = NULL;
				} else {
					try { 
						result r = pipes[i]->retrieve(queries[i]);
						newEnabledMask &= r[0][0].as(nodemask_t());
					} catch (pqxx_exception const& x) { 
						conns[i] = NULL;
						fprintf(stderr, "Failed to retrieve result from node %d: %s\n", (int)i+1, x.base().what());
					}							
				}
			}
		}
		if (newEnabledMask == ~0) { 
			if (oldEnabledMask != ~0) { 
				fprintf(stdout, "There are no more live nodes\n");
			}				
			// No live nodes: 
			disabledNodeMask = 0;
		} else { 
			if (newEnabledMask != oldEnabledMask) { 
				for (size_t i = 0; i < nConns; i++) {        
					if (BIT_CHECK(newEnabledMask ^ oldEnabledMask, i)) { 					
						fprintf(stdout, "Node %d is %s\n", (int)i+1, BIT_CHECK(newEnabledMask, i) ? "enabled" : "disabled");
					}
				}
			}	   
		}
	}
}
