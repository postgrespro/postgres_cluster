package main

import (
	"archive/tar"
	"bufio"
	"flag"
	"fmt"
	"github.com/fsouza/go-dockerclient"
	"io"
	"log"
	"os"
	"path"
	"path/filepath"
	"strings"
)

var cfg struct {
	Docker *docker.Client
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

func get_containers(morelabels ...string) []docker.APIContainers {
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

	opts := docker.ListContainersOptions{
		All: true,
		Size: false,
		Filters: filters,
	}
	containers, err := cfg.Docker.ListContainers(opts)
	if err != nil {
		log.Fatal(err)
	}

	return containers
}

func get_ip(container_id string) (string, bool) {
	cont, err := cfg.Docker.InspectContainer(container_id)
	if err != nil {
		log.Fatal(err)
	}

	ip := cont.NetworkSettings.Networks["contain"].IPAddress

	return ip, cont.State.Running
}

func status() {
	log.Println("--- status")

	for _, c := range get_containers() {
		name := c.Names[0]
		ip, running := get_ip(c.ID)
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

	tarstream := open_as_tar(map[string]string{
		"Dockerfile": "Dockerfile",
		cfg.Paths.Postgres: "postgres",
		cfg.Paths.Dtmd: "dtmd",
		cfg.Paths.DtmBench: "dtmbench",
	})
	defer tarstream.Close()

	buildreader, buildstream := io.Pipe()
	config := docker.BuildImageOptions{
		InputStream         : tarstream,
		OutputStream        : buildstream,
		Name                : "postgrespro",
		SuppressOutput      : false,
		ForceRmTmpContainer : true,
		RmTmpContainer      : true,
	}

	err := cfg.Docker.BuildImage(config)
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

func create_bulk_containers(image string, netname string, role string, num int) {
	clabels := make(map[string]string)
	for k, v := range labels {
		clabels[k] = v
	}
	clabels["role"] = role

	for i := 0; i < num; i++ {
		name := fmt.Sprintf("%s%d", role, i)
		opts := docker.CreateContainerOptions{
			Name: name,
			Config: &docker.Config{
				Hostname : name,
				Labels   : clabels,
				Tty      : true,
				Image    : image,
			},
			HostConfig: &docker.HostConfig{
				Privileged  : true,
				NetworkMode : netname,
			},
		}
		c, err := cfg.Docker.CreateContainer(opts)
		if err != nil {
			log.Fatal(err)
		}

		log.Printf("created container %s (%s...)\n", name, c.ID[:8])
	}
}

func create_network(name string) {
	opts := docker.CreateNetworkOptions{
		Name           : name,
		CheckDuplicate : true,
		Driver         : "bridge",
	}
	net, err := cfg.Docker.CreateNetwork(opts)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("created network %s (%s...)\n", name, net.ID[:8])
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

	image := "postgrespro:latest"
	netname := "contain"
	create_network(netname)

	create_bulk_containers(image, netname, "postgres", cfg.NodesAs.Postgres)
	create_bulk_containers(image, netname,     "dtmd", cfg.NodesAs.Dtmd)
	create_bulk_containers(image, netname, "dtmbench", cfg.NodesAs.DtmBench)
}

func start_containers() {
	log.Println("--- up")

	for _, c := range get_containers() {
		name := c.Names[0]
		ip, running := get_ip(c.ID)
		if running {
			fmt.Printf("%s (%s)\n", name, ip)
		} else {
			log.Printf("starting %s\n", name)
			err := cfg.Docker.StartContainer(c.ID, nil)
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
		ip, running := get_ip(c.ID)
		if running {
			log.Printf("stopping %s (%s)\n", name, ip)
			err := cfg.Docker.StopContainer(c.ID, 5)
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

	createopts := docker.CreateExecOptions{
		AttachStdin  : false,
		AttachStdout : true,
		AttachStderr : true,
		Tty          : true,
		Cmd          : argv,
		Container    : container_id,
	}

	exec, err := cfg.Docker.CreateExec(createopts)
	if err != nil {
		log.Fatal(err)
	}

	reader, writer := io.Pipe()

	startopts := docker.StartExecOptions{
		Detach       : false,
		Tty          : true,
		OutputStream : writer,
		ErrorStream  : writer,
	}

	err = cfg.Docker.StartExec(exec.ID, startopts)
	if err != nil {
		log.Fatal(err)
	}

	scanner := bufio.NewScanner(reader)
	for scanner.Scan() {
		fmt.Println(scanner.Text())
	}

	if err := scanner.Err(); err != nil {
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
		initdb(c.ID)
	}
}

func clean_all() {
	log.Println("--- clean")

	for _, c := range get_containers() {
		name := c.Names[0]
		log.Printf("removing %s\n", name)

		opts := docker.RemoveContainerOptions{
			ID            : c.ID,
			RemoveVolumes : true,
			Force         : true,
		}

		err := cfg.Docker.RemoveContainer(opts)
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

	cfg.Docker, _ = docker.NewClientFromEnv()
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
