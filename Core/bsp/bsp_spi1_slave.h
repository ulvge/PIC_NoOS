#ifndef __BSP_SPI1_SLAVE_H
#define	__BSP_SPI1_SLAVE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>  

typedef struct {
    uint32_t recevedStartClk;
    uint32_t recevedRespondClk;
    uint32_t recevedDurationThis;    //0.1us
    uint32_t recevedMaxDuration;    //0.1us
    uint32_t recevedRespondTimeoutCnt;

    uint32_t sendStartClk;
    uint32_t sendFinishedClk;
    uint32_t sendCntThis;
    uint32_t sendAverageTimeThis;   //us
    uint32_t sendAverageTimeMax;    //us
} Diagnosis;

extern DMA_HandleTypeDef g_hdma_spi1_tx;
extern void SPI1_Init(void);
extern Diagnosis g_diagnosis;
extern inline void bsp_spi_DiagSendFinished(uint32_t sendTimes);
extern inline void bsp_spi_DiagSendStart(void);
extern void SPI1_startReceviceIT(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_SPI1_SLAVE_H */
