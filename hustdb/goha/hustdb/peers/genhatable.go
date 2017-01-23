package peers

import (
	"sync"

	"../../internal/utils"
	"../comm"

	"github.com/cihub/seelog"
)

type HaTableStruct struct {
	HashTable []*PeerInfo
	Rwlock    sync.RWMutex
}

type PeerInfo struct {
	Region   []int        `json:"region,omitempty"`
	Backends *BackendInfo `json:"backends,omitempty"`
}

type BackendDetail struct {
	Host  string `json:"host,omitempty"`
	Alive bool   `json:alive,omitempty`
}

type BackendInfo struct {
	Master BackendDetail `json:"master,omitempty"`
	Slave  BackendDetail `json:"slave,omitempty"`
}

type HustdbTable struct {
	Table []*HustdbItem `json:"table,omitempty"`
}

type HustdbItem struct {
	Item struct {
		Key []int    `json:"key,omitempty"`
		Val []string `json:"val,omitempty"`
	} `json:"item,omitempty"`
}

var HaTable *HaTableStruct

type GlobalHashTable *[]BackendInfo

var globalhashtable GlobalHashTable

func Init(path string) bool {
	if !LoadHustdbTable(path) {
		return false
	}
	if !GenHashTable() {
		return false
	}
	return GenGlobleHashtable()
}

var hustdbTable *HustdbTable

func LoadHustdbTable(path string) bool {
	hustdbTable = new(HustdbTable)
	return utils.LoadConf(path, hustdbTable)
}

func GenHashTable() bool {
	HaTable = new(HaTableStruct)
	HaTable.Rwlock = sync.RWMutex{}

	HaTable.Rwlock.Lock()
	defer HaTable.Rwlock.Unlock()
	HaTable.HashTable = make([]*PeerInfo, 0, len(hustdbTable.Table))
	for _, item := range hustdbTable.Table {
		if peer, ok := HustdbItem2PeerInfo(item); ok {
			HaTable.HashTable = append(HaTable.HashTable, peer)
		} else {
			return false
		}
	}

	return true
}

func HustdbItem2PeerInfo(item *HustdbItem) (*PeerInfo, bool) {
	if len(item.Item.Key) != 2 || len(item.Item.Val) != 2 {
		return nil, false
	}

	peer := new(PeerInfo)
	peer.Region = item.Item.Key

	peer.Backends = &BackendInfo{
		Master: BackendDetail{
			Host:  item.Item.Val[0],
			Alive: true,
		},
		Slave: BackendDetail{
			Host:  item.Item.Val[1],
			Alive: true,
		},
	}
	return peer, true
}

/*
func RefreshHashTable(path string) bool {
	HaTable.Rwlock.Lock()
	defer HaTable.Rwlock.Unlock()
	rc := utils.LoadConf(path, &HaTable.HashTable)
	return rc
}
*/

func GenGlobleHashtable() bool {
	ghTable := make([]BackendInfo, comm.HustdbTableSize)
	HaTable.Rwlock.RLock()
	defer HaTable.Rwlock.RUnlock()
	for _, peer := range HaTable.HashTable {
		if len(peer.Region) != 2 {
			seelog.Critical("Globalhashtable Format Error")
			return false
		}
		for ix := peer.Region[0]; ix < peer.Region[1]; ix++ {
			ghTable[ix] = *peer.Backends
		}
	}

	globalhashtable = &ghTable
	return true
}

func RefreshGlobleHashtable() bool {
	HaTable.Rwlock.RLock()
	defer HaTable.Rwlock.RUnlock()
	for _, peer := range HaTable.HashTable {
		if len(peer.Region) != 2 {
			seelog.Critical("Globalhashtable Format Error")
			return false
		}
		for ix := peer.Region[0]; ix < peer.Region[1]; ix++ {
			(*globalhashtable)[ix] = *peer.Backends
		}
	}

	return true
}

func SaveHashTable(path string) bool {
	return utils.SaveConf(HaTable.HashTable, path)
}

func GetGlobleHashtable() *GlobalHashTable {
	return &globalhashtable
}
