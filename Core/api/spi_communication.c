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


//ProtocolCmd g_protocolCmd __attribute__((at(0x20000000)));
//ProtocolData g_protocolData __attribute__((at(0x20000000 + sizeof(ProtocolCmd))));

SemaphoreHandle_t g_sem_WriteBack;

ProtocolCmd g_protocolCmd;
ProtocolData g_protocolData;
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
            break;
        case RECVEVENT_WAIT_DATA_COUNT_LOW:
            g_protocolWriteBack.count |= val;
            g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
            return true;
        default:
            SPI_ProtocolError();
            break;
    }
    return false;
}

void SPI_ProtocolParsing(uint8_t val)
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
            //xSemaphoreGiveFromISR(g_sem_WriteBack, &xHigherPriorityTaskWoken_NO);
        }
    } else if (g_protocol_type == PROTOCOL_TYPE_CMD) {
        if (g_protocolCmd.writeByteCount < PROTOCOL_CMD_FILED_LEN) {
            *g_protocolCmd.wp = val;
            g_protocolCmd.wp++;
            g_protocolCmd.writeByteCount++;
            if (g_protocolCmd.writeByteCount == PROTOCOL_CMD_FILED_LEN) {
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
typedef struct {
    uint16_t position;
    uint16_t slope;
} protocolData __attribute__((aligned(2)));
protocolData g_protocolDataBak[3];// __attribute__((section(".MY_SECTION")));;
static void SPI_swapEndianness(uint16_t *srcArray, uint16_t *destArray, int size) {
    uint16_t value;
    for (int i = 0; i < size; i++) {
        value = srcArray[i];
        destArray[i] = (value >> 8) | (value << 8);
    }
}
void Task_WriteBack(void *argument)
{
    g_sem_WriteBack = xSemaphoreCreateBinary();

    while (1) {
        if (xSemaphoreTake(g_sem_WriteBack, portMAX_DELAY) == pdTRUE)
        {
            SPI_writeBack();	
		}
    }
}

void SPI_writeBack(void)
{
    //static uint8_t unused = 0;
    if (g_protocolData.recvedGroupCount > SPI_RECV_BUFF_GROUP_COUNT){
        g_protocolData.recvedGroupCount = SPI_RECV_BUFF_GROUP_COUNT;
    }
    SPI_swapEndianness((uint16_t *)g_protocolData.data, (uint16_t *)g_protocolDataBak, g_protocolData.recvedGroupCount * 2);

    //HAL_SPI_Transmit_DMA(&g_hspi1, (uint8_t *)&g_protocolDataBak, g_protocolData.recvedGroupCount * 4);
    do{
        //HAL_SPI_Abort(&g_hspi1);
        HAL_StatusTypeDef st = HAL_SPI_Transmit(&g_hspi1, (uint8_t *)g_protocolDataBak, g_protocolData.recvedGroupCount * 4, 3000);
        //HAL_StatusTypeDef st = HAL_SPI_Transmit_IT(&g_hspi1, (uint8_t *)g_protocolDataBak, g_protocolData.recvedGroupCount * 4, 3000);
        //SPI1_startReceviceIT();
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
    } while (0);
    
    SPI_RecOver();
}

