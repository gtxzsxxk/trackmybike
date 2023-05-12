/*
 * sim800l.c
 *
 *  Created on: 2023年4月16日
 *      Author: hanyuan
 */
#include "sim800l.h"

struct SIM800L_DATA_SUMMARY sim800l_data_summary;

const uint8_t sim_cmd_ate0[]="ATE0\r\n";
const uint8_t sim_cmd_csq[]="at+csq\r\n";
const uint8_t sim_cmd_reg[]="at+creg?\r\n";
const uint8_t sim_cmd_cgatt[]="at+cgatt?\r\n";
const uint8_t sim_cmd_apn[]="AT+CSTT=\"CMNET\"\r\n";
const uint8_t sim_cmd_ciicr[]="AT+CIICR\r\n";
const uint8_t sim_cmd_cifsr[]="AT+CIFSR\r\n";
const uint8_t sim_cmd_tcp[]="AT+CIPSTART=\"tcp\",\"";
const uint8_t sim_cmd_tcp_2[]="\",9812\r\n";
const uint8_t sim_cmd_send[]="AT+CIPSEND\r\n";
const uint8_t sim_cmd_reset[]="AT+CFUN=1,1\r\n";
const uint8_t sim_cmd_send_stop[2]={0x1a,0};

void sim800l_write(const uint8_t *cmd){
	HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen((char*)cmd), 1000);
}

void sim800l_delay(uint32_t ms){
	if(sim800l_data_summary.osReady){
		osDelay(pdMS_TO_TICKS(ms));
	}else{
		HAL_Delay(ms);
	}
}

void sim800l_uart_receive_sync(uint32_t size,uint32_t wait_ms){
	memset(sim800l_data_summary.rev_sync_buffer,0,sizeof(sim800l_data_summary.rev_sync_buffer));
	sim800l_data_summary.rev_sync_cnt=0;
	sim800l_data_summary.rev_sync_mode=1;
	uint32_t wait_timeout=0;
	uint32_t TMOUT=wait_ms/100;
	while(sim800l_data_summary.rev_sync_cnt<size){
		sim800l_delay(100);
		wait_timeout++;
		if(wait_timeout>=TMOUT){
			sim800l_data_summary.rev_sync_mode=0;
			break;
		}
	}
	sim800l_data_summary.rev_sync_mode=0;

//	memset(u2_normal_buffer,0,sizeof(u2_normal_buffer));
}

uint8_t sim800l_try_connect(uint16_t wait_ms){
	sim800l_write(sim_cmd_tcp);
	sim800l_write(SERVER);
	sim800l_write(sim_cmd_tcp_2);
	sim800l_uart_receive_sync(20,wait_ms);
	for(uint8_t i=0;i<20;i++){
		if(sim800l_data_summary.rev_sync_buffer[i]=='C'&&
				sim800l_data_summary.rev_sync_buffer[i+1]=='O'&&
				sim800l_data_summary.rev_sync_buffer[i+2]=='N'){
			if(sim800l_data_summary.rev_sync_buffer[i+8]=='O'&&
					sim800l_data_summary.rev_sync_buffer[i+9]=='K'){
				return 1;
			}
		}
		if(sim800l_data_summary.rev_sync_buffer[i]=='A'&&
				sim800l_data_summary.rev_sync_buffer[i+1]=='L'&&
				sim800l_data_summary.rev_sync_buffer[i+2]=='R'){
			return 1;
		}
	}
	return 0;
}

uint8_t sim800l_check_connection(uint16_t wait_ms){
	memset(sim800l_data_summary.rev_sync_buffer,0,sizeof(sim800l_data_summary.rev_sync_buffer));
	sim800l_write((uint8_t*)"AT+CIPSTATUS\r\n");
	sim800l_uart_receive_sync(50,wait_ms);
	if(strlen((char*)sim800l_data_summary.rev_sync_buffer)>10){
		uint8_t has_state=0;
		for(uint8_t i=0;i<50;i++){
			//CONNECT OK
			if(sim800l_data_summary.rev_sync_buffer[i]=='C'&&
					sim800l_data_summary.rev_sync_buffer[i+1]=='O'&&
					sim800l_data_summary.rev_sync_buffer[i+2]=='N'){
				if(sim800l_data_summary.rev_sync_buffer[i+8]=='O'){
					sim800l_data_summary.connection_loss_flag=0;
					return 1;
				}
			}
			if(sim800l_data_summary.rev_sync_buffer[i]=='S'&&
					sim800l_data_summary.rev_sync_buffer[i+1]=='T'&&
					sim800l_data_summary.rev_sync_buffer[i+2]=='A'&&
					sim800l_data_summary.rev_sync_buffer[i+3]=='T'){
				has_state=1;
			}
			if(sim800l_data_summary.rev_sync_buffer[i]=='I'&&
					sim800l_data_summary.rev_sync_buffer[i+1]=='P'&&
					sim800l_data_summary.rev_sync_buffer[i+2]==' '&&
					sim800l_data_summary.rev_sync_buffer[i+3]=='I'){
				sim800l_data_summary.connection_loss_flag=1;
				return 0;
			}
			if(sim800l_data_summary.rev_sync_buffer[i]=='C'&&
					sim800l_data_summary.rev_sync_buffer[i+1]=='L'&&
					sim800l_data_summary.rev_sync_buffer[i+2]=='O'&&
					sim800l_data_summary.rev_sync_buffer[i+3]=='S'){
				sim800l_data_summary.connection_loss_flag=1;
				return 0;
			}
		}
		if(!has_state){
			sim800l_data_summary.connection_loss_flag=0;
			return 1;
		}
		sim800l_data_summary.connection_loss_flag=1;
		return 0;
	}else{
		sim800l_data_summary.connection_loss_flag=0;
		return 1;
	}
}

