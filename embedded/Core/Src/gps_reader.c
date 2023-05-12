#include "gps_reader.h"

struct GPS_DATA_SUMMARY gps_data_summary;
uint8_t gps_msg_queue[queue_len][queue_data_len]={0};

void find_by_segment(uint8_t number,uint8_t* source,uint8_t len,uint8_t* dest){
	uint8_t cnt=0;
	for(uint16_t i=0;i<len;i++){
		if(source[i]==','){
			cnt++;
			if(cnt==number){
				cnt=i;
				break;
			}
		}
	}
	if(cnt==0){
		return;
	}
	uint8_t* tmp=source+cnt+1;
	cnt=0;
	while(*tmp!=','&&tmp<source+len){
		dest[cnt++]=*tmp;
		tmp++;
	}
}

uint8_t number_check(uint8_t* str){
	uint8_t len=strlen((char*)str);
	uint8_t check_len=len;
	uint8_t num=0;
	uint8_t dot=0;
	uint8_t i=0;
	if(len==0){
		return 0;
	}
	if(str[0]=='-'){
		i=1;
		check_len--;
	}
	for(;i<len;i++){
		if(str[i]>='0'&&str[i]<='9'){
			num++;
		}
		if(str[i]=='.'){
			if(i==0||i==len-1){
				return 0;
			}else{
				dot++;
			}
		}
	}
	if(dot>1){
		return 0;
	}
	return (num+dot)==check_len;
}

double str2double(uint8_t *str){
	uint8_t len=strlen((char*)str);
	double value=0;
	double after_point_value=0;
	uint8_t point_flag=0;
	uint8_t point_cnt=0;
	uint8_t i=0;
	if(str[0]=='-'){
		i=1;
	}
	for(;i<len;i++){
		if(str[i]=='.'){
			point_flag=1;
			continue;
		}
		if(!point_flag){
			value*=10;
			value+=(str[i]-'0');
		}else{
			after_point_value*=10;
			after_point_value+=(str[i]-'0');
			point_cnt++;
		}

	}
	for(uint8_t i=0;i<point_cnt;i++){
		after_point_value/=10;
	}
	if(str[0]=='-'){
		return -(value+after_point_value);
	}
	return value+after_point_value;
}

uint8_t info_fill(uint8_t last,uint8_t* src,const uint8_t* data){
	uint8_t temp=strlen((char*)data);
	memcpy(src+last+1,data,temp);
	src[last+1+temp]=':';
	return last+1+temp;
}

double my_abs(double x){
	return x>=0?x:-x;
}

double square_root(double a){
	double x_l=a;
	double x_n=0;
	if(a==0){
		return a;
	}
	while(1){
		x_n=0.5*(x_l+a/x_l);
		if(my_abs(x_n-x_l)<0.01){
			return x_n;
		}
		x_l=x_n;
	}
}

double get_distance_meter(double lat1,double lon1,double lat2,double lon2){
	double radius=lat1/180*3.14;
	double dx=lat1*111120-lat2*111120;
	double dy=(lon1*111120-lon2*111120)*(1-0.5*radius*radius+0.042*radius*radius*radius*radius);
	return square_root(dx*dx+dy*dy);
}

uint8_t gps_count_dots(uint8_t* buffer,uint8_t len){
	uint8_t dots=0;
	for(uint8_t i=0;i<len;i++){
		if(buffer[i]==','){
			dots++;
		}
	}
	return dots;
}

uint8_t gps_time_str[15];
uint8_t latitude_str[20]="4013.12913";
uint8_t longitude_str[20]="11612.13138";
uint8_t lat_dir_str[2]="N";
uint8_t lon_dir_str[2]="E";
uint8_t velocity_str[7]="0.002";
uint8_t heading_str[7]="169.88";
uint8_t satellite_str[]="00";
uint8_t altitude_str[8]="-1112.5";
uint8_t HDOP_str[8]="99.99";
double HDOP=99.99;

