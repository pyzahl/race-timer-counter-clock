/*
 * rtc_ds1302.c
 *
 *  Created on: May 13, 2020
 *  DS1302 external Maxim RTC support
 *  borrowed from  Created on: 11 июн. 2017 г. stavinsky  https://gist.github.com/stavinsky/61a466a9b1bf12092b1293e86654f325
 *      Author: pzahl
 */


#include "rtc_ds1302.h"

extern void delay_us (uint16_t us);

#define USE_STM32RTC

#ifdef USE_STM32RTC

/* RTC structure */
extern RTC_HandleTypeDef hrtc;

void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *day){
	RTC_TimeTypeDef cTime = {0};
	RTC_DateTypeDef cDate = {0};
	HAL_RTC_GetTime(&hrtc, &cTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cDate, RTC_FORMAT_BIN);
	if (date)  *date  = cDate.Date;
	if (month) *month = cDate.Month;
	if (year)  *year  = cDate.Year;
	if (day)   *day   = cDate.WeekDay;
}

void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec){
	RTC_TimeTypeDef cTime = {0};
	RTC_DateTypeDef cDate = {0};
	HAL_RTC_GetTime(&hrtc, &cTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cDate, RTC_FORMAT_BIN);
	*sec  = cTime.Seconds;
	*min  = cTime.Minutes;
	*hour = cTime.Hours;
}

int rtc_get_seconds_of_day (){
	RTC_TimeTypeDef cTime = {0};
	RTC_DateTypeDef cDate = {0};
	HAL_RTC_GetTime(&hrtc, &cTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cDate, RTC_FORMAT_BIN);
	return cTime.Seconds + 60*cTime.Minutes + cTime.Hours*3600;
}

#else

#define WR_MODE_SWITCH

uint8_t bcd_to_dec(uint8_t bcd) {
  return (10 * ((bcd & 0xF0) >> 4) + (bcd & 0x0F));
}

uint8_t decToBcd(uint8_t dec) {
  const uint8_t tens = dec / 10;
  const uint8_t ones = dec % 10;
  return (tens << 4) | ones;
}

uint8_t hourFromRegisterValue(const uint8_t value) {
  uint8_t adj;
  if (value & 128)  // 12-hour mode
    adj = 12 * ((value & 32) >> 5);
  else           // 24-hour mode
    adj = 10 * ((value & (32 + 16)) >> 4);
  return (value & 15) + adj;
}
void dat_pin_write_mode(){
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	GPIOB->CRH |= 0x3;
	GPIOB->CRH &= ~(0xc);
}
void dat_pin_read_mode(){
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	GPIOB->CRH &= ~(0xf);
	GPIOB->CRH |= 0x8;
}

static void pin_write_mode(uint16_t pin)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

#ifdef  WR_MODE_SWITCH
static void pin_read_mode(uint16_t pin)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
#endif


void pulse_clk(void){
	delay_us (1);
	HAL_GPIO_WritePin(GPIOB, DS_1302_CLK_PIN, GPIO_PIN_SET);
	delay_us (1);
	HAL_GPIO_WritePin(GPIOB, DS_1302_CLK_PIN, GPIO_PIN_RESET);
}

void rtc_write(uint8_t value){
#ifdef  WR_MODE_SWITCH
	pin_write_mode(DS_1302_DAT_PIN);
#else
	dat_pin_write_mode();
#endif
	for (int i=0; i<8; i++){
		HAL_GPIO_WritePin(GPIOB, DS_1302_DAT_PIN, (value>>i) & 1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
		pulse_clk();
	}
}

uint8_t rtc_read(){
	uint8_t input_value = 0;
	uint8_t bit = 0;
#ifdef  WR_MODE_SWITCH
	pin_read_mode(DS_1302_DAT_PIN);
#else
	dat_pin_read_mode();
#endif
	for (int i = 0; i < 8; ++i) {
		bit = HAL_GPIO_ReadPin(GPIOB, DS_1302_DAT_PIN);
		input_value |= (bit << i);
		pulse_clk();
	}
	return input_value;
}

uint8_t rtc_read_reg(uint8_t reg){
	volatile uint8_t cmd_byte = (0x81 | (reg << 1));
	uint8_t result = 0;
	HAL_GPIO_WritePin(GPIOB, DS_1302_RST_PIN, GPIO_PIN_SET);
	delay_us (3);
	rtc_write(cmd_byte);
	result = rtc_read();
	HAL_GPIO_WritePin(GPIOB, DS_1302_RST_PIN, GPIO_PIN_RESET);

	return result;
}

void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *day){
	  if (date)  *date  = bcd_to_dec (rtc_read_reg (kDateReg));
	  if (month) *month = bcd_to_dec (rtc_read_reg (kMonthReg));
	  if (year)  *year  = bcd_to_dec (rtc_read_reg (kYearReg));
	  if (day)   *day   = rtc_read_reg (kDayReg);
}
void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec){
	  *sec  = bcd_to_dec (rtc_read_reg (kSecondReg));
	  *min  = bcd_to_dec (rtc_read_reg (kMinuteReg));
	  *hour = hourFromRegisterValue (rtc_read_reg (kHourReg));
}

int rtc_get_seconds_of_day (){
	uint8_t h,m,s;
	rtc_get_time (&h, &m, &s);
	return ((int)h*3600+(int)m*60+(int)s);
}

void rtc_write_reg(uint8_t reg, uint8_t num){
	uint8_t cmd_byte = (0x80 | (reg << 1));
	uint8_t reg_data = decToBcd(num);
	HAL_GPIO_WritePin(GPIOB, DS_1302_RST_PIN, GPIO_PIN_SET);
	delay_us (3);
	rtc_write(cmd_byte);
	rtc_write(reg_data);
	HAL_GPIO_WritePin(GPIOB, DS_1302_RST_PIN, GPIO_PIN_RESET);

}

void rtc_set_time_bcd(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec){
	rtc_write_reg(kYearReg, year);
	rtc_write_reg(kMonthReg, month);
	rtc_write_reg(kDateReg, date);
	rtc_write_reg(kHourReg, hour); // 24H
	rtc_write_reg(kMinuteReg, min);
	rtc_write_reg(kSecondReg, sec);

}

void rtc_set_time(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec){
	rtc_set_time_bcd(
			decToBcd(year),
			decToBcd(month),
			decToBcd(date),
			decToBcd(hour),
			decToBcd(min),
			decToBcd(sec)
			);
}

void rtc_init(void){
	HAL_Delay(100); // 1000 ???
	__HAL_RCC_GPIOB_CLK_ENABLE();
	pin_write_mode(DS_1302_CLK_PIN);
	pin_write_mode(DS_1302_RST_PIN);
	HAL_GPIO_WritePin(GPIOB, DS_1302_CLK_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, DS_1302_RST_PIN, GPIO_PIN_RESET);
}

#endif
