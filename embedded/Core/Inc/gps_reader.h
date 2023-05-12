/*
 * gps_reader.h
 *
 *  Created on: 2023年4月16日
 *      Author: hanyuan
 */

#ifndef INC_GPS_READER_H_
#define INC_GPS_READER_H_
#include "main.h"
#define queue_len 25
#define queue_data_len 78
struct GPS_DATA_SUMMARY{
	uint8_t os_ready;
	uint8_t connected;
	uint8_t signal_level;
	uint8_t pos_available; //for distance accumulate
	uint8_t velocity_kmh_int;
	uint8_t heading_int;
	uint8_t parked_mode;
	uint8_t queue_tail;
	uint8_t satellite;
	double velocity;
	double heading;
	double miles;
};
extern struct GPS_DATA_SUMMARY gps_data_summary; //RW data
extern uint8_t gps_msg_queue[queue_len][queue_data_len]; //一秒更新一次，最多存25秒数据。25秒内数据必须都发掉
extern uint8_t gps_pulse_message[queue_data_len];
void gps_msg_read(uint8_t* buffer);
void gps_generate_data_queue();
void gps_generate_null();
#endif /* INC_GPS_READER_H_ */
