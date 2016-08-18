package main

import (
	"log"
	"fmt"
	"os/exec"
	"io"
	"bufio"
	"sync"
	"flag"
	"os"
	"strconv"
	"strings"
	"time"
)

func read_to_channel(r io.Reader, c chan string, wg *sync.WaitGroup) {
	defer wg.Done()
	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		c <- scanner.Text()
	}
	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
}

func mux_readers_into_channel(a io.Reader, b io.Reader, c chan string) {
	var wg sync.WaitGroup
	wg.Add(2)
	go read_to_channel(a, c, &wg)
	go read_to_channel(b, c, &wg)
	wg.Wait()
	close(c)
}

func cmd_to_channel(argv []string, name string, out chan string) {
	var cmd exec.Cmd
	cmd.Path = argv[0]
	cmd.Args = argv
	log.Printf("starting '%s'\n", name)

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		log.Println(err)
	}

	stderr, err := cmd.StderrPipe()
	if err != nil {
		log.Println(err)
	}

	if err := cmd.Start(); err != nil {
		log.Fatal(err)
	}

	mux_readers_into_channel(stdout, stderr, out)
	cmd.Wait()
	log.Printf("'%s' finished\n", name)
}

const (
	PgPort = 5432
	RaftPort = 6543
)

func appendfile(filename string, lines ...string) {
	f, err := os.OpenFile(filename, os.O_APPEND | os.O_WRONLY, 0600)
	if err != nil {
		log.Fatal(err)
	}

	defer f.Close()

	for _, l := range lines {
		if _, err = f.WriteString(l + "\n"); err != nil {
			log.Fatal(err)
		}
	}
}

func initdb(bin string, datadir string) {
	if err := os.RemoveAll(datadir); err != nil {
		log.Fatal(err)
	}

	argv := []string{bin, datadir}
	name := "initdb " + datadir
	c := make(chan string)

	go cmd_to_channel(argv, name, c)

	for s := range c {
		log.Printf("[%s] %s\n", name, s)
	}

	appendfile(
		datadir + "/pg_hba.conf",
		"local replication all trust",
		"host replication all 127.0.0.1/32 trust",
		"host replication all ::1/128 trust",
	)
}

func postgres(bin string, datadir string, postgresi []string, port int, nodeid int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-D", datadir,
		"-p", strconv.Itoa(port),
		"-c", "autovacuum=off",
		"-c", "fsync=off",
		"-c", "max_connections=200",
		"-c", "max_replication_slots=10",
		"-c", "max_wal_senders=10",
		"-c", "max_worker_processes=11",
		"-c", "max_prepared_transactions=10",
		"-c", "default_transaction_isolation=repeatable read",
		"-c", "multimaster.conn_strings=" + strings.Join(postgresi, ","),
		"-c", "multimaster.node_id=" + strconv.Itoa(nodeid + 1),
		"-c", "multimaster.queue_size=52857600",
		"-c", "multimaster.max_nodes=3",
		"-c", "multimaster.workers=4",
		"-c", "multimaster.use_raftable=true",
		"-c", "multimaster.ignore_tables_without_pk=1",
		"-c", "multimaster.heartbeat_recv_timeout=1000",
		"-c", "multimaster.heartbeat_send_timeout=250",
		"-c", "multimaster.twopc_min_timeout=40000",
		"-c", "shared_preload_libraries=raftable,multimaster",
		"-c", "synchronous_commit=on",
		"-c", "wal_level=logical",
		"-c", "wal_sender_timeout=0",
		"-c", "log_checkpoints=on",
		"-c", "log_autovacuum_min_duration=0",
	}
	name := "postgres " + datadir
	c := make(chan string)

	go cmd_to_channel(argv, name, c)

	for s := range c {
		log.Printf("[%s] %s\n", name, s)
	}

	wg.Done()
}

func check_bin(bin *map[string]string) {
	for k, v := range *bin {
		path, err := exec.LookPath(v)
		if err != nil {
			log.Fatalf("'%s' executable not found", k)
		} else {
			log.Printf("'%s' executable is '%s'", k, path)
		}
	}
}

func get_prefix(srcroot string) string {
	makefile, err := os.Open(srcroot + "/src/Makefile.global")
	if err != nil {
		return "."
	}

	scanner := bufio.NewScanner(makefile)
	for scanner.Scan() {
		s := scanner.Text()
		if strings.HasPrefix(s, "prefix := ") {
			return strings.TrimPrefix(s, "prefix := ")
		}
	}
	return "."
}

var doInitDb bool = false
func init() {
	flag.BoolVar(&doInitDb, "i", false, "perform initdb")
	flag.Parse()
}

func main() {
	srcroot := "../../.."
	prefix := get_prefix(srcroot)

	bin := map[string]string{
		"initdb": prefix + "/bin/initdb",
		"postgres": prefix + "/bin/postgres",
	}

	datadirs := []string{"/tmp/data0", "/tmp/data1", "/tmp/data2"}

	check_bin(&bin);

	if doInitDb {
		for _, datadir := range datadirs {
			initdb(bin["initdb"], datadir)
		}
	}

	var wg sync.WaitGroup

	time.Sleep(3 * time.Second)

	var postgresi []string
	for i := range datadirs {
		postgresi = append(
			postgresi,
			fmt.Sprintf("dbname=postgres host=127.0.0.1 port=%d raftport=%d sslmode=disable", PgPort + i, RaftPort + i),
		)
	}
	for i, dir := range datadirs {
		wg.Add(1)
		go postgres(bin["postgres"], dir, postgresi, PgPort + i, i, &wg)
	}

	wg.Wait()

	log.Println("done.")
}

