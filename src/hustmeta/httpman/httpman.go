package httpman

import (
	"bytes"
	"crypto/tls"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httputil"
	"net/url"
	"time"

	"strings"

	"fmt"

	seelog "github.com/cihub/seelog"
)

type HttpConfig struct {
	MaxIdleConnsPerHost   int
	ResponseHeaderTimeout int
	Timeout               int
	KeepAlive             int
	Debug                 bool
}

type Session struct {
	Client *http.Client
}

var session *Session
var debug bool

var (
	timeout   int
	keepalive int
)

func Init(httpConfig HttpConfig) {
	timeout, keepalive = httpConfig.Timeout, httpConfig.KeepAlive
	session = NewSession(httpConfig.MaxIdleConnsPerHost, httpConfig.ResponseHeaderTimeout)
	debug = httpConfig.Debug
}

func SetDebug(on bool) {
	debug = on
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

type Args struct {
	Url      string
	Data     []byte
	Headers  map[string]string
	Cookies  []*http.Cookie
	Username string
	Passwd   string
}

func (this *Session) Get(args *Args) (int, []byte, http.Header) {
	return fetch(this.Client, "GET", args)
}

func (this *Session) Post(args *Args) (int, []byte, http.Header) {
	return fetch(this.Client, "POST", args)
}

func (this *Session) Put(args *Args) (int, []byte, http.Header) {
	return fetch(this.Client, "PUT", args)
}

func (this *Session) Delete(args *Args) (int, []byte, http.Header) {
	return fetch(this.Client, "DELETE", args)
}

func setRequest(headers map[string]string, cookies []*http.Cookie, username string, passwd string, req *http.Request) {
	if len(username) > 0 && len(passwd) > 0 {
		req.SetBasicAuth(username, passwd)
	}

	if nil != headers {
		for key, val := range headers {
			if "host" == strings.ToLower(key) {
				req.Host = val
			} else {
				req.Header.Set(key, val)
			}
		}
	}

	if nil != cookies {
		for _, cookie := range cookies {
			req.AddCookie(cookie)
		}
	}
}

func fetch(client *http.Client, method string, args *Args) (int, []byte, http.Header) {

	defer protect()

	var body io.Reader
	if len(args.Data) < 1 {
		body = nil
	} else {
		body = ioutil.NopCloser(bytes.NewReader(args.Data))
	}
	req, err := http.NewRequest(method, args.Url, body)
	if err != nil {
		_ = seelog.Errorf("NewRequest_Error: %v", err)
		return http.StatusInternalServerError, nil, nil
	}

	setRequest(args.Headers, args.Cookies, args.Username, args.Passwd, req)

	if debug {
		r, _ := httputil.DumpRequestOut(req, true)
		seelog.Debug(string(r))
	}

	resp, err := client.Do(req)
	if err != nil {
		_ = seelog.Errorf("Client_Do: %v", err)
		return http.StatusInternalServerError, nil, nil
	}
	defer func() {
		if err := resp.Body.Close(); err != nil {
			_ = seelog.Errorf("close failed, %v", err)
		}
	}()

	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		_ = seelog.Errorf("Read_Response_Body_Error: %v", err)
		return http.StatusInternalServerError, nil, nil
	}
	return resp.StatusCode, respBody, resp.Header
}

func localDial(network, addr string) (net.Conn, error) {
	dial := net.Dialer{
		Timeout:   time.Duration(timeout) * time.Second,
		KeepAlive: time.Duration(keepalive) * time.Second,
	}
	return dial.Dial(network, addr)
}

func protect() {
	if err := recover(); err != nil {
		_ = seelog.Errorf("Http_Panic:%v", err)
	}
}

func BuildArgs(args map[string]string) string {
	u, _ := url.Parse("")
	q := u.Query()
	for k, v := range args {
		q.Set(k, v)
	}
	return q.Encode()
}

func HttpBuildQuery(strArgs map[string]string, arrArgs map[string][]string) string {
	u, _ := url.Parse("")
	q := u.Query()
	if nil != strArgs {
		for k, val := range strArgs {
			q.Set(k, val)
		}
	}
	if nil != arrArgs {
		for k, vals := range arrArgs {
			for i, v := range vals {
				key := fmt.Sprintf("%v[%v]", k, i)
				q.Set(key, v)
			}
		}
	}
	return q.Encode()
}

func UrlEncode(str string) (string, error) {
	u, err := url.Parse(str)
	if nil != err {
		return "", err
	}
	q := u.Query()
	return q.Encode(), nil
}

func QueryArray(args url.Values, key string) []string {
	v, ok := args[key]
	if ok {
		return v
	}
	vals := []string{}
	for k, v := range args {
		if strings.Contains(k, key) {
			vals = append(vals, v[0])
		}
	}
	return vals
}
