#include "FIFO.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include <string.h>

SPI_HandleTypeDef hspi1;

#define UART1_BUFF_SIZE (200)
static INT8U g_buffSend[2048];
static INT8U g_buffRec[UART1_BUFF_SIZE];

static __inline void SPI_ProtocolParsing(uint8_t val);

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_SLAVE;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 0x0;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
}
void SPI1_Init(void)
{
    MX_SPI1_Init();
    HAL_SPI_Receive_IT(&hspi1, g_buffRec, sizeof(g_buffRec));
}

/**
 * @brief  Handle SPI interrupt request.
 * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for the specified SPI module.
 * @retval None
 */
void HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi)
{
    uint32_t itsource = hspi->Instance->IER;
    uint32_t itflag = hspi->Instance->SR;
    uint32_t trigger = itsource & itflag;
    uint32_t cfg1 = hspi->Instance->CFG1;

    HAL_SPI_StateTypeDef State = hspi->State;

    /* SPI in mode Receiver ----------------------------------------------------*/
    if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_RXP) &&
        HAL_IS_BIT_CLR(trigger, SPI_FLAG_DXP)) {
        hspi->RxISR(hspi);  // HAL_SPI_Receive_IT  SPI_RxISR_8BIT 
        
        SPI_ProtocolParsing(*((__IO uint8_t *)(&hspi->Instance->RXDR)));
		return;
    }

    /* SPI End Of Transfer: DMA or IT based transfer */
//    if (HAL_IS_BIT_SET(trigger, SPI_FLAG_EOT)) {
//        /* Clear EOT/TXTF/SUSP flag */
//        __HAL_SPI_CLEAR_EOTFLAG(hspi);
//        __HAL_SPI_CLEAR_TXTFFLAG(hspi);
//        __HAL_SPI_CLEAR_SUSPFLAG(hspi);

//        /* Disable EOT interrupt */
//        __HAL_SPI_DISABLE_IT(hspi, SPI_IT_EOT);

//        /* For the IT based receive extra polling maybe required for last packet */
//        if (HAL_IS_BIT_CLR(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN)) {
//            /* Pooling remaining data */
//            while (hspi->RxXferCount != 0UL) {
//                /* Receive data in 8 Bit mode */
//                *((uint8_t *)hspi->pRxBuffPtr) = *((__IO uint8_t *)&hspi->Instance->RXDR);
//                hspi->pRxBuffPtr += sizeof(uint8_t);

//                hspi->RxXferCount--;
//            }
//        }

//        /* Call SPI Standard close procedure */
//        SPI_CloseTransfer(hspi);

//        hspi->State = HAL_SPI_STATE_READY;
//        if (hspi->ErrorCode != HAL_SPI_ERROR_NONE) {
//#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1UL)
//            hspi->ErrorCallback(hspi);
//#else
//            HAL_SPI_ErrorCallback(hspi);
//#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
//            return;
//        }

//#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1UL)
//        /* Call appropriate user callback */
//        if (State == HAL_SPI_STATE_BUSY_TX_RX) {
//            hspi->TxRxCpltCallback(hspi);
//        } else if (State == HAL_SPI_STATE_BUSY_RX) {
//            hspi->RxCpltCallback(hspi);
//        } else if (State == HAL_SPI_STATE_BUSY_TX) {
//            hspi->TxCpltCallback(hspi);
//        }
//#else
//        /* Call appropriate user callback */
//        if (State == HAL_SPI_STATE_BUSY_TX_RX) {
//            HAL_SPI_TxRxCpltCallback(hspi);
//        } else if (State == HAL_SPI_STATE_BUSY_RX) {
//            HAL_SPI_RxCpltCallback(hspi);
//        } else if (State == HAL_SPI_STATE_BUSY_TX) {
//            HAL_SPI_TxCpltCallback(hspi);
//        }
//#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
//        else {
//            /* End of the appropriate call */
//        }

//        return;
//    }
    /* SPI in Error Treatment --------------------------------------------------*/
    if ((trigger & (SPI_FLAG_MODF | SPI_FLAG_OVR | SPI_FLAG_FRE | SPI_FLAG_UDR)) != 0UL) {
        /* SPI Overrun error interrupt occurred ----------------------------------*/
        if ((trigger & SPI_FLAG_OVR) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
            __HAL_SPI_CLEAR_OVRFLAG(hspi);
        }

        /* SPI Mode Fault error interrupt occurred -------------------------------*/
        if ((trigger & SPI_FLAG_MODF) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
            __HAL_SPI_CLEAR_MODFFLAG(hspi);
        }

        /* SPI Frame error interrupt occurred ------------------------------------*/
        if ((trigger & SPI_FLAG_FRE) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
            __HAL_SPI_CLEAR_FREFLAG(hspi);
        }

        /* SPI Underrun error interrupt occurred ------------------------------------*/
        if ((trigger & SPI_FLAG_UDR) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
            __HAL_SPI_CLEAR_UDRFLAG(hspi);
        }

        if (hspi->ErrorCode != HAL_SPI_ERROR_NONE) {
            /* Disable SPI peripheral */
            __HAL_SPI_DISABLE(hspi);

            /* Disable all interrupts */
            __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_RXP | SPI_IT_TXP | SPI_IT_MODF |
                                        SPI_IT_OVR | SPI_IT_FRE | SPI_IT_UDR));

            
			/* Restore hspi->State to Ready */
			hspi->State = HAL_SPI_STATE_READY;

                /* Call user error callback */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1UL)
                hspi->ErrorCallback(hspi);
#else
                HAL_SPI_ErrorCallback(hspi);
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
        }
        return;
    }
}

static __inline void SPI_ProtocolParsing(uint8_t val)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(val);

    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_SPI_RxCpltCallback should be implemented in the user file
     */
}


