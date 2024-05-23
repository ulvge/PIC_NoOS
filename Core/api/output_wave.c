
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

SemaphoreHandle_t g_sem_recvedWaveData;
SemaphoreHandle_t g_sem_isSending;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t oldStamp, nowStamp;
    if (GPIO_Pin == MCLR_PIN) {
		uint32_t x=API_EnterCirtical();
		oldStamp = Get_dealyTimer_cnt();
        while(GPIO_isPinActive(GPIO_MCLR, NULL) == 1){
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

void SPI_RecOver(void){
    // GPIO_Set_INTRPT(GPIO_PIN_SET); //PC2
    // delay_us(OUTPUT_DELAY_1U5S);
    // GPIO_Set_INTRPT(GPIO_PIN_RESET);
}
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

inline static void output_waitMasterBeReady(void)
{
    while (!GPIO_isPinActive(GPIO_GLITCH_SHUTDOWN, NULL)) {
        GPIO_Set_PIC_LED(GPIO_PIN_RESET);
        if (!g_protocolData.isRecvedFinished){
            return;
        }
    }
}
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
    // while (!GPIO_isPinActive(GPIO_MATCH, NULL)) {
    // }
}
void output_debug(void){
    static int16_t step = 0;
    static uint16_t dir = 1;
    while(1){
        if (dir){
            step += 100;
            if (step >= 0x1000){
                dir = 0;
            }
        }else{
            step -= 100;
            if (step <= 0){
                dir = 1;
                break;
            }
        }
        GPIO_SetDAC(step);
        GPIO_Set_LD_POS(GPIO_PIN_SET);
        delay_us(OUTPUT_DELAY_0U1S, 0);
        GPIO_Set_LD_POS(GPIO_PIN_RESET);
    }
}
void Task_outputWave(void *argument)
{
    bool isFirst = true;
    uint16_t reSendCount;
    uint16_t slp;
    g_sem_recvedWaveData = xSemaphoreCreateBinary();
    g_sem_isSending = xSemaphoreCreateMutex();
    uint32_t dlyUs = 65; // 65 5.7k
    while (1) {
        HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_RESET);
        if (xSemaphoreTake(g_sem_recvedWaveData, portMAX_DELAY) == pdTRUE) {
            HAL_GPIO_WritePin(LD_MSLOPE_PORT, LD_MSLOPE_PIN, GPIO_PIN_SET);
            bsp_spi_DiagSendStart();
            vTaskSuspendAll();
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
                        //output_debug();
                        //isFirst = false;
                    }
                    // // send slope val
                    output_setDirection(slp);   // send Direction
                    GPIO_SetDAC(slp);
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_SET);
                    // wait master match
                    delay_us(g_protocolCmd.sleepUsWave, 0);
                    output_waitMasterMatch();
                    
                    delay_us(dlyUs, 0); 
                }

                delay_us(g_protocolCmd.sleepUsGroupData, 0);
            }

            GPIO_Set_BUSY(GPIO_PIN_RESET);

            GPIO_Set_INTRPT(GPIO_PIN_SET);
            delay_us(1, 5);
            GPIO_Set_INTRPT(GPIO_PIN_RESET);

            bsp_spi_DiagSendFinished(reSendCount);

            if (xSemaphoreTake(g_sem_isSending, 0) == pdTRUE) {// is sending
                g_protocolData.isSending = false;
                xSemaphoreGive(g_sem_isSending);
            }
            if (!xTaskResumeAll()) {
                taskYIELD();
            }
        }
    }
}
