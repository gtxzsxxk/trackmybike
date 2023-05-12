/*
 * sim800l.h
 *
 *  Created on: 2023年4月16日
 *      Author: hanyuan
 */

#ifndef INC_SIM800L_H_
#define INC_SIM800L_H_
#include "main.h"

struct SIM800L_DATA_SUMMARY{
	uint8_t osReady;
	uint8_t connection_loss_flag;
	uint8_t rssi;
	uint8_t signal_level;
	uint8_t net_hour;
	uint8_t net_min;
	uint8_t net_sec;
	uint8_t net_week;
	uint8_t server_rxne;
	uint8_t wait_until_ok_mode;
	uint8_t wait_until_ok_flag;
	uint8_t rev_sync_mode;
	uint8_t rev_sync_buffer[64];
	uint16_t rev_sync_cnt;
	uint8_t rev_cmd_buffer[64];
	uint16_t rev_cmd_cnt;
	uint8_t net_aux[4];
};
extern struct SIM800L_DATA_SUMMARY sim800l_data_summary;
void sim800l_server_cmd_resolve();
void sim800l_init();
void sim800l_send(uint8_t* what);
void sim800l_send_queue();
void sim800l_noecho();
void sim800l_read_rssi();
void sim800l_wait_until_revmsg(uint32_t timeout);
uint8_t sim800l_check_connection(uint16_t wait_ms);
void sim800l_init_quick();
#endif /* INC_SIM800L_H_ */
