package utils

import (
	"hustmeta/httpman"
)

type DebugFlags struct {
	WatchPerformance bool
	EnableErrorTrace bool
	DumpRequest      bool
	DumpResponse     bool
	DumpConfig       bool
}

type HttpServerInfo struct {
	IP   string
	Port int
}

type WatchPerformanceCron struct {
	Cron            string
	DumpFileTimeFmt string
}

type CrontabConfig struct {
	WatchPerformance WatchPerformanceCron
}

type ServiceConfig struct {
	Https  bool
	Domain string
}

type GlobalConf struct {
	Debugger DebugFlags
	Server   HttpServerInfo
	Http     httpman.HttpConfig
	Crontabs CrontabConfig
	Timefmt  string
}

func (this *GlobalConf) Reload(other *GlobalConf) {
	this.Debugger = other.Debugger
	this.Http.Debug = other.Http.Debug
	this.Timefmt = other.Timefmt
}
