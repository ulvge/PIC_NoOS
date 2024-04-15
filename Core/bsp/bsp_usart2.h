#ifndef __BSP_USART2_H
#define	__BSP_USART2_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>  
#include <bsp_uartcomm.h>
			 
void UART2_init(void);
extern void UART_RxISR_8BIT(UART_HandleTypeDef *huart);
extern DMA_HandleTypeDef g_hdma_usart2_tx;
#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART2_H */
