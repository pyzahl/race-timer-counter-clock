/*
 * ws2812.c
 *
 *  Created on: May 15, 2020
 *      Author: pzahl
 */


#include "ws2812.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/* Variables -----------------------------------------------*/
static uint8_t LEDbuffer[WS_LED_BUFFER_SIZE];

/*            29 x 35
 4  7  1        16        1
 *
 *        32 33 .. 46 47       1
 *     31                48
 *     30                49
 *117  ..                ..   16
 *116  17                62
 *     16                63
 *        15 14 .. 01 00       1
 *    111                64
 *115 110                65
 *114   ..                ..  16
 *     97                78
 *113  96                79
 *112     95 94 .. 81 80       1
 *
 */

#if 1
static const int8_t LEDDigitXY[35][24] = {
		// 0   1   2   3   4   5   6   7   8   9  10  11  12  12  14  15  16  17  18  19  20  21  22  23
		{ -1, -1, -1, -1, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, -1 }, // 0
		{ -1, -1, -1, -1, -1, -1, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 48 }, // 1
		{ -1, -1, -1, -1, -1, -1, 30, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 49 }, // 2
		{ -1, -1, -1, -1, -1, -1, 29, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50 },
		{ -1, -1, -1, -1, -1, -1, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 51 },
		{ -1, -1, -1, -1, -1, -1, 27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 52 },
		{ -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 53 },
		{ -1, -1, -1, -1, -1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 54 },
		{ -1, -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 55 },
		{117, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 56 },
		{116, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 57 }, // 10
		{ -1, -1, -1, -1, -1, -1, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 58 }, // 11
		{ -1, -1, -1, -1, -1, -1, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 59 },
		{ -1, -1, -1, -1, -1, -1, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 60 },
		{ -1, -1, -1, -1, -1, -1, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 61 },
		{ -1, -1, -1, -1, -1, -1, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62 },
		{ -1, -1, -1, -1, -1, -1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 63 }, // 16
		{ -1, -1, -1, -1, -1, -1, -1, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1 , 0, -1 }, // 17
		{ -1, -1, -1, -1, -1, -1,111, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 64 }, // 18
		{ -1, -1, -1, -1, -1, -1,110, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 65 },
		{ -1, -1, -1, -1, -1, -1,109, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 66 },
		{ -1, -1, -1, -1, -1, -1,108, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 67 },
		{ -1, -1, -1, -1, -1, -1,107, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 68 }, // 22
		{ -1, -1, -1, -1, -1, -1,106, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 69 }, // 23
		{115, -1, -1, -1, -1, -1,105, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 70 },
		{114, -1, -1, -1, -1, -1,104, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 71 },
		{ -1, -1, -1, -1, -1, -1,103, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 72 },
		{ -1, -1, -1, -1, -1, -1,102, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 73 },
		{ -1, -1, -1, -1, -1, -1,101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74 },
		{ -1, -1, -1, -1, -1, -1,100, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 75 },
		{ -1, -1, -1, -1, -1, -1, 99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 76 },
		{ -1, -1, -1, -1, -1, -1, 98, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 77 },
		{ -1, -1, -1, -1, -1, -1, 97, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78 }, // 32
		{113, -1, -1, -1, -1, -1, 96, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 79 }, // 33
		{112, -1, -1, -1, -1, -1, -1, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81 ,80, -1 }  // 34
};
#endif


/* Functions -----------------------------------------------*/

void ws2812_init(void) {
	fillBufferBlack();
	HAL_TIM_PWM_Start_DMA (&htim4, TIM_CHANNEL_1, (uint32_t *) LEDbuffer, WS_LED_BUFFER_SIZE);

	//uint16_t pData[25]={1,2,3,4,5,6,13,0,1,2,3,4,5,6,13,0,1,2,3,4,5,6,13,0 ,0};
	//HAL_TIM_PWM_Start_DMA (&htim4, TIM_CHANNEL_1, (uint32_t*)pData, 25);
}

void ws2812_update(void) {
	HAL_TIM_PWM_Start_DMA (&htim4, TIM_CHANNEL_1, (uint32_t *) LEDbuffer, WS_LED_BUFFER_SIZE);
}


RGB_color HsvToRgb(HSV_color hsv)
{
    RGB_color rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6;

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}

void setLEDcolorHSV(uint32_t LEDnumber, uint8_t h, uint8_t s, uint8_t v) {
	   HSV_color hsv = { h,s,v };
	   RGB_color rgb = HsvToRgb (hsv);
	   setLEDcolor (LEDnumber, rgb.r, rgb.g, rgb.b);
}

// [-48 ... +48] [ -17 ... +17 ]
void setLEDcolorHSVxy(int x, int y,  uint8_t h, uint8_t s, uint8_t v) {
	const int NLEDdigit = 7*16+6; // 7x16 segments + 3x2 dots
	int n = 3*NLEDdigit;

	if (x<-48 || x > 48 || y < -17 || y > 17) return;
	x += 48;
	while (x > 24){
		x -= 24;
		n -= NLEDdigit;
	}
	int i = n + (int)LEDDigitXY[y+17][x];

	if (i>=0 && i < WS_LED_NUMBER)
		setLEDcolorHSV (i, h, s, v);
}



#define MAX_MIX_LEDS (7*16)
static RGB_color leds_rgb[MAX_MIX_LEDS];

void setLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint8_t tempBuffer[24];
	uint32_t i;
	uint32_t LEDindex;

	if (LEDnumber >= WS_LED_NUMBER)
		return;

	if (LEDnumber < MAX_MIX_LEDS){
			leds_rgb[LEDnumber].r = RED;
			leds_rgb[LEDnumber].g = GREEN;
			leds_rgb[LEDnumber].b = BLUE;
	}

	LEDindex = LEDnumber % WS_LED_NUMBER;

	for (i = 0; i < 8; i++) // GREEN data
		tempBuffer[i] = ((GREEN << i) & 0x80) ? WS2812_1 : WS2812_0;
	for (i = 0; i < 8; i++) // RED
		tempBuffer[8 + i] = ((RED << i) & 0x80) ? WS2812_1 : WS2812_0;
	for (i = 0; i < 8; i++) // BLUE
		tempBuffer[16 + i] = ((BLUE << i) & 0x80) ? WS2812_1 : WS2812_0;

	for (i = 0; i < 24; i++)
		LEDbuffer[RESET_SLOTS_BEGIN + LEDindex * 24 + i] = tempBuffer[i];
}

void addLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint8_t tempBuffer[24];
	uint32_t i;
	uint32_t LEDindex;

	if (LEDnumber >= WS_LED_NUMBER)
		return;

	if (LEDnumber < MAX_MIX_LEDS){
			leds_rgb[LEDnumber].r += RED;
			leds_rgb[LEDnumber].g += GREEN;
			leds_rgb[LEDnumber].b += BLUE;
	}

	LEDindex = LEDnumber % WS_LED_NUMBER;

	for (i = 0; i < 8; i++) // GREEN data
		tempBuffer[i] = ((leds_rgb[LEDnumber].g << i) & 0x80) ? WS2812_1 : WS2812_0;
	for (i = 0; i < 8; i++) // RED
		tempBuffer[8 + i] = ((leds_rgb[LEDnumber].r << i) & 0x80) ? WS2812_1 : WS2812_0;
	for (i = 0; i < 8; i++) // BLUE
		tempBuffer[16 + i] = ((leds_rgb[LEDnumber].b << i) & 0x80) ? WS2812_1 : WS2812_0;

	for (i = 0; i < 24; i++)
		LEDbuffer[RESET_SLOTS_BEGIN + LEDindex * 24 + i] = tempBuffer[i];
}



void fillLEDcolorHSV(uint32_t start, uint32_t end, uint8_t h, uint8_t s, uint8_t v) {
	HSV_color hsv = { h,s,v };
	RGB_color rgb = HsvToRgb (hsv);
	if (start >= WS_LED_NUMBER || end >= WS_LED_NUMBER)
		return;

	for (uint32_t index = start; index <= end; index++)
		setLEDcolor (index, rgb.r, rgb.g, rgb.b);
}

void fillLEDcolor(uint32_t start, uint32_t end, uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	if (start >= WS_LED_NUMBER || end >= WS_LED_NUMBER)
		return;

	for (uint32_t index = start; index <= end; index++)
		setLEDcolor(index, RED, GREEN, BLUE);
}

void setWHOLEcolor(uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint32_t index;

	for (index = 0; index < WS_LED_NUMBER; index++)
		setLEDcolor(index, RED, GREEN, BLUE);
}

void fillBufferBlack(void) {
	/*Fill LED buffer - ALL OFF*/
	uint32_t index, buffIndex;
	buffIndex = 0;

	for (index = 0; index < RESET_SLOTS_BEGIN; index++) {
		LEDbuffer[buffIndex] = WS2812_RESET;
		buffIndex++;
	}
	for (index = 0; index < WS_LED_DATA_SIZE; index++) {
		LEDbuffer[buffIndex] = WS2812_0;
		buffIndex++;
	}
	LEDbuffer[buffIndex] = WS2812_0;
	buffIndex++;
	for (index = 0; index < RESET_SLOTS_END; index++) {
		LEDbuffer[buffIndex] = 0;
		buffIndex++;
	}
}

void fillBufferWhite(void) {
	/*Fill LED buffer - ALL OFF*/
	uint32_t index, buffIndex;
	buffIndex = 0;

	for (index = 0; index < RESET_SLOTS_BEGIN; index++) {
		LEDbuffer[buffIndex] = WS2812_RESET;
		buffIndex++;
	}
	for (index = 0; index < WS_LED_DATA_SIZE; index++) {
		LEDbuffer[buffIndex] = WS2812_1;
		buffIndex++;
	}
	LEDbuffer[buffIndex] = WS2812_0;
	buffIndex++;
	for (index = 0; index < RESET_SLOTS_END; index++) {
		LEDbuffer[buffIndex] = 0;
		buffIndex++;
	}
}



#if 0
// ======================================
// JUST A COPY OF CubeIDE generated code
// ======================================

static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */
  /*
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = (uint32_t)((SystemCoreClock / WS_TIMER_CLOCK_FREQ) - 1);
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = WS_TIMER_PERIOD - 1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  htim4.Init.RepetitionCounter = WS_LED_BUFFER_SIZE + 1;
  */
  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = (uint32_t)((SystemCoreClock / WS_TIMER_CLOCK_FREQ) - 1);
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = WS_TIMER_PERIOD - 1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  htim4.Init.RepetitionCounter = WS_LED_BUFFER_SIZE + 1;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

  /* USER CODE END DMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_tim4_ch1);
  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

  /* USER CODE END DMA1_Channel1_IRQn 1 */
}

// ==================== END COPY ==================
#endif


