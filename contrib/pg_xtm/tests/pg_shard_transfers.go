package main

import (
    "fmt"
    "sync"
    // "github.com/jackc/pgx"
    "pgx"
)

const (
    TRANSFER_CONNECTIONS = 1
    INIT_AMOUNT = 10000
    N_ITERATIONS = 5000
    N_ACCOUNTS = TRANSFER_CONNECTIONS
)

var cfg = pgx.ConnConfig{
        Host:     "127.0.0.1",
        Port:     5433,
        // Database: "postgres",
    }

var running = false

func transfer(id int, wg *sync.WaitGroup) {
    var err error
    var conn *pgx.Conn

    conn, err = pgx.Connect(cfg)
    checkErr(err)
    defer conn.Close()

    for i:=0; i < N_ITERATIONS; i++ {
        exec(conn, "begin")
        exec(conn, "update t_10000 set v = v + 1 where u=3")
        exec(conn, "update t_10000 set v = v - 1 where u=4")
        exec(conn, "commit")
    }

    wg.Done()
}

func inspect(wg *sync.WaitGroup) {
    var sum int64
    var prevSum int64 = 0

    conn, err := pgx.Connect(cfg)
    checkErr(err)

    for running {
        sum = execQuery(conn, "select sum(v) from t_10000")
        if sum != prevSum {
        	fmt.Println("Total = ", sum);
        	prevSum = sum
        }
    }

    conn.Close()
    wg.Done()
}

func main() {
    // var transferWg sync.WaitGroup
    // var inspectWg sync.WaitGroup
    var err error
    var conn *pgx.Conn
    var s int64

    conn, err = pgx.Connect(cfg)
    checkErr(err)
    defer conn.Close()

    // err = conn.QueryRow("select sum(v) from t_10000").Scan(&s)
    // checkErr(err)

    s = execQuery(conn, "select sum(v) from t_10000")
    fmt.Println(s)


    // transferWg.Add(TRANSFER_CONNECTIONS)
    // for i:=0; i<TRANSFER_CONNECTIONS; i++ {
    //     go transfer(i, &transferWg)
    // }

    // running = true
    // inspectWg.Add(1)
    // go inspect(&inspectWg)

    // transferWg.Wait()

    // running = false
    // inspectWg.Wait()

    fmt.Printf("done\n")
}

func exec(conn *pgx.Conn, stmt string) {
    var err error
    _, err = conn.Exec(stmt)
    checkErr(err)
}

func execQuery(conn *pgx.Conn, stmt string) int64 {
    var err error
    var result int64
    // result, err = conn.SimpleQuery(stmt)
    // err = conn.QueryRow(stmt).Scan(&result)
    err = conn.SimpleQuery(stmt).Scan(&result)
    checkErr(err)
    return result
}

func checkErr(err error) {
    if err != nil {
        panic(err)
    }
}

