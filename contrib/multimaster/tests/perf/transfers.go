package main

import (
    "fmt"
    "sync"
    "math/rand"
    "strconv"
    "time"
    "github.com/jackc/pgx"
)

type Transfers struct {}

func (t Transfers) prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs))
    for _, connstr := range connstrs {
        go t.prepare_one(connstr, &wg)
    }
    wg.Wait()
}

func (t Transfers) prepare_one(connstr string, wg *sync.WaitGroup) {
    dbconf, err := pgx.ParseDSN(connstr)
    checkErr(err)

    conn, err := pgx.Connect(dbconf)
    checkErr(err)

    defer conn.Close()

    if cfg.UseDtm {
        exec(conn, "drop extension if exists pg_dtm")
        exec(conn, "create extension pg_dtm")
    }
    exec(conn, "drop table if exists t")
    exec(conn, "create table t(u int primary key, v int)")
    exec(conn, "insert into t (select generate_series(0,$1-1), $2)",
        cfg.AccountsNum, 0)

    exec(conn, "commit")
    wg.Done()
}

func (t Transfers) writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    var nAborts = 0
    var nCommits = 0
    var myCommits = 0

    var conns []*pgx.Conn

    if len(cfg.ConnStrs) == 1 {
        cfg.ConnStrs.Set(cfg.ConnStrs[0])
    }

    for _, connstr := range cfg.ConnStrs {
        dbconf, err := pgx.ParseDSN(connstr)
        checkErr(err)
        conn, err := pgx.Connect(dbconf)
        checkErr(err)
        defer conn.Close()
        conns = append(conns, conn)
    }

    start := time.Now()
    for myCommits < cfg.IterNum {
        amount := 1

        from_acc := cfg.Writers.StartId + 2*id + 1
        to_acc   := cfg.Writers.StartId + 2*id + 2

        src := conns[rand.Intn(len(conns))]
        dst := conns[rand.Intn(len(conns))]
        if src == dst {
            continue
        }

        if cfg.UseDtm {
            xid := execQuery(src, "select dtm_begin_transaction()")
            exec(dst, "select dtm_join_transaction($1)", xid)
        }

        parallel_exec(
            []*pgx.Conn{src,dst},
            []string{"begin transaction isolation level " + cfg.Isolation,
            "begin transaction isolation level " + cfg.Isolation})

        ok := true


        sql1 := "update t set v = v - " + 
            strconv.Itoa(amount) + " where u=" + strconv.Itoa(from_acc)
        sql2 := "update t set v = v + " + 
            strconv.Itoa(amount) + " where u=" + strconv.Itoa(to_acc)

        ok = parallel_exec([]*pgx.Conn{src,dst}, []string{sql1,sql2})


        if ok {
            commit(src, dst)
            nCommits += 1
            myCommits += 1
        } else {
            exec(src, "rollback")
            exec(dst, "rollback")
            nAborts += 1
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

func (t Transfers) reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool) {
    var prevSum int64 = 0

    var conns []*pgx.Conn

    for _, connstr := range cfg.ConnStrs {
        dbconf, err := pgx.ParseDSN(connstr)
        checkErr(err)
        conn, err := pgx.Connect(dbconf)
        checkErr(err)
        defer conn.Close()
        conns = append(conns, conn)
    }

    for running {
        var sum int64 = 0
        var xid int32
        for i, conn := range conns {
            if cfg.UseDtm {
                if i == 0 {
                    xid = execQuery(conn, "select dtm_begin_transaction()")
                } else {
                    exec(conn, "select dtm_join_transaction($1)", xid)
                }
            }

            exec(conn, "begin transaction isolation level " + cfg.Isolation)
            sum += execQuery64(conn, "select sum(v) from t")
        }
        commit(conns...)

        if (sum != prevSum) {
            fmt.Printf("Total=%d xid=%d\n", sum, xid)
            if (prevSum != 0) {
                fmt.Printf("inconsistency!\n")
                *inconsistency = true
            }
            prevSum = sum
        }
    }

    wg.Done()
}

// vim: expandtab ts=4 sts=4 sw=4
