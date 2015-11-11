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
#include <pqxx/pipeline>

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
typedef uint32_t xid_t;

struct thread
{
    pthread_t t;
    size_t proceeded;
    size_t aborts;
    int id;

    void start(int tid, thread_proc_t proc) { 
        id = tid;
        proceeded = 0;
        aborts = 0;
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
    char const* isolationLevel;
    vector<string> connections;

    config() {
        nReaders = 1;
        nWriters = 10;
        nIterations = 1000;
        nAccounts = 1000;      
        isolationLevel = "read committed";
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

xid_t execQuery( transaction_base& txn, char const* sql, ...)
{
    va_list args;
    va_start(args, sql);
    char buf[1024];
    vsprintf(buf, sql, args);
    va_end(args);
    result r = txn.exec(buf);
    return r[0][0].as(xid_t());
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
        xid_t xid = 0;
        for (size_t i = 0; i < conns.size(); i++) {        
            work txn(*conns[i]);
            if (i == 0) {
                xid = execQuery(txn, "select dtm_begin_transaction()");
            } else {
                exec(txn, "select dtm_join_transaction(%u)", xid);
            }
            txn.commit();
        }
        vector< unique_ptr<nontransaction> > txns(conns.size());
        vector< unique_ptr<pipeline> > pipes(conns.size());
        vector<pipeline::query_id> results(conns.size());
        for (size_t i = 0; i < conns.size(); i++) {        
            txns[i] = new nontransaction(*conns[i]);
            pipes[i] = new pipeline(*txns[i]);
            results[i] = pipes[i]->insert("select sum(v) from t");
        }
        int64_t sum = 0;
        for (size_t i = 0; i < conns.size(); i++) {        
            pipes[i]->complete();
            result r = pipes[i]->retrieve(results[i]);
            sum += r[0][0].as(int64_t());
        }
        if (sum != prevSum) {
            printf("Total=%ld xid=%u\n", sum, xid);
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
        int srcCon, dstCon;
        //int srcAcc = (random() % ((cfg.nAccounts-cfg.nWriters)/cfg.nWriters))*cfg.nWriters + t.id;
        //int dstAcc = (random() % ((cfg.nAccounts-cfg.nWriters)/cfg.nWriters))*cfg.nWriters + t.id;
        int srcAcc = random() % cfg.nAccounts;
        int dstAcc = random() % cfg.nAccounts;

        do { 
            srcCon = random() % cfg.connections.size();
            dstCon = random() % cfg.connections.size();
        } while (srcCon == dstCon);
        
        nontransaction srcTx(*conns[srcCon]);
        nontransaction dstTx(*conns[dstCon]);
        
        xid_t xid = execQuery(srcTx, "select dtm_begin_transaction()");
        exec(dstTx, "select dtm_join_transaction(%u)", xid);

        exec(srcTx, "begin transaction isolation level %s", cfg.isolationLevel);
        exec(dstTx, "begin transaction isolation level %s", cfg.isolationLevel);

        try { 
            exec(srcTx, "update t set v = v - 1 where u=%d", srcAcc);
            exec(dstTx, "update t set v = v + 1 where u=%d", dstAcc);
        } catch (pqxx_exception const& x) { 
            exec(srcTx, "rollback");
            exec(dstTx, "rollback");
            t.aborts += 1;
            i -= 1;
            continue;
        }
        pipeline srcPipe(srcTx);
        pipeline dstPipe(dstTx);
        srcPipe.insert("commit transaction");
        dstPipe.insert("commit transaction");
        srcPipe.complete();
        dstPipe.complete();
        
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
        exec(txn, "insert into t (select generate_series(0,%d), %d)", cfg.nAccounts-1, 0);
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
            case 'l':
                cfg.isolationLevel = argv[++i];
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
               "\t-l STR\tisolation level (read committed)\n"
               "\t-c STR\tdatabase connection string\n"
               "\t-i\tinitialize datanase\n");
        return 1;
    }
    if (cfg.connections.size() < 2) { 
        printf("At least two connections has to be specified\n");
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
    size_t nAborts = 0;
    
    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].start(i, reader);
    }
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].start(i, writer);
    }
    
    for (int i = 0; i < cfg.nWriters; i++) { 
        writers[i].wait();
        nWrites += writers[i].proceeded;
        nAborts += writers[i].aborts;
    }
    
    running = false;

    for (int i = 0; i < cfg.nReaders; i++) { 
        readers[i].wait();
        nReads += readers[i].proceeded;
    }
 
    time_t elapsed = getCurrentTime() - start;
    if (elapsed == 0) { 
        printf("Test is completed too fast\n");
    } else { 
        printf("TPS(updates)=%f, TPS(selects)=%f, aborts=%ld\n", (double)(nWrites*USEC)/elapsed, (double)(nReads*USEC)/elapsed, nAborts);
    }
    return 0;
}
