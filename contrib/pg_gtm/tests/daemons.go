package main

import (
	"log"
	"os/exec"
	"io"
	"bufio"
	"sync"
	"os"
	"strconv"
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

func postgres(bin string, datadir string, port int, wg *sync.WaitGroup) {
	argv := []string{bin, "-D", datadir, "-p", strconv.Itoa(port)}
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

func main() {
	bin := map[string]string{
		"dtmd": "/home/kvap/postgrespro/contrib/pg_dtm/dtmd/bin/dtmd",
		"initdb": "/home/kvap/postgrespro-build/bin/initdb",
		"postgres": "/home/kvap/postgrespro-build/bin/postgres",
	}

	datadirs := []string{"/tmp/data1", "/tmp/data2"}

	check_bin(&bin);

	for _, datadir := range datadirs {
		initdb(bin["initdb"], datadir)
	}

	var wg sync.WaitGroup

	wg.Add(1)
	go dtmd(bin["dtmd"], &wg)

	for i, datadir := range datadirs {
		wg.Add(1)
		go postgres(bin["postgres"], datadir, 5432 + i, &wg)
	}

	wg.Wait()

	log.Println("done.")
}

