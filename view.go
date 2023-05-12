package main

import (
	"crypto/md5"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"net"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/xxtea/xxtea-go/xxtea"
)

func index(c *gin.Context) {
	c.HTML(200, "index.html", gin.H{})
}

func login(c *gin.Context) {
	username := c.Param("username")
	password := c.Param("password")
	var user_s User
	err := Db.First(&user_s, "username=?", username).Error
	if err != nil {
		c.String(404, "{\"msg\":\"No such an user.\"}")
		return
	}
	if password == user_s.Password {
		c.String(200, "{\"msg\":\"success\",\"permission\":"+fmt.Sprintf("%d", user_s.Permission)+"}")
	} else {
		c.String(401, "{\"msg\":\"Invalid login attempt\"}")
	}
}

func addDevice(c *gin.Context) {
	device_name := c.Param("name")
	device_type := c.Param("type")
	device_icon := c.Param("icon")
	device_new := Device{
		Name: device_name,
		Type: device_type,
		Icon: device_icon,
	}
	Db.Create(&device_new)
	updateDeviceProperty(c)
}

func getDevice(c *gin.Context) {
	if !hasLogin(c, 0) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	var devices []Device
	Db.Find(&devices)
	fmt.Printf("======================================\r\n")
	for i, v := range devices {
		device_id := v.ID
		var property Property
		t_1 := time.Now()
		err := Db.First(&property, "device_id=?", device_id).Error
		t_2 := time.Since(t_1).Seconds()

		fmt.Printf("dev:%s 执行时间1：%.2f\r\n", v.Name, t_2)
		if err != nil {
			devices[i].Online = 0
			continue
		}
		var property_value PropertyValue
		t_1 = time.Now()
		err = Db.Last(&property_value, "property_id=?", property.ID).Error
		t_2 = time.Since(t_1).Seconds()
		fmt.Printf("dev:%s 执行时间2：%.2f\r\n", v.Name, t_2)
		if err != nil {
			devices[i].Online = 0
			continue
		}
		t_now := time.Now()
		elapsed := t_now.Sub(property_value.CreatedAt).Seconds()

		if elapsed > 120 {
			devices[i].Online = 0
		} else {
			devices[i].Online = 1
		}
		devices[i].LastSeen = uint(property_value.UpdatedAt.UnixMilli())
	}
	bytedata, _ := json.Marshal(devices)
	c.String(200, string(bytedata))
}

func deleteDevice(c *gin.Context) {
	if !hasLogin(c, 2) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	device_name := c.Param("name")
	var dev Device
	if err := Db.First(&dev, "name=?", device_name).Error; err != nil {
		c.String(400, "{\"msg\":\"No such a device.\"}")
		return
	}
	Db.Delete(&dev)
	c.String(200, "{\"msg\":\"success\"}")
}

