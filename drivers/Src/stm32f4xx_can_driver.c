/*
 * stm32f4xx_can_driver.c
 *
 *  Created on: Mar 14, 2026
 *      Author: Nikita Volkov (https://github.com/spark1e)
 *
 *  Bare-metal CAN driver for STM32F446RE.
 *  No HAL — direct register access only.
 *
 *  Supports:
 *    - CAN1 and CAN2
 *    - Polling (blocking) TX and RX
 *    - Interrupt-driven (non-blocking) TX and RX
 *    - Hardware filter bank configuration
 *    - Error / SCE interrupt handling
 */

#include "stm32f4xx_can_driver.h"

/*
 * Private helper function prototypes
 * These are not exposed in the header — they are internal to this driver only.
 */
static uint8_t can_get_free_tx_mailbox(CAN_RegDef_t *pCANx);
static void    can_tx_mailbox_IThandle(CAN_Handle_t *pCANHandle);
static void    can_rx_fifo_IThandle(CAN_Handle_t *pCANHandle, uint8_t FIFONumber);


/*
 * ============================================================
 * PERIPHERAL CLOCK CONTROL
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_PeriClockControl
 *
 * @brief		- Enables or disables the peripheral clock for the given CAN port
 *
 * @param[in]	- pCANx: base address of the CAN peripheral (CAN1 or CAN2)
 * @param[in]	- ENorDI: ENABLE or DISABLE macro
 *
 * @return		- none
 *
 * @Note		- CAN2 requires the CAN1 clock to be running because all 28 filter
 *                banks are owned by CAN1. Enabling CAN2 here automatically enables
 *                CAN1 as well. Do NOT disable CAN1 while CAN2 is still active.
 *
 * */
void CAN_PeriClockControl(CAN_RegDef_t *pCANx, uint8_t ENorDI){
	if(ENorDI == ENABLE){
		if(pCANx == CAN1){
			CAN1_PCLK_EN();
		}
		else if(pCANx == CAN2){
			CAN1_PCLK_EN();	// CAN1 clock needed for shared filter bank access
			CAN2_PCLK_EN();
		}
	}
	else{
		if(pCANx == CAN1)		CAN1_PCLK_DI();
		else if(pCANx == CAN2)	CAN2_PCLK_DI();
	}
}


/*
 * ============================================================
 * INIT / DE-INIT
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_Init
 *
 * @brief		- Initialises the CAN peripheral with the settings stored in the handle
 *
 * @param[in]	- pCANHandle: pointer to a fully populated CAN_Handle_t
 *
 * @return		- none
 *
 * @Note		- The function drives the peripheral through:
 *                  1. Enable peripheral clock
 *                  2. Request Initialization mode (INRQ) — MCR/MSR handshake
 *                  3. Configure MCR control bits (TTCM, ABOM, AWUM, NART, RFLM, TXFP)
 *                  4. Configure BTR (prescaler, TS1, TS2, SJW, operating mode)
 *                  5. Leave Initialization mode — peripheral enters Normal/Test mode
 *
 *              Call CAN_ConfigFilter() after CAN_Init() to accept specific message IDs.
 *              Until at least one filter is active the RX FIFOs will not receive frames.
 *
 * */
