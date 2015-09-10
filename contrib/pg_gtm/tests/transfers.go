package main

import (
    "fmt"
    "sync"
    "strconv"
    "math/rand"
    "github.com/jackc/pgx"
)

const (
    TRANSFER_CONNECTIONS = 4
    INIT_AMOUNT = 10000
    N_ITERATIONS = 1000
    N_ACCOUNTS = 1//100000
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
    
    exec(conn1, "select dtm_register_node(1)")
    exec(conn2, "select dtm_register_node(2)")

    exec(conn1, "begin transaction")
    exec(conn2, "begin transaction")

    snapshot = execQuery(conn1, "select dtm_extend($1)", gtid)
    snapshot = execQuery(conn2, "select dtm_access($1, $2)", snapshot, gtid)

    for i := 0; i < N_ACCOUNTS; i++ {
        exec(conn1, "insert into t values($1, $2)", i, INIT_AMOUNT)
        exec(conn2, "insert into t values($1, $2)", i, INIT_AMOUNT)
    }
           
    exec(conn1, "prepare transaction '" + gtid + "'")
    exec(conn2, "prepare transaction '" + gtid + "'")

    exec(conn1, "select dtm_begin_prepare($1, 2)", gtid)
    exec(conn2, "select dtm_begin_prepare($1, 2)", gtid)

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

    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    for i := 0; i < N_ITERATIONS; i++ {
        gtid := strconv.Itoa(id) + "." + strconv.Itoa(i)
        //amount := 2*rand.Intn(2) - 1
        amount := 1
        account1 := rand.Intn(N_ACCOUNTS) 
        account2 := rand.Intn(N_ACCOUNTS)

        exec(conn1, "begin transaction")
        exec(conn2, "begin transaction")

        snapshot = execQuery(conn1, "select dtm_extend($1)", gtid)
        snapshot = execQuery(conn2, "select dtm_access($1, $2)", snapshot, gtid)

        exec(conn1, "update t set v = v + $1 where u=$2", amount, account1)
        exec(conn2, "update t set v = v - $1 where u=$2", amount, account2)

        exec(conn1, "prepare transaction '" + gtid + "'")
        exec(conn2, "prepare transaction '" + gtid + "'")

        exec(conn1, "select dtm_begin_prepare($1, 2)", gtid)
        exec(conn2, "select dtm_begin_prepare($1, 2)", gtid)

        csn = execQuery(conn1, "select dtm_prepare($1, 0)", gtid)
        csn = execQuery(conn2, "select dtm_prepare($1, $2)", gtid, csn)

        exec(conn1, "select dtm_end_prepare($1, $2)", gtid, csn)
        exec(conn2, "select dtm_end_prepare($1, $2)", gtid, csn)

        exec(conn1, "commit prepared '" + gtid + "'")
        exec(conn2, "commit prepared '" + gtid + "'")
    }

    fmt.Println("Test completed")
    wg.Done()
}

func total() int64 {
    var err error
    var sum1 int64
    var sum2 int64
    var snapshot int64

    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    for { 
        exec(conn1, "begin transaction")
        exec(conn2, "begin transaction")
 
        snapshot = execQuery(conn1, "select dtm_extend()")
        snapshot = execQuery(conn2, "select dtm_access($1)", snapshot)

        sum1 = execQuery(conn1, "select sum(v) from t")
        sum2 = execQuery(conn2, "select sum(v) from t")

        exec(conn1, "commit")
        exec(conn2, "commit")

        return sum1 + sum2
    }
}

func totalrep(wg *sync.WaitGroup) {
    var prevSum int64 = 0 
    for running {
        sum := total()
        if (sum != prevSum) {
            fmt.Println("Total = ", sum)
            prevSum = sum
        }
    }
    wg.Done()
}

func main() {
    var transferWg sync.WaitGroup
    var inspectWg sync.WaitGroup

    prepare_db()

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


