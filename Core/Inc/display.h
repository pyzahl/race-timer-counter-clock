/*
 * display.h
 *
 *  Created on: Apr 29, 2020
 *      Author: pzahl
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#define HIGH GPIO_PIN_SET
#define LOW  GPIO_PIN_RESET

//#define RACE_SIGN_ALEX
#ifdef RACE_SIGN_ALEX

//Pin connected to ST_CP (DATA-LATCH) of 74HC595
#define latchPin(X) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, X) // PB1 (pin19)
//Pin connected to SH_CP (SERIAL-CLK) of 74HC595
#define clockPin(X) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, X) // PB12 (pin25)
////Pin connected to DS of 74HC595
#define RedDataPin(X)   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, X) // PB14 (pin27)
#define GreenDataPin(X) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, X) // PC14 (pin3)
#define BlueDataPin(X)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, X) // PB13 (pin26)

#else

//#define LED_SERIAL595_CONTROL(GPORT, GPIN, X) HAL_GPIO_WritePin(GPORT, GPIN, X)
#define LED_SERIAL595_CONTROL(GPORT, GPIN, X) ;

//Pin connected to ST_CP (DATA-LATCH) of 74HC595
#define latchPin(X) LED_SERIAL595_CONTROL(GPIOB, GPIO_PIN_3, X) // PB1 (pin19)
//Pin connected to SH_CP (SERIAL-CLK) of 74HC595
#define clockPin(X) LED_SERIAL595_CONTROL(GPIOB, GPIO_PIN_4, X) // PB12 (pin25)
////Pin connected to DS of 74HC595
#define RedDataPin(X)   LED_SERIAL595_CONTROL(GPIOB, GPIO_PIN_5, X) // PB14 (pin27)
#define GreenDataPin(X) LED_SERIAL595_CONTROL(GPIOB, GPIO_PIN_6, X) // PC14 (pin3)
#define BlueDataPin(X)  LED_SERIAL595_CONTROL(GPIOB, GPIO_PIN_7, X) // PB13 (pin26)

#endif

#define DIGIT_m 10
#define DIGIT_E 11
#define DIGIT_F 12
#define DIGIT_C 13
#define DIGIT_o 14
#define DIGIT_O 15
#define DIGIT_n 16
#define DIGIT_d 17
#define DIGIT_u 18
#define DIGIT_t 19
#define DIGIT_i 20
#define DIGIT_r 21
#define DIGIT_l 22
#define DIGIT_k 23
#define DIGIT_Minus 24
#define DIGIT__   25
#define DIGIT_OFF 26


void ClearDisplay (void); // nothing to initialize, just clear it.
void SetSegments (uint8_t color[4], uint8_t segm[4]);
void RunSegmentTest (void);
void RunDisplayTest (void);

void display_number(int numberToDisplay, int color, int dots);
void display_two_numbers(int numberToDisplayLeft, int numberToDisplayRight, int colorLeft, int colorRight, int dots);
void display_two_numbers_leadzero(int numberToDisplayLeft, int numberToDisplayRight, int colorLeft, int colorRight, int dots);
void display_colon(int color);
void display_dot(int pos, int lv, int color);
void display_TempC(int Tc10, int color);
void display_RH(int Rh10, int color);
void display_mode(int mode, int color[8]);
void display_error(int i, int c1, int c2);

#endif /* INC_DISPLAY_H_ */
