package comm

import "github.com/cihub/seelog"

func Protect() {
	if p := recover(); p != nil {
		seelog.Errorf("Panic Catched :%#v", p)
	}
}
