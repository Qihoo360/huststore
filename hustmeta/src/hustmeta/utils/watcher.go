package utils

import (
	"fmt"
	"os"
	"path/filepath"
	"sync"

	"github.com/cihub/seelog"
	"github.com/fsnotify/fsnotify"
)

var mutex sync.Mutex

func reload(path string) error {
	mutex.Lock()
	defer mutex.Unlock()

	filename := filepath.Base(path)

	if GlobalFile == filename {
		return ReloadGlobalConf(path)
	}
	return fmt.Errorf("unknown file: %v", filename)
}

func getConfigJson(path string) string {
	filename := filepath.Base(path)
	if GlobalFile == filename {
		return GlobalConfJson()
	}
	return ""
}

func reloadConfig(path string) {
	if err := reload(path); nil != err {
		seelog.Error(err.Error())
	} else {
		if GetGlobalConf().Debugger.DumpConfig {
			seelog.Debugf("reload \"%v\" success. data: %v", path, getConfigJson(path))
		} else {
			seelog.Debugf("reload \"%v\" success.", path)
		}
	}
}

func NewWatcher(paths []string) {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		seelog.Critical(err)
		os.Exit(1)
	}
	defer watcher.Close()

	done := make(chan bool)
	go func() {
		for {
			select {
			case event := <-watcher.Events:
				if event.Op&fsnotify.Write == fsnotify.Write {
					reloadConfig(event.Name)
				}
			case err := <-watcher.Errors:
				seelog.Error(err)
			}
		}
	}()
	for _, path := range paths {
		err = watcher.Add(path)
		if err != nil {
			seelog.Critical(err)
			os.Exit(1)
		}
	}
	<-done
}
