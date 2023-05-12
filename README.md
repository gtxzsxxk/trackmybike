# Intelli-sw 设备数据交换平台框架

## 简介
支持多类设备接入该框架，提供数据显示，且对用户进行分级处理，不同级别的用户拥有不同的浏览、管理权限。支持显示、控制、脚本、表格、地图。

## 设备属性：
#### 类型type
- display：用于显示value。图标由icon决定，单位由unit控制，若设置为小块，则在size中写入single。若为大块，置空size
- control：用来显示设备状态并提供控制功能。若要实现控制功能，则需要服务器与设备建立长连接，保证双向数据通信。icon、size在此处将不会生效。对于unit：
    - toggle：开关类设备。value应为boolean类型，使用string存储
    - range(a,b)：值类设备。value为[a,b]中的值，使用string存储
    - exec：扳机类设备。value无意义。激活exec后，在设备上对应事件将被执行一次。
- script：用于在server端执行其它脚本。value对应该脚本的激活状态。unit应为toggle
- chart：unit为y轴单位。value:{xAxis:[...],yAxis:[xxx],type:'line...'}。存储value需要stringify
- map：存储地图数据。value:[[x1,y1],[x2,y2],...]。存储value需要stringify

## 用户权限说明：
- level-0：仅浏览数据
- level-1：允许上传数据，不允许编辑设备
- level-2：允许上传数据，允许编辑设备

## 如何运行这个项目
- 首先`go mod tidy`，然后`nohup go run . &`，把网站服务跑起来，为了维护用户，你需要手动进入database.db添加用户并维护它的密码，密码存储是md5加密过的密文。
- 接着进入`extension`目录，这里存放着网站服务的一些拓展。这些拓展与网站服务通信需要使用到xxtea加密。为了完成加密，你需要配置`config.json`，更改你的上传用户信息。上传用户需要具有一定的权限，密钥是md5加密过的密文。配置好`config.json`后，执行`nohup python3 bike_extension.py &`，再观察log，确保拓展正常跑起来了。
- 确保单片机的程序已经烧录。外形文件存放在`model`目录。PCB以及固件都存放在`embedded`目录下。进入`embedded`目录，进入main.c，修改单片机固件配置好的写死的终端名称、终端协议与服务器地址。

## 关于trackbike的说明
trackbike是intelli-sw网站服务的一个拓展，本项目以trackbike命名，但是其本身集成了intelli-sw网站服务。不过在没加上trackbike拓展前，该网站服务不具备物联网上位功能。在拓展的支持下网站服务才能跑得起来。