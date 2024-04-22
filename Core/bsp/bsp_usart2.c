#include "bsp_usart2.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"

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
static INT8U g_buffSend[2048];
static INT8U g_buffRec[UART1_BUFF_SIZE];
static void MX_DMA_Init(void);
static void HAL_UART_DMATxCpltCallback(DMA_HandleTypeDef *hdma);

void UART2_init(void)
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));

    MX_DMA_Init();
	
    com_registHandler(&g_UARTPara);

    if (HAL_UART_DeInit(&g_uart2Handle) != HAL_OK)
    {
        Error_Handler();
    }
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
    HAL_DMA_RegisterCallback(&g_hdma_usart2_tx, HAL_DMA_XFER_CPLT_CB_ID, HAL_UART_DMATxCpltCallback);
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
/* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, IRQHANDLER_PRIORITY_UART_DMA, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}

static void HAL_UART_DMATxCpltCallback(DMA_HandleTypeDef *hdma)
{
    UART_sendContinue(USART2);
}

void UART_RxISR_8BIT(UART_HandleTypeDef *huart)
{
    uint8_t val;

    /* Check that a Rx process is ongoing */
    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
        val = (uint8_t)READ_REG(huart->Instance->RDR);
        
        FIFO_Write(&g_UARTPara.fifo.rfifo, val);
    }
}
