/*
 * 020can_loopback_test.c
 *
 *  Created on: Sep 11, 2026
 *      Author: Nikita Volkov (https://github.com/spark1e)
 *
 *  CAN1 loopback smoke test — single board, no transceivers needed.
 *  CAN_MODE_LOOPBACK connects TX internally to RX inside the peripheral.
 *
 *  Pins (STM32F446RE Nucleo-64):
 *    PB8  — CAN1_RX (AF9)
 *    PB9  — CAN1_TX (AF9)
 *    PA2  — USART2_TX (AF7) → ST-LINK virtual COM port
 *    PA3  — USART2_RX (AF7)
 *
 *  Expected UART output (115200 8N1):
 *    [CAN LOOPBACK] Sending frame ID=0x65 DATA=DE AD BE EF 00 00 00 00
 *    [CAN LOOPBACK] Received ID=0x65 DATA=DE AD BE EF 00 00 00 00 -- PASS
 *
 */

#include <string.h>
#include "stm32f4xx.h"

/* ──────────────────────────────────────────────────
 * UART2 handle (needed for debug output)
 * ────────────────────────────────────────────────── */
static USART_Handle_t uart2;
static CAN_Handle_t   can1;

/* ──────────────────────────────────────────────────
 * Tiny UART helpers
 * ────────────────────────────────────────────────── */
static void uart_print(const char *s){
    USART_SendData(&uart2, (uint8_t *)s, strlen(s));
}

static void print_hex_byte(uint8_t b){
    const char hex[] = "0123456789ABCDEF";
    char buf[3];
    buf[0] = hex[b >> 4];
    buf[1] = hex[b & 0xF];
    buf[2] = '\0';
    uart_print(buf);
}

/* ──────────────────────────────────────────────────
 * GPIO init for UART2 (PA2 TX, PA3 RX, AF7)
 * ────────────────────────────────────────────────── */
static void uart2_gpio_init(void){
    GPIO_Handle_t g;
    memset(&g, 0, sizeof(g));
    g.pGPIOx                            = GPIOA;
    g.GPIO_PinConfig.GPIO_PinMode       = GPIO_MODE_ALTFN;
    g.GPIO_PinConfig.GPIO_PinOPType     = GPIO_OT_TYPE_PP;
    g.GPIO_PinConfig.GPIO_PinSpeed      = GPIO_SPEED_FAST;
    g.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PIN_PU;
    g.GPIO_PinConfig.GPIO_PinAltFunMode = 7;   /* AF7 = USART2 */

    g.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_2;  /* TX */
    GPIO_Init(&g);
    g.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;  /* RX */
    GPIO_Init(&g);
}

/* ──────────────────────────────────────────────────
 * USART2 init — 115200 8N1
 * ────────────────────────────────────────────────── */
static void uart2_init(void){
    uart2.pUSARTx                          = USART2;
    uart2.USART_Config.USART_Baud          = USART_STD_BAUD_115200;
    uart2.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
    uart2.USART_Config.USART_Mode          = USART_MODE_TXRX;
    uart2.USART_Config.USART_NoOfStopBits  = USART_STOPBITS_1;
    uart2.USART_Config.USART_WordLength    = USART_WORDLEN_8BITS;
    uart2.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;
    USART_Init(&uart2);
    USART_PeripheralControl(USART2, ENABLE);
}

/* ──────────────────────────────────────────────────
 * GPIO init for CAN1 (PB8 RX, PB9 TX, AF9)
 * ────────────────────────────────────────────────── */
static void can1_gpio_init(void){
    GPIO_Handle_t g;
    memset(&g, 0, sizeof(g));
    g.pGPIOx                            = GPIOB;
    g.GPIO_PinConfig.GPIO_PinMode       = GPIO_MODE_ALTFN;
    g.GPIO_PinConfig.GPIO_PinOPType     = GPIO_OT_TYPE_PP;
    g.GPIO_PinConfig.GPIO_PinSpeed      = GPIO_SPEED_FAST;
    g.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PIN_PU;
    g.GPIO_PinConfig.GPIO_PinAltFunMode = 9;   /* AF9 = CAN1/2 */

    g.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_8;  /* CAN1_RX */
    GPIO_Init(&g);
    g.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_9;  /* CAN1_TX */
    GPIO_Init(&g);
}

