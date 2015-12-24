// vim: expandtab ts=4 sts=4 sw=4
package main

import (
    "fmt"
    "sync"
    "math/rand"
    "strconv"
    // "time"
    "github.com/jackc/pgx"
)

type TransfersTS struct {}

func (t TransfersTS) prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs))
    for _, connstr := range connstrs {
        go t.prepare_one(connstr, &wg)
    }
    wg.Wait()
}

func (t TransfersTS) prepare_one(connstr string, wg *sync.WaitGroup) {
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
    exec(conn, "vacuum full")

    wg.Done()
}

func (t TransfersTS) writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    var nGlobalTrans = 0
    var snapshot int64
    var csn int64
    nWriters := cfg.Writers.Num

    if len(cfg.ConnStrs) == 1 {
        cfg.ConnStrs.Set(cfg.ConnStrs[0])
    }

    // for _, connstr := range cfg.ConnStrs {
    //     dbconf, err := pgx.ParseDSN(connstr)
    //     checkErr(err)
    //     conn, err := pgx.Connect(dbconf)
    //     checkErr(err)
    //     defer conn.Close()
    //     conns = append(conns, conn)
    // }

    dbconf1, err := pgx.ParseDSN(cfg.ConnStrs[ id % len(cfg.ConnStrs) ])
    checkErr(err)
    conn1, err := pgx.Connect(dbconf1)
    checkErr(err)
    defer conn1.Close()

    dbconf2, err := pgx.ParseDSN(cfg.ConnStrs[ (id + 1) % len(cfg.ConnStrs) ])
    checkErr(err)
    conn2, err := pgx.Connect(dbconf2)
    checkErr(err)
    defer conn2.Close()

    
    for i := 0; i < cfg.IterNum; i++ {
        gtid := strconv.Itoa(cfg.Writers.StartId) + "." + strconv.Itoa(id) + "." + strconv.Itoa(i)
        amount := 2*rand.Intn(2) - 1
        from_acc := rand.Intn((cfg.AccountsNum-nWriters)/nWriters)*nWriters+id
        to_acc   := rand.Intn((cfg.AccountsNum-nWriters)/nWriters)*nWriters+id

        exec(conn1, "begin transaction")
        exec(conn2, "begin transaction")

        if cfg.UseDtm {
            snapshot = _execQuery(conn1, "select dtm_extend($1)", gtid)
            snapshot = _execQuery(conn2, "select dtm_access($1, $2)", snapshot, gtid)
        }

        exec(conn1, "update t set v = v - $1 where u=$2", amount, from_acc)
        exec(conn2, "update t set v = v + $1 where u=$2", amount, to_acc)
        exec(conn1, "prepare transaction '" + gtid + "'")
        exec(conn2, "prepare transaction '" + gtid + "'")

        if cfg.UseDtm {
            exec(conn1, "select dtm_begin_prepare($1)", gtid)
            exec(conn2, "select dtm_begin_prepare($1)", gtid)
            csn = _execQuery(conn1, "select dtm_prepare($1, 0)", gtid)
            csn = _execQuery(conn2, "select dtm_prepare($1, $2)", gtid, csn)
            exec(conn1, "select dtm_end_prepare($1, $2)", gtid, csn)
            exec(conn2, "select dtm_end_prepare($1, $2)", gtid, csn)
        }
        
        exec(conn1, "commit prepared '" + gtid + "'")
        exec(conn2, "commit prepared '" + gtid + "'")

        nGlobalTrans++
    }

    cCommits <- nGlobalTrans
    cAborts <- 0
    wg.Done()
}

func (t TransfersTS) reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool) {
    var prevSum int64 = 0
    var snapshot int64

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

        for _, conn := range conns {
            exec(conn, "begin transaction")
        }

        for i, conn := range conns {
            if cfg.UseDtm {
                if i == 0 {
                    // xid = execQuery(conn, "select dtm_begin_transaction()")
                    snapshot = _execQuery(conn, "select dtm_extend()")
                } else {
                    // exec(conn, "select dtm_join_transaction($1)", xid)
                    snapshot = _execQuery(conn, "select dtm_access($1)", snapshot)
                }
            }
        }

        for _, conn := range conns {
            sum += _execQuery(conn, "select sum(v) from t")
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

func _execQuery(conn *pgx.Conn, stmt string, arguments ...interface{}) int64 {
    var err error
    var result int64
    err = conn.QueryRow(stmt, arguments...).Scan(&result)
    checkErr(err)
    return result
}

