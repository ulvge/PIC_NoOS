
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
static void delay_us(uint32_t dly)
{
    static uint32_t oldStamp, nowStamp;

    oldStamp = Get_dealyTimer_cnt();
    do{
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
    GPIO_Set_DIRECTION((GPIO_PinState)(val & BIT(12)));
}
/// @brief send  mode, is slope
/// @param val 
inline static void output_setRunMode(uint16_t val)
{
    GPIO_Set_SPOT((GPIO_PinState)(val & BIT(13)));
}

inline static void output_waitMasterBeReady(void)
{
    while (!GPIO_isPinActive(GPIO_GLITCH_SHUTDOWN, NULL)) {
        GPIO_Set_PIC_LED(GPIO_PIN_RESET);
    }
}
inline static void output_waitMasterMatch(void)
{
    while (!GPIO_isPinActive(GPIO_MATCH, NULL)) {
    }
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
        delay_us(OUTPUT_DELAY_0U1S);
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
    //uint32_t dly = 0x5a00; // 0x5a00 5.7k
    uint32_t dly = 0x2000; // 0x5a00 5.7k
    while (1) {
        if (xSemaphoreTake(g_sem_recvedWaveData, portMAX_DELAY) == pdTRUE) {
            bsp_spi_DiagSendStart();
            vTaskSuspendAll();
            GPIO_Set_BUSY(GPIO_PIN_SET);
            reSendCount = 0;

            isFirst = true;

            while (g_protocolCmd.reSendTimes == 0 || reSendCount++ < g_protocolCmd.reSendTimes) {
                for (size_t i = 1; i < g_protocolData.recvedGroupCount; i++) {
                    // wait master ready
                    output_waitMasterBeReady();    
                    GPIO_Set_PIC_LED(GPIO_PIN_SET); // run normal

                    slp = g_protocolData.data[i].slope;
                    output_setRunMode(slp);
                    if (isFirst) 
					{
                        HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_RESET);
                        // send position val
                        GPIO_SetDAC(g_protocolData.data[i].position);
                        delay_us(dly); 
                        HAL_GPIO_WritePin(LD_POS_PORT, LD_POS_PIN, GPIO_PIN_SET);
                        //output_debug();
                        //isFirst = false;
                    }
                    // // send slope val
                    output_setDirection(slp);   // send Direction
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_RESET);
                    GPIO_SetDAC(slp);
                    delay_us(dly); 
                    HAL_GPIO_WritePin(LD_SLOPE_PORT, LD_SLOPE_PIN, GPIO_PIN_SET);
                    // wait master match
                    delay_us(g_protocolCmd.sleepUsWave);
                    //output_waitMasterMatch();
                }

                delay_us(g_protocolCmd.sleepUsGroupData);
            }


            GPIO_Set_BUSY(GPIO_PIN_RESET);

            GPIO_Set_INTRPT(GPIO_PIN_SET);
            delay_us(OUTPUT_DELAY_1U5S);
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
