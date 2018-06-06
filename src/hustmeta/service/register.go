package service

import (
	"github.com/labstack/echo"
)

// router
func Register(e *echo.Echo) {
	e.GET("/get", Get)
	e.POST("/set", Set)
}
