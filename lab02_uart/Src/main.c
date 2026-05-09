#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* UART handle — contains all config and state */
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void UART2_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init();

    /* Send startup message */
    char *msg = "STM32 HAL UART OK\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    uint32_t counter = 0;

    while(1)
    {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "Tick: %lu\r\n", counter++);
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        HAL_Delay(1000);
    }
}

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

void UART2_Init(void)
{
    /* Configure the handle */
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

/* HAL_UART_Init calls this internally to configure the GPIO pins
 * We must implement it — HAL calls it as a callback */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2)
    {
        /* Enable clocks */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA2 = TX, PA3 = RX — Alternate Function AF7 */
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


/*
Tu código llama HAL_UART_Init(&huart2)
        │
        ▼
HAL configura los registros USART internamente
        │
        ▼
HAL llama HAL_UART_MspInit(&huart2)  ← callback
        │
        ▼
TÚ implementas aquí:
  - encender clock de USART2
  - encender clock de GPIOA
  - configurar PA2 y PA3 como AF7

#######################################################################

HAL_Init()
    → SysTick cada 1ms

SystemClock_Config()
    → HSI 16MHz

UART2_Init()
    → llena huart2
    → HAL_UART_Init(&huart2)
          → HAL configura USART2 registers
          → HAL llama HAL_UART_MspInit()
                → tú enciendes clocks
                → tú configuras PA2/PA3 como AF7

while(1)
    → snprintf formatea el string
    → HAL_UART_Transmit envía byte por byte
    → HAL_Delay espera 1 segundo
*/


/*
#############################################################

Algo nuevo — HAL_UART_MspInit
Este es un concepto clave de HAL que no existe en bare metal:
HAL_UART_Init()
      │
      └── llama internamente → HAL_UART_MspInit()
                                    │
                                    └── tú implementas aquí:
                                        - habilitar clocks
                                        - configurar pines GPIO
Msp = MCU Support Package. HAL separa la configuración del periférico de la configuración del hardware. Tú implementas MspInit y HAL lo llama automáticamente.

*/