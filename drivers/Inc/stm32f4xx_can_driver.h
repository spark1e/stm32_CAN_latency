/*
 * stm32f4xx_can_driver.h
 *
 *  Created on: Mar 14, 2026
 *      Author: Nikita Volkov (https://github.com/spark1e)
 *
 *  CAN driver for STM32F446RE — bare-metal, no HAL.
 *  Supports CAN1 and CAN2, polling and interrupt modes.
 *
 *  Bit timing for 500 kbps @ 45 MHz APB1:
 *    Prescaler = 5, TS1 = 14 TQ, TS2 = 3 TQ  → sample point 83.3%
 *    BRP register  = 4  (CAN_PRESCALER_500KBPS - 1)
 *    TS1 register  = 13 (CAN_TS1_14TQ)
 *    TS2 register  = 2  (CAN_TS2_3TQ)
 *    SJW register  = 0  (CAN_SJW_1TQ)
 */

#ifndef INC_STM32F4XX_CAN_DRIVER_H_
#define INC_STM32F4XX_CAN_DRIVER_H_

#include "stm32f4xx.h"


/*
 * Configuration structure for CANx peripheral
 */
typedef struct{
	uint16_t CAN_Prescaler;		//@CAN_Prescaler  — Baud rate prescaler value (1-1024); actual = value
	uint8_t  CAN_Mode;			//@CAN_Mode       — Operating mode (normal, loopback, silent, silent+loopback)
	uint8_t  CAN_SyncJumpWidth;	//@CAN_SJW        — Resynchronisation jump width in TQ (1-4)
	uint8_t  CAN_TimeSeg1;		//@CAN_TS1        — Time segment 1 length in TQ (1-16)
	uint8_t  CAN_TimeSeg2;		//@CAN_TS2        — Time segment 2 length in TQ (1-8)
	uint8_t  CAN_TTCM;			// Time triggered communication mode:  ENABLE / DISABLE
	uint8_t  CAN_ABOM;			// Automatic bus-off management:        ENABLE / DISABLE
	uint8_t  CAN_AWUM;			// Automatic wakeup mode:               ENABLE / DISABLE
	uint8_t  CAN_NART;			// No automatic retransmission:         ENABLE / DISABLE
	uint8_t  CAN_RFLM;			// Receive FIFO locked mode:            ENABLE / DISABLE
	uint8_t  CAN_TXFP;			// Transmit FIFO priority:              ENABLE / DISABLE
}CAN_Config_t;

/*
 * TX message structure — fill this before calling CAN_SendMessage / CAN_SendMessageIT
 */
typedef struct{
	uint32_t StdId;		// Standard 11-bit identifier (0x000-0x7FF); used when IDE = CAN_ID_STD
	uint32_t ExtId;		// Extended 29-bit identifier; used when IDE = CAN_ID_EXT
	uint8_t  IDE;		// Identifier type: CAN_ID_STD or CAN_ID_EXT
	uint8_t  RTR;		// Frame type: CAN_RTR_DATA or CAN_RTR_REMOTE
	uint8_t  DLC;		// Data length code: number of data bytes (0-8)
	uint8_t  Data[8];	// Payload bytes (only DLC bytes are transmitted)
}CAN_TxMsg_t;

/*
 * RX message structure — filled by CAN_ReceiveMessage / ISR
 */
typedef struct{
	uint32_t StdId;		// Standard 11-bit identifier (valid when IDE = CAN_ID_STD)
	uint32_t ExtId;		// Extended 29-bit identifier  (valid when IDE = CAN_ID_EXT)
	uint8_t  IDE;		// Identifier type: CAN_ID_STD or CAN_ID_EXT
	uint8_t  RTR;		// Frame type: CAN_RTR_DATA or CAN_RTR_REMOTE
	uint8_t  DLC;		// Number of received data bytes (0-8)
	uint8_t  Data[8];	// Received payload bytes
	uint8_t  FMI;		// Filter match index — which filter accepted this message
}CAN_RxMsg_t;

/*
 * Filter configuration structure — pass to CAN_ConfigFilter()
 * All filter registers are controlled through CAN1 even when filtering for CAN2
 */
