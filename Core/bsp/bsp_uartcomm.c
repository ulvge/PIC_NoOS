#include <string.h>
#include "main.h"
#include "bsp_uartcomm.h"
#include "bsp_usart2.h"
#include "task.h"
#include "freertos.h"
#include "semphr.h"

#define UART_NUM_TOTAL 1

static UART_PARA_STRUCT *g_pUARTSHandler[UART_NUM_TOTAL] = {NULL};	

//打印接口
int fputc(int ch, FILE *f)
{
    return UART_sendByte(DEBUG_UART_PERIPH, ch);
}
/**
 * @brief 注册 UART,对其统一管理
 * @param uartPara UART参数结构体指针
 * @return 成功返回true，否则返回false
 */
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
/**
 * @brief 获取指定的 UART 参数
 * @param uartPara 指定的UART
 * @return 成功返回 Handler，否则返回 NULL
 */
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
/**
 * @brief 从指定的UART的fifo中获取一个字节的数据
 * @param usart_periph UART设备的寄存器指针
 * @param p_buffer 用于存储数据的指针
 * @return 成功返回true，否则返回false
 */
bool UART_getByte(USART_TypeDef *usart_periph, uint8_t *p_buffer)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_Read(&uartPara->fifo.rfifo, p_buffer);
}

/**
 * @brief 从指定的UART的fifo中获取一些字节的数据
 * @param usart_periph UART设备的寄存器指针
 * @param p_buffer 用于存储数据的指针
 * @return 成功返回true，否则返回false
 */
bool UART_getData(USART_TypeDef *usart_periph, uint8_t *p_buffer, uint32_t buffSize, INT16U *retLen)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_ReadN(&uartPara->fifo.rfifo, p_buffer, buffSize, (INT16U *)retLen);
}


/**
 * @brief 往指定的UART的fifo中写入一个字节的数据
 * @param usart_periph UART设备的寄存器指针
 * @param dat 数据
 * @return 成功返回true，否则返回false
 */
__inline bool UART_sendByte(USART_TypeDef *usart_periph, uint8_t dat)
{
    return UART_sendData(usart_periph, &dat, 1);
}
/**
 * @brief 往指定的UART的fifo中写入一些数据
 * @param usart_periph UART设备的寄存器指针
 * @param str 数据指针
 * @param len 数据长度
 * @return 成功返回true，否则返回false
 */
bool UART_sendData(USART_TypeDef *usart_periph, uint8_t *str, uint16_t len)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }

    if (FIFO_Writes(&uartPara->fifo.sfifo, str, len) == FALSE){
        return HAL_UART_Transmit_DMA(uartPara->uartHandle, (uint8_t*)str, len);
    }else{
        UART_sendContinue(DEBUG_UART_PERIPH);
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
/**
 * @brief 继续发送UART数据
 * @param usart_periph UART设备的寄存器指针
 * @return void
 */
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
    bool isNotTail = fifo->rp + fifo->occupy < fifo->limit;
    if (isNotTail) {
        sendSize = fifo->occupy;
    }else{
        sendSize = fifo->limit - fifo->rp;
    }

    if(HAL_UART_Transmit_DMA(uartPara->uartHandle, fifo->rp, sendSize) == HAL_OK)
    {
        uint32_t x=API_EnterCirtical();
        fifo->occupy -= sendSize;
        if (isNotTail) {
            fifo->rp += sendSize;
        }else{
            fifo->rp = fifo->array;
        }
        API_ExitCirtical(x);
    }
}
/**
 * @brief UART init
 * @param void
 * @return void
 */
void UART_init(void)
{
    UART2_init();
}
