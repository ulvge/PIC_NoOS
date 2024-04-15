#include "api_utc.h"
#include <string.h>  
#include <stdlib.h>
#include <main.h>
#include "debug_print.h"
// 定义宏
#define PROTOCOL_DATA_ID 0x03
#define PROTOCOL_CMD_CMD 0x0b
#define PROTOCOL1_HEADER_DATA {0x03, 0, 0}

#define SPI_RECV_BUFF_GROUP_COUNT   100
// 定义第一种协议的数据结构
typedef struct {
    uint8_t *wp;
    uint32_t wpGroupIndex;
    uint8_t isRecvedOverflow;
    uint8_t isSending;

    uint16_t count;
    struct {
        uint16_t position;
        uint16_t slope;
    } data[SPI_RECV_BUFF_GROUP_COUNT];
    uint8_t limit[2];
} __attribute__((packed, aligned(2))) ProtocolData;

// 定义第二种协议的数据结构
typedef struct {
    uint8_t *wp;
    uint16_t al;
    uint16_t reSendTimes;
    uint32_t sleepUsWave;
    uint32_t sleepUsGroupData;
    uint8_t limit[2];
    uint8_t isRecvedOverflow;
} ProtocolCmd;

static ProtocolData g_protocolDdata;
static ProtocolCmd g_protocolCmd;

typedef enum {
    RECVEVENT_IDLE = 0,
    RECVEVENT_RECVED_DATA_ID,
    RECVEVENT_RECVED_DATA_HEAD1,
    RECVEVENT_RECVED_DATA_HEAD2,
    RECVEVENT_RECVED_DATA_COUNT_HI,
    RECVEVENT_RECVED_DATA_COUNT_LOW,
    
    RECVEVENT_RECVED_DATA_POSITION_HI,
    RECVEVENT_RECVED_DATA_POSITION_LOW,

    RECVEVENT_RECVED_DATA_SLOPE_HI,
    RECVEVENT_RECVED_DATA_SLOPE_LOW,
    
    RECVEVENT_RECVED_CMD_ID,
    RECVEVENT_RECVED_CMD_RESEND_TIMES_HI,
    RECVEVENT_RECVED_CMD_RESEND_TIMES_LOW,
    
    RECVEVENT_RECVED_CMD_SLEEPUSWAVE_HI,
    RECVEVENT_RECVED_CMD_SLEEPUSWAVE_LOW,

    RECVEVENT_RECVED_CMD_SLEEPUSGROUPDATA_HI,
    RECVEVENT_RECVED_CMD_SLEEPUSGROUPDATA_LOW,
} FSM_RecvEvent;

typedef enum {
    PROTOCOL_TYPE_UNKNOWN= 0,
    PROTOCOL_TYPE_DATA,
    PROTOCOL_TYPE_CMD,
} PROTOCOL_TYPE;

static PROTOCOL_TYPE g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
static FSM_RecvEvent g_FSM_RecvEvent = RECVEVENT_IDLE;

inline static void SPI_ProtocolInit()
{
    g_protocol_type = PROTOCOL_TYPE_UNKNOWN;
    g_FSM_RecvEvent = RECVEVENT_IDLE;
    // memset(&g_protocolDdata, 0, sizeof(g_protocolDdata));
    // memset(&g_protocolCmd, 0, sizeof(g_protocolCmd));

    g_protocolDdata.wpGroupIndex = 0;
    g_protocolDdata.isRecvedOverflow = 0;
    g_protocolDdata.wp = (uint8_t *)&g_protocolDdata.data;
    g_protocolCmd.wp = (uint8_t *)&g_protocolCmd.reSendTimes;
    // g_protocolDdata.limit = (uint8_t *)&g_protocolDdata.data[100];
    // g_protocolCmd.limit = (uint8_t *)&g_protocolCmd.sleepUsGroupData;

}
inline static void SPI_ProtocolError()
{
    SPI_ProtocolInit();
}

static void SPI_DoWork()
{
    if (g_protocolDdata.wpGroupIndex && !g_protocolDdata.isSending)
    {
        //portYIELD_FROM_ISR();
        //sending  now 
    }
}
void SPI_ProtocolParsing(uint8_t val)
{
    if (g_protocol_type == PROTOCOL_TYPE_UNKNOWN) {
        if (val == PROTOCOL_DATA_ID) {
            g_FSM_RecvEvent = RECVEVENT_RECVED_DATA_ID;
            g_protocol_type = PROTOCOL_TYPE_DATA;
        } else if (val == PROTOCOL_CMD_CMD) {
            g_FSM_RecvEvent = RECVEVENT_RECVED_CMD_ID;
            g_protocol_type = PROTOCOL_TYPE_CMD;
        }
    }
    else if (g_protocol_type == PROTOCOL_TYPE_DATA) {
        switch (g_FSM_RecvEvent) {
            case RECVEVENT_RECVED_DATA_ID: 
                if (val == 0) {
                    g_protocolDdata.count = val << 8;
                    g_FSM_RecvEvent++;
                }else{
                    SPI_ProtocolError();
                }
                break;
            case RECVEVENT_RECVED_DATA_HEAD1: 
                if (val == 0) {
                    g_protocolDdata.count |= val;
                    g_FSM_RecvEvent++;
                }else{
                    SPI_ProtocolError();
                }
                break;
            default:
                if (g_protocolDdata.wp < g_protocolDdata.limit){
                    *g_protocolDdata.wp = val;
                    g_protocolDdata.wp++;
                    g_protocolDdata.wpGroupIndex = (g_protocolDdata.wp - (uint8_t *)&g_protocolDdata.data) / 4;
					SPI_DoWork();
                }else{
                    g_protocolDdata.isRecvedOverflow = 1;
                    SPI_ProtocolError();
                }
                break;
        }
    }
    else if (g_protocol_type == PROTOCOL_TYPE_CMD) {
        if (g_protocolCmd.wp < g_protocolCmd.limit){
            *g_protocolCmd.wp = val;
            g_protocolCmd.wp++;
        }else{
			g_protocolCmd.isRecvedOverflow = 1;
			SPI_ProtocolError();
		}
    }
}
