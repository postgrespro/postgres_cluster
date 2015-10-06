package main

import (
    "fmt"
    "sync"
    "math/rand"
    "time"
    "github.com/jackc/pgx"
)

const (
    TRANSFER_CONNECTIONS = 8
    INIT_AMOUNT = 10000
    N_ITERATIONS = 10000
    N_ACCOUNTS = 100000
    //ISOLATION_LEVEL = "repeatable read"
    ISOLATION_LEVEL = "read committed"
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
var nodes []int32 = []int32{0,1}

func asyncCommit(conn *pgx.Conn, wg *sync.WaitGroup) {
    exec(conn, "commit")
    wg.Done()
}

func commit(conn1, conn2 *pgx.Conn) {
    var wg sync.WaitGroup
    wg.Add(2)
    go asyncCommit(conn1, &wg)
    go asyncCommit(conn2, &wg)
    wg.Wait()
}

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

    // strt transaction
    exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
    exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)

    start := time.Now()
    for i := 0; i < N_ACCOUNTS; i++ {
        exec(conn1, "insert into t values($1, $2)", i, INIT_AMOUNT)
        exec(conn2, "insert into t values($1, $2)", i, INIT_AMOUNT)
        if time.Since(start).Seconds() > 1 {
            fmt.Printf(
                "inserted %0.2f%%: %d of %d records\n",
                float32(i + 1) * 100.0 / float32(N_ACCOUNTS), i + 1, N_ACCOUNTS,
            )
            start = time.Now()
        }
    }

    commit(conn1, conn2)
}

func max(a, b int64) int64 {
    if a >= b {
        return a
    }
    return b
}

func progress(total int, cCommits chan int, cAborts chan int) {
    commits := 0
    aborts := 0
    start := time.Now()
    for newcommits := range cCommits {
        newaborts := <-cAborts
        commits += newcommits
        aborts += newaborts
        if time.Since(start).Seconds() > 1 {
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

    var conn [2]*pgx.Conn

    conn[0], err = pgx.Connect(cfg1)
    checkErr(err)
    defer conn[0].Close()

    conn[1], err = pgx.Connect(cfg2)
    checkErr(err)
    defer conn[1].Close()

    start := time.Now()
    for myCommits < N_ITERATIONS {
        //amount := 2*rand.Intn(2000) - 1
        amount := 1
        account1 := rand.Intn(N_ACCOUNTS)
        account2 := rand.Intn(N_ACCOUNTS)

        src := conn[0]
        dst := conn[1]

        xid = execQuery(src, "select dtm_begin_transaction()")
        exec(dst, "select dtm_join_transaction($1)", xid)

        // start transaction
        exec(src, "begin transaction isolation level " + ISOLATION_LEVEL)
        exec(dst, "begin transaction isolation level " + ISOLATION_LEVEL)

        ok1 := execUpdate(src, "update t set v = v - $1 where u=$2", amount, account1)
        ok2 := execUpdate(dst, "update t set v = v + $1 where u=$2", amount, account2)

        if !ok1 || !ok2 {
            exec(src, "rollback")
            exec(dst, "rollback")
            nAborts += 1
        } else {
            commit(src, dst)
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

func inspect(wg *sync.WaitGroup) {
    var sum1, sum2, sum int64
    var prevSum int64 = 0
    var xid int32

    {
        conn1, err := pgx.Connect(cfg1)
        checkErr(err)

        conn2, err := pgx.Connect(cfg2)
        checkErr(err)

        for running {
            xid = execQuery(conn1, "select dtm_begin_transaction()")
            exec(conn2, "select dtm_join_transaction($1)", xid)

            exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
            exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)

            sum1 = execQuery64(conn1, "select sum(v) from t")
            sum2 = execQuery64(conn2, "select sum(v) from t")

            sum = sum1 + sum2
            if (sum != prevSum) {
                xmin1 := execQuery(conn1, "select dtm_get_current_snapshot_xmin()")
                xmax1 := execQuery(conn1, "select dtm_get_current_snapshot_xmax()")
                xmin2 := execQuery(conn2, "select dtm_get_current_snapshot_xmin()")
                xmax2 := execQuery(conn2, "select dtm_get_current_snapshot_xmax()")
                fmt.Printf(
                    "Total=%d xid=%d snap1=[%d, %d) snap2=[%d, %d)\n",
                    sum, xid, xmin1, xmax1, xmin2, xmax2,
                )
                prevSum = sum
            }

            commit(conn1, conn2)
        }
        conn1.Close()
        conn2.Close()
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
