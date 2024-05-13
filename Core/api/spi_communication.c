#include "spi_communication.h"
#include "api_utc.h"
#include "debug_print.h"
#include <main.h>
#include <stdlib.h>
#include <string.h>
#include "output_wave.h"
#include "Types.h"
#include "bsp_uartcomm.h"
#include "bsp_spi1_slave.h"


ProtocolCmd g_protocolCmd;
#if DATA_BUFF_IN_128K == 1
ProtocolData g_protocolData;
#else
ProtocolData g_protocolData __attribute__((section(".MY_SECTION")));
#endif
ProtocolData g_protocolDataOri __attribute__((section(".MY_SECTION")));
ProtocolWriteBack g_protocolWriteBack;

static PROTOCOL_TYPE g_protocol_type;
static FSM_RecvEvent g_FSM_RecvEvent;
static void SPI_ProtocolReset(PROTOCOL_TYPE type);
static void SPI_writeBack(void);
extern SPI_HandleTypeDef g_hspi1;

void SPI_ProtocolInit(void)
{
    SPI_ProtocolReset(PROTOCOL_TYPE_DATA);
    SPI_ProtocolReset(PROTOCOL_TYPE_DATA_WRITEBACK);
    SPI_ProtocolReset(PROTOCOL_TYPE_CMD);
}

inline static void SPI_ProtocolReset(PROTOCOL_TYPE type)
{
    switch (type)
    {
        case PROTOCOL_TYPE_DATA:
            memset(&g_protocolData, 0, (uint32_t)(&g_protocolData.data) - (uint32_t)(&g_protocolData));
            g_protocolData.wp = (uint8_t *)&g_protocolData.data[0];

            g_protocolDataOri.wp = (uint8_t *)&g_protocolDataOri.data[0];
            break;
        case PROTOCOL_TYPE_DATA_WRITEBACK:
            memset(&g_protocolWriteBack, 0, sizeof(g_protocolWriteBack));
            break;
        case PROTOCOL_TYPE_CMD:
            memset(&g_protocolCmd, 0, sizeof(g_protocolCmd));
            g_protocolCmd.wp = (uint8_t *)&g_protocolCmd.reSendTimes;
            break;
        default:
            break;
    }
    g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
}

inline static void SPI_ProtocolError()
{
    SPI_ProtocolReset(g_protocol_type);
}

inline static void SPI_SendSemOutputWave()
{
    if (g_protocolData.isSending || !g_protocolData.recvedGroupCount) {
        return;
    }
    if (xSemaphoreTakeFromISR(g_sem_isSending, 0) == pdTRUE) {// not sending
        g_protocolData.isSending = true;
        xSemaphoreGiveFromISR(g_sem_isSending, &xHigherPriorityTaskWoken_NO);

        xSemaphoreGiveFromISR(g_sem_recvedWaveData, &xHigherPriorityTaskWoken_YES);
    }
}
inline static bool SPI_decode_TYPE_DATA(uint8_t val){
    static uint16_t *pSwap;
    switch (g_FSM_RecvEvent) {
        case RECVEVENT_WAIT_DATA_HEAD1:
        case RECVEVENT_WAIT_DATA_HEAD2:
            if (val == 0) {
                g_FSM_RecvEvent++;
            } else {
                SPI_ProtocolError();
            }
            break;

        case RECVEVENT_WAIT_DATA_COUNT_HI:
            g_protocolData.count = (val << 8);
            g_FSM_RecvEvent++;
            break;
        case RECVEVENT_WAIT_DATA_COUNT_LOW:
            g_protocolData.count |= val;
            g_FSM_RecvEvent++;
            break;
        case RECVEVENT_WAIT_DATA_DATA:
            if (g_protocolData.recvedGroupCount < g_protocolData.count) {
                *g_protocolDataOri.wp++ = val;
                *g_protocolData.wp = val;
                g_protocolData.wp++;
                g_protocolData.recvedByteCount++;
                if (g_protocolData.recvedByteCount % 4 == 0) {
                    pSwap = (uint16_t *)&g_protocolData.data[g_protocolData.recvedGroupCount];
                    *pSwap = BIG_LITTLE_SWAP16(*pSwap);
                    pSwap++;
                    *pSwap = BIG_LITTLE_SWAP16(*pSwap);
                }
                g_protocolData.recvedGroupCount = g_protocolData.recvedByteCount / 4;
                if (g_protocolData.recvedGroupCount == g_protocolData.count) {
                    g_protocolData.isRecvedFinished = 1;
                    g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
					return true;
                }
            }
            break;
        default:
        SPI_ProtocolError();
        break;
    }
    return false;
}

