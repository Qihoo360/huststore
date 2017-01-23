package binlog

import (
	def "../../internal/defines"
)

var (
	BinlogRoutineCnt  int
	BinlogTaskChanCap int
)

type BinlogTask struct {
	Req TaskFunc
	Ack chan interface{}
}

type TaskFunc func() interface{}

func Init(conf def.BinlogConf) {
	BinlogRoutineCnt = conf.RoutineCnt
	BinlogTaskChanCap = conf.TaskChanCap
	globalBinlogTaskChan = make(map[int]chan *BinlogTask)
	for ix := 0; ix < BinlogRoutineCnt; ix++ {
		globalBinlogTaskChan[ix] = make(chan *BinlogTask, BinlogTaskChanCap)
	}
	go RunBinlog()
}

var globalBinlogTaskChan map[int]chan *BinlogTask

func RunBinlog() {
	for idx, _ := range globalBinlogTaskChan {
		go func(idx int) {
			for {
				select {
				case task := <-globalBinlogTaskChan[idx]:
					if task.Ack != nil {
						task.Ack <- task.Req()
					} else {
						task.Req()
					}
				}
			}
		}(idx)
	}
}

func DeliverBinlogTask(idx int, taskFunc TaskFunc, ch chan interface{}) {
	taskCh, exists := globalBinlogTaskChan[idx]
	if exists {
		task := &BinlogTask{Req: taskFunc, Ack: ch}
		taskCh <- task
	}
}
