#include <stdint.h>

/* ── RCC ────────────────────────────────────── */
#define RCC_AHB2ENR   (*((volatile uint32_t*)0x4002104C))
#define RCC_APB1ENR1  (*((volatile uint32_t*)0x40021058))
#define RCC_APB1RSTR1 (*((volatile uint32_t*)0x40021038))
#define RCC_CR        (*((volatile uint32_t*)0x40021000))
#define RCC_CFGR      (*((volatile uint32_t*)0x40021008))

/* ── GPIOA ──────────────────────────────────── */
#define GPIOA_MODER   (*((volatile uint32_t*)0x48000000))
#define GPIOA_AFRL    (*((volatile uint32_t*)0x48000020))

/* ── GPIOB ──────────────────────────────────── */
#define GPIOB_MODER   (*((volatile uint32_t*)0x48000400))
#define GPIOB_AFRH    (*((volatile uint32_t*)0x48000424))

/* ── USART2 ─────────────────────────────────── */
#define U2_CR1  (*((volatile uint32_t*)0x40004400))
#define U2_BRR  (*((volatile uint32_t*)0x4000440C))
#define U2_ISR  (*((volatile uint32_t*)0x4000441C))
#define U2_TDR  (*((volatile uint32_t*)0x40004428))

/* ── CAN1 ───────────────────────────────────── */
#define CAN_BASE  0x40006400UL
#define CAN_MCR   (*((volatile uint32_t*)(CAN_BASE + 0x000)))
#define CAN_MSR   (*((volatile uint32_t*)(CAN_BASE + 0x004)))
#define CAN_BTR   (*((volatile uint32_t*)(CAN_BASE + 0x01C)))
#define CAN_RF0R  (*((volatile uint32_t*)(CAN_BASE + 0x00C)))
#define CAN_FMR   (*((volatile uint32_t*)(CAN_BASE + 0x200)))
#define CAN_FM1R  (*((volatile uint32_t*)(CAN_BASE + 0x204)))
#define CAN_FS1R  (*((volatile uint32_t*)(CAN_BASE + 0x20C)))
#define CAN_FFA1R (*((volatile uint32_t*)(CAN_BASE + 0x214)))
#define CAN_FA1R  (*((volatile uint32_t*)(CAN_BASE + 0x21C)))

/* Filter bank 0 registers */
#define CAN_F0R1  (*((volatile uint32_t*)(CAN_BASE + 0x240)))
#define CAN_F0R2  (*((volatile uint32_t*)(CAN_BASE + 0x244)))

/* RX FIFO0 mailbox */
#define CAN_RI0R  (*((volatile uint32_t*)(CAN_BASE + 0x1B0)))
#define CAN_RDT0R (*((volatile uint32_t*)(CAN_BASE + 0x1B4)))
#define CAN_RDL0R (*((volatile uint32_t*)(CAN_BASE + 0x1B8)))
#define CAN_RDH0R (*((volatile uint32_t*)(CAN_BASE + 0x1BC)))

/* TX mailbox 0 */
#define CAN_TI0R  (*((volatile uint32_t*)(CAN_BASE + 0x180)))
#define CAN_TDT0R (*((volatile uint32_t*)(CAN_BASE + 0x184)))
#define CAN_TDL0R (*((volatile uint32_t*)(CAN_BASE + 0x188)))
#define CAN_TDH0R (*((volatile uint32_t*)(CAN_BASE + 0x18C)))
#define CAN_TSR   (*((volatile uint32_t*)(CAN_BASE + 0x008)))

/* ── UART helpers ───────────────────────────── */
static void putc_(char c){
    while(!(U2_ISR & (1 << 7)));
    U2_TDR = (uint8_t)c;
}
static void print_(const char *s){
    while(*s) putc_(*s++);
}

/* ── Init functions ─────────────────────────── */
static void uart_init(uint32_t brr){
    RCC_AHB2ENR  |= (1 << 0);          /* GPIOA clock */
    GPIOA_MODER  &= ~(3U << 4);
    GPIOA_MODER  |=  (2U << 4);        /* PA2 = AF */
    GPIOA_AFRL   &= ~(0xFU << 8);
    GPIOA_AFRL   |=  (7U   << 8);      /* PA2 = AF7 (USART2) */
    RCC_APB1ENR1 |= (1 << 17);         /* USART2 clock */
    U2_CR1 = 0;
    U2_BRR = brr;
    U2_CR1 = (1 << 3)|(1 << 0);       /* TE + UE */
}

