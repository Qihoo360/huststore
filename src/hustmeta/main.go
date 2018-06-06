package main

import (
	"flag"
	"fmt"
	"hustmeta/globals"
	"os"
	"os/exec"
	"path/filepath"

	"hustmeta/service"

	"github.com/cihub/seelog"
	"github.com/labstack/echo"
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

	datadir := filepath.Join(root, "data")

	logger, err := seelog.LoggerFromConfigAsFile(filepath.Join(conf, "seelog.xml"))

	if err != nil {
		fmt.Println(err.Error())
		return
	}

	seelog.ReplaceLogger(logger)
	defer seelog.Flush()

	seelog.Debug("hustmeta start...")
	globals.StartService(func(e *echo.Echo) { service.Register(e) }, conf, datadir)
}
