/*
 * stm32f4xx_rcc_driver.c
 *
 *  Created on: Mar 23, 2025
 *      Author: Wolf
 */


#include "stm32f4xx_rcc_driver.h"


uint16_t AHB_PreScaler[8] = {2,4,8,16,64,128,256,512};
uint8_t APB_PreScaler[4] = { 2, 4 , 8, 16};



uint32_t RCC_GetPCLK1Value(void)
{
	uint32_t pclk1,SystemClk;
	uint8_t clksrc,temp,ahbp,apb1p;

	//Determine source mode (SWS: System clock switch status)
	clksrc = ((RCC->RCC_CFGR >> 2) & 0x3);
	//HSI source
	if(clksrc == 0 ) SystemClk = 16000000;
	//HSE source
	else if(clksrc == 1) SystemClk = 8000000;
	//PLL source
	else if (clksrc == 2) SystemClk = RCC_GetPLLOutputClock();

	//apply prescallers

	//for AHB (HPRE: AHB prescaler)
	temp = ((RCC->RCC_CFGR >> 4 ) & 0xF);
	if(temp < 8) ahbp = 1;
	else ahbp = AHB_PreScaler[temp-8];

	//for APB1 (PPRE1: APB Low speed prescaler)
	temp = ((RCC->RCC_CFGR >> 10 ) & 0x7);
	if(temp < 4) apb1p = 1;
	else apb1p = APB_PreScaler[temp-4];

	//PLCK1 calculation
	pclk1 =  (SystemClk / ahbp) /apb1p;

	return pclk1;
}



/*********************************************************************
 * @fn      		  - RCC_GetPCLK2Value
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */
uint32_t RCC_GetPCLK2Value(void)
{
	uint32_t pclk2,SystemClk;
	uint8_t clksrc,temp,ahbp,apb2p;

	//Determine source mode (SWS: System clock switch status)
	clksrc = ((RCC->RCC_CFGR >> 2) & 0x3);
	//HSI source
	if(clksrc == 0 ) SystemClk = 16000000;
	//HSE source
	else if(clksrc == 1) SystemClk = 8000000;
	//PLL source
	else if (clksrc == 2) SystemClk = RCC_GetPLLOutputClock();

	//apply prescallers

	//for AHB (HPRE: AHB prescaler)
	temp = ((RCC->RCC_CFGR >> 4 ) & 0xF);
	if(temp < 8) ahbp = 1;
	else ahbp = AHB_PreScaler[temp-8];

	//for APB2 (PPRE2: APB high-speed prescaler)
	temp = (RCC->RCC_CFGR >> 13 ) & 0x7;
	if(temp < 4) apb2p = 1;
	else apb2p = APB_PreScaler[temp-4];

	//PCLK2 calculation
	pclk2 = (SystemClk / ahbp )/ apb2p;

	return pclk2;
}

uint32_t  RCC_GetPLLOutputClock(void)
{

	return 0;
}
