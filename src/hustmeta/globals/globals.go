package globals

import (
	"fmt"
	"hustmeta/httpman"
	"hustmeta/utils"
	"net/http"
	"os"
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

func loadGlobalConf(path string) error {
	globalConf = new(utils.GlobalConf)
	return utils.LoadConfigure(path, globalConf)
}

func GetGlobalConf() *utils.GlobalConf {
	return globalConf
}

func reloadGlobalConf(path string) error {
	cf := &utils.GlobalConf{}
	if err := utils.LoadConfigure(path, cf); nil != err {
		return err
	}
	globalConf.Reload(cf)
	return nil
}

func globalConfJson() string {
	return utils.MarshalJson(globalConf)
}

// Performance
var collector *utils.Collector

func initPerformance() {
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

func getPerformanceData() string {
	if !globalConf.Debugger.WatchPerformance || nil == collector {
		return ""
	}
	return collector.GetData()
}

func dumpPerformanceData(path string) {
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
		dumpPerformanceData(outfile)
	}
}

func startCron(ctx *CronCtx) {
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
		return reloadGlobalConf(path)
	}
	return fmt.Errorf("unknown file: %v", filename)
}

func getConfigJson(path string) string {
	filename := filepath.Base(path)
	if GlobalFile == filename {
		return globalConfJson()
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

func startServer(info *utils.HttpServerInfo, flags *utils.DebugFlags) {
	serverInstance = echo.New()
	// service.Register(serverInstance)
	serverInstance.GET("/status.html", func(c echo.Context) error {
		return c.String(http.StatusOK, "ok\n")
	})
	serverInstance.GET("/performance", func(c echo.Context) error {
		return c.String(http.StatusOK, getPerformanceData())
	})
	serverInstance.Use(middleware.BodyDump(bodyDumpHandler(flags)))
	serverInstance.Logger.Fatal(serverInstance.Start(fmt.Sprintf("%v:%v", info.IP, info.Port)))
}

func stopServer() {
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

// StartService
func StartService(conf, datadir string) {
	if err := loadGlobalConf(filepath.Join(conf, GlobalFile)); nil != err {
		seelog.Critical(err.Error())
		os.Exit(1)
	}

	utils.EnableErrorTrace(globalConf.Debugger.EnableErrorTrace)
	initPerformance()

	httpman.Init(globalConf.Http)

	go startCron(&CronCtx{
		DataDir: datadir,
		Conf:    &globalConf.Crontabs})
	go utils.NewWatcher(reloadConfig, []string{
		filepath.Join(conf, GlobalFile)})

	startServer(&globalConf.Server, &globalConf.Debugger)
}
