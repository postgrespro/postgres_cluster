package main

import (
	"archive/tar"
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/samalba/dockerclient"
	"io"
	"log"
	"os"
	"path"
	"path/filepath"
	"strings"
//	"time"
)

var cfg struct {
	Docker *dockerclient.DockerClient
	Paths struct {
		Postgres string
		Dtmd     string
		DtmBench string
	}
	NodesAs struct {
		Postgres int
		Dtmd     int
		DtmBench int
	}
	Actions []string
}

var labels = map[string]string{
	"vendor"  : "postgrespro",
	"version" : "xtm",
	"test"    : "contain",
}

func exists(path string) bool {
	if _, err := os.Stat(path); err != nil {
		if os.IsNotExist(err) {
			return false
		}
	}
	return true
}

func should_exist(path string) {
	if !exists(path) {
		log.Fatalf("'%s' not found", path)
	}
}

func get_labels(container_id string) map[string]string {
	info, err := cfg.Docker.InspectContainer(container_id)
	if err != nil {
		log.Fatal(err)
	}
	return info.Config.Labels
}

func get_containers(morelabels ...string) []dockerclient.Container {
	labellist := make([]string, len(labels) + len(morelabels))
	i := 0
	for k, v := range labels {
		labellist[i] = fmt.Sprintf("%s=%s", k, v)
		i += 1
	}
	for _, l := range morelabels {
		labellist[i] = l
		i += 1
	}
	filters := map[string][]string{"label": labellist}
	filterstr, err := json.Marshal(filters)
	if err != nil {
		log.Fatal(err)
	}

	containers, err := cfg.Docker.ListContainers(true, true, string(filterstr))
	if err != nil {
		log.Fatal(err)
	}

	return containers
}

func get_ip(container_id string) (string, bool) {
	info, err := cfg.Docker.InspectContainer(container_id)
	if err != nil {
		log.Fatal(err)
	}

	net, err := cfg.Docker.InspectNetwork("contain")
	if err != nil {
		log.Fatal(err)
	}

	return net.Containers[container_id].IPv4Address, info.State.Running
}

func status() {
	log.Println("--- status")

	for _, c := range get_containers() {
		name := c.Names[0]
		ip, running := get_ip(c.Id)
		if running {
			fmt.Printf("%s (%s)\n", name, ip)
		} else {
			fmt.Printf("%s down\n", name)
		}
	}
}

