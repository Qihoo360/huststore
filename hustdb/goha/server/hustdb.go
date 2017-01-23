package server

import (
	"encoding/json"
)

type DBHandle struct {
}

type HustdbResponse struct {
	Data    []byte
	Code    int
	Version int
	Backend string
}

func (this *DBHandle) HustdbExists(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbPut(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbGet(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbDel(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbHdel(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbHincrby(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbHset(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbSadd(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbSismember(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbSrem(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbZadd(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbZrangebyrank(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbZrangebyscore(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbZrem(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbZscore(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbHexists(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
func (this *DBHandle) HustdbHget(params map[string][]byte) *HustdbResponse {
	b, _ := json.Marshal(params)
	return &HustdbResponse{
		Data: b,
		Code: 200,
	}
}
