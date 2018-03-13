package utils

import (
	"hustmeta/httpman"
	"path/filepath"
	"time"

	"github.com/cihub/seelog"
	"github.com/robfig/cron"
)

const (
	GlobalFile = "hustmeta.json"
)

// Configuration
var globalConf *GlobalConf

func LoadGlobalConf(path string) error {
	globalConf = new(GlobalConf)
	return LoadConfigure(path, globalConf)
}

func GetGlobalConf() *GlobalConf {
	return globalConf
}

func ReloadGlobalConf(path string) error {
	cf := &GlobalConf{}
	if err := LoadConfigure(path, cf); nil != err {
		return err
	}
	globalConf.Reload(cf)
	return nil
}

func GlobalConfJson() string {
	return MarshalJson(globalConf)
}

// Performance
var collector *Collector

func InitPerformance() {
	collector = NewCollector()
}

func Collect(start time.Time) {

	if !globalConf.Debugger.WatchPerformance || nil == collector {
		return
	}

	end := time.Now()
	delta := end.Sub(start)
	callstack := GetCallStack(2, nil)
	collector.Refresh(callstack.Func, delta.Seconds())
}

func GetPerformanceData() string {
	if !globalConf.Debugger.WatchPerformance || nil == collector {
		return ""
	}
	return collector.GetData()
}

func DumpPerformanceData(path string) {
	if !globalConf.Debugger.WatchPerformance || nil == collector {
		return
	}
	collector.Dump(path)
}

type CronCtx struct {
	DataDir string
	Conf    *CrontabConfig
}

func getWatchPerformanceHandler(datadir string) func() {
	return func() {
		now := time.Now()
		outfile := filepath.Join(datadir, now.Format(globalConf.Crontabs.WatchPerformance.DumpFileTimeFmt))
		DumpPerformanceData(outfile)
	}
}

func StartCron(ctx *CronCtx) {
	c := cron.New()
	if len(ctx.Conf.WatchPerformance.Cron) > 0 {
		handler := getWatchPerformanceHandler(ctx.DataDir)
		err := c.AddFunc(ctx.Conf.WatchPerformance.Cron, handler)
		if nil != err {
			c.AddFunc("0 0 * * * *", handler)
		}
	}
	c.Start()
	select {}
}

// Initialize
func Initialize(conf, datadir string) bool {
	if err := LoadGlobalConf(filepath.Join(conf, GlobalFile)); nil != err {
		seelog.Critical(err.Error())
		return false
	}

	EnableErrorTrace(globalConf.Debugger.EnableErrorTrace)
	InitPerformance()

	httpman.Init(globalConf.Http)

	go StartCron(&CronCtx{
		DataDir: datadir,
		Conf:    &globalConf.Crontabs})
	go NewWatcher([]string{
		filepath.Join(conf, GlobalFile)})

	return true
}
