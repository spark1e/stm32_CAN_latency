/*
 * 018can_latency_test.c
 *
 *  Created on: Mar 14, 2026
 *      Author: Nikita Volkov (https://github.com/spark1e)
 *
 *  Phase 1 test application: Basic CAN communication with latency measurement
 *  and UART debug output.
 *
 *  Test flow:
 *    1. STM32 sends a CAN frame (ID 0x100) every 100 ms — records T0 (TIM2 µs)
 *    2. BeagleBone Black receives the frame, logs it, and sends back ACK (ID 0x200)
 *    3. STM32 receives the ACK — records T1 (TIM2 µs)
 *    4. Latency = T1 - T0 is printed over UART2 along with deadline status
 *
 *  Pin assignments (STM32F446RE Nucleo-64):
 *    PA11 — CAN1_RX  (AF9)
 *    PA12 — CAN1_TX  (AF9)
 *    PA2  — USART2_TX (AF7)  → ST-LINK virtual COM port
 *    PA3  — USART2_RX (AF7)  → ST-LINK virtual COM port
 *    TIM2 — 32-bit free-running 1 µs counter for timestamps
 *
 *  CAN configuration:
 *    Bitrate     : 500 kbps
 *    Mode        : Normal
 *    Deadline    : 200 ms (200 000 µs)
 *    TX ID       : 0x100 (STM32 → BBB)
 *    RX ID       : 0x200 (BBB → STM32, ACK)
 *    Payload     : 8 bytes [counter (4B) | timestamp T0 (4B)]
 */

#include <string.h>
#include "stm32f4xx.h"

/*
 * ============================================================
 * APPLICATION CONFIGURATION
 * ============================================================
 */
#define CAN_TX_ID			0x100U		// Outgoing message ID  (STM32 → BBB)
#define CAN_RX_ID			0x200U		// ACK message ID       (BBB → STM32)
#define DEADLINE_US			200000U		// 200 ms in microseconds
#define TX_PERIOD_MS		100U		// Send period

/*
 * TIM2 microsecond timer — minimal direct register access
 * (a full TIM driver isn't part of this project yet)
 *
 * TIM2 is a 32-bit timer on APB1. The APB1 timer clock is:
 *   - PCLK1       if APB1 prescaler == 1
 *   - PCLK1 * 2   if APB1 prescaler  > 1   (default Nucleo config)
 *
 * For a 1 MHz tick (1 µs/count) on a typical STM32F446 clock tree
 * (HCLK 90 MHz, PCLK1 45 MHz, APB1 prescaler /2 → TIM clk = 90 MHz):
 *     PSC = 90 - 1 = 89
 * If APB1 prescaler = 1 (PCLK1 == HCLK), use PSC = 44 instead.
 */
#define TIM2_BASE_ADDR	(APB1PERIPH_BASE + 0x0000U)
#define TIM2_CR1		(*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x00U))
#define TIM2_EGR		(*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x14U))
#define TIM2_CNT		(*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x24U))
#define TIM2_PSC		(*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x28U))
#define TIM2_ARR		(*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x2CU))

#define TIM2_PRESCALER	89		// → 1 MHz tick (90 MHz / 90)
#define TIM_CR1_CEN		0		// Counter enable bit position


/*
 * ============================================================
 * GLOBAL HANDLES
 * ============================================================
 */
USART_Handle_t  uart2_handle;
CAN_Handle_t    can1_handle;


/*
 * ============================================================
 * UTILITY FUNCTIONS — UART output and number formatting
 * ============================================================
 */

/* Send a null-terminated string over USART2 */
static void uart_print(const char *str){
	USART_SendData(&uart2_handle, (uint8_t *)str, strlen(str));
}

/* Convert a uint32_t to a zero-padded decimal string of fixed width
 * Example: u32_to_dec(123, buf, 6) → "000123"
 */
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

	// Pad with leading zeros up to 'width'
	while(i < width){
		tmp[i++] = '0';
	}

	// Reverse into output buffer and null-terminate
	int8_t j;
	for(j = 0; j < i; j++){
		buf[j] = tmp[i - 1 - j];
	}
	buf[j] = '\0';
}

/* Convert a uint32_t to a hex string of fixed width (uppercase, no 0x prefix) */
static void u32_to_hex(uint32_t value, char *buf, uint8_t width){
	const char hex[] = "0123456789ABCDEF";
	int8_t i;
	for(i = width - 1; i >= 0; i--){
		buf[i] = hex[value & 0xFU];
		value >>= 4;
	}
	buf[width] = '\0';
}


