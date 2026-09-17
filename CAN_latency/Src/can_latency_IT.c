/*
	can_latency_IT.c

	Created on: Apr 2, 2026
	Author: Nikita Volkov (https://github.com/spark1e)

	Phase 2 test application: Interrupt-driven CAN latency measurement.
	T1 is recorded inside the CAN RX ISR - eliminates polling delay.

	Differences from can_latency_polling.c:
	- CAN1_RX0_IRQHandler records T1 immediately on frame arrival
	- Main loop waits on g_rx_done flag instead of blocking CAN_ReceiveMessage
	- All other logic identical - direct apples-to-apples comparison

	Pin assignments (STM32F446RE):
	PB8  - CAN1_RX (AF9)
	PB9  - CAN1_TX (AF9)
	PA2  - USART2_TX (AF7) -> ST-LINK virtual COM port
	PA3  - USART2_RX (AF7)
	TIM2 - 32-bit free-running 1 µs counter for timestamps

	CAN configuration:
	Bitrate  : 500 kbps
	Mode     : Normal
	Deadline : 9 ms (9000 µs)
	TX ID    : 0x100 (Node 1 -> Node 2)
	RX ID    : 0x200 (Node 2 -> Node 1, echo)
 */

#include <string.h>
#include "stm32f4xx.h"

/*
 * ============================================================
 * CONFIGURATION
 * ============================================================
 */
#define CAN_TX_ID       0x100U
#define CAN_RX_ID       0x200U
#define DEADLINE_US     9000U
#define TX_PERIOD_MS    100U

#define TIM2_BASE_ADDR  (APB1PERIPH_BASE + 0x0000U)
#define TIM2_CR1        (*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x00U))
#define TIM2_EGR        (*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x14U))
#define TIM2_CNT        (*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x24U))
#define TIM2_PSC        (*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x28U))
#define TIM2_ARR        (*(__vo uint32_t *)(TIM2_BASE_ADDR + 0x2CU))

#define TIM2_PRESCALER  15      /* 16 MHz / (15+1) = 1 MHz -> 1 µs/tick */
#define TIM_CR1_CEN     0

/*
 * ============================================================
 * GLOBAL HANDLES
 * ============================================================
 */
USART_Handle_t  uart2_handle;
CAN_Handle_t    can1_handle;

/* ISR-shared variables - volatile required */
volatile uint8_t  g_rx_done = 0;
volatile uint32_t g_t1      = 0;
CAN_RxMsg_t       g_rx_msg;

/*
 * ============================================================
 * UTILITY - UART
 * ============================================================
 */
static void uart_print(const char *str){
    USART_SendData(&uart2_handle, (uint8_t *)str, strlen(str));
}

static void u32_to_dec(uint32_t value, char *buf, uint8_t width){
    char tmp[11];
    int8_t i = 0;
    if(value == 0){ tmp[i++] = '0'; }
    else{ while(value > 0){ tmp[i++] = (char)('0' + (value % 10)); value /= 10; } }
    while(i < width){ tmp[i++] = '0'; }
    int8_t j;
    for(j = 0; j < i; j++){ buf[j] = tmp[i - 1 - j]; }
    buf[j] = '\0';
}

static void u32_to_hex(uint32_t value, char *buf, uint8_t width){
    const char hex[] = "0123456789ABCDEF";
    int8_t i;
    for(i = width - 1; i >= 0; i--){ buf[i] = hex[value & 0xFU]; value >>= 4; }
    buf[width] = '\0';
}

/*
 * ============================================================
 * TIM2 - 1 µs free-running counter
 * ============================================================
 */
static void TIM2_MicrosTimer_Init(void){
    RCC->RCC_APB1ENR |= (1 << 0);
    TIM2_PSC = TIM2_PRESCALER;
    TIM2_ARR = 0xFFFFFFFFU;
    TIM2_EGR = 0x1U;
    TIM2_CR1 |= (1 << TIM_CR1_CEN);
}

static inline uint32_t micros(void){ return TIM2_CNT; }

static void delay_us(uint32_t us){
    uint32_t start = micros();
    while((uint32_t)(micros() - start) < us);
}

static void delay_ms(uint32_t ms){ while(ms--){ delay_us(1000); } }

/*
 * ============================================================
 * GPIO
 * ============================================================
 */
static void CAN1_GPIO_Init(void){
    GPIO_Handle_t g;
    memset(&g, 0, sizeof(g));
    g.pGPIOx                            = GPIOB;
    g.GPIO_PinConfig.GPIO_PinMode       = GPIO_MODE_ALTFN;
    g.GPIO_PinConfig.GPIO_PinOPType     = GPIO_OT_TYPE_PP;
    g.GPIO_PinConfig.GPIO_PinSpeed      = GPIO_SPEED_HIGH;
    g.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PIN_PU;
    g.GPIO_PinConfig.GPIO_PinAltFunMode = 9;
    g.GPIO_PinConfig.GPIO_PinNumber     = GPIO_PIN_NO_9;  /* TX */
    GPIO_Init(&g);
    g.GPIO_PinConfig.GPIO_PinNumber     = GPIO_PIN_NO_8;  /* RX */
    GPIO_Init(&g);
}

