#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* MPU6050 I2C address (AD0 = GND) */
#define MPU6050_ADDR     (0x68 << 1)  /* HAL uses 8-bit address */

/* MPU6050 registers */
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_WHO_AM_I     0x75     /* should return 0x68 */

/* Handles */
I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void I2C1_Init(void);
void UART2_Init(void);
void MPU6050_Init(void);
void uart_send(const char *msg);

/* Raw sensor data */
typedef struct {
    int16_t ax, ay, az;  /* accelerometer */
    int16_t gx, gy, gz;  /* gyroscope     */
} MPU6050_Data;

MPU6050_Data mpu_read(void);


int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init();
    I2C1_Init();

    uart_send("MPU6050 HAL I2C Lab\r\n");

    /* Verify sensor is present */
    uint8_t who_am_i = 0;
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR,
                     MPU6050_WHO_AM_I, 1,
                     &who_am_i, 1, HAL_MAX_DELAY);

    if (who_am_i == 0x68)
        uart_send("MPU6050 found OK\r\n");
    else
        uart_send("MPU6050 NOT found — check wiring\r\n");

    MPU6050_Init();

    while(1)
    {
        MPU6050_Data data = mpu_read();

        char buf[96];
        int len = snprintf(buf, sizeof(buf),
            "AX:%6d AY:%6d AZ:%6d | GX:%6d GY:%6d GZ:%6d\r\n",
            data.ax, data.ay, data.az,
            data.gx, data.gy, data.gz);

        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        HAL_Delay(200);
    }
}

/* ── UART helpers ────────────────────────────────────── */
void uart_send(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ── MPU6050 ─────────────────────────────────────────── */
void MPU6050_Init(void)
{
    /* Wake up MPU6050 — clear sleep bit in PWR_MGMT_1 */
    uint8_t data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR,
                      MPU6050_PWR_MGMT_1, 1,
                      &data, 1, HAL_MAX_DELAY);
    HAL_Delay(100);
}

MPU6050_Data mpu_read(void)
{
    uint8_t buf[14];
    MPU6050_Data data = {0};

    /* Read 14 bytes starting from ACCEL_XOUT_H
     * Order: AX_H AX_L AY_H AY_L AZ_H AZ_L
     *        TEMP_H TEMP_L
     *        GX_H GX_L GY_H GY_L GZ_H GZ_L */
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR,
                     MPU6050_ACCEL_XOUT_H, 1,
                     buf, 14, HAL_MAX_DELAY);

    data.ax = (int16_t)(buf[0]  << 8 | buf[1]);
    data.ay = (int16_t)(buf[2]  << 8 | buf[3]);
    data.az = (int16_t)(buf[4]  << 8 | buf[5]);
    /* buf[6] buf[7] = temperature — skip */
    data.gx = (int16_t)(buf[8]  << 8 | buf[9]);
    data.gy = (int16_t)(buf[10] << 8 | buf[11]);
    data.gz = (int16_t)(buf[12] << 8 | buf[13]);

    return data;
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

void I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 100000;   /* 100KHz standard mode */
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB8 = SCL, PB9 = SDA — AF4 = I2C1 */
        GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;   /* open drain */
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
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