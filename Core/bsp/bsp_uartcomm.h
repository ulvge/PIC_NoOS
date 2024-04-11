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

bool UART_sendDataBlock(USART_TypeDef *usart_periph, const uint8_t *str, uint16_t len);
//int fputc(int ch, FILE *f);
bool com_registHandler(UART_PARA_STRUCT *uartPara);

bool UART_getByte(USART_TypeDef *usart_periph, uint8_t *p_buffer);
bool UART_getData(USART_TypeDef *usart_periph, uint8_t *p_buffer, uint32_t buffSize, INT16U *retLen);
bool UART_sendByte(USART_TypeDef *usart_periph, uint8_t dat);
bool UART_sendData(USART_TypeDef *usart_periph, uint8_t *str, uint16_t len);
INT8U UART_sendFinally(USART_TypeDef *usart_periph, FIFO_Buf_STRUCT *fifoUart);
void UART_sendContinue(USART_TypeDef *usart_periph);
void UART_init(void);

#endif /* __BSP_UARTCOMM_H */
