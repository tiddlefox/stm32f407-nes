/**
 * main.h - Top-level header for NES-STM32 project
 */

#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*  External handles (initialized by CubeMX-generated code)          */
/*-------------------------------------------------------------------*/
extern SPI_HandleTypeDef  hspi1;
extern SD_HandleTypeDef   hsd;     /* SDIO */
extern UART_HandleTypeDef huart1;

/*-------------------------------------------------------------------*/
/*  Function prototypes                                              */
/*-------------------------------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);

/* Called from SysTick_Handler each ms */
extern void NES_TickInc(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
