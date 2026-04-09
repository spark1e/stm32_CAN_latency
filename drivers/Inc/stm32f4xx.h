/*
 * stm32f4xx.h
 *
 *  Created on: Feb 13, 2025
 *      Author: Wolf
 */

#ifndef INC_STM32F4XX_H_
#define INC_STM32F4XX_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define __vo volatile
#define __weak __attribute__((weak))

/**********************PROCESSOR SPECIFIC DETAILS*********************************
 *
 */
//ARM Cortex Mx Processor NVIC ISERx Register Addresses
#define NVIC_ISER0		((__vo uint32_t*)0xE000E100)
#define NVIC_ISER1		((__vo uint32_t*)0xE000E104)
#define NVIC_ISER2		((__vo uint32_t*)0xE000E108)
#define NVIC_ISER3		((__vo uint32_t*)0xE000E10C)


 //ARM Cortex Mx Processor NVIC ICERx Register Addresses
#define NVIC_ICER0		((__vo uint32_t*)0xE000E180)
#define NVIC_ICER1		((__vo uint32_t*)0xE000E184)
#define NVIC_ICER2		((__vo uint32_t*)0xE000E188)
#define NVIC_ICER3		((__vo uint32_t*)0xE000E18C)

 //ARM Cortex Mx Processor Priority Register Address
#define NVIC_PR_BASEADDR	((__vo uint32_t*)0xE000E400)

//ARM Cortex Mx Processor number of priority bits implemented in Priority Register
#define NO_PR_BITS_IMPLEMENTED 4


#define FLASH_BASEADDR	0x08000000U
#define SRAM1_BASEADDR	0x20000000U
#define SRAM2_BASEADDR	0x2001C000U
#define SRAM 			SRAM1_BASEADDR
#define ROM_BASEADDR	0x1FFF0000U


//APBx and AHBx Bus Peripheral base addresses
#define PERIPH_BASE		0x40000000U
#define APB1PERIPH_BASE	PERIPH_BASE
#define APB2PERIPH_BASE	0x40010000U
#define AHB1PERIPH_BASE	0x40020000U
#define AHB2PERIPH_BASE	0x50000000U

//Base addresses of peripherals that are hanging on AHB1 bus
#define GPIOA_BASEADDR	(AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASEADDR	(AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASEADDR	(AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASEADDR	(AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASEADDR	(AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASEADDR	(AHB1PERIPH_BASE + 0x1400)
#define GPIOG_BASEADDR	(AHB1PERIPH_BASE + 0x1800)
#define GPIOH_BASEADDR	(AHB1PERIPH_BASE + 0x1C00)

#define RCC_BASEADDR 	((AHB1PERIPH_BASE + 0x3800))

//Base addresses of peripherals that are hanging on APB1 bus
#define I2C1_BASEADDR	(APB1PERIPH_BASE + 0x5400)
#define I2C2_BASEADDR	(APB1PERIPH_BASE + 0x5800)
#define I2C3_BASEADDR	(APB1PERIPH_BASE + 0x5C00)

#define SPI2_BASEADDR	(APB1PERIPH_BASE + 0x3800)
#define SPI3_BASEADDR	(APB1PERIPH_BASE + 0x3C00)
#define USART2_BASEADDR	(APB1PERIPH_BASE + 0x4400)
#define USART3_BASEADDR	(APB1PERIPH_BASE + 0x4800)
#define UART4_BASEADDR	(APB1PERIPH_BASE + 0x4C00)
#define UART5_BASEADDR	(APB1PERIPH_BASE + 0x5000)
#define UART7_BASEADDR	(APB1PERIPH_BASE + 0x7800)
#define UART8_BASEADDR	(APB1PERIPH_BASE + 0x7C00)
#define CAN1_BASEADDR	(APB1PERIPH_BASE + 0x6400)
#define CAN2_BASEADDR	(APB1PERIPH_BASE + 0x6800)

//Base addresses of peripherals that are hanging on APB2 bus
#define EXTI_BASEADDR	(APB2PERIPH_BASE + 0x3C00)
#define SPI1_BASEADDR	(APB2PERIPH_BASE + 0x3000)
#define SPI4_BASEADDR	(APB2PERIPH_BASE + 0x3400)
#define USART1_BASEADDR	(APB2PERIPH_BASE + 0x1000)
#define USART6_BASEADDR	(APB2PERIPH_BASE + 0x1400)
#define SYSCFG_BASEADDR	(APB2PERIPH_BASE + 0x3800)


/****************PERIPHERAL REGISTER DEFINITION STRUCTURES****************/

typedef struct
{
	__vo uint32_t MODER;		//GPIO port mode register				OFFSET: 0x00
	__vo uint32_t OTYPER;		//GPIO port output type register		OFFSET: 0x04
	__vo uint32_t OSPEEDER;		//GPIO port output speed register		OFFSET: 0x08
	__vo uint32_t PUPDR;		//GPIO port pull-up/pull-down register	OFFSET:	0x0C
	__vo uint32_t IDR;			//GPIO port input data register			OFFSET: 0x10
	__vo uint32_t ODR;			//GPIO port output data register		OFFSET: 0x14
	__vo uint32_t BSRR;			//GPIO port bit set/reset register		OFFSET: 0x18
	__vo uint32_t LCKR;			//GPIO port configuration lock register	OFFSET: 0x1C
	__vo uint32_t AFRL;			//GPIO alternate function low register	OFFSET: 0x20
	__vo uint32_t AFRH;			//GPIO alternate function high register	OFFSET: 0x24

}GPIO_RegDef_t;

