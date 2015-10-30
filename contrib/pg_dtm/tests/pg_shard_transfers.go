package main

import (
    "fmt"
    "sync"
    _ "github.com/lib/pq"
    "database/sql"
    "strconv"
    "math/rand"
    "time"
)

const (
    TRANSFER_CONNECTIONS = 150
    INIT_AMOUNT = 1000
    N_ITERATIONS = 3000
    N_ACCOUNTS = 10000
)

var cfg = "host=astro10 port=25432 dbname=postgres sslmode=disable"
var cfg1 = "host=astro9 port=25432 dbname=postgres sslmode=disable"
var cfg2 = "host=astro8 port=25432 dbname=postgres sslmode=disable"
var cfg3 = "host=astro6 port=25432 dbname=postgres sslmode=disable"

// var cfg = "host=127.0.0.1 port=5432 sslmode=disable"
// var cfg1 = "host=127.0.0.1 port=5433 sslmode=disable"
// var cfg2 = "host=127.0.0.1 port=5434 sslmode=disable"

var running = false

func prepare_db() {
    conn1, err := sql.Open("postgres", cfg1)
    checkErr(err)
    exec(conn1, "drop table if exists t_10000")
    conn1.Close()

    conn2, err := sql.Open("postgres", cfg2)
    checkErr(err)
    exec(conn2, "drop table if exists t_10001")
    conn2.Close()


    conn3, err := sql.Open("postgres", cfg3)
    checkErr(err)
    exec(conn3, "drop table if exists t_10003")
    conn3.Close()

    conn, err := sql.Open("postgres", cfg)
    checkErr(err)

    exec(conn, "drop extension if exists pg_shard CASCADE")
    exec(conn, "create extension pg_shard")
    exec(conn, "drop table if exists t")
    exec(conn, "create table t(u int primary key, v int)")
    exec(conn, "select master_create_distributed_table(table_name := 't', partition_column := 'u')")
    exec(conn, "select master_create_worker_shards(table_name := 't', shard_count := 3, replication_factor := 1)")

    for i:=1; i<=N_ACCOUNTS; i++ {
        exec(conn, "insert into t values(" + strconv.Itoa(i) + ",10000)")
    }
    fmt.Printf("Prepared!\n")
    conn.Close()
}

func transfer(id int, wg *sync.WaitGroup) {
    conn, err := sql.Open("postgres", cfg)
    checkErr(err)
    defer conn.Close()

    for i:=0; i < N_ITERATIONS; i++ {
        amount := 1
        account1 := rand.Intn(N_ACCOUNTS-1)+1
        account2 := rand.Intn(N_ACCOUNTS-1)+1
        exec(conn, "begin")
        exec(conn, fmt.Sprintf("update t set v = v - %d where u=%d", amount, account1))
        exec(conn, fmt.Sprintf("update t set v = v + %d where u=%d", amount, account2))

        // exec(conn, "update t set v = v + 1 where u=1")
        // exec(conn, "update t set v = v - 1 where u=2")
        exec(conn, "commit")

        if i%1000==0 {
            fmt.Printf("%d tx processed.\n", i)
        }
    }

    wg.Done()
}

func inspect(wg *sync.WaitGroup) {
    var sum int64
    var prevSum int64 = 0

    conn, err := sql.Open("postgres", cfg)
    checkErr(err)

    for running {
        sum = execQuery(conn, "select sum(v) from t")
        if sum != prevSum {
            fmt.Println("Total = ", sum);
            prevSum = sum
        }
    }

    conn.Close()
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
    go inspect(&inspectWg)

    transferWg.Wait()
    running = false
    
    inspectWg.Wait()

    fmt.Printf("Elapsed time %f seconds\n", time.Since(start).Seconds())
    fmt.Printf("TPS = %f\n", float64(TRANSFER_CONNECTIONS*N_ITERATIONS)/time.Since(start).Seconds())
}

func exec(conn *sql.DB, stmt string) {
    var err error
    _, err = conn.Exec(stmt)
    checkErr(err)
}

func execQuery(conn *sql.DB, stmt string) int64 {
    var err error
    var result int64
    err = conn.QueryRow(stmt).Scan(&result)
    checkErr(err)
    return result
}

func checkErr(err error) {
    if err != nil {
        panic(err)
    }
}
