#ifndef __SPI_COMMUNICATION_H
#define	__SPI_COMMUNICATION_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>


// 定义宏
#define PROTOCOL_DATA_ID 0x03
#define PROTOCOL_CMD_CMD 0x0b

#define SPI_RECV_BUFF_GROUP_COUNT 100     // 128K 32000
#define PROTOCOL_CMD_FILED_LEN 4


typedef enum {
    RECVEVENT_WAIT_DATA_ID = 0,
    RECVEVENT_WAIT_DATA_HEAD1,
    RECVEVENT_WAIT_DATA_HEAD2,
    RECVEVENT_WAIT_DATA_DATA,

    RECVEVENT_RECEVED_CMD_ID,
} FSM_RecvEvent;

typedef enum {
    PROTOCOL_TYPE_UNKNOWN = 0,
    PROTOCOL_TYPE_DATA,
    PROTOCOL_TYPE_CMD,
} PROTOCOL_TYPE;


typedef struct {
    uint8_t *wp;
    uint32_t recvedGroupCount;
    uint32_t writeByteCount;
    uint8_t isRecvedFinished;
    uint8_t isRecvedOverflow;
    uint8_t isSending;

    uint16_t count __attribute__((aligned(2)));
    struct {
        uint16_t position;
        uint16_t slope;
    } data[SPI_RECV_BUFF_GROUP_COUNT] __attribute__((aligned(2)));
} ProtocolData;

// 定义第二种协议的数据结构
typedef struct {
    uint8_t *wp;
    uint16_t writeByteCount;

    uint16_t reSendTimes __attribute__((aligned(2)));
    uint8_t sleepUsWave;
    uint8_t sleepUsGroupData;
    uint8_t isRecvedFinished;
}__attribute__ ((packed)) ProtocolCmd ;


extern ProtocolData g_protocolData;
extern ProtocolCmd g_protocolCmd;

void SPI_ProtocolParsing(uint8_t val);
#ifdef __cplusplus
}
#endif

#endif /* __SPI_COMMUNICATION_H */

