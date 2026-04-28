# HAL STM32F411RE

STM32F411RE Nucleo-64 labs using STM32 HAL — no CubeMX, no IDE, just VSCode and Makefile.

## Prerequisites

### Toolchain
- arm-none-eabi-gcc
- make
- openocd

### STM32CubeF4 — clone separately
This repo does NOT include the HAL drivers (too large).
Clone them once at `~/stm32/hal_stm32/STM32CubeF4`:

git clone --depth=1 https://github.com/STMicroelectronics/STM32CubeF4.git
cd STM32CubeF4
git submodule update --init --recursive

## Labs

| Lab | Topic |
|-----|-------|
| [lab01_blink](./lab01_blink) | HAL GPIO — LED blink |

> Actively updated. More labs coming soon.