/*
 * ============================================================
 * TIM2 — microsecond free-running counter for timestamps
 * ============================================================
 */
static void TIM2_MicrosTimer_Init(void){
	// 1. Enable TIM2 peripheral clock (APB1ENR bit 0)
	RCC->RCC_APB1ENR |= (1 << 0);

	// 2. Set prescaler so the counter ticks at 1 MHz (1 µs per count)
	TIM2_PSC = TIM2_PRESCALER;

	// 3. Set auto-reload to maximum (32-bit free-running)
	TIM2_ARR = 0xFFFFFFFFU;

	// 4. Generate an update event so PSC takes effect immediately
	TIM2_EGR = 0x1U;

	// 5. Enable the counter (CEN bit in CR1)
	TIM2_CR1 |= (1 << TIM_CR1_CEN);
}

/* Read current TIM2 count (microseconds since boot, wraps at ~71 minutes) */
static inline uint32_t micros(void){
	return TIM2_CNT;
}

/* Simple busy-wait delay using TIM2 (handles 32-bit wrap correctly) */
static void delay_us(uint32_t us){
	uint32_t start = micros();
	while((uint32_t)(micros() - start) < us);
}

static void delay_ms(uint32_t ms){
	while(ms--){
		delay_us(1000);
	}
}


/*
 * ============================================================
 * GPIO INITIALISATION — CAN1 (PA11/PA12) and USART2 (PA2/PA3)
 * ============================================================
 */
static void CAN1_GPIO_Init(void){
	GPIO_Handle_t can_gpio;
	memset(&can_gpio, 0, sizeof(can_gpio));

	can_gpio.pGPIOx                          = GPIOA;
	can_gpio.GPIO_PinConfig.GPIO_PinMode     = GPIO_MODE_ALTFN;
	can_gpio.GPIO_PinConfig.GPIO_PinOPType   = GPIO_OT_TYPE_PP;
	can_gpio.GPIO_PinConfig.GPIO_PinSpeed    = GPIO_SPEED_HIGH;
	can_gpio.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;	// Pull-up on idle bus
	can_gpio.GPIO_PinConfig.GPIO_PinAltFunMode  = 9;			// AF9 = CAN1

	// PA12 — CAN1_TX
	can_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_12;
	GPIO_Init(&can_gpio);

	// PA11 — CAN1_RX
	can_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
	GPIO_Init(&can_gpio);
}

static void UART2_GPIO_Init(void){
	GPIO_Handle_t uart_gpio;
	memset(&uart_gpio, 0, sizeof(uart_gpio));

	uart_gpio.pGPIOx                          = GPIOA;
	uart_gpio.GPIO_PinConfig.GPIO_PinMode     = GPIO_MODE_ALTFN;
	uart_gpio.GPIO_PinConfig.GPIO_PinOPType   = GPIO_OT_TYPE_PP;
	uart_gpio.GPIO_PinConfig.GPIO_PinSpeed    = GPIO_SPEED_FAST;
	uart_gpio.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	uart_gpio.GPIO_PinConfig.GPIO_PinAltFunMode  = 7;			// AF7 = USART2

	// PA2 — USART2_TX
	uart_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_2;
	GPIO_Init(&uart_gpio);

	// PA3 — USART2_RX
	uart_gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;
	GPIO_Init(&uart_gpio);
}


/*
 * ============================================================
 * USART2 INITIALISATION — 115200 8N1 for debug output
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
 * CAN1 INITIALISATION — 500 kbps Normal mode
 * ============================================================
 *
 * Bit timing (PCLK1 = 16 MHz — HSI default, no PLL):
 *   Prescaler = 2 → TQ = 2 / 16 MHz = 125 ns
 *   1 + TS1 + TS2 = 1 + 13 + 2 = 16 TQ per bit → bit time = 2 µs → 500 kbps
 *   Sample point  = (1 + 13) / 16 = 87.5 %  (matches BBB c_can default)
 */