func getDeviceDetail(c *gin.Context) {
	if !hasLogin(c, 0) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	var device Device
	device_name := c.Param("name")
	Db.First(&device, "name=?", device_name)
	var properties []Property
	Db.Where("device_id=?", device.ID).Find(&properties)
	type DeviceDetail struct {
		Property
		PropertyValue
	}
	var dd_list []DeviceDetail
	for _, v := range properties {
		tmp_dd := DeviceDetail{}
		tmp_dd.Type = v.Type
		tmp_dd.Name = v.Name
		var tmp_d_value PropertyValue
		Db.Last(&tmp_d_value, "property_id=?", v.ID)
		if v.Type == "map" {
			var tmp_d_value PropertyValue
			Db.Last(&tmp_d_value, "property_id=?", v.ID)
			tm_of_the_day := tmp_d_value.UpdatedAt
			tm_of_the_day_begin := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 0, 0, 0, 0, tm_of_the_day.Location())
			tm_of_the_day_end := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 23, 59, 59, 0, tm_of_the_day.Location())
			var results []PropertyValue
			err := Db.Where("property_id=? AND updated_at BETWEEN ? AND ?", v.ID, tm_of_the_day_begin, tm_of_the_day_end).Find(&results).Error
			if err != nil {
				tmp_dd.Value = "[[0,0]]"
			}
			var tmp_values []string
			for _, v := range results {
				tmp_values = append(tmp_values, v.Value)
			}
			tmp_byt, _ := json.Marshal(tmp_values)
			tmp_value_result := strings.ReplaceAll(string(tmp_byt), "\"", "")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, "\\", "")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, ",[[", ",[")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, "]]]", "]]")
			tmp_dd.Value = tmp_value_result
		} else if v.Type == "chart" {
			if !strings.Contains(v.Name, "@") {
				c.String(501, "{\"msg\":\"Bad settings of properties.\"}")
				return
			}
			type ChartResult struct {
				XAXIS []string `json:"xAxis"`
				Type  string   `json:"type"`
				YAXIS []string `json:"yAxis"`
			}
			var corresponding_property Property
			Db.Last(&corresponding_property, "name=? AND device_id=?", strings.ReplaceAll(v.Name, "@", ""), device.ID)
			var tmp_d_value PropertyValue
			Db.Last(&tmp_d_value, "property_id=?", corresponding_property.ID)
			tm_of_the_day := tmp_d_value.UpdatedAt
			tm_of_the_day_begin := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 0, 0, 0, 0, tm_of_the_day.Location())
			tm_of_the_day_end := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 23, 59, 59, 0, tm_of_the_day.Location())
			var results []PropertyValue
			err := Db.Where("property_id=? AND updated_at BETWEEN ? AND ?", corresponding_property.ID, tm_of_the_day_begin, tm_of_the_day_end).Find(&results).Error
			if err != nil {
				tmp_dd.Value = "[[0,0]]"
			}
			var xaxis_arr []string
			var yaxis_arr []string
			for _, v := range results {
				xaxis_arr = append(xaxis_arr, fmt.Sprintf("%02d:%02d", v.UpdatedAt.Hour(), v.UpdatedAt.Minute()))
				yaxis_arr = append(yaxis_arr, v.Value)
			}
			cr := ChartResult{
				XAXIS: xaxis_arr,
				YAXIS: yaxis_arr,
				Type:  v.Icon,
			}
			value_byt, _ := json.Marshal(cr)
			tmp_dd.Value = string(value_byt)
		} else {
			tmp_dd.Value = tmp_d_value.Value
		}
		tmp_dd.Size = v.Size
		tmp_dd.Icon = v.Icon
		tmp_dd.Unit = v.Unit
		tmp_dd.PropertyID = tmp_d_value.PropertyID
		dd_list = append(dd_list, tmp_dd)
	}
	bytedata, _ := json.Marshal(dd_list)
	c.String(200, string(bytedata))
	/*properties := strings.Split(device.Property, ",")
	var tmp_device_detail DeviceDetails
	var deviceDetails []DeviceDetails
	for _, v := range properties {
		Db.Where("device_id=? AND name=?", device.ID, v).Last(&tmp_device_detail)
		deviceDetails = append(deviceDetails, tmp_device_detail)
	}
	bytedata, _ := json.Marshal(deviceDetails)
	c.String(200, string(bytedata))*/
}

func addDeviceDetail(c *gin.Context) {
	name := c.Param("name")
	value := c.PostForm("value")
	usr_real := c.PostForm("username")
	value_inkey := c.PostForm("signature")
	var user User
	if err := Db.First(&user, "username=?", usr_real).Error; err != nil {
		c.String(401, "{\"msg\":\"No such an user.\"}")
		return
	}
	if user.Permission == 0 {
		c.String(401, "{\"msg\":\"User has no permission.\"}")
		return
	}
	value_tmp_bytes, _ := base64.StdEncoding.DecodeString(value_inkey)
	/* encrypt_data := xxtea.Encrypt([]byte("29.3"), []byte(user.Password[0:16]))*/
	/* fmt.Println(encrypt_data) */
	value_decrypt := string(xxtea.Decrypt(value_tmp_bytes, []byte(user.Password[0:16])))
	if value_decrypt != value {
		c.String(401, "{\"msg\":\"Check Error.\"}")
		return
	}
	var device Device
	if err := Db.First(&device, "name=?", name).Error; err != nil {
		c.String(400, "{\"msg\":\"No such a device.\"}")
		return
	}
	var values []string
	if err := json.Unmarshal([]byte(value), &values); err != nil {
		c.String(400, "{\"msg\":\"Bad JSON grammar.\"}")
		return
	}
	var properties []Property
	if err := Db.Find(&properties, "device_id=?", device.ID).Error; err != nil {
		c.String(500, "{\"msg\":\"Property Query Error.\"}")
		return
	}
	if len(values) != len(properties) {
		c.String(400, "{\"msg\":\"Value Relation Error.\"}")
		return
	}
	// for i, v := range values {
	// 	tmp_property_value := PropertyValue{
	// 		Property:   properties[i],
	// 		PropertyID: int(properties[i].ID),
	// 		Value:      v,
	// 	}
	// 	Db.Create(&tmp_property_value)
	// }
	for i, v := range properties {
		if values[i] != "\"\"" {
			tmp_property_value := PropertyValue{
				Property:   v,
				PropertyID: int(v.ID),
				Value:      values[i],
			}
			Db.Create(&tmp_property_value)
		}
	}
	c.String(200, "{\"msg\":\"success\"}")
	/*
		var properties []Property
		if err := Db.First(&property, "ID=?", property_id).Error; err != nil {
			c.String(400, "{\"msg\":\"No such a property.\"}")
			return
		}
		prop_id_int, _ := strconv.ParseInt(property_id, 10, 64)
		Db.Create(&PropertyValue{PropertyID: int(prop_id_int), Value: value})
		c.String(200, "{\"msg\":\"success\"}")*/
}

