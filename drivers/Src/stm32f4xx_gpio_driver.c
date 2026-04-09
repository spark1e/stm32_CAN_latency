/*
 * stm32f4xx_gpio_driver.c
 *
 *  Created on: Feb 16, 2025
 *      Author: Wolf
 */

#include "stm32f4xx_gpio_driver.h"




/***********************************************************************************
 * @fn			-GPIO_PeriClockControl
 *
 * @brief		-This function enables or disables peripheral clock for the given GPIO port
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-ENABLE or DISABLE macros
 * @param[in]	-none
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t ENorDI){
	if(ENorDI == ENABLE){
		if(pGPIOx == GPIOA)	GPIOA_PCLK_EN();
		else if(pGPIOx == GPIOB)  GPIOB_PCLK_EN();
		else if(pGPIOx == GPIOC)  GPIOC_PCLK_EN();
		else if(pGPIOx == GPIOD)  GPIOD_PCLK_EN();
		else if(pGPIOx == GPIOE)  GPIOE_PCLK_EN();
		else if(pGPIOx == GPIOF)  GPIOF_PCLK_EN();
		else if(pGPIOx == GPIOG)  GPIOG_PCLK_EN();
		else if(pGPIOx == GPIOH)  GPIOH_PCLK_EN();
	}
}

/***********************************************************************************
 * @fn			-GPIO_Init
 *
 * @brief		-This function sets up a pin
 *
 * @param[in]	-Handle for a GPIO Pin
 * @param[in]	-none
 * @param[in]	-none
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle){
	uint32_t temp = 0; //temporary register

	//enable peripheral clock
	GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);


	// GPIO_PinNumber
	if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG){
		//non interrupt mode here
		temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode << (2 *pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
		pGPIOHandle->pGPIOx->MODER &= ~(0x3 << 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);	//clearing
		pGPIOHandle->pGPIOx->MODER |= temp;	//setting
		temp = 0;
	}
	else{
		//this is interrupt mode
		if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT){
			//1. Configure the FTSR
			EXTI->EXTI_FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			//clear corresponding RTSR bit
			EXTI->EXTI_RTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}else if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT){
			//1. Configure the RTSR
			EXTI->EXTI_RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			//clear corresponding FTSR bit
			EXTI->EXTI_FTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}else if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT){
			//1. Configure both FTSR and RTSR
			EXTI->EXTI_RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			EXTI->EXTI_FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}

		//2. Configure GPIO port selection in SYSCFG_EXTICR
		uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4;
		uint8_t temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;
		uint8_t portcode = GPIO_BASE_ADDR_TO_CODE(pGPIOHandle->pGPIOx);
		SYSCFG_PCLK_EN();
		SYSCFG->SYSCFG_EXTICR[temp1] = portcode << (temp2 * 4);

		//3. Enable the EXTI interrupt delivery using IMR
		EXTI->EXTI_IMR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
	}
	// GPIO_PinSpeed
	temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed << (2 *pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
	pGPIOHandle->pGPIOx->OSPEEDER &= ~(0x3 << 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);	//clearing
	pGPIOHandle->pGPIOx->OSPEEDER |= temp;
	temp = 0;
	// GPIO_PinPuPdControl
	temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl << (2 *pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
	pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 << 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);	//clearing
	pGPIOHandle->pGPIOx->PUPDR |= temp;
	temp = 0;
	// GPIO_PinOPType;
	temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinOPType << (pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));
	pGPIOHandle->pGPIOx->OTYPER &= ~(0x3 << 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);	//clearing
	pGPIOHandle->pGPIOx->OTYPER |= temp;
	temp = 0;
	// GPIO_PinAltFunMode;
	if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_ALTFN){
		uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;
		if(pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber <= 7){
			pGPIOHandle->pGPIOx->AFRL &= ~(0xF  << (4 * temp1));
			pGPIOHandle->pGPIOx->AFRL |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode  << (4 * temp1));
		}
		else{
			pGPIOHandle->pGPIOx->AFRH &= ~(0xF  << (4 * temp1));
			pGPIOHandle->pGPIOx->AFRH |= (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode  << (4 * temp1));
		}
	}

}



void GPIO_DeInit(GPIO_RegDef_t *pGPIOx){
	if(pGPIOx == GPIOA)	GPIOA_REG_RESET();
		else if(pGPIOx == GPIOB)  GPIOB_REG_RESET();
		else if(pGPIOx == GPIOC)  GPIOC_REG_RESET();
		else if(pGPIOx == GPIOD)  GPIOD_REG_RESET();
		else if(pGPIOx == GPIOE)  GPIOE_REG_RESET();
		else if(pGPIOx == GPIOF)  GPIOF_REG_RESET();
		else if(pGPIOx == GPIOG)  GPIOG_REG_RESET();
		else if(pGPIOx == GPIOH)  GPIOH_REG_RESET();
}

//Data Read - Write

/***********************************************************************************
 * @fn			-GPIO_ReadFromInputPin
 *
 * @brief		-
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-Pin Number
 * @param[in]	-none
 *
 * @return		-0 or 1
 *
 * @Note		-none
 *
 * */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber){
	 uint8_t value;
	 value = (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x00000001);
	 return value;

}