void CAN_Init(CAN_Handle_t *pCANHandle){
	uint32_t tempreg = 0;

	//1. Enable peripheral clock
	CAN_PeriClockControl(pCANHandle->pCANx, ENABLE);

	//2. Exit Sleep mode (SLEEP bit) and request Initialization mode (INRQ bit)
	pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_SLEEP);	// Exit sleep
	pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_INRQ);	// Request init mode

	//   Wait for hardware to acknowledge Initialization mode via INAK bit in MSR
	while(!(pCANHandle->pCANx->MSR & (1 << CAN_MSR_INAK)));

	//3. Configure MCR feature bits (must be done while in Initialization mode)

	//   TTCM — Time Triggered Communication Mode
	if(pCANHandle->CAN_Config.CAN_TTCM == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_TTCM);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_TTCM);

	//   ABOM — Automatic Bus-Off Management
	//   When enabled the peripheral recovers from bus-off automatically after 128 * 11 recessive bits
	if(pCANHandle->CAN_Config.CAN_ABOM == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_ABOM);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_ABOM);

	//   AWUM — Automatic Wakeup Mode
	if(pCANHandle->CAN_Config.CAN_AWUM == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_AWUM);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_AWUM);

	//   NART — No Automatic Retransmission
	//   Set ENABLE to transmit each frame only once regardless of errors (useful for real-time systems)
	if(pCANHandle->CAN_Config.CAN_NART == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_NART);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_NART);

	//   RFLM — Receive FIFO Locked Mode
	//   ENABLE: once full the FIFO is locked and new messages are discarded
	//   DISABLE: oldest message is overwritten by the newest (default behaviour)
	if(pCANHandle->CAN_Config.CAN_RFLM == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_RFLM);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_RFLM);

	//   TXFP — Transmit FIFO Priority
	//   ENABLE: transmission priority determined by chronological order of requests
	//   DISABLE: priority determined by message identifier (lower ID = higher priority)
	if(pCANHandle->CAN_Config.CAN_TXFP == ENABLE)
		pCANHandle->pCANx->MCR |=  (1 << CAN_MCR_TXFP);
	else
		pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_TXFP);

	//4. Configure BTR — Bit Timing Register
	//   This register can only be written while the peripheral is in Initialization mode
	tempreg = 0;

	//   BRP[9:0]: Baud Rate Prescaler. Actual clock divisor = BRP + 1.
	//   Store (CAN_Prescaler - 1) in the register field.
	tempreg |= (uint32_t)(pCANHandle->CAN_Config.CAN_Prescaler - 1) << CAN_BTR_BRP;

	//   TS1[19:16]: Time Segment 1. The macro values already match register encoding (actual = value + 1).
	tempreg |= (uint32_t)pCANHandle->CAN_Config.CAN_TimeSeg1 << CAN_BTR_TS1;

	//   TS2[22:20]: Time Segment 2.
	tempreg |= (uint32_t)pCANHandle->CAN_Config.CAN_TimeSeg2 << CAN_BTR_TS2;

	//   SJW[25:24]: Synchronisation Jump Width.
	tempreg |= (uint32_t)pCANHandle->CAN_Config.CAN_SyncJumpWidth << CAN_BTR_SJW;

	//   LBKM / SILM: test-mode bits (set based on CAN_Mode selection)
	if(pCANHandle->CAN_Config.CAN_Mode == CAN_MODE_LOOPBACK){
		tempreg |= (1 << CAN_BTR_LBKM);					// Enable internal loopback
	}
	else if(pCANHandle->CAN_Config.CAN_Mode == CAN_MODE_SILENT){
		tempreg |= (1 << CAN_BTR_SILM);					// Enable silent monitoring
	}
	else if(pCANHandle->CAN_Config.CAN_Mode == CAN_MODE_SILENT_LOOPBACK){
		tempreg |= (1 << CAN_BTR_LBKM) | (1 << CAN_BTR_SILM);	// Both bits set
	}
	// CAN_MODE_NORMAL: neither bit set — already zero in tempreg

	pCANHandle->pCANx->BTR = tempreg;

	//5. Leave Initialization mode — peripheral transitions to Normal (or test) mode
	pCANHandle->pCANx->MCR &= ~(1 << CAN_MCR_INRQ);

	//   Wait for hardware to confirm it has left Initialization mode (INAK cleared)
	while(pCANHandle->pCANx->MSR & (1 << CAN_MSR_INAK));

	//6. Initialise driver state variables
	pCANHandle->TxState = CAN_READY;
	pCANHandle->RxState = CAN_READY;
	pCANHandle->pRxMsg  = NULL;
}


/***********************************************************************************
 * @fn			- CAN_DeInit
 *
 * @brief		- Resets the CAN peripheral registers to their power-on defaults
 *
 * @param[in]	- pCANx: base address of the CAN peripheral (CAN1 or CAN2)
 *
 * @return		- none
 *
 * @Note		- none
 *
 * */
void CAN_DeInit(CAN_RegDef_t *pCANx){
	if(pCANx == CAN1)		CAN1_REG_RESET();
	else if(pCANx == CAN2)	CAN2_REG_RESET();
}


/*
 * ============================================================
 * FILTER CONFIGURATION
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_ConfigFilter
 *
 * @brief		- Configures one filter bank to accept (or reject) specific CAN IDs
 *
 * @param[in]	- pCANHandle: pointer to CAN handle (used only for reference; all
 *                filter registers are always accessed through CAN1)
 * @param[in]	- pFilterConfig: pointer to the populated filter configuration struct
 *
 * @return		- none
 *
 * @Note		- All 28 filter banks are owned by CAN1 even when filtering for CAN2.
 *                The number of banks assigned to CAN1 vs CAN2 is controlled by
 *                CAN_FMR[CAN2SB]. Default split: banks 0-13 → CAN1, 14-27 → CAN2.
 *
 *              Mask mode (CAN_FILTERMODE_IDMASK):
 *                FR1 = ID bits to match, FR2 = care mask (1=must match, 0=don't care)
 *
 *              List mode (CAN_FILTERMODE_IDLIST):
 *                FR1 = first accepted ID, FR2 = second accepted ID
 *
 *              For 32-bit scale: IDs must be shifted left by 3 (STID at [31:21] or
 *              EXID at [31:3]) with IDE and RTR bits included if needed.
 *
 * */
