# Lab 03 — HAL I2C + MPU6050

Read accelerometer and gyroscope data from an MPU6050 sensor over I2C using STM32 HAL.

---

## Hardware

| Pin Nucleo | Label | STM32 Pin | MPU6050 |
|------------|-------|-----------|---------|
| CN9        | D15   | PB8       | SCL     |
| CN9        | D14   | PB9       | SDA     |
| 3.3V       | 3.3V  | —         | VCC     |
| GND        | GND   | —         | GND     |
| GND        | GND   | —         | AD0 (address = 0x68) |

---

## What this lab covers

- I2C_HandleTypeDef — the HAL handle for I2C
- HAL_I2C_MspInit — hardware callback (GPIO open-drain + AF4)
- HAL_I2C_Mem_Read — read registers from I2C device
- HAL_I2C_Mem_Write — write registers to I2C device
- WHO_AM_I — standard technique to verify sensor is connected
- Combining two bytes into int16_t — raw sensor data parsing

---

## Key concepts

### HAL address format
HAL uses 8-bit addresses. The MPU6050 has a 7-bit address (0x68).
Always shift left by 1 when using HAL:

```c
#define MPU6050_ADDR (0x68 << 1)
```

### The handle — identifier + state + config

```c
I2C_HandleTypeDef hi2c1;
hi2c1.Instance = I2C1;  /* which bus */
```

The handle serves three purposes at once:
- Identifies which I2C bus to use (I2C1, I2C2...)
- Holds the bus configuration (clock speed, addressing mode)
- Tracks the current bus state (READY, BUSY, ERROR)

Passing &hi2c1 to every HAL function means:
"use this bus, with this config, and update this state"

### HAL_I2C_Mem_Read

```c
HAL_I2C_Mem_Read(&hi2c1,           // which bus
                 MPU6050_ADDR,      // device address
                 MPU6050_WHO_AM_I,  // register to read
                 1,                 // register size (bytes)
                 &who_am_i,         // where to store result
                 1,                 // how many bytes to read
                 HAL_MAX_DELAY);    // timeout
```

### WHO_AM_I — verify sensor before reading data

Register 0x75 always returns 0x68 on the MPU6050.
Standard technique to confirm the sensor is wired correctly
before attempting to read measurements.

### Wake up the sensor

MPU6050 starts in sleep mode by default.
Writing 0x00 to PWR_MGMT_1 (0x6B) wakes it up:

```c
uint8_t data = 0x00;
HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR,
                  MPU6050_PWR_MGMT_1, 1,
                  &data, 1, HAL_MAX_DELAY);
```

### Reading 14 bytes at once

The MPU6050 auto-increments the register address.
One read starting at ACCEL_XOUT_H (0x3B) returns:

```
buf[0:1]   → AX (high, low)
buf[2:3]   → AY
buf[4:5]   → AZ
buf[6:7]   → TEMP (ignored)
buf[8:9]   → GX
buf[10:11] → GY
buf[12:13] → GZ
```

### Combining bytes into int16_t

```c
data.ax = (int16_t)(buf[0] << 8 | buf[1]);
```

Shift the high byte left 8 bits, OR with the low byte.
Cast to int16_t to handle negative values correctly.

### I2C open-drain GPIO mode

Unlike UART (push-pull), I2C requires open-drain:

```c
GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;   /* open drain */
GPIO_InitStruct.Pull = GPIO_PULLUP;
```

I2C is a shared bus — devices can only pull the line LOW.
Pull-up resistors hold the line HIGH when no device is pulling.

---

## Bare metal vs HAL

| Bare metal | HAL |
|---|---|
| Manual I2C register config | `HAL_I2C_Init(&hi2c1)` |
| Read registers manually byte by byte | `HAL_I2C_Mem_Read(...)` |
| Manual address shifting | `(0x68 << 1)` convention |

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

## Expected output

```
MPU6050 HAL I2C Lab
MPU6050 found OK
AX:  1234 AY:  -456 AZ: 16384 | GX:   12 GY:   -8 GZ:    3
```

AZ ~16384 = 1g — sensor is reading gravity on the Z axis at rest.

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) <- this repo
- FreeRTOS — coming soon
