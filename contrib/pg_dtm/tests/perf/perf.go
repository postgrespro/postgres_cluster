package main

import (
    "fmt"
    "flag"
    "os"
    "sync"
    "time"
    "github.com/jackc/pgx"
)

type ConnStrings []string

var backend interface{
    prepare(connstrs []string)
    writer(id int, cCommits chan int, cAborts chan int, wg *sync.WaitGroup)
    reader(wg *sync.WaitGroup, cFetches chan int, inconsistency *bool)
}

var cfg struct {
    ConnStrs ConnStrings
    Backend string
    Verbose bool
    UseDtm bool
    Init bool
    Parallel bool
    Isolation string
    AccountsNum int
    ReadersNum int
    IterNum int

    Writers struct {
        Num int
        StartId int
    }
}

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
        cfg.AccountsNum, 0,
    )
    fmt.Printf("Readers: %d\n", cfg.ReadersNum)

    fmt.Printf(
        "Writers: %d × %d updates\n",
        cfg.Writers.Num, cfg.IterNum,
    )
}

func init() {
    flag.StringVar(&cfg.Backend, "b", "transfers",
        "Backend to use. Possible optinos: transfers, fdw, pgshard, readers.")
    flag.Var(&cfg.ConnStrs, "C",
    	"Connection string (repeat for multiple connections)")
    flag.BoolVar(&cfg.Init, "i", false,
    	"Init database")
    flag.BoolVar(&cfg.UseDtm, "g", false,
    	"Use DTM to keep global consistency")
    flag.IntVar(&cfg.AccountsNum, "a", 100000,
    	"The number of bank accounts")
    flag.IntVar(&cfg.Writers.StartId, "s", 0,
    	"StartID. Script will update rows starting from this value")
    flag.IntVar(&cfg.IterNum, "n", 10000,
    	"The number updates each writer (reader in case of Reades backend) performs")
    flag.IntVar(&cfg.ReadersNum, "r", 1,
    	"The number of readers")
    flag.IntVar(&cfg.Writers.Num, "w", 8,
    	"The number of writers")
    flag.BoolVar(&cfg.Verbose, "v", false,
    	"Show progress and other stuff for mortals")
    flag.BoolVar(&cfg.Parallel, "p", false,
    	"Use parallel execs")
    repread := flag.Bool("l", false,
    	"Use 'repeatable read' isolation level instead of 'read committed'")
    flag.Parse()

    if len(cfg.ConnStrs) == 0 {
        flag.PrintDefaults()
        os.Exit(1)
    }

    if cfg.AccountsNum < 2 {
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
    if len(cfg.ConnStrs) < 2 {
        fmt.Println("ERROR: This test needs at leas two connections")
        os.Exit(1)
    }

    switch cfg.Backend {
        case "transfers":
            backend = new(Transfers)
        case "fdw":
            backend = new(TransfersFDW)
        case "readers":
            backend = new(Readers)
        case "pgshard":
            backend = new(TransfersPgShard)
        default:
            fmt.Println("No backend named: '%s'\n", cfg.Backend)
            return
    }

    start := time.Now()

    if (cfg.Init){
        backend.prepare(cfg.ConnStrs)
        fmt.Printf("database prepared in %0.2f seconds\n", time.Since(start).Seconds())
        return
    }

    var writerWg sync.WaitGroup
    var readerWg sync.WaitGroup

    cCommits := make(chan int)
    cFetches:= make(chan int)
    cAborts := make(chan int)

    go progress(cfg.Writers.Num * cfg.IterNum, cCommits, cAborts)

    start = time.Now()
    writerWg.Add(cfg.Writers.Num)
    for i := 0; i < cfg.Writers.Num; i++ {
        go backend.writer(i, cCommits, cAborts, &writerWg)
    }
    running = true

    inconsistency := false
    readerWg.Add(cfg.ReadersNum)
    for i := 0; i < cfg.ReadersNum; i++ {
        go backend.reader(&readerWg, cFetches, &inconsistency)
    }

    writerWg.Wait()
    running = false
    readerWg.Wait()

    fmt.Printf("writers finished in %0.2f seconds\n",
    	time.Since(start).Seconds())
    fmt.Printf("TPS = %0.2f\n",
    	float64(cfg.Writers.Num*cfg.IterNum)/time.Since(start).Seconds())

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