void CAN_ConfigFilter(CAN_Handle_t *pCANHandle, CAN_FilterConfig_t *pFilterConfig){
	(void)pCANHandle;	// Filter banks always accessed through CAN1 regardless of handle

	uint32_t filternbrbitpos = (1U << pFilterConfig->FilterBank);

	//1. Enter filter Initialisation mode — FINIT bit in FMR
	//   While FINIT = 1 the active filters are frozen; they can be modified safely
	CAN1->FMR |= (1 << CAN_FMR_FINIT);

	//2. Deactivate the target filter bank before modifying its registers
	CAN1->FA1R &= ~filternbrbitpos;

	//3. Set filter scale: 16-bit pair or single 32-bit
	if(pFilterConfig->FilterScale == CAN_FILTERSCALE_16BIT){
		CAN1->FS1R &= ~filternbrbitpos;	// 0 = 16-bit scale

		//   Two 16-bit filters are packed into the two 32-bit filter registers:
		//     FR1[15:0]  = FilterIdLow      FR1[31:16] = FilterMaskIdLow
		//     FR2[15:0]  = FilterIdHigh     FR2[31:16] = FilterMaskIdHigh
		CAN1->sFilterRegister[pFilterConfig->FilterBank].FR1 =
			((pFilterConfig->FilterMaskIdLow  & 0xFFFFU) << 16) |
			 (pFilterConfig->FilterIdLow      & 0xFFFFU);

		CAN1->sFilterRegister[pFilterConfig->FilterBank].FR2 =
			((pFilterConfig->FilterMaskIdHigh & 0xFFFFU) << 16) |
			 (pFilterConfig->FilterIdHigh     & 0xFFFFU);
	}
	else{
		CAN1->FS1R |= filternbrbitpos;	// 1 = 32-bit scale

		//   One 32-bit filter:
		//     FR1 = full 32-bit ID   (mask mode: ID to match;   list mode: first accepted ID)
		//     FR2 = full 32-bit mask (mask mode: care mask;     list mode: second accepted ID)
		CAN1->sFilterRegister[pFilterConfig->FilterBank].FR1 =
			((pFilterConfig->FilterIdHigh & 0xFFFFU) << 16) |
			 (pFilterConfig->FilterIdLow  & 0xFFFFU);

		CAN1->sFilterRegister[pFilterConfig->FilterBank].FR2 =
			((pFilterConfig->FilterMaskIdHigh & 0xFFFFU) << 16) |
			 (pFilterConfig->FilterMaskIdLow  & 0xFFFFU);
	}

	//4. Set filter mode: identifier mask or identifier list
	if(pFilterConfig->FilterMode == CAN_FILTERMODE_IDMASK)
		CAN1->FM1R &= ~filternbrbitpos;	// 0 = mask mode
	else
		CAN1->FM1R |=  filternbrbitpos;	// 1 = list mode

	//5. Assign filter bank output to FIFO 0 or FIFO 1
	if(pFilterConfig->FilterFIFOAssignment == CAN_FIFO0)
		CAN1->FFA1R &= ~filternbrbitpos;	// 0 = FIFO 0
	else
		CAN1->FFA1R |=  filternbrbitpos;	// 1 = FIFO 1

	//6. Activate or deactivate this filter bank
	if(pFilterConfig->FilterActivation == ENABLE)
		CAN1->FA1R |=  filternbrbitpos;
	else
		CAN1->FA1R &= ~filternbrbitpos;

	//7. Leave filter Initialisation mode — configured filters go active immediately
	CAN1->FMR &= ~(1 << CAN_FMR_FINIT);
}


/*
 * ============================================================
 * POLLING (BLOCKING) SEND AND RECEIVE
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_SendMessage (blocking)
 *
 * @brief		- Loads a message into a free TX mailbox and blocks until transmission
 *                is confirmed by the CAN hardware (TXOK flag in TSR)
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 * @param[in]	- pTxMsg: pointer to the message to transmit
 *
 * @return		- CAN_TX_MAILBOX0/1/2 — the mailbox that was used
 *                CAN_TX_MAILBOX_NONE  — returned immediately if all mailboxes are busy
 *
 * @Note		- For Phase 2 latency measurement:
 *                  Record T0 (TIM2 count) immediately before calling this function.
 *                  The CAN frame hits the bus within ~2 µs of TXRQ being set.
 *
 * */
