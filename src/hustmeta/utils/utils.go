package utils

import (
	"crypto/md5"
	"crypto/sha1"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"runtime"
	"strings"
	"time"
	"unsafe"

	seelog "github.com/cihub/seelog"
)

const (
	timefmt = "2006-01-02 15:04:05"
)

func Recover() {
	if e := recover(); e != nil {
		seelog.Errorf("Dispatch Panic: %v", e)
	}
}

func BytesToString(b []byte) string {
	return *(*string)(unsafe.Pointer(&b))
}

func FileSize(file string) (int64, error) {
	f, e := os.Stat(file)
	if e != nil {
		return 0, e
	}
	return f.Size(), nil
}

func IsExist(path string) bool {
	_, err := os.Stat(path)
	return err == nil || os.IsExist(err)
}

func Md5(plain string) string {
	h := md5.New()
	h.Write([]byte(plain))
	cipherStr := h.Sum(nil)
	return hex.EncodeToString(cipherStr)
}

func Sha1(plain string) string {
	h := sha1.New()
	h.Write([]byte(plain))
	cipherStr := h.Sum(nil)
	return hex.EncodeToString(cipherStr)
}

func SaveConfigure(conf interface{}, path string) error {
	buf, err := json.MarshalIndent(conf, "", "    ")
	if nil != err {
		return err
	}
	f, err := os.Create(path)
	if nil != err {
		return err
	}
	defer f.Close()
	size := len(buf)
	n, err := f.Write(buf)
	if nil != err {
		return err
	}
	if size != n {
		return fmt.Errorf("filesize not match, filesize: %v, readsize: %v", size, n)
	}
	return nil
}

func LoadConfigure(path string, cf interface{}) error {
	filesize, err := FileSize(path)
	if nil != err || filesize < 1 {
		return err
	}
	buf := make([]byte, filesize)
	f, err := os.Open(path)
	defer f.Close()
	if nil != err {
		return err
	}
	n, err := f.Read(buf)
	if nil != err {
		return err
	}
	if filesize != int64(n) {
		return fmt.Errorf("filesize not match, filesize: %v, readsize: %v", filesize, n)
	}
	if err := UnmarshalJson(buf, cf); nil != err {
		return err
	}
	return nil
}

func MarshalJson(objVal interface{}) string {
	data, err := json.Marshal(objVal)
	if nil != err {
		return ""
	}
	return BytesToString(data)
}

func UnmarshalJson(jsonVal []byte, objVal interface{}) error {
	decoder := json.NewDecoder(strings.NewReader(BytesToString(jsonVal)))
	decoder.UseNumber()
	return decoder.Decode(objVal)
}

type CallStack struct {
	Func string
	Line int
	Err  string
}

func (this *CallStack) ToString() string {
	if len(this.Err) < 1 {
		return fmt.Sprintf("{%v:%v}", this.Func, this.Line)
	}
	return fmt.Sprintf("{%v:%v|%v}", this.Func, this.Line, this.Err)
}

func GetCallStack(skip int, err error) *CallStack {
	// skip: 0 => self
	// skip: 1 => caller
	// skip: 2 => caller's caller
	pc, _, line, _ := runtime.Caller(skip)
	p := runtime.FuncForPC(pc)

	errStr := ""
	if nil != err {
		errStr = err.Error()
	}

	return &CallStack{p.Name(), line, errStr}
}

var enableErrorTrace = true

func EnableErrorTrace(enable bool) {
	enableErrorTrace = enable
}

// MakeError
// @lastErr: should not be nil
// @newErr: could be nil
func MakeError(lastErr error, newErr error, skip int) error {
	if !enableErrorTrace {
		if nil == newErr {
			return lastErr
		}
		return newErr
	}
	callstack := GetCallStack(skip, newErr)
	return fmt.Errorf("%v|%v", callstack.ToString(), lastErr.Error())
}

func AppendError(lastErr error, newErr error) error {
	return MakeError(lastErr, newErr, 3)
}

func NewError(err error) error {
	return MakeError(err, nil, 3)
}

func IsSameDay(t1, t2 *time.Time) bool {
	y1, m1, d1 := t1.Date()
	y2, m2, d2 := t2.Date()
	return y1 == y2 && m1 == m2 && d1 == d2
}

func ParseTime(tm string) (time.Time, error) {
	if "0000-00-00 00:00:00" == tm {
		return time.Time{}, nil
	}
	v, err := time.Parse("2006-01-02 15:04:05.999999999 -0700 MST", tm)
	if nil == err {
		return v, nil
	}
	v, err = time.Parse(timefmt, tm)
	if nil == err {
		return v, nil
	}
	return time.Parse("2006-01-02", tm)
}

func ParseUnixTime(tm string) (int64, error) {
	t, err := ParseTime(tm)
	if nil != err {
		return 0, err
	}
	return t.Unix(), nil
}

func TimeToString(t time.Time) string {
	return t.Format(timefmt)
}

func Days(begin time.Time, end time.Time) uint {
	return uint(end.Sub(begin).Hours() / 24)
}

func LeftSecsOfToday(now time.Time) int64 {
	y, m, d := now.Date()
	tomorrow := time.Date(y, m, d, 23, 59, 59, 0, now.Location())
	return tomorrow.Unix() - now.Unix()
}

func IsValidUUID(uuid string) bool {
	r := regexp.MustCompile("^[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}$")
	return r.MatchString(uuid)
}

func GetDatetime(t time.Time) time.Time {
	return time.Date(t.Year(), t.Month(), t.Day(), 0, 0, 0, 0, t.Location())
}

func DecodeJsonField(k, v string, d map[string]interface{}) error {
	if len(v) < 1 {
		delete(d, k)
		return nil
	}
	o := map[string]interface{}{}
	err := UnmarshalJson([]byte(v), &o)
	if nil != err {
		return err
	}
	delete(d, k)
	d[k] = o
	return nil
}
