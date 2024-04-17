#ifndef __OUTPUT_WAVE_H
#define	__OUTPUT_WAVE_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "semphr.h"

extern SemaphoreHandle_t g_sem_recvedWaveData;


#ifdef __cplusplus
}
#endif

#endif /* __OUTPUT_WAVE_H */

