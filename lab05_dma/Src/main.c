#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* ── Handles ─────────────────────────────────────────── */
/* Three handles required:
 * - UART handle for USART2
 * - DMA handle for TX channel
 * - DMA handle for RX channel
 * UART uses DMA, so both must be linked via __HAL_LINKDMA */
UART_HandleTypeDef huart2;
DMA_HandleTypeDef  hdma_usart2_tx;
DMA_HandleTypeDef  hdma_usart2_rx;

/* ── Flags set from ISR callbacks ────────────────────── */
/* volatile is REQUIRED — these flags are modified by ISR
 * (outside main's control). Without volatile, the compiler
 * may optimize the while loop and never see the change. */
volatile uint8_t tx_complete = 0;
volatile uint8_t rx_complete = 0;

/* ── RX Buffer — DMA writes received bytes here ──────── */
#define RX_SIZE 32
uint8_t rx_buf[RX_SIZE];

/* ── Prototypes ──────────────────────────────────────── */
void SystemClock_Config(void);
void UART2_DMA_Init(void);

/* ────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_DMA_Init();

    /* ── TX DMA test ─────────────────────────────────── */
    char *msg = "STM32 HAL DMA UART\r\n";

    /* Always reset the flag BEFORE launching DMA transfer */
    tx_complete = 0;

    /* Returns immediately — DMA transmits in background.
     * HAL_OK confirms transfer was launched. */
    if (HAL_UART_Transmit_DMA(&huart2,
                              (uint8_t *)msg,
                              strlen(msg)) != HAL_OK)
    {
        while (1);   /* halt on error for debugging */
    }

    /* CPU is FREE while DMA transmits.
     * Counter shows how much work CPU did during transmission. */
    volatile uint32_t counter = 0;

    while (!tx_complete)
    {
        counter++;
    }

    /* ── Print counter value ─────────────────────────── */
    char buf[64];

    int len = snprintf(buf,
                       sizeof(buf),
                       "TX done. CPU counted: %lu\r\n",
                       counter);

    tx_complete = 0;

    HAL_UART_Transmit_DMA(&huart2,
                          (uint8_t *)buf,
                          len);

    while (!tx_complete);

    /* ── RX DMA test ─────────────────────────────────── */
    char *prompt = "Send 10 chars:\r\n";

    tx_complete = 0;

    HAL_UART_Transmit_DMA(&huart2,
                          (uint8_t *)prompt,
                          strlen(prompt));

    while (!tx_complete);

    /* Clear RX buffer to avoid garbage from previous data */
    memset(rx_buf, 0, RX_SIZE);

    rx_complete = 0;

    /* DMA waits for EXACTLY 10 bytes from UART.
     * Like cin.get(buf, 10) but non-blocking — DMA waits in hardware,
     * CPU is free until 10 bytes arrive. */
    if (HAL_UART_Receive_DMA(&huart2,
                             rx_buf,
                             10) != HAL_OK)
    {
        while (1);
    }

    while (!rx_complete);

    /* Add null terminator for safe string handling */
    rx_buf[10] = '\0';

    /* Echo received data back */
    len = snprintf(buf,
                   sizeof(buf),
                   "Received: %s\r\n",
                   rx_buf);

    tx_complete = 0;

    HAL_UART_Transmit_DMA(&huart2,
                          (uint8_t *)buf,
                          len);

    while (!tx_complete);

    while (1)
    {
        HAL_Delay(1000);
    }
}

/* ── UART DMA Callbacks ──────────────────────────────── */
/* Called automatically by HAL when DMA + UART complete
 * their transfer. The chain is:
 *
 * DMA finishes copying bytes
 *      -> DMA1_Stream6_IRQHandler
 *      -> HAL_DMA_IRQHandler
 *
 * UART finishes physical transmission (last bit out the pin)
 *      -> USART2_IRQHandler
 *      -> HAL_UART_IRQHandler
 *      -> HAL_UART_TxCpltCallback */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* The check is needed because this callback fires
     * for ANY UART instance. We only care about USART2. */
    if (huart->Instance == USART2)
    {
        tx_complete = 1;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        rx_complete = 1;
    }
}

