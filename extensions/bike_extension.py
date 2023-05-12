from socket import *
import threading
import logging
import insw_library
import time
import traceback
import math
import json

bike_whitelist=["GO5M","GO5T"]
bikename_redirect={"GO5M":"Giant OCR 5600","GO5T":"Giant OCR 5500 NG"}
LICENSE="ib2"

class server_listener(threading.Thread):
    def __init__(self,port):
        threading.Thread.__init__(self)
        self.port=port

    def run(self):
        global bike_list
        lis_sock=socket(AF_INET,SOCK_STREAM)
        lis_sock.bind(('127.0.0.1',self.port))
        lis_sock.listen(5)
        logger.info("IOT BIKE服务端推送插件开始监听")
        while True:
            try:
                sock,addr=lis_sock.accept()
                logger.info("服务端推送ACCEPT终端 %s",addr)
                data=sock.recv(8192)
                if data.__len__()==0:
                    sock.close()
                else:
                    jsonObj=json.loads(data)
                    print(jsonObj)
                    devicename=jsonObj["device"]
                    task_property=jsonObj["property"]
                    if task_property.__contains__("频闪"):
                        dev_alias=""
                        for k in bikename_redirect.keys():
                            if devicename==bikename_redirect[k]:
                                dev_alias=k
                                break
                        if dev_alias=="":
                            pass
                        else:
                            for bike in bike_list:
                                if bike.name==dev_alias:
                                    bike.pos_light=int(eval(jsonObj["value"]))
                                    bike.push_message()
                                    break
            except Exception as e:
                logger.error("服务端推送Error with %s,%s,%s.",addr,e,traceback.format_exc())
                continue
            finally:
                sock.close()



class iot_listener(threading.Thread):
    suicide=False
    client:socket=None
    client_addr=()
    def __init__(self,client,addr):
        threading.Thread.__init__(self)
        self.client=client
        self.client_addr=addr

    def run(self):
        global bike_list
        rev_zero_cnt=0
        while True:
            try:
                if self.suicide:
                    break
                time.sleep(0.1)
                data=self.client.recv(8192)
                text=data.decode('utf-8','ignore')
                logger.info("终端%s发送信息%s",self.client_addr,text)
                if text.__len__()==0:
                    logger.error("终端%s发送空信息",self.client_addr)
                    rev_zero_cnt+=1
                    if rev_zero_cnt==10:
                        logger.error("终端%s发送空信息达到10次，结束socket",self.client_addr)
                        break
                    continue
                #TODO:这里处理，可能第一个包不是$，然后加入许可的车辆，比如name只能是Giant OCR 5600，把车的类实时保存到一个文件里
                #TODO: 完善格式检测
                # if text[0]!='$':
                #     continue
                # packet: $bike:gps status:lat:lon:velocity:heading#
                parts=text.split('#')
                if parts.__len__()>1:
                    # localtime=time.localtime(time.time())
                    # reply="&server:time:%d:%02d:%02d:%02d\r\n"%(localtime.tm_wday+1,localtime.tm_hour,localtime.tm_min,localtime.tm_sec)
                    # self.client.send(reply.encode())
                    for te in parts:
                        if te=="":
                            continue
                        if te[0]!='$':
                            continue
                        text=te+'#'
                        print(text)
                        text_arr=text.split(':')
                        if text_arr.__len__()!=12:
                            logger.error("终端%s发送格式不正确信息，缺少信息",self.client_addr)
                            continue
                        for i in text_arr:
                            if i=='':
                                logger.error("终端%s发送格式不正确信息，信息被截断",self.client_addr)
                                continue
                        name=text_arr[0].replace('$','')
                        client_license=text_arr[1]
                        if client_license!=LICENSE:
                            logger.error("终端%s，车辆%s不支持的协议，Rejected.",self.client_addr,name)
                            self.client.close()
                            return
                        in_whitelist=False
                        for bw in bike_whitelist:
                            if bw==name:
                                in_whitelist=True
                        if in_whitelist is False:
                            logger.error("终端%s，车辆%s不在白名单，Rejected.",self.client_addr,name)
                            self.client.close()
                            return
                        has_bike=False
                        for i in bike_list:
                            if i.suicide:
                                bike_list.remove(i)
                        for i in bike_list:
                            if i.name==name:
                                has_bike=True
                                i.client=self.client
                                i.parse(text)
                                i.upload()
                                # logger.info("已从%s上传%s信息",self.client_addr,name)
                                break
                        if not has_bike:
                            logger.info("添加%s终端%s",self.client_addr,name)
                            bike=bike_info(name,self.client,self)
                            bike_list.append(bike)
                            bike.parse(text)
                            bike.upload()
                            bike.start()
                            # logger.info("已从%s上传%s信息",self.client_addr,name)
            except Exception as e:
                logger.error("Error with %s,%s,%s. Shutdown.",self.client_addr,e,traceback.format_exc())
                break
        
        self.client.close()

