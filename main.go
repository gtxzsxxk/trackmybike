package main

import (
	"github.com/gin-gonic/gin"
)

func main() {
	db_initialize()
	// 创建基于cookie的存储引擎，secret11111 参数是用于加密的密钥
	r := gin.Default()
	r.Delims("{[{", "}]}")
	r.LoadHTMLFiles("index.html", "share.html")
	r.Static("/assets", "./assets")
	r.StaticFile("/favicon.ico", "./favicon.ico")
	r.StaticFile("/favicon512.png", "./favicon512.png")
	r.StaticFile("/manifest.json", "./manifest.json")
	r.StaticFile("/sw.js", "./sw.js")
	r.GET("/", index)
	r.GET("/login/:username/:password", login)
	/* Operations on Device Level */
	r.GET("/devices", getDevice)
	r.POST("/devices/:name/:type/:icon", addDevice)
	r.DELETE("/devices/:name", deleteDevice)
	/* For management use */
	r.PUT("/device_property/:name", updateDeviceProperty)
	/* Operations on Device's Property Level */
	r.GET("/device_details/:name", getDeviceDetail)
	/* For IOT clients to add a new series of data */
	r.POST("/device_details/:name", addDeviceDetail)
	/* For Web Page to update single latest property's value */
	r.PUT("/device_details/:name", updateDeviceDetail)
	r.GET("/share/:name", share_device_page)
	r.GET("/share/devices/:name", getDevice_share)
	r.PUT("/share/devices/:name", allowShare)
	r.DELETE("/share/devices/:name", disallowShare)
	r.GET("/share/device_details/:name", getDeviceDetail_share)

	r.Run("[::]:9080") // 监听并在 0.0.0.0:9080 上启动服务
}