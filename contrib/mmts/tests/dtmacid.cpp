#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/nontransaction>
#include <pqxx/pipeline>

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

typedef void* (*thread_proc_t)(void*);
typedef uint32_t xid_t;

struct thread
{
    pthread_t t;
    size_t transactions;
    size_t updates;
    size_t selects;
    size_t aborts;
    int id;

    void start(int tid, thread_proc_t proc) { 
        id = tid;
        updates = 0;
        selects = 0;
        aborts = 0;
        transactions = 0;
        pthread_create(&t, NULL, proc, this);
    }

    void wait() { 
        pthread_join(t, NULL);
    }
};

struct config
{
    int nReaders;
    int nWriters;
    int nIterations;
    int nAccounts;
    int updatePercent;
    vector<string> connections;
	bool scatter;

    config() {
        nReaders = 1;
        nWriters = 10;
        nIterations = 1000;
        nAccounts = 100000;
        updatePercent = 100;
		scatter = false;
    }
};

config cfg;
bool running;

#define USEC 1000000

static time_t getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (time_t)tv.tv_sec*USEC + tv.tv_usec;
}


void exec(transaction_base& txn, char const* sql, ...)
{
    va_list args;
    va_start(args, sql);
    char buf[1024];
    vsprintf(buf, sql, args);
    va_end(args);
    txn.exec(buf);
}

template<class T>
T execQuery( transaction_base& txn, char const* sql, ...)
{
    va_list args;
    va_start(args, sql);
    char buf[1024];
    vsprintf(buf, sql, args);
    va_end(args);
    result r = txn.exec(buf);
    return r[0][0].as(T());
}  

void* reader(void* arg)
{
    thread& t = *(thread*)arg;
    vector< my_unique_ptr<connection> > conns(cfg.connections.size());
    for (size_t i = 0; i < conns.size(); i++) {
        conns[i] = new connection(cfg.connections[i]);
    } 
	char buf[256];
	sprintf(buf, "dump-%d.txt", t.id);	
	FILE* out = fopen(buf, "w");
	int lt = 0;
	int gt = 0;
	while (running) {
		int c1 = random() % conns.size();
		int c2;
		while ((c2 = random() % conns.size()) == c1);
        work txn1(*conns[c1]);
        work txn2(*conns[c2]);
        result r1 = txn1.exec("select v,xmin,xmax,mtm.get_csn(xmin),mtm.get_csn(xmax),mtm.get_snapshot(),mtm.get_last_csn() from t order by u");
        result r2 = txn2.exec("select v,xmin,xmax,mtm.get_csn(xmin),mtm.get_csn(xmax),mtm.get_snapshot(),mtm.get_last_csn() from t order by u");
		int delta = 0;
		assert((int)r1.size() == cfg.nAccounts && (int)r2.size() == cfg.nAccounts);
		for (int i=0; i < cfg.nAccounts; i++) { 
			int diff = r1[i][0].as(int()) - r2[i][0].as(int());
#if 0
			fprintf(out, "%d: %d %c %d - [%d,%d]->[%ld,%ld] (snapshot %ld, last CSN %ld) vs. [%d,%d]->[%ld,%ld] (snapshot %ld, last CSN %ld)\n",
					i, r1[i][0].as(int()), diff < 0 ? '<' : diff == 0 ? '=' : '>', r2[i][0].as(int()), 
					r1[i][1].as(int()), r1[i][2].as(int()), r1[i][3].as(int64_t()), r1[i][4].as(int64_t()), r1[i][5].as(int64_t()), r1[i][6].as(int64_t()), 
					r2[i][1].as(int()), r2[i][2].as(int()), r2[i][3].as(int64_t()), r2[i][4].as(int64_t()), r2[i][5].as(int64_t()), r2[i][6].as(int64_t()));
#endif
			if (diff != 0) { 
				if (delta == 0) { 
					delta = diff;
					if (delta < 0) lt++; else gt++;
				} else if ((delta ^ diff) < 0) { 
					fflush(out);
					printf("Inconsistency found for record %d: [%d,%d]->[%ld,%ld] (snapshot %ld, last CSN %ld) vs. [%d,%d]->[%ld,%ld] (snapshot %ld, last CSN %ld)\n", i, 
						   r1[i][1].as(int()), r1[i][2].as(int()), r1[i][3].as(int64_t()), r1[i][4].as(int64_t()), r1[i][5].as(int64_t()), r1[i][6].as(int64_t()), 
						   r2[i][1].as(int()), r2[i][2].as(int()), r2[i][3].as(int64_t()), r2[i][4].as(int64_t()), r2[i][5].as(int64_t()), r2[i][6].as(int64_t()));
				}
			}
		}
        t.selects += 2;
        txn1.commit();
        txn2.commit();
    }
	fclose(out);
	printf("lt=%d, gt=%d\n", lt, gt);
    return NULL;
}
 