static void CAN1_Init_500kbps(void){
	can1_handle.pCANx = CAN1;
	// NORMAL mode — frames are transmitted on the physical CAN bus.
	// Requires at least one other active node (BeagleBone Black) to ACK each frame.
	can1_handle.CAN_Config.CAN_Mode          = CAN_MODE_NORMAL;
	can1_handle.CAN_Config.CAN_Prescaler     = CAN_PRESCALER_500KBPS_16MHZ;	// 2 (for HSI 16 MHz default)
	can1_handle.CAN_Config.CAN_TimeSeg1      = CAN_TS1_13TQ;
	can1_handle.CAN_Config.CAN_TimeSeg2      = CAN_TS2_2TQ;
	can1_handle.CAN_Config.CAN_SyncJumpWidth = CAN_SJW_1TQ;
	can1_handle.CAN_Config.CAN_TTCM          = DISABLE;
	can1_handle.CAN_Config.CAN_ABOM          = ENABLE;	// Auto recover from bus-off
	can1_handle.CAN_Config.CAN_AWUM          = DISABLE;
	can1_handle.CAN_Config.CAN_NART          = DISABLE;	// Retransmit forever on error → continuous bus activity for diagnosis
	can1_handle.CAN_Config.CAN_RFLM          = DISABLE;
	can1_handle.CAN_Config.CAN_TXFP          = DISABLE;	// Priority by ID

	CAN_Init(&can1_handle);
}


/*
 * ============================================================
 * CAN1 FILTER — accept standard ID 0x200 into FIFO 0
 * ============================================================
 *
 * 32-bit filter register layout:
 *   bits [31:21] = STID[10:0]
 *   bits [20:3]  = EXID[17:0]
 *   bit  [2]     = IDE
 *   bit  [1]     = RTR
 *   bit  [0]     = 0
 *
 * For standard ID 0x200, IDE=0:
 *   Filter value = (0x200 << 21) | (0 << 2) = 0x40000000
 *   Mask         = (0x7FF << 21) | (1 << 2) = 0xFFE00004   // exact STID match + IDE must be 0
 */
static void CAN1_Filter_Init(void){
	CAN_FilterConfig_t filter;

	// Pass-all filter: ID=0, mask=0 → every bit is "don't care" → accept every frame.
	// Use this for the loopback bench test. Tighten later when bringing up the BBB.
	filter.FilterIdHigh         = 0x0000;
	filter.FilterIdLow          = 0x0000;
	filter.FilterMaskIdHigh     = 0x0000;
	filter.FilterMaskIdLow      = 0x0000;
	filter.FilterFIFOAssignment = CAN_FIFO0;
	filter.FilterBank           = 0;
	filter.FilterMode           = CAN_FILTERMODE_IDMASK;
	filter.FilterScale          = CAN_FILTERSCALE_32BIT;
	filter.FilterActivation     = ENABLE;

	CAN_ConfigFilter(&can1_handle, &filter);
}


/*
 * ============================================================
 * UART REPORT HELPERS
 * ============================================================
 */

/* Print the program banner once at startup */
static void print_banner(void){
	uart_print("\r\n");
	uart_print("==========================================\r\n");
	uart_print("  STM32F446RE CAN Latency Monitor\r\n");
	uart_print("  500 kbps | Deadline: 200 ms\r\n");
	uart_print("  TX ID: 0x100  |  RX ID: 0x200\r\n");
	uart_print("==========================================\r\n");
}

/* Print a TX line: "[TX] #0001 | ID:0x100 | T0:000012345 us" */
static void print_tx(uint32_t counter, uint32_t t0){
	char buf[16];

	uart_print("[TX] #");
	u32_to_dec(counter, buf, 4);
	uart_print(buf);

	uart_print(" | ID:0x");
	u32_to_hex(CAN_TX_ID, buf, 3);
	uart_print(buf);

	uart_print(" | T0:");
	u32_to_dec(t0, buf, 10);
	uart_print(buf);
	uart_print(" us\r\n");
}

/* Print an RX line: "[RX] #0001 | ID:0x200 | T1:000012678 us" */
static void print_rx(uint32_t counter, uint32_t rx_id, uint32_t t1){
	char buf[16];

	uart_print("[RX] #");
	u32_to_dec(counter, buf, 4);
	uart_print(buf);

	uart_print(" | ID:0x");
	u32_to_hex(rx_id, buf, 3);
	uart_print(buf);

	uart_print(" | T1:");
	u32_to_dec(t1, buf, 10);
	uart_print(buf);
	uart_print(" us\r\n");
}