func updateDeviceDetail(c *gin.Context) {
	if !hasLogin(c, 1) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	name := c.Param("name")
	body_json := make(map[string]string)
	c.BindJSON(&body_json)
	property_str := body_json["property"]
	value_str := body_json["value"]
	var device Device
	if err := Db.First(&device, "name=?", name).Error; err != nil {
		c.String(400, "{\"msg\":\"No such a device.\"}")
		return
	}
	var property Property
	if err := Db.First(&property, "device_id=? AND name=?", device.ID, property_str).Error; err != nil {
		c.String(400, "{\"msg\":\"No such a property.\"}")
		return
	}
	var property_value PropertyValue
	if err := Db.Last(&property_value, "property_id=?", property.ID).Error; err != nil {
		property_value := PropertyValue{PropertyID: int(property.ID), Value: value_str}
		if err := Db.Create(&property_value).Error; err != nil {
			c.String(500, "{\"msg\":\"Server internal error.\"}")
			return
		}
	}
	property_value.Value = value_str
	Db.Save(&property_value)
	//向iot客户端推送所有信息
	type ServerPush struct {
		DeviceName string `json:"device"`
		Property   string `json:"property"`
		Value      string `json:"value"`
	}
	serverpush := ServerPush{DeviceName: name, Property: property_str, Value: value_str}
	bytedata, _ := json.Marshal(serverpush)
	conn, err := net.Dial("tcp", "127.0.0.1:9813")
	if err != nil {
		c.String(501, "{\"msg\":\"Failed to open connection with iot maintainer program.\"}")
	}
	_, err = conn.Write(bytedata)
	if err != nil {
		c.String(501, "{\"msg\":\"Failed to push data to iot client.\"}")
	}
	conn.Close()
	c.String(200, "{\"msg\":\"success\"}")
}

func updateDeviceProperty(c *gin.Context) {
	if !hasLogin(c, 2) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	name := c.Param("name")
	var device Device
	if err := Db.First(&device, "name=?", name).Error; err != nil {
		c.String(400, "{\"msg\":\"No such a device.\"}")
		return
	}
	/* Build new properties */
	body_json := make(map[string]string)
	c.BindJSON(&body_json)
	properties_str := body_json["properties"]

	type DeviceDetail struct {
		Property
		PropertyValue
	}
	var new_device_details []DeviceDetail
	if err := json.Unmarshal([]byte(properties_str), &new_device_details); err != nil {
		c.String(400, "{\"msg\":\"Cannot unmarshal json text.\"}")
		return
	}
	var old_device_details []Property
	Db.Where("device_id=?", device.ID).Find(&old_device_details)
	/* Inherit the properties' legacy */

	for _, v := range new_device_details {
		p_name := v.Name
		new_property := Property{
			Name:     v.Name,
			Type:     v.Type,
			Icon:     v.Icon,
			Unit:     v.Unit,
			Size:     v.Size,
			DeviceID: int(device.ID),
		}
		Db.Save(&new_property)
		for _, o_p := range old_device_details {
			if o_p.Name == p_name {
				o_p_id := o_p.ID
				Db.Model(&PropertyValue{}).Where("property_id=?", o_p_id).Update("property_id", new_property.ID)
				pv := PropertyValue{}
				if v.Type == "map" {
					if (!strings.Contains(v.Value, "[")) || strings.Contains(v.Value, "[[") || (!(v.Value[1] == '[' && v.Value[2] != '[')) {
						continue
					}
				}
				if v.Value == "\"\"" {
					continue
				}
				if err := Db.Last(&pv, "property_id=?", new_property.ID).Error; err != nil {
					pv := PropertyValue{
						Value:      v.Value,
						PropertyID: (int)(new_property.ID),
						Property:   new_property,
					}
					Db.Save(&pv)
				} else {
					Db.Model(&pv).Update("value", v.Value)
				}
				/*var old_property_values []PropertyValue
				Db.Where("property_id=?",o_p_id).Find(&old_property_values)
				for _,old_value:=range old_property_values {
					Db.Model(&old_value).Update()
				}*/
				break
			}
		}
	}
	for _, o_p := range old_device_details {
		Db.Delete(&o_p)
	}
	c.String(200, "{\"msg\":\"success\"}")
}

