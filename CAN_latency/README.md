# CAN Network Communication: Real-Time Latency Analysis
### STM32F446RE + STM32L476RG + BeagleBone Black

A bare-metal embedded project that measures how fast a CAN message
can travel between two STM32 nodes — STM32F446RE (transmitter) and
STM32L476RG (receiver) — monitored by a BeagleBone Black.
The goal is to test if the system can meet a 200ms response deadline
under different load conditions — and how FreeRTOS changes that behavior.

## The Question
> Can a 2-node CAN-based embedded system guarantee a 200ms response deadline under load?
> Does FreeRTOS improve or add overhead compared to bare-metal?

## Hardware
- STM32F446RE (Nucleo-64) — Node 1, main transmitter
- STM32L476RG (Nucleo-64) — Node 2, receiver + responder
- BeagleBone Black — monitors CAN bus, logs data
- SN65HVD230 CAN transceivers

## Project Phases

### Phase 1 — Basic CAN Driver
Getting two STM32 boards talking over CAN without HAL libraries.
Everything written directly at the register level.

- Bare-metal CAN driver on both STM32 boards (no HAL)
- 500 kbps CAN bus
- Microsecond timer (TIM2) for latency measurement
- UART debug output at 115200 baud
- BeagleBone Black passively monitors the bus

### Phase 2 — Interrupt-Driven Latency Measurement (current)
Adding real latency measurement and stress testing.

- Interrupt-driven event detection (ISR/EXTI)
- Timestamp T0 at send, T1 at ACK received
- Latency = T1 - T0 in microseconds
- Test under load: does latency stay under 200ms?
- BeagleBone logs all results via SocketCAN

### Phase 3 — FreeRTOS vs Bare-Metal
Replacing the while(1) loop with FreeRTOS tasks and comparing results.

- FreeRTOS task-based architecture
- High priority tasks for CAN TX/RX
- Compare worst-case latency: bare-metal vs RTOS
- Final answer: which approach meets the deadline more reliably?

## Sample Output
```
[1] latency: 124 us  | DEADLINE MET
[2] latency: 118 us  | DEADLINE MET
[3] latency: 891 us  | DEADLINE MET
[4] latency: 1243 us | DEADLINE MET
...
[load test]
[5] latency: 87432 us | DEADLINE MET
[6] latency: 203451 us | DEADLINE MISSED
```

## Setup
| Parameter | Value |
|-----------|-------|
| CAN speed | 500 kbps |
| Node 1 clock (F446RE) | 180 MHz |
| Node 2 clock (L476RG) | 80 MHz |
| Timer precision | ~1 µs |
| Deadline | 200 ms |
| UART baud | 115200 |

## Tech Stack
- **Language:** C (bare-metal), C with FreeRTOS
- **IDE:** STM32CubeIDE
- **BeagleBone:** Linux SocketCAN, C userspace program
- **Debug:** UART, Logic Analyzer, Oscilloscope

## Author
Nikita Volkov — [github.com/spark1e](https://github.com/spark1e)
