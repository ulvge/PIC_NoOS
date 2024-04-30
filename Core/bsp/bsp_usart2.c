#include "bsp_usart2.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"
#include "uart_monitor.h"
#include "stm32h7xx_ll_usart.h"

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
static UART_PARA_STRUCT g_UARTPara = {
    .periph = USART2,
    .uartHandle = &g_uart2Handle,
};

DMA_HandleTypeDef g_hdma_usart2_tx;

#define UART1_BUFF_SIZE 	(200)
static INT8U g_buffSend[2048] __attribute__((section(".MY_SECTION")));
static INT8U g_buffRec[UART1_BUFF_SIZE];

void UART2_init(void)
{
    FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
    FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
	
    com_registHandler(&g_UARTPara);

    if (HAL_UART_Init(&g_uart2Handle) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&g_uart2Handle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&g_uart2Handle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&g_uart2Handle) != HAL_OK) {
        Error_Handler();
    }
	LL_USART_EnableIT_RXNE(g_uart2Handle.Instance);
    __HAL_DMA_ENABLE_IT(&g_hdma_usart2_tx, DMA_IT_TC);
}
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t isrflags = READ_REG(huart->Instance->ISR);

    /* If no error occurs */
    /* UART in mode Receiver ---------------------------------------------------*/
    if ((isrflags & USART_ISR_RXNE_RXFNE) != 0U) {
        FIFO_Write(&g_UARTPara.fifo.rfifo, (uint8_t)READ_REG(huart->Instance->RDR));
    }
	if (isrflags & USART_ISR_TC){
		huart->gState = HAL_UART_STATE_READY;
		uart_PostdMsg(false);
	}
    __HAL_UART_CLEAR_FLAG(huart, USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE | USART_ISR_RTOF | USART_ISR_TC);
}


