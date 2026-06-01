# Lab 08 — HAL RTC

Real Time Clock keeping track of date and time, running independently from the system clock.

---

## What this lab covers

- RTC_HandleTypeDef — HAL handle for the RTC
- LSI oscillator (Low Speed Internal) as RTC clock source
- Async + sync prescalers — getting 1 Hz from 32 kHz
- HAL_RTC_SetTime / SetDate — initialize the clock
- HAL_RTC_GetTime / GetDate — read current time
- BCD vs BIN formats
- Why TIME must be read before DATE

---

## Key concepts

### What is the RTC
A peripheral that counts real-world time (year, month, day, hour, minute, second). Unlike SysTick which counts milliseconds since boot, the RTC tracks calendar time.

### Clock sources
The STM32 has 4 oscillators:

| Oscillator | Speed | Use |
|------------|-------|-----|
| HSI | 16 MHz internal | CPU, peripherals |
| HSE | external crystal | CPU, peripherals (Nucleo doesn't have one) |
| LSI | ~32 kHz internal | RTC, watchdog (used here) |
| LSE | 32.768 kHz external | RTC (Nucleo doesn't have one) |

LSE would be more accurate but the Nucleo-64 board doesn't have it soldered.

### The two prescalers
The RTC needs to count 1 second per tick. It divides the LSI frequency in two stages:

```
LSI 32 kHz
   │
   ▼ async prescaler = 127 (divide by 128)
   │
250 Hz
   │
   ▼ sync prescaler = 249 (divide by 250)
   │
1 Hz → one tick per second
```

Formula:
```
1 Hz = LSI / ((async+1) × (sync+1))
1 Hz = 32000 / (128 × 250) = 1 Hz ✓
```

Two stages instead of one save power — async runs unsynchronized (low power), sync provides fine adjustment.

### Time vs Date — read order matters
```c
HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);  // FIRST
HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  // SECOND
```

GetTime locks the RTC registers internally to prevent inconsistent reads. If you read DATE first, seconds might tick over mid-read and give a corrupted timestamp.

### BIN vs BCD format
```
RTC_FORMAT_BIN → standard binary (14 = decimal 14)
RTC_FORMAT_BCD → binary-coded decimal (14 = 0x14)
```
BIN is easier for snprintf and arithmetic.

### Year storage
The RTC stores year as 2 digits (00-99). To display 2026, prefix with "20":
```c
snprintf(buf, sizeof(buf), "20%02d-%02d-%02d", sDate.Year, ...);
```

### Where the RTC lives
The RTC and its backup registers live in a separate power domain:

```
STM32F411
├── CPU + normal peripherals    (off in sleep modes)
└── RTC + backup domain         (keeps running)
```

With a VBAT battery (CR2032 in production), the RTC survives full power loss. The Nucleo doesn't expose VBAT by default — the time resets on every power cycle.

---

## Two-step clock setup

### Step 1 — connect oscillator to RTC (SystemClock_Config)
```c
RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
RCC_OscInit.LSIState = RCC_LSI_ON;

RCC_PeriphCLKInitTypeDef periphCLK = {0};
periphCLK.PeriphClockSelection = RCC_PERIPHCLK_RTC;
periphCLK.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;
HAL_RCCEx_PeriphCLKConfig(&periphCLK);
```
This activates LSI and connects it as the RTC clock source.

### Step 2 — enable RTC peripheral (MspInit)
```c
__HAL_RCC_RTC_ENABLE();
```
This powers on the RTC peripheral itself.

---

## Hardware
No external wiring required. The RTC is fully internal.

The Nucleo-64 does NOT have:
- LSE crystal (32.768 kHz) — would give better accuracy
- VBAT battery connection — time would survive power loss

---

## Bare metal vs HAL

| Bare metal | HAL |
|------------|-----|
| Unlock RTC backup domain manually | `HAL_RTC_Init` does it |
| Wait for RTC sync flags manually | HAL handles synchronization |
| Configure async/sync prescalers in raw registers | `Init.AsynchPrediv` / `SynchPrediv` |
| Manual BCD encoding | `RTC_FORMAT_BIN` and HAL converts |

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

## Expected output

```
STM32 HAL RTC Lab
2026-05-25 14:30:00
2026-05-25 14:30:01
2026-05-25 14:30:02
2026-05-25 14:30:03
...
```

The clock increments once per second. Each second is generated entirely in hardware by LSI + prescalers.

---

## Common variations

| Change | Result |
|--------|--------|
| Change initial date/time in RTC_Init | Different starting timestamp |
| Use 12-hour format | Add AM/PM field to sTime |
| Add alarm with `HAL_RTC_SetAlarm` | Trigger event at specific time |
| Switch to LSE (with crystal) | Much better accuracy (~ppm vs ~%) |

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Time doesn't advance | LSI not enabled | Check `RCC_OscInit.LSIState = RCC_LSI_ON` |
| Compile error: RTC undefined | Module not enabled | `#define HAL_RTC_MODULE_ENABLED` in hal_conf.h |
| Time drifts noticeably | LSI is imprecise (~ppm error) | Use LSE crystal in production |
| Date wrong but time correct | Read order swapped | Always GetTime first, GetDate second |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) <- this repo
- FreeRTOS — coming soon