typedef struct{
	__vo uint32_t RCC_CR;		//RCC clock control register			OFFSET: 0x00
	__vo uint32_t RCC_PLLCFGR;	//RCC PLL configuration register 		OFFSET: 0x04
	__vo uint32_t RCC_CFGR;		//RCC clock configuration register		OFFSET: 0x08
	__vo uint32_t RCC_CIR;		//RCC clock interrupt register			OFFSET: 0x0C
	__vo uint32_t RCC_AHB1RSTR;	//RCC AHB1 peripheral reset register	OFFSET: 0x10
	__vo uint32_t RCC_AHB2RSTR;	//RCC AHB2 peripheral reset register	OFFSET: 0x14
	__vo uint32_t RCC_AHB3RSTR;	//RCC AHB3 peripheral reset register	OFFSET: 0x18
	uint32_t RESERVED0;			//Reserved at 0x1C
	__vo uint32_t RCC_APB1RSTR;	//RCC APB1 peripheral reset register	OFFSET: 0x20
	__vo uint32_t RCC_APB2RSTR;	//RCC APB2 peripheral reset register	OFFSET: 0x24
	uint32_t RESERVED1[2];		//Reserved at 0x28-0x2C
	__vo uint32_t RCC_AHB1ENR;	//RCC AHB1 peripheral clock enable register		OFFSET: 0x30
	__vo uint32_t RCC_AHB2ENR;	//RCC AHB2 peripheral clock enable register		OFFSET: 0x34
	__vo uint32_t RCC_AHB3ENR;	//RCC AHB3 peripheral clock enable register		OFFSET: 0x38
	uint32_t RESERVED2;			//Reserved at 0x3C
	__vo uint32_t RCC_APB1ENR;	//RCC APB1 peripheral clock enable register		OFFSET: 0x40
	__vo uint32_t RCC_APB2ENR;	//RCC APB2 peripheral clock enable register		OFFSET: 0x44
	uint32_t RESERVED3[2];		//Reserved at 0x48-0x4C
	__vo uint32_t RCC_AHB1LPENR;//RCC AHB1 peripheral clock enable in low power mode register	OFFSET: 0x50
	__vo uint32_t RCC_AHB2LPENR;//RCC AHB2 peripheral clock enable in low power mode register	OFFSET: 0x54
	__vo uint32_t RCC_AHB3LPENR;//RCC AHB3 peripheral clock enable in low power mode register	OFFSET: 0x58
	uint32_t RESERVED4;			//Reserved at 0x5C
	__vo uint32_t RCC_APB1LPENR;//RCC APB1 peripheral clock enable in low power mode register	OFFSET: 0x60
	__vo uint32_t RCC_APB2LPENR;//RCC APB2 peripheral clock enabled in low power mode register	OFFSET: 0x64
	uint32_t RESERVED5[2];		//Reserved at 0x68-0x6C
	__vo uint32_t RCC_BDCR;		//RCC Backup domain control register	OFFSET: 0x70
	__vo uint32_t RCC_CSR;		//RCC clock control & status register	OFFSET: 0x74
	uint32_t RESERVED6[2];		//Reserved at 0x78-0x7C
	__vo uint32_t RCC_SSCGR;	//RCC spread spectrum clock generation register	OFFSET: 0x80
	__vo uint32_t RCC_PLLI2SCFGR;	//RCC PLLI2S configuration register		OFFSET: 0x84
	__vo uint32_t RCC_PLLSAICFGR;	//RCC PLL configuration register		OFFSET: 0x88
	__vo uint32_t RCC_DCKCFGR;		//RCC dedicated clock configuration register	OFFSET: 0x8C
	__vo uint32_t CKGATENR;		//RCC clocks gated enable register					OFFSET: 0x90
	__vo uint32_t DCKCFGR2;		//RCC dedicated clocks configuration register 2		OFFSET: 0x94
}RCC_RegDef_t;


/*
 * Peripheral register definition structure for EXTI
 *
 * */
typedef struct{
	__vo uint32_t EXTI_IMR;		//Interrupt mask register				OFFSET: 0x00
	__vo uint32_t EXTI_EMR;		//Event mask register					OFFSET: 0x04
	__vo uint32_t EXTI_RTSR;	//Rising trigger selection register		OFFSET: 0x08
	__vo uint32_t EXTI_FTSR;	//Falling trigger selection register	OFFSET:	0x0C
	__vo uint32_t EXTI_SWIER;	//Software interrupt event register		OFFSET: 0x10
	__vo uint32_t EXTI_PR;		//Pending register						OFFSET: 0x14
}EXTI_RegDef_t;

typedef struct{
	__vo uint32_t SYSCFG_MEMRMP;	//SYSCFG memory remap register								OFFSET: 0x00
	__vo uint32_t SYSCFG_PMC;		//SYSCFG peripheral mode configuration register				OFFSET: 0x04
	__vo uint32_t SYSCFG_EXTICR[4];	//SYSCFG external interrupt configuration register 1 - 4	OFFSET: 0x08-0x14
	uint32_t RESERVED0[2];			//															OFFSET: 0x18-0x1C
	__vo uint32_t SYSCFG_CMPCR;		//Compensation cell control register						OFFSET: 0x20
	uint32_t RESERVED1[2];			//															OFFSET: 0x24-0x28
	__vo uint32_t SYSCFG_CFGR;		//SYSCFG configuration register								OFFSET: 0x2C
}SYSCFG_RegDef_t;

