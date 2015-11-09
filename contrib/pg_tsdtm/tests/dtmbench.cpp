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
#include <pqxx/nontransaction>

using namespace std;
using namespace pqxx;

template<class T>
class unique_ptr
{
    T* ptr;
    
  public:
    unique_ptr(T* p = NULL) : ptr(p) {}
    ~unique_ptr() { delete ptr; }
    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
    void operator=(T* p) { ptr = p; }
    void operator=(unique_ptr& other) {
        ptr = other.ptr;
        other.ptr = NULL;
    }        
};

typedef void* (*thread_proc_t)(void*);
typedef int64_t csn_t;

struct thread
{
    pthread_t t;
    size_t proceeded;
    int id;

    void start(int tid, thread_proc_t proc) { 
        id = tid;
        proceeded = 0;
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
    vector<string> connections;

    config() {
        nReaders = 1;
        nWriters = 10;
        nIterations = 1000;
        nAccounts = 1000;        
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

int64_t execQuery( transaction_base& txn, char const* sql, ...)
{
    va_list args;
    va_start(args, sql);
    char buf[1024];
    vsprintf(buf, sql, args);
    va_end(args);
    result r = txn.exec(buf);
    return r[0][0].as(int64_t());
}  

void* reader(void* arg)
{
    thread& t = *(thread*)arg;
    vector< unique_ptr<connection> > conns(cfg.connections.size());
    for (size_t i = 0; i < conns.size(); i++) {
        conns[i] = new connection(cfg.connections[i]);
    }
    int64_t prevSum = 0;

    while (running) {
        csn_t snapshot;
        vector< unique_ptr<work> > txns(conns.size());
        for (size_t i = 0; i < conns.size(); i++) {        
            txns[i] = new work(*conns[i]);
        }
        for (size_t i = 0; i < txns.size(); i++) {        
            if (i == 0) {
                snapshot = execQuery(*txns[i], "select dtm_extend()");
            } else {
                snapshot = execQuery(*txns[i], "select dtm_access(%ld)", snapshot);
            }
        }
        int64_t sum = 0;
        for (size_t i = 0; i < txns.size(); i++) {        
            sum += execQuery(*txns[i], "select sum(v) from t");
        }
        if (sum != prevSum) {
            printf("Total=%ld snapshot=%ld\n", sum, snapshot);
            prevSum = sum;
        }
        t.proceeded += 1;
    }
    return NULL;
}
 
void* writer(void* arg)
{
    thread& t = *(thread*)arg;
    vector< unique_ptr<connection> > conns(cfg.connections.size());
    for (size_t i = 0; i < conns.size(); i++) {
        conns[i] = new connection(cfg.connections[i]);
    }
    for (int i = 0; i < cfg.nIterations; i++)
    { 
        char gtid[32];
        int srcCon, dstCon;
        int srcAcc = (random() % ((cfg.nAccounts-cfg.nWriters)/cfg.nWriters))*cfg.nWriters + t.id;
        int dstAcc = (random() % ((cfg.nAccounts-cfg.nWriters)/cfg.nWriters))*cfg.nWriters + t.id;

        sprintf(gtid, "%d.%d", t.id, i);

        do { 
            srcCon = random() % cfg.connections.size();
            dstCon = random() % cfg.connections.size();
        } while (srcCon == dstCon);
        
        nontransaction srcTx(*conns[srcCon]);
        nontransaction dstTx(*conns[dstCon]);
        
        exec(srcTx, "begin transaction");
        exec(dstTx, "begin transaction");

        csn_t snapshot = execQuery(srcTx, "select dtm_extend('%s')", gtid);
        snapshot = execQuery(dstTx, "select dtm_access(%ld, '%s')", snapshot, gtid);
            
        exec(srcTx, "update t set v = v - 1 where u=%d", srcAcc);
        exec(dstTx, "update t set v = v + 1 where u=%d", dstAcc);
        
        exec(srcTx, "prepare transaction '%s'", gtid);
        exec(dstTx, "prepare transaction '%s'", gtid);
        exec(srcTx, "select dtm_begin_prepare('%s')", gtid);
        exec(dstTx, "select dtm_begin_prepare('%s')", gtid);
        csn_t csn = execQuery(srcTx, "select dtm_prepare('%s', 0)", gtid);
        csn = execQuery(dstTx, "select dtm_prepare('%s', %ld)", gtid, csn);
        exec(srcTx, "select dtm_end_prepare('%s', %ld)", gtid, csn);
        exec(dstTx, "select dtm_end_prepare('%s', %ld)", gtid, csn);
        exec(srcTx, "commit prepared '%s'", gtid);
        exec(dstTx, "commit prepared '%s'", gtid);

        t.proceeded += 1;
    }
    return NULL;
}
      
void initializeDatabase()
{
    for (size_t i = 0; i < cfg.connections.size(); i++) { 
        connection conn(cfg.connections[i]);
        work txn(conn);
        exec(txn, "drop extension if exists pg_dtm");
        exec(txn, "create extension pg_dtm");
        exec(txn, "drop table if exists t");
        exec(txn, "create table t(u int primary key, v int)");
        exec(txn, "insert into t (select generate_series(0,%d), %d)", cfg.nAccounts, 0);
        txn.commit();
    }        
}

int main (int argc, char* argv[])
{
    bool initialize = false;
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
               "\t-a N\tnumber of accounts (1000)\n"
               "\t-n N\tnumber of iterations (1000)\n"
               "\t-c STR\tdatabase connection string\n"
               "\t-i\tinitialize datanase\n");
        return 1;
    }
    if (initialize) { 
        initializeDatabase();
    }

    time_t start = getCurrentTime();
    running = true;

    vector<thread> readers(cfg.nReaders);
    vector<thread> writers(cfg.nWriters);
    size_t nReads = 0;
    size_t nWrites = 0;
    
    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].start(i, reader);
    }
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].start(i, writer);
    }
    
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].wait();
        nWrites += writers[i].proceeded;
    }
    
    running = false;

    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].wait();
        nReads += writers[i].proceeded;
    }
 
    time_t elapsed = getCurrentTime() - start;
    printf("TPS(updates)=%f, TPS(selects)=%f\n", (double)(nWrites*USEC)/elapsed, (double)(nReads*USEC)/elapsed);
    return 0;
}
