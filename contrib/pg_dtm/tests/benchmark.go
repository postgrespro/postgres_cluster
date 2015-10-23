package main

import (
    "fmt"
    "flag"
    "os"
    "sync"
    "math/rand"
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

    Isolation string // "repeatable read" or "read committed"

    Time int

    Accounts struct {
        Num int
        Balance int
    }

    Readers struct {
        Num int
    }

    Writers struct {
        Num int
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
    fmt.Printf("Writers: %d\n", cfg.Writers.Num)
}

func init() {
    flag.Var(&cfg.ConnStrs, "d", "Connection string (repeat for multiple connections)")
    repread := flag.Bool("i", false, "Use 'repeatable read' isolation level instead of 'read committed'")
    flag.IntVar(&cfg.Accounts.Num, "a", 100000, "The number of bank accounts")
    flag.IntVar(&cfg.Accounts.Balance, "b", 0, "The initial balance of each bank account")
    flag.IntVar(&cfg.Readers.Num, "r", 4, "The number of readers")
    flag.IntVar(&cfg.Writers.Num, "w", 4, "The number of writers")
    flag.IntVar(&cfg.Time, "t", 10, "Time in seconds of running test")
    flag.BoolVar(&cfg.UseDtm, "m", false, "Use DTM to keep global consistency")
    flag.BoolVar(&cfg.InitOnly, "f", false, "Only feed databses with data")
    flag.BoolVar(&cfg.SkipInit, "s", false, "Skip init phase")
    flag.Parse()

    if len(cfg.ConnStrs) == 0 {
        flag.PrintDefaults()
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

    var wg sync.WaitGroup

    cUpdates := make(chan int)
    cFetches := make(chan int)

    wg.Add(cfg.Writers.Num + cfg.Readers.Num)
    running = true

    for i := 0; i < cfg.Writers.Num; i++ {
        go writer(&wg, cUpdates)
    }

    for i := 0; i < cfg.Readers.Num; i++ {
        go reader(&wg, cFetches)
    }
    
    time.Sleep(time.Duration(cfg.Time) * time.Second)      
    running = false

    totalUpdates := 0
    for i := 0; i < cfg.Writers.Num; i++ {
        totalUpdates += <- cUpdates
    }

    totalFetches := 0
    for i := 0; i < cfg.Readers.Num; i++ {
        totalFetches += <- cFetches
    }

    wg.Wait()    
    fmt.Printf("Perform %d updates and %d fetches\n", totalUpdates, totalFetches)
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

func writer(wg *sync.WaitGroup, cUpdates chan int) {
    var updates = 0
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
        acc := rand.Intn(cfg.Accounts.Num) 
        xid := execQuery(conns[0], "select dtm_begin_transaction()") 
        for i := 1; i < len(conns); i++ {
            exec(conns[i], "select dtm_join_transaction($1)", xid)
        }
        for _, conn := range conns {
            exec(conn, "begin transaction isolation level " + cfg.Isolation)
            exec(conn, "update t set v = v + 1 where u=$1", acc)
        }
        commit(conns...)
        updates++
    }    
    cUpdates <- updates 
    wg.Done()
}


func reader(wg *sync.WaitGroup, cFetches chan int) {
    var fetches = 0
    var conns []*pgx.Conn
    var sum int32 = 0

    for _, connstr := range cfg.ConnStrs {
        dbconf, err := pgx.ParseDSN(connstr)
        checkErr(err)

        conn, err := pgx.Connect(dbconf)
        checkErr(err)

        defer conn.Close()
        conns = append(conns, conn)
    }
    for running {   
        acc := rand.Intn(cfg.Accounts.Num) 
        con := rand.Intn(len(conns))
        sum += execQuery(conns[con], "select v from t where u=$1", acc)
        fetches++
    }    
    cFetches <- fetches
    wg.Done()
}

func exec(conn *pgx.Conn, stmt string, arguments ...interface{}) {
    var err error
    // fmt.Println(stmt)
    _, err = conn.Exec(stmt, arguments... )
    checkErr(err)
}

func execQuery(conn *pgx.Conn, stmt string, arguments ...interface{}) int32 {
    var err error
    var result int32
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