static void UART2_GPIO_Init(void){
    GPIO_Handle_t g;
    memset(&g, 0, sizeof(g));
    g.pGPIOx                            = GPIOA;
    g.GPIO_PinConfig.GPIO_PinMode       = GPIO_MODE_ALTFN;
    g.GPIO_PinConfig.GPIO_PinOPType     = GPIO_OT_TYPE_PP;
    g.GPIO_PinConfig.GPIO_PinSpeed      = GPIO_SPEED_FAST;
    g.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PIN_PU;
    g.GPIO_PinConfig.GPIO_PinAltFunMode = 7;
    g.GPIO_PinConfig.GPIO_PinNumber     = GPIO_PIN_NO_2;  /* TX */
    GPIO_Init(&g);
    g.GPIO_PinConfig.GPIO_PinNumber     = GPIO_PIN_NO_3;  /* RX */
    GPIO_Init(&g);
}

/*
 * ============================================================
 * USART2
 * ============================================================
 */
static void UART2_Init(void){
    uart2_handle.pUSARTx                         = USART2;
    uart2_handle.USART_Config.USART_Baud         = USART_STD_BAUD_115200;
    uart2_handle.USART_Config.USART_HWFlowControl= USART_HW_FLOW_CTRL_NONE;
    uart2_handle.USART_Config.USART_Mode         = USART_MODE_TXRX;
    uart2_handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
    uart2_handle.USART_Config.USART_WordLength   = USART_WORDLEN_8BITS;
    uart2_handle.USART_Config.USART_ParityControl= USART_PARITY_DISABLE;
    USART_Init(&uart2_handle);
    USART_PeripheralControl(USART2, ENABLE);
}

/*
 * ============================================================
 * CAN1 - 500 kbps, Normal mode
 * ============================================================
 */
static void CAN1_Init_500kbps(void){
    can1_handle.pCANx                        = CAN1;
    can1_handle.CAN_Config.CAN_Mode          = CAN_MODE_NORMAL;
    can1_handle.CAN_Config.CAN_Prescaler     = CAN_PRESCALER_500KBPS_16MHZ;
    can1_handle.CAN_Config.CAN_TimeSeg1      = CAN_TS1_13TQ;
    can1_handle.CAN_Config.CAN_TimeSeg2      = CAN_TS2_2TQ;
    can1_handle.CAN_Config.CAN_SyncJumpWidth = CAN_SJW_1TQ;
    can1_handle.CAN_Config.CAN_TTCM         = DISABLE;
    can1_handle.CAN_Config.CAN_ABOM         = ENABLE;
    can1_handle.CAN_Config.CAN_AWUM         = DISABLE;
    can1_handle.CAN_Config.CAN_NART         = DISABLE;
    can1_handle.CAN_Config.CAN_RFLM         = DISABLE;
    can1_handle.CAN_Config.CAN_TXFP         = DISABLE;
    CAN_Init(&can1_handle);
}

/*
 * ============================================================
 * CAN1 FILTER - accept all frames into FIFO0
 * ============================================================
 */
static void CAN1_Filter_Init(void){
    CAN_FilterConfig_t f;
    f.FilterIdHigh         = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    f.FilterFIFOAssignment = CAN_FIFO0;
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterActivation     = ENABLE;
    CAN_ConfigFilter(&can1_handle, &f);
}

/*
 * ============================================================
 * CAN1 RX INTERRUPT - enable FIFO0 message pending IRQ
 * ============================================================
 */
static void CAN1_RX_Interrupt_Init(void){
    /* Enable FIFO0 message pending interrupt in CAN IER */
    can1_handle.pCANx->IER |= (1 << CAN_IER_FMPIE0);
    /* Enable CAN1_RX0 in NVIC (IRQ 20) */
    *NVIC_ISER0 |= (1 << IRQ_NO_CAN1_RX0);
}

/*
 * ============================================================
 * UART REPORT HELPERS
 * ============================================================
 */
static void print_banner(void){
    uart_print("\r\n");
    uart_print("==========================================\r\n");
    uart_print("  STM32F446RE CAN Latency - INTERRUPT\r\n");
    uart_print("  500 kbps | Deadline: 9 ms\r\n");
    uart_print("  TX ID: 0x100  |  RX ID: 0x200\r\n");
    uart_print("==========================================\r\n");
}

static void print_tx(uint32_t counter, uint32_t t0){
    char buf[16];
    uart_print("[TX] #"); u32_to_dec(counter, buf, 4); uart_print(buf);
    uart_print(" | ID:0x"); u32_to_hex(CAN_TX_ID, buf, 3); uart_print(buf);
    uart_print(" | T0:"); u32_to_dec(t0, buf, 10); uart_print(buf);
    uart_print(" us\r\n");
}

