/*
 * main.c
 *
 *  Created on: Feb 25, 2025
 *      Author: Wolf
 */
#include "stm32f4xx.h"

int main(void){

	return 0;
}


void EXTI0_IRQHandler(void){
	GPIO_IRQHandling(0);
}

