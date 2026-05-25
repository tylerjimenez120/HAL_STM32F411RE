#include "stm32f4xx_hal.h"

/* Handle for TIM2 */
TIM_HandleTypeDef htim2;

void SystemClock_Config(void);  // configure system clock
void TIM2_PWM_Init(void);       // configure peripheral

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    TIM2_PWM_Init();

    /* Start PWM on TIM2 channel 1 */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    uint32_t duty = 0;
    int8_t   direction = 1;  /* 1 = brightening, -1 = dimming */

    while(1)
    {
        /* Update CCR1 register — this changes the duty cycle */
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);

        duty += direction;

        /* Reverse direction at the limits */
        if (duty >= 1000) direction = -1;
        if (duty == 0)    direction = 1;

        HAL_Delay(1);  /* 2ms * 1000 steps = 2s per fade cycle */
    }
}

/* ── Inits ─────────────────────────────────────────────
 * Configure the system clock.
 * Use the internal 16 MHz oscillator and feed it to all buses
 * without dividing.
 */

void SystemClock_Config(void)
{
    // Which oscillator to use
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    RCC_OscInit.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInit.HSIState            = RCC_HSI_ON;
    RCC_OscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInit.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInit);


    // How to distribute that clock
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

void TIM2_PWM_Init(void)
{
    /* ── Timer base configuration ──
     * Clock = 16 MHz HSI
     * PSC = 15  → 16 MHz / 16 = 1 MHz (timer clock)
     * ARR = 999 → counts 0-999 = 1000 ticks
     * PWM frequency = 1 MHz / 1000 = 1 kHz
     *
     * TIM2 counts at 1 MHz, reaches 999, then restarts
     * → produces cycles of 1 ms (frequency 1 kHz)
     */
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 15;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    /* ── PWM channel configuration ──
     * Channel 1 in PWM1 mode, starts with duty 0%
     * → PA5 starts off, ready for the duty to change
     */
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;                  /* initial duty = 0% */
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM2)
    {
        // Enable clocks
        __HAL_RCC_TIM2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA5 = TIM2_CH1 — AF1
         * PA5 is the "output cable" — this is where the PWM signal
         * leaves the chip to the outside world.
         */
        GPIO_InitStruct.Pin       = GPIO_PIN_5;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/*
 * Every 1ms the SysTick fires → runs this function → +1 to HAL's
 * internal tick counter.
 *
 * This is what allows HAL_Delay(2) to know when 2ms have passed.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}