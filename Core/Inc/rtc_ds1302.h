/*
 * rtc_ds1302.h
 *
 *  Created on: May 13, 2020
 *  DS1302 external Maxim RTC support
 *      Author: pzahl
 */

#ifndef INC_RTC_DS1302_H_
#define INC_RTC_DS1302_H_

#include "stm32f1xx_hal.h"

#define DS_1302_CLK_PIN GPIO_PIN_7 // SCLK
#define DS_1302_DAT_PIN GPIO_PIN_6 // I/O
#define DS_1302_RST_PIN GPIO_PIN_5 // CE

enum Register {
  kSecondReg       = 0,
  kMinuteReg       = 1,
  kHourReg         = 2,
  kDateReg         = 3,
  kMonthReg        = 4,
  kDayReg          = 5,
  kYearReg         = 6,
  kWriteProtectReg = 7,
  kRamAddress0     = 32
};

enum Command {
  kClockBurstRead  = 0xBF,
  kClockBurstWrite = 0xBE,
  kRamBurstRead    = 0xFF,
  kRamBurstWrite   = 0xFE
};
void rtc_init(void);
uint8_t bcd_to_dec(uint8_t bcd);
uint8_t rtc_read_reg(uint8_t reg);
void rtc_write_reg(uint8_t reg, uint8_t num);
void rtc_set_time(
		uint8_t year, uint8_t month, uint8_t date,
		uint8_t hour, uint8_t min, uint8_t sec);

void rtc_set_time_bcd(
		uint8_t year, uint8_t month, uint8_t date,
		uint8_t hour, uint8_t min, uint8_t sec);

uint8_t hourFromRegisterValue(const uint8_t value);
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *day);
void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);
int rtc_get_seconds_of_day ();


#endif /* INC_RTC_DS1302_H_ */
