package globals

import (
	"fmt"
	"hustmeta/httpman"
	"hustmeta/utils"
	"path/filepath"
	"sync"
	"time"

	"github.com/cihub/seelog"
	"github.com/robfig/cron"
)

const (
	GlobalFile = "hustmeta.json"
)

// Configuration
var globalConf *utils.GlobalConf

func LoadGlobalConf(path string) error {
	globalConf = new(utils.GlobalConf)
	return utils.LoadConfigure(path, globalConf)
}

func GetGlobalConf() *utils.GlobalConf {
	return globalConf
}

func ReloadGlobalConf(path string) error {
	cf := &utils.GlobalConf{}
	if err := utils.LoadConfigure(path, cf); nil != err {
		return err
	}
	globalConf.Reload(cf)
	return nil
}

func GlobalConfJson() string {
	return utils.MarshalJson(globalConf)
}

// Performance
var collector *utils.Collector

func InitPerformance() {
	collector = utils.NewCollector()
}

func Collect(start time.Time) {

	if !globalConf.Debugger.WatchPerformance || nil == collector {
		return
	}

	end := time.Now()
	delta := end.Sub(start)
	callstack := utils.GetCallStack(2, nil)
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

// Crontabs
type CronCtx struct {
	DataDir string
	Conf    *utils.CrontabConfig
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

// reload configuration
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

// Initialize
func Initialize(conf, datadir string) bool {
	if err := LoadGlobalConf(filepath.Join(conf, GlobalFile)); nil != err {
		seelog.Critical(err.Error())
		return false
	}

	utils.EnableErrorTrace(globalConf.Debugger.EnableErrorTrace)
	InitPerformance()

	httpman.Init(globalConf.Http)

	go StartCron(&CronCtx{
		DataDir: datadir,
		Conf:    &globalConf.Crontabs})
	go utils.NewWatcher(reloadConfig, []string{
		filepath.Join(conf, GlobalFile)})

	return true
}
