# Lab 07 — HAL ADC + DMA

Continuous analog sampling using ADC + DMA in circular mode.
A potentiometer on PA1 is sampled at ~41 kHz and the CPU reads the latest value whenever needed.

---

## What this lab covers

- ADC_HandleTypeDef — HAL handle for the ADC
- HAL_ADC_Start_DMA — start continuous sampling with DMA
- DMA in CIRCULAR mode — buffer auto-restarts forever
- Analog GPIO mode — pin disconnected from digital logic
- DMA HALFWORD (16-bit) transfers — ADC outputs 12 bits
- ADC channel selection — internal channel mapped to physical pin
- Sampling time tradeoff — speed vs accuracy

---

## Key concepts

### Polling vs DMA
```
Polling (bare metal style):
  CPU → "ADC, sample"
  CPU → waits
  CPU → reads result
  CPU → 100% busy

DMA + circular (this lab):
  ADC samples continuously in hardware
  DMA writes each result to RAM
  CPU only reads RAM when it wants → instant
```

The CPU can be doing other tasks. The ADC sample in RAM is always fresh.

### Why HALFWORD instead of BYTE
The ADC outputs 12 bits (0-4095). 8 bits would lose data. So the DMA transfers 16-bit values:
```c
hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
```

### Why CIRCULAR mode
```
NORMAL   → transfer N samples then STOP (one-shot)
CIRCULAR → transfer N samples, restart, transfer N more, forever
```
Circular is required for continuous ADC sampling.

### The DMA stream/channel mapping
Like UART, the ADC has fixed DMA assignments in hardware:

| Peripheral | DMA | Stream | Channel |
|------------|-----|--------|---------|
| ADC1 | DMA2 | Stream0 | Channel0 |

Note: ADC1 uses DMA2 (not DMA1 like USART2).

### Analog GPIO mode
Different from any other peripheral:
```c
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
GPIO_InitStruct.Pull = GPIO_NOPULL;
```
In analog mode the pin disconnects from the digital input/output circuit completely — the ADC reads the raw voltage directly.

### Sampling time
```
3 cycles   → fastest but noisy
84 cycles  → balanced (used here)
480 cycles → slowest but most precise
```
High-impedance sources (like a potentiometer) need more sampling time.

With ADC clock at 4 MHz and 84-cycle sampling + 12-cycle conversion:
```
total per sample = (84 + 12) / 4 MHz ≈ 24 µs
sampling rate    ≈ 41 kHz
```

### ADC channel mapping
The ADC channel number is fixed in silicon to a specific pin:

| ADC Channel | Pin |
|-------------|-----|
| ADC_CHANNEL_0 | PA0 |
| ADC_CHANNEL_1 | PA1 |
| ADC_CHANNEL_2 | PA2 |
| ... | ... |
| ADC_CHANNEL_15 | PC5 |

---

## Voltage conversion
```c
uint32_t mv = (value * 3300) / 4095;
```
4095 = 3300 mV (3.3V) → linear interpolation for any value.

---

## Hardware

| Pin Nucleo | Label | STM32 Pin | Function |
|------------|-------|-----------|----------|
| CN8        | A1    | PA1       | ADC1_IN1 (potentiometer wiper) |
| —          | 3.3V  | —         | Potentiometer leg 1 |
| —          | GND   | —         | Potentiometer leg 3 |

```
Potentiometer:
  Leg 1 → 3.3V
  Leg 2 (wiper) → PA1
  Leg 3 → GND
```

---

## Bare metal vs HAL ADC

| Bare metal | HAL |
|------------|-----|
| Manual ADC register setup | `HAL_ADC_Init(&hadc1)` |
| Poll EOC flag manually | `HAL_ADC_Start_DMA` (no polling) |
| Read ADC->DR for each sample | Read `adc_buffer[0]` from RAM |
| No background sampling | Continuous in hardware |

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

## Expected output

Turning the potentiometer changes the values continuously:

```
ADC:    0 (   0 mV)   → at minimum
ADC: 2048 (1650 mV)   → at middle
ADC: 4091 (3296 mV)   → at maximum
```

Values are stable when the potentiometer is held still — proof of clean sampling.

---

## Common variations

| Change | Result |
|--------|--------|
| `ADC_SAMPLETIME_3CYCLES` | Faster sampling, more noise |
| `ADC_SAMPLETIME_480CYCLES` | Slower but very precise |
| `ADC_RESOLUTION_8B` | Faster conversion, less resolution (0-255) |
| Add more channels with Scan mode | Multi-channel sampling |

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| All values = 0 | Potentiometer not connected | Check wiring |
| All values = 4095 | PA1 floating or shorted to VCC | Check GND connection |
| Noisy values | Short sampling time | Increase to 84 or 144 cycles |
| Compile error: ADC undefined | Module not enabled | `#define HAL_ADC_MODULE_ENABLED` in hal_conf.h |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) <- this repo
- FreeRTOS — coming soon
