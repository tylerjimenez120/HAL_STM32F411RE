#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

RTC_HandleTypeDef  hrtc;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void RTC_Clock_Config(void);
void RTC_Init(void);
void UART2_Init(void);
void LED_Init(void);
void uart_send(const char *msg);

int main(void)
{
    HAL_Init();

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    // 🔴 IMPORTANTE: evitar interferencia del debugger
    HAL_DBGMCU_DisableDBGStopMode();

    SystemClock_Config();
    UART2_Init();
    LED_Init();

    RTC_Clock_Config();
    RTC_Init();

    uart_send("STM32F411RE Low Power RTC Test\r\n");
    uart_send("Wake every ~5 seconds\r\n\r\n");

    uint32_t wake_count = 0;

    while(1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

        wake_count++;
        char buf[64];
        int len = snprintf(buf, sizeof(buf),
        "Wake #%lu -> entering STOP\r\n", wake_count);
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);

        HAL_Delay(10);

    // RTC sequence
        HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
        __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();

        HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 5, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

        // 🔴 CRÍTICO: detener SysTick
        HAL_SuspendTick();

        // STOP
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    // 🔴 reactivar SysTick
        HAL_ResumeTick();

        SystemClock_Config();

        uart_send("Woke up!\r\n");
    }
}

/* ============================= */
/* RTC CALLBACK */
/* ============================= */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
}

/* ============================= */
/* UART */
/* ============================= */
void uart_send(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ============================= */
/* SYSTEM CLOCK */
/* ============================= */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    RCC_OscInit.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInit.HSIState            = RCC_HSI_ON;
    RCC_OscInit.LSIState            = RCC_LSI_ON;
    RCC_OscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInit.PLL.PLLState        = RCC_PLL_NONE;

    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_0);
}

/* ============================= */
/* RTC CLOCK CONFIG */
/* ============================= */
void RTC_Clock_Config(void)
{
    RCC_PeriphCLKInitTypeDef periphCLK = {0};

    // 🔴 CRÍTICO: activar y esperar LSI
    __HAL_RCC_LSI_ENABLE();
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);

    HAL_Delay(100); // estabilización

    periphCLK.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periphCLK.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;

    HAL_RCCEx_PeriphCLKConfig(&periphCLK);
}

/* ============================= */
/* RTC INIT */
/* ============================= */
void RTC_Init(void)
{
    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;
    hrtc.Init.SynchPrediv    = 249;
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;

    HAL_RTC_Init(&hrtc);
}

/* ============================= */
/* RTC MSP */
/* ============================= */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        __HAL_RCC_RTC_ENABLE();

        HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    }
}

/* ============================= */
/* LED */
/* ============================= */
void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_5;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &g);
}

/* ============================= */
/* UART */
/* ============================= */
void UART2_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;

    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};

    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_NOPULL;
        g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2;

        HAL_GPIO_Init(GPIOA, &g);
    }
}

/* ============================= */
/* IRQ */
/* ============================= */
void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}


/*
1. MCU despierta (boot inicial o post-STOP)
2. Toggle LED
3. Imprime "Wake #N"
4. Configura wakeup timer = 5 segundos
5. Delay 10ms (que UART termine)
6. EnterSTOPMode → MCU duerme
   │
   │  ... 5 segundos pasan ...
   │  CPU detenido, ~10 µA de consumo
   │
7. RTC dispara wakeup
8. IRQ ejecuta → callback desactiva timer
9. EnterSTOPMode retorna
10. SystemClock_Config (reconfigura clocks)
11. Loop continúa → paso 2


--------------------------------------------
RUN:     CPU corriendo, 6-12 mA
STOP:    CPU detenido, RAM conservada, ~10 µA

Durante STOP:
├── CPU detenido ✓
├── RAM conservada ✓ (variables intactas)
├── HSI/PLL detenidos ✓
├── Periféricos detenidos ✓
└── RTC + LSI corriendo (en backup domain)
*/