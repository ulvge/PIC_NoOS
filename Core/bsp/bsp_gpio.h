#ifndef __BSP_GPIO_H
#define	__BSP_GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>  
#include "stm32h7xx_hal.h"


#define BUSY_PORT                   GPIOA
#define BUSY_PIN                    GPIO_PIN_1

#define GLITCH_SHUTDOWN_PORT        GPIOA
#define GLITCH_SHUTDOWN_PIN         GPIO_PIN_10

#define SPOT_PORT                   GPIOB
#define SPOT_PIN                    GPIO_PIN_0

#define MATCH_PORT                  GPIOB
#define MATCH_PIN                   GPIO_PIN_10

#define LD_POS_PORT                 GPIOB
#define LD_POS_PIN                  GPIO_PIN_12

#define LD_SLOPE_PORT               GPIOB
#define LD_SLOPE_PIN                GPIO_PIN_14

#define PIC_LED_PORT                GPIOC
#define PIC_LED_PIN                 GPIO_PIN_0

#define INTRPT_PORT                 GPIOC
#define INTRPT_PIN                  GPIO_PIN_2

#define DIRECTION_PORT              GPIOC
#define DIRECTION_PIN               GPIO_PIN_5

#define MCLR_PORT                   GPIOC
#define MCLR_PIN                    GPIO_PIN_13

#define XBEAM_PORT                   GPIOC
#define XBEAM_PIN                    GPIO_PIN_1

typedef enum
{
    GPIO_GLITCH_SHUTDOWN = 0U,
    GPIO_PIC_LED,
    GPIO_INTRPT,
    GPIO_BUSY,
    GPIO_DIRECTION,
    GPIO_SPOT,
    GPIO_MATCH,
    GPIO_LD_POS,
    GPIO_LD_SLOPE,
    GPIO_MCLR,

    GPIO_DAC_A,
    GPIO_DAC_C,
    GPIO_DAC_B,
    
    GPIO_ADC_XBEAM,
    GPIO_MAX,
} GPIO_Idex;

extern void GPIO_Init(void);

inline extern bool GPIO_Get_GLITCH_SHUTDOWN(void);
inline extern bool GPIO_Get_MATCH(void);

inline extern void GPIO_Set_PIC_LED(GPIO_PinState st);
inline extern void GPIO_Set_INTRPT(GPIO_PinState st);
inline extern void GPIO_Set_BUSY(GPIO_PinState st);
inline extern void GPIO_Set_DIRECTION(GPIO_PinState st);
inline extern void GPIO_Set_SPOT(GPIO_PinState st);
inline extern void GPIO_Set_LD_POS(GPIO_PinState st);
inline extern void GPIO_Set_LD_SLOPE(GPIO_PinState st);

inline extern void GPIO_SetDAC(uint32_t val);
inline extern bool GPIO_Get_MCLR(void);

void GPIO_printIdexAndName(void);
int GPIO_isPinActive(GPIO_Idex idex, GPIO_PinState *config);
bool GPIO_setPinStatus(GPIO_Idex idex, FunctionalState isActive, GPIO_PinState *config);
bool GPIO_getPinName(GPIO_Idex idex, const char **name);
#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_H */
