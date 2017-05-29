#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
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
    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
    void operator=(T* p) { ptr = p; }
    void operator=(my_unique_ptr& other) {
        ptr = other.ptr;
        other.ptr = NULL;
    }        
};

int main (int argc, char* argv[])
{
	vector<string> connection_strings;

	if (argc == 1){
        printf("Use -h to show usage options\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) {
			  case 't':
				cfs.timeout = atoi(argv[++i]);
				continue;
			  case 'c':
                cfg.connections.push_back(string(argv[++i]));
                continue;
			}
        }
		printf("Options:\n"
                "\t-t TIMEOUT\ttimeout in seconds of waiting database connection string\n"
			   "\t-c STR\tdatabase connection string\n");
        return 1;
    }
	
	size_t nConns = connection_strings.size();
    vector< my_unique_ptr<connection> > conns(nConns);
	for (size_t i = 0; i < nConns; i++) {
		conns[i] = new connection(connection_strings[i]);
    }
	nodemask_t disabledMask = 0;
	nodemask_t enabledMask = 0;

	while (true) { 
        vector< my_unique_ptr<nontransaction> > txns(conns.size());
        vector< my_unique_ptr<pipeline> > pipes(nConns);
        vector<pipeline::query_id> queries(nConns);
		char sql[128];
		sprintf(sql, "select mtm.arbitrator_poll(%lld)", disabledMask);

        for (size_t i = 0; i < nConns; i++) {        
			if (BIT_CHECK(disabledMask, i)) { 
				if (BIT_CHECK(enabledMask, i)) { 
					try { 
						delete conns[i];
						conns[i] = new connection(connection_strings[i]);
						BIT_CLEAR(disabledMask, i);
					} catch (pqxx_exception const& x) { 
						conns[i] = NULL;
						fprintf(stderr, "Failed to connect to node %d: %s\n", (int)i+1, x.base().what());
					}
				}
			}
			if (!BIT_CHECK(disabledMask, i)) { 
				txns[i] = new nontransaction(*conns[i]);
				pipes[i] = new pipeline(*txns[i]);
				queries[i] = pipes[i]->insert(sql);
			}
			sleep(cfg.timeout);
			enabledMask = 0;
			for (size_t i = 0; i < nConns; i++) {        
				if (!BIT_CHECK(didsabledMask, i)) { 
					if (!pipes[i]->is_finished(queries[i])) 
					{ 
						fprintf(stderr, "Doesn't receive response from node %d within %d seconds\n", (int)i+1, cfs.timeout);
						BIT_SET(disabledMask, i);
						delete conns[i];
						conns[i] = NULL;
					} else {
						try { 
							result r = pipes[i]->retrieve(results[i]);
							enabledMask |= r[0][0].as(nodemask_t());
						} catch (pqxx_exception const& x) { 
							delete conns[i];
							conns[i] = NULL;
							fprintf(stderr, "Failed to retrieve result from node %d: %s\n", (int)i+1, x.base().what());
						}							
					}
				}
			}
		}
	}
}
