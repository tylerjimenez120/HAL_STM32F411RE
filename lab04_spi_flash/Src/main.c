#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* W25Q32 commands */
#define W25Q_MANUFACTURER_ID   0x90
#define W25Q_READ_DATA         0x03
#define W25Q_PAGE_PROGRAM      0x02
#define W25Q_SECTOR_ERASE      0x20
#define W25Q_WRITE_ENABLE      0x06
#define W25Q_READ_STATUS_REG   0x05

/* CS pin — PB6 */
#define CS_PORT   GPIOB
#define CS_PIN    GPIO_PIN_6

#define CS_LOW()  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET)

/* Handles */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void SPI1_Init(void);
void UART2_Init(void);
void uart_send(const char *msg);

/* W25Q32 functions */
void     W25Q_ReadID(uint8_t *manufacturer, uint8_t *device_id);
void     W25Q_WriteEnable(void);
void     W25Q_WaitBusy(void);
void     W25Q_SectorErase(uint32_t address);
void     W25Q_PageProgram(uint32_t address, uint8_t *data, uint16_t len);
void     W25Q_ReadData(uint32_t address, uint8_t *buf, uint16_t len);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init();
    SPI1_Init();

    HAL_Delay(100); 

    CS_HIGH();  /* deselect chip at startup */

    uart_send("W25Q32 SPI Flash Lab\r\n");

    /* ── Read Manufacturer ID ── */
    uint8_t mfr = 0, dev_id = 0;
    W25Q_ReadID(&mfr, &dev_id);

    char buf[64];
    int len = snprintf(buf, sizeof(buf),
        "Manufacturer: 0x%02X  Device ID: 0x%02X\r\n", mfr, dev_id);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);

    /* Winbond manufacturer = 0xEF, W25Q32 device = 0x15 */
    if (mfr == 0xEF)
        uart_send("W25Q32 found OK\r\n");
    else
        uart_send("W25Q32 NOT found — check wiring\r\n");

    /* ── Write test ── */
    uint32_t test_addr = 0x000000;  /* sector 0 */
    uint8_t  write_buf[] = "STM32 HAL SPI OK";
    uint8_t  read_buf[16] = {0};

    uart_send("Erasing sector 0...\r\n");
    W25Q_WriteEnable();
    W25Q_SectorErase(test_addr);
    W25Q_WaitBusy();

    uart_send("Writing data...\r\n");
    W25Q_WriteEnable();
    W25Q_PageProgram(test_addr, write_buf, sizeof(write_buf) - 1);
    W25Q_WaitBusy();

    uart_send("Reading back...\r\n");
    W25Q_ReadData(test_addr, read_buf, sizeof(read_buf));

    len = snprintf(buf, sizeof(buf), "Read: %s\r\n", read_buf);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);

    if (memcmp(write_buf, read_buf, sizeof(read_buf)) == 0)
        uart_send("Verify OK\r\n");
    else
        uart_send("Verify FAILED\r\n");

    while(1)
    {
        HAL_Delay(1000);
    }
}

/* ── W25Q32 driver ───────────────────────────────────── */

void W25Q_ReadID(uint8_t *manufacturer, uint8_t *device_id)
{
    uint8_t cmd[4] = {W25Q_MANUFACTURER_ID, 0x00, 0x00, 0x00};
    uint8_t resp[2] = {0};

    CS_LOW();
    HAL_Delay(1);
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, resp, 2, HAL_MAX_DELAY);
    CS_HIGH();

    *manufacturer = resp[0];
    *device_id    = resp[1];
}

void W25Q_WriteEnable(void)
{
    uint8_t cmd = W25Q_WRITE_ENABLE;
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    CS_HIGH();
}

void W25Q_WaitBusy(void)
{
    uint8_t cmd = W25Q_READ_STATUS_REG;
    uint8_t status = 0;

    do {
        CS_LOW();
        HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(&hspi1, &status, 1, HAL_MAX_DELAY);
        CS_HIGH();
    } while (status & 0x01);  /* bit 0 = BUSY */
}

void W25Q_SectorErase(uint32_t address)
{
    uint8_t cmd[4] = {
        W25Q_SECTOR_ERASE,
        (address >> 16) & 0xFF,
        (address >> 8)  & 0xFF,
        (address)       & 0xFF
    };
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    CS_HIGH();
}

void W25Q_PageProgram(uint32_t address, uint8_t *data, uint16_t len)
{
    uint8_t cmd[4] = {
        W25Q_PAGE_PROGRAM,
        (address >> 16) & 0xFF,
        (address >> 8)  & 0xFF,
        (address)       & 0xFF
    };
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);
    CS_HIGH();
}

void W25Q_ReadData(uint32_t address, uint8_t *buf, uint16_t len)
{
    uint8_t cmd[4] = {
        W25Q_READ_DATA,
        (address >> 16) & 0xFF,
        (address >> 8)  & 0xFF,
        (address)       & 0xFF
    };
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);
    CS_HIGH();
}

/* ── UART ────────────────────────────────────────────── */
void uart_send(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ── Inits ───────────────────────────────────────────── */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    RCC_OscInit.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInit.HSIState            = RCC_HSI_ON;
    RCC_OscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInit.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK  |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1  |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_0);
}

void SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hspi->Instance == SPI1)
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PA6=MISO, PA7=MOSI — AF5 = SPI1 */
        GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PB3=SCK — AF5 = SPI1 */
        GPIO_InitStruct.Pin       = GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;  /* ← era OUTPUT_PP */
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;    /* ← faltaba esto */
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* PB6 = CS — GPIO output normal */
        GPIO_InitStruct.Pin       = GPIO_PIN_6;
        GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = 0;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void UART2_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}