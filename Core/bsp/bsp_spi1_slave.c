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

SPI_HandleTypeDef g_hspi1;
DMA_HandleTypeDef g_hdma_spi1_tx;
/**
 * @brief 使能SPI1的片选引脚
 * @return void
 */
static void SPI_EnableChipSelect() {
    GPIO_setPinStatus(GPIO_SPI1_NSS_IDEX, ENABLE);
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
}
/**
 * @brief 开始SPI1的接收（中断模式）
 * @return void
 */
void SPI1_startReceviceIT(void)
{
    static uint8_t rxBuffer;
    HAL_SPI_Receive_IT(&g_hspi1, &rxBuffer, sizeof(rxBuffer));
}
/**
 * @brief SPI1初始化函数
 * @return void
 */
void SPI1_Init(void)
{
    MX_SPI1_Init();
    SPI_ProtocolInit();
    SPI_EnableChipSelect();
    SPI1_startReceviceIT();
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

    itsource = hspi->Instance->IER;
    itflag = hspi->Instance->SR;
    trigger = itsource & itflag;

    HAL_SPI_StateTypeDef State = hspi->State;
    hspi->State = HAL_SPI_STATE_READY;

    /* SPI in mode Receiver ----------------------------------------------------*/
    if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_RXP)) {
        SPI_ProtocolParsing(*((__IO uint8_t *)(&hspi->Instance->RXDR)));
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

        return;
    }
}