typedef struct{
	__vo uint32_t SPI_CR1;			//SPI control register 1									OFFSET: 0x00
	__vo uint32_t SPI_CR2;			//SPI control register 2									OFFSET: 0x04
	__vo uint32_t SPI_SR;			//SPI status register										OFFSET: 0x08
	__vo uint32_t SPI_DR;			//SPI data register											OFFSET: 0x0C
	__vo uint32_t SPI_CRCPR;		//SPI CRC polynomial register								OFFSET: 0x10
	__vo uint32_t SPI_RXCRCR;		//SPI RX CRC register										OFFSET: 0x14
	__vo uint32_t SPI_TXCRCR;		//SPI TX CRC register										OFFSET: 0x18
	__vo uint32_t SPI_I2SCFGR;		//SPI_I2S configuration register							OFFSET: 0x1C
	__vo uint32_t SPI_I2SPR;		//SPI_I2S prescaler register								OFFSET: 0x20
}SPI_RegDef_t;

typedef struct{
	__vo uint32_t I2C_CR1;				//I2C control register 1						OFFSET:0x00
	__vo uint32_t I2C_CR2;				//I2C control register 2						OFFSET:0x04
	__vo uint32_t I2C_OAR1;				//I2C own address register 1					OFFSET:0x08
	__vo uint32_t I2C_OAR2;				//I2C own address register 2					OFFSET:0x0C
	__vo uint32_t I2C_DR;				//I2C data register								OFFSET:0x10
	__vo uint32_t I2C_SR1;				//I2C status register 1							OFFSET:0x14
	__vo uint32_t I2C_SR2;				//I2C status register 2							OFFSET:0x18
	__vo uint32_t I2C_CCR;				//I2C clock control register					OFFSET:0x1C
	__vo uint32_t I2C_TRISE;			//I2C TRISE register							OFFSET:0x20
	__vo uint32_t I2C_FLTR;				//I2C FLTR register								OFFSET:0x24
}I2C_RegDef_t;

typedef struct{
	__vo uint32_t USART_SR;				//Status register								OFFSET:0x00
	__vo uint32_t USART_DR;				//Data register									OFFSET:0x04
	__vo uint32_t USART_BRR;			//Baud rate register							OFFSET:0x08
	__vo uint32_t USART_CR1;			//Control register 1							OFFSET:0x0C
	__vo uint32_t USART_CR2;			//Control register 2							OFFSET:0x10
	__vo uint32_t USART_CR3;			//Control register 3							OFFSET:0x14
	__vo uint32_t USART_GTPR;			//Guard time and prescaler register				OFFSET:0x18
}USART_RegDef_t;

/*
 * CAN TX mailbox register set (one instance per mailbox, 3 mailboxes total)
 */
typedef struct{
	__vo uint32_t TIR;		//TX mailbox identifier register				OFFSET: 0x00 relative to mailbox base
	__vo uint32_t TDTR;		//TX mailbox data length & timestamp register	OFFSET: 0x04
	__vo uint32_t TDLR;		//TX mailbox data low register  (bytes 0-3)		OFFSET: 0x08
	__vo uint32_t TDHR;		//TX mailbox data high register (bytes 4-7)		OFFSET: 0x0C
}CAN_TxMailBox_t;

/*
 * CAN RX FIFO mailbox register set (one instance per FIFO, 2 FIFOs total)
 */
typedef struct{
	__vo uint32_t RIR;		//RX FIFO mailbox identifier register			OFFSET: 0x00 relative to FIFO base
	__vo uint32_t RDTR;		//RX FIFO mailbox data length & timestamp		OFFSET: 0x04
	__vo uint32_t RDLR;		//RX FIFO mailbox data low register  (bytes 0-3)	OFFSET: 0x08
	__vo uint32_t RDHR;		//RX FIFO mailbox data high register (bytes 4-7)	OFFSET: 0x0C
}CAN_FIFOMailBox_t;

/*
 * CAN filter bank register pair (one pair per filter bank, 28 banks on CAN1)
 */
typedef struct{
	__vo uint32_t FR1;		//Filter bank register 1 — ID or ID/mask (low word)
	__vo uint32_t FR2;		//Filter bank register 2 — mask or ID2     (high word)
}CAN_FilterRegister_t;

/*
 * Peripheral register definition structure for CAN1 / CAN2
 * Register map from RM0390 section 32.9
 */
