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

	type Request struct {
		Key string          `query:"key"`
		Val json.RawMessage `query:"val"`
	}
	request := Request{}
	if err := c.Bind(&request); err != nil {
		return c.String(http.StatusBadRequest, utils.NewError(err).Error())
	}
	seelog.Trace(utils.MarshalJson(request))

	// TODO
	return c.String(http.StatusOK, "")
}
