/*
 * op_modes.h
 *
 *  Created on: Apr 29, 2020
 *      Author: pzahl
 */

#ifndef INC_OP_MODES_H_
#define INC_OP_MODES_H_

#define HIGH GPIO_PIN_SET
#define LOW  GPIO_PIN_RESET

#define BUTTON_PRESSED LOW

//#define RACE_SIGN_ALEX
#ifdef RACE_SIGN_ALEX
#define Button1Pin  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) // PA4 (pin14)
#define Button2Pin  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) // PA5 (pin15)
#define Button3Pin  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) // PA6 (pin16)
#define Button4Pin  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) // PA7 (pin17)
#define ButtonMDPin HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) // PC13 (pin2)
#define ButtonS1Pin HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) // PA0 (pin10)
#define ButtonS2Pin HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) // PA1 (pin11)
#else
#define Button1Pin  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) // PA4 (pin14)
#define Button2Pin  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) // PA5 (pin15)
#define Button3Pin  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) // PA6 (pin16)
#define Button4Pin  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) // PA7 (pin17)
#define ButtonMDPin HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) // PC13 (pin2)
#define ButtonS1Pin HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) // PA0 (pin10)
#define ButtonS2Pin HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) // PA1 (pin11)
#endif

void RunModeLapCntr (void);
void RunModeTime (int index, int timer_dir);
void RunTimerTest (void);
int RunModeControl(int m);
void StandByMode(void);


int DHT22_Start (void);
uint8_t DHT22_Check_Response (void);
uint8_t DHT22_Read (void);
void RunModeDisplayTempHumidity(void);

void RunModeLight(void);

void DisplayRTCDate (void);
void RunRTCClock (void);


#endif /* INC_OP_MODES_H_ */
