package main

import (
    "fmt"
    "flag"
    "os"
    "sync"
    "math/rand"
    "strconv"
    "time"
    "github.com/jackc/pgx"
)

type ConnStrings []string

// The first method of flag.Value interface
func (c *ConnStrings) String() string {
    if len(*c) > 0 {
        return (*c)[0]
    } else {
        return ""
    }
}

// The second method of flag.Value interface
func (c *ConnStrings) Set(value string) error {
    *c = append(*c, value)
    return nil
}

var cfg struct {
    ConnStrs ConnStrings

    Verbose bool
    UseDtm bool
    InitOnly bool
    SkipInit bool
    Parallel bool

    Isolation string // "repeatable read" or "read committed"

    Accounts struct {
        Num int
        Balance int
    }

    Readers struct {
        Num int
    }

    Writers struct {
        Num int
        Updates int
        StartId int
        AllowGlobal bool
        AllowLocal bool
        PrivateRows bool
        UseCursors bool
    }
}

func append_with_comma(s *string, x string) {
    if len(*s) > 0 {
        *s = *s + ", " + x
    } else {
        *s = x
    }
}

func dump_cfg() {
    fmt.Printf("Connections: %d\n", len(cfg.ConnStrs))
    for _, cs := range cfg.ConnStrs {
        fmt.Printf("    %s\n", cs)
    }
    fmt.Printf("Isolation: %s\n", cfg.Isolation)
    fmt.Printf(
        "Accounts: %d × $%d\n",
        cfg.Accounts.Num, cfg.Accounts.Balance,
    )
    fmt.Printf("Readers: %d\n", cfg.Readers.Num)

    utypes := ""
    if cfg.Writers.AllowGlobal {
        append_with_comma(&utypes, "global")
    }
    if cfg.Writers.AllowLocal {
        append_with_comma(&utypes, "local")
    }
    if cfg.Writers.PrivateRows {
        append_with_comma(&utypes, "private")
    }
    if cfg.Writers.UseCursors {
        append_with_comma(&utypes, "cursors")
    }

    fmt.Printf(
        "Writers: %d × %d updates (%s)\n",
        cfg.Writers.Num, cfg.Writers.Updates,
        utypes,
    )
}

func init() {
    flag.Var(&cfg.ConnStrs, "d", "Connection string (repeat for multiple connections)")
    repread := flag.Bool("i", false, "Use 'repeatable read' isolation level instead of 'read committed'")
    flag.IntVar(&cfg.Accounts.Num, "a", 100000, "The number of bank accounts")
    flag.IntVar(&cfg.Accounts.Balance, "b", 10000, "The initial balance of each bank account")
    flag.IntVar(&cfg.Readers.Num, "r", 1, "The number of readers")
    flag.IntVar(&cfg.Writers.Num, "w", 8, "The number of writers")
    flag.IntVar(&cfg.Writers.Updates, "u", 10000, "The number updates each writer performs")
    flag.IntVar(&cfg.Writers.StartId, "k", 0, "Script will update rows starting from this value")
    flag.BoolVar(&cfg.Verbose, "v", false, "Show progress and other stuff for mortals")
    flag.BoolVar(&cfg.UseDtm, "m", false, "Use DTM to keep global consistency")
    flag.BoolVar(&cfg.Writers.AllowGlobal, "g", false, "Allow global updates")
    flag.BoolVar(&cfg.Writers.AllowLocal, "l", false, "Allow local updates")
    flag.BoolVar(&cfg.Writers.PrivateRows, "p", false, "Private rows (avoid waits/aborts caused by concurrent updates of the same rows)")
    flag.BoolVar(&cfg.Writers.UseCursors, "c", false, "Use cursors for updates")
    flag.BoolVar(&cfg.InitOnly, "f", false, "Only feed databases with data")
    flag.BoolVar(&cfg.SkipInit, "s", false, "Skip init phase")
    flag.BoolVar(&cfg.Parallel, "o", false, "Use parallel execs")
    flag.Parse()

    if len(cfg.ConnStrs) == 0 {
        flag.PrintDefaults()
        os.Exit(1)
    }

    if !cfg.Writers.AllowGlobal && !cfg.Writers.AllowLocal {
        fmt.Println(
            "Both local and global updates disabled,\n" +
            "please enable at least one of the types!",
        )
        os.Exit(1)
    }

    if cfg.Accounts.Num < 2 {
        fmt.Println(
            "There should be at least 2 accounts (to avoid deadlocks)",
        )
        os.Exit(1)
    }

    if *repread {
        cfg.Isolation = "repeatable read"
    } else {
        cfg.Isolation = "read committed"
    }

    dump_cfg()
}