typedef struct{
	uint32_t FilterIdHigh;			// Filter ID / ID1 high  word (bits [31:16])
	uint32_t FilterIdLow;			// Filter ID / ID1 low   word (bits [15:0])
	uint32_t FilterMaskIdHigh;		// Filter mask / ID2 high word (bits [31:16])
	uint32_t FilterMaskIdLow;		// Filter mask / ID2 low  word (bits [15:0])
	uint8_t  FilterFIFOAssignment;	// Which FIFO receives matching messages: CAN_FILTER_FIFO0 or CAN_FILTER_FIFO1
	uint8_t  FilterBank;			// Filter bank index: 0-27 for CAN1, 0-13 for CAN2 (default split)
	uint8_t  FilterMode;			// CAN_FILTERMODE_IDMASK or CAN_FILTERMODE_IDLIST
	uint8_t  FilterScale;			// CAN_FILTERSCALE_16BIT or CAN_FILTERSCALE_32BIT
	uint8_t  FilterActivation;		// ENABLE to activate this filter bank, DISABLE to deactivate
}CAN_FilterConfig_t;

/*
 * Handle structure for CANx peripheral
 */
typedef struct{
	CAN_RegDef_t  *pCANx;		// Base address of CAN1 or CAN2 peripheral
	CAN_Config_t   CAN_Config;	// User-supplied configuration
	uint8_t        TxState;		// Current TX driver state (CAN_READY or CAN_BUSY_IN_TX)
	uint8_t        RxState;		// Current RX driver state (CAN_READY, CAN_BUSY_IN_RX0/1)
	CAN_RxMsg_t   *pRxMsg;		// Pointer to RX buffer supplied by CAN_ReceiveMessageIT()
	uint8_t        RxFIFO;		// Which FIFO is being listened on in IT mode
}CAN_Handle_t;


//@CAN_Mode — operating mode (written to BTR[LBKM] and BTR[SILM])
#define CAN_MODE_NORMAL				0	// Normal operation; TX drives bus, RX samples bus
#define CAN_MODE_LOOPBACK			1	// Loopback: TX is looped back to RX internally; no bus traffic
#define CAN_MODE_SILENT				2	// Silent: monitors bus only; does not drive TX line
#define CAN_MODE_SILENT_LOOPBACK	3	// Combined loopback + silent: self-test without any bus activity

//@CAN_SJW — synchronisation jump width (BTR[SJW] register value; actual TQ = value + 1)
#define CAN_SJW_1TQ		0
#define CAN_SJW_2TQ		1
#define CAN_SJW_3TQ		2
#define CAN_SJW_4TQ		3

//@CAN_TS1 — time segment 1 (BTR[TS1] register value; actual TQ = value + 1)
#define CAN_TS1_1TQ		0
#define CAN_TS1_2TQ		1
#define CAN_TS1_3TQ		2
#define CAN_TS1_4TQ		3
#define CAN_TS1_5TQ		4
#define CAN_TS1_6TQ		5
#define CAN_TS1_7TQ		6
#define CAN_TS1_8TQ		7
#define CAN_TS1_9TQ		8
#define CAN_TS1_10TQ	9
#define CAN_TS1_11TQ	10
#define CAN_TS1_12TQ	11
#define CAN_TS1_13TQ	12
#define CAN_TS1_14TQ	13
#define CAN_TS1_15TQ	14
#define CAN_TS1_16TQ	15

//@CAN_TS2 — time segment 2 (BTR[TS2] register value; actual TQ = value + 1)
#define CAN_TS2_1TQ		0
#define CAN_TS2_2TQ		1
#define CAN_TS2_3TQ		2
#define CAN_TS2_4TQ		3
#define CAN_TS2_5TQ		4
#define CAN_TS2_6TQ		5
#define CAN_TS2_7TQ		6
#define CAN_TS2_8TQ		7

//@CAN_Prescaler — pre-calculated prescaler for common baud rates
// Baudrate = PCLK1 / (Prescaler * (1 + TS1_actual + TS2_actual))
//
// At PCLK1 = 16 MHz (HSI default — no PLL configured):
//   500 kbps: 16 MHz / (2 * 16) = 500 kbps — use TS1=CAN_TS1_13TQ, TS2=CAN_TS2_2TQ (87.5% sample)
//   250 kbps: 16 MHz / (4 * 16) = 250 kbps — same TS1/TS2
//
// At PCLK1 = 45 MHz (HCLK = 90 MHz, APB1 prescaler /2):
//   500 kbps: 45 MHz / (5 * 18) = 500 kbps — use TS1=CAN_TS1_14TQ, TS2=CAN_TS2_3TQ (83.3% sample)
//   250 kbps: 45 MHz / (9 * 20) = 250 kbps — use TS1=CAN_TS1_15TQ, TS2=CAN_TS2_4TQ
#define CAN_PRESCALER_500KBPS_16MHZ		2
#define CAN_PRESCALER_500KBPS_45MHZ		5
#define CAN_PRESCALER_250KBPS_16MHZ		4
#define CAN_PRESCALER_250KBPS_45MHZ		9

