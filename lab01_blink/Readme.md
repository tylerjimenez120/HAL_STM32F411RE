# Lab 01 — HAL GPIO Blink

Blink the green LED (PA5) on the Nucleo-64 using STM32 HAL.

## What this lab covers
- HAL project structure without CubeMX
- SystemClock_Config — configuring HSI at 16MHz
- HAL_GPIO_Init — configure PA5 as output
- HAL_GPIO_TogglePin — toggle LED
- HAL_Delay — millisecond delay via SysTick

## Bare metal vs HAL

| Bare metal | HAL |
|---|---|
| `RCC_AHB1ENR \|= (1U << 0)` | `__HAL_RCC_GPIOA_CLK_ENABLE()` |
| `GPIOA_MODER \|= (0x1U << 10)` | `HAL_GPIO_Init(GPIOA, &cfg)` |
| `GPIOA_ODR ^= (1U << 5)` | `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5)` |

## Build

make
make flash

## Expected result
Green LED blinks every 500ms.