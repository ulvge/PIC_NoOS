#include <string.h>
#include "main.h"
//#include "Message.h"
#include "bsp_uartcomm.h"
#include "bsp_usart2.h"
#include "task.h"

#define UART_NUM_TOTAL 1

static UART_PARA_STRUCT *g_pUARTSHandler[UART_NUM_TOTAL] = {NULL};	
bool g_isPrintNoBlock = true;

//use FIFO
int fputc(int ch, FILE *f)
{
    if (g_isPrintNoBlock) {
        return UART_sendByte(DEBUG_UART_PERIPH, ch);
    } else {
        return UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)&ch, 1);
    }
}
bool com_registHandler(UART_PARA_STRUCT *uartPara)
{
    uint32_t i;
    for (i = 0; i < UART_NUM_TOTAL; i++)
    {
        if (g_pUARTSHandler[i] == NULL){
            break;
        }
        if (g_pUARTSHandler[i]->periph == uartPara->periph) {
            return true; //alread exist
        }
    }
    if (i < UART_NUM_TOTAL) {
        g_pUARTSHandler[i] = uartPara;
        return true; //success
    } else{
        return false;
    }
}
UART_PARA_STRUCT *com_getHandler(USART_TypeDef *usart_periph)
{
    for (uint32_t i = 0; i < UART_NUM_TOTAL; i++)
    {
        if (g_pUARTSHandler[i]->periph == usart_periph) {
            return g_pUARTSHandler[i]; //alread exist
        }
        if (g_pUARTSHandler[i] == NULL){
            break;
        }
    }
    return NULL; //alread exist
}

bool UART_getByte(USART_TypeDef *usart_periph, uint8_t *p_buffer)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_Read(&uartPara->fifo.rfifo, p_buffer);
}

bool UART_getData(USART_TypeDef *usart_periph, uint8_t *p_buffer, uint32_t buffSize, INT16U *retLen)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_ReadN(&uartPara->fifo.rfifo, p_buffer, buffSize, (INT16U *)retLen);
}


__inline bool UART_sendByte(USART_TypeDef *usart_periph, uint8_t dat)
{
    return UART_sendData(usart_periph, &dat, 1);
}

bool UART_sendData(USART_TypeDef *usart_periph, uint8_t *str, uint16_t len)
{
    // 如果buff中，有数据，说明上次的还没发送完，则先写到buff中
    // 如果buff中，没有数据了，检测是否已经发送完成。如果busy，则写到buff中；如果idle，则发送

    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    // 如果buff没有数据，且 uart 空闲，则直接发送
    if (FIFO_Empty(&(uartPara->fifo.sfifo)) & (uartPara->uartHandle->gState == HAL_UART_STATE_READY))
    {
        if(HAL_UART_Transmit_DMA(uartPara->uartHandle, (uint8_t*)str, len)!= HAL_OK)
        {
            Error_Handler();
            return false;
        }
    }
    else{
        // 则写到buff中
        if (FIFO_Writes(&uartPara->fifo.sfifo, str, len) == FALSE){
            return false;
        }
    }
	return true;
}

/// @brief NO fifo,can be called in HardFault_Handler
/// @param usart_periph 
/// @param str 
/// @param len 
bool UART_sendDataBlock(USART_TypeDef *usart_periph, const uint8_t *str, uint16_t len)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    HAL_UART_Transmit(uartPara->uartHandle, (uint8_t*)str, len, 5000);
    return true;
}

/// @brief read one byte from fifo,and start transmit
/// @param usart_periph 
/// @param fifoUart 
/// @return 
INT8U UART_sendFinally(USART_TypeDef *usart_periph, FIFO_Buf_STRUCT *fifoUart)
{
    INT8U data;
    if (FIFO_Empty(&(fifoUart->sfifo)))
    {
        fifoUart->status &= ~UART_SENDING;
        return false;
    }
    else
    {
        if (fifoUart->status != UART_SENDING)
        {
            fifoUart->status |= UART_SENDING;
        }
        if (FIFO_Read(&(fifoUart->sfifo), &data) == true)
        { // sending data
            UART_sendByte(usart_periph, data);
        }
        return true;
    }
}

void UART_sendContinue(USART_TypeDef *usart_periph)
{
    INT16U sendSize;

    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return;
    }
    FIFO *fifo= &uartPara->fifo.sfifo;
    if (fifo->occupy == 0) {
		return;
	}
    // 1、 array      xxxxx   limit
    // 2、 array xx       xxx limit
    uint32_t x=API_EnterCirtical();
    if (fifo->rp + fifo->occupy < fifo->limit) {
        sendSize = fifo->occupy;
        fifo->rp += sendSize;
        fifo->occupy = 0;
    }else{
        sendSize = fifo->limit - fifo->rp;
        fifo->occupy -= sendSize;
        fifo->rp = fifo->array;
    }
    API_ExitCirtical(x);
    if(HAL_UART_Transmit_DMA(uartPara->uartHandle, fifo->rp, sendSize)!= HAL_OK)
    {
        Error_Handler();
    }
}

void UART_init(void)
{
    UART2_init();
}
