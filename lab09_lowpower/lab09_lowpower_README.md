# Lab 09 — HAL Low Power Mode (STOP + RTC Wakeup)

MCU enters STOP mode and wakes every 5 seconds via RTC.
Demonstrates the critical interaction between SysTick and wakeup sources.

---

## What this lab covers

- STOP mode entry/exit (~10 µA consumption)
- RTC Wakeup Timer as wake source
- HAL_SuspendTick / HAL_ResumeTick — critical for low power
- Backup domain access enable
- Clock reconfiguration after STOP

---

## Key concept — the SysTick trap

**Any enabled interrupt wakes the MCU from STOP — including SysTick.**

SysTick fires every 1 ms by default:

```
STOP -> SysTick (1ms) -> wake -> STOP -> SysTick (1ms) -> wake -> ...
```

The RTC wakeup at 5 seconds never gets to fire because SysTick wakes us first every millisecond.

### The fix

Always wrap STOP mode with tick suspension:

```c
HAL_SuspendTick();
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
HAL_ResumeTick();
```

### Rule of thumb for low power
Before entering STOP, disable ALL interrupts that are not your wake source:
- SysTick
- Unused timers
- UART RX interrupts
- Floating EXTI lines

---

## Other gotchas

### Clear RTC wakeup flags BEFORE arming timer

```c
HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
__HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 5, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
```

The wakeup flag is level-triggered. If left set, the MCU exits STOP immediately.

### Reconfigure system clock after STOP
After STOP the HSI is at default state. Call `SystemClock_Config()` after waking to restore the expected clock setup.

### Enable backup domain access
Once at boot:

```c
__HAL_RCC_PWR_CLK_ENABLE();
HAL_PWR_EnableBkUpAccess();
```

Without this, RTC operations silently fail.

---

## Flow of one cycle

```
1. Toggle LED, send UART message
2. Clear RTC wakeup flags
3. Arm wakeup timer (5 sec)
4. HAL_SuspendTick - disable SysTick
5. EnterSTOPMode -> CPU stops, ~10 uA
   ... 5 seconds elapse ...
6. RTC fires -> wake
7. HAL_ResumeTick - re-enable SysTick
8. SystemClock_Config - restore clocks
9. Loop continues
```

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

### Flashing while MCU is in STOP
OpenOCD cannot connect when MCU is asleep. Hold the RESET button while running `make flash`, release when you see "Programming Started".

---

## Expected output

```
STM32F411RE Low Power RTC Test
Wake every ~5 seconds

Wake #1 -> entering STOP
[5 second pause]
Woke up!
Wake #2 -> entering STOP
[5 second pause]
Woke up!
```

---

## Bare metal vs HAL

| Bare metal | HAL |
|---|---|
| Manual PWR register config | `HAL_PWR_EnterSTOPMode` |
| Manual RTC wakeup setup | `HAL_RTCEx_SetWakeUpTimer_IT` |
| Manual SysTick disable | `HAL_SuspendTick / ResumeTick` |
| Manual ISR for RTC wakeup | `HAL_RTCEx_WakeUpTimerEventCallback` |

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Wakes immediately (no 5s pause) | SysTick enabled during STOP | `HAL_SuspendTick()` before STOP |
| OpenOCD fails to connect | MCU asleep when flashing | Hold RESET while flashing |
| Wakes but data corrupted | Clocks not restored | Call `SystemClock_Config()` after STOP |
| RTC never fires | Backup domain locked | `HAL_PWR_EnableBkUpAccess()` at boot |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) <- this repo
- FreeRTOS - coming soon
