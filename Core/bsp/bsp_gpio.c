#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "Types.h"
#include "bsp_gpio.h"
#include "stm32h7xx_hal.h"

const static GPIO_InitTypeDef g_gpioConfigComm[] = {
    {GLITCH_SHUTDOWN_PORT, GLITCH_SHUTDOWN_PIN, GPIO_MODE_INPUT,     GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},//?
    {PIC_LED_PORT, PIC_LED_PIN,                 GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {INTRPT_PORT, INTRPT_PIN,                   GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {BUSY_PORT, BUSY_PIN,                       GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {DIRECTION_PORT, DIRECTION_PIN,             GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {SPOT_PORT, SPOT_PIN,                       GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {MATCH_PORT, MATCH_PIN,                     GPIO_MODE_INPUT,     GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {LD_POS_PORT, LD_POS_PIN,                   GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {LD_SLOPE_PORT, LD_SLOPE_PIN,               GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},

    {GPIOA, GPIO_PIN_15,                        GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},

    {GPIOC, GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_10, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {GPIOB, 0x3fc,                              GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},// GPIO_PIN_9~ GPIO_PIN_2
};


static void HAL_GPIO_SetGroupPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint16_t val)
{
  /* get current Output Data Register value */
  uint32_t odr = GPIOx->ODR & (~GPIO_Pin);
  GPIOx->ODR = odr | val;
}
__attribute__((unused)) static void GPIO_SetDAC(uint32_t val)
{
    const GPIO_InitTypeDef *p_gpioCfg;
   
    p_gpioCfg = &g_gpioConfigComm[GPIO_DAC_A];
    HAL_GPIO_SetGroupPin(p_gpioCfg->PORT, p_gpioCfg->Pin, (val & BITS(11, 11)) << 4);

    p_gpioCfg = &g_gpioConfigComm[GPIO_DAC_C];
    HAL_GPIO_SetGroupPin(p_gpioCfg->PORT, p_gpioCfg->Pin, (val & BITS(8, 10)) >> 2);
    
    p_gpioCfg = &g_gpioConfigComm[GPIO_DAC_B];
    HAL_GPIO_SetGroupPin(p_gpioCfg->PORT, p_gpioCfg->Pin,(val & BITS(0, 7)) << 2);

    p_gpioCfg = &g_gpioConfigComm[GPIO_LD_POS];
    HAL_GPIO_WritePin(p_gpioCfg->PORT, p_gpioCfg->Pin, (GPIO_PinState)(p_gpioCfg->ActiveSignal == (GPIO_PinState)GPIO_PIN_SET));
    //GPIO_setPinStatus(GPIO_LD_POS, ENABLE);
}
static void GPIO_InitGPIOs(const GPIO_InitTypeDef *config, uint32_t size)
{
    const GPIO_InitTypeDef *p_gpioCfg;
    if (config == NULL) {
        return;
    }
    for (uint32_t i = 0; i < size; i++)
    {
        p_gpioCfg = &config[i];

        HAL_GPIO_Init(p_gpioCfg->PORT, (GPIO_InitTypeDef *)p_gpioCfg);
        HAL_GPIO_WritePin(p_gpioCfg->PORT, p_gpioCfg->Pin, p_gpioCfg->ActiveSignal);
    }
}
/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}


GPIO_PinState GPIO_getPinStatus(GPIO_NAMES alias)
{
    if (alias >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return GPIO_PIN_RESET;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[alias];
    return HAL_GPIO_ReadPin(p_gpioCfg->PORT, p_gpioCfg->Pin);
}
bool GPIO_isPinActive(GPIO_NAMES alias)
{
    if (alias >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return false;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[alias];
    GPIO_PinState staus = HAL_GPIO_ReadPin(p_gpioCfg->PORT, p_gpioCfg->Pin);
    if (staus == p_gpioCfg->ActiveSignal) {
        return true;
    }
    return false;
}
/// @brief ActiveSignal  isActive  res
//          1               1      1
//          1               0      0
//          0               0      1
//          0               1      0

/// @param alias 
/// @param isActive 
/// @return 
bool GPIO_setPinStatus(GPIO_NAMES alias, FunctionalState isActive)
{
    if (alias >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return false;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[alias];

    if ((p_gpioCfg->Mode < GPIO_MODE_OUTPUT_PP) || (p_gpioCfg->Mode > GPIO_MODE_AF_OD))
    {
        return false;
    }

    HAL_GPIO_WritePin(p_gpioCfg->PORT, p_gpioCfg->Pin, (GPIO_PinState)(p_gpioCfg->ActiveSignal == (GPIO_PinState)isActive));
    return true;
}

void GPIO_Init(void)
{
    MX_GPIO_Init();
    
    GPIO_InitGPIOs(&g_gpioConfigComm[0], ARRARY_SIZE(g_gpioConfigComm));
}