/* ──────────────────────────────────────────────────
 * CAN1 init — 500 kbps, LOOPBACK mode
 *
 * APB1 = 16 MHz (HSI default, no PLL)
 * BRP = 2 → tq = 2/16 MHz = 125 ns
 * TS1 = 13 tq, TS2 = 2 tq, SJW = 1 tq
 * Bit time = 1+13+2 = 16 tq = 2 µs → 500 kbps ✓
 * ────────────────────────────────────────────────── */
static void can1_init(void){
    can1.pCANx                      = CAN1;
    //can1.CAN_Config.CAN_Mode        = CAN_MODE_LOOPBACK;
    can1.CAN_Config.CAN_Mode = CAN_MODE_NORMAL;
    can1.CAN_Config.CAN_Prescaler   = 2;
    can1.CAN_Config.CAN_TimeSeg1    = 12;
    can1.CAN_Config.CAN_TimeSeg2    = 1;
    can1.CAN_Config.CAN_SyncJumpWidth = 0;
    can1.CAN_Config.CAN_TTCM        = DISABLE;
    can1.CAN_Config.CAN_ABOM        = DISABLE;
    can1.CAN_Config.CAN_AWUM        = DISABLE;
    can1.CAN_Config.CAN_NART        = DISABLE;
    can1.CAN_Config.CAN_RFLM        = DISABLE;
    can1.CAN_Config.CAN_TXFP        = DISABLE;
    CAN_Init(&can1);
}

/* ──────────────────────────────────────────────────
 * Filter — accept everything into FIFO0
 * 32-bit mask mode, ID=0 mask=0 → all IDs pass
 * ────────────────────────────────────────────────── */
static void can1_filter_init(void){
    CAN_FilterConfig_t f;
    memset(&f, 0, sizeof(f));
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh         = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    f.FilterFIFOAssignment = CAN_FIFO0;
    f.FilterActivation     = ENABLE;
    CAN_ConfigFilter(&can1, &f);
}


/* ──────────────────────────────────────────────────
 * MAIN
 * ────────────────────────────────────────────────── */
int main(void){
    uart2_gpio_init();
    uart2_init();

    uart_print("\r\n==========================================\r\n");
    uart_print("  CAN1 Normal Mode — Node 1 TX\r\n");
    uart_print("  500 kbps | PB8=RX  PB9=TX\r\n");
    uart_print("==========================================\r\n");

    can1_gpio_init();
    can1_init();
    can1_filter_init();

    CAN_TxMsg_t tx;
    CAN_RxMsg_t rx;
    uint32_t    counter = 0;

    while(1){
        counter++;

        /* Build frame */
        memset(&tx, 0, sizeof(tx));
        tx.StdId   = 0x065;
        tx.IDE     = CAN_ID_STD;
        tx.RTR     = CAN_RTR_DATA;
        tx.DLC     = 4;
        tx.Data[0] = (counter >> 24) & 0xFF;
        tx.Data[1] = (counter >> 16) & 0xFF;
        tx.Data[2] = (counter >>  8) & 0xFF;
        tx.Data[3] = (counter      ) & 0xFF;

        uart_print("[TX] Sending counter=");
        print_hex_byte(tx.Data[2]);
        print_hex_byte(tx.Data[3]);
        uart_print("\r\n");

        /* Send */
        uint8_t mb = CAN_SendMessage(&can1, &tx);
        if(mb == CAN_TX_MAILBOX_NONE){
            uart_print("[TX] FAILED -- check wiring / Node 2\r\n");
            /* small delay then retry */
            for(volatile int i = 0; i < 100000; i++);
            continue;
        }

        /* Wait for echo from Node 2 (ID = 0x066) */
        memset(&rx, 0, sizeof(rx));
        CAN_ReceiveMessage(&can1, CAN_FIFO0, &rx);

        if(rx.StdId == 0x066){
            uart_print("[RX] Echo received -- PASS\r\n");
        } else {
            uart_print("[RX] Wrong ID -- FAIL\r\n");
        }

        /* ~500ms gap */
        for(volatile int i = 0; i < 800000; i++);
    }
}