func share_device_page(c *gin.Context) {
	name := c.Param("name")
	var device Device
	if err := Db.First(&device, "name=?", name).Error; err != nil {
		c.String(404, "{\"msg\":\"No such a device.\"}")
		return
	}
	c.HTML(200, "share.html", gin.H{})
}

func getDevice_share(c *gin.Context) {
	name := c.Param("name")
	var devices []Device
	var devices_output []Device
	Db.Find(&devices)
	for i, v := range devices {
		if v.Name != name {
			continue
		}
		if v.Share == 0 {
			c.String(404, "{\"msg\":\"No such a device.\"}")
			return
		}
		device_id := v.ID
		var property Property
		err := Db.First(&property, "device_id=?", device_id).Error
		if err != nil {
			devices[i].Online = 0
			continue
		}
		var property_value PropertyValue
		err = Db.Last(&property_value, "property_id=?", property.ID).Error
		if err != nil {
			devices[i].Online = 0
			continue
		}
		t_now := time.Now()
		elapsed := t_now.Sub(property_value.CreatedAt).Seconds()
		if elapsed > 120 {
			devices[i].Online = 0
		} else {
			devices[i].Online = 1
		}
		devices[i].LastSeen = uint(property_value.UpdatedAt.UnixMilli())
		devices_output = append(devices_output, devices[i])
	}
	bytedata, _ := json.Marshal(devices_output)
	c.String(200, string(bytedata))
}

func allowShare(c *gin.Context) {
	if !hasLogin(c, 2) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	name := c.Param("name")
	var devices []Device
	Db.Find(&devices)
	for _, v := range devices {
		if v.Name != name {
			continue
		}
		v.Share = 1
		Db.Save(&v)
	}
	c.String(200, "{\"msg\":\"success\"}")
}

func disallowShare(c *gin.Context) {
	if !hasLogin(c, 2) {
		c.String(401, "{\"msg\":\"No permission.\"}")
		return
	}
	name := c.Param("name")
	var devices []Device
	Db.Find(&devices)
	for _, v := range devices {
		if v.Name != name {
			continue
		}
		v.Share = 0
		Db.Save(&v)
	}
	c.String(200, "{\"msg\":\"success\"}")
}

