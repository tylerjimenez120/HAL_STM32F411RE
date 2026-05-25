#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handles */
ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;
UART_HandleTypeDef huart2;

/* ADC buffer — DMA writes here continuously */
#define ADC_SAMPLES 1
volatile uint16_t adc_buffer[ADC_SAMPLES];

void SystemClock_Config(void);
void ADC1_DMA_Init(void);
void UART2_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init();
    ADC1_DMA_Init();

    /* Start ADC in continuous mode with DMA
     * The ADC will sample forever and DMA fills adc_buffer automatically
     * HAL_ADC_Start_DMA tells the hardware:
     *   1. ADC, start sampling continuously
     *   2. DMA, copy each result to adc_buffer
     *   3. When you reach the end of the buffer, restart (circular mode)
     */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, ADC_SAMPLES); // from here the hardware runs on its own — CPU is free

    char buf[64]; // UART transmit buffer

    while(1)
    {
        /* Read the latest value — always fresh, no waiting */
        uint16_t value = adc_buffer[0];

        /* Convert to millivolts: 4095 = 3.3V = 3300mV */
        uint32_t mv = (value * 3300) / 4095;

        int len = snprintf(buf, sizeof(buf),
            "ADC: %4u (%4lu mV)\r\n", value, mv);
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);

        HAL_Delay(100);
    }
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

void ADC1_DMA_Init(void)
{
    // ADC1 uses DMA2, not DMA1
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* ── DMA for ADC1 — DMA2 Stream0 Channel0 ── */
    hdma_adc1.Instance                 = DMA2_Stream0;
    hdma_adc1.Init.Channel             = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;  /* 16 bits */
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR; // Without this, DMA would stop after the first sample. Circular mode is required for continuous ADC.
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH; // High priority — ADC is timing-sensitive. If DMA is delayed, samples are lost.
    hdma_adc1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1); // Link DMA to ADC — same as with UART. Without this the ADC doesn't know which DMA to use.

    // Enable DMA interrupt
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    // This enables the interrupt. When the DMA completes one pass through the buffer it fires the IRQ.
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* ── ADC1 configuration ── */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B; // 12 bits → 0 to 4095
    hadc1.Init.ScanConvMode          = DISABLE; // Scan mode = sample multiple channels in sequence. Since we only have 1 channel (PA1), disable it.
    hadc1.Init.ContinuousConvMode    = ENABLE;  // Continuous mode — ADC samples non-stop
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;       // DMA loops too — makes the ADC keep requesting DMA transfers indefinitely.
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    /* Configure channel 1 (PA1) */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1)
    {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA1 = ADC1_IN1 — analog input mode */
        GPIO_InitStruct.Pin  = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; // analog input
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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

/* ── IRQ Handlers ─────────────────────────────────────
 * Function executed when the interrupt fires
 */
void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}


/*
HAL_ADC_Start_DMA
        │
        ▼
ADC starts → samples PA1 → stores in DR
        │
        ▼
DMA copies DR → adc_buffer[0]
        │
        ▼
ADC samples again (continuous mode)
        │
        ▼
DMA copies again (circular mode)
        │
... infinite hardware loop ...

Meanwhile in main:
        │
        ▼
every 100ms → read adc_buffer[0] → print
*/