void gps_msg_read(uint8_t* buffer){
	uint8_t valid=0;
	if(!gps_data_summary.os_ready){
		return;
	}
	if(buffer[3]=='R'&&buffer[4]=='M'&&buffer[5]=='C'){
		uint8_t buffer_len=strlen((char*)buffer);
		//此处进hardfault
		find_by_segment(2, buffer,buffer_len, &valid);
		if(valid!='A'){
			gps_data_summary.pos_available=0;
			gps_data_summary.connected=0;
			return;
		}else{
			gps_data_summary.connected=1;
		}
		//make sure there are 12 dots
		if(gps_count_dots(buffer,buffer_len)<12){
			return;
		}
		memset(gps_time_str,0,sizeof(gps_time_str));
		memset(latitude_str,0,sizeof(latitude_str));
		memset(longitude_str,0,sizeof(longitude_str));
		memset(lat_dir_str,0,sizeof(lat_dir_str));
		memset(lon_dir_str,0,sizeof(lon_dir_str));
		memset(velocity_str,0,sizeof(velocity_str));
		memset(heading_str,0,sizeof(heading_str));
		find_by_segment(1,buffer,buffer_len,gps_time_str);
		find_by_segment(3,buffer,buffer_len,latitude_str);
		if(!number_check(latitude_str)) return;
		find_by_segment(4,buffer,buffer_len,lat_dir_str);
		find_by_segment(5,buffer,buffer_len,longitude_str);
		if(!number_check(longitude_str)) return;
		find_by_segment(6,buffer,buffer_len,lon_dir_str);

		find_by_segment(7,buffer,buffer_len,velocity_str);
		if(!number_check(velocity_str)) return;
		gps_data_summary.velocity=str2double(velocity_str);
		double velocity_kmh=gps_data_summary.velocity*1.852;
		gps_data_summary.velocity_kmh_int=(uint8_t)velocity_kmh;
		if(gps_data_summary.velocity<0.6){
			gps_data_summary.parked_mode=1;
		}else{
			gps_data_summary.parked_mode=0;
			if(HDOP<=1.25){
				if(gps_data_summary.pos_available){
//					uint32_t now_tick=HAL_GetTick();
//					double distance=velocity_kmh/3.6*(now_tick-last_gps_tick)/1000;
//					last_gps_tick=now_tick;
//					miles+=distance;
					//Read from the server
				}
			}
		}
		if(!gps_data_summary.pos_available){
			gps_data_summary.pos_available=1;
		}

		find_by_segment(8,buffer,buffer_len,heading_str);
		if(!number_check(heading_str)) return;
		gps_data_summary.heading=str2double(heading_str);
		gps_data_summary.heading_int=(uint16_t)gps_data_summary.heading;

		if(!gps_data_summary.parked_mode){
			gps_generate_data_queue();
		}
//		uint32_t test_2=HAL_GetTick();
//		uint32_t delta=test_2-test_1;
//		delta+=1;
	}
	else if(buffer[3]=='G'&&buffer[4]=='G'&&buffer[5]=='A'){
		if(!gps_data_summary.connected){
			return;
		}
		//make sure there are 14 dots
		uint8_t buffer_len=strlen((char*)buffer);
		if(gps_count_dots(buffer,buffer_len)<14){
			return;
		}
		memset(satellite_str,0,sizeof(satellite_str));
		memset(altitude_str,0,sizeof(altitude_str));
		memset(HDOP_str,0,sizeof(HDOP_str));

		find_by_segment(7, buffer,buffer_len, satellite_str);
		if(!number_check(satellite_str)) return;
		gps_data_summary.satellite=(satellite_str[0]-'0')*10+satellite_str[1]-'0';
		if(!gps_data_summary.connected){
			gps_data_summary.signal_level=0;
		}else{
			if(gps_data_summary.satellite<10){
				gps_data_summary.signal_level=1;
			}else if(gps_data_summary.satellite>=10&&gps_data_summary.satellite<20){
				gps_data_summary.signal_level=2;
			}else{
				gps_data_summary.signal_level=3;
			}
		}

		find_by_segment(8, buffer,buffer_len, HDOP_str);//A*4830
		//$GNGGA,091442.000,4000.53562,N,11619.16754,E,1,13,2.26,53.8,M,-8.4,M,,*5F
		if(!number_check(HDOP_str)){
			HDOP_str[0]='9';
			HDOP_str[1]='9';
			HDOP_str[2]='.';
			HDOP_str[3]='9';
		}
		HDOP=str2double(HDOP_str);

		find_by_segment(9, buffer,buffer_len, altitude_str);// 直接为空
		if(!number_check(altitude_str)){
			altitude_str[0]='0';
			altitude_str[1]='.';
			altitude_str[2]='0';
		}
	}
}

