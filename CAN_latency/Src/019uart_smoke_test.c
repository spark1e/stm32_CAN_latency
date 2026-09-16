/*
 * 019uart_smoke_test.c
 *
 *  Created on: Mar 14, 2026
 *      Author: Nikita Volkov (https://github.com/spark1e)
 *
 *  Minimal UART2 smoke test — no CAN, no TIM2, no dependencies.
 *  Use this to confirm the UART path works (wiring + GPIO + USART2 init)
 *  before chasing problems in higher-level test files.
 *
 *  Pin assignments (STM32F446RE Nucleo-64):
 *    PA2  — USART2_TX (AF7)  → ST-LINK virtual COM port  /  external adapter RX
 *    PA3  — USART2_RX (AF7)  → ST-LINK virtual COM port  /  external adapter TX
 *
 *  Terminal settings: 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control.
 *
 *  Expected output (one line per ~1 s):
 *    [UART OK] tick #0001
 *    [UART OK] tick #0002
 *    [UART OK] tick #0003
 *    ...
 *
 *  IMPORTANT: Exclude all other Src/*.c files that contain main() from the build
 *  (right-click → Resource Configurations → Exclude from Build).
 */

#include <string.h>
#include "stm32f4xx.h"


/*
 * ============================================================
 * GLOBAL HANDLE
 * ============================================================
 */
USART_Handle_t uart2_handle;


/*
 * ============================================================
 * UTILITIES
 * ============================================================
 */

/* Send a null-terminated string over USART2 */
static void uart_print(const char *str){
	USART_SendData(&uart2_handle, (uint8_t *)str, strlen(str));
}

/* Convert a uint32_t to a zero-padded decimal string of fixed width */
static void u32_to_dec(uint32_t value, char *buf, uint8_t width){
	char tmp[11];
	int8_t i = 0;

	if(value == 0){
		tmp[i++] = '0';
	}
	else{
		while(value > 0){
			tmp[i++] = (char)('0' + (value % 10));
			value /= 10;
		}
	}

	while(i < width){
		tmp[i++] = '0';
	}

	int8_t j;
	for(j = 0; j < i; j++){
		buf[j] = tmp[i - 1 - j];
	}
	buf[j] = '\0';
}

/* Crude software delay — no timer required.
 * Tuned roughly for ~1 second at 16 MHz HSI default. Not precise — that's fine
 * for a smoke test where we just want a visible heartbeat.
 */
static void rough_delay(volatile uint32_t loops){
	while(loops--){
		__asm volatile ("nop");
	}
}


/*
 * ============================================================
 * GPIO INIT — PA2 / PA3 as USART2 TX / RX (AF7)
 * ============================================================
 */
static void UART2_GPIO_Init(void){
	GPIO_Handle_t uart_gpio;
	memset(&uart_gpio, 0, sizeof(uart_gpio));

	uart_gpio.pGPIOx                             = GPIOA;
	uart_gpio.GPIO_PinConfig.GPIO_PinMode        = GPIO_MODE_ALTFN;
	uart_gpio.GPIO_PinConfig.GPIO_PinOPType      = GPIO_OT_TYPE_PP;
	uart_gpio.GPIO_PinConfig.GPIO_PinSpeed       = GPIO_SPEED_FAST;
	uart_gpio.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	uart_gpio.GPIO_PinConfig.GPIO_PinAltFunMode  = 7;	// AF7 = USART1/2/3

	// PA2 — USART2_TX
	uart_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_2;
	GPIO_Init(&uart_gpio);

	// PA3 — USART2_RX
	uart_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;
	GPIO_Init(&uart_gpio);
}


/*
 * ============================================================
 * USART2 INIT — 115200 8N1
 * ============================================================
 */
static void UART2_Init(void){
	uart2_handle.pUSARTx                          = USART2;
	uart2_handle.USART_Config.USART_Baud          = USART_STD_BAUD_115200;
	uart2_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
	uart2_handle.USART_Config.USART_Mode          = USART_MODE_TXRX;
	uart2_handle.USART_Config.USART_NoOfStopBits  = USART_STOPBITS_1;
	uart2_handle.USART_Config.USART_WordLength    = USART_WORDLEN_8BITS;
	uart2_handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;

	USART_Init(&uart2_handle);
	USART_PeripheralControl(USART2, ENABLE);
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */
int main(void){
	uint32_t counter = 0;
	char     buf[16];

	// 1. Bring up UART2 (PA2/PA3)
	UART2_GPIO_Init();
	UART2_Init();

	// 2. Print a one-shot banner so you can confirm the program reset cleanly
	uart_print("\r\n");
	uart_print("==========================================\r\n");
	uart_print("  UART2 Smoke Test\r\n");
	uart_print("  115200 8N1 | PA2=TX  PA3=RX\r\n");
	uart_print("==========================================\r\n");

	// 3. Heartbeat loop — print a counter forever
	while(1){
		counter++;

		uart_print("[UART OK] tick #");
		u32_to_dec(counter, buf, 4);
		uart_print(buf);
		uart_print("\r\n");

		// Roughly 1 second at 16 MHz HSI (the Nucleo's reset default)
		// If your clock is faster, the heartbeat just goes faster — still fine for a smoke test
		rough_delay(1600000);
	}
}
