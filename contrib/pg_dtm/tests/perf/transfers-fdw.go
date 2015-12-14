package main

import (
    "fmt"
    "sync"
    "time"
    "os"
    "github.com/jackc/pgx"
)

type TransfersFDW struct {}

func (t TransfersFDW) prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs))

    for i, connstr := range connstrs {
        go t.prepare_slave(i, connstr, &wg)
    }
    wg.Wait()
    t.prepare_master()
}

func (t TransfersFDW) prepare_slave(id int, connstr string, wg *sync.WaitGroup) {
    dbconf, err := pgx.ParseDSN(connstr)
    checkErr(err)
    conn, err := pgx.Connect(dbconf)
    checkErr(err)
    defer conn.Close()

    if len(dbconf.User) == 0 {
        fmt.Println("ERROR: FDW test need explicit usernames in connection strings")
        os.Exit(2)
    }

    if cfg.UseDtm {
        exec(conn, "drop extension if exists pg_dtm")
        exec(conn, "create extension pg_dtm")
    }
    exec(conn, "drop extension if exists postgres_fdw cascade")
    exec(conn, "drop table if exists t cascade")
    exec(conn, "create table t(u int primary key, v int)")
    exec(conn, "insert into t (select $3*generate_series(0,$1-1) + $4, $2)",
        cfg.AccountsNum, 0, len(cfg.ConnStrs), id)
    wg.Done()
}

func (t TransfersFDW) prepare_master() {
    dbconf, err := pgx.ParseDSN(cfg.ConnStrs[0])
    checkErr(err)
    conn, err := pgx.Connect(dbconf)
    checkErr(err)
    defer conn.Close()

    exec(conn, "CREATE EXTENSION postgres_fdw")

    for i, connstr := range cfg.ConnStrs[1:] {
        conf, _ := pgx.ParseDSN(connstr)
        server_sql := fmt.Sprintf(
            "CREATE SERVER dtm%d FOREIGN DATA WRAPPER postgres_fdw options(dbname '%s', host '%s', port '%d')",
            i, conf.Database, conf.Host, conf.Port)
        foreign_sql := fmt.Sprintf(
            "CREATE FOREIGN TABLE t_fdw%d() inherits (t) server dtm%d options(table_name 't')",
            i, i)
        mapping_sql := fmt.Sprintf(
            "CREATE USER MAPPING for %s SERVER dtm%d options (user '%s')",
            conf.User, i, conf.User)

        exec(conn, server_sql)
        exec(conn, foreign_sql)
        exec(conn, mapping_sql)
    }

    if dbconf.User != "" {
        
    }

}

func (t TransfersFDW) writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    var err error
    var nAborts = 0
    var nCommits = 0
    var myCommits = 0

    dbconf, err := pgx.ParseDSN(cfg.ConnStrs[0])
    checkErr(err)
    conn, err := pgx.Connect(dbconf)
    checkErr(err)
    defer conn.Close()

    start := time.Now()
    for myCommits < cfg.IterNum {
        amount := 1
        from_acc := cfg.Writers.StartId + 2*id + 1
        to_acc   := cfg.Writers.StartId + 2*id + 2

        if cfg.UseDtm {
            exec(conn, "select dtm_begin_transaction()")
        }
        exec(conn, "begin transaction isolation level " + cfg.Isolation)

        ok1 := execUpdate(conn, "update t set v = v - $1 where u=$2", amount, from_acc)
        ok2 := execUpdate(conn, "update t set v = v + $1 where u=$2", amount, to_acc)
        if !ok1 || !ok2 {
           exec(conn, "rollback")
           nAborts += 1
        } else {
           exec(conn, "commit")
           nCommits += 1
           myCommits += 1
        }

        if time.Since(start).Seconds() > 1 {
            cCommits <- nCommits
            cAborts <- nAborts
            nCommits = 0
            nAborts = 0
            start = time.Now()
        }
    }
    cCommits <- nCommits
    cAborts <- nAborts
    wg.Done()
}

func (t TransfersFDW) reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool) {
    var sum int64
    var prevSum int64 = 0

    dbconf, err := pgx.ParseDSN(cfg.ConnStrs[0])
    checkErr(err)
    conn, err := pgx.Connect(dbconf)
    checkErr(err)
    defer conn.Close()

    for running {
        exec(conn, "select dtm_begin_transaction()")
        exec(conn, "begin transaction isolation level " + cfg.Isolation)
        sum = execQuery64(conn, "select sum(v) from t")
        if (sum != prevSum) {
            fmt.Printf("Total=%d\n", sum)
            fmt.Printf("inconsistency!\n")
            *inconsistency = true
            prevSum = sum
        }
        exec(conn, "commit")
    }
    conn.Close()
    wg.Done()
}

// vim: expandtab ts=4 sts=4 sw=4
