package main

import (
    "fmt"
    "sync"
    "strconv"
    "math/rand"
    "time"
    "github.com/jackc/pgx"
)

const (
    TRANSFER_CONNECTIONS = 100
    INIT_AMOUNT = 10000
    N_ITERATIONS = 1000
    N_ACCOUNTS = 100000
)


var cfg1 = pgx.ConnConfig{
        Host:     "127.0.0.1",
        Port:     5432,
        Database: "postgres",
    }

var cfg2 = pgx.ConnConfig{
        Host:     "127.0.0.1",
        Port:     5433,
        Database: "postgres",
    }

var running = false

func prepare_db() {
    var snapshot int64
    var csn int64
    var gtid string = "init"

    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    exec(conn1, "drop extension if exists pg_dtm")
    exec(conn1, "create extension pg_dtm")
    exec(conn1, "drop table if exists t")
    exec(conn1, "create table t(u int primary key, v int)")

    exec(conn2, "drop extension if exists pg_dtm")
    exec(conn2, "create extension pg_dtm")
    exec(conn2, "drop table if exists t")
    exec(conn2, "create table t(u int primary key, v int)")
    
    exec(conn1, "begin transaction")
    exec(conn2, "begin transaction")

    snapshot = execQuery(conn1, "select dtm_extend($1)", gtid)
    snapshot = execQuery(conn2, "select dtm_access($1, $2)", snapshot, gtid)

    //for i := 0; i < N_ACCOUNTS; i++ {
    //    exec(conn1, "insert into t values($1, $2)", i, INIT_AMOUNT)
    //    exec(conn2, "insert into t values($1, $2)", i, INIT_AMOUNT)
    //}
    exec(conn1, "insert into t (select generate_series(0,$1-1), $2)",N_ACCOUNTS,0)
    exec(conn2, "insert into t (select generate_series(0,$1-1), $2)",N_ACCOUNTS,0)
           
    exec(conn1, "prepare transaction '" + gtid + "'")
    exec(conn2, "prepare transaction '" + gtid + "'")

    exec(conn1, "select dtm_begin_prepare($1)", gtid)
    exec(conn2, "select dtm_begin_prepare($1)", gtid)

    csn = execQuery(conn1, "select dtm_prepare($1, 0)", gtid)
    csn = execQuery(conn2, "select dtm_prepare($1, $2)", gtid, csn)

    exec(conn1, "select dtm_end_prepare($1, $2)", gtid, csn)
    exec(conn2, "select dtm_end_prepare($1, $2)", gtid, csn)

    exec(conn1, "commit prepared '" + gtid + "'")
    exec(conn2, "commit prepared '" + gtid + "'")
}

func max(a, b int64) int64 {
    if a >= b {
        return a
    } 
    return b
}

func transfer(id int, wg *sync.WaitGroup) {
    var err error
    var snapshot int64
    var csn int64

    nGlobalTrans := 0
    
    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    for i := 0; i < N_ITERATIONS; i++ {

        gtid := strconv.Itoa(id) + "." + strconv.Itoa(i)
        amount := 2*rand.Intn(2) - 1
        account1 := rand.Intn(N_ACCOUNTS)
        account2 := rand.Intn(N_ACCOUNTS)

        exec(conn1, "begin transaction")
        exec(conn2, "begin transaction")
        snapshot = execQuery(conn1, "select dtm_extend($1)", gtid)
        snapshot = execQuery(conn2, "select dtm_access($1, $2)", snapshot, gtid)


        exec(conn1, "update t set v = v - $1 where u=$2", amount, account1)
        exec(conn2, "update t set v = v + $1 where u=$2", amount, account2)

        exec(conn1, "prepare transaction '" + gtid + "'")
        exec(conn2, "prepare transaction '" + gtid + "'")

        exec(conn1, "select dtm_begin_prepare($1)", gtid)
        exec(conn2, "select dtm_begin_prepare($1)", gtid)

        csn = execQuery(conn1, "select dtm_prepare($1, 0)", gtid)
        csn = execQuery(conn2, "select dtm_prepare($1, $2)", gtid, csn)

        exec(conn1, "select dtm_end_prepare($1, $2)", gtid, csn)
        exec(conn2, "select dtm_end_prepare($1, $2)", gtid, csn)

        exec(conn1, "commit prepared '" + gtid + "'")
        exec(conn2, "commit prepared '" + gtid + "'")
        nGlobalTrans++

    }

    fmt.Printf("Test completed, performed %d global transactions\n", nGlobalTrans)
    wg.Done()
}

func totalrep(wg *sync.WaitGroup) {
    var snapshot int64
    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    var prevSum int64 = 0 

    for running {
        exec(conn1, "begin transaction")
        exec(conn2, "begin transaction")
 
        snapshot = execQuery(conn1, "select dtm_extend()")
        snapshot = execQuery(conn2, "select dtm_access($1)", snapshot)

        sum1 := execQuery(conn1, "select sum(v) from t")
        sum2 := execQuery(conn2, "select sum(v) from t")

        exec(conn1, "commit")
        exec(conn2, "commit")

        sum := sum1 + sum2
        if (sum != prevSum) {
            fmt.Printf("Total=%d snapshot=%d\n", sum, snapshot)
            prevSum = sum
        }
    }
    wg.Done()
}

func main() {
    var transferWg sync.WaitGroup
    var inspectWg sync.WaitGroup

    prepare_db()
    start := time.Now()     
    transferWg.Add(TRANSFER_CONNECTIONS)
    for i:=0; i<TRANSFER_CONNECTIONS; i++ {
        go transfer(i, &transferWg)
    }
    running = true
    inspectWg.Add(1)
    go totalrep(&inspectWg)

    transferWg.Wait()
    running = false
    inspectWg.Wait()

    fmt.Printf("Elapsed time %f sec\n", time.Since(start).Seconds())
    fmt.Printf("TPS = %f\n", float64(TRANSFER_CONNECTIONS*N_ITERATIONS)/time.Since(start).Seconds())
}

func exec(conn *pgx.Conn, stmt string, arguments ...interface{}) {
    var err error
    _, err = conn.Exec(stmt, arguments... )
    checkErr(err)
}

func execQuery(conn *pgx.Conn, stmt string, arguments ...interface{}) int64 {
    var err error
    var result int64
    err = conn.QueryRow(stmt, arguments...).Scan(&result)
    checkErr(err)
    return result
}

func checkErr(err error) {
    if err != nil {
        panic(err)
    }
}

