package main

import (
	"flag"
	"fmt"
	"hustmeta/utils"
	"os"
	"os/exec"
	"path/filepath"

	"github.com/cihub/seelog"
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

	root = filepath.Dir(conf)
	datadir := filepath.Join(root, "data")

	logger, err := seelog.LoggerFromConfigAsFile(filepath.Join(conf, "seelog.xml"))

	if err != nil {
		fmt.Println(err.Error())
		return
	}

	seelog.ReplaceLogger(logger)
	defer seelog.Flush()

	seelog.Debug("hustmeta start...")

	if !utils.Initialize(conf, datadir) {
		return
	}
}
