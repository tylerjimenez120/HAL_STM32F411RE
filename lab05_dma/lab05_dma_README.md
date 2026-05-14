# Lab 05 — HAL DMA + UART

Non-blocking UART communication using DMA. CPU stays free while data is transmitted in hardware.

---

## What this lab covers

- DMA_HandleTypeDef — the HAL handle for DMA
- Linking DMA handles to UART handle via __HAL_LINKDMA
- HAL_UART_Transmit_DMA — non-blocking TX (returns immediately)
- HAL_UART_Receive_DMA — non-blocking RX (returns immediately)
- Callbacks: HAL_UART_TxCpltCallback, HAL_UART_RxCpltCallback
- NVIC interrupt configuration for DMA streams AND USART
- Why USART2_IRQHandler is required even when using DMA

---

## Key concept — the full interrupt chain

DMA completing the transfer is NOT the same as UART finishing transmission.
The DMA copies bytes to the USART data register, but the last byte still
needs to clock out through the USART shift register physically.

Full chain when TX completes:

```
DMA copies last byte to USART DR
    │
    ▼
DMA1_Stream6_IRQHandler  ← DMA interrupt
    │
    ▼
HAL_DMA_IRQHandler       ← marks DMA done
    │
    ▼
USART shift register flushes last bit out the pin
    │
    ▼
USART2_IRQHandler        ← UART interrupt (TC flag)
    │
    ▼
HAL_UART_IRQHandler      ← HAL processes event
    │
    ▼
HAL_UART_TxCpltCallback  ← finally fires
    │
    ▼
tx_complete = 1          ← main() sees it
```

If USART2_IRQn is NOT enabled, the chain breaks at step 4 and the callback never fires.

---

## Three required NVIC enables

```c
HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);   // DMA TX
HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);   // DMA RX
HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);         // UART itself — REQUIRED
HAL_NVIC_EnableIRQ(USART2_IRQn);
```

And three IRQ handlers:

```c
void DMA1_Stream6_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart2_tx); }
void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart2_rx); }
void USART2_IRQHandler(void)       { HAL_UART_IRQHandler(&huart2); }
```

---

## DMA stream/channel mapping

These values are fixed in the STM32F411 hardware — see Reference Manual Table 27:

| Peripheral | DMA | Stream | Channel |
|------------|-----|--------|---------|
| USART2_TX  | DMA1 | Stream6 | Channel4 |
| USART2_RX  | DMA1 | Stream5 | Channel4 |

You cannot change these — they are wired in silicon.

---

## __HAL_LINKDMA — the critical link

```c
__HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
__HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);
```

These macros link the DMA handles to the UART handle.
Without these, HAL_UART_Transmit_DMA does not know which DMA to use.

After linking:
- huart2.hdmatx → points to hdma_usart2_tx
- huart2.hdmarx → points to hdma_usart2_rx

---

## DMA Init parameters explained

| Parameter | Value | Why |
|-----------|-------|-----|
| Direction | MEMORY_TO_PERIPH (TX) | RAM → USART2 |
| Direction | PERIPH_TO_MEMORY (RX) | USART2 → RAM |
| PeriphInc | DISABLE | USART2 DR stays at same address |
| MemInc | ENABLE | advance through buf[0], buf[1]... |
| DataAlignment | BYTE | UART works in 8-bit bytes |
| Mode | NORMAL | transfer N bytes and stop |
| Priority | LOW | not time-critical |
| FIFOMode | DISABLE | direct mode — no buffering |

---

## Flag handling pattern

```c
tx_complete = 0;                    // 1. reset BEFORE launching
HAL_UART_Transmit_DMA(...);         // 2. launch — returns immediately
while (!tx_complete);               // 3. wait for callback
```

The flag must be volatile because it is written by an ISR and read by main.

---

## Common pitfall — DMA done ≠ UART done

If you only enable DMA interrupts, the TxCpltCallback never fires.
The LED toggle test confirmed this: callback fired exactly once on the
first transmission (when something else triggered it) but never again.

Fix: enable USART2_IRQn AND implement USART2_IRQHandler.

---

## Bare metal vs HAL DMA

| Bare metal | HAL |
|---|---|
| Manual DMA register config | `HAL_DMA_Init(&hdma)` |
| Manual stream/channel selection | Same — hardware constrained |
| Manual ISR with status flag clearing | `HAL_DMA_IRQHandler` |
| Manual TX complete check | Callback pattern |

---

## Hardware

| Pin Nucleo | Label | STM32 Pin | Function |
|------------|-------|-----------|----------|
| —          | —     | PA2       | USART2 TX (to ST-Link VCP) |
| —          | —     | PA3       | USART2 RX (from ST-Link VCP) |

DMA uses no external pins — it operates on internal buses.

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

## Expected output

```
STM32 HAL DMA UART
TX done. CPU counted: 12345
Send 10 chars:
```

After typing 10 characters in minicom:

```
Received: helloworld
```

The CPU counter value proves the CPU was working while DMA transmitted.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| First message prints, then hangs | USART2_IRQn not enabled | Enable USART2 NVIC + implement USART2_IRQHandler |
| Callback fires once then never | DMA IRQ alone insufficient | Add USART2 interrupt to chain |
| Counter is 0 or very small | Optimizer removing the loop | Declare counter as volatile |
| RX never completes | DMA RX not linked | Verify __HAL_LINKDMA for hdmarx |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) ← this repo
- FreeRTOS — coming soon
