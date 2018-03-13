package utils

import (
	"os"

	"github.com/cihub/seelog"
	"github.com/fsnotify/fsnotify"
)

type Reloader func(path string)

func NewWatcher(reload Reloader, paths []string) {
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
					reload(event.Name)
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
