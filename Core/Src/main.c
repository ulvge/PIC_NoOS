/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "bsp_uartcomm.h"
#include "debug_print.h"
#include "bsp_spi1_slave.h"
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "shell.h"
#include "api_utc.h"
#include "cm_backtrace.h"
#include "uart_monitor.h"
#include "spi_communication.h"

/* Private function prototypes -----------------------------------------------*/
BaseType_t xHigherPriorityTaskWoken_YES = pdTRUE;
BaseType_t xHigherPriorityTaskWoken_NO = pdFALSE;

void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_TIM5_Init(void);
void StartDefaultTask(void *argument);

TIM_HandleTypeDef g_htim5;
int g_debugLevel = DBG_INFO;
static __IO uint64_t g_utc_time_firmware_build = 0;
static const char *projectInfo =
    "\r\n"
    "********************************************\r\n"
    "************   BOARD   INFO      ************\r\n"
    "********************************************\r\n"
    "Build:    "__DATE__
    "  "__TIME__
    "\r\n"
    "Soft Version:  " SOFT_VERSION " \r\n"
    "Copyright: (c) HXZY\r\n"
    "********************************************\r\n"
    "\r\n";
    

/* Definitions for outputWave Task */
__attribute__((unused)) static osThreadId_t g_taskHandle_OutputWave;
static const osThreadAttr_t task_attributes_outputWave = {
    .name = "outputWave",
    .stack_size = 128 * 8,
    .priority = (osPriority_t)osPriorityRealtime,
};
/* Definitions for WriteBack Task */
__attribute__((unused)) static osThreadId_t g_taskHandle_writeBack;
static const osThreadAttr_t task_attributes_writeBack = {
    .name = "writeBack",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for uartMonitor Task */
__attribute__((unused)) static osThreadId_t g_taskHandle_uartMonitor;
static const osThreadAttr_t task_attributes_uartMonitor = {
    .name = "uartMonitor",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for shell Task */
__attribute__((unused)) static osThreadId_t g_taskHandle_Shell;
static const osThreadAttr_t task_attributes_Shell = {
    .name = "shell",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
__attribute__((unused)) static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

static void DebugConfig(void)
{
    __HAL_DBGMCU_FREEZE_WWDG1();
    __HAL_DBGMCU_FREEZE_TIM5();
    __HAL_DBGMCU_FREEZE_TIM5();
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    g_utc_time_firmware_build = currentSecsSinceEpoch(__DATE__, __TIME__);
    /* MPU Configuration--------------------------------------------------------*/
    MPU_Config();
    /* Enable the CPU Cache */
    //CPU_CACHE_Enable();

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init(); // set HAL_NVIC_SetPriorityGrouping
    // set AHB and APB buses clocks, 
    SystemClock_Config(); 

    /* Initialize all configured peripherals */
    UART_init();
    LOG_I("%s", projectInfo); 
    GPIO_Init();
    SPI1_Init();
    ADC_init();
    MX_TIM5_Init();
    DebugConfig();
    LOG_I("init other peripherals over\r\n");
    /* CmBacktrace initialize */
    cm_backtrace_init("CmBacktrace", HARDWARE_VERSION, SOFT_VERSION);

    /* creation of outputWave */
    g_taskHandle_OutputWave = osThreadNew(Task_outputWave, NULL, &task_attributes_outputWave);
    /* creation of WriteBack */
    g_taskHandle_writeBack = osThreadNew(Task_WriteBack, NULL, &task_attributes_writeBack);
    /* creation of shell */
    g_taskHandle_Shell = osThreadNew(shellTask, NULL, &task_attributes_Shell);
    /* creation of outputWave */
    g_taskHandle_uartMonitor = osThreadNew(Task_uartMonitor, NULL, &task_attributes_uartMonitor);
    LOG_I("create all task finished and succeed\r\n");

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */

    while (1) {
    }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /*AXI clock gating */
    RCC->CKGAENR = 0xFFFFFFFF;

    /** Supply configuration update enable
     */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = 64;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 30;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/* MPU Configuration */

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    /* Disables the MPU */
    HAL_MPU_Disable();

    /** Initializes and configures the Region and the memory to be protected
     */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM7 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM7) {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

inline uint32_t Get_dealyTimer_cnt(void)
{
    return g_htim5.Instance->CNT;
}
/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  g_htim5.Instance = TIM5;
  g_htim5.Init.Prescaler = 0;
  g_htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  g_htim5.Init.Period = 4294967295;
  g_htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  g_htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&g_htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&g_htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&g_htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_Base_Start(&g_htim5);
}
/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
inline void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
