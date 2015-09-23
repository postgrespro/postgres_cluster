package main

import (
    "fmt"
    "sync"
    "math/rand"
    "github.com/jackc/pgx"
)

const (
    TRANSFER_CONNECTIONS = 8
    INIT_AMOUNT = 10000
    N_ITERATIONS = 10000
    N_ACCOUNTS = TRANSFER_CONNECTIONS//100000
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
//    var xid int32

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
    
//    xid = execQuery(conn1, "select dtm_begin_transaction(2)")
//    exec(conn2, "select dtm_join_transaction($1)", xid)

    // strt transaction
    exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
    exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)
        
    for i := 0; i < N_ACCOUNTS; i++ {
        exec(conn1, "insert into t values($1, $2)", i, INIT_AMOUNT)
        exec(conn2, "insert into t values($1, $2)", i, INIT_AMOUNT)
    }
    
    commit(conn1, conn2)
}

func max(a, b int64) int64 {
    if a >= b {
        return a
    } 
    return b
}

func transfer(id int, wg *sync.WaitGroup) {
    var err error
    var xid int32
    var nConflicts = 0

    conn1, err := pgx.Connect(cfg1)
    checkErr(err)
    defer conn1.Close()

    conn2, err := pgx.Connect(cfg2)
    checkErr(err)
    defer conn2.Close()

    for i := 0; i < N_ITERATIONS; i++ {
        //amount := 2*rand.Intn(2) - 1
        amount := 1
        account1 := rand.Intn(N_ACCOUNTS) 
        account2 := rand.Intn(N_ACCOUNTS)

        xid = execQuery(conn1, "select dtm_begin_transaction(2)")
        exec(conn2, "select dtm_join_transaction($1)", xid)

        // start transaction
        exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
        exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)
        
        ok1 := execUpdate(conn1, "update t set v = v + $1 where u=$2", amount, account1)  
        ok2 := execUpdate(conn2, "update t set v = v - $1 where u=$2", amount, account2) 
        if !ok1 || !ok2 {  
            exec(conn1, "rollback")
            exec(conn2, "rollback")
            nConflicts += 1
            i -= 1
        } else { 
            commit(conn1, conn2)
        }       
    }
    fmt.Println("Test completed with ",nConflicts," conflicts")
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

       
        xid = execQuery(conn1, "select dtm_begin_transaction(2)")
        exec(conn2, "select dtm_join_transaction($1)", xid)
        
        exec(conn1, "begin transaction isolation level " + ISOLATION_LEVEL)
        exec(conn2, "begin transaction isolation level " + ISOLATION_LEVEL)
 
        sum1 = execQuery64(conn1, "select sum(v) from t")
        sum2 = execQuery64(conn2, "select sum(v) from t")

        sum = sum1 + sum2
        if (sum != prevSum) {
            fmt.Println("Total = ", sum, "xid=", xid, "snap1={", execQuery(conn1, "select dtm_get_current_snapshot_xmin()"), execQuery(conn1, "select dtm_get_current_snapshot_xmax()"), "}, snap2={",  execQuery(conn2, "select dtm_get_current_snapshot_xmin()"), execQuery(conn2, "select dtm_get_current_snapshot_xmax()"), "}")
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

    transferWg.Add(TRANSFER_CONNECTIONS)
    for i:=0; i<TRANSFER_CONNECTIONS; i++ {
        go transfer(i, &transferWg)
    }
    running = true
    inspectWg.Add(1)
    go inspect(&inspectWg)

    transferWg.Wait()
    running = false
    inspectWg.Wait()
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