//$GO5M:ib2:18:1:21:4000.53191N:11619.17746E:89.7:0.000:219.03:0.67:3.72#
void gps_generate_data_queue(){
	if(gps_data_summary.queue_tail>=queue_len){
		//队列已满，除非发送，否则拒绝添加
		return;
	}
	uint8_t *message=gps_msg_queue[gps_data_summary.queue_tail++];
	uint8_t temp=0;
	memset(message,0,queue_data_len);
	message[0]='$';
	temp=info_fill(temp,message,BIKENAME);
	temp=info_fill(temp,message,SERVER_LICENSE);
	uint8_t rssi_str[3]={0};
	rssi_str[0]='0'+sim800l_data_summary.rssi/10;
	rssi_str[1]='0'+sim800l_data_summary.rssi%10;
	temp=info_fill(temp,message,rssi_str);
	uint8_t ok[]="1";
	uint8_t bad[]="0";
	if(gps_data_summary.connected){
		temp=info_fill(temp,message,ok);
	}else{
		temp=info_fill(temp,message,bad);
	}
	rssi_str[0]='0'+gps_data_summary.satellite/10;
	rssi_str[1]='0'+gps_data_summary.satellite%10;
	temp=info_fill(temp,message,rssi_str);
	uint8_t tempstr[14]={0};
	memset(tempstr,0,sizeof(tempstr));
	memcpy(tempstr,latitude_str,10);
	tempstr[10]='N';
	temp=info_fill(temp,message,tempstr);
	memset(tempstr,0,sizeof(tempstr));
	memcpy(tempstr,longitude_str,11);
	tempstr[11]='E';
	temp=info_fill(temp,message,tempstr);
	temp=info_fill(temp,message,altitude_str);
	temp=info_fill(temp,message,velocity_str);
	temp=info_fill(temp,message,heading_str);
	temp=info_fill(temp,message,HDOP_str);
	temp=info_fill(temp,message,battery_voltage_str);
	if(temp>=queue_data_len){
		gps_data_summary.queue_tail--;
		return;
	}
	message[temp]='#';
}

uint8_t gps_pulse_message[queue_data_len];

void gps_generate_null(){
	uint8_t *message=gps_pulse_message;
	uint8_t temp=0;
	memset(message,0,queue_data_len);
	message[0]='$';
	temp=info_fill(temp,message,BIKENAME);
	temp=info_fill(temp,message,SERVER_LICENSE);
	uint8_t rssi_str[3]={0};
	rssi_str[0]='0'+sim800l_data_summary.rssi/10;
	rssi_str[1]='0'+sim800l_data_summary.rssi%10;
	temp=info_fill(temp,message,rssi_str);
	uint8_t ok[]="1";
	uint8_t bad[]="0";
	if(gps_data_summary.connected){
		temp=info_fill(temp,message,ok);
	}else{
		temp=info_fill(temp,message,bad);
	}
	rssi_str[0]='0'+gps_data_summary.satellite/10;
	rssi_str[1]='0'+gps_data_summary.satellite%10;
	temp=info_fill(temp,message,rssi_str);
	uint8_t tempstr[14]={0};
	memset(tempstr,0,sizeof(tempstr));
	memcpy(tempstr,latitude_str,10);
	tempstr[10]='N';
	temp=info_fill(temp,message,tempstr);
	memset(tempstr,0,sizeof(tempstr));
	memcpy(tempstr,longitude_str,11);
	tempstr[11]='E';
	temp=info_fill(temp,message,tempstr);
	temp=info_fill(temp,message,altitude_str);
	temp=info_fill(temp,message,velocity_str);
	temp=info_fill(temp,message,heading_str);
	temp=info_fill(temp,message,HDOP_str);
	temp=info_fill(temp,message,battery_voltage_str);
	message[temp]='#';
}