typedef struct{
	__vo uint32_t         MCR;				//Master control register					OFFSET: 0x000
	__vo uint32_t         MSR;				//Master status register					OFFSET: 0x004
	__vo uint32_t         TSR;				//Transmit status register					OFFSET: 0x008
	__vo uint32_t         RF0R;				//Receive FIFO 0 register					OFFSET: 0x00C
	__vo uint32_t         RF1R;				//Receive FIFO 1 register					OFFSET: 0x010
	__vo uint32_t         IER;				//Interrupt enable register					OFFSET: 0x014
	__vo uint32_t         ESR;				//Error status register						OFFSET: 0x018
	__vo uint32_t         BTR;				//Bit timing register						OFFSET: 0x01C
	uint32_t              RESERVED0[88];	//Reserved									OFFSET: 0x020 - 0x17F
	CAN_TxMailBox_t       sTxMailBox[3];	//TX mailboxes 0-2							OFFSET: 0x180 - 0x1AC
	CAN_FIFOMailBox_t     sFIFOMailBox[2];	//RX FIFO mailboxes 0-1						OFFSET: 0x1B0 - 0x1CC
	uint32_t              RESERVED1[12];	//Reserved									OFFSET: 0x1D0 - 0x1FF
	__vo uint32_t         FMR;				//Filter master register					OFFSET: 0x200
	__vo uint32_t         FM1R;				//Filter mode register						OFFSET: 0x204
	uint32_t              RESERVED2;		//Reserved									OFFSET: 0x208
	__vo uint32_t         FS1R;				//Filter scale register						OFFSET: 0x20C
	uint32_t              RESERVED3;		//Reserved									OFFSET: 0x210
	__vo uint32_t         FFA1R;			//Filter FIFO assignment register			OFFSET: 0x214
	uint32_t              RESERVED4;		//Reserved									OFFSET: 0x218
	__vo uint32_t         FA1R;				//Filter activation register				OFFSET: 0x21C
	uint32_t              RESERVED5[8];		//Reserved									OFFSET: 0x220 - 0x23F
	CAN_FilterRegister_t  sFilterRegister[28]; //Filter bank registers (28 banks)		OFFSET: 0x240 - 0x31F
}CAN_RegDef_t;


//peripheral definitions (Peripheral base addresses typecasted to xxx_RegDef_t)
#define GPIOA		((GPIO_RegDef_t*) GPIOA_BASEADDR)
#define GPIOB		((GPIO_RegDef_t*) GPIOB_BASEADDR)
#define GPIOC		((GPIO_RegDef_t*) GPIOC_BASEADDR)
#define GPIOD		((GPIO_RegDef_t*) GPIOD_BASEADDR)
#define GPIOE		((GPIO_RegDef_t*) GPIOE_BASEADDR)
#define GPIOF		((GPIO_RegDef_t*) GPIOF_BASEADDR)
#define GPIOG		((GPIO_RegDef_t*) GPIOG_BASEADDR)
#define GPIOH		((GPIO_RegDef_t*) GPIOH_BASEADDR)

#define RCC			((RCC_RegDef_t*) RCC_BASEADDR)

#define EXTI 		((EXTI_RegDef_t*) EXTI_BASEADDR)

#define SYSCFG 		((SYSCFG_RegDef_t*) SYSCFG_BASEADDR)

#define SPI1		((SPI_RegDef_t*) SPI1_BASEADDR)
#define SPI2		((SPI_RegDef_t*) SPI2_BASEADDR)
#define SPI3		((SPI_RegDef_t*) SPI3_BASEADDR)
#define SPI4		((SPI_RegDef_t*) SPI4_BASEADDR)

#define I2C1		((I2C_RegDef_t*) I2C1_BASEADDR)
#define I2C2		((I2C_RegDef_t*) I2C2_BASEADDR)
#define I2C3		((I2C_RegDef_t*) I2C3_BASEADDR)

#define USART1		((USART_RegDef_t*) USART1_BASEADDR)
#define USART2		((USART_RegDef_t*) USART2_BASEADDR)
#define USART3		((USART_RegDef_t*) USART3_BASEADDR)
#define UART4		((USART_RegDef_t*) UART4_BASEADDR)
#define UART5		((USART_RegDef_t*) UART5_BASEADDR)
#define USART6		((USART_RegDef_t*) USART6_BASEADDR)
#define UART7		((USART_RegDef_t*) UART7_BASEADDR)
#define UART8		((USART_RegDef_t*) UART8_BASEADDR)

#define CAN1		((CAN_RegDef_t*) CAN1_BASEADDR)
#define CAN2		((CAN_RegDef_t*) CAN2_BASEADDR)



//Clock Enable Macros for GPIOx Peripherals
#define GPIOA_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 0))
#define GPIOB_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 1))
#define GPIOC_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 2))
#define GPIOD_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 3))
#define GPIOE_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 4))
#define GPIOF_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 5))
#define GPIOG_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 6))
#define GPIOH_PCLK_EN()	(RCC->RCC_AHB1ENR |= (1 << 7))
//Clock Enable Macros for I2Cx Peripherals
#define I2C1_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 21))
#define I2C2_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 22))
#define I2C3_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 23))
//Clock Enable Macros for SPIx Peripherals
#define SPI1_PCLK_EN() (RCC->RCC_APB2ENR |= (1 << 12))
#define SPI2_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 14))
#define SPI3_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 15))
#define SPI4_PCLK_EN() (RCC->RCC_APB2ENR |= (1 << 13))
//Clock Enable Macros for USARTx Peripherals
#define USART1_PCLK_EN() (RCC->RCC_APB2ENR |= (1 << 4))
#define USART2_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 17))
#define USART3_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 18))
#define UART4_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 19))
#define UART5_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 20))
#define USART6_PCLK_EN() (RCC->RCC_APB2ENR |= (1 << 5))
#define UART7_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 30))
#define UART8_PCLK_EN() (RCC->RCC_APB1ENR |= (1 << 31))
//Clock Enable Macros for CANx Peripherals
#define CAN1_PCLK_EN()  (RCC->RCC_APB1ENR |= (1 << 25))
#define CAN2_PCLK_EN()  (RCC->RCC_APB1ENR |= (1 << 26))
//Clock Enable for SYSCFG Peripheral
#define SYSCFG_PCLK_EN() (RCC->RCC_APB2ENR |= (1 << 14))


