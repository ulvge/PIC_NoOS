
#include <main.h>
#include <stdlib.h>
#include <string.h>
#include <Types.h>
#include "output_wave.h"
#include "debug_print.h"
#include "spi_communication.h"
#include "bsp_gpio.h"
#include "bsp_spi1_slave.h"
#include "FIFO.h"

/// @brief 重启 中断回调函数，用于检测MCLR引脚是否被按下
/// @param  
void HAL_GPIO_EXTI_Callback_MCLR(void)
{
    static uint32_t oldStamp, nowStamp;
	uint32_t x=API_EnterCirtical();

    oldStamp = Get_dealyTimer_cnt();
    while(GPIO_isPinActive(GPIO_MCLR) == 1){
        nowStamp = Get_dealyTimer_cnt();
        if ((nowStamp - oldStamp) >= OUTPUT_DELAY_4US) {
            HAL_NVIC_SystemReset();
        }
    }
    API_ExitCirtical(x);
}
/// @brief 匹配 中断回调函数
/// @param  
void HAL_GPIO_EXTI_Callback_MATCH(void)
{
    if (GPIO_isPinActive(GPIO_GLITCH_SHUTDOWN)) {
        return;
    }
    Task_outputWave();
}
/// @brief 暂停 中断回调函数
/// @param  
void HAL_GPIO_EXTI_Callback_SHUTDOWN(void)
{
    if (GPIO_isPinActive(GPIO_GLITCH_SHUTDOWN)){
        //pause
        GPIO_Set_BUSY(GPIO_PIN_RESET);
        GPIO_Set_PIC_LED(GPIO_PIN_RESET);
    }
}
// htim5 run clok: 280M
// Period = count 280 = 1us
static void delay_us(uint32_t us, uint32_t _0us)
{
    static uint32_t oldStamp, nowStamp;

    oldStamp = Get_dealyTimer_cnt();
    uint32_t dly = OUTPUT_DELAY_1US * us + OUTPUT_DELAY_0U1S * _0us;
    do{
        if (!g_protocolData.isRecvedFinished){
            break;
        }
        nowStamp = Get_dealyTimer_cnt();
    } while ((nowStamp - oldStamp) < dly);
}

/**
 * @brief 接收完成后的动作
 * @param
 * @return
 */
void SPI_RecOver(void)
{
    // GPIO_Set_INTRPT(GPIO_PIN_SET); //PC2
    // delay_us(OUTPUT_DELAY_1U5S);
    // GPIO_Set_INTRPT(GPIO_PIN_RESET);
}
/**
 * @brief 设置方向信号
 * @param val
 * @return
 */
inline static void output_setDirection(uint16_t val)
{
    GPIO_Set_DIRECTION((GPIO_PinState)(!!(val & BIT(12))));
}
/// @brief send  mode, is slope
/// @param val 
inline static void output_setRunMode(uint16_t val)
{
    GPIO_Set_SPOT((GPIO_PinState)(!!(val & BIT(13))));
}

/**
 * @brief 输出波形的任务
 */
void Task_outputWave(void)
{
    static bool isFirst = true;
    static uint16_t reSendCount, sendDataIndex = 0;

    if (g_protocolData.SendEnable & g_protocolData.isRecvedFinished) {
        reSendCount = 0;
        sendDataIndex = 0;
        // no sending 
        GPIO_Set_BUSY(GPIO_PIN_RESET);
        // no data need be send
        HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_RESET);
        return;
    }

    if (g_protocolCmd.reSendTimes == 0 || (reSendCount < g_protocolCmd.reSendTimes)) {
        uint16_t slp = g_protocolData.data[sendDataIndex].slope;
        output_setRunMode(slp);
        if (isFirst) 
        {
            // send position val
            GPIO_SetDAC(g_protocolData.data[sendDataIndex].position);
            HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_SET);
            //isFirst = false;
        }
        // // send slope val
        output_setDirection(slp);   // send Direction
        GPIO_SetDAC(slp);
        HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_SET);

        if (sendDataIndex >= g_protocolData.recvedGroupCount) {
            sendDataIndex = 0;
            reSendCount++;
            delay_us(g_protocolCmd.sleepUsWave, 0);
            delay_us(g_protocolCmd.sleepUsGroupData, 0);
        }
        //isFirst = false;
        
        // is sending 
        GPIO_Set_BUSY(GPIO_PIN_SET);
        // run normal
        GPIO_Set_PIC_LED(GPIO_PIN_SET); 
        // is need send
        HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_SET);
        return;
    }else{
        // send finished, clear flag
        g_protocolData.SendEnable--;

        // send over
        GPIO_Set_INTRPT(GPIO_PIN_SET);
        delay_us(1, 5);
        GPIO_Set_INTRPT(GPIO_PIN_RESET);
        
        // no sending 
        GPIO_Set_BUSY(GPIO_PIN_RESET);
        // no data need be send
        HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_RESET);
    }
}