func get_prefix(srcroot string) string {
	makefilepath := path.Join(srcroot, "src", "Makefile.global")
	should_exist(makefilepath)

	makefile, err := os.Open(makefilepath)
	if err != nil {
		log.Fatal("could not open the makefile")
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

func dump_cfg() {
	fmt.Printf("Postgres: %s\n", cfg.Paths.Postgres)
	fmt.Printf("    Dtmd: %s\n", cfg.Paths.Dtmd)
	fmt.Printf("DtmBench: %s\n", cfg.Paths.DtmBench)
	fmt.Printf(" Actions: %s\n", strings.Join(cfg.Actions, " "))
}

func usage() {
	fmt.Printf("Usage: %s [OPTIONS] ACTION [ ACTION ... ]\n", os.Args[0])
	flag.PrintDefaults()
	fmt.Printf("ACTION can be one of:\n\tstatus\n\tbuild\n\tcreate\n\tstart\n\tkill\n\tinit\n\tclean\n")
}

func entar(dst *io.PipeWriter, paths map[string]string) {
	defer dst.Close()
	tw := tar.NewWriter(dst)
	defer tw.Close()

	for from, to := range paths {
		walkFn := func(curpath string, info os.FileInfo, err error) error {
			if info.Mode().IsDir() {
				return nil
			}

			name := path.Join(to, curpath[len(from):])
			if len(name) == 0 {
				return nil
			}

			//log.Printf("tarring %s as %s\n", curpath, name)

			if h, err := tar.FileInfoHeader(info, name); err != nil {
				log.Fatal(err)
			} else {
				h.Name = name
				if err = tw.WriteHeader(h); err != nil {
					log.Fatalln(err)
				}
			}

			target, err := os.Readlink(curpath)
			if err == nil {
				tw.Write([]byte(target))
			} else {
				fr, err := os.Open(curpath)
				if err != nil {
					return err
				}
				defer fr.Close()

				if _, err := io.Copy(tw, fr); err != nil {
					log.Fatal(err)
//				} else {
//					fmt.Println(length)
				}
			}
			return nil
		}

		if err := filepath.Walk(from, walkFn); err != nil {
			log.Fatal(err)
		}
	}
}

func open_as_tar(paths map[string]string) *io.PipeReader {
	reader, writer := io.Pipe()
	go entar(writer, paths)
	return reader
}

func build_image() {
	log.Println("--- build")

	reader := open_as_tar(map[string]string{
		"Dockerfile": "Dockerfile",
		cfg.Paths.Postgres: "postgres",
		cfg.Paths.Dtmd: "dtmd",
		cfg.Paths.DtmBench: "dtmbench",
	})
	defer reader.Close()

	config := &dockerclient.BuildImage{
		Context        : reader,
		RepoName       : "postgrespro",
		SuppressOutput : false,
		ForceRemove    : true,
		Remove         : true,
	}

	buildreader, err := cfg.Docker.BuildImage(config)
	if err != nil {
		log.Fatal(err)
	}

	scanner := bufio.NewScanner(buildreader)
	for scanner.Scan() {
		fmt.Println(scanner.Text())
	}

	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
}

func create_bulk_containers(config dockerclient.ContainerConfig, role string, num int) {
	for i := 0; i < num; i++ {
		config.Labels["role"] = role
		name := fmt.Sprintf("%s%d", role, i)
		config.Hostname = name
		id, err := cfg.Docker.CreateContainer(&config, name, nil)
		if err != nil {
			log.Fatal(err)
		}

		log.Printf("created container %s (%s...)\n", name, id[:8])
	}
}

func create_network(name string) {
	netconfig := dockerclient.NetworkCreate{
		Name           : name,
		CheckDuplicate : true,
		Driver         : "bridge",
	}
	response, err := cfg.Docker.CreateNetwork(&netconfig)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("created network %s (%s...)\n", name, response.ID[:8])
}

func remove_network(name string) {
	log.Printf("removing network %s\n", name)
	err := cfg.Docker.RemoveNetwork(name)
	if err != nil {
		log.Println(err)
	}
}

func create_containers() {
	log.Println("--- create")

	netname := "contain"
	create_network(netname)

	clabels := make(map[string]string)
	for k, v := range labels {
		clabels[k] = v
	}

	config := dockerclient.ContainerConfig{
		Image  : "postgrespro:latest",
		Tty    : true,
		Labels : clabels,
	}
	config.HostConfig.Privileged = true
	config.HostConfig.NetworkMode = netname

	create_bulk_containers(config, "postgres", cfg.NodesAs.Postgres)
	create_bulk_containers(config,     "dtmd", cfg.NodesAs.Dtmd)
	create_bulk_containers(config, "dtmbench", cfg.NodesAs.DtmBench)
}

func start_containers() {
	log.Println("--- up")

	for _, c := range get_containers() {
		name := c.Names[0]
		ip, running := get_ip(c.Id)
		if running {
			fmt.Printf("%s (%s)\n", name, ip)
		} else {
			log.Printf("starting %s\n", name)
			err := cfg.Docker.StartContainer(c.Id, nil)
			if err != nil {
				log.Fatal(err)
			}
		}
	}
}

func stop_containers() {
	log.Println("--- down")

	for _, c := range get_containers() {
		name := c.Names[0]
		ip, running := get_ip(c.Id)
		if running {
			log.Printf("stopping %s (%s)\n", name, ip)
			err := cfg.Docker.StopContainer(c.Id, 5)
			if err != nil {
				log.Println(err)
			}
		} else {
			fmt.Printf("%s down\n", name)
		}
	}
}

func run_in_container(container_id string, argv ...string) {
	log.Printf("run in %s: %v", container_id[:8], argv)

	config := dockerclient.ExecConfig{
		AttachStdin  : true,
		AttachStdout : true,
		AttachStderr : true,
		Tty          : true,
		Cmd          : argv,
		Container    : container_id,
		Detach       : false,
	}

	execid, err := cfg.Docker.ExecCreate(&config)
	if err != nil {
		log.Fatal(err)
	}

	err = cfg.Docker.ExecStart(execid, &config)
	if err != nil {
		log.Fatal(err)
	}
}

func initdb(container_id string) {
	run_in_container(container_id, "rm", "-rf", "/datadir")
	run_in_container(container_id, "postgres/bin/initdb", "/datadir")
}

func init_data() {
	log.Println("--- init")

	for _, c := range get_containers("role=postgres") {
		initdb(c.Id)
	}
}

func clean_all() {
	log.Println("--- clean")

	for _, c := range get_containers() {
		name := c.Names[0]
		log.Printf("removing %s\n", name)
		err := cfg.Docker.RemoveContainer(c.Id, true, true)
		if err != nil {
			log.Println(err)
		}
	}

	remove_network("contain")
}

func init() {
	srcroot := path.Join("..", "..", "..")

	flag.StringVar(
		&cfg.Paths.Postgres,
		"postgres", get_prefix(srcroot),
		"Postgres installation prefix",
	)
	flag.StringVar(
		&cfg.Paths.Dtmd,
		"dtmd", path.Join(srcroot, "contrib", "pg_dtm", "dtmd", "bin", "dtmd"),
		"Path to dtmd binary",
	)
	flag.StringVar(
		&cfg.Paths.DtmBench,
		"dtmbench", path.Join(srcroot, "contrib", "pg_dtm", "tests", "dtmbench"),
		"Path to dtmbench binary",
	)
	flag.IntVar(&cfg.NodesAs.Postgres, "postgreses", 3, "Number of Postgres instances")
	flag.IntVar(&cfg.NodesAs.Dtmd,          "dtmds", 3, "Number of dtmd instances")
	flag.IntVar(&cfg.NodesAs.DtmBench, "dtmbenches", 1, "Number of dtmbench instances")

	flag.Usage = usage
	flag.Parse()

	if len(flag.Args()) > 0 {
		cfg.Actions = flag.Args()
	} else {
		cfg.Actions = []string{"status"}
	}

	cfg.Docker, _ = dockerclient.NewDockerClient("unix:///var/run/docker.sock", nil)
	dump_cfg()

	should_exist(path.Join(cfg.Paths.Postgres, "bin", "postgres"))
	should_exist(path.Join(cfg.Paths.Postgres, "bin", "initdb"))
	should_exist(path.Join(cfg.Paths.Postgres, "lib", "pg_dtm.so"))
	should_exist(cfg.Paths.Dtmd)
	should_exist(cfg.Paths.DtmBench)
}

func main() {
	for _, action := range cfg.Actions {
		switch action {
			default:
				fmt.Printf("unknown action '%s'\n", action)
			case "status": status()
			case  "build": build_image()
			case "create": create_containers()
			case     "up": start_containers()
			case   "init": init_data()
			case   "down": stop_containers()
			case  "clean": clean_all()
		}
	}
}