//Clock Disable Macros for GPIOx Peripherals
#define GPIOA_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 0))
#define GPIOB_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 1))
#define GPIOC_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 2))
#define GPIOD_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 3))
#define GPIOE_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 4))
#define GPIOF_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 5))
#define GPIOG_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 6))
#define GPIOH_PCLK_DI()	(RCC->RCC_AHB1ENR &= ~(1 << 7))
//Clock Disable Macros for I2Cx Peripherals
#define I2C1_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 21))
#define I2C2_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 22))
#define I2C3_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 23))
//Clock Disable Macros for SPIx Peripherals
#define SPI1_PCLK_DI() (RCC->RCC_APB2ENR &= ~(1 << 12))
#define SPI2_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 14))
#define SPI3_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 15))
#define SPI4_PCLK_DI() (RCC->RCC_APB2ENR &= ~(1 << 13))
//Clock Disable Macros for USARTx Peripherals
#define USART1_PCLK_DI() (RCC->RCC_APB2ENR &= ~(1 << 4))
#define USART2_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 17))
#define USART3_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 18))
#define UART4_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 19))
#define UART5_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 20))
#define USART6_PCLK_DI() (RCC->RCC_APB2ENR &= ~(1 << 5))
#define UART7_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 30))
#define UART8_PCLK_DI() (RCC->RCC_APB1ENR &= ~(1 << 31))
//Clock Disable Macros for CANx Peripherals
#define CAN1_PCLK_DI()  (RCC->RCC_APB1ENR &= ~(1 << 25))
#define CAN2_PCLK_DI()  (RCC->RCC_APB1ENR &= ~(1 << 26))
//Clock Disable for SYSCFG Peripheral
#define SYSCFG_PCLK_DI() (RCC->RCC_APB2ENR &= ~(1 << 14))

//Macros to reset GPIOx peripherals
#define GPIOA_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 0)); (RCC->RCC_AHB1RSTR &= ~(1 << 0));} while(0)
#define GPIOB_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 1)); (RCC->RCC_AHB1RSTR &= ~(1 << 1));} while(0)
#define GPIOC_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 2)); (RCC->RCC_AHB1RSTR &= ~(1 << 2));} while(0)
#define GPIOD_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 3)); (RCC->RCC_AHB1RSTR &= ~(1 << 3));} while(0)
#define GPIOE_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 4)); (RCC->RCC_AHB1RSTR &= ~(1 << 4));} while(0)
#define GPIOF_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 5)); (RCC->RCC_AHB1RSTR &= ~(1 << 5));} while(0)
#define GPIOG_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 6)); (RCC->RCC_AHB1RSTR &= ~(1 << 6));} while(0)
#define GPIOH_REG_RESET()  do{(RCC->RCC_AHB1RSTR |= (1 << 7)); (RCC->RCC_AHB1RSTR &= ~(1 << 7));} while(0)
//Macros to reset SPIx peripherals
#define SPI1_REG_RESET() do{(RCC->RCC_APB2RSTR |= (1 << 12)); (RCC->RCC_APB2RSTR &= ~(1 << 12));} while(0)
#define SPI4_REG_RESET() do{(RCC->RCC_APB2RSTR |= (1 << 13)); (RCC->RCC_APB2RSTR &= ~(1 << 13));} while(0)
#define SPI2_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 14)); (RCC->RCC_APB1RSTR &= ~(1 << 14));} while(0)
#define SPI3_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 15)); (RCC->RCC_APB1RSTR &= ~(1 << 15));} while(0)
//Macros to reset I2Cx peripherals
#define I2C1_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 21)); (RCC->RCC_APB1RSTR &= ~(1 << 21));} while(0)
#define I2C2_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 22)); (RCC->RCC_APB1RSTR &= ~(1 << 22));} while(0)
#define I2C3_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 23)); (RCC->RCC_APB1RSTR &= ~(1 << 23));} while(0)
//Macros to reset USARTx peripherals
#define USART1_REG_RESET() do{(RCC->RCC_APB2RSTR |= (1 << 4)); (RCC->RCC_APB2RSTR &= ~(1 << 4));} while(0)
#define USART2_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 17)); (RCC->RCC_APB1RSTR &= ~(1 << 17));} while(0)
#define USART3_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 18)); (RCC->RCC_APB1RSTR &= ~(1 << 18));} while(0)
#define UART4_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 19)); (RCC->RCC_APB1RSTR &= ~(1 << 19));} while(0)
#define UART5_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 20)); (RCC->RCC_APB1RSTR &= ~(1 << 20));} while(0)
#define USART6_REG_RESET() do{(RCC->RCC_APB2RSTR |= (1 << 5)); (RCC->RCC_APB2RSTR &= ~(1 << 5));} while(0)
#define UART7_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 30)); (RCC->RCC_APB1RSTR &= ~(1 << 30));} while(0)
#define UART8_REG_RESET() do{(RCC->RCC_APB1RSTR |= (1 << 31)); (RCC->RCC_APB1RSTR &= ~(1 << 31));} while(0)
//Macros to reset CANx peripherals
#define CAN1_REG_RESET()  do{(RCC->RCC_APB1RSTR |= (1 << 25)); (RCC->RCC_APB1RSTR &= ~(1 << 25));} while(0)
#define CAN2_REG_RESET()  do{(RCC->RCC_APB1RSTR |= (1 << 26)); (RCC->RCC_APB1RSTR &= ~(1 << 26));} while(0)




