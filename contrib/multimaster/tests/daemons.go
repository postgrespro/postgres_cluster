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
	DtmHost = "127.0.0.1"
	DtmPort = 5431
	PgPort = 5432
)

func arbiter(bin string, datadir string, servers []string, id int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-d", datadir,
		"-i", strconv.Itoa(id),
	}
	for _, server := range servers {
		argv = append(argv, "-r", server)
	}
	log.Println(argv)

	name := "arbiter " + datadir
	c := make(chan string)

	go cmd_to_channel(argv, name, c)

	for s := range c {
		log.Printf("[%s] %s\n", name, s)
	}

	wg.Done()
}

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

func initarbiter(arbiterdir string) {
	if err := os.RemoveAll(arbiterdir); err != nil {
		log.Fatal(err)
	}
	if err := os.MkdirAll(arbiterdir, os.ModeDir | 0777); err != nil {
		log.Fatal(err)
	}
}

func postgres(bin string, datadir string, postgresi []string, arbiters []string, port int, nodeid int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-D", datadir,
		"-p", strconv.Itoa(port),
		"-c", "multimaster.buffer_size=65536",
		"-c", "multimaster.conn_strings=" + strings.Join(postgresi, ","),
		"-c", "multimaster.node_id=" + strconv.Itoa(nodeid + 1),
		"-c", "multimaster.arbiters=" + strings.Join(arbiters, ","),
		"-c", "multimaster.workers=8",
		"-c", "multimaster.queue_size=1073741824",
		"-c", "wal_level=logical",
		"-c", "wal_sender_timeout=0",
		"-c", "max_wal_senders=10",
		"-c", "max_worker_processes=100",
		"-c", "max_replication_slots=10",
		"-c", "autovacuum=off",
		"-c", "fsync=off",
		"-c", "synchronous_commit=on",
		"-c", "max_connections=200",
		"-c", "shared_preload_libraries=multimaster",
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
		"arbiter": srcroot + "/contrib/arbiter/bin/arbiter",
		"initdb": prefix + "/bin/initdb",
		"postgres": prefix + "/bin/postgres",
	}

	datadirs := []string{"/tmp/data0", "/tmp/data1", "/tmp/data2"}
	//arbiterdirs := []string{"/tmp/arbiter0", "/tmp/arbiter1", "/tmp/arbiter2"}
	arbiterdirs := []string{"/tmp/arbiter0"}

	check_bin(&bin);

	if doInitDb {
		for _, datadir := range datadirs {
			initdb(bin["initdb"], datadir)
		}
		for _, arbiterdir := range arbiterdirs {
			initarbiter(arbiterdir)
		}
	}

	var wg sync.WaitGroup

	var arbiters []string
	for i := range arbiterdirs {
		arbiters = append(arbiters, DtmHost + ":" + strconv.Itoa(DtmPort - i))
	}
	for i, dir := range arbiterdirs {
		wg.Add(1)
		go arbiter(bin["arbiter"], dir, arbiters, i, &wg)
	}

	time.Sleep(3 * time.Second)

	var postgresi []string
	for i := range datadirs {
		postgresi = append(
			postgresi,
			fmt.Sprintf("dbname=postgres host=127.0.0.1 port=%d sslmode=disable", PgPort + i),
		)
	}
	for i, dir := range datadirs {
		wg.Add(1)
		go postgres(bin["postgres"], dir, postgresi, arbiters, PgPort + i, i, &wg)
	}

	wg.Wait()

	log.Println("done.")
}