# ${name}:{license version}:{sim rssi}:{gps status:available}:{satellite number}:{lat}:{lon}:{altitude}:{velocity}:{heading}#
# web显示连接信息
# version:协议 insw.bike.0.2.0
# 返回：里程
# $bike:insw.bike.0.2.0:16:1:20:4013.12913 N:11612.13138 E:20:0.2:136#

"""
返回：
#server:version:refused
#server:mile:reset  重置里程
#server:light:on    开关背光
#server:light:off 
#server:light:auto
#server:time:week:HH:mm:ss
"""
# TODO:新版本的协议ib2，加入电池电压与最大速度的检测，修复上位机速度单位的bug
class bike_info(threading.Thread):

    def __init__(self,name,client:socket,socket_thd:iot_listener) -> None:
        threading.Thread.__init__(self)
        self.name=name
        self.sim_rssi=0
        self.gps_available=False
        self.gps_available_bool=False
        self.gps_satellite=0
        self.latitude=0
        self.longitude=0
        self.altitude=0
        self.velocity=0
        self.heading=0
        self.mile_total=0
        self.average_speed=0
        self.last_lat=0
        self.last_lon=0
        self.pos_available=False
        self.spd_list=[]
        self.client=client
        self.last_update_timestamp=time.time()
        self.socket_thread=socket_thd
        self.suicide=False
        self.data_error=False
        self.pulse_lose=False
        self.HDOP=99.9
        self.displacement=0
        self.last_upload_to_iot=0
        self.last_lat_avg=0
        self.last_lon_avg=0
        self.battery=0
        self.max_speed=0
        self.ride_time=0
        self.last_ride_timestamp=0
        self.pos_light=0

    def distance(self,lat1,lon1,lat2,lon2):
        radius=lat1/180*3.14;
        dx=lat1*111120-lat2*111120;
        dy=(lon1*111120-lon2*111120)*(1-0.5*radius*radius+0.042*radius*radius*radius*radius);
        return math.sqrt(dx*dx+dy*dy);

    def get_gps_evaluate(self,connected,HDOP):
        if connected is False:
            return "LOST"
        else:
            if HDOP>=2:
                return "BAD"
            elif HDOP>=1.15:
                return "NORM"
            elif HDOP>=1.1:
                return "GOOD"
            else:
                return "ACCU"
            

    def parse(self,data):
        self.data_error=False
        self.pulse_lose=True
        self.last_update_timestamp=time.time()
        data=data.replace('#','')
        data_array=data.split(':')
        if(self.name!=data_array[0].replace("$","")):
            return
        try:
            if(data_array[10]!=''):
                HDOP=float(data_array[10])
                self.HDOP=HDOP
            else:
                self.HDOP=99.99
            self.gps_available=self.get_gps_evaluate(data_array[3]=="1",self.HDOP)
            self.gps_available_bool=data_array[3]=="1"
            self.gps_satellite=data_array[4]
            self.battery=data_array[11]
            self.sim_rssi=data_array[2]
            if data_array[3]=="1" and int(self.gps_satellite)>7:
                    self.latitude="%.7lf"%((float(data_array[5][0:2])+float(data_array[5][2:10])/60)*(1 if data_array[5][10]=='N' else -1))
                    self.longitude="%.7lf"%((float(data_array[6][0:3])+float(data_array[6][3:11])/60)*(1 if data_array[6][11]=='E' else -1))
                    if(data_array[7]!=''):
                        self.altitude="%.1f"%(float(data_array[7]))
                    self.velocity="%.1f"%(float(data_array[8])*1.852)
                    self.heading="%d"%(int(float(data_array[9])))
                    # self.pos_available=True
                    if self.pos_available is False:
                        self.last_lat=self.latitude
                        self.last_lon=self.longitude
                        self.pos_available=True
                        return
            else:
                self.latitude=""
                self.longitude=""
                self.altitude=""
                self.velocity=""
                self.heading=""
                self.pos_available=False
                self.displacement=0
        except Exception as e:
            self.data_error=True
            logger.error("%s 数据解码错误，%s，%s",self.name,e,traceback.format_exc())
            return
        if self.pos_available and self.HDOP<=3 and int(self.gps_satellite)>7:
            if float(self.velocity)>0.3:
                self.spd_list.append(float(self.velocity))
                # in meters
                self.displacement=self.distance(float(self.latitude),float(self.longitude),\
                                           float(self.last_lat),float(self.last_lon))
                self.mile_total+=(self.displacement/1000)
                if self.last_ride_timestamp>0:
                    self.ride_time+=(time.time()-self.last_ride_timestamp)
                self.last_ride_timestamp=time.time()
            else:
                self.displacement=0
                self.last_ride_timestamp=0

            if float(self.velocity)>self.max_speed:
                self.max_speed=float(self.velocity)

            self.last_lat=self.latitude
            self.last_lon=self.longitude
            
            n_len=self.spd_list.__len__()
            self.average_speed=0
            for i in self.spd_list:
                self.average_speed+=(1/n_len*i)
            logger.info("Bike:%s MILE:%.2f km  GS_AVERAGE:%.2f km/h"%(self.name,self.mile_total,self.average_speed))
        # self.mile_total='0'
        # self.average_speed='0'

    def upload(self):
        # already in string
        try:
            if self.data_error:
                logger.error("%s数据有误，拒绝上传",self.name)
                return
            value_list=[]
            if self.gps_available!="LOST":
                value_list=[self.sim_rssi,self.gps_available,self.battery,int(111.1*(float(self.battery)-3.3)),self.gps_satellite,\
                            self.altitude,self.velocity,self.heading,"%.2f"%self.mile_total,\
                                "%.2f"%self.average_speed,'%.2f'%self.max_speed,'%.2f'%(self.ride_time/60/60),'0','0','0','0',"[%s,%s]"%(self.latitude,self.longitude)]
            else:
                # value_list=[self.sim_rssi,self.gps_available,self.gps_satellite,\
                #             '','','','','','','','','','',""]
                value_list=[self.sim_rssi,self.gps_available,self.battery,int(111.1*(float(self.battery)-3.3)),self.gps_satellite,\
                            '','','','',\
                                '','','','','','','',""]
            time_delta=time.time()-self.last_upload_to_iot
            if self.gps_available_bool is False:
                if time_delta<45:
                    return
                
            if self.velocity=='':
                if time_delta<45:
                    return
            elif float(self.velocity)>=0.3 and int(self.gps_satellite)>7:
                self.last_lat_avg=0
                self.last_lon_avg=0
            elif float(self.velocity)<0.3:
                # 一阶滤波
                K=0.0001
                if self.last_lat_avg==0 or self.last_lon_avg==0:
                    self.last_lat_avg=float(self.latitude)
                    self.last_lon_avg=float(self.longitude)
                else:
                    tmp_lat=float(self.latitude)*K+float(self.last_lat_avg)*(1-K)
                    tmp_lon=float(self.longitude)*K+float(self.last_lon_avg)*(1-K)
                    self.last_lat_avg=tmp_lat
                    self.last_lon_avg=tmp_lon
                    if value_list.__len__()==14:
                        value_list[13]="[%s,%s]"%(tmp_lat,tmp_lon)
                if time_delta<45:
                    return
            print("Displacement: ",self.displacement)
            print("Dt: ",time_delta)
            logger.info("已上传%s数据，服务器响应%s",self.name,insw_library.device_update(bikename_redirect[self.name],value_list))
            self.last_upload_to_iot=time.time()
            self.push_message()
        except Exception as e:
            logger.error("%s 数据解码错误，%s，%s",self.name,e,traceback.format_exc())

    def push_message(self):
        localtime=time.localtime(time.time())
        reply="&server:%d%d%d%d:%d:%02d:%02d:%02d\r\n"%(self.pos_light,(int(self.mile_total)/10)%10,int(self.mile_total)%10,int((float(self.mile_total)-int(self.mile_total))*10),localtime.tm_wday+1,localtime.tm_hour,localtime.tm_min,localtime.tm_sec)
        self.client.send(reply.encode())

    def run(self):
        heart_counter=0
        while True:
            time.sleep(1)
            now_ts=time.time()
            heart_counter+=1
            if heart_counter==20:
                heart_counter=0
                try:
                    self.push_message()
                except:
                    pass
            if now_ts-self.last_update_timestamp>120 and self.pulse_lose is False:
                # self.client.shutdown(2)
                # self.client.close()
                # self.socket_thread.suicide=True
                # self.suicide=True
                logger.error("终端%s 失去心跳数据包，Shutdown",self.name)
                self.pulse_lose=True
                
                # return

bike_list=[]

insw_library.init()
host='0.0.0.0'
port=9812
addr=(host,port)

logger = logging.getLogger(__name__)
logger.setLevel(level = logging.INFO)
handler = logging.FileHandler("log.txt")
handler.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
console = logging.StreamHandler()
console.setLevel(logging.INFO)
logger.addHandler(handler)
logger.addHandler(console)

server=socket(AF_INET,SOCK_STREAM)
server.bind(addr)
server.listen(5)
logger.info("IOT BIKE插件开始监听 %s",addr)

sl=server_listener(port+1)
sl.start()

while True:
    sock,addr=server.accept()
    logger.info("ACCEPT终端 %s",addr)
    lis=iot_listener(sock,addr)
    lis.start()