//Returns portcode (0-7) for a given GPIO base address(x)
#define GPIO_BASE_ADDR_TO_CODE(x) ( (x ==GPIOA)?0: (x ==GPIOB)?1: (x ==GPIOC)?2:(x ==GPIOD)?3:(x ==GPIOE)?4: (x ==GPIOF)?5: (x ==GPIOG)?6: (x ==GPIOH)?7:0 )

/*
 * IRQ (Interrupt Request) Numbers of STM32F4x MCU
 */

#define IRQ_NO_EXTI0		6
#define IRQ_NO_EXTI1		7
#define IRQ_NO_EXTI2		8
#define IRQ_NO_EXTI3		9
#define IRQ_NO_EXTI4		10
#define IRQ_NO_EXTI9_5		23
#define IRQ_NO_EXTI15_10	40
#define IRQ_NO_SPI1			35
#define IRQ_NO_SPI2			36
#define IRQ_NO_SPI3			51
#define IRQ_NO_SPI4			84
#define IRQ_NO_I2C1_EV		31
#define IRQ_NO_I2C1_ER		32
#define IRQ_NO_I2C2_EV		33
#define IRQ_NO_I2C2_ER		34
#define IRQ_NO_I2C3_EV		72
#define IRQ_NO_I2C3_ER		73
#define IRQ_NO_CAN1_TX		19
#define IRQ_NO_CAN1_RX0		20
#define IRQ_NO_CAN1_RX1		21
#define IRQ_NO_CAN1_SCE		22
#define IRQ_NO_CAN2_TX		63
#define IRQ_NO_CAN2_RX0		64
#define IRQ_NO_CAN2_RX1		65
#define IRQ_NO_CAN2_SCE		66





//IRQ priority numbers
#define IRQ_PRIORITY_0		0
#define IRQ_PRIORITY_15		15


// Generic Macros
#define ENABLE 1
#define DISABLE 0
#define SET ENABLE
#define RESET DISABLE
#define GPIO_PIN_SET SET
#define GPIO_PIN_RESET RESET
#define FLAG_RESET	RESET
#define FLAG_SET	SET


//////////////////////////////////////////////////////
//Bit position definitions of SPI peripheral
///////////////////////////////////////////////////////

//Bit position definitions for SPI_CR1
#define SPI_CR1_CPHA 		0
#define SPI_CR1_CPOL 		1
#define SPI_CR1_MSTR 		2
#define SPI_CR1_BR	 		3
#define SPI_CR1_SPE	 		6
#define SPI_CR1_LSBFIRST	7
#define SPI_CR1_SSI	 		8
#define SPI_CR1_SSM 		9
#define SPI_CR1_RXONLY 		10
#define SPI_CR1_DFF 		11
#define SPI_CR1_CRCNEXT	 	12
#define SPI_CR1_CRCEN 		13
#define SPI_CR1_BIDIOE	 	14
#define SPI_CR1_BIDIMODE 	15

//Bit position definitions for SPI_CR2
#define SPI_CR2_RXDMAEN		0
#define SPI_CR2_TXDMAEN		1
#define SPI_CR2_SSOE		2
//RESERVED 3
#define SPI_CR2_FRF			4
#define SPI_CR2_ERRIE		5
#define SPI_CR2_RXNEIE		6
#define SPI_CR2_TXEIE		7

//Bit position definitions for SPI_SR
#define SPI_SR_RXNE			0
#define SPI_SR_TXE			1
#define SPI_SR_CHSIDE		2
#define SPI_SR_UDR			3
#define SPI_SR_CRCERR		4
#define SPI_SR_MODF			5
#define SPI_SR_OVR			6
#define SPI_SR_BSY			7
#define SPI_SR_FRE			8


////////////////////////////////////////////////////////////
//Bit position definitions of I2C peripheral////////////////
////////////////////////////////////////////////////////////
//Bit positions in I2C_CR1
#define I2C_CR1_PE				0
#define I2C_CR1_NOSTRETCH		7
#define I2C_CR1_START			8
#define I2C_CR1_STOP			9
#define I2C_CR1_ACK				10
#define I2C_CR1_SWRST			15
//Bit position in I2C_CR2
#define I2C_CR2_FREQ			0
#define I2C_CR2_ITERREN			8
#define I2C_CR2_ITEVTEN			9
#define I2C_CR2_ITBUFEN			10
//Bit position in I2C_SR1
#define I2C_SR1_SB				0
#define I2C_SR1_ADDR			1
#define I2C_SR1_BTF				2
#define I2C_SR1_ADD10			3
#define I2C_SR1_STOPF			4
#define I2C_SR1_RXNE			6
#define I2C_SR1_TXE				7
#define I2C_SR1_BERR			8
#define I2C_SR1_ARLO			9
#define I2C_SR1_AF				10
#define I2C_SR1_OVR				11
#define I2C_SR1_TIMEOUT			14
//Bit position in I2C_SR2
#define I2C_SR2_MSL				0
#define I2C_SR2_BUSY			1
#define I2C_SR2_TRA				2
#define I2C_SR2_GENCALL			4
#define I2C_SR2_DUALF			7
//Bit position in I2C_CCR
#define I2C_CCR_CCR				0
#define I2C_CCR_DUTY			14
#define I2C_CCR_FS				15

