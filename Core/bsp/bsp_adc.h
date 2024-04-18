
#ifndef __BSP_ADC_H
#define	__BSP_ADC_H

#include "stdint.h"
#include "main.h" 
#include "Types.h"
#include "bsp_gpio.h"

/* Definition for ADCx clock resources */
#define ADCx                            ADC1
#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC12_CLK_ENABLE()
#define ADCx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DMAx_CHANNELx_CLK_ENABLE()      __HAL_RCC_DMA1_CLK_ENABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC12_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC12_RELEASE_RESET()

/* Definition for ADCx Channel Pin */
#define ADCx_CHANNEL_PIN_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define ADCx_CHANNEL_PIN                XBEAM_PIN
#define ADCx_CHANNEL_GPIO_PORT          XBEAM_PORT

/* Definition for ADCx's Channel */
#define ADCx_CHANNEL                    ADC_CHANNEL_11

extern void ADC_init(void);
extern float ADC_get_value(void);
#endif /* __BSP_ADC_H */