inline static bool SPI_decode_TYPE_WRITEBACK(uint8_t val){
    switch (g_FSM_RecvEvent) {
        case RECVEVENT_WAIT_DATA_HEAD1:
        case RECVEVENT_WAIT_DATA_HEAD2:
            if (val == 0) {
                g_FSM_RecvEvent++;
            } else {
                SPI_ProtocolError();
            }
            break;
        case RECVEVENT_WAIT_DATA_COUNT_HI:
            g_protocolWriteBack.count = val << 8;
            g_FSM_RecvEvent++;
            return true;
        case RECVEVENT_WAIT_DATA_COUNT_LOW:
            g_protocolWriteBack.count |= val;
            g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
            break;
        default:
            SPI_ProtocolError();
            break;
    }
    return false;
}

inline void SPI_ProtocolParsing(uint8_t val)
{
    //UART_sendByte(DEBUG_UART_PERIPH, val);
    if (g_protocol_type == PROTOCOL_TYPE_UNKNOWN) {
        switch (val)
        {
        case PROTOCOL_DATA_ID:
			SPI_ProtocolReset(PROTOCOL_TYPE_DATA);
            g_FSM_RecvEvent = RECVEVENT_WAIT_DATA_HEAD1;
            g_protocol_type = PROTOCOL_TYPE_DATA;
            break;
        case PROTOCOL_DATA_WRITEBACK:
			SPI_ProtocolReset(PROTOCOL_TYPE_DATA_WRITEBACK);
            g_FSM_RecvEvent = RECVEVENT_WAIT_DATA_HEAD1;
            g_protocol_type = PROTOCOL_TYPE_DATA_WRITEBACK;
            break;
        case PROTOCOL_CMD_CMD:
			SPI_ProtocolReset(PROTOCOL_TYPE_CMD);
            g_FSM_RecvEvent = RECVEVENT_RECEVED_CMD_ID;
            g_protocol_type = PROTOCOL_TYPE_CMD;
            break;
        default:
            break;
        }
    } else if (g_protocol_type == PROTOCOL_TYPE_DATA) {
        if (SPI_decode_TYPE_DATA(val)){
            SPI_RecOver();
        }
    } else if (g_protocol_type == PROTOCOL_TYPE_DATA_WRITEBACK) {
        if(SPI_decode_TYPE_WRITEBACK(val)){
            SPI_writeBack();
        } 
    } else if (g_protocol_type == PROTOCOL_TYPE_CMD) {
        if (g_protocolCmd.recvedByteCount < PROTOCOL_CMD_FILED_LEN) {
            *g_protocolCmd.wp = val;
            g_protocolCmd.wp++;
            g_protocolCmd.recvedByteCount++;
            if (g_protocolCmd.recvedByteCount == PROTOCOL_CMD_FILED_LEN) {
                g_protocolCmd.isRecvedFinished = 1;
                g_protocolCmd.reSendTimes = BIG_LITTLE_SWAP16(g_protocolCmd.reSendTimes);
                g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
                SPI_SendSemOutputWave();
            }
        } else {
            SPI_ProtocolError();
        }
    }
}

void SPI_writeBack(void)
{
    HAL_StatusTypeDef st = HAL_SPI_Transmit_DMA(&g_hspi1, (uint8_t *)&g_protocolDataOri.data, g_protocolData.recvedGroupCount * 4);
    switch (st) {
        case HAL_OK:
            break;
        case HAL_TIMEOUT:
            break;
        case HAL_ERROR:
            break;
        default:
            break;
    }
    
    SPI_RecOver();
}