////////////////////////////////////////////////////////////
//Bit position definitions of USART peripheral////////////////
////////////////////////////////////////////////////////////
//Bit positions in USART_CR1
#define USART_CR1_SBK			0
#define USART_CR1_RWU			1
#define USART_CR1_RE			2
#define USART_CR1_TE			3
#define USART_CR1_IDLEIE		4
#define USART_CR1_RXNEIE		5
#define USART_CR1_TCIE			6
#define USART_CR1_TXEIE			7
#define USART_CR1_PEIE			8
#define USART_CR1_PS			9
#define USART_CR1_PCE			10
#define USART_CR1_WAKE			11
#define USART_CR1_M				12
#define USART_CR1_UE			13
#define USART_CR1_OVER8			15
//Bit positions in USART_CR2
#define USART_CR2_ADD			0
#define USART_CR2_LBDL			5
#define USART_CR2_LBDIE			6
#define USART_CR2_LBCL			8
#define USART_CR2_CPHA			9
#define USART_CR2_CPOL			10
#define USART_CR2_CLKEN			11
#define USART_CR2_STOP			12
#define USART_CR2_LINEN			14
//Bit positions in USART_CR3
#define USART_CR3_EIE			0
#define USART_CR3_IREN			1
#define USART_CR3_IRLP			2
#define USART_CR3_HDSEL			3
#define USART_CR3_NACK			4
#define USART_CR3_SCEN			5
#define USART_CR3_DMAR			6
#define USART_CR3_DMAT			7
#define USART_CR3_RTSE			8
#define USART_CR3_CTSE			9
#define USART_CR3_CTSIE			10
#define USART_CR3_ONEBIT		11
//Bit positions in USART_GTPR
#define USART_GTPR_PSC			0
#define USART_GTPR_GT			8
//Bit positions in USART_SR
#define USART_SR_PE				0
#define USART_SR_FE				1
#define USART_SR_NE				2
#define USART_SR_ORE			3
#define USART_SR_IDLE			4
#define USART_SR_RXNE			5
#define USART_SR_TC				6
#define USART_SR_TXE			7
#define USART_SR_LBD			8
#define USART_SR_CTS			9


////////////////////////////////////////////////////////////
//Bit position definitions of CAN peripheral////////////////
////////////////////////////////////////////////////////////

//Bit positions in CAN_MCR
#define CAN_MCR_INRQ		0		//Initialization request
#define CAN_MCR_SLEEP		1		//Sleep mode request
#define CAN_MCR_TXFP		2		//Transmit FIFO priority
#define CAN_MCR_RFLM		3		//Receive FIFO locked mode
#define CAN_MCR_NART		4		//No automatic retransmission
#define CAN_MCR_AWUM		5		//Automatic wakeup mode
#define CAN_MCR_ABOM		6		//Automatic bus-off management
#define CAN_MCR_TTCM		7		//Time triggered communication mode
#define CAN_MCR_RESET		15		//CAN software master reset
#define CAN_MCR_DBF			16		//Debug freeze

//Bit positions in CAN_MSR
#define CAN_MSR_INAK		0		//Initialization acknowledge
#define CAN_MSR_SLAK		1		//Sleep acknowledge
#define CAN_MSR_ERRI		2		//Error interrupt
#define CAN_MSR_WKUI		3		//Wakeup interrupt
#define CAN_MSR_SLAKI		4		//Sleep acknowledge interrupt
#define CAN_MSR_TXM			8		//Transmit mode
#define CAN_MSR_RXM			9		//Receive mode
#define CAN_MSR_SAMP		10		//Last sample point
#define CAN_MSR_RX			11		//CAN RX signal

//Bit positions in CAN_TSR
#define CAN_TSR_RQCP0		0		//Request completed for mailbox 0
#define CAN_TSR_TXOK0		1		//Transmission OK for mailbox 0
#define CAN_TSR_ALST0		2		//Arbitration lost for mailbox 0
#define CAN_TSR_TERR0		3		//Transmission error for mailbox 0
#define CAN_TSR_ABRQ0		7		//Abort request for mailbox 0
#define CAN_TSR_RQCP1		8		//Request completed for mailbox 1
#define CAN_TSR_TXOK1		9		//Transmission OK for mailbox 1
#define CAN_TSR_ALST1		10		//Arbitration lost for mailbox 1
#define CAN_TSR_TERR1		11		//Transmission error for mailbox 1
#define CAN_TSR_ABRQ1		15		//Abort request for mailbox 1
#define CAN_TSR_RQCP2		16		//Request completed for mailbox 2
#define CAN_TSR_TXOK2		17		//Transmission OK for mailbox 2
#define CAN_TSR_ALST2		18		//Arbitration lost for mailbox 2
#define CAN_TSR_TERR2		19		//Transmission error for mailbox 2
#define CAN_TSR_ABRQ2		23		//Abort request for mailbox 2
#define CAN_TSR_TME0		26		//Transmit mailbox 0 empty
#define CAN_TSR_TME1		27		//Transmit mailbox 1 empty
#define CAN_TSR_TME2		28		//Transmit mailbox 2 empty
#define CAN_TSR_LOW0		29		//Lowest priority flag for mailbox 0
#define CAN_TSR_LOW1		30		//Lowest priority flag for mailbox 1
#define CAN_TSR_LOW2		31		//Lowest priority flag for mailbox 2

