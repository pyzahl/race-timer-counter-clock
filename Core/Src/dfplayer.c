/*
 * dfplayer.c
 *
 *  Created on: Jun 14, 2020
 *      Author: pzahl
 */



#include <stdio.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "delay_us.h"

#include "dfplayer.h"

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;


uint8_t dfp_message_in[DFPLAYER_RECEIVED_LENGTH];
uint8_t dfp_message_out[DFPLAYER_SEND_LENGTH] = {0x7E, 0xFF, 06, 00, 01, 00, 00, 00, 00, 0xEF};


void dfplayer_init(){

	// Format: 0x7E [VER LEN CMD FEEDBACK PARAM_HI8 PARAM_LO8] CRC 0xEF
	//                           0: no feedback
	//                           1: feedback
	// Format:   7E FF 06 09 00 00 04 FF DD EF

	// Volume 30 (max)
	dfplayer_volume(16);
	delay_us(100000);
}

void chime(int id){
	dfplayer_play(id);
}

void dfplayer_send(){
	HAL_UART_Transmit_DMA(&huart2, dfp_message_out, sizeof(dfp_message_out));
}

uint16_t dfplayer_calculate_checksum(uint8_t *buffer){
	uint16_t sum = 0;
	for (int i=Stack_Version; i<Stack_CheckSum; i++) {
		sum += buffer[i];
	}
	return -sum;
}

void uint16ToArray(uint16_t value, uint8_t *array){
	*array     = (uint8_t)(value>>8);
	*(array+1) = (uint8_t)(value);
}

void dfplayer_send_cmd(uint8_t command){
	dfplayer_send_cmd16(command, 0);
}

void dfplayer_send_cmd16(uint8_t command, uint16_t argument){
	dfp_message_out[Stack_Command] = command;
	uint16ToArray (argument, &dfp_message_out[Stack_Parameter]);
	uint16ToArray (dfplayer_calculate_checksum (dfp_message_out), &dfp_message_out[Stack_CheckSum]);
	dfplayer_send ();
}

void dfplayer_send_cmd88(uint8_t command, uint8_t argumentHigh, uint8_t argumentLow){
  uint16_t buffer = argumentHigh;
  buffer <<= 8;
  dfplayer_send_cmd16(command, buffer | argumentLow);
}


// player commands

void dfplayer_next(){
	dfplayer_send_cmd(0x01);
}

void dfplayer_previous(){
	dfplayer_send_cmd(0x02);
}

void dfplayer_play(int track){
	dfplayer_send_cmd16(0x03, track);
}
void dfplayer_volume_up(){
	dfplayer_send_cmd(0x04);
}
void dfplayer_volume_down(){
	dfplayer_send_cmd(0x05);
}
void dfplayer_volume(uint8_t vol){
	dfplayer_send_cmd16(0x06, vol);
}

void dfplayer_EQ(uint8_t eq){
	dfplayer_send_cmd16(0x07, eq);
}

void dfplayer_loop(int track){
	dfplayer_send_cmd16(0x08, track);
}
void dfplayer_outputdevice(uint8_t d){
	dfplayer_send_cmd16(0x09, d);
	delay_us(200000);
}
void dfplayer_sleep(){
	dfplayer_send_cmd(0x0A);
}
void dfplayer_reset(){
	dfplayer_send_cmd(0x0C);
}
void dfplayer_start(){
	dfplayer_send_cmd(0x0D);
}

void dfplayer_pause(){
	dfplayer_send_cmd(0x0E);
}
void dfplayer_play_folder(uint8_t folder, uint8_t track){
	dfplayer_send_cmd88(0x0F, folder, track);
}







