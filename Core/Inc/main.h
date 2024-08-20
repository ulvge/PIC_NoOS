/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
		

#ifndef STM32H7A3xx
#define STM32H7A3xx
#endif
#define DEBUG_UART_PERIPH    USART2

#define    HARDWARE_VERSION     "1.0"
#define    SOFT_VERSION         "1.1"


// config IRQ HANDLER_PRIORITY
#define IRQHANDLER_PRIORITY_TICK        TICK_INT_PRIORITY
#define IRQHANDLER_PRIORITY_PENDSV      15

#define IRQHANDLER_PRIORITY_ADC         14
#define IRQHANDLER_PRIORITY_UART_DMA    14
#define IRQHANDLER_PRIORITY_UART        14

#define IRQHANDLER_PRIORITY_SPI_DMA     9
#define IRQHANDLER_PRIORITY_SPI         8

#define IRQHANDLER_PRIORITY_GPIO        5
// config IRQ HANDLER_PRIORITY end

extern TIM_HandleTypeDef g_htim5;
extern int g_debugLevel;
extern void Task_outputWave(void);
extern inline uint32_t Get_dealyTimer_cnt(void);
/* Exported functions prototypes ---------------------------------------------*/
extern void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
