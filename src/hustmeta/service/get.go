package service

import (
	"hustmeta/utils"
	"net/http"
	"time"

	seelog "github.com/cihub/seelog"
	"github.com/labstack/echo"
)

// Get "/get"
func Get(c echo.Context) error {
	defer utils.Recover()
	collectTime := time.Now()
	defer utils.Collect(collectTime)

	key := c.QueryParam("key")
	if len(key) < 1 {
		return c.String(http.StatusBadRequest, "no key in param")
	}
	seelog.Trace("key: ", key)

	// TODO
	return c.String(http.StatusOK, "")
}
