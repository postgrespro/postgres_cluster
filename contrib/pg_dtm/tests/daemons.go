package main

import (
	"log"
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

func dtmd(bin string, datadir string, servers []string, id int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-d", datadir,
		"-i", strconv.Itoa(id),
	}
	for _, server := range servers {
		argv = append(argv, "-r", server)
	}
	log.Println(argv)

	name := "dtmd " + datadir
	c := make(chan string)

	go cmd_to_channel(argv, name, c)

	for s := range c {
		log.Printf("[%s] %s\n", name, s)
	}

	wg.Done()
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
}

func postgres(bin string, datadir string, dtmservers []string, port int, nodeid int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-D", datadir,
		"-p", strconv.Itoa(port),
		"-c", "dtm.buffer_size=65536",
//		"-c", "dtm.buffer_size=0",
		"-c", "dtm.servers=" + strings.Join(dtmservers, ","),
		"-c", "autovacuum=off",
		"-c", "fsync=off",
		"-c", "synchronous_commit=on",
		"-c", "shared_preload_libraries=pg_dtm",
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
		"dtmd": srcroot + "/contrib/pg_dtm/dtmd/bin/dtmd",
		"initdb": prefix + "/bin/initdb",
		"postgres": prefix + "/bin/postgres",
	}

	datadirs := []string{"/tmp/data0", "/tmp/data1", "/tmp/data2"}
	dtmdirs := []string{"/tmp/dtm0", "/tmp/dtm1", "/tmp/dtm2"}

	check_bin(&bin);

	if doInitDb {
		for _, datadir := range datadirs {
			initdb(bin["initdb"], datadir)
		}
	}

	var wg sync.WaitGroup

	var dtmservers []string
	for i := range dtmdirs {
		dtmservers = append(dtmservers, DtmHost + ":" + strconv.Itoa(DtmPort - i))
	}
	for i, dir := range dtmdirs {
		wg.Add(1)
		go dtmd(bin["dtmd"], dir, dtmservers, i, &wg)
	}

	time.Sleep(3 * time.Second)

	for i, dir := range datadirs {
		wg.Add(1)
		go postgres(bin["postgres"], dir, dtmservers, PgPort + i, i, &wg)
	}

	wg.Wait()

	log.Println("done.")
}

