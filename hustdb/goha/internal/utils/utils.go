package utils

import (
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"os"
	"strings"
	"unsafe"
)

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

func LoadFile(path string) ([]byte, bool) {
	filesize, err := FileSize(path)
	if nil != err || filesize < 1 {
		return nil, false
	}
	buf := make([]byte, filesize)
	f, err := os.Open(path)
	defer f.Close()

	if nil != err {
		return nil, false
	}
	n, err := f.Read(buf)
	if nil != err || filesize != int64(n) {
		return nil, false
	}

	return buf, true
}

func Md5(plain string) string {
	h := md5.New()
	h.Write([]byte(plain))
	cipherStr := h.Sum(nil)
	return hex.EncodeToString(cipherStr)
}

func SaveConf(conf interface{}, path string) bool {
	buf, err := json.MarshalIndent(conf, "", "    ")
	if nil != err {
		return false
	}
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0755)
	if nil != err {
		fmt.Println(err.Error())
		return false
	}
	defer f.Close()
	size := len(buf)
	n, err := f.Write(buf)
	if nil != err || n != size {
		return false
	}
	return true
}

func LoadConf(path string, cf interface{}) bool {
	filesize, err := FileSize(path)
	if nil != err || filesize < 1 {
		fmt.Println(err.Error())
		return false
	}
	buf := make([]byte, filesize)
	f, err := os.Open(path)
	defer f.Close()
	if nil != err {
		fmt.Println(err.Error())
		return false
	}
	n, err := f.Read(buf)
	if nil != err || filesize != int64(n) {
		fmt.Println(err.Error())
		return false
	}
	if nil != UnmarshalJson(buf, cf) {
		fmt.Println("UnmarshalJson error")
		return false
	}
	return true
}

func UnmarshalJson(jsonVal []byte, objVal interface{}) error {
	decoder := json.NewDecoder(strings.NewReader(BytesToString(jsonVal)))
	decoder.UseNumber()
	return decoder.Decode(objVal)
}