void sim800l_wait_until_ok(uint32_t timeout){
	sim800l_data_summary.wait_until_ok_flag=0;
	sim800l_data_summary.wait_until_ok_mode=1;
	uint32_t TMOUT=timeout/100;
	uint32_t tmoutcnt=0;
	while(sim800l_data_summary.wait_until_ok_flag==0&&(tmoutcnt<TMOUT)){
		sim800l_delay(100);
		tmoutcnt++;
	}
	sim800l_data_summary.wait_until_ok_flag=0;
}

void sim800l_wait_until_revmsg(uint32_t timeout){
	sim800l_data_summary.wait_until_ok_flag=0;
	sim800l_data_summary.wait_until_ok_mode=1;
	uint32_t TMOUT=timeout/100;
	uint32_t tmoutcnt=0;
	while(sim800l_data_summary.wait_until_ok_flag==0&&(tmoutcnt<TMOUT)&&sim800l_data_summary.server_rxne==0){
		sim800l_delay(100);
		tmoutcnt++;
	}
	sim800l_data_summary.server_rxne=0;
	sim800l_data_summary.wait_until_ok_flag=0;
}

void sim800l_noecho(){
	sim800l_write(sim_cmd_ate0);
	sim800l_wait_until_ok(100);
}

void sim800l_reset(){
	sim800l_write(sim_cmd_reset);
	sim800l_delay(500);
}

void sim800l_read_rssi(){
	sim800l_write(sim_cmd_csq);
	sim800l_uart_receive_sync(12, 200);
	for(uint8_t i=0;i<12;i++){
		if(sim800l_data_summary.rev_sync_buffer[i]=='C'&&
				sim800l_data_summary.rev_sync_buffer[i+1]=='S'&&
				sim800l_data_summary.rev_sync_buffer[i+2]=='Q'){
			sim800l_data_summary.rssi=(sim800l_data_summary.rev_sync_buffer[i+5]-'0')*10+
					sim800l_data_summary.rev_sync_buffer[i+6]-'0';
			break;
		}
	}
	if(sim800l_data_summary.rssi<16){
		sim800l_data_summary.signal_level=1;
	}else if(sim800l_data_summary.rssi>=16&&sim800l_data_summary.rssi<20){
		sim800l_data_summary.signal_level=2;
	}else{
		sim800l_data_summary.signal_level=3;
	}
}

void sim800l_init(){
	sim800l_data_summary.net_week=4;
	sim800l_data_summary.net_hour=19;
	sim800l_data_summary.net_min=19;
	sim800l_data_summary.net_sec=18;
	sim800l_data_summary.connection_loss_flag=0;
	GUI_BOOT_REGISTER_NET();
	sim800l_noecho();
	sim800l_reset();
	sim800l_delay(1000);
	sim800l_noecho();
register_retry:
	sim800l_noecho();
	sim800l_delay(1000);
	sim800l_write(sim_cmd_reg);
	sim800l_uart_receive_sync(sizeof(sim800l_data_summary.rev_sync_buffer), 1000);
	uint8_t reg_len=strlen((char*)sim800l_data_summary.rev_sync_buffer);
	if(reg_len<5){
		goto register_retry;
	}
	for(uint8_t i=0;i<reg_len;i++){
		if(sim800l_data_summary.rev_sync_buffer[i]=='C'&&
				sim800l_data_summary.rev_sync_buffer[i+1]=='R'&&
				sim800l_data_summary.rev_sync_buffer[i+2]=='E'){
			if(sim800l_data_summary.rev_sync_buffer[i+8]=='1'){
				break;
			}else{
				HAL_Delay(1000);
				goto register_retry;
			}

		}
	}
	sim800l_noecho();
	sim800l_delay(3000);
	sim800l_read_rssi();
	//Begin with TCP
	GUI_BOOT_TCP_CONNECT();
	sim800l_write(sim_cmd_cgatt);
	sim800l_wait_until_ok(500);
	sim800l_write(sim_cmd_apn);
	sim800l_wait_until_ok(500);
	sim800l_write(sim_cmd_ciicr);
	sim800l_wait_until_ok(45*1000);
	sim800l_write(sim_cmd_cifsr);
	sim800l_wait_until_ok(500);
	//AT+CIPQSEND=1
	if(!sim800l_try_connect(8000)){
		GUI_BOOT_TCP_FAIL();
		sim800l_delay(3000);
		HAL_NVIC_SystemReset();
	}
	sim800l_write("AT+CIPQSEND=1\r\n");
	sim800l_wait_until_ok(500);
	sim800l_delay(500);
	LCD_Clean();
}

