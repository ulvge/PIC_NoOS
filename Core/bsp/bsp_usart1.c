#include "bsp_usart1.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"

UART_HandleTypeDef g_uart1Handle = {
    .Instance = USART1,
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
    .periph = USART1,
    .uartHandle = &g_uart1Handle,
};

DMA_HandleTypeDef g_hdma_usart1_tx;

#define UART1_BUFF_SIZE 	(200)
static INT8U g_buffSend[2048];
static INT8U g_buffRec[UART1_BUFF_SIZE];
static void MX_DMA_Init(void);

void UART1_init(void)
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
    
    MX_DMA_Init();
	
    com_registHandler(&g_UARTPara);

    if (HAL_UART_DeInit(&g_uart1Handle) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UART_Init(&g_uart1Handle) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&g_uart1Handle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&g_uart1Handle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&g_uart1Handle) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *husart)
{
    if (husart->Instance == USART1){
        UART_sendContinue(USART1);
    }
}

// void USART1_IRQHandler(void)
// {
// #define COM_NUM    COM0
//     uint8_t res;
// 	static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?
//     BaseType_t err;
//     static bool is_start = false;

//     if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_RBNE))
//     {
//         res = usart_data_receive(COM_NUM);
//         usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_RBNE);
//         /* receive data */
//         if (res == START_BYTE)
//         { // start
//             is_start = true;
//             g_uart_Req.Size = 0;
//         }
//         else if (res == STOP_BYTE && is_start == true)
//         { // stop
//             is_start = false;
//             // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
//             if (RecvDatMsg_Queue != NULL)
//             {
//                 g_uart_Req.Param = SERIAL_REQUEST;
//                 err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
//                 portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//                 if (err == pdFAIL)
//                 {
//                     //LOG_E("uart queue msg send failed!");
//                 }
//             }
//         }
//         else
//         {
//             g_uart_Req.Data[g_uart_Req.Size++] = res;
//             if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
//             {
//                 is_start = false;
//                 g_uart_Req.Size = 0;
//                 //LOG_E("uart recv overlap!");
//             }
//         }  

// 	// use FIFO store all
// 		if (is_start == false)
// 		{
// 			/* receive data */
//             //UART_sendByte(COM_NUM, res);  //loopback
// 			FIFO_Write(&g_UARTPara.fifo.rfifo, (INT8U)res);
// 		}
// 	}

//     if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_TC))
//     {
//         usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_TC);
//         /* send data continue */
// 		UART_sendFinally(COM_NUM, &g_UARTPara.fifo);
//     }
// }