/* Print latency line + deadline status, then a separator */
static void print_latency(uint32_t latency_us){
	char buf[16];

	uart_print("[>>] Latency: ");
	u32_to_dec(latency_us, buf, 6);
	uart_print(buf);
	uart_print(" us | DEADLINE: ");

	if(latency_us <= DEADLINE_US){
		uart_print("MET\r\n");
	}
	else{
		uart_print("MISSED\r\n");
	}

	uart_print("------------------------------------------\r\n");
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */
int main(void){
	uint32_t    counter = 0;
	uint32_t    t0, t1, latency;
	CAN_TxMsg_t tx_msg;
	CAN_RxMsg_t rx_msg;
	uint8_t     mailbox;

	// 1. Set up the µs timer first — used for everything that follows
	TIM2_MicrosTimer_Init();

	// 2. Bring up UART2 (PA2/PA3 → ST-LINK virtual COM port)
	UART2_GPIO_Init();
	UART2_Init();

	print_banner();

	// Report the actual APB1 (PCLK1) clock so we can verify CAN bit timing assumptions
	{
		char dbg[16];
		uart_print("[INIT] PCLK1 = ");
		u32_to_dec(RCC_GetPCLK1Value(), dbg, 0);
		uart_print(dbg);
		uart_print(" Hz\r\n");
	}

	// 3. Bring up CAN1 (PA11/PA12 → SN65HVD230 → CAN bus)
	CAN1_GPIO_Init();
	CAN1_Init_500kbps();
	CAN1_Filter_Init();

	uart_print("[INIT] CAN1 ready, entering main loop...\r\n");

	// Dump CAN registers so we can see whether init / loopback / filters look right
	{
		char dbg[16];
		uart_print("[DBG] MCR=0x"); u32_to_hex(can1_handle.pCANx->MCR, dbg, 8); uart_print(dbg); uart_print("\r\n");
		uart_print("[DBG] MSR=0x"); u32_to_hex(can1_handle.pCANx->MSR, dbg, 8); uart_print(dbg); uart_print("\r\n");
		uart_print("[DBG] BTR=0x"); u32_to_hex(can1_handle.pCANx->BTR, dbg, 8); uart_print(dbg); uart_print("\r\n");
		uart_print("[DBG] TSR=0x"); u32_to_hex(can1_handle.pCANx->TSR, dbg, 8); uart_print(dbg); uart_print("\r\n");
		uart_print("[DBG] FA1R=0x"); u32_to_hex(CAN1->FA1R, dbg, 8); uart_print(dbg); uart_print("\r\n\r\n");
	}

	// 4. Pre-fill the TX message — only payload changes per iteration
	tx_msg.StdId = CAN_TX_ID;
	tx_msg.ExtId = 0;
	tx_msg.IDE   = CAN_ID_STD;
	tx_msg.RTR   = CAN_RTR_DATA;
	tx_msg.DLC   = 8;

	while(1){
		counter++;

		// Pack payload: bytes [0..3] = counter, bytes [4..7] = T0 timestamp
		t0 = micros();

		tx_msg.Data[0] = (uint8_t)(counter      );
		tx_msg.Data[1] = (uint8_t)(counter >>  8);
		tx_msg.Data[2] = (uint8_t)(counter >> 16);
		tx_msg.Data[3] = (uint8_t)(counter >> 24);
		tx_msg.Data[4] = (uint8_t)(t0           );
		tx_msg.Data[5] = (uint8_t)(t0      >>  8);
		tx_msg.Data[6] = (uint8_t)(t0      >> 16);
		tx_msg.Data[7] = (uint8_t)(t0      >> 24);

		// Send the frame (blocking — returns mailbox on success, CAN_TX_MAILBOX_NONE on failure)
		mailbox = CAN_SendMessage(&can1_handle, &tx_msg);

		if(mailbox == CAN_TX_MAILBOX_NONE){
			char dbg[16];
			uart_print("[ERR] TX failed | TSR=0x");
			u32_to_hex(can1_handle.pCANx->TSR, dbg, 8); uart_print(dbg);
			uart_print(" ESR=0x");
			u32_to_hex(can1_handle.pCANx->ESR, dbg, 8); uart_print(dbg);
			uart_print(" MSR=0x");
			u32_to_hex(can1_handle.pCANx->MSR, dbg, 8); uart_print(dbg);
			uart_print("\r\n");
			delay_ms(TX_PERIOD_MS);
			continue;
		}

		print_tx(counter, t0);

		// Wait (blocking) for the BeagleBone Black ACK on ID 0x200
		CAN_ReceiveMessage(&can1_handle, CAN_FIFO0, &rx_msg);

		t1 = micros();

		print_rx(counter, rx_msg.StdId, t1);

		// 32-bit subtract handles TIM2 wrap correctly
		latency = (uint32_t)(t1 - t0);
		print_latency(latency);

		// Wait until the next 100 ms slot
		delay_ms(TX_PERIOD_MS);
	}
}