func main() {
    start := time.Now()

    if (!cfg.SkipInit){
        prepare(cfg.ConnStrs)
        fmt.Printf("database prepared in %0.2f seconds\n", time.Since(start).Seconds())
    }

    if (cfg.InitOnly) {
        return
    }

    var writerWg sync.WaitGroup
    var readerWg sync.WaitGroup

    cCommits := make(chan int)
    cAborts := make(chan int)
    go progress(cfg.Writers.Num * cfg.Writers.Updates, cCommits, cAborts)

    start = time.Now()
    writerWg.Add(cfg.Writers.Num)
    for i := 0; i < cfg.Writers.Num; i++ {
        go writer(i, cCommits, cAborts, &writerWg)
    }
    running = true

    inconsistency := false
    readerWg.Add(cfg.Readers.Num)
    for i := 0; i < cfg.Readers.Num; i++ {
        go reader(&readerWg, &inconsistency)
    }

    writerWg.Wait()
    fmt.Printf("writers finished in %0.2f seconds\n", time.Since(start).Seconds())
    fmt.Printf("TPS = %0.2f\n", float64(cfg.Writers.Num*cfg.Writers.Updates)/time.Since(start).Seconds())

    running = false
    readerWg.Wait()

    if inconsistency {
        fmt.Printf("INCONSISTENCY DETECTED\n")
    }
    fmt.Printf("done.\n")
}

var running = false

func asyncCommit(conn *pgx.Conn, wg *sync.WaitGroup) {
    exec(conn, "commit")
    wg.Done()
}

func commit(conns ...*pgx.Conn) {
    var wg sync.WaitGroup
    wg.Add(len(conns))
    for _, conn := range conns {
        go asyncCommit(conn, &wg)
    }
    wg.Wait()
}

func parallel_exec(conns []*pgx.Conn, requests []string) bool {
    var wg sync.WaitGroup
    state := true
    wg.Add(len(conns))
    for i := range conns {
        if cfg.Parallel {
            go func(j int) {
                _, err := conns[j].Exec(requests[j])
                if err != nil {
                    state = false
                }
                wg.Done()
            }(i)
        } else {
            _, err := conns[i].Exec(requests[i])
            if err != nil {
                state = false
            }
            wg.Done()
        }
    }
    wg.Wait()
    return state
}

func prepare_one(connstr string, wg *sync.WaitGroup) {
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
    exec(conn, "insert into t (select generate_series(0,$1-1), $2)", cfg.Accounts.Num, cfg.Accounts.Balance)

    exec(conn, "commit")
    wg.Done()
}

func prepare(connstrs []string) {
    var wg sync.WaitGroup
    wg.Add(len(connstrs))
    for _, connstr := range connstrs {
        go prepare_one(connstr, &wg)
    }
    wg.Wait()
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
            if cfg.Verbose {
                fmt.Printf(
                    "progress %0.2f%%: %d commits, %d aborts\n",
                    float32(commits) * 100.0 / float32(total), commits, aborts,
                )
            }
            start = time.Now()
        }
    }
}

func writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup) {
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

    // start := time.Now()
    for myCommits < cfg.Writers.Updates {
        amount := 1

        from_acc := cfg.Writers.StartId + 2*id + 1
        to_acc   := cfg.Writers.StartId + 2*id + 2

        src := conns[rand.Intn(len(conns))]
        dst := conns[rand.Intn(len(conns))]
        if src == dst {
            continue
        }

        if cfg.UseDtm {
            xid := execQuery(src, "select dtm_begin_transaction(); begin transaction isolation level " + cfg.Isolation)
            exec(dst, "select dtm_join_transaction(" + strconv.Itoa(xid) + "); begin transaction isolation level " + cfg.Isolation)
        }

        // parallel_exec([]*pgx.Conn{src,dst}, []string{"begin transaction isolation level " + cfg.Isolation, "begin transaction isolation level " + cfg.Isolation})

        ok := true


        sql1 := "update t set v = v - " + strconv.Itoa(amount) + " where u=" + strconv.Itoa(from_acc)
        sql2 := "update t set v = v + " + strconv.Itoa(amount) + " where u=" + strconv.Itoa(to_acc)

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

        // if time.Since(start).Seconds() > 1 {
        //     cCommits <- nCommits
        //     cAborts <- nAborts
        //     nCommits = 0
        //     nAborts = 0
        //     start = time.Now()
        // }
    }
    cCommits <- nCommits
    cAborts <- nAborts
    wg.Done()
}

func reader(wg *sync.WaitGroup, inconsistency *bool) {
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
