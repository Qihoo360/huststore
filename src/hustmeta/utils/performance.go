package utils

import (
	"math"
	"sync"
	"time"
)

type MeterInfo struct {
	Hit  uint64  `json:"hit"`
	Cost float64 `json:"cost"`
}

func (this *MeterInfo) Update(cost float64) {
	this.Hit += 1
	this.Cost += cost
}

func newMeterInfo(cost float64) *MeterInfo {
	return &MeterInfo{Hit: 1, Cost: cost}
}

type PerformanceData struct {
	LastResetTime time.Time             `json:"last_reset_time"`
	ResetTime     time.Time             `json:"reset_time"`
	Apis          map[string]*MeterInfo `json:"apis"`
	TotalMeter    MeterInfo             `json:"total_meter"`
}

func (this *PerformanceData) canRefresh(cost float64) bool {
	if (math.MaxUint64 - this.TotalMeter.Hit) <= 1 {
		return false
	}
	if (math.MaxFloat64 - this.TotalMeter.Cost) < cost {
		return false
	}
	return true
}

func (this *PerformanceData) reset() {
	this.LastResetTime = this.ResetTime
	this.ResetTime = time.Now()

	this.Apis = map[string]*MeterInfo{}
	this.TotalMeter = MeterInfo{0, 0}
}

func (this *PerformanceData) Refresh(key string, cost float64) {
	if !this.canRefresh(cost) {
		this.reset()
	}

	this.TotalMeter.Update(cost)

	_, ok := this.Apis[key]
	if ok {
		this.Apis[key].Update(cost)
	} else {
		this.Apis[key] = newMeterInfo(cost)
	}
}

type Collector struct {
	sync.RWMutex
	data PerformanceData
}

func (this *Collector) Refresh(key string, cost float64) {
	this.Lock()
	this.refresh(key, cost)
	this.Unlock()
}

func (this *Collector) refresh(key string, cost float64) {
	this.data.Refresh(key, cost)
}

func (this *Collector) Dump(path string) {
	this.RLock()
	SaveConfigure(this.data, path)
	this.RUnlock()
}

func (this *Collector) GetData() string {
	this.RLock()
	data := MarshalJson(this.data)
	this.RUnlock()
	return data
}

func NewCollector() *Collector {
	return &Collector{data: PerformanceData{
		LastResetTime: time.Now(),
		ResetTime:     time.Now(),
		Apis:          map[string]*MeterInfo{},
		TotalMeter:    MeterInfo{0, 0}}}
}

type PerformanceCollector func(start time.Time)

var performanceCollector PerformanceCollector

func SetCollector(collector PerformanceCollector) {
	performanceCollector = performanceCollector
}

func Collect(start time.Time) {
	if nil == performanceCollector {
		return
	}
	performanceCollector(start)
}
