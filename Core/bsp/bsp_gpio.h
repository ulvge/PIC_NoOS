#ifndef __BSP_GPIO_H
#define	__BSP_GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>  


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

    GPIO_DAC_A,
    GPIO_DAC_C,
    GPIO_DAC_B,

    GPIO_MAX,
} GPIO_NAMES;

extern void GPIO_Init(void);
#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_H */
