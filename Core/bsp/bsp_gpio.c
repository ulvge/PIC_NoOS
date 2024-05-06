#include <string.h>   
#include <stdarg.h> 
#include "main.h"
#include "Types.h"
#include "bsp_gpio.h"
#include "shell.h"
#include "debug_print.h"

#define GPIO_GROUP_START  GPIO_DAC_A

#define STR(x) #x
#define EXPAND(x) STR(x)
#define PIN_NAME(val) .Name = EXPAND(val), .Pin = val##_PIN

// const static GPIO_InitTypeDef g_gpioConfigComm[] = {
//     {GLITCH_SHUTDOWN_PORT, PIN_NAME(GLITCH_SHUTDOWN), GPIO_MODE_INPUT,     GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},//?
// };
const static GPIO_InitTypeDef g_gpioConfigComm[] = {
    {GLITCH_SHUTDOWN_PORT,  PIN_NAME(GLITCH_SHUTDOWN), GPIO_MODE_INPUT,     GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},//?
    {PIC_LED_PORT,  		PIN_NAME(PIC_LED),         GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, NULL, GPIO_PIN_RESET},
    {INTRPT_PORT,  			PIN_NAME(INTRPT),          GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {BUSY_PORT,  			PIN_NAME(BUSY),            GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {DIRECTION_PORT,  		PIN_NAME(DIRECTION),       GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {SPOT_PORT,  			PIN_NAME(SPOT),            GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {MATCH_PORT,  			PIN_NAME(MATCH),           GPIO_MODE_INPUT,     GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {LD_POS_PORT,  			PIN_NAME(LD_POS),          GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {LD_SLOPE_PORT,  		PIN_NAME(LD_SLOPE),        GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_RESET},
    {MCLR_PORT,  			PIN_NAME(MCLR),            GPIO_MODE_IT_FALLING, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, NULL, GPIO_PIN_RESET},

    {GPIOA, "GroupA", GPIO_PIN_15,                     GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},

    {GPIOC, "GroupC", GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_10, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},
    {GPIOB, "GroupB", 0x3fc,                           GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, NULL, GPIO_PIN_SET},// GPIO_PIN_9~ GPIO_PIN_2
};

static const GPIO_InitTypeDef *p_gpioCfg0 = &g_gpioConfigComm[GPIO_GLITCH_SHUTDOWN];
static const GPIO_InitTypeDef *p_gpioCfg1 = &g_gpioConfigComm[GPIO_PIC_LED];
static const GPIO_InitTypeDef *p_gpioCfg2 = &g_gpioConfigComm[GPIO_INTRPT];
static const GPIO_InitTypeDef *p_gpioCfg3 = &g_gpioConfigComm[GPIO_BUSY];
static const GPIO_InitTypeDef *p_gpioCfg4 = &g_gpioConfigComm[GPIO_DIRECTION];
static const GPIO_InitTypeDef *p_gpioCfg5 = &g_gpioConfigComm[GPIO_SPOT];
static const GPIO_InitTypeDef *p_gpioCfg6 = &g_gpioConfigComm[GPIO_MATCH];
static const GPIO_InitTypeDef *p_gpioCfg7 = &g_gpioConfigComm[GPIO_LD_POS];
static const GPIO_InitTypeDef *p_gpioCfg8 = &g_gpioConfigComm[GPIO_LD_SLOPE];
static const GPIO_InitTypeDef *p_gpioCfg9 = &g_gpioConfigComm[GPIO_MCLR];

static void HAL_GPIO_SetGroupPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint16_t val)
{
    /* get current Output Data Register value */
    uint32_t odr = GPIOx->ODR & (~GPIO_Pin);
    GPIOx->ODR = odr | val;
}
inline void GPIO_SetDAC(uint32_t val)
{
    static const GPIO_InitTypeDef *p_gpioCfgA = &g_gpioConfigComm[GPIO_DAC_A];
    static const GPIO_InitTypeDef *p_gpioCfgC = &g_gpioConfigComm[GPIO_DAC_C];
    static const GPIO_InitTypeDef *p_gpioCfgB = &g_gpioConfigComm[GPIO_DAC_B];
   
    HAL_GPIO_SetGroupPin(p_gpioCfgA->PORT, p_gpioCfgA->Pin, (val & BITS(11, 11)) << 4);

    HAL_GPIO_SetGroupPin(p_gpioCfgC->PORT, p_gpioCfgC->Pin, (val & BITS(8, 10)) >> 2);
    
    HAL_GPIO_SetGroupPin(p_gpioCfgB->PORT, p_gpioCfgB->Pin,(val & BITS(0, 7)) << 2);
}
/// @brief A pause signal sent by the host
/// @param  
/// @return 
inline bool GPIO_Get_GLITCH_SHUTDOWN(void)
{
    return (bool)HAL_GPIO_ReadPin(p_gpioCfg0->PORT, p_gpioCfg0->Pin);
}

inline void GPIO_Set_PIC_LED(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg1->PORT, p_gpioCfg1->Pin, (GPIO_PinState)(p_gpioCfg1->ActiveSignal == st));
}
/// @brief send finished
/// @param st is need actived
inline void GPIO_Set_INTRPT(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg2->PORT, p_gpioCfg2->Pin, (GPIO_PinState)(p_gpioCfg2->ActiveSignal == st));
}
/// @brief is sending
/// @param st is need actived
inline void GPIO_Set_BUSY(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg3->PORT, p_gpioCfg3->Pin, (GPIO_PinState)(p_gpioCfg3->ActiveSignal == st));
}
inline void GPIO_Set_DIRECTION(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg4->PORT, p_gpioCfg4->Pin, (GPIO_PinState)(p_gpioCfg4->ActiveSignal == st));
}
inline void GPIO_Set_SPOT(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg5->PORT, p_gpioCfg5->Pin, (GPIO_PinState)(p_gpioCfg5->ActiveSignal == st));
}
/// @brief is host receved finished
/// @param  
/// @return 
inline bool GPIO_Get_MATCH(void)
{
    return (bool)HAL_GPIO_ReadPin(p_gpioCfg6->PORT, p_gpioCfg6->Pin);
}
inline void GPIO_Set_LD_POS(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg7->PORT, p_gpioCfg7->Pin, (GPIO_PinState)(p_gpioCfg7->ActiveSignal == st));
}
inline void GPIO_Set_LD_SLOPE(GPIO_PinState st)
{
    HAL_GPIO_WritePin(p_gpioCfg8->PORT, p_gpioCfg8->Pin, (GPIO_PinState)(p_gpioCfg8->ActiveSignal == st));
}
inline bool GPIO_Get_MCLR(void)
{
    return (bool)HAL_GPIO_ReadPin(p_gpioCfg9->PORT, p_gpioCfg9->Pin);
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
		uint32_t st = !p_gpioCfg->ActiveSignal;
        HAL_GPIO_WritePin(p_gpioCfg->PORT, p_gpioCfg->Pin, (GPIO_PinState)st);
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


GPIO_PinState GPIO_getPinStatus(GPIO_Idex idex)
{
    if (idex >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return GPIO_PIN_RESET;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[idex];
    return HAL_GPIO_ReadPin(p_gpioCfg->PORT, p_gpioCfg->Pin);
}
int GPIO_isPinActive(GPIO_Idex idex, GPIO_PinState *config)
{
    if (idex >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return -1;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[idex];
    *config = p_gpioCfg->ActiveSignal;
    if (idex <= GPIO_GROUP_START){
        GPIO_PinState staus = HAL_GPIO_ReadPin(p_gpioCfg->PORT, p_gpioCfg->Pin);
        if (staus == p_gpioCfg->ActiveSignal) {
            return 1;
        }
		return 0;
    }else{
        /* get current Output Data Register value */
        uint32_t odr = p_gpioCfg->PORT->ODR & (~p_gpioCfg->Pin);
        uint32_t res;
        if (idex == GPIO_DAC_C){
            res = GET_BITS(odr, 10, 12);
        }else{
            res = GET_BITS(odr, 2, 9);
        }
        return res;
    }
}

bool GPIO_getPinName(GPIO_Idex idex, const char **name)
{
    if (idex >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return false;
    }
    *name = g_gpioConfigComm[idex].Name;
    return true;
}
void GPIO_printIdexAndName(void)
{
    LOG_RAW("GPIO table: idx    |     name\r\n");
    for (uint32_t i = 0; i < ARRARY_SIZE(g_gpioConfigComm); i++)
    {
        LOG_RAW("%14d         %s\r\n", i, g_gpioConfigComm[i].Name);
    }
}
/// @brief ActiveSignal  isActive  res
//          1               1      1
//          1               0      0
//          0               0      1
//          0               1      0

/// @param alias 
/// @param isActive 
/// @return 
bool GPIO_setPinStatus(GPIO_Idex idex, FunctionalState isActive, GPIO_PinState *config)
{
    if (idex >= ARRARY_SIZE(g_gpioConfigComm))
    {
        return false;
    }
    const GPIO_InitTypeDef *p_gpioCfg = &g_gpioConfigComm[idex];

    if ((p_gpioCfg->Mode < GPIO_MODE_OUTPUT_PP) || (p_gpioCfg->Mode > GPIO_MODE_AF_OD))
    {
        return false;
    }

    if (config != NULL){
        *config = p_gpioCfg->ActiveSignal;
    }
    HAL_GPIO_WritePin(p_gpioCfg->PORT, p_gpioCfg->Pin, (GPIO_PinState)(p_gpioCfg->ActiveSignal == (GPIO_PinState)isActive));
    return true;
}


/**
  * @brief  Configures EXTI lines 15 to 10 (connected to PC.13 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTI15_10_IRQHandler_Config(void)
{
    /* Enable and set EXTI lines 15 to 10 Interrupt to the lowest priority */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, IRQHANDLER_PRIORITY_GPIO, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void GPIO_Init(void)
{
    MX_GPIO_Init();
    
    GPIO_InitGPIOs(&g_gpioConfigComm[0], ARRARY_SIZE(g_gpioConfigComm));
    EXTI15_10_IRQHandler_Config();
}

