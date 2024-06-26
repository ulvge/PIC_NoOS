#ifndef __BSP_SPI1_SLAVE_H
#define	__BSP_SPI1_SLAVE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>  

extern DMA_HandleTypeDef g_hdma_spi1_tx;
extern void SPI1_Init(void);
extern inline void bsp_spi_DiagSendFinished(uint32_t sendTimes);
extern inline void bsp_spi_DiagSendStart(void);
extern void SPI1_startReceviceIT(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_SPI1_SLAVE_H */
