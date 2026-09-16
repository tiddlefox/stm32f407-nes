/**
 * stm32f4xx_hal_conf.h — HAL configuration for STM32F407VET6 NES Emulator
 */
#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#define HSE_VALUE              8000000UL
#define HSE_STARTUP_TIMEOUT    100
#define LSE_STARTUP_TIMEOUT    5000
#define HSI_VALUE              16000000UL
#define LSE_VALUE              32768UL
#define LSI_VALUE              32000UL
#define VDD_VALUE              3300UL

/* Enable HAL modules we use */
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_SD_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED

/* System config */
#define TICK_INT_PRIORITY  0x0F
#define USE_HAL_DRIVER

/* Include each enabled module header (defines types like HAL_StatusTypeDef) */
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_spi.h"
#include "stm32f4xx_hal_sd.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_pwr.h"

/* HAL assert (disabled for size) */
#define assert_param(expr) ((void)0U)

/* Note: stm32f4xx_hal.h is included separately by stm32f4xx.h */

#endif /* STM32F4XX_HAL_CONF_H */