uint8_t CAN_SendMessage(CAN_Handle_t *pCANHandle, CAN_TxMsg_t *pTxMsg){
	uint32_t tempreg = 0;
	uint8_t  mailbox = can_get_free_tx_mailbox(pCANHandle->pCANx);

	if(mailbox == CAN_TX_MAILBOX_NONE){
		return CAN_TX_MAILBOX_NONE;	// All three mailboxes are occupied
	}

	//1. Build the TX identifier register value (do NOT set TXRQ yet)
	tempreg = 0;
	if(pTxMsg->IDE == CAN_ID_STD){
		//   Standard 11-bit ID: STID[10:0] sits at bits [31:21] of TIR
		tempreg |= (pTxMsg->StdId << CAN_TIR_STID);
	}
	else{
		//   Extended 29-bit ID: full ID spans STID[31:21] + EXID[20:3]
		tempreg |= (pTxMsg->ExtId << CAN_TIR_EXID);
		tempreg |= (1 << CAN_TIR_IDE);	// Set IDE bit to mark as extended frame
	}
	if(pTxMsg->RTR == CAN_RTR_REMOTE)
		tempreg |= (1 << CAN_TIR_RTR);	// Set RTR bit for remote frame

	pCANHandle->pCANx->sTxMailBox[mailbox].TIR = tempreg;

	//2. Write DLC (data length code) into TDTR[3:0]
	pCANHandle->pCANx->sTxMailBox[mailbox].TDTR = (pTxMsg->DLC & 0xFU);

	//3. Load data bytes into TDLR (bytes 0-3) and TDHR (bytes 4-7)
	pCANHandle->pCANx->sTxMailBox[mailbox].TDLR =
		((uint32_t)pTxMsg->Data[3] << 24) |
		((uint32_t)pTxMsg->Data[2] << 16) |
		((uint32_t)pTxMsg->Data[1] <<  8) |
		((uint32_t)pTxMsg->Data[0]);

	pCANHandle->pCANx->sTxMailBox[mailbox].TDHR =
		((uint32_t)pTxMsg->Data[7] << 24) |
		((uint32_t)pTxMsg->Data[6] << 16) |
		((uint32_t)pTxMsg->Data[5] <<  8) |
		((uint32_t)pTxMsg->Data[4]);

	//4. Set TXRQ bit in TIR to request transmission — frame enters the CAN bus scheduler
	pCANHandle->pCANx->sTxMailBox[mailbox].TIR |= (1 << CAN_TIR_TXRQ);

	//5. Wait (block) until the mailbox transaction completes — successful or not.
	//   RQCP (Request Completed) is set when the mailbox is done, regardless of outcome.
	//   After RQCP, check TXOK to know whether the frame was actually ACKed by another node.
	//   Without this, a missing ACK in NART=DISABLE mode would hang forever (TXOK never sets
	//   while the controller keeps retransmitting); in NART=ENABLE one-shot mode TXOK also
	//   never sets when the frame fails, so polling just on TXOK is unsafe in either mode.
	uint32_t rqcp_bit = (mailbox == 0) ? CAN_TSR_RQCP0 :
	                    (mailbox == 1) ? CAN_TSR_RQCP1 : CAN_TSR_RQCP2;
	uint32_t txok_bit = (mailbox == 0) ? CAN_TSR_TXOK0 :
	                    (mailbox == 1) ? CAN_TSR_TXOK1 : CAN_TSR_TXOK2;

	//   Coarse timeout — large enough that a successful frame at any reasonable bitrate
	//   will complete well before expiry, but small enough that the caller is not stuck
	//   forever if the bus is silent or misconfigured.
	uint32_t timeout = 0x00FFFFFFU;
	while(!(pCANHandle->pCANx->TSR & (1U << rqcp_bit))){
		if(--timeout == 0){
			return CAN_TX_MAILBOX_NONE;	// Bus stuck — caller decides what to do
		}
	}

	//   Clear RQCP by writing 1 (it is the only way to clear this flag)
	pCANHandle->pCANx->TSR |= (1U << rqcp_bit);

	//   Return the mailbox number on success; CAN_TX_MAILBOX_NONE on failure (no ACK / error)
	if(pCANHandle->pCANx->TSR & (1U << txok_bit)){
		return mailbox;
	}
	return CAN_TX_MAILBOX_NONE;
}


/***********************************************************************************
 * @fn			- CAN_ReceiveMessage (blocking)
 *
 * @brief		- Blocks until a message arrives in the specified FIFO, then reads
 *                it into pRxMsg and releases the FIFO slot back to hardware
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 * @param[in]	- FIFONumber: CAN_FIFO0 or CAN_FIFO1
 * @param[out]	- pRxMsg: pointer to the structure that will be filled with the
 *                received message
 *
 * @return		- none
 *
 * @Note		- For Phase 2 latency measurement:
 *                  Record T1 (TIM2 count) immediately after this function returns.
 *                  Latency = T1 - T0 (microseconds, given TIM2 runs at 1 MHz).
 *
 * */
void CAN_ReceiveMessage(CAN_Handle_t *pCANHandle, uint8_t FIFONumber, CAN_RxMsg_t *pRxMsg){
	//1. Wait until at least one message is pending in the selected FIFO
	//   FMP[1:0] in RF0R / RF1R = number of messages waiting (0-3)
	if(FIFONumber == CAN_FIFO0){
		while(!(pCANHandle->pCANx->RF0R & (0x3U << CAN_RF0R_FMP0)));
	}
	else{
		while(!(pCANHandle->pCANx->RF1R & (0x3U << CAN_RF1R_FMP1)));
	}

	//2. Read the identifier from RIR
	if(!(pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR & (1 << CAN_RIR_IDE))){
		//   Standard frame (IDE = 0): read 11-bit STID from bits [31:21]
		pRxMsg->IDE   = CAN_ID_STD;
		pRxMsg->StdId = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR >> CAN_RIR_STID) & 0x7FFU;
	}
	else{
		//   Extended frame (IDE = 1): read 29-bit ID from STID[31:21] + EXID[20:3]
		pRxMsg->IDE   = CAN_ID_EXT;
		pRxMsg->ExtId = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR >> CAN_RIR_EXID) & 0x1FFFFFFFU;
	}

	//   Read RTR bit
	pRxMsg->RTR = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR >> CAN_RIR_RTR) & 0x1U;

	//3. Read DLC and Filter Match Index from RDTR
	pRxMsg->DLC = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDTR)                  & 0xFU;
	pRxMsg->FMI = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDTR >> CAN_RDTR_FMI) & 0xFFU;

	//4. Read data bytes from RDLR (bytes 0-3) and RDHR (bytes 4-7)
	pRxMsg->Data[0] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR)        & 0xFFU;
	pRxMsg->Data[1] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >>  8)  & 0xFFU;
	pRxMsg->Data[2] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >> 16)  & 0xFFU;
	pRxMsg->Data[3] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >> 24)  & 0xFFU;
	pRxMsg->Data[4] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR)        & 0xFFU;
	pRxMsg->Data[5] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >>  8)  & 0xFFU;
	pRxMsg->Data[6] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >> 16)  & 0xFFU;
	pRxMsg->Data[7] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >> 24)  & 0xFFU;

	//5. Release the FIFO output mailbox so hardware can write the next received frame
	//   Setting RFOM (Release FIFO Output Mailbox) causes FMP to decrement by 1
	if(FIFONumber == CAN_FIFO0)
		pCANHandle->pCANx->RF0R |= (1 << CAN_RF0R_RFOM0);
	else
		pCANHandle->pCANx->RF1R |= (1 << CAN_RF1R_RFOM1);
}


