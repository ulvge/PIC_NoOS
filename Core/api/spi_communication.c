#include "spi_communication.h"
#include "api_utc.h"
#include "debug_print.h"
#include <main.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "semphr.h"
#include "output_wave.h"
#include "Types.h"

//ProtocolCmd g_protocolCmd __attribute__((at(0x20000000)));
//ProtocolData g_protocolData __attribute__((at(0x20000000 + sizeof(ProtocolCmd))));

ProtocolCmd g_protocolCmd;
ProtocolData g_protocolData;

static BaseType_t xHigherPriorityTaskWoken_YES = pdTRUE;
static BaseType_t xHigherPriorityTaskWoken_NO = pdFALSE;
static PROTOCOL_TYPE g_protocol_type;
static FSM_RecvEvent g_FSM_RecvEvent;


inline static void SPI_ProtocolInit()
{
    g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
    g_FSM_RecvEvent = RECVEVENT_WAIT_DATA_ID;
    memset(&g_protocolData, 0, (uint32_t)(&g_protocolData.data) - (uint32_t)(&g_protocolData));

    memset(&g_protocolCmd, 0, sizeof(g_protocolCmd));
    g_protocolCmd.wp = (uint8_t *)&g_protocolCmd.reSendTimes;
}
inline static void SPI_ProtocolError()
{
    SPI_ProtocolInit();
}

inline static void SPI_SendSem()
{
    if (g_protocolData.isSending || !g_protocolData.recvedGroupCount) {
        return;
    }
    if (xSemaphoreTake(g_sem_isSending, 0) == pdTRUE) {// not sending
        g_protocolData.isSending = true;
        xSemaphoreGiveFromISR(g_sem_isSending, &xHigherPriorityTaskWoken_NO);

        xSemaphoreGiveFromISR(g_sem_recvedWaveData, &xHigherPriorityTaskWoken_YES);
    }
}
void SPI_ProtocolParsing(uint8_t val)
{
    static uint16_t *pSwap;
    if (g_protocol_type == PROTOCOL_TYPE_UNKNOWN) {
        if (val == PROTOCOL_DATA_ID) {
            g_FSM_RecvEvent = RECVEVENT_WAIT_DATA_HEAD1;
            g_protocol_type = PROTOCOL_TYPE_DATA;
        } else if (val == PROTOCOL_CMD_CMD) {
            g_FSM_RecvEvent = RECVEVENT_RECEVED_CMD_ID;
            g_protocol_type = PROTOCOL_TYPE_CMD;
        }
    } else if (g_protocol_type == PROTOCOL_TYPE_DATA) {
        switch (g_FSM_RecvEvent) {
        case RECVEVENT_WAIT_DATA_HEAD1:
            if (val == 0) {
                g_protocolData.count = val << 8;
                g_FSM_RecvEvent++;
            } else {
                SPI_ProtocolError();
            }
            break;
        case RECVEVENT_WAIT_DATA_HEAD2:
            if (val == 0) {
                g_protocolData.count |= val;
                g_FSM_RecvEvent++;
            } else {
                SPI_ProtocolError();
            }
            break;
        default:
            if (g_protocolData.recvedGroupCount < g_protocolData.count) {
                *g_protocolData.wp = val;
                g_protocolData.wp++;
                g_protocolData.writeByteCount++;
                if (g_protocolData.writeByteCount % 4 == 0) {
                    pSwap = (uint16_t *)&g_protocolData.data[g_protocolData.recvedGroupCount];
                    *pSwap = BIG_LITTLE_SWAP16(*pSwap);
                    pSwap++;
                    *pSwap = BIG_LITTLE_SWAP16(*pSwap);
                }
                g_protocolData.recvedGroupCount = g_protocolData.writeByteCount / 4;
                if (g_protocolData.recvedGroupCount == g_protocolData.count) {
                    g_protocolData.isRecvedFinished = 1;
                }
                SPI_SendSem();
            } else {
                g_protocolData.isRecvedOverflow = 1;
                SPI_ProtocolError();
            }
            break;
        }
    } else if (g_protocol_type == PROTOCOL_TYPE_CMD) {
        if (g_protocolCmd.writeByteCount < PROTOCOL_CMD_FILED_LEN) {
            *g_protocolCmd.wp = val;
            g_protocolCmd.wp++;
            g_protocolCmd.writeByteCount++;
            if (g_protocolCmd.writeByteCount == PROTOCOL_CMD_FILED_LEN) {
                g_protocolCmd.isRecvedFinished = 1;
                g_protocolCmd.reSendTimes = BIG_LITTLE_SWAP16(g_protocolCmd.reSendTimes);
            }
        } else {
            SPI_ProtocolError();
        }
    }
}