//Identifier type (IDE bit in TIxR / RIxR)
#define CAN_ID_STD		0	// Standard 11-bit identifier
#define CAN_ID_EXT		1	// Extended 29-bit identifier

//Frame type (RTR bit)
#define CAN_RTR_DATA	0	// Data frame
#define CAN_RTR_REMOTE	1	// Remote frame

//FIFO numbers
#define CAN_FIFO0		0	// Receive FIFO 0
#define CAN_FIFO1		1	// Receive FIFO 1

//Filter mode (FM1R)
#define CAN_FILTERMODE_IDMASK	0	// Identifier mask mode (ID + mask pair)
#define CAN_FILTERMODE_IDLIST	1	// Identifier list mode  (two exact IDs per bank)

//Filter scale (FS1R)
#define CAN_FILTERSCALE_16BIT	0	// Two 16-bit filters per bank
#define CAN_FILTERSCALE_32BIT	1	// One 32-bit filter  per bank

//TX mailbox numbers (returned by CAN_SendMessage / CAN_SendMessageIT)
#define CAN_TX_MAILBOX0		0U
#define CAN_TX_MAILBOX1		1U
#define CAN_TX_MAILBOX2		2U
#define CAN_TX_MAILBOX_NONE	4U	// Returned when all three mailboxes are occupied

//CAN driver application states
#define CAN_READY			0
#define CAN_BUSY_IN_TX		1
#define CAN_BUSY_IN_RX0		2	// Waiting for FIFO 0 message (IT mode)
#define CAN_BUSY_IN_RX1		3	// Waiting for FIFO 1 message (IT mode)

//CAN application event codes — passed to CAN_ApplicationEventCallback()
#define CAN_EVENT_TX_MAILBOX_EMPTY		1	// TX complete: all occupied mailboxes are now empty
#define CAN_EVENT_RX_FIFO0_MSG			2	// A message was received in FIFO 0
#define CAN_EVENT_RX_FIFO1_MSG			3	// A message was received in FIFO 1
#define CAN_EVENT_RX_FIFO0_FULL			4	// FIFO 0 is full (3 messages pending)
#define CAN_EVENT_RX_FIFO1_FULL			5	// FIFO 1 is full
#define CAN_EVENT_RX_FIFO0_OVERRUN		6	// FIFO 0 overrun — a message was lost
#define CAN_EVENT_RX_FIFO1_OVERRUN		7	// FIFO 1 overrun
#define CAN_EVENT_ERROR					8	// Bus error detected (check ESR for details)


//////////////// API SUPPORTED BY THIS DRIVER ////////////////////

//Peripheral clock setup
void CAN_PeriClockControl(CAN_RegDef_t *pCANx, uint8_t ENorDI);

//Init and De-init
void CAN_Init(CAN_Handle_t *pCANHandle);
void CAN_DeInit(CAN_RegDef_t *pCANx);

//Filter configuration (must be called after CAN_Init)
void CAN_ConfigFilter(CAN_Handle_t *pCANHandle, CAN_FilterConfig_t *pFilterConfig);

//Data send and receive — polling (blocking)
uint8_t CAN_SendMessage(CAN_Handle_t *pCANHandle, CAN_TxMsg_t *pTxMsg);
void    CAN_ReceiveMessage(CAN_Handle_t *pCANHandle, uint8_t FIFONumber, CAN_RxMsg_t *pRxMsg);

//Data send and receive — interrupt mode (non-blocking)
uint8_t CAN_SendMessageIT(CAN_Handle_t *pCANHandle, CAN_TxMsg_t *pTxMsg);
uint8_t CAN_ReceiveMessageIT(CAN_Handle_t *pCANHandle, uint8_t FIFONumber, CAN_RxMsg_t *pRxMsg);

//IRQ configuration and ISR handling
void CAN_IRQITConfig(uint8_t IRQNumber, uint8_t ENorDI);
void CAN_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void CAN_TX_IRQHandling(CAN_Handle_t *pCANHandle);
void CAN_RX0_IRQHandling(CAN_Handle_t *pCANHandle);
void CAN_RX1_IRQHandling(CAN_Handle_t *pCANHandle);
void CAN_SCE_IRQHandling(CAN_Handle_t *pCANHandle);

//Application callback — weak, override in application code
void CAN_ApplicationEventCallback(CAN_Handle_t *pCANHandle, uint8_t AppEvent);


#endif /* INC_STM32F4XX_CAN_DRIVER_H_ */
