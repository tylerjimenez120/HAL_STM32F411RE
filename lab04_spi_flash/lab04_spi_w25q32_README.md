# Lab 04 — HAL SPI + W25Q32 Flash Memory

Read and write data to an external SPI Flash memory (W25Q32) using STM32 HAL.

---

## Hardware

| Pin Nucleo | Label | STM32 Pin | W25Q32 |
|------------|-------|-----------|--------|
| CN9        | D3    | PB3       | CLK    |
| CN9        | D11   | PA7       | D1 (MOSI) |
| CN9        | D12   | PA6       | D0 (MISO) |
| CN9        | D10   | PB6       | CS     |
| 3.3V       | 3.3V  | —         | VCC    |
| GND        | GND   | —         | GND    |

> **Note:** D0/D1 labels on the W25Q32 module may be swapped depending on the manufacturer.
> If the chip is not found (0x00), swap D0 and D1 cables.

---

## What this lab covers

- SPI_HandleTypeDef — the HAL handle for SPI
- HAL_SPI_MspInit — hardware callback (GPIO AF5 + CS as plain GPIO)
- HAL_SPI_Transmit — send bytes to device
- HAL_SPI_Receive — receive bytes from device
- CS manual control — CS_LOW() / CS_HIGH() macros
- W25Q32 driver — ReadID, WriteEnable, SectorErase, PageProgram, ReadData
- Address decomposition — splitting 24-bit address into 3 bytes

---

## Key concepts

### SPI vs I2C selection mechanism

```
I2C → address in message — all devices share 2 wires
SPI → CS pin per device  — all devices share 3 wires (SCK/MOSI/MISO)
                           1 extra CS pin per device
```

### CS is a plain GPIO — not Alternate Function
```c
/* SCK, MOSI, MISO → AF5 (controlled by SPI peripheral) */
GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;

/* CS → plain GPIO output (controlled by you) */
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
```

### Every SPI transaction pattern
```
CS_LOW()                    ← activate chip
HAL_SPI_Transmit(command)   ← send command
HAL_SPI_Receive(data)       ← receive response (if needed)
CS_HIGH()                   ← deactivate chip
```

### Address decomposition — 24-bit address in 3 bytes
The W25Q32 uses 24-bit addresses. SPI sends 1 byte at a time:
```c
uint32_t address = 0x001234;

(address >> 16) & 0xFF = 0x00  ← high byte
(address >>  8) & 0xFF = 0x12  ← mid byte
(address)       & 0xFF = 0x34  ← low byte
```

### WriteEnable — required before every write/erase
```c
W25Q_WriteEnable();   // send 0x06
W25Q_SectorErase();   // send 0x20 + address
W25Q_WaitBusy();      // poll status register until BUSY=0

W25Q_WriteEnable();   // must enable again before each write
W25Q_PageProgram();   // send 0x02 + address + data
W25Q_WaitBusy();
```
WriteEnable resets automatically after each write/erase operation.
It exists to prevent accidental writes from bus noise.

### WaitBusy — polling the status register
Erase (~400ms) and write (~3ms) take real time.
Poll bit 0 of the status register until it clears:
```c
do {
    // read status register (0x05)
    // chip responds with 1 byte
} while (status & 0x01);  // bit 0 = BUSY
```

### SPI loopback test
To verify SPI works before connecting a device:
connect MOSI (D11) to MISO (D12) with a jumper:
```c
HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, HAL_MAX_DELAY);
// if RX == TX → SPI is working
```

---

## W25Q32 commands used

| Command | Code | Description |
|---------|------|-------------|
| MANUFACTURER_ID | 0x90 | Read manufacturer + device ID |
| READ_DATA | 0x03 | Read data from address |
| PAGE_PROGRAM | 0x02 | Write up to 256 bytes |
| SECTOR_ERASE | 0x20 | Erase 4KB sector |
| WRITE_ENABLE | 0x06 | Enable write/erase |
| READ_STATUS_REG | 0x05 | Read status (BUSY bit) |

---

## Bare metal vs HAL

| Bare metal | HAL |
|---|---|
| Manual SPI register config | `HAL_SPI_Init(&hspi1)` |
| Manual byte TX/RX loop | `HAL_SPI_Transmit / Receive` |
| Manual GPIO for CS | `CS_LOW() / CS_HIGH()` macros |

---

## Build

```bash
make
make flash
minicom -D /dev/ttyACM0 -b 115200
```

## Expected output

```
Loopback TX:0xAB RX:0x00
W25Q32 SPI Flash Lab
Manufacturer: 0xEF  Device ID: 0x15
W25Q32 found OK
Erasing sector 0...
Writing data...
Reading back...
Read: STM32 HAL SPI OK
Verify OK
```

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Manufacturer: 0x00 | No communication | Check wiring |
| Manufacturer: 0xFF | D0/D1 swapped | Swap MOSI/MISO cables |
| Loopback RX:0x00 | SPI not working | Check AF5 pin config |
| Verify FAILED | Old data in sector | Sector erase before write |

---

## Full series

- [Bare Metal](https://github.com/tylerjimenez120/Bare_metal_stm32f411re)
- [HAL STM32F411RE](https://github.com/tylerjimenez120/HAL_STM32F411RE) ← this repo
- FreeRTOS — coming soon