func getDeviceDetail_share(c *gin.Context) {
	var device Device
	device_name := c.Param("name")
	Db.First(&device, "name=?", device_name)
	if device.Share == 0 {
		c.String(404, "{\"msg\":\"No such a device.\"}")
		return
	}
	var properties []Property
	Db.Where("device_id=?", device.ID).Find(&properties)
	type DeviceDetail struct {
		Property
		PropertyValue
	}
	var dd_list []DeviceDetail
	for _, v := range properties {
		tmp_dd := DeviceDetail{}
		tmp_dd.Type = v.Type
		tmp_dd.Name = v.Name
		var tmp_d_value PropertyValue
		Db.Last(&tmp_d_value, "property_id=?", v.ID)
		if v.Type == "map" {
			var tmp_d_value PropertyValue
			Db.Last(&tmp_d_value, "property_id=?", v.ID)
			tm_of_the_day := tmp_d_value.UpdatedAt
			tm_of_the_day_begin := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 0, 0, 0, 0, tm_of_the_day.Location())
			tm_of_the_day_end := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 23, 59, 59, 0, tm_of_the_day.Location())
			var results []PropertyValue
			err := Db.Where("property_id=? AND updated_at BETWEEN ? AND ?", v.ID, tm_of_the_day_begin, tm_of_the_day_end).Find(&results).Error
			if err != nil {
				tmp_dd.Value = "[[0,0]]"
			}
			var tmp_values []string
			for _, v := range results {
				tmp_values = append(tmp_values, v.Value)
			}
			tmp_byt, _ := json.Marshal(tmp_values)
			tmp_value_result := strings.ReplaceAll(string(tmp_byt), "\"", "")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, "\\", "")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, ",[[", ",[")
			tmp_value_result = strings.ReplaceAll(tmp_value_result, "]]]", "]]")
			tmp_dd.Value = tmp_value_result
		} else if v.Type == "chart" {
			if !strings.Contains(v.Name, "@") {
				c.String(501, "{\"msg\":\"Bad settings of properties.\"}")
				return
			}
			type ChartResult struct {
				XAXIS []string `json:"xAxis"`
				Type  string   `json:"type"`
				YAXIS []string `json:"yAxis"`
			}
			var corresponding_property Property
			Db.Last(&corresponding_property, "name=? AND device_id=?", strings.ReplaceAll(v.Name, "@", ""), device.ID)
			var tmp_d_value PropertyValue
			Db.Last(&tmp_d_value, "property_id=?", corresponding_property.ID)
			tm_of_the_day := tmp_d_value.UpdatedAt
			tm_of_the_day_begin := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 0, 0, 0, 0, tm_of_the_day.Location())
			tm_of_the_day_end := time.Date(tm_of_the_day.Year(), tm_of_the_day.Month(), tm_of_the_day.Day(), 23, 59, 59, 0, tm_of_the_day.Location())
			var results []PropertyValue
			err := Db.Where("property_id=? AND updated_at BETWEEN ? AND ?", corresponding_property.ID, tm_of_the_day_begin, tm_of_the_day_end).Find(&results).Error
			if err != nil {
				tmp_dd.Value = "[[0,0]]"
			}
			var xaxis_arr []string
			var yaxis_arr []string
			for _, v := range results {
				xaxis_arr = append(xaxis_arr, fmt.Sprintf("%02d:%02d", v.UpdatedAt.Hour(), v.UpdatedAt.Minute()))
				yaxis_arr = append(yaxis_arr, v.Value)
			}
			cr := ChartResult{
				XAXIS: xaxis_arr,
				YAXIS: yaxis_arr,
				Type:  v.Icon,
			}
			value_byt, _ := json.Marshal(cr)
			tmp_dd.Value = string(value_byt)
		} else {
			tmp_dd.Value = tmp_d_value.Value
		}
		tmp_dd.Size = v.Size
		tmp_dd.Icon = v.Icon
		tmp_dd.Unit = v.Unit
		tmp_dd.PropertyID = tmp_d_value.PropertyID
		dd_list = append(dd_list, tmp_dd)
	}
	bytedata, _ := json.Marshal(dd_list)
	c.String(200, string(bytedata))
	/*properties := strings.Split(device.Property, ",")
	var tmp_device_detail DeviceDetails
	var deviceDetails []DeviceDetails
	for _, v := range properties {
		Db.Where("device_id=? AND name=?", device.ID, v).Last(&tmp_device_detail)
		deviceDetails = append(deviceDetails, tmp_device_detail)
	}
	bytedata, _ := json.Marshal(deviceDetails)
	c.String(200, string(bytedata))*/
}

func hasLogin(c *gin.Context, minlevel uint) bool {
	if len(c.Request.Header["Token"]) == 0 {
		return false
	}
	token := c.Request.Header["Token"][0]
	if token == "" {
		return false
	}
	payload := strings.Split(token, ",")
	var user_s User
	if err := Db.First(&user_s, "username=?", payload[0]).Error; err != nil {
		return false
	}
	token_test_src := user_s.Username + user_s.Password
	token_test := fmt.Sprintf("%x", md5.Sum([]byte(token_test_src)))
	if user_s.Permission < minlevel {
		return false
	}
	return token_test == payload[1]
}
