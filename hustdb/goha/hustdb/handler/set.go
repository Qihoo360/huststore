package handler

import (
	binlog "../binlog"
	"../comm"
	"../peers"
)

func (p *HustdbHandler) HustdbSadd(args map[string][]byte) *comm.HustdbResponse {
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")
	_, ok = args["tb"]
	if !ok {
		return NilHustdbResponse
	}

	backends := peers.FetchHustdbPeers(string(key))
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	retChan := make(chan *comm.HustdbResponse, len(backends))
	for _, backend := range backends {
		go comm.HustdbSadd(backend, args, key, retChan)
	}

	putSucc := 0
	var putFailedBackend string
	var putSuccessBackend string
	hustdbResp := &comm.HustdbResponse{Code: 0}
	for ix := 0; ix < cap(retChan); ix++ {
		resp := <-retChan
		if resp.Code == comm.HttpOk {
			putSucc++
			hustdbResp.Code = comm.HttpOk
			putSuccessBackend = resp.Backend
		} else {
			putFailedBackend = resp.Backend
		}
	}

	/* Need Binlog */
	if putSucc != 0 && putSucc != len(backends) {
		binlog.Do(putSuccessBackend, putFailedBackend, "sadd", args, key)
	}

	return hustdbResp
}

func (p *HustdbHandler) HustdbSismember(args map[string][]byte) *comm.HustdbResponse {
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")
	_, ok = args["tb"]
	if !ok {
		return NilHustdbResponse
	}

	backends := peers.FetchHustdbPeers(string(key))
	for _, backend := range backends {
		resp := comm.HustdbSismember(backend, args, key)
		if resp.Code == comm.HttpOk {
			return resp
		}
	}

	return &comm.HustdbResponse{Code: 0}
}

func (p *HustdbHandler) HustdbSrem(args map[string][]byte) *comm.HustdbResponse {
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")
	_, ok = args["tb"]
	if !ok {
		return NilHustdbResponse
	}

	backends := peers.FetchHustdbPeers(string(key))
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	retChan := make(chan *comm.HustdbResponse, len(backends))
	for _, backend := range backends {
		go comm.HustdbSrem(backend, args, key, retChan)
	}

	delSucc := 0
	var delFailedBackend string
	var delSuccessBackend string
	hustdbResp := &comm.HustdbResponse{Code: 0}
	for ix := 0; ix < cap(retChan); ix++ {
		resp := <-retChan
		if resp.Code == comm.HttpOk {
			delSucc++
			hustdbResp.Code = comm.HttpOk
			delSuccessBackend = resp.Backend
		} else {
			delFailedBackend = resp.Backend
		}
	}

	/* Need Binlog */
	if delSucc != 0 && delSucc != len(backends) {
		binlog.Do(delSuccessBackend, delFailedBackend, "srem", args, key)
	}
	return hustdbResp
}
