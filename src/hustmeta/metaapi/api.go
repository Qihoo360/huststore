package metaapi

import (
	"hustmeta/utils"
)

func Set(key string, val []byte) error {
	return utils.GetDB().Put([]byte(key), val, nil)
}

func Get(key string) (string, error) {
	val, err := utils.GetDB().Get([]byte(key), nil)
	if nil != err {
		return "", utils.NewError(err)
	}
	return utils.BytesToString(val), nil
}
