package httpman

import (
	"bytes"
	"crypto/tls"

	def "../../internal/defines"

	"io"
	"io/ioutil"
	"net"
	"net/http"
	"time"

	seelog "github.com/cihub/seelog"
)

type Session struct {
	Client *http.Client
}

var hcClient *http.Client

var session *Session

var (
	timeout   int
	hctimeout int
	keepalive int
)

func InitHttp(httpConfig def.HttpConf, hctimeout int) {
	timeout, keepalive = httpConfig.Timeout, httpConfig.KeepAlive
	session = NewSession(httpConfig.MaxIdleConnsPerHost, httpConfig.ResponseHeaderTimeout)
	hcClient = &http.Client{
		Transport: &http.Transport{
			Dial:                  dialTimeout,
			DisableKeepAlives:     false,
			MaxIdleConnsPerHost:   httpConfig.MaxIdleConnsPerHost,
			ResponseHeaderTimeout: time.Duration(hctimeout) * time.Second,
			TLSClientConfig:       &tls.Config{InsecureSkipVerify: true},
		},
	}
}

func NewSession(conn, timeout int) *Session {
	session := &Session{
		Client: &http.Client{
			Transport: &http.Transport{
				Dial:                  localDial,
				DisableKeepAlives:     false,
				MaxIdleConnsPerHost:   conn,
				ResponseHeaderTimeout: time.Duration(timeout) * time.Second,
				TLSClientConfig:       &tls.Config{InsecureSkipVerify: true},
			},
		},
	}
	return session
}

func GetSession() *Session {
	return session
}

func HttpBasicWithHeader(url, method string, data []byte, headers map[string]string, username, passwd string, shorttimeout bool) (int, []byte, http.Header) {
	defer Protect()
	var body io.Reader
	if len(data) == 0 {
		body = nil
	} else {
		body = ioutil.NopCloser(bytes.NewReader(data))
	}
	req, err := http.NewRequest(method, url, body)
	if err != nil {
		seelog.Errorf("NewRequest_Error: %v", err)
		return http.StatusInternalServerError, nil, nil
	}
	req.SetBasicAuth(username, passwd)

	for key, val := range headers {
		req.Header.Set(key, val)
	}

	var client *http.Client
	if !shorttimeout {
		client = GetSession().Client
	} else {
		client = hcClient
	}

	resp, err := client.Do(req)
	if err != nil {
		seelog.Errorf("Client_Do: %v", err)
		return http.StatusInternalServerError, nil, nil
	}
	defer resp.Body.Close()

	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		seelog.Errorf("Read_Response_Body_Error: %v", err)
		return http.StatusInternalServerError, nil, nil
	}
	return resp.StatusCode, respBody, resp.Header
}

func HttpBasic(url, method string, data []byte, headers map[string]string, username, passwd string) (int, []byte, http.Header) {
	return HttpBasicWithHeader(url, method, data, headers, username, passwd, false)
}

func HttpBasicWithTimeout(url, method string, data []byte, headers map[string]string, username, passwd string) (int, []byte, http.Header) {
	return HttpBasicWithHeader(url, method, data, headers, username, passwd, true)
}

func localDial(network, addr string) (net.Conn, error) {
	dial := net.Dialer{
		Timeout:   time.Duration(timeout) * time.Second,
		KeepAlive: time.Duration(keepalive) * time.Second,
	}
	return dial.Dial(network, addr)
}

func dialTimeout(network, addr string) (net.Conn, error) {
	return net.DialTimeout(network, addr, time.Second*time.Duration(hctimeout))
}

func Protect() {
	if p := recover(); p != nil {
		seelog.Errorf("Panic Catched :%#v", p)
	}
}