static void print_rx(uint32_t counter, uint32_t rx_id, uint32_t t1){
    char buf[16];
    uart_print("[RX] #"); u32_to_dec(counter, buf, 4); uart_print(buf);
    uart_print(" | ID:0x"); u32_to_hex(rx_id, buf, 3); uart_print(buf);
    uart_print(" | T1:"); u32_to_dec(t1, buf, 10); uart_print(buf);
    uart_print(" us\r\n");
}

static void print_latency(uint32_t latency_us){
    char buf[16];
    uart_print("[>>] Latency: "); u32_to_dec(latency_us, buf, 6); uart_print(buf);
    uart_print(" us | DEADLINE: ");
    uart_print(latency_us <= DEADLINE_US ? "MET\r\n" : "MISSED\r\n");
    uart_print("------------------------------------------\r\n");
}

/*
 * ============================================================
 * CAN1 RX0 ISR - records T1 immediately on frame arrival
 * ============================================================
 */
void CAN1_RX0_IRQHandler(void){
    g_t1 = micros();                                    /* timestamp first */
    CAN_ReceiveMessage(&can1_handle, CAN_FIFO0, &g_rx_msg);
    g_rx_done = 1;
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

    TIM2_MicrosTimer_Init();

    UART2_GPIO_Init();
    UART2_Init();
    print_banner();

    {
        char dbg[16];
        uart_print("[INIT] PCLK1 = ");
        u32_to_dec(RCC_GetPCLK1Value(), dbg, 0);
        uart_print(dbg);
        uart_print(" Hz\r\n");
    }

    CAN1_GPIO_Init();
    CAN1_Init_500kbps();
    CAN1_Filter_Init();
    CAN1_RX_Interrupt_Init();   /* enable CAN RX interrupt */

    uart_print("[INIT] CAN1 ready (interrupt mode), entering main loop...\r\n\r\n");

    tx_msg.StdId = CAN_TX_ID;
    tx_msg.ExtId = 0;
    tx_msg.IDE   = CAN_ID_STD;
    tx_msg.RTR   = CAN_RTR_DATA;
    tx_msg.DLC   = 8;

    while(1){
        counter++;

        /* 1. Fill mailbox 0 and 1 with background frames - non-blocking */
        can1_handle.pCANx->sTxMailBox[0].TIR  = (0x7FFU << 21);
        can1_handle.pCANx->sTxMailBox[0].TDTR = 8;
        can1_handle.pCANx->sTxMailBox[0].TDLR = 0xFFFFFFFF;
        can1_handle.pCANx->sTxMailBox[0].TDHR = 0xFFFFFFFF;
        can1_handle.pCANx->sTxMailBox[0].TIR |= (1 << 0);

        can1_handle.pCANx->sTxMailBox[1].TIR  = (0x7FFU << 21);
        can1_handle.pCANx->sTxMailBox[1].TDTR = 8;
        can1_handle.pCANx->sTxMailBox[1].TDLR = 0xFFFFFFFF;
        can1_handle.pCANx->sTxMailBox[1].TDHR = 0xFFFFFFFF;
        can1_handle.pCANx->sTxMailBox[1].TIR |= (1 << 0);

        /* 2. Mailbox 2 free - measurement frame */
        t0 = micros();

        tx_msg.Data[0] = (uint8_t)(counter      );
        tx_msg.Data[1] = (uint8_t)(counter >>  8);
        tx_msg.Data[2] = (uint8_t)(counter >> 16);
        tx_msg.Data[3] = (uint8_t)(counter >> 24);
        tx_msg.Data[4] = (uint8_t)(t0           );
        tx_msg.Data[5] = (uint8_t)(t0      >>  8);
        tx_msg.Data[6] = (uint8_t)(t0      >> 16);
        tx_msg.Data[7] = (uint8_t)(t0      >> 24);

        g_rx_done = 0;

        mailbox = CAN_SendMessage(&can1_handle, &tx_msg);

        if(mailbox == CAN_TX_MAILBOX_NONE){
            uart_print("[ERR] TX failed\r\n");
            continue;
        }

        /* 3. Burst while Node 2 is processing */
        CAN_TxMsg_t bg;
        bg.StdId = 0x7FF;
        bg.IDE   = CAN_ID_STD;
        bg.RTR   = CAN_RTR_DATA;
        bg.DLC   = 8;
        for(int i = 0; i < 8; i++) bg.Data[i] = 0xFF;

        for(int b = 0; b < 10; b++){
            CAN_SendMessage(&can1_handle, &bg);
        }

        print_tx(counter, t0);

        /* 4. Wait for ISR to signal frame received */
        while(!g_rx_done);

        t1     = g_t1;
        rx_msg = g_rx_msg;

        print_rx(counter, rx_msg.StdId, t1);
        latency = (uint32_t)(t1 - t0);
        print_latency(latency);
    }
}
