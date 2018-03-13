package utils

import (
	"time"

	"github.com/syndtr/goleveldb/leveldb"
)

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

var ldb *leveldb.DB

func SetDB(db *leveldb.DB) {
	ldb = db
}

func GetDB() *leveldb.DB { return ldb }
