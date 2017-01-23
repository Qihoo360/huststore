package binlog

import (
	"../../internal/utils"
	"../comm"

	"github.com/cihub/seelog"
)

var (
	BinlogMethodCodeMap = map[string]string{
		"put":  "1",
		"del":  "2",
		"hset": "3",
		"hdel": "4",
		"sadd": "5",
		"srem": "6",
		"zadd": "7",
		"zrem": "8",
	}
)

func Do(succBackend, failBackend, cmd string, args map[string][]byte, val []byte) {
	switch cmd {
	case "put":
		args["method"] = []byte(BinlogMethodCodeMap["put"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	case "del":
		args["method"] = []byte(BinlogMethodCodeMap["del"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	case "hset":
		args["method"] = []byte(BinlogMethodCodeMap["hset"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	case "hdel":
		args["method"] = []byte(BinlogMethodCodeMap["hdel"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	case "sadd":
		args["method"] = []byte(BinlogMethodCodeMap["sadd"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	case "srem":
		args["method"] = []byte(BinlogMethodCodeMap["srem"])
		args["host"] = []byte(failBackend)
		HandleHustdbWriteFailedTask(succBackend, args, val)
	default:
		seelog.Warnf("Unknow Binlog Type : %v\n", cmd)
	}
}

func HandleHustdbWriteFailedTask(succBackend string, args map[string][]byte, val []byte) {
	var code int
	retCh := make(chan interface{}, 1)

	DeliverBinlogTask(utils.NgxHashKey(succBackend)%BinlogRoutineCnt, func() interface{} {
		code = comm.HustdbBinlog(succBackend, args, val)
		return true
	}, retCh)
	<-retCh
}