void* writer(void* arg)
{
    thread& t = *(thread*)arg;
    vector< my_unique_ptr<connection> > conns(cfg.connections.size());
    for (size_t i = 0; i < conns.size(); i++) {
        conns[i] = new connection(cfg.connections[i]);
    }
    for (int i = 0; i < cfg.nIterations; i++)
    { 
        //work 
        //transaction<repeatable_read> txn(*conns[random() % conns.size()]);
        transaction<read_committed> txn(*conns[random() % conns.size()]);
        int acc = cfg.scatter
			? random() % (cfg.nAccounts/cfg.nWriters) * cfg.nWriters + t.id
			: random() % cfg.nAccounts;
			
        try {            
			exec(txn, "update t set v = v + 1 where u=%d", acc);
            txn.commit();
			t.updates += 1;
            t.transactions += 1;
        } catch (pqxx_exception const& x) { 
            txn.abort();
            t.aborts += 1;
            i -= 1;
        }
    }
    return NULL;
}
      
void initializeDatabase()
{
	connection conn(cfg.connections[0]);
    time_t start = getCurrentTime();
	printf("Creating database schema...\n");
	{
		nontransaction txn(conn);
        exec(txn, "drop extension if exists multimaster");
        exec(txn, "create extension multimaster");
		exec(txn, "drop table if exists t");
		exec(txn, "create table t(u int primary key, v int)");
	}
	printf("Populating data...\n");
	{
		work txn(conn);
		exec(txn, "insert into t (select generate_series(0,%d), %d)", cfg.nAccounts-1, 0);
		txn.commit();
	}
	printf("Initialization completed in %f seconds\n", (getCurrentTime() - start)/100000.0);
}

int main (int argc, char* argv[])
{
    bool initialize = false;

    if (argc == 1){
        printf("Use -h to show usage options\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) { 
            case 'r':
                cfg.nReaders = atoi(argv[++i]);
                continue;
            case 'w':
                cfg.nWriters = atoi(argv[++i]);
                continue;                
            case 'a':
                cfg.nAccounts = atoi(argv[++i]);
                continue;
            case 'n':
                cfg.nIterations = atoi(argv[++i]);
                continue;
            case 'p':
                cfg.updatePercent = atoi(argv[++i]);
                continue;
            case 's':
  			    cfg.scatter = true;
                continue;
            case 'c':
                cfg.connections.push_back(string(argv[++i]));
                continue;
            case 'i':
                initialize = true;
                continue;
            }
        }
        printf("Options:\n"
               "\t-r N\tnumber of readers (1)\n"
               "\t-w N\tnumber of writers (10)\n"
               "\t-a N\tnumber of accounts (100000)\n"
               "\t-n N\tnumber of iterations (1000)\n"
               "\t-p N\tupdate percent (100)\n"
               "\t-c STR\tdatabase connection string\n"
               "\t-s\tavoid conflicts\n"
               "\t-i\tinitialize database\n");
        return 1;
    }

    if (initialize) { 
        initializeDatabase();
        printf("%d accounts inserted\n", cfg.nAccounts);
        return 0;
    }

    time_t start = getCurrentTime();
    running = true;

    vector<thread> readers(cfg.nReaders);
    vector<thread> writers(cfg.nWriters);
    size_t nAborts = 0;
    size_t nUpdates = 0;
    size_t nSelects = 0;
    size_t nTransactions = 0;

    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].start(i, reader);
    }
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].start(i, writer);
    }
    
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].wait();
        nUpdates += writers[i].updates;
        nSelects += writers[i].selects;
        nAborts += writers[i].aborts;
        nTransactions += writers[i].transactions;
    }
    
    running = false;

    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].wait();
        nSelects += readers[i].selects;
    }
 
    time_t elapsed = getCurrentTime() - start;

    printf(
        "{\"tps\":%f, \"transactions\":%ld,"
        " \"selects\":%ld, \"updates\":%ld, \"aborts\":%ld, \"abort_percent\": %d,"
        " \"readers\":%d, \"writers\":%d, \"update_percent\":%d, \"accounts\":%d, \"iterations\":%d, \"hosts\":%ld}\n",
        (double)(nTransactions*USEC)/elapsed,
        nTransactions,
        nSelects, 
        nUpdates,
        nAborts,
        (int)(nAborts*100/nTransactions),        
        cfg.nReaders,
        cfg.nWriters,
        cfg.updatePercent,
        cfg.nAccounts,
        cfg.nIterations,
        cfg.connections.size()
        );

    return 0;
}