//Bit positions in CAN_RF0R
#define CAN_RF0R_FMP0		0		//FIFO 0 message pending count [1:0]
#define CAN_RF0R_FULL0		3		//FIFO 0 full
#define CAN_RF0R_FOVR0		4		//FIFO 0 overrun
#define CAN_RF0R_RFOM0		5		//Release FIFO 0 output mailbox

//Bit positions in CAN_RF1R
#define CAN_RF1R_FMP1		0		//FIFO 1 message pending count [1:0]
#define CAN_RF1R_FULL1		3		//FIFO 1 full
#define CAN_RF1R_FOVR1		4		//FIFO 1 overrun
#define CAN_RF1R_RFOM1		5		//Release FIFO 1 output mailbox

//Bit positions in CAN_IER
#define CAN_IER_TMEIE		0		//Transmit mailbox empty interrupt enable
#define CAN_IER_FMPIE0		1		//FIFO 0 message pending interrupt enable
#define CAN_IER_FFIE0		2		//FIFO 0 full interrupt enable
#define CAN_IER_FOVIE0		3		//FIFO 0 overrun interrupt enable
#define CAN_IER_FMPIE1		4		//FIFO 1 message pending interrupt enable
#define CAN_IER_FFIE1		5		//FIFO 1 full interrupt enable
#define CAN_IER_FOVIE1		6		//FIFO 1 overrun interrupt enable
#define CAN_IER_EWGIE		8		//Error warning interrupt enable
#define CAN_IER_EPVIE		9		//Error passive interrupt enable
#define CAN_IER_BOFIE		10		//Bus-off interrupt enable
#define CAN_IER_LECIE		11		//Last error code interrupt enable
#define CAN_IER_ERRIE		15		//Error interrupt enable
#define CAN_IER_WKUIE		16		//Wakeup interrupt enable
#define CAN_IER_SLKIE		17		//Sleep interrupt enable

//Bit positions in CAN_ESR
#define CAN_ESR_EWGF		0		//Error warning flag
#define CAN_ESR_EPVF		1		//Error passive flag
#define CAN_ESR_BOFF		2		//Bus-off flag
#define CAN_ESR_LEC			4		//Last error code [6:4] (3 bits)
#define CAN_ESR_TEC			16		//Transmit error counter [23:16]
#define CAN_ESR_REC			24		//Receive error counter  [31:24]

//Bit positions in CAN_BTR
#define CAN_BTR_BRP			0		//Baud rate prescaler [9:0]  (actual = BRP+1)
#define CAN_BTR_TS1			16		//Time segment 1     [19:16] (actual = TS1+1 TQ)
#define CAN_BTR_TS2			20		//Time segment 2     [22:20] (actual = TS2+1 TQ)
#define CAN_BTR_SJW			24		//Synchronization jump width [25:24] (actual = SJW+1 TQ)
#define CAN_BTR_LBKM		30		//Loopback mode
#define CAN_BTR_SILM		31		//Silent mode

//Bit positions in CAN TX mailbox identifier register (TIxR)
#define CAN_TIR_TXRQ		0		//Transmission request — set to start TX
#define CAN_TIR_RTR			1		//Remote transmission request (0=data, 1=remote)
#define CAN_TIR_IDE			2		//Identifier extension (0=standard, 1=extended)
#define CAN_TIR_EXID		3		//Extended identifier [20:3]  (18 bits of the 29-bit ID)
#define CAN_TIR_STID		21		//Standard identifier [31:21] (11-bit ID in standard frames)
									//                    [31:21] is also the upper 11 bits of extended ID

//Bit positions in CAN TX mailbox data length register (TDTxR)
#define CAN_TDTR_DLC		0		//Data length code [3:0]
#define CAN_TDTR_TGT		8		//Transmit global time
#define CAN_TDTR_TIME		16		//Message time stamp [31:16]

//Bit positions in CAN RX FIFO mailbox identifier register (RIxR)
#define CAN_RIR_RTR			1		//Remote transmission request
#define CAN_RIR_IDE			2		//Identifier extension (0=standard, 1=extended)
#define CAN_RIR_EXID		3		//Extended identifier [20:3]
#define CAN_RIR_STID		21		//Standard identifier [31:21]

//Bit positions in CAN RX FIFO mailbox data length register (RDTxR)
#define CAN_RDTR_DLC		0		//Data length code [3:0]
#define CAN_RDTR_FMI		8		//Filter match index [15:8]
#define CAN_RDTR_TIME		16		//Message time stamp [31:16]

//Bit positions in CAN_FMR
#define CAN_FMR_FINIT		0		//Filter init mode (1 = init, filters can be configured)
#define CAN_FMR_CAN2SB		8		//CAN2 start bank [13:8] — first filter bank assigned to CAN2

#include "stm32f4xx_gpio_driver.h"
#include "stm32f4xx_spi_driver.h"
#include "stm32f4xx_i2c_driver.h"
#include "stm32f4xx_usart_driver.h"
#include "stm32f4xx_rcc_driver.h"
#include "stm32f4xx_can_driver.h"


#endif /* INC_STM32F4XX_H_ */
