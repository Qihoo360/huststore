package comm

import (
	"bytes"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	def "../../internal/defines"
	"../../internal/httpman"
	"../../internal/utils"

	"github.com/cihub/seelog"
)

var (
	hustdbUser      string
	hustdbPwd       string
	hustdbReqHeader map[string]string
)

func init() {
	hustdbReqHeader = map[string]string{}
	hustdbReqHeader["Content-Type"] = "text/plain"
}

func HustdbInit(conf *def.HustdbConf) {
	hustdbUser = conf.User
	hustdbPwd = conf.Passwd
}

func ComposeUrl(backend string, op string, fieldmap map[string][]byte) string {
	var buffer bytes.Buffer
	buffer.WriteString("http://")
	buffer.WriteString(backend)
	buffer.WriteString("/hustdb/")
	buffer.WriteString(op)
	buffer.WriteString("?")
	for k, v := range fieldmap {
		buffer.WriteString(k)
		buffer.WriteString("=")
		buffer.Write(v)
		buffer.WriteString("&")
	}

	return strings.TrimSuffix(buffer.String(), "&")
}

func ComposeUrlWithKey(backend string, op string, fieldmap map[string][]byte) string {
	var buffer bytes.Buffer
	buffer.WriteString("http://")
	buffer.WriteString(backend)
	buffer.WriteString("/hustdb/")
	buffer.WriteString(op)
	buffer.WriteString("?")
	for k, v := range fieldmap {
		buffer.WriteString(k)
		buffer.WriteString("=")
		buffer.Write(v)
		buffer.WriteString("&")
	}

	return strings.TrimSuffix(buffer.String(), "&")
}

/* Hustdb kv API */
func HustdbPut(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "put", args)
	httpCode, _, _ := HttpPost(url, val)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbGet(backend string, args map[string][]byte) *HustdbResponse {
	url := ComposeUrl(backend, "get", args)
	httpCode, body, _ := HttpGet(url)

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return &HustdbResponse{Code: httpCode, Data: body}
}

func HustdbGet2(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "get", args)
	httpCode, body, header := HttpGet(url)
	//fmt.Printf("header :%v\n", header)
	ver, _ := strconv.Atoi(header.Get("Version"))

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Data: body, Version: ver}
}

func HustdbDel(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "del", args)
	httpCode, _, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbExist(backend string, args map[string][]byte) *HustdbResponse {
	url := ComposeUrl(backend, "exist", args)
	httpCode, _, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return &HustdbResponse{Code: httpCode}
}

/* Hustdb hash API */
func HustdbHset(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "hset", args)
	httpCode, _, respHeader := HttpPost(url, val)
	// fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	ver, _ := strconv.Atoi(respHeader.Get("Version"))
	retChan <- &HustdbResponse{Code: httpCode, Version: ver, Backend: backend}
}

func HustdbHget(backend string, args map[string][]byte) *HustdbResponse {
	url := ComposeUrl(backend, "hget", args)
	httpCode, body, respHeader := HttpGet(url)
	ver, _ := strconv.Atoi(respHeader.Get("Version"))

	// fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return &HustdbResponse{Code: httpCode, Data: body, Version: ver}
}

func HustdbHget2(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "hget", args)
	httpCode, body, respHeader := HttpGet(url)
	ver, _ := strconv.Atoi(respHeader.Get("Version"))

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Data: body, Version: ver}
}

func HustdbHdel(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "hdel", args)
	httpCode, _, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbHexist(backend string, args map[string][]byte) *HustdbResponse {
	url := ComposeUrl(backend, "hexist", args)
	httpCode, _, _ := HttpGet(url)
	return &HustdbResponse{Code: httpCode}
}

/* Hustdb set API */
func HustdbSadd(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "sadd", args)
	httpCode, _, _ := HttpPost(url, val)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbSrem(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "srem", args)
	httpCode, _, _ := HttpPost(url, val)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbSismember(backend string, args map[string][]byte, val []byte) *HustdbResponse {
	url := ComposeUrl(backend, "sismember", args)
	httpCode, _, _ := HttpPost(url, val)
	return &HustdbResponse{Code: httpCode}
}

