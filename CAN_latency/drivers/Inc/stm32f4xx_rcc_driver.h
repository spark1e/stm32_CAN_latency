/*
 * stm32f4xx_rcc_driver.h
 *
 *  Created on: Mar 23, 2025
 *      Author: Wolf
 */

#ifndef INC_STM32F4XX_RCC_DRIVER_H_
#define INC_STM32F4XX_RCC_DRIVER_H_

#include "stm32f4xx.h"

//APB1
uint32_t RCC_GetPCLK1Value(void);
//APB2
uint32_t RCC_GetPCLK2Value(void);

//PLL
uint32_t RCC_GetPLLOutputClock();


#endif /* INC_STM32F4XX_RCC_DRIVER_H_ */
