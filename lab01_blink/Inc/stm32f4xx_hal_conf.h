#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H


/* Disable parameter checking — reduces code size, required for bare builds
Tells all HAL libraries included later (like stm32f4xx_hal_uart.h) not to include error-checking code. */
#define USE_FULL_ASSERT    0
/*
Each HAL function internally has an assert_param that checks if the provided data is valid. 
By defining it as ((void)0U), the compiler removes all those checks.
*/
#define assert_param(expr) ((void)0U)


/* Enabled HAL modules — only the ones we need 
You only enable what you will use so the code stays lightweight and efficient.
*/
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED

/* External oscillator frequency — Nucleo does not have HSE
 * we use HSI = 16MHz 
These constants tell the library the real clock frequencies so it can correctly compute system timings.
Internal and external clocks of the board 
HSI = 16 MHz → internal oscillator of the MCU
HSE = 8 MHz → external oscillator (board crystal)
 */
#define HSE_VALUE    8000000U
#define HSI_VALUE    16000000U

/* External clock value — not used on Nucleo (no HSE crystal)
 * but HAL requires it to be defined */
#define EXTERNAL_CLOCK_VALUE    12288000U

/* Default timeout values */
#define HSE_STARTUP_TIMEOUT    100U
#define LSE_STARTUP_TIMEOUT    5000U

/* SysTick — HAL uses it internally for HAL_Delay() */
#define  TICK_INT_PRIORITY     0x0FU

/* Include headers for each enabled module */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
  #include "stm32f4xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f4xx_hal_uart.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
#endif

#endif /* __STM32F4xx_HAL_CONF_H */