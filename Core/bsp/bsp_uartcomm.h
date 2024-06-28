#ifndef __BSP_UARTCOMM_H
#define	__BSP_UARTCOMM_H

#include <stdio.h>
#include <stdbool.h>
#include <FIFO.h>
#include "stm32h7xx_hal.h"

typedef struct {
    USART_TypeDef               *periph; 
    FIFO_Buf_STRUCT             fifo;
    UART_HandleTypeDef          *uartHandle;
}UART_PARA_STRUCT;

bool UART_sendData(uint8_t *str, uint16_t len);
void UART_sendContinue(void);
void UART_init(void);

#endif /* __BSP_UARTCOMM_H */
