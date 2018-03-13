package service

import (
	"encoding/json"
	"hustmeta/utils"
	"net/http"
	"time"

	seelog "github.com/cihub/seelog"
	"github.com/labstack/echo"
)

// Set "/set"
func Set(c echo.Context) error {
	defer utils.Recover()
	collectTime := time.Now()
	defer utils.Collect(collectTime)

	key := c.QueryParam("key")
	if len(key) < 1 {
		return c.String(http.StatusBadRequest, "no key in param")
	}
	val := json.RawMessage{}
	if err := c.Bind(&val); nil != err {
		return c.String(http.StatusBadRequest, utils.NewError(err).Error())
	}

	seelog.Trace("key: ", key)
	seelog.Trace("val", utils.BytesToString(val))

	// TODO
	return c.String(http.StatusOK, "")
}
