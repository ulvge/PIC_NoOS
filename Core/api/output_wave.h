#ifndef __OUTPUT_WAVE_H
#define	__OUTPUT_WAVE_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>
#include "freertos.h"
#include "semphr.h"



#define SYSTEM_CORE_CLOCK_PER_X_US 280 
#ifndef SystemCoreClock
#define SystemCoreClock 280000000
#endif
#define OUTPUT_DELAY_0U1S   ((SystemCoreClock / (1000000 * 10)))
#define OUTPUT_DELAY_1US (SYSTEM_CORE_CLOCK_PER_X_US)
#define OUTPUT_DELAY_1U5S (SYSTEM_CORE_CLOCK_PER_X_US + (SYSTEM_CORE_CLOCK_PER_X_US >> 1))

#define OUTPUT_DELAY_4US (4 * SYSTEM_CORE_CLOCK_PER_X_US)

#define OUTPUT_DELAY_5US (5 * SYSTEM_CORE_CLOCK_PER_X_US)

#define OUTPUT_DELAY_xUS(x) (x * SYSTEM_CORE_CLOCK_PER_X_US)



extern SemaphoreHandle_t g_sem_recvedWaveData;
extern SemaphoreHandle_t g_sem_isSending;
extern void SPI_RecOver(void);
#undef SystemCoreClock
#ifdef __cplusplus
}
#endif

#endif /* __OUTPUT_WAVE_H */

