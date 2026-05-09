# HAL STM32F411RE

STM32F411RE Nucleo-64 labs using STM32 HAL — no CubeMX, no IDE, just VSCode and Makefile.

---

## Prerequisites

### Option A — Local toolchain
- arm-none-eabi-gcc
- make
- openocd

### Option B — Docker (no installation needed)
Just have Docker installed. The container includes the toolchain and STM32CubeF4 drivers.

---

## Quick start — Docker

```bash
# Pull the image (first time only)
sudo docker pull tylerjimenez120/stm32-hal

# Compile any lab
sudo docker run --rm -v $(pwd):/workspace stm32-hal \
    make -C /workspace/labXX_name \
    CUBE=/opt/STM32CubeF4 \
    PREFIX=/usr/bin/arm-none-eabi

# Flash to board (requires physical hardware + ST-Link)
cd labXX_name
make flash


> **Note:** Docker handles compilation only. Flashing requires physical hardware.
> Install OpenOCD locally to flash:
> ```
> sudo apt install openocd
> make flash
> ```
```

---

## Quick start — Local

```bash
# Clone STM32CubeF4 once
git clone --depth=1 https://github.com/STMicroelectronics/STM32CubeF4.git
cd STM32CubeF4 && git submodule update --init --recursive

# Build and flash
cd labXX_name
make
make flash
```

---

## Labs

| Lab | Topic | Key concept |
|-----|-------|-------------|
| [lab01_blink](./lab01_blink) | HAL GPIO — LED blink | HAL_GPIO_Init, HAL_Delay |
| [lab02_uart](./lab02_uart) | HAL UART — serial debug | UART handle, MspInit, HAL_UART_Transmit |

> Actively updated. More labs coming soon.

---

## Project structure
labXX_name/
├── Inc/
│   └── stm32f4xx_hal_conf.h   ← HAL module selection
├── Src/
│   └── main.c
├── startup_stm32f411xe.s      ← ST-provided startup
├── STM32F411RETX_FLASH.ld     ← linker script
└── Makefile                   ← CUBE and PREFIX are overridable


---

## Related repos

- [Bare Metal STM32F411RE](https://github.com/tylerjimenez120/Bare_metal_stm32f411re) — Phase 1
- HAL STM32F411RE (this repo) — Phase 2
- FreeRTOS STM32F411RE — coming soon

---

## Part of an ongoing series

Bare Metal → HAL → FreeRTOS → libopencm3