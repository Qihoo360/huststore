package main

import (
	"flag"
	"fmt"

	"./hustdb/peers"
	"./internal/httpman"
	"./internal/utils"

	"./hustdb/binlog"
	"./hustdb/comm"
	hc "./hustdb/healthcheck"

	"os"
	"os/exec"
	"path/filepath"

	"github.com/cihub/seelog"

	server "./server"
)

func main() {
	file, _ := exec.LookPath(os.Args[0])
	path, _ := filepath.Abs(file)
	root := filepath.Dir(path)

	var conf string
	flag.StringVar(&conf, "conf", "", "")
	flag.Parse()

	if "" == conf {
		conf = filepath.Join(root, "conf")
	} else {
		conf, _ = filepath.Abs(conf)
	}

	utils.SetGlobalConfPath(conf)

	logger, err := seelog.LoggerFromConfigAsFile(filepath.Join(conf, "log.xml"))

	if err != nil {
		seelog.Critical("err parsing config log file", err)
		return
	}

	seelog.ReplaceLogger(logger)
	defer seelog.Flush()

	cfpath := filepath.Join(conf, "server.json")

	if !utils.LoadGlobalConf(cfpath) {
		seelog.Critical("LoadGlobalConf error")
		return
	}

	gconf := utils.GetGlobalConf()

	fmt.Printf("global conf :%v\n", gconf)

	comm.HustdbInit(&gconf.Hustdb)
	hc.Init(gconf.HealthCheck.HealthCheckCycle)
	binlog.Init(gconf.Binlog)
	httpman.InitHttp(gconf.Http, gconf.HealthCheck.Timeout)
	bpath := filepath.Join(conf, "backends.json")
	if !peers.Init(bpath) {
		seelog.Error("Peers Init Failed")
		return
	}

	srv, err := server.NewServer(fmt.Sprintf(":%d", gconf.Server.Port), gconf.Concurrency)
	if err != nil {
		panic(err)
	}
	srv.Run()
}