/***********************************************************************************
 * @fn			-GPIO_ReadFromInputPort
 *
 * @brief		-
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-none
 * @param[in]	-none
 *
 * @return		-
 *
 * @Note		-none
 *
 * */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx){
	 uint16_t value;
	 value = (uint16_t)(pGPIOx->IDR) ;
	 return value;
}


/***********************************************************************************
 * @fn			-GPIO_WriteToOutputPin
 *
 * @brief		-
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-Pin Number
 * @param[in]	-output value
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value){
	if(Value == GPIO_PIN_SET){
		//write 1 to the output data register at the bit field corresponding to the pin number
		pGPIOx->ODR |= (1 << PinNumber);
	}
	else{
		//write 0
		pGPIOx->ODR &= ~(1 << PinNumber);
	}


}

/***********************************************************************************
 * @fn			-GPIO_WriteToOutputPort
 *
 * @brief		-
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-output value
 * @param[in]	-none
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value){
	pGPIOx->ODR = Value;
}


/***********************************************************************************
 * @fn			-GPIO_ToggleOutputPin
 *
 * @brief		-
 *
 * @param[in]	-base address of the GPIO peripheral
 * @param[in]	-pin number
 * @param[in]	-none
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber){
	pGPIOx->ODR  ^= (1 << PinNumber);
}


/***********************************************************************************
 * @fn			-GPIO_IRQITConfig
 *
 * @brief		-
 *
 * @param[in]	-IRQ Number
 * @param[in]	-Enable or Disable
 * @param[in]	-none
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_IRQITConfig(uint8_t IRQNumber, uint8_t ENorDI){
	if(ENorDI == ENABLE){
		if(IRQNumber <= 31){
			*NVIC_ISER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64){
			*NVIC_ISER1 |= (1 << (IRQNumber% 32));
		}
		else if(IRQNumber >=64 && IRQNumber < 96){
			*NVIC_ISER2 |= (1 << (IRQNumber% 64));
		}
	}
	else{
		if(IRQNumber <= 31){
			*NVIC_ICER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64){
			*NVIC_ICER1 |= (1 << (IRQNumber% 32));
		}
		else if(IRQNumber >=64 && IRQNumber < 96){
			*NVIC_ICER2 |= (1 << (IRQNumber% 64));
		}
	}
}

/***********************************************************************************
 * @fn			-GPIO_IRQPriorityCOnfig
 *
 * @brief		-
 *
 * @param[in]	-IRQ Number
 * @param[in]	-IRQ priority
 * @param[in]	-
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority){
	//1. IPR register
	uint8_t iprx = IRQNumber / 4;
	uint8_t section = IRQNumber % 4;

	uint8_t shift_amount = (section * 8) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASEADDR + iprx) |= (IRQPriority << shift_amount);
}

/***********************************************************************************
 * @fn			-GPIO_IRQHandling
 *
 * @brief		-
 *
 * @param[in]	-IRQ Number
 * @param[in]	-
 * @param[in]	-
 *
 * @return		-none
 *
 * @Note		-none
 *
 * */
void GPIO_IRQHandling(uint8_t PinNumber){
	//clear the exti pr register corresponding to the pin number
	if(EXTI->EXTI_PR & (1 << PinNumber)){
		//clear
		EXTI->EXTI_PR |= (1 << PinNumber);
	}
}








