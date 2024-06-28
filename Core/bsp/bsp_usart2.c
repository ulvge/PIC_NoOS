#include "bsp_usart2.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"
#include "stm32h7xx_ll_usart.h"

/**
 * @brief UART2的 SDK 参数配置
 */
UART_HandleTypeDef g_uart2Handle = {
    .Instance = USART2,
    .Init.BaudRate = 115200,
    .Init.WordLength = UART_WORDLENGTH_8B,
    .Init.StopBits = UART_STOPBITS_1,
    .Init.Parity = UART_PARITY_NONE,
    .Init.Mode = UART_MODE_TX_RX,
    .Init.HwFlowCtl = UART_HWCONTROL_NONE,
    .Init.OverSampling = UART_OVERSAMPLING_16,
    .Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
    .Init.ClockPrescaler = UART_PRESCALER_DIV1,
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};
/**
 * @brief UART2 配置 其它参数
 */
UART_PARA_STRUCT g_UARTPara2 = {
    .periph = USART2,
    .uartHandle = &g_uart2Handle,
};

DMA_HandleTypeDef g_hdma_usart2_tx;
//定义发送 buff长度
#define UART1_BUFF_SIZE 	(200)
//定义发送 buff
static INT8U g_buffSend[2048] __attribute__((section(".MY_SECTION")));
//定义接收buff
static INT8U g_buffRec[UART1_BUFF_SIZE];
/**
 * @brief UART2初始化函数
 * @return void
 */
void UART2_init(void)
{
    /* 初始化 UART 指定的 fifo */
    FIFO_Init(&g_UARTPara2.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
    FIFO_Init(&g_UARTPara2.fifo.rfifo, g_buffRec, sizeof(g_buffRec));

    /* 将 UART 硬件 init  */
    if (HAL_UART_Init(&g_uart2Handle) != HAL_OK) {
        Error_Handler();
    }
    /* config UART 硬件 Tx fifo   */
    if (HAL_UARTEx_SetTxFifoThreshold(&g_uart2Handle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    /* config UART 硬件 Rx fifo   */
    if (HAL_UARTEx_SetRxFifoThreshold(&g_uart2Handle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&g_uart2Handle) != HAL_OK) {
        Error_Handler();
    }
    /* config UART interrupt enable   */
	LL_USART_EnableIT_RXNE(g_uart2Handle.Instance);
    __HAL_DMA_ENABLE_IT(&g_hdma_usart2_tx, DMA_IT_TC);
}
/**
 * @brief UART中断处理函数
 * @param huart UART句柄
 * @return void
 */
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    /* get interrupt status register */
    uint32_t isrflags = READ_REG(huart->Instance->ISR);

    /* If no error occurs */
    /* UART in mode Receiver ---------------------------------------------------*/
    if ((isrflags & USART_ISR_RXNE_RXFNE) != 0U) {
        FIFO_Write(&g_UARTPara2.fifo.rfifo, (uint8_t)READ_REG(huart->Instance->RDR));
    }
    /* UART Transmission Complete ---------------------------------------------------*/
	if (isrflags & USART_ISR_TC){
		huart->gState = HAL_UART_STATE_READY;
	}
    // 清除异常中断标记
    __HAL_UART_CLEAR_FLAG(huart, USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE | USART_ISR_RTOF | USART_ISR_TC);
}

//串口发送数据
void UART2_monitor(void)
{
    if (!FIFO_Empty(&g_UARTPara2.fifo.sfifo))
    {
        if (g_UARTPara2.uartHandle->gState == HAL_UART_STATE_READY)
        {
            UART_sendContinue();
        }
    }
}