/* ── Clock Config ────────────────────────────────────── */
/* System runs from HSI (16 MHz internal oscillator).
 * No PLL — keeps things simple for this lab. */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    RCC_OscInit.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInit.HSIState            = RCC_HSI_ON;
    RCC_OscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInit.PLL.PLLState        = RCC_PLL_NONE;

    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      =
        RCC_CLOCKTYPE_HCLK   |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1  |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInit,
                        FLASH_LATENCY_0);
}

/* ── UART + DMA Init ─────────────────────────────────── */

void UART2_DMA_Init(void)
{
    /* DMA1 lives on AHB1 bus — must enable its clock first */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* ── DMA TX : USART2_TX -> DMA1 Stream6 Ch4 ──────── */
    /* Stream6 + Channel4 is FIXED in hardware for USART2_TX.
     * See Reference Manual RM0383 Table 27. */

    hdma_usart2_tx.Instance                 = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_usart2_tx);

    /* Link DMA TX handle to UART handle.
     * Without this, HAL_UART_Transmit_DMA does not know which DMA to use.
     * Internally:
     *   huart2.hdmatx        = &hdma_usart2_tx
     *   hdma_usart2_tx.Parent = &huart2 */
    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);

    /* ── DMA RX : USART2_RX -> DMA1 Stream5 Ch4 ──────── */
    /* Stream5 + Channel4 is FIXED in hardware for USART2_RX. */

    hdma_usart2_rx.Instance                 = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode                = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_usart2_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_usart2_rx);

    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);

    /* ── DMA IRQs ────────────────────────────────────── */
    /* Required so the DMA can signal completion to the CPU.
     * Without these, the callbacks never fire. */

    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    /* ── USART2 IRQ — CRITICAL FOR TX COMPLETION ─────── */
    /* DMA completion alone is NOT enough for TX.
     * DMA copies bytes to the USART data register, but the
     * last byte still needs to clock out through the USART shift
     * register physically. USART2_IRQn signals that final
     * completion via the TC flag. */

    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    /* ── UART2 Config — same as previous labs ────────── */

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);
    /* Note: huart2.hdmatx and huart2.hdmarx are linked now —
     * HAL uses them automatically in Transmit_DMA / Receive_DMA */
}

/* ── MSP Init — Hardware-specific configuration ──────── */
/* Called automatically by HAL_UART_Init.
 * Separates protocol config from hardware config:
 *   UART2_DMA_Init   -> protocol (baud, bits, parity, DMA)
 *   HAL_UART_MspInit -> hardware (GPIO pins, clocks) */

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2)
    {
        /* Enable clocks for the peripherals we use */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA2 = TX  (to ST-Link Virtual COM Port)
         * PA3 = RX  (from ST-Link Virtual COM Port)
         * AF7 = USART2 alternate function */

        GPIO_InitStruct.Pin =
            GPIO_PIN_2 |
            GPIO_PIN_3;

        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;

        HAL_GPIO_Init(GPIOA,
                      &GPIO_InitStruct);
    }
}

/* ── IRQ Handlers ────────────────────────────────────── */
/* Function names MUST match the entries in the vector table
 * (defined in startup_stm32f411xe.s). The linker replaces
 * the weak default handlers with these strong definitions. */

/* Fires when DMA TX transfer completes */
void DMA1_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

/* Fires when DMA RX transfer completes */
void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

/* Fires when UART physical transmission completes (TC flag).
 * CRITICAL — without this, HAL_UART_TxCpltCallback never fires. */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/* ── SysTick — 1ms tick for HAL_Delay ────────────────── */
/* Called every 1ms by the SysTick timer.
 * HAL_IncTick           -> increments HAL's millisecond counter
 * HAL_SYSTICK_IRQHandler -> runs any registered tick callbacks */

void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}