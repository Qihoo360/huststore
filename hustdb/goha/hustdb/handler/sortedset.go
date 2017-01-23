package handler

import (
	"../binlog"
	"../comm"
	"../peers"
)

func (p *HustdbHandler) HustdbZismember(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")

	backends := peers.FetchHustdbPeers(tb)
	for _, backend := range backends {
		resp := comm.HustdbZismember(backend, args, key)
		if resp.Code == comm.HttpOk {
			return resp
		}
	}

	return &comm.HustdbResponse{Code: 0}
}

func (p *HustdbHandler) HustdbZscore2(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")

	backends := peers.FetchHustdbPeers(tb)
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	retChan := make(chan *comm.HustdbResponse, len(backends))
	for _, backend := range backends {
		go comm.HustdbZscore2(backend, args, key, retChan)
	}

	maxVer := 0
	hustdbResp := &comm.HustdbResponse{Code: comm.HttpNotFound}
	for ix := 0; ix < cap(retChan); ix++ {
		resp := <-retChan
		if resp.Code == comm.HttpOk {
			hustdbResp.Code = comm.HttpOk
		}
		if resp.Version > maxVer {
			maxVer = resp.Version
			hustdbResp.Data = resp.Data
		}
	}

	return hustdbResp
}

func (p *HustdbHandler) HustdbZscore(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")

	backends := peers.FetchHustdbPeers(tb)

	hustdbResp := &comm.HustdbResponse{Code: 0}
	for _, backend := range backends {
		resp := comm.HustdbZscore(backend, args, key)

		if resp.Code == comm.HttpOk {
			return resp
		}
	}

	return hustdbResp
}

func (p *HustdbHandler) HustdbZadd(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")

	backends := peers.FetchHustdbPeers(tb)
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	retChan := make(chan *comm.HustdbResponse, len(backends))
	for _, backend := range backends {
		go comm.HustdbZadd(backend, args, key, retChan)
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
		binlog.Do(putSuccessBackend, putFailedBackend, "zadd", args, key)
	}

	return hustdbResp
}

func (p *HustdbHandler) HustdbZrangebyscore(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	backends := peers.FetchHustdbPeers(tb)
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	if _, ok := args["offset"]; !ok {
		args["offset"] = []byte("0")
	}

	if _, ok := args["size"]; !ok {
		args["size"] = []byte("1000")
	}

	for _, backend := range backends {
		code, body := comm.HustdbZrangebyscore(backend, args)
		if code == comm.HttpOk {
			return &comm.HustdbResponse{Code: comm.HttpOk, Data: body}
		}
	}

	return &comm.HustdbResponse{Code: comm.HttpNotFound}
}

func (p *HustdbHandler) HustdbZrem(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}

	key, ok := args["key"]
	if !ok {
		return NilHustdbResponse
	}
	delete(args, "key")
	backends := peers.FetchHustdbPeers(tb)
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	retChan := make(chan *comm.HustdbResponse, len(backends))
	for _, backend := range backends {
		go comm.HustdbZrem(backend, args, key, retChan)
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
		binlog.Do(delSuccessBackend, delFailedBackend, "zrem", args, key)
	}
	return hustdbResp
}

func (p *HustdbHandler) HustdbZrangebyrank(args map[string][]byte) *comm.HustdbResponse {
	itb, ok := args["tb"]
	tb := string(itb)
	if !ok {
		return NilHustdbResponse
	}
	backends := peers.FetchHustdbPeers(tb)
	if len(backends) == 0 {
		return NilHustdbResponse
	}

	for _, backend := range backends {
		code, body := comm.HustdbZrangebyrank(backend, args)
		if code == comm.HttpOk {
			return &comm.HustdbResponse{Code: comm.HttpOk, Data: body}
		}
	}

	return &comm.HustdbResponse{Code: comm.HttpNotFound}
}