void sim800l_init_quick(){
	sim800l_noecho();
//	sim800l_write(sim_cmd_cgatt);
//	sim800l_wait_until_ok(500);
//	sim800l_write(sim_cmd_apn);
//	sim800l_wait_until_ok(500);
//	sim800l_write(sim_cmd_ciicr);
//	sim800l_wait_until_ok(45*1000);
	sim800l_delay(1500);
	sim800l_write(sim_cmd_cifsr);
	sim800l_wait_until_ok(500);
	sim800l_delay(500);
	if(!sim800l_try_connect(8000)){
		GUI_BOOT_TCP_FAIL();
		sim800l_delay(3000);
		HAL_NVIC_SystemReset();
	}
}

int str2int_2(uint8_t* begin){
	return (begin[0]-'0')*10+begin[1]-'0';
}

void sim800l_server_cmd_resolve(){
	//&server:1007:4:17:22:23
	if(strlen((char*)sim800l_data_summary.rev_cmd_buffer)<23){
		return;
	}
	if(!(sim800l_data_summary.rev_cmd_buffer[1]=='s'&&
			sim800l_data_summary.rev_cmd_buffer[6]=='r'&&
			sim800l_data_summary.rev_cmd_buffer[7]==':'&&
			sim800l_data_summary.rev_cmd_buffer[8]>='0'&&sim800l_data_summary.rev_cmd_buffer[8]<='9'&&
			sim800l_data_summary.rev_cmd_buffer[9]>='0'&&sim800l_data_summary.rev_cmd_buffer[9]<='9'&&
			sim800l_data_summary.rev_cmd_buffer[10]>='0'&&sim800l_data_summary.rev_cmd_buffer[10]<='9'&&
			sim800l_data_summary.rev_cmd_buffer[11]>='0'&&sim800l_data_summary.rev_cmd_buffer[11]<='9'&&
			sim800l_data_summary.rev_cmd_buffer[12]==':'&&
			sim800l_data_summary.rev_cmd_buffer[14]==':'&&
			sim800l_data_summary.rev_cmd_buffer[17]==':'&&
			sim800l_data_summary.rev_cmd_buffer[20]==':')){
		return;
	}
	sim800l_data_summary.net_aux[0]=sim800l_data_summary.rev_cmd_buffer[8]-'0';
	sim800l_data_summary.net_aux[1]=sim800l_data_summary.rev_cmd_buffer[9]-'0';
	sim800l_data_summary.net_aux[2]=sim800l_data_summary.rev_cmd_buffer[10]-'0';
	sim800l_data_summary.net_aux[3]=sim800l_data_summary.rev_cmd_buffer[11]-'0';
	sim800l_data_summary.net_week=sim800l_data_summary.rev_cmd_buffer[13]-'0';
	sim800l_data_summary.net_hour=str2int_2(sim800l_data_summary.rev_cmd_buffer+15);
	sim800l_data_summary.net_min=str2int_2(sim800l_data_summary.rev_cmd_buffer+18);
	sim800l_data_summary.net_sec=str2int_2(sim800l_data_summary.rev_cmd_buffer+21);
	sim800l_data_summary.server_rxne=1;
}

void sim800l_send(uint8_t* what){
	sim800l_write(sim_cmd_send);
	sim800l_uart_receive_sync(4, 7000);
	for(uint8_t i=0;i<4;i++){
		if(sim800l_data_summary.rev_sync_buffer[i]=='>'){
			break;
		}
	}
	sim800l_write(what);
	sim800l_write(sim_cmd_send_stop);
	sim800l_data_summary.server_rxne=0;
}

void sim800l_send_queue(){
	if(gps_data_summary.queue_tail==0){
		return;
	}
	sim800l_write(sim_cmd_send);
	sim800l_uart_receive_sync(4, 7000);
	for(uint8_t i=0;i<4;i++){
		if(sim800l_data_summary.rev_sync_buffer[i]=='>'){
			break;
		}
	}
	for(uint8_t i=0;i<gps_data_summary.queue_tail;i++){
		sim800l_write(gps_msg_queue[i]);
	}
	sim800l_write(sim_cmd_send_stop);
	gps_data_summary.queue_tail=0;
	sim800l_data_summary.server_rxne=0;
//	u2_it_mode=1;
}
