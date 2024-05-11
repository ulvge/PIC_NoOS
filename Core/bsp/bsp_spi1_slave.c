#include "FIFO.h"
#include "main.h"
#include "shell_ext.h"
#include "shell_port.h"
#include "debug_print.h"
#include "bsp_spi1_slave.h"
#include "bsp_gpio.h"

#include "stm32h7xx_hal.h"
#include <string.h>
#include "spi_communication.h"
#include "output_wave.h"

#define SPI_DIAG_LIMIT_MAX_xUS 5
#define SPI_DIAG_LIMIT_MAX_CLK  OUTPUT_DELAY_xUS(SPI_DIAG_LIMIT_MAX_xUS)

SPI_HandleTypeDef g_hspi1;
Diagnosis g_diagnosis;

DMA_HandleTypeDef g_hdma_spi1_tx;

static void SPI_EnableChipSelect() {
    GPIO_setPinStatus(GPIO_SPI1_NSS_IDEX, ENABLE, NULL);
}
/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{
    /* SPI1 parameter configuration*/
    g_hspi1.Instance = SPI1;
    g_hspi1.Init.Mode = SPI_MODE_SLAVE;
    g_hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    g_hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    g_hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    g_hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    g_hspi1.Init.NSS = SPI_NSS_SOFT;    // SSM
    g_hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    g_hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    g_hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    g_hspi1.Init.CRCPolynomial = 0x0;
    g_hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    g_hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;//ssiop
    g_hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    g_hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    g_hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    g_hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    g_hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    g_hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    g_hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    g_hspi1.Init.IOSwap = SPI_IO_SWAP_ENABLE;
    g_hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    if (HAL_SPI_Init(&g_hspi1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
}
void SPI1_startReceviceIT(void)
{
    static uint8_t rxBuffer;
    HAL_SPI_Receive_IT(&g_hspi1, &rxBuffer, sizeof(rxBuffer));
}
void SPI1_Init(void)
{
    MX_SPI1_Init();
    SPI_ProtocolInit();
    SPI_EnableChipSelect();
    SPI1_startReceviceIT();
}

inline void bsp_spi_DiagReceved()
{
    g_diagnosis.recevedDurationThis = (g_diagnosis.recevedRespondClk - g_diagnosis.recevedStartClk) / OUTPUT_DELAY_0U1S;
    if (g_diagnosis.recevedDurationThis > g_diagnosis.recevedMaxDuration) {
        g_diagnosis.recevedMaxDuration = g_diagnosis.recevedDurationThis;
    }
    if (g_diagnosis.recevedDurationThis > SPI_DIAG_LIMIT_MAX_xUS){
        g_diagnosis.recevedRespondTimeoutCnt++;
    }
}
inline void bsp_spi_DiagSendStart(void)
{
    g_diagnosis.sendStartClk = Get_dealyTimer_cnt();
}
inline void bsp_spi_DiagSendFinished(uint32_t sendTimes)
{
    g_diagnosis.sendFinishedClk = Get_dealyTimer_cnt();
    if (sendTimes == 0){
        return;
    }
    g_diagnosis.sendCntThis = sendTimes;
    g_diagnosis.sendAverageTimeThis = ((g_diagnosis.sendFinishedClk - g_diagnosis.sendStartClk) / OUTPUT_DELAY_1US) / sendTimes;
    if (g_diagnosis.sendAverageTimeThis > g_diagnosis.sendAverageTimeMax) {
        g_diagnosis.sendAverageTimeMax = g_diagnosis.sendAverageTimeThis;
    }
}

/**
 * @brief  Handle SPI interrupt request.
 * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for the specified SPI module.
 * @retval None
 */
void HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi)
{
    static uint32_t itsource,itflag,trigger;
    uint32_t handled  = 0UL;

    g_diagnosis.recevedStartClk = Get_dealyTimer_cnt();
    itsource = hspi->Instance->IER;
    itflag = hspi->Instance->SR;
    trigger = itsource & itflag;

    HAL_SPI_StateTypeDef State = hspi->State;
    hspi->State = HAL_SPI_STATE_READY;

    /* SPI in mode Receiver ----------------------------------------------------*/
    //if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_RXP) && HAL_IS_BIT_CLR(trigger, SPI_FLAG_DXP)) {
    if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_RXP)) {
        
        //hspi->RxISR(hspi);  // HAL_SPI_Receive_IT  SPI_RxISR_8BIT 
        
        SPI_ProtocolParsing(*((__IO uint8_t *)(&hspi->Instance->RXDR)));
        g_diagnosis.recevedRespondClk = Get_dealyTimer_cnt();
        bsp_spi_DiagReceved();
        handled = 1UL;
    }
    /* SPI in mode Transmitter -------------------------------------------------*/
    if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_UDR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_TXP))
    {
        hspi->TxISR(hspi);
        handled = 1UL;
    }
    if (handled != 0UL)
    {
        return;
    }
    /* Clear EOT/TXTF/SUSP flag */
    __HAL_SPI_CLEAR_EOTFLAG(hspi);
    __HAL_SPI_CLEAR_TXTFFLAG(hspi);
    __HAL_SPI_CLEAR_SUSPFLAG(hspi);
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

//        if (hspi->ErrorCode != HAL_SPI_ERROR_NONE) {
//            /* Disable SPI peripheral */
//            __HAL_SPI_DISABLE(hspi);

//            /* Disable all interrupts */
//            __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_RXP | SPI_IT_TXP | SPI_IT_MODF |
//                                        SPI_IT_OVR | SPI_IT_FRE | SPI_IT_UDR));

//            
//			/* Restore hspi->State to Ready */
//			hspi->State = HAL_SPI_STATE_READY;

//                /* Call user error callback */
//#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1UL)
//                hspi->ErrorCallback(hspi);
//#else
//                HAL_SPI_ErrorCallback(hspi);
//#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
//        }
        return;
    }
}

// shell command
static void display_usage(void)
{
    LOG_RAW("\t\tSPI, Get its real-time performance \r\n");
    return;
}

static int spi_shell(int argc, char *argv[])
{
    if (argc != 1)
    {
        display_usage();
        return 0;
    }
    LOG_RAW(" *****  SPI diganosis *****\r\n\r\n");
    LOG_RAW("\t receve, this=[ %d.%d us], max[ %d.%d us], timeoutCnt=[ %d ]\r\n", 
            g_diagnosis.recevedDurationThis / 10, g_diagnosis.recevedMaxDuration % 10, 
            g_diagnosis.recevedMaxDuration / 10, g_diagnosis.recevedMaxDuration % 10, 
            g_diagnosis.recevedRespondTimeoutCnt);

    LOG_RAW("\t send Avg, this=[ %d us], max=[ %d us], send cnt=[ %d ]\r\n", 
            g_diagnosis.sendAverageTimeThis, g_diagnosis.sendAverageTimeMax, g_diagnosis.sendCntThis);

    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, spi, spi_shell, Get its real-time performance);

