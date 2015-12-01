package main

import (
    "fmt"
    "sync"
    _ "github.com/lib/pq"
    "database/sql"
    "strconv"
    "math/rand"
)

type TransfersPgShard struct {}

func (t TransfersPgShard) prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs)-1)
    for i, connstr := range connstrs[1:] {
        go t.prepare_slave(i, connstr, &wg)
    }
    wg.Wait()
    t.prepare_master()
}

func (t TransfersPgShard) prepare_slave(id int, connstr string, wg *sync.WaitGroup) {
    conn, err := sql.Open("postgres", connstr)
    checkErr(err)
    defer conn.Close()

    if cfg.UseDtm {
        _exec(conn, "drop extension if exists pg_dtm --")
        _exec(conn, "create extension pg_dtm")
    }

    drop_sql := fmt.Sprintf("drop table if exists t_1000%d", id)
    _exec(conn, drop_sql)

    conn.Close()
    wg.Done()
}

func (t TransfersPgShard) prepare_master() {
    conn, err := sql.Open("postgres", cfg.ConnStrs[0])
    checkErr(err)

    _exec(conn, "drop extension if exists pg_shard CASCADE")
    _exec(conn, "create extension pg_shard")
    _exec(conn, "drop table if exists t")
    _exec(conn, "create table t(u int primary key, v int)")
    _exec(conn, "select master_create_distributed_table(table_name := 't', partition_column := 'u')")

    master_sql := fmt.Sprintf(
        "select master_create_worker_shards(table_name := 't', shard_count := %d, replication_factor := 1)",
        len(cfg.ConnStrs)-1)
    _exec(conn, master_sql)

    fmt.Println("Database feed started")
    for i:=0; i<=cfg.AccountsNum; i++ {
        _exec(conn, "insert into t values(" + strconv.Itoa(i) + ", 0)")
    }
    fmt.Println("Database feed finished")

    conn.Close()
}

func (t TransfersPgShard) writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
    conn, err := sql.Open("postgres", cfg.ConnStrs[0])
    checkErr(err)

    i:=0
    for i=0; i < cfg.IterNum; i++ {
        amount := 1
        account1 := rand.Intn(cfg.AccountsNum-1)+1
        account2 := rand.Intn(cfg.AccountsNum-1)+1

        _exec(conn, "begin")
        _exec(conn, fmt.Sprintf("update t set v = v - %d where u=%d", amount, account1))
        _exec(conn, fmt.Sprintf("update t set v = v + %d where u=%d", amount, account2))
        _exec(conn, "commit")

        if i%1000==0 {
            fmt.Printf("%d tx processed.\n", i)
        }
    }

    cCommits <- i
    cAborts <- 0

    conn.Close()
    wg.Done()
}

func (t TransfersPgShard) reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool) {
    var sum int64
    var prevSum int64 = 0

    conn, err := sql.Open("postgres", cfg.ConnStrs[0])
    checkErr(err)

    for running {
        sum = _execQuery(conn, "select sum(v) from t")
        if sum != prevSum {
            fmt.Println("Total = ", sum)
            *inconsistency = true
            prevSum = sum
        }
    }

    conn.Close()
    wg.Done()
}

func _exec(conn *sql.DB, stmt string) {
    var err error
    _, err = conn.Exec(stmt)
    checkErr(err)
}

func _execQuery(conn *sql.DB, stmt string) int64 {
    var err error
    var result int64
    err = conn.QueryRow(stmt).Scan(&result)
    checkErr(err)
    return result
}