static void hsi_init(void){
    RCC_CR   |=  (1 << 8);             /* HSION */
    while(!(RCC_CR & (1 << 10)));      /* wait HSIRDY */
    RCC_CFGR &= ~(0x3U);
    RCC_CFGR |=  (0x1U);              /* SW = HSI */
    while((RCC_CFGR & 0xCU) != 0x4U); /* wait SWS = HSI */
}

static void can_gpio_init(void){
    RCC_AHB2ENR |= (1 << 1);           /* GPIOB clock */
    /* PB8 = CAN1_RX, PB9 = CAN1_TX → AF9 */
    GPIOB_MODER &= ~((3U << 16)|(3U << 18));
    GPIOB_MODER |=  ((2U << 16)|(2U << 18)); /* AF mode */
    GPIOB_AFRH  &= ~((0xFU << 0)|(0xFU << 4));
    GPIOB_AFRH  |=  ((9U   << 0)|(9U   << 4)); /* AF9 = CAN1 */
}

static void can_init(void){
    RCC_APB1ENR1 |= (1 << 25);         /* CAN1 clock */

    /* Exit sleep, enter init mode */
    CAN_MCR &= ~(1 << 1);              /* SLEEP=0 */
    CAN_MCR |=  (1 << 0);             /* INRQ=1 */
    while(!(CAN_MSR & (1 << 0)));      /* wait INAK */

    /* BTR: 500 kbps @ HSI 16MHz
     * BRP=2, TS1=12, TS2=1, SJW=0
     * → tq=125ns, bit=16tq=2µs → 500kbps */
    CAN_BTR = (0  << 24) |  /* SJW=1tq */
              (1  << 20) |  /* TS2=2tq */
              (12 << 16) |  /* TS1=13tq */
              (1  <<  0);   /* BRP=2 (prescaler=BRP+1=2) */

    /* Leave init mode */
    CAN_MCR &= ~(1 << 0);              /* INRQ=0 */
    while(CAN_MSR & (1 << 0));         /* wait INAK cleared */
}

static void can_filter_init(void){
    CAN_FMR  |=  (1 << 0);            /* FINIT=1 */
    CAN_FA1R &= ~(1 << 0);            /* deactivate bank 0 */
    CAN_FS1R |=  (1 << 0);            /* 32-bit scale */
    CAN_FM1R &= ~(1 << 0);            /* mask mode */
    CAN_F0R1  =  0x00000000;          /* accept all IDs */
    CAN_F0R2  =  0x00000000;
    CAN_FFA1R &= ~(1 << 0);           /* assign to FIFO0 */
    CAN_FA1R  |=  (1 << 0);           /* activate bank 0 */
    CAN_FMR   &= ~(1 << 0);           /* FINIT=0 */
}

/* ── MAIN ───────────────────────────────────── */
int main(void){
    /* Start on MSI 4MHz — init UART first */
    uart_init(35);   /* 115200 @ MSI 4MHz */
    print_("\r\n[Node 2] Starting on MSI 4MHz\r\n");

    /* Switch to HSI 16MHz */
    hsi_init();

    /* Re-init UART on HSI 16MHz */
    U2_CR1 = 0;
    U2_BRR = 139;    /* 115200 @ HSI 16MHz: 16000000/115200 = 138.9 → 139 */
    U2_CR1 = (1 << 3)|(1 << 0);
    print_("[Node 2] HSI 16MHz OK\r\n");

    /* CAN init */
    can_gpio_init();
    can_init();
    can_filter_init();
    print_("[Node 2] CAN ready — waiting for frames\r\n");

    while(1){
        /* Wait for message in FIFO0 */
        while(!(CAN_RF0R & 0x3));

        print_("[Node 2] Frame received!\r\n");

        /* Read frame */
        uint32_t id  = (CAN_RI0R >> 21) & 0x7FF;
        uint32_t dlc = CAN_RDT0R & 0xF;
        uint32_t dl  = CAN_RDL0R;
        uint32_t dh  = CAN_RDH0R;

        /* Release FIFO */
        CAN_RF0R |= (1 << 5);   /* RFOM0 */

        /* Wait TX mailbox 0 free (TME0 bit 26) */
        while(!(CAN_TSR & (1 << 26)));

        /* Send echo with ID+1 */
        CAN_TI0R = (0x200U << 21);
        CAN_TDT0R = dlc;
        CAN_TDL0R = dl;
        CAN_TDH0R = dh;
        CAN_TI0R |= (1 << 0);           /* TXRQ=1 — fire */

        print_("[Node 2] Echo sent\r\n");
    }
}
