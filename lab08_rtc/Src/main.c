#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handles */
RTC_HandleTypeDef  hrtc;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void RTC_Init(void);
void UART2_Init(void);
void uart_send(const char *msg);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init();
    RTC_Init();

    uart_send("STM32 HAL RTC Lab\r\n");

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    char buf[64];

    while(1)
    {
        /* Read time (must read time BEFORE date — HAL requirement) */
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        int len = snprintf(buf, sizeof(buf),
            "20%02d-%02d-%02d %02d:%02d:%02d\r\n",
            sDate.Year, sDate.Month, sDate.Date,
            sTime.Hours, sTime.Minutes, sTime.Seconds);

        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        HAL_Delay(1000);
    }
}

/* ── UART helper ─────────────────────────────────────── */
void uart_send(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ── Inits ───────────────────────────────────────────── 
SystemClock_Config → conecta el oscilador LSI al RTC*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    /* Enable LSI for RTC 
    Diferencia clave con labs anteriores. Activamos dos osciladores:
        HSI → 16 MHz para el CPU y periféricos normales
        LSI → 32 kHz para el RTC
    */
    RCC_OscInit.OscillatorType      = RCC_OSCILLATORTYPE_HSI |
                                       RCC_OSCILLATORTYPE_LSI;
    RCC_OscInit.HSIState            = RCC_HSI_ON;
    RCC_OscInit.LSIState            = RCC_LSI_ON;        /* enable LSI for RTC */
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

    /* Use LSI as RTC clock source */
    RCC_PeriphCLKInitTypeDef periphCLK = {0};
    periphCLK.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periphCLK.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;
    HAL_RCCEx_PeriphCLKConfig(&periphCLK);
}

void RTC_Init(void)
{
    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;//La otra opción es 12 con AM/PM.
    hrtc.Init.AsynchPrediv   = 127;   /* LSI 32kHz / 128 = 250 Hz */
    hrtc.Init.SynchPrediv    = 249;   /* 250 Hz / 250 = 1 Hz */
    //El RTC puede sacar señales por pines especiales (alarmas, segundos, etc). Aquí lo deshabilitamos — solo queremos leer la hora internamente.
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    HAL_RTC_Init(&hrtc);

    /* Set initial date — 2026-05-25 */
    RTC_DateTypeDef sDate = {0};
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month   = RTC_MONTH_MAY;
    sDate.Date    = 25;
    sDate.Year    = 26;
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    /* Set initial time — 14:30:00 */
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours          = 14;
    sTime.Minutes        = 30;
    sTime.Seconds        = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        /* Enable RTC clock (lives in backup domain) 
        enciende el bus de energía del RTC*/
        __HAL_RCC_RTC_ENABLE();
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