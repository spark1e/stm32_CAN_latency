# CAN Network Communication: Real-Time Latency Analysis
**STM32F446RE + STM32L476RG + BeagleBone Black | bare-metal | 500 kbps**

A bare-metal embedded project that measures round-trip CAN latency between two STM32 nodes monitored by a BeagleBone Black, testing whether the system can meet a real-time deadline under bus load. Written entirely at the register level — no HAL.

---

## The Question

Can a 2-node CAN-based embedded system guarantee a real-time deadline under bus load? How does interrupt-driven RX compare to polling in latency and worst-case behavior? Does FreeRTOS add overhead or improve determinism?

---

## Hardware

| Component | Role |
|---|---|
| STM32F446RE (Nucleo-64) | Node 1 — transmitter, latency measurement |
| STM32L476RG (Nucleo-64) | Node 2 — receiver + echo responder |
| BeagleBone Black | Passive CAN bus logger via SocketCAN |
| SN65HVD230 x2 | CAN transceivers |

---

## Project Phases

### Phase 1 — Bare-Metal CAN Driver

Two STM32 boards communicating over CAN with no HAL, no vendor middleware. BeagleBone Black passively monitors and logs all bus traffic via Linux SocketCAN.

- Register-level bxCAN driver from scratch (no HAL)
- 500 kbps CAN bus, SN65HVD230 transceivers, PB8/PB9
- TIM2 microsecond counter (1 µs resolution) for timestamps
- UART debug output at 115200 baud
- BeagleBone Black logs all frames via SocketCAN for post-capture analysis

### Phase 2 — Latency Measurement + Load Test (complete)

Round-trip latency measurement under bus saturation, comparing polling vs interrupt-driven RX.

- Node 1 sends measurement frame (ID 0x100), records T0 via TIM2
- Node 2 receives frame, immediately echoes back (ID 0x200)
- **Polling:** Node 1 records T1 after `CAN_ReceiveMessage` returns
- **Interrupt:** T1 recorded inside `CAN1_RX0_IRQHandler` — before any processing
- Load test: 2 non-blocking background frames in mailbox 0/1 + 10 blocking burst frames (ID 0x7FF)
- CAN arbitration verified: ID 0x100 preempts ID 0x7FF (lower ID = higher priority)

### Phase 3 — FreeRTOS vs Bare-Metal (planned)

Replace polling loop with FreeRTOS task architecture and compare worst-case latency against bare-metal interrupt results.

---

## Results

### Phase 2 — Polling vs Interrupt

| Mode | Load | Avg Latency | Deadline 9 ms |
|---|---|---|---|
| Polling | No load | 8,570 µs | MET |
| Polling | 10 burst frames | 11,974 µs | MISSED |
| Interrupt | No load | 2,601 µs | MET |
| Interrupt | 10 burst frames | 10,610 µs | MISSED |

**Key finding:** Interrupt-driven RX is **3.3× faster** than polling on baseline (2,601 µs vs 8,570 µs). The ~6,000 µs difference is the overhead of `CAN_ReceiveMessage` polling the FIFO status register in a blocking loop. With interrupts, T1 is captured at the exact moment the frame arrives in hardware — before any software processing.

Under load, both modes miss the 9 ms deadline, but interrupt mode misses by less (10,610 µs vs 11,974 µs). The remaining latency is dominated by CAN bus arbitration time — 10 low-priority frames (ID 0x7FF) contending with the measurement frame (ID 0x100).

### Bare-Metal STM32 vs Linux Node (BeagleBone Black)

When BeagleBone Black acts as the ACK node instead of STM32 L476RG:

| Node 2 | Latency under load | Deadline |
|---|---|---|
| STM32L476RG bare-metal | ~10,610 µs | MISSED (9ms) |
| BeagleBone Black (Linux) | 203,451 µs | MISSED (200ms) |

Linux scheduling adds ~19× latency compared to bare-metal interrupt under the same bus load.

---

## Setup

| Parameter | Value |
|---|---|
| CAN speed | 500 kbps |
| Node 1 clock (F446RE) | 16 MHz (HSI) |
| Node 2 clock (L476RG) | 16 MHz (HSI) |
| Timer resolution | 1 µs (TIM2, PSC=15) |
| Deadline | 9 ms |
| UART baud | 115200 |
| CAN pins | PB8 (RX), PB9 (TX), AF9 |
| Load | 2 non-blocking + 10 burst frames, ID 0x7FF |

---

## Tech Stack

- Language: C (bare-metal, no HAL)
- IDE: STM32CubeIDE
- BeagleBone: Linux SocketCAN, passive logger
- Debug: UART, Logic Analyzer

---

## Author

Nikita Volkov — [github.com/spark1e](https://github.com/spark1e)
