package defines

type ServerConf struct {
	Id   int
	Port int
}

type HttpConf struct {
	MaxIdleConnsPerHost   int
	ResponseHeaderTimeout int
	Timeout               int
	KeepAlive             int
}

type HustdbConf struct {
	User   string
	Passwd string
}

type BinlogConf struct {
	RoutineCnt  int
	TaskChanCap int
}

type HealthCheckConf struct {
	HealthCheckCycle int
	Timeout          int
}
