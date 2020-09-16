#ifndef __ws2812
#define __ws2812

#include <stdio.h>
#include <string.h>
//#include "stm32l4xx_hal.h"
#include "stm32f1xx_hal.h"

/* Definition of TIM instance */
#define TIMx                             	TIM4

/* Definition for TIMx clock resources */
#define TIMx_CLK_ENABLE                  	__HAL_RCC_TIM4_CLK_ENABLE
#define DMAx_CLK_ENABLE                  	__HAL_RCC_DMA1_CLK_ENABLE

/* Definition for TIMx Pins */
#define TIMx_CHANNEL1_GPIO_CLK_ENABLE    	__HAL_RCC_GPIOB_CLK_ENABLE
#define TIMx_GPIO_CHANNEL1_PORT          	GPIOB
#define GPIO_PIN_CHANNEL1                	GPIO_PIN_6
#define GPIO_AF_TIMx                     	GPIO_AF1_TIM4

/* Definition for TIMx's DMA */
#define TIMx_CC1_DMA_REQUEST             	DMA_REQUEST_4
#define TIMx_CC1_DMA_INST                	DMA1_Channel1

/* Definition for DMAx's NVIC */
#define TIMx_DMA_IRQn                    	DMA1_Channel1_IRQn
#define TIMx_DMA_IRQHandler              	DMA1_Channel1_IRQHandler

//WS2812
#define WS2812_FREQ							(800000) 		// it is fixed: WS2812 require 800kHz
#define WS_TIMER_CLOCK_FREQ					(16000000)   	// can be modified - multiples of 800kHz are suggested
#define WS_TIMER_PERIOD						(WS_TIMER_CLOCK_FREQ / WS2812_FREQ)
#define WS_LED_NUMBER						(3*6+4*7*16)					// how many LEDs the MCU should control?
#define WS_LED_DATA_SIZE					(WS_LED_NUMBER * 24)
#define RESET_SLOTS_BEGIN					(50)
#define RESET_SLOTS_END						(50)
#define WS2812_LAST_SLOT					(1)
#define WS_LED_BUFFER_SIZE					(RESET_SLOTS_BEGIN + WS_LED_DATA_SIZE + WS2812_LAST_SLOT + RESET_SLOTS_END)
#define WS2812_0							(WS_TIMER_PERIOD / 3)				// WS2812's zero high time is long about one third of the period
#define WS2812_1							(WS_TIMER_PERIOD * 2 / 3)		// WS2812's one high time is long about two thirds of the period
#define WS2812_RESET						(0)

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
}RGB_color;

typedef struct
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
}HSV_color;

RGB_color HSV2RGB(HSV_color hsv);

void WS_HAL_GPIO_Init(void);
void HWS_AL_TIM_PWMDMA_MspInit(TIM_HandleTypeDef *htim);

void ws2812_init(void);
void ws2812_update(void);
void ws2812_update_first_n(uint16_t n_to_update);

void setLEDcolorHSVxy(int x, int y,  uint8_t h, uint8_t s, uint8_t v);
void setLEDcolorHSV(uint32_t LEDnumber, uint8_t h, uint8_t s, uint8_t v);
void fillLEDcolorHSV(uint32_t start, uint32_t end, uint8_t h, uint8_t s, uint8_t v);

void setLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE);
void addLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE);
void setWHOLEcolor(uint8_t RED, uint8_t GREEN, uint8_t BLUE);
void fillBufferBlack(void);
void fillBufferWhite(void);
void TIMx_DMA_IRQHandler(void);

#endif
