/*
 * delay_us.h
 *
 *  Created on: May 25, 2020
 *      Author: pzahl
 */

#ifndef INC_DELAY_US_H_
#define INC_DELAY_US_H_

extern void delay_us(uint32_t micros);


/**
 * @brief  Delays for amount of micro seconds
 * @param  micros: Number of microseconds for delay
 * @retval None
 */
__STATIC_INLINE void inline_delay_us(__IO uint32_t micros) {
    /* Go to clock cycles */
    micros *= (SystemCoreClock / 1000000) / 5;

    /* Wait till done */
    while (micros--);
}



#endif /* INC_DELAY_US_H_ */