/*
 * ============================================================
 * INTERRUPT MODE SEND AND RECEIVE
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_SendMessageIT (non-blocking)
 *
 * @brief		- Loads a message into a free TX mailbox, enables the TX mailbox
 *                empty interrupt (TMEIE), and returns immediately
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 * @param[in]	- pTxMsg: pointer to message to transmit
 *
 * @return		- Mailbox number used (CAN_TX_MAILBOX0/1/2)
 *                CAN_TX_MAILBOX_NONE if all mailboxes are occupied
 *
 * @Note		- CAN_ApplicationEventCallback() is called with event code
 *                CAN_EVENT_TX_MAILBOX_EMPTY when transmission is complete.
 *                For latency measurement record T0 before this call and T1
 *                at the start of the callback.
 *
 * */
uint8_t CAN_SendMessageIT(CAN_Handle_t *pCANHandle, CAN_TxMsg_t *pTxMsg){
	uint32_t tempreg = 0;
	uint8_t  mailbox = can_get_free_tx_mailbox(pCANHandle->pCANx);

	if(mailbox == CAN_TX_MAILBOX_NONE){
		return CAN_TX_MAILBOX_NONE;
	}

	//1. Build TIR — identical to the polling version (see CAN_SendMessage)
	tempreg = 0;
	if(pTxMsg->IDE == CAN_ID_STD){
		tempreg |= (pTxMsg->StdId << CAN_TIR_STID);
	}
	else{
		tempreg |= (pTxMsg->ExtId << CAN_TIR_EXID);
		tempreg |= (1 << CAN_TIR_IDE);
	}
	if(pTxMsg->RTR == CAN_RTR_REMOTE)
		tempreg |= (1 << CAN_TIR_RTR);

	pCANHandle->pCANx->sTxMailBox[mailbox].TIR  = tempreg;
	pCANHandle->pCANx->sTxMailBox[mailbox].TDTR = (pTxMsg->DLC & 0xFU);

	pCANHandle->pCANx->sTxMailBox[mailbox].TDLR =
		((uint32_t)pTxMsg->Data[3] << 24) | ((uint32_t)pTxMsg->Data[2] << 16) |
		((uint32_t)pTxMsg->Data[1] <<  8) | ((uint32_t)pTxMsg->Data[0]);

	pCANHandle->pCANx->sTxMailBox[mailbox].TDHR =
		((uint32_t)pTxMsg->Data[7] << 24) | ((uint32_t)pTxMsg->Data[6] << 16) |
		((uint32_t)pTxMsg->Data[5] <<  8) | ((uint32_t)pTxMsg->Data[4]);

	//2. Enable TX mailbox empty interrupt — ISR fires when mailbox becomes empty after TX
	pCANHandle->pCANx->IER |= (1 << CAN_IER_TMEIE);

	//3. Mark driver as busy
	pCANHandle->TxState = CAN_BUSY_IN_TX;

	//4. Set TXRQ — frame enters bus scheduler, transmission starts
	pCANHandle->pCANx->sTxMailBox[mailbox].TIR |= (1 << CAN_TIR_TXRQ);

	return mailbox;
}


/***********************************************************************************
 * @fn			- CAN_ReceiveMessageIT (non-blocking)
 *
 * @brief		- Arms the specified FIFO to generate an interrupt on message arrival.
 *                The ISR reads the message into pRxMsg and calls the callback.
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 * @param[in]	- FIFONumber: CAN_FIFO0 or CAN_FIFO1
 * @param[out]	- pRxMsg: pointer to RX message buffer filled by the ISR
 *
 * @return		- CAN_READY on success
 *                CAN_BUSY_IN_RX0 or CAN_BUSY_IN_RX1 if already armed for that FIFO
 *
 * @Note		- The FIFO interrupt is disarmed after each received message.
 *                Call CAN_ReceiveMessageIT() again inside the callback to re-arm
 *                for continuous reception.
 *
 * */
