import json
import xxtea
import base64
import requests

INTERNAL_SERVER_USERNAME=""
INTERNAL_SERVER_PASSWORD=""  
API_URL=""
def init():
    global INTERNAL_SERVER_USERNAME,INTERNAL_SERVER_PASSWORD,API_URL
    fp=open("config.json","r")
    data=fp.read()
    fp.close()
    jsonObj=json.loads(data)
    INTERNAL_SERVER_USERNAME=jsonObj["username"]
    INTERNAL_SERVER_PASSWORD=jsonObj["password"]
    API_URL=jsonObj["url"]

def device_update(device_name,valuelist):
    tosend_data={
        "value":"",
        "username":INTERNAL_SERVER_USERNAME,
        "signature":""
    }
    tmp_list=[]
    for i in valuelist:
        tmp_list.append('"'+str(i)+'"')
    value_str=json.dumps(tmp_list)
    tosend_data["value"]=value_str
    if(value_str.__contains__("[,]")):
        return "[,] detected. Refused to upload."
    src_bytes=xxtea.encrypt(tosend_data["value"],INTERNAL_SERVER_PASSWORD[0:16])
    tosend_data["signature"]=base64.b64encode(src_bytes).decode()
    r=requests.post(API_URL+device_name,tosend_data)
    return r.text