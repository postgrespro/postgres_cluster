package main

import (
    "sync"
    "math/rand"
    "github.com/jackc/pgx"
)

type Readers struct {}

func (t Readers) prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs))
    for _, connstr := range connstrs {
        go t.prepare_one(connstr, &wg)
    }
    wg.Wait()
}

func (t Readers) prepare_one(connstr string, wg *sync.WaitGroup) {
    dbconf, err := pgx.ParseDSN(connstr)
    checkErr(err)
    conn, err := pgx.Connect(dbconf)
    checkErr(err)
    defer conn.Close()

    if cfg.UseDtm {
        exec(conn, "drop extension if exists pg_dtm")
        exec(conn, "create extension pg_dtm")
    }
    exec(conn, "drop table if exists t cascade")
    exec(conn, "create table t(u int primary key, v int)")
    exec(conn, "insert into t (select generate_series(0,$1-1), $2)", cfg.AccountsNum, 0)
    wg.Done()
}

func (t Readers) writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    var updates = 0
    var conns []*pgx.Conn

    for _, connstr := range cfg.ConnStrs {
        dbconf, err := pgx.ParseDSN(connstr)
        checkErr(err)
        conn, err := pgx.Connect(dbconf)
        checkErr(err)
        defer conn.Close()
        conns = append(conns, conn)
    }
    for updates < cfg.IterNum {   
        acc := rand.Intn(cfg.AccountsNum) 

        if cfg.UseDtm {
            xid := execQuery(conns[0], "select dtm_begin_transaction()") 
            for i := 1; i < len(conns); i++ {
                exec(conns[i], "select dtm_join_transaction($1)", xid)
            }
        }
        for _, conn := range conns {
            exec(conn, "begin transaction isolation level " + cfg.Isolation)
            exec(conn, "update t set v = v + 1 where u=$1", acc)
        }
        commit(conns...)
        updates++
    }    
    // cCommits <- updates
    wg.Done()
}

func (t Readers) reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool) {
    var fetches = 0
    var conns []*pgx.Conn
    var sum int32 = 0

    for _, connstr := range cfg.ConnStrs {
        dbconf, err := pgx.ParseDSN(connstr)
        checkErr(err)
        conn, err := pgx.Connect(dbconf)
        checkErr(err)
        defer conn.Close()
        conns = append(conns, conn)
    }
    for running {
        acc := rand.Intn(cfg.AccountsNum)
        con := rand.Intn(len(conns))
        sum += execQuery(conns[con], "select v from t where u=$1", acc)
        fetches++
    }
    // cFetches <- fetches
    wg.Done()
}

// vim: expandtab ts=4 sts=4 sw=4
