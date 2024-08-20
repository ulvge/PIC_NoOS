#include <string.h>
#include "main.h"
#include "bsp_uartcomm.h"
#include "bsp_usart2.h"

#define UART_NUM_TOTAL 1

//打印接口
int fputc(int ch, FILE *f)
{
    uint8_t dat = (uint8_t)ch;
    return UART_sendData(&dat, 1);
}

/**
 * @brief 往指定的UART的fifo中写入一些数据
 * @param usart_periph UART设备的寄存器指针
 * @param str 数据指针
 * @param len 数据长度
 * @return 成功返回true，否则返回false
 */
bool UART_sendData(uint8_t *str, uint16_t len)
{
    if (FIFO_Writes(&g_UARTPara2.fifo.sfifo, str, len) == FALSE){
        return HAL_UART_Transmit_DMA(g_UARTPara2.uartHandle, (uint8_t*)str, len);
    }
	return true;
}

/**
 * @brief 继续发送UART数据
 * @param usart_periph UART设备的寄存器指针
 * @return void
 */
void UART_sendContinue(void)
{
    INT16U sendSize;

    FIFO *fifo= &g_UARTPara2.fifo.sfifo;
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

    if(HAL_UART_Transmit_DMA(g_UARTPara2.uartHandle, fifo->rp, sendSize) == HAL_OK)
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