func HustdbZadd(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "zadd", args)
	httpCode, _, _ := HttpPost(url, val)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbZscore(backend string, args map[string][]byte, val []byte) *HustdbResponse {
	url := ComposeUrl(backend, "zscore", args)
	httpCode, body, _ := HttpPost(url, val)

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return &HustdbResponse{Code: httpCode, Data: body}
}

func HustdbZscore2(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "zscore", args)
	httpCode, body, respHeader := HttpPost(url, val)
	ver, _ := strconv.Atoi(respHeader.Get("Version"))

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Data: body, Version: ver}
}

func HustdbZrem(backend string, args map[string][]byte, val []byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "zrem", args)
	httpCode, _, _ := HttpPost(url, val)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	retChan <- &HustdbResponse{Code: httpCode, Backend: backend}
}

func HustdbZismember(backend string, args map[string][]byte, val []byte) *HustdbResponse {
	url := ComposeUrl(backend, "zismember", args)
	httpCode, _, _ := HttpPost(url, val)
	return &HustdbResponse{Code: httpCode}
}

func HustdbZrangebyrank(backend string, args map[string][]byte) (int, []byte) {
	url := ComposeUrl(backend, "zrangebyrank", args)
	httpCode, body, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)

	return httpCode, body
}

func HustdbZrangebyscore(backend string, args map[string][]byte) (int, []byte) {
	url := ComposeUrl(backend, "zrangebyscore", args)
	httpCode, body, _ := HttpGet(url)

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return httpCode, body
}

func HustdbAlive(backend string) int {
	url := utils.ConcatString("http://", backend, "/status.html")
	httpCode, _, _ := HttpGetWithTimeout(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return httpCode
}

func HustdbBinlog(backend string, args map[string][]byte, val []byte) int {
	defer Protect()
	url := ComposeUrl(backend, "binlog", args)
	httpCode, _, _ := HttpPost(url, val)

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	if httpCode != HttpOk {
		seelog.Criticalf("Binlog|%v\n%v", url, val)
	}
	return httpCode
}

func HustdbSismembers(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "sismembers", args)
	httpCode, body, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	if httpCode == HttpOk {
		retChan <- &HustdbResponse{Code: httpCode, Data: body}
	} else {
		retChan <- &HustdbResponse{Code: httpCode}
	}
}

func HustdbHkeys(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "hkeys", args)
	httpCode, body, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	if httpCode == HttpOk {
		retChan <- &HustdbResponse{Code: httpCode, Data: body}
	} else {
		retChan <- &HustdbResponse{Code: httpCode}
	}
}

func HustdbKeys(backend string, args map[string][]byte, retChan chan *HustdbResponse) {
	url := ComposeUrl(backend, "keys", args)
	httpCode, body, _ := HttpGet(url)
	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	if httpCode == HttpOk {
		retChan <- &HustdbResponse{Code: httpCode, Data: body}
	} else {
		retChan <- &HustdbResponse{Code: httpCode}
	}
}

func HustdbHincrby(backend string, args map[string][]byte) *HustdbResponse {
	url := ComposeUrl(backend, "hincrby", args)
	httpCode, body, _ := HttpGet(url)

	//fmt.Printf("url : %v\nhttpCode : %v\n", url, httpCode)
	return &HustdbResponse{Code: httpCode, Data: body}
}

func HttpPostWithTimeout(url string, data []byte) (int, []byte, http.Header) {
	defer Protect()
	return httpman.HttpBasicWithTimeout(url, "POST", data, hustdbReqHeader, hustdbUser, hustdbPwd)
}

func HttpPost(url string, data []byte) (int, []byte, http.Header) {
	defer Protect()
	return httpman.HttpBasic(url, "POST", data, hustdbReqHeader, hustdbUser, hustdbPwd)
}

func HttpGetWithTimeout(url string) (int, []byte, http.Header) {
	defer Protect()
	return httpman.HttpBasicWithTimeout(url, "GET", nil, hustdbReqHeader, hustdbUser, hustdbPwd)
}

func HttpGet(url string) (int, []byte, http.Header) {
	defer Protect()
	return httpman.HttpBasic(url, "GET", nil, hustdbReqHeader, hustdbUser, hustdbPwd)
}
