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
	"github.com/syndtr/goleveldb/leveldb"
)

const (
	GlobalFile = "hustmeta.json"
)

type Globals struct {
	sync.Mutex
	conf           *utils.GlobalConf
	collector      *utils.Collector
	serverInstance *echo.Echo
}

func (this *Globals) Initialize(conf, datadir string) error {
	path := filepath.Join(conf, GlobalFile)
	this.collector = utils.NewCollector()
	this.conf = &utils.GlobalConf{}
	if err := utils.LoadConfigure(path, this.conf); nil != err {
		return err
	}
	utils.SetGlobalConf(this.conf)
	utils.EnableErrorTrace(this.conf.Debugger.EnableErrorTrace)
	httpman.Init(this.conf.Http)
	utils.SetCollector(func(start time.Time) { this.collect(start) })

	go this.startCron(datadir, &this.conf.Crontabs)
	go utils.NewWatcher(func(path string) { this.reloadConfig(path) }, []string{path})

	return nil
}

// configure
func (this *Globals) reloadGlobalConf(path string) error {
	cf := &utils.GlobalConf{}
	if err := utils.LoadConfigure(path, cf); nil != err {
		return err
	}
	this.conf.Reload(cf)
	return nil
}

func (this *Globals) reload(path string) error {
	this.Lock()
	defer this.Unlock()

	filename := filepath.Base(path)

	if GlobalFile == filename {
		return this.reloadGlobalConf(path)
	}
	return fmt.Errorf("unknown file: %v", filename)
}

func (this *Globals) getConfigJson(path string) string {
	filename := filepath.Base(path)
	if GlobalFile == filename {
		return utils.MarshalJson(this.conf)
	}
	return ""
}

func (this *Globals) reloadConfig(path string) {
	if err := this.reload(path); nil != err {
		seelog.Error(err.Error())
	} else {
		if this.conf.Debugger.DumpConfig {
			seelog.Debugf("reload \"%v\" success. data: %v", path, this.getConfigJson(path))
		} else {
			seelog.Debugf("reload \"%v\" success.", path)
		}
	}
}

// performance
func (this *Globals) collect(start time.Time) {
	if !this.conf.Debugger.WatchPerformance || nil == this.collector {
		return
	}

	end := time.Now()
	delta := end.Sub(start)
	callstack := utils.GetCallStack(2, nil)
	this.collector.Refresh(callstack.Func, delta.Seconds())
}

func (this *Globals) getPerformanceData() string {
	if !this.conf.Debugger.WatchPerformance || nil == this.collector {
		return ""
	}
	return this.collector.GetData()
}

func (this *Globals) dumpPerformanceData(datadir string) {
	if !this.conf.Debugger.WatchPerformance || nil == this.collector {
		return
	}
	now := time.Now()
	outfile := filepath.Join(datadir, now.Format(this.conf.Crontabs.WatchPerformance.DumpFileTimeFmt))
	this.collector.Dump(outfile)
}

// crontabs
func (this *Globals) startCron(datadir string, conf *utils.CrontabConfig) {
	c := cron.New()
	if len(conf.WatchPerformance.Cron) > 0 {
		c.AddFunc(conf.WatchPerformance.Cron, func() { this.dumpPerformanceData(datadir) })
	}
	c.Start()
	select {}
}

// http server
func (this *Globals) StartServer(register RegisterService) {
	info := &this.conf.Server
	flags := &this.conf.Debugger

	this.serverInstance = echo.New()
	register(this.serverInstance)
	this.serverInstance.GET("/status.html", func(c echo.Context) error {
		return c.String(http.StatusOK, "ok\n")
	})
	this.serverInstance.GET("/performance", func(c echo.Context) error {
		return c.String(http.StatusOK, this.getPerformanceData())
	})
	this.serverInstance.Use(middleware.BodyDump(bodyDumpHandler(flags)))
	this.serverInstance.Logger.Fatal(this.serverInstance.Start(fmt.Sprintf("%v:%v", info.IP, info.Port)))
}

func (this *Globals) StopServer() {
	if nil == this.serverInstance {
		return
	}
	err := this.serverInstance.Close()
	if nil != err {
		seelog.Error(err.Error())
	} else {
		seelog.Debug("server stopped")
	}
}

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

type RegisterService func(e *echo.Echo)

// StartService
func StartService(register RegisterService, conf, datadir string) {
	db, err := leveldb.OpenFile(filepath.Join(datadir, "hustmeta"), nil)
	if nil != err {
		seelog.Critical(err.Error())
		os.Exit(1)
	}
	defer db.Close()

	utils.SetDB(db)

	g := &Globals{}
	if err := g.Initialize(conf, datadir); nil != err {
		seelog.Critical(err.Error())
		os.Exit(1)
	}
	g.StartServer(register)
}