uint8_t CAN_ReceiveMessageIT(CAN_Handle_t *pCANHandle, uint8_t FIFONumber, CAN_RxMsg_t *pRxMsg){
	//Save the receive buffer pointer so the ISR can write directly into it
	pCANHandle->pRxMsg = pRxMsg;
	pCANHandle->RxFIFO = FIFONumber;

	if(FIFONumber == CAN_FIFO0){
		if(pCANHandle->RxState == CAN_BUSY_IN_RX0)
			return CAN_BUSY_IN_RX0;	// Already armed for FIFO 0

		pCANHandle->RxState = CAN_BUSY_IN_RX0;

		//Enable FIFO 0 message pending interrupt (fires as long as FMP0 > 0)
		pCANHandle->pCANx->IER |= (1 << CAN_IER_FMPIE0);

		//Enable FIFO 0 overrun interrupt so the application is notified of lost frames
		pCANHandle->pCANx->IER |= (1 << CAN_IER_FOVIE0);
	}
	else{
		if(pCANHandle->RxState == CAN_BUSY_IN_RX1)
			return CAN_BUSY_IN_RX1;	// Already armed for FIFO 1

		pCANHandle->RxState = CAN_BUSY_IN_RX1;

		//Enable FIFO 1 message pending interrupt
		pCANHandle->pCANx->IER |= (1 << CAN_IER_FMPIE1);

		//Enable FIFO 1 overrun interrupt
		pCANHandle->pCANx->IER |= (1 << CAN_IER_FOVIE1);
	}

	return CAN_READY;
}


/*
 * ============================================================
 * IRQ CONFIGURATION
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_IRQITConfig
 *
 * @brief		- Enables or disables a CAN interrupt line in the NVIC
 *
 * @param[in]	- IRQNumber: NVIC IRQ number (use IRQ_NO_CAN1_TX, IRQ_NO_CAN1_RX0, etc.)
 * @param[in]	- ENorDI: ENABLE or DISABLE
 *
 * @return		- none
 *
 * @Note		- none
 *
 * */
void CAN_IRQITConfig(uint8_t IRQNumber, uint8_t ENorDI){
	if(ENorDI == ENABLE){
		if(IRQNumber <= 31)
			*NVIC_ISER0 |= (1 << IRQNumber);
		else if(IRQNumber > 31 && IRQNumber < 64)
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		else if(IRQNumber >= 64 && IRQNumber < 96)
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
	}
	else{
		if(IRQNumber <= 31)
			*NVIC_ICER0 |= (1 << IRQNumber);
		else if(IRQNumber > 31 && IRQNumber < 64)
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		else if(IRQNumber >= 64 && IRQNumber < 96)
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
	}
}


/***********************************************************************************
 * @fn			- CAN_IRQPriorityConfig
 *
 * @brief		- Sets the priority of a CAN interrupt in the NVIC
 *
 * @param[in]	- IRQNumber: NVIC IRQ number
 * @param[in]	- IRQPriority: priority value (0 = highest; STM32F446 implements 4 bits → 0-15)
 *
 * @return		- none
 *
 * @Note		- For the latency project: assign CAN_RX0 a higher priority than
 *                any other interrupt to minimise RX ISR latency jitter.
 *
 * */
void CAN_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority){
	uint8_t iprx         = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASEADDR + iprx) |= (IRQPriority << shift_amount);
}


/*
 * ============================================================
 * ISR HANDLING — call these from your CAN IRQ handler stubs
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_TX_IRQHandling
 *
 * @brief		- TX interrupt handler — place a call to this inside
 *                CAN1_TX_IRQHandler() or CAN2_TX_IRQHandler() in your application
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 *
 * @return		- none
 *
 * @Note		- Calls the private can_tx_mailbox_IThandle() helper which:
 *                  1. Clears the RQCP (request completed) flags for each mailbox
 *                  2. Disables TMEIE once all mailboxes are empty
 *                  3. Fires CAN_ApplicationEventCallback(CAN_EVENT_TX_MAILBOX_EMPTY)
 *
 * */
void CAN_TX_IRQHandling(CAN_Handle_t *pCANHandle){
	can_tx_mailbox_IThandle(pCANHandle);
}


/***********************************************************************************
 * @fn			- CAN_RX0_IRQHandling
 *
 * @brief		- RX FIFO 0 interrupt handler — call from CAN1_RX0_IRQHandler()
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 *
 * @return		- none
 *
 * @Note		- Handles two FIFO 0 events:
 *                  FMPIE0 — message pending → read message, release slot, callback
 *                  FOVIE0 — overrun         → clear flag, callback with overrun event
 *
 * */
