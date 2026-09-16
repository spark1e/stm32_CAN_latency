/*
 * stm32L4xx_port.h
 *
 *  Created on: Sep 12, 2026
 *      Author: Wolf
 */

#ifndef INC_STM32L4XX_PORT_H_
#define INC_STM32L4XX_PORT_H_

/*
 * stm32l4xx_port.h
 * Include AFTER stm32f4xx.h in L476 source files.
 * Overrides macros that differ between F4 and L4 using raw register access.
 * Avoids struct incompatibility — no new RCC_RegDef_t needed.
 */

/* ── Raw L476 RCC register access ─────────────────────────────
 * RCC base: 0x40021000 (same as F446)
 * Register offsets differ on L4 — use raw pointers to avoid struct clash
 */
#define L4_RCC_CR         (*((volatile uint32_t*)0x40021000))
#define L4_RCC_CFGR       (*((volatile uint32_t*)0x40021008))
#define L4_RCC_AHB2ENR    (*((volatile uint32_t*)0x4002104C))
#define L4_RCC_APB1ENR1   (*((volatile uint32_t*)0x40021058))
#define L4_RCC_APB1RSTR1  (*((volatile uint32_t*)0x40021038))

/* ── GPIO base addresses differ on L4 ─────────────────────────
 * F446: 0x40020xxx  →  L476: 0x48000xxx
 */
#undef GPIOA
#undef GPIOB
#undef GPIOC
#define GPIOA  ((GPIO_RegDef_t*)0x48000000)
#define GPIOB  ((GPIO_RegDef_t*)0x48000400)
#define GPIOC  ((GPIO_RegDef_t*)0x48000800)

/* ── GPIO clock macros — L4 uses AHB2ENR ──────────────────────
 * F446: RCC->AHB1ENR  →  L476: AHB2ENR (raw)
 */
#undef GPIOA_PCLK_EN
#undef GPIOB_PCLK_EN
#undef GPIOC_PCLK_EN
#define GPIOA_PCLK_EN()   (L4_RCC_AHB2ENR |=  (1 << 0))
#define GPIOB_PCLK_EN()   (L4_RCC_AHB2ENR |=  (1 << 1))
#define GPIOC_PCLK_EN()   (L4_RCC_AHB2ENR |=  (1 << 2))

/* ── CAN1 clock macros — L4 uses APB1ENR1 ─────────────────────
 * F446: RCC->APB1ENR  →  L476: APB1ENR1 (raw), bit 25 same
 */
#undef CAN1_PCLK_EN
#undef CAN1_PCLK_DI
#undef CAN1_REG_RESET
#define CAN1_PCLK_EN()    (L4_RCC_APB1ENR1 |=  (1 << 25))
#define CAN1_PCLK_DI()    (L4_RCC_APB1ENR1 &= ~(1 << 25))
#define CAN1_REG_RESET()  do{ L4_RCC_APB1RSTR1 |=  (1 << 25); \
                               L4_RCC_APB1RSTR1 &= ~(1 << 25); }while(0)
/* USART2 baud override for L476 on MSI 4 MHz
 * BRR = PCLK1 / baud = 4000000 / 115200 = 34.7 → 35 = 0x23
 */
#define L4_USART2_BRR_115200_MSI4   (0x23U)


/* ── USART2 clock macros — L4 uses APB1ENR1 ───────────────────
 * bit 17, same position, different register
 */
#undef USART2_PCLK_EN
#undef USART2_PCLK_DI
#define USART2_PCLK_EN()  (L4_RCC_APB1ENR1 |=  (1 << 17))
#define USART2_PCLK_DI()  (L4_RCC_APB1ENR1 &= ~(1 << 17))

/* ── HSI switch helper ─────────────────────────────────────────
 * L476 defaults to MSI 4 MHz — switch to HSI 16 MHz
 * so CAN bit timing matches F446 (BRP=2, TS1=12, TS2=1 → 500 kbps)
 */
static inline void l4_switch_to_hsi(void){
    L4_RCC_CR   |=  (1 << 8);              /* HSION */
    while(!(L4_RCC_CR & (1 << 10)));       /* wait HSIRDY */
    L4_RCC_CFGR &= ~(0x3U);               /* clear SW */
    L4_RCC_CFGR |=  (0x1U);               /* SW = HSI */
    while((L4_RCC_CFGR & 0xCU) != 0x4U);  /* wait SWS = HSI */
}

/* ── L476 USART2 raw TX (F4 driver несовместим — разный layout) ── */
#define L4_U2_BASE   0x40004400UL
#define L4_U2_CR1    (*((volatile uint32_t*)(L4_U2_BASE + 0x00)))
#define L4_U2_BRR    (*((volatile uint32_t*)(L4_U2_BASE + 0x0C)))
#define L4_U2_ISR    (*((volatile uint32_t*)(L4_U2_BASE + 0x1C)))
#define L4_U2_TDR    (*((volatile uint32_t*)(L4_U2_BASE + 0x28)))

static inline void l4_uart2_init(void){
    L4_RCC_APB1ENR1 |= (1 << 17);   /* USART2 clock */
    L4_U2_BRR = 35;                  /* 115200 @ MSI 4MHz: 4000000/115200 = 34.7 → 35 */
    L4_U2_CR1 = (1 << 3)|(1 << 0);  /* TE + UE */
}

static inline void l4_uart2_putc(char c){
    while(!(L4_U2_ISR & (1 << 7))); /* wait TXE */
    L4_U2_TDR = (uint8_t)c;
}

static inline void l4_uart2_print(const char *s){
    while(*s) l4_uart2_putc(*s++);
}

#endif /* INC_STM32L4XX_PORT_H_ */
