package main

import (
	"log"
	"os/exec"
	"io"
	"bufio"
	"sync"
	"os"
	"strconv"
	"strings"
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

func dtmd(bin string, wg *sync.WaitGroup) {
	argv := []string{bin}
	name := "dtmd"
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

func postgres(bin string, datadir string, port int, nodeid int, wg *sync.WaitGroup) {
	argv := []string{
		bin,
		"-D", datadir,
		"-p", strconv.Itoa(port),
		"-c", "dtm.node_id=" + strconv.Itoa(nodeid),
		"-c", "dtm.host=127.0.0.2",
		"-c", "dtm.port=" + strconv.Itoa(5431),
		"-c", "autovacuum=off",
		"-c", "fsync=off",
		"-c", "synchronous_commit=off",
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

func main() {
	srcroot := "../../.."
	prefix := get_prefix(srcroot)

	bin := map[string]string{
		"dtmd": srcroot + "/contrib/pg_dtm/dtmd/bin/dtmd",
		"initdb": prefix + "/bin/initdb",
		"postgres": prefix + "/bin/postgres",
	}

	datadirs := []string{"/tmp/data1", "/tmp/data2", "/tmp/data3"}

	check_bin(&bin);

	for _, datadir := range datadirs {
		initdb(bin["initdb"], datadir)
	}

	var wg sync.WaitGroup

	wg.Add(1)
	go dtmd(bin["dtmd"], &wg)

	for i, datadir := range datadirs {
		wg.Add(1)
		go postgres(bin["postgres"], datadir, 5432 + i, i, &wg)
	}

	wg.Wait()

	log.Println("done.")
}