void CAN_RX0_IRQHandling(CAN_Handle_t *pCANHandle){
	uint8_t temp1, temp2;

	//Check: at least one message is pending in FIFO 0 and FMPIE0 is enabled
	temp1 = (pCANHandle->pCANx->RF0R & (0x3U << CAN_RF0R_FMP0)) ? 1 : 0;
	temp2 = (pCANHandle->pCANx->IER  & (1    << CAN_IER_FMPIE0)) ? 1 : 0;
	if(temp1 && temp2){
		can_rx_fifo_IThandle(pCANHandle, CAN_FIFO0);
	}

	//Check: FIFO 0 overrun and FOVIE0 enabled
	temp1 = (pCANHandle->pCANx->RF0R & (1 << CAN_RF0R_FOVR0)) ? 1 : 0;
	temp2 = (pCANHandle->pCANx->IER  & (1 << CAN_IER_FOVIE0)) ? 1 : 0;
	if(temp1 && temp2){
		pCANHandle->pCANx->RF0R |= (1 << CAN_RF0R_FOVR0);	// Clear overrun flag by writing 1
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_RX_FIFO0_OVERRUN);
	}
}


/***********************************************************************************
 * @fn			- CAN_RX1_IRQHandling
 *
 * @brief		- RX FIFO 1 interrupt handler — call from CAN1_RX1_IRQHandler()
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 *
 * @return		- none
 *
 * @Note		- Mirrors CAN_RX0_IRQHandling() for FIFO 1
 *
 * */
void CAN_RX1_IRQHandling(CAN_Handle_t *pCANHandle){
	uint8_t temp1, temp2;

	//Check: message pending in FIFO 1
	temp1 = (pCANHandle->pCANx->RF1R & (0x3U << CAN_RF1R_FMP1)) ? 1 : 0;
	temp2 = (pCANHandle->pCANx->IER  & (1    << CAN_IER_FMPIE1)) ? 1 : 0;
	if(temp1 && temp2){
		can_rx_fifo_IThandle(pCANHandle, CAN_FIFO1);
	}

	//Check: FIFO 1 overrun
	temp1 = (pCANHandle->pCANx->RF1R & (1 << CAN_RF1R_FOVR1)) ? 1 : 0;
	temp2 = (pCANHandle->pCANx->IER  & (1 << CAN_IER_FOVIE1)) ? 1 : 0;
	if(temp1 && temp2){
		pCANHandle->pCANx->RF1R |= (1 << CAN_RF1R_FOVR1);
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_RX_FIFO1_OVERRUN);
	}
}


/***********************************************************************************
 * @fn			- CAN_SCE_IRQHandling
 *
 * @brief		- Status Change / Error interrupt handler
 *                Call from CAN1_SCE_IRQHandler() or CAN2_SCE_IRQHandler()
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 *
 * @return		- none
 *
 * @Note		- When CAN_EVENT_ERROR fires the application should read ESR to
 *                diagnose the fault:
 *                  ESR[BOFF]   — Bus-off: TX error counter exceeded 255
 *                  ESR[EPVF]   — Error passive: error counter >= 128
 *                  ESR[EWGF]   — Error warning: error counter >= 96
 *                  ESR[LEC]    — Last error code (bit-error, stuff-error, etc.)
 *                  ESR[TEC/REC]— Transmit / Receive error counters
 *
 * */
void CAN_SCE_IRQHandling(CAN_Handle_t *pCANHandle){
	//Check ERRI flag in MSR — set by hardware when an error condition occurs
	if(pCANHandle->pCANx->MSR & (1 << CAN_MSR_ERRI)){
		//Clear ERRI by writing 1 (rc_w1 — read/clear with write-1)
		pCANHandle->pCANx->MSR |= (1 << CAN_MSR_ERRI);
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_ERROR);
	}
}


/*
 * ============================================================
 * APPLICATION CALLBACK
 * ============================================================
 */

/***********************************************************************************
 * @fn			- CAN_ApplicationEventCallback
 *
 * @brief		- Weak default callback — override in your application source file
 *
 * @param[in]	- pCANHandle: pointer to the CAN handle that triggered the event
 * @param[in]	- AppEvent: event code (CAN_EVENT_TX_MAILBOX_EMPTY, CAN_EVENT_RX_FIFO0_MSG, ...)
 *
 * @return		- none
 *
 * @Note		- Declared __weak so the linker uses the application's version when
 *                one is provided, without modifying this driver file.
 *
 * */
__weak void CAN_ApplicationEventCallback(CAN_Handle_t *pCANHandle, uint8_t AppEvent){
	//Weak implementation — override this function in your application
	(void)pCANHandle;
	(void)AppEvent;
}


/*
 * ============================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================
 */

/***********************************************************************************
 * @fn			- can_get_free_tx_mailbox (private)
 *
 * @brief		- Checks the TME0/TME1/TME2 bits in TSR and returns the index of
 *                the first empty TX mailbox
 *
 * @param[in]	- pCANx: base address of the CAN peripheral
 *
 * @return		- CAN_TX_MAILBOX0, CAN_TX_MAILBOX1, CAN_TX_MAILBOX2, or CAN_TX_MAILBOX_NONE
 *
 * */
