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
    TRANSFER_CONNECTIONS = 8
    INIT_AMOUNT = 10000
    N_ITERATIONS = 10000
    N_ACCOUNTS = TRANSFER_CONNECTIONS//100000
    ISOLATION_LEVEL = "repeatable read"
    //ISOLATION_LEVEL = "read committed"
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

    exec(conn1, "CREATE EXTENSION postgres_fdw");
    exec(conn1, "CREATE SERVER dtm FOREIGN DATA WRAPPER postgres_fdw options (dbname 'postgres', host '127.0.0.1', port '5433')");
    exec(conn1, "CREATE FOREIGN TABLE t_fdw() inherits (t) server dtm options(table_name 't')");
    exec(conn1, "CREATE USER MAPPING for knizhnik SERVER dtm options  (user 'knizhnik')");

    // start transaction
    exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
    exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)

    for i := 0; i < N_ACCOUNTS; i++ {
        exec(conn1, "insert into t values($1, $2)", i, INIT_AMOUNT)
        exec(conn2, "insert into t values($1, $2)", ^i, INIT_AMOUNT)
    }

    exec(conn1, "commit")
    exec(conn2, "commit")
}

func progress(total int, cCommits chan int, cAborts chan int) {
    commits := 0
    aborts := 0
    start := time.Now()
    for newcommits := range cCommits {
        newaborts := <-cAborts
        commits += newcommits
        aborts += newaborts
        if time.Since(start).Seconds() > 10 {
            fmt.Printf(
                "progress %0.2f%%: %d commits, %d aborts\n",
                float32(commits) * 100.0 / float32(total), commits, aborts,
            )
            start = time.Now()
        }
    }
}

func transfer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    var err error
    var xid int32
    var nAborts = 0
    var nCommits = 0
    var myCommits = 0

    conn, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn.Close()

    start := time.Now()
    for myCommits < N_ITERATIONS {
        amount := 1
        account1 := rand.Intn(N_ACCOUNTS)
        account2 := ^rand.Intn(N_ACCOUNTS)

        exec(conn, "begin")
        xid = execQuery(conn, "select dtm_begin_transaction(2)")
        exec(conn, "select postgres_fdw_exec('t_fdw'::regclass::oid, 'select public.dtm_join_transaction(" + strconv.Itoa(int(xid)) + ")')")
        exec(conn, "commit")

        exec(conn, "begin transaction isolation level " + ISOLATION_LEVEL)

        ok1 := execUpdate(conn, "update t set v = v - $1 where u=$2", amount, account1)
        ok2 := execUpdate(conn, "update t set v = v + $1 where u=$2", amount, account2)

        if !ok1 || !ok2 {
           exec(conn, "rollback")
           nAborts += 1
        } else {
           exec(conn, "commit")
           nCommits += 1
           myCommits += 1
        }

        if time.Since(start).Seconds() > 10 {
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

func inspect(wg *sync.WaitGroup) {
    var sum int64
    var prevSum int64 = 0
    var xid int32

    {
        conn, err := pgx.Connect(cfg1)
        checkErr(err)

        for running {

            exec(conn, "begin")
            xid = execQuery(conn, "select dtm_begin_transaction(2)")
            exec(conn, "select postgres_fdw_exec('t_fdw'::regclass::oid, 'select public.dtm_join_transaction(" + strconv.Itoa(int(xid)) + ")')")
            exec(conn, "commit")

            exec(conn, "begin transaction isolation level " + ISOLATION_LEVEL)

            sum = execQuery64(conn, "select sum(v) from t")

            if (sum != prevSum) {
                fmt.Printf("Total=%d xid=%d\n", sum, xid)
                prevSum = sum
            }

            exec(conn, "commit")
        }
        conn.Close()
    }
    wg.Done()
}

func main() {
    var transferWg sync.WaitGroup
    var inspectWg sync.WaitGroup

    prepare_db()

    cCommits := make(chan int)
    cAborts := make(chan int)
    go progress(TRANSFER_CONNECTIONS * N_ITERATIONS, cCommits, cAborts)

    transferWg.Add(TRANSFER_CONNECTIONS)
    for i:=0; i<TRANSFER_CONNECTIONS; i++ {
        go transfer(i, cCommits, cAborts, &transferWg)
    }
    running = true
    inspectWg.Add(1)
    go inspect(&inspectWg)

    transferWg.Wait()
    running = false
    inspectWg.Wait()

    fmt.Printf("done\n")
}

func exec(conn *pgx.Conn, stmt string, arguments ...interface{}) {
    var err error
    // fmt.Println(stmt)
    _, err = conn.Exec(stmt, arguments... )
    checkErr(err)
}

func execUpdate(conn *pgx.Conn, stmt string, arguments ...interface{}) bool {
    var err error
    // fmt.Println(stmt)
    _, err = conn.Exec(stmt, arguments... )
    //if err != nil {
    //    fmt.Println(err)
    //}
    return err == nil
}

func execQuery(conn *pgx.Conn, stmt string, arguments ...interface{}) int32 {
    var err error
    var result int32
    err = conn.QueryRow(stmt, arguments...).Scan(&result)
    checkErr(err)
    return result
}

func execQuery64(conn *pgx.Conn, stmt string, arguments ...interface{}) int64 {
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

// vim: expandtab ts=4 sts=4 sw=4
