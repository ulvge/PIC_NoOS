#ifndef __UARTMONITOR_H
#define	__UARTMONITOR_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "semphr.h"

extern void uart_PostdMsg(bool isReSend);
extern void Task_uartMonitor(void *param);

#ifdef __cplusplus
}
#endif

#endif /* __UARTMONITOR_H */