static uint8_t can_get_free_tx_mailbox(CAN_RegDef_t *pCANx){
	//TME0/TME1/TME2 at bits 26/27/28 in TSR indicate empty (ready) mailboxes
	if(pCANx->TSR & (1 << CAN_TSR_TME0))  return CAN_TX_MAILBOX0;
	if(pCANx->TSR & (1 << CAN_TSR_TME1))  return CAN_TX_MAILBOX1;
	if(pCANx->TSR & (1 << CAN_TSR_TME2))  return CAN_TX_MAILBOX2;
	return CAN_TX_MAILBOX_NONE;
}


/***********************************************************************************
 * @fn			- can_tx_mailbox_IThandle (private)
 *
 * @brief		- Processes the TX mailbox empty interrupt:
 *                  1. Clears RQCP flags for any completed mailboxes
 *                  2. Disables TMEIE once all mailboxes are free
 *                  3. Resets TxState and notifies the application
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 *
 * */
static void can_tx_mailbox_IThandle(CAN_Handle_t *pCANHandle){
	//Clear RQCP (Request Completed) for each mailbox that has finished
	//RQCP is a read/clear-by-write-1 flag
	if(pCANHandle->pCANx->TSR & (1 << CAN_TSR_RQCP0)){
		pCANHandle->pCANx->TSR |= (1 << CAN_TSR_RQCP0);
	}
	if(pCANHandle->pCANx->TSR & (1 << CAN_TSR_RQCP1)){
		pCANHandle->pCANx->TSR |= (1 << CAN_TSR_RQCP1);
	}
	if(pCANHandle->pCANx->TSR & (1 << CAN_TSR_RQCP2)){
		pCANHandle->pCANx->TSR |= (1 << CAN_TSR_RQCP2);
	}

	//If all three mailboxes are now empty, disable TMEIE to stop further TX interrupts
	if(pCANHandle->pCANx->TSR & ((1 << CAN_TSR_TME0) | (1 << CAN_TSR_TME1) | (1 << CAN_TSR_TME2))){
		pCANHandle->pCANx->IER &= ~(1 << CAN_IER_TMEIE);
		pCANHandle->TxState = CAN_READY;
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_TX_MAILBOX_EMPTY);
	}
}


/***********************************************************************************
 * @fn			- can_rx_fifo_IThandle (private)
 *
 * @brief		- Reads one pending message from the given FIFO into the buffer
 *                registered via CAN_ReceiveMessageIT(), releases the FIFO slot,
 *                disarms the interrupt, and fires the application callback
 *
 * @param[in]	- pCANHandle: pointer to CAN handle
 * @param[in]	- FIFONumber: CAN_FIFO0 or CAN_FIFO1
 *
 * */
static void can_rx_fifo_IThandle(CAN_Handle_t *pCANHandle, uint8_t FIFONumber){
	CAN_RxMsg_t *pRxMsg = pCANHandle->pRxMsg;

	//Read identifier
	if(!(pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR & (1 << CAN_RIR_IDE))){
		pRxMsg->IDE   = CAN_ID_STD;
		pRxMsg->StdId = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR >> CAN_RIR_STID) & 0x7FFU;
	}
	else{
		pRxMsg->IDE   = CAN_ID_EXT;
		pRxMsg->ExtId = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR >> CAN_RIR_EXID) & 0x1FFFFFFFU;
	}

	pRxMsg->RTR = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RIR  >> CAN_RIR_RTR)  & 0x1U;
	pRxMsg->DLC = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDTR)                  & 0xFU;
	pRxMsg->FMI = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDTR >> CAN_RDTR_FMI) & 0xFFU;

	pRxMsg->Data[0] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR)        & 0xFFU;
	pRxMsg->Data[1] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >>  8)  & 0xFFU;
	pRxMsg->Data[2] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >> 16)  & 0xFFU;
	pRxMsg->Data[3] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDLR >> 24)  & 0xFFU;
	pRxMsg->Data[4] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR)        & 0xFFU;
	pRxMsg->Data[5] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >>  8)  & 0xFFU;
	pRxMsg->Data[6] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >> 16)  & 0xFFU;
	pRxMsg->Data[7] = (pCANHandle->pCANx->sFIFOMailBox[FIFONumber].RDHR >> 24)  & 0xFFU;

	//Release FIFO mailbox slot — hardware can now use it for the next incoming frame
	if(FIFONumber == CAN_FIFO0)
		pCANHandle->pCANx->RF0R |= (1 << CAN_RF0R_RFOM0);
	else
		pCANHandle->pCANx->RF1R |= (1 << CAN_RF1R_RFOM1);

	//Disarm the interrupt and reset state — call CAN_ReceiveMessageIT() again to re-arm
	if(FIFONumber == CAN_FIFO0){
		pCANHandle->pCANx->IER &= ~(1 << CAN_IER_FMPIE0);
		pCANHandle->RxState = CAN_READY;
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_RX_FIFO0_MSG);
	}
	else{
		pCANHandle->pCANx->IER &= ~(1 << CAN_IER_FMPIE1);
		pCANHandle->RxState = CAN_READY;
		CAN_ApplicationEventCallback(pCANHandle, CAN_EVENT_RX_FIFO1_MSG);
	}
}
