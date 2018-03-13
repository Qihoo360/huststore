package globals

import (
	"fmt"
	"hustmeta/httpman"
	"hustmeta/utils"
	"net/http"
	"path/filepath"
	"sync"
	"time"

	"github.com/cihub/seelog"
	"github.com/labstack/echo"
	"github.com/labstack/echo/middleware"
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

// http server
var serverInstance *echo.Echo

func bodyDumpHandler(flags *utils.DebugFlags) middleware.BodyDumpHandler {
	return func(c echo.Context, reqBody, resBody []byte) {
		if flags.DumpRequest {
			if len(reqBody) > 0 {
				seelog.Debug(utils.BytesToString(reqBody))
			}
		}
		if flags.DumpResponse {
			if len(resBody) > 0 {
				seelog.Debug(utils.BytesToString(resBody))
			}
		}
	}
}

func StartServer(info *utils.HttpServerInfo, flags *utils.DebugFlags) {
	serverInstance = echo.New()
	// service.Register(serverInstance)
	serverInstance.GET("/status.html", func(c echo.Context) error {
		return c.String(http.StatusOK, "ok\n")
	})
	serverInstance.GET("/performance", func(c echo.Context) error {
		return c.String(http.StatusOK, GetPerformanceData())
	})
	serverInstance.Use(middleware.BodyDump(bodyDumpHandler(flags)))
	serverInstance.Logger.Fatal(serverInstance.Start(fmt.Sprintf("%v:%v", info.IP, info.Port)))
}

func StopServer() {
	if nil == serverInstance {
		return
	}
	err := serverInstance.Close()
	if nil != err {
		seelog.Error(err.Error())
	} else {
		seelog.Debug("server stopped")
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
