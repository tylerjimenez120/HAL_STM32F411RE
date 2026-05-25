# Lab 06 — HAL Timer + PWM

LED fade using PWM on TIM2 Channel 1 (PA5).

---

## What this lab covers

- TIM_HandleTypeDef — HAL handle for timers
- HAL_TIM_PWM_Init — configure timer base for PWM
- HAL_TIM_PWM_ConfigChannel — configure a specific channel
- HAL_TIM_PWM_Start — start PWM output (hardware-only)
- __HAL_TIM_SET_COMPARE — change duty cycle on the fly
- PSC, ARR, CCR — the three key registers of any timer
- Output Compare modes (PWM1 vs PWM2)

---

## Key concepts

### What is PWM
A square wave with variable duty cycle:

```
0% duty   __________   always LOW
25% duty  ▔▔________   HIGH 25% of the time
50% duty  ▔▔▔▔______   HIGH 50% of the time
75% duty  ▔▔▔▔▔▔____   HIGH 75% of the time
100% duty ▔▔▔▔▔▔▔▔▔▔   always HIGH
```

The LED sees the time-averaged voltage. By switching fast enough
(1 kHz here), the eye perceives a steady brightness level.

### The three timer registers

```
PSC (Prescaler) → divides the input clock
ARR (Period)    → maximum count value before reset
CCR (Compare)   → where HIGH → LOW transition happens
```

Frequency formula:
```
F_pwm = clock_in / ((PSC + 1) × (ARR + 1))
```

Duty cycle formula:
```
duty = CCR / (ARR + 1)
```

### This lab configuration

```
clock_in = 16 MHz HSI
PSC = 15  → 16 MHz / 16 = 1 MHz (timer clock)
ARR = 999 → counts 0-999 = 1000 ticks per cycle
F_pwm = 1 MHz / 1000 = 1 kHz
```

### Why the LED appears to fade smoothly
PWM frequency is 1000 Hz — far above the eye's flicker fusion threshold (~60-80 Hz). The eye sees only the average brightness, which is proportional to duty cycle.

### Hardware does everything
After `HAL_TIM_PWM_Start`, the timer generates the PWM signal entirely in hardware. The CPU is free — it only intervenes when you change CCR to update the duty cycle.

---

## Timer channels on TIM2

A single timer has 4 independent channels sharing the same counter:

| Channel | Possible pins (STM32F411RE) |
|---------|-----------------------------|
| TIM2_CH1 | PA0, PA5 |
| TIM2_CH2 | PA1, PB3 |
| TIM2_CH3 | PA2, PB10 |
| TIM2_CH4 | PA3, PB11 |

All channels share PSC and ARR but each has its own CCR — so you can generate 4 PWM signals at the same frequency but different duty cycles using one timer.

---

## Bare metal vs HAL

| Bare metal | HAL |
|------------|-----|
| Manual register writes (TIMx->PSC, ARR, CCR1) | `HAL_TIM_PWM_Init` + handle |
| Manual GPIO AF setup | `MspInit` callback |
| Manual CR1 enable bits | `HAL_TIM_PWM_Start` |
| Direct CCR1 writes | `__HAL_TIM_SET_COMPARE` macro |

---

## Hardware

| Pin Nucleo | Label | STM32 Pin | Function |
|------------|-------|-----------|----------|
| —          | —     | PA5       | TIM2_CH1 output (LD2 LED on board) |

No external wiring required — the green LED is already connected to PA5.

---

## Build

```bash
make
make flash
```

## Expected result

LED LD2 fades smoothly from off to full brightness and back,
in a ~4 second cycle. No visible flicker — appears to "breathe".

---

## Common variations

| Change | Result |
|--------|--------|
| Reduce `HAL_Delay(2)` to 1 | Faster fade |
| Increase `HAL_Delay(2)` to 5 | Slower fade |
| Change `Period` to 99 | Lower resolution (10 levels instead of 1000) |
| Change `Prescaler` to 7 | Higher PWM frequency (2 kHz) |
| Change polarity to LOW | Inverted fade |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) <- this repo
- FreeRTOS — coming soon
