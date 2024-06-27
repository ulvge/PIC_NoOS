
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

//定义信号量
/**
 * @brief EXTI中断回调函数
 * @param GPIO_Pin EXTI引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t oldStamp, nowStamp;
    if (GPIO_Pin == MCLR_PIN) {
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
 * @brief 等待主机Ready
 * @param
 * @return
 */
inline static void output_waitMasterBeReady(void)
{
    while (!GPIO_isPinActive(GPIO_GLITCH_SHUTDOWN)) {
        GPIO_Set_PIC_LED(GPIO_PIN_RESET);
        if (!g_protocolData.isRecvedFinished){
            return;
        }
    }
}

/**
 * @brief 等待匹配信号
 * @param
 * @return
 */
inline static void output_waitMasterMatch(void)
{
    static GPIO_PinState matchStatusLast = GPIO_PIN_RESET;
    GPIO_PinState matchStatus;
    do{
        if (!g_protocolData.isRecvedFinished){
            break;
        }
        matchStatus = HAL_GPIO_ReadPin(MATCH_PORT, MATCH_PIN);
    } while (matchStatus == matchStatusLast);

    matchStatusLast = matchStatus;
}

/**
 * @brief 输出波形的任务
 */
void Task_outputWave(void)
{
    bool isFirst = true;
    uint16_t reSendCount;
    uint16_t slp;
    while (1) {
        HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_RESET);
        if (g_protocolData.isSending & g_protocolData.isRecvedFinished) {
            HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_SET);
            vTaskSuspendAll();
            // BUSY set 
            GPIO_Set_BUSY(GPIO_PIN_SET);
            reSendCount = 0;

            isFirst = true;

            while (g_protocolData.isRecvedFinished && (g_protocolCmd.reSendTimes == 0 || reSendCount < g_protocolCmd.reSendTimes)) {
                reSendCount++;
                for (size_t i = 0; i < g_protocolData.recvedGroupCount; i++) {
                    // wait master ready
                    output_waitMasterBeReady();    
                    GPIO_Set_PIC_LED(GPIO_PIN_SET); // run normal

                    slp = g_protocolData.data[i].slope;
                    output_setRunMode(slp);
                    if (isFirst) 
					{
                        // send position val
                        GPIO_SetDAC(g_protocolData.data[i].position);
                        HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_SET);
                        //isFirst = false;
                    }
                    // // send slope val
                    output_setDirection(slp);   // send Direction
                    GPIO_SetDAC(slp);
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_SET);
                    // wait master match
                    output_waitMasterMatch();
                }

                delay_us(g_protocolCmd.sleepUsWave, 0);
                delay_us(g_protocolCmd.sleepUsGroupData, 0);
            }

            // BUSY reset 
            GPIO_Set_BUSY(GPIO_PIN_RESET);

            GPIO_Set_INTRPT(GPIO_PIN_SET);
            delay_us(1, 5);
            GPIO_Set_INTRPT(GPIO_PIN_RESET);
            
            // send finished, clear flag
            g_protocolData.isSending = false;
        }
    }
}
