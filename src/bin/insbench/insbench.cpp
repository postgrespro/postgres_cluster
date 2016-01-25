#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/nontransaction>
#include <pqxx/pipeline>
#include <pqxx/version>

using namespace std;
using namespace pqxx;

typedef void* (*thread_proc_t)(void*);

struct thread
{
    pthread_t t;

    void start(thread_proc_t proc) { 
        pthread_create(&t, NULL, proc, this);
    }

    void wait() { 
        pthread_join(t, NULL);
    }
};

struct config
{
    int indexUpdateInterval;
    int nInserters;
    int nIndexes;
    int nIterations;
	int transactionSize;
	bool useSystemTime;
	bool noPK;
    string connection;

    config() {
		indexUpdateInterval = 0;
        nInserters = 1;
		nIndexes = 8;
        nIterations = 10000;
		transactionSize = 100;
		useSystemTime = false;
		noPK = false;
    }
};

config cfg;
bool running;
int nIndexUpdates;
time_t maxIndexUpdateTime;
time_t totalIndexUpdateTime;
time_t currTimestamp;

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

void* inserter(void* arg)
{
    connection con(cfg.connection);
	if (cfg.useSystemTime) 
	{
#if PQXX_VERSION_MAJOR >= 4
		con.prepare("insert", "insert into t values ($1,$2,$3,$4,$5,$6,$7,$8,$9)");
#else
		con.prepare("insert", "insert into t values ($1,$2,$3,$4,$5,$6,$7,$8,$9)")("bigint")("bigint")("bigint")("bigint")("bigint")("bigint")("bigint")("bigint")("bigint");
#endif
	} else {
		con.prepare("insert", "insert into t (select generate_series($1::integer,$2::integer),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000),ceil(random()*1000000000))");
	}
	
    for (int i = 0; i < cfg.nIterations; i++)
    { 
		work txn(con);
		if (cfg.useSystemTime) 
		{
		    for (int j = 0; j < cfg.transactionSize; j++) 
			{ 
		        txn.prepared("insert")(getCurrentTime())(random())(random())(random())(random())(random())(random())(random())(random()).exec();
	        }
	    } else { 
			currTimestamp = i*cfg.transactionSize;
		    txn.prepared("insert")(i*cfg.transactionSize)((i+1)*cfg.transactionSize-1).exec();
	    }
		txn.commit();
	}
	return NULL;
}

void* indexUpdater(void* arg)
{
    connection con(cfg.connection);
	while (running) {
		sleep(cfg.indexUpdateInterval);
		printf("Alter indexes\n");
		time_t now = getCurrentTime();
		{
			work txn(con);
			for (int i = 0; i < cfg.nIndexes; i++) { 
				exec(txn, "alter index idx%d where pk<%lu", i, cfg.useSystemTime ? now : currTimestamp);
			}
			txn.commit();
		}
		printf("End alter indexes\n");
		nIndexUpdates += 1;
		time_t elapsed = getCurrentTime() - now;
		totalIndexUpdateTime += elapsed;
		if (elapsed > maxIndexUpdateTime) { 
			maxIndexUpdateTime = elapsed;
		}
	}
    return NULL;
}
      
void initializeDatabase()
{
    connection conn(cfg.connection);
	work txn(conn);
	time_t now = getCurrentTime();
	exec(txn, "drop table if exists t");
	exec(txn, "create table t (pk bigint, k1 bigint, k2 bigint, k3 bigint, k4 bigint, k5 bigint, k6 bigint, k7 bigint, k8 bigint)");
	if (!cfg.noPK) { 
		exec(txn, "create index pk on t(pk)");
	}
	for (int i = 0; i < cfg.nIndexes; i++) { 
		if (cfg.indexUpdateInterval == 0)  { 
			exec(txn, "create index idx%d on t(k%d)", i, i+1);
		} else if (cfg.useSystemTime) { 
			exec(txn, "create index idx%d on t(k%d) where pk<%ld", i, i+1, now);
		} else { 
			exec(txn, "create index idx%d on t(k%d) where pk<%ld", i, i+1, 0);
		}
	}
	txn.commit();
}
			
	
int main (int argc, char* argv[])
{
    if (argc == 1){
        printf("Use -h to show usage options\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) { 
            case 't':
                cfg.transactionSize = atoi(argv[++i]);
                continue;
            case 'w':
                cfg.nInserters = atoi(argv[++i]);
                continue;                
            case 'u':
                cfg.indexUpdateInterval = atoi(argv[++i]);
                continue;
            case 'n':
                cfg.nIterations = atoi(argv[++i]);
                continue;
            case 'i':
                cfg.nIndexes = atoi(argv[++i]);
                continue;
            case 'c':
                cfg.connection = string(argv[++i]);
                continue;
			  case 'q':
				cfg.useSystemTime = true;
				continue;
			  case 'p':
				cfg.noPK = true;
				continue;
            }
        }
        printf("Options:\n"
               "\t-t N\ttransaction size (100)\n"
               "\t-w N\tnumber of inserters (1)\n"
               "\t-u N\tindex update interval (0)\n"
               "\t-n N\tnumber of iterations (10000)\n"
               "\t-i N\tnumber of indexes (8)\n"
               "\t-q\tuse system time and libpq\n"
               "\t-p\tno primary key\n"
               "\t-c STR\tdatabase connection string\n");
        return 1;
    }

	initializeDatabase();

    time_t start = getCurrentTime();
    running = true;

    vector<thread> inserters(cfg.nInserters);
	thread bgw;
    for (int i = 0; i < cfg.nInserters; i++) { 
        inserters[i].start(inserter);
    }
	if (cfg.indexUpdateInterval != 0) {
		bgw.start(indexUpdater);
	}
    for (int i = 0; i < cfg.nInserters; i++) { 
        inserters[i].wait();
    }    
    running = false;
	bgw.wait();
 
    time_t elapsed = getCurrentTime() - start;

    printf(
        "{\"tps\":%f, \"index_updates\":%d, \"max_update_time\":%ld, \"avg_update_time\":%f,"
        " \"inserters\":%d, \"indexes\":%d, \"transaction_size\":%d, \"iterations\":%d}\n",
        (double)cfg.nInserters*cfg.transactionSize*cfg.nIterations*USEC/elapsed,
        nIndexUpdates,
		maxIndexUpdateTime,
		(double)totalIndexUpdateTime/nIndexUpdates,
		cfg.nInserters, 
		cfg.nIndexes, 
		cfg.transactionSize,
		cfg.nIterations);
    return 0;
}
