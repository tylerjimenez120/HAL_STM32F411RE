// include the entire STM32 HAL library
// gives access to functions, structures, macros
#include "stm32f4xx_hal.h"

/* Nucleo green LED = PA5 */
#define LED_PIN   GPIO_PIN_5
#define LED_PORT  GPIOA

void SystemClock_Config(void);
void GPIO_Init(void);

int main(void)
{
    /* 1. Initialize HAL — configures SysTick for HAL_Delay() */
    HAL_Init();

    /* 2. Configure system clock */
    SystemClock_Config();

    /* 3. Initialize GPIO */
    GPIO_Init();

    /* 4. Blink loop */
    while(1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(500);
    }
}

void SystemClock_Config(void)
{
    /* We use HSI at 16MHz — default clock, no PLL
     * In later labs we will configure PLL to reach 100MHz */
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    /* Enable HSI 
    Structure where you define the clock source */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInit.HSIState       = RCC_HSI_ON;
    RCC_OscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInit);

    /* Configure buses 
    Structure to define how the clock is distributed */
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
/* I use the internal 16 MHz clock as the main source and distribute it without division across the system.
    Source = HSI (16 MHz)
    SYSCLK = 16 MHz
    All buses = 16 MHz

    PLL → multiplies frequency -> used when you need speed, precision or high performance
    SYSCLK → main system clock
    HCLK → CPU clock
    PCLK1/2 → peripherals
    Dividers → reduce frequency
*/


void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure PA5 as output */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  /* push-pull output */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}
//Enables GPIOA and configures PA5 as a simple digital output to control an LED

/* HAL requires this function — called from startup */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
/*
SysTick reaches 0
An interrupt is triggered
The CPU executes this function automatically
*/