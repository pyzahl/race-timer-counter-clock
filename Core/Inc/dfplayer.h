/*
 * dfplayer.h
 *
 *  Created on: Jun 14, 2020
 *      Author: pzahl
 */

#ifndef INC_DFPLAYER_H_
#define INC_DFPLAYER_H_


#define DFPLAYER_EQ_NORMAL 0
#define DFPLAYER_EQ_POP 1
#define DFPLAYER_EQ_ROCK 2
#define DFPLAYER_EQ_JAZZ 3
#define DFPLAYER_EQ_CLASSIC 4
#define DFPLAYER_EQ_BASS 5

#define DFPLAYER_DEVICE_U_DISK 1
#define DFPLAYER_DEVICE_SD 2
#define DFPLAYER_DEVICE_AUX 3
#define DFPLAYER_DEVICE_SLEEP 4
#define DFPLAYER_DEVICE_FLASH 5

#define DFPLAYER_RECEIVED_LENGTH 10
#define DFPLAYER_SEND_LENGTH 10

//#define _DEBUG

#define TimeOut 0
#define WrongStack 1
#define DFPlayerCardInserted 2
#define DFPlayerCardRemoved 3
#define DFPlayerCardOnline 4
#define DFPlayerPlayFinished 5
#define DFPlayerError 6
#define DFPlayerUSBInserted 7
#define DFPlayerUSBRemoved 8
#define DFPlayerUSBOnline 9
#define DFPlayerCardUSBOnline 10
#define DFPlayerFeedBack 11

#define Busy 1
#define Sleeping 2
#define SerialWrongStack 3
#define CheckSumNotMatch 4
#define FileIndexOut 5
#define FileMismatch 6
#define Advertise 7

#define Stack_Header 0
#define Stack_Version 1
#define Stack_Length 2
#define Stack_Command 3
#define Stack_ACK 4
#define Stack_Parameter 5
#define Stack_CheckSum 7
#define Stack_End 9


void chime(int id);

void dfplayer_init();
void dfplayer_play(int track);
void dfplayer_pause();
void dfplayer_volume(uint8_t vol);
void dfplayer_volume_up();
void dfplayer_volume_dn();


uint16_t dfplayer_calculate_checksum(uint8_t *buffer);
void dfplayer_send_cmd(uint8_t command);
void dfplayer_send_cmd16(uint8_t command, uint16_t argument);
void dfplayer_send_cmd88(uint8_t command, uint8_t argumentHigh, uint8_t argumentLow);

void dfplayer_next();
void dfplayer_previous();
void dfplayer_play(int track);
void dfplayer_volume_up();
void dfplayer_volume_down();
void dfplayer_volume(uint8_t vol);
void dfplayer_EQ(uint8_t eq);
void dfplayer_loop(int track);
void dfplayer_outputdevice(uint8_t d);
void dfplayer_sleep();
void dfplayer_reset();
void dfplayer_start();
void dfplayer_pause();
void dfplayer_play_folder(uint8_t folder, uint8_t track);

#endif /* INC_DFPLAYER_H_ */
