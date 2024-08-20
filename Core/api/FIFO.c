/****************************************Copyright (c)****************************************************

*********************************************************************************************************/
#define FIFO_GLOBALS 
#include "FIFO.h"   
#include "stdint.h" 

/************************************************************************************************************
   FIFO  底层接口层
******************************************************************/
void FIFO_Init(FIFO *fifo, INT8U *array, INT16U deepth)
{
    fifo->deepth    = deepth;
    fifo->occupy    = 0;
    fifo->array     = array;
    fifo->limit     = array + deepth;
    fifo->wp = fifo->array;
    fifo->rp = fifo->array;
}

// void FIFO_Reset(FIFO *fifo)
// {
//     fifo->occupy = 0;
//     fifo->rp = fifo->array;
//     fifo->wp = fifo->array;
// }
/**
 * @brief 向FIFO写入数据
 * @param fifo FIFO结构体指针
 * @param data 要写入的数据
 * @return 成功返回true，否则返回false
 */
BOOLEAN FIFO_Write(FIFO *fifo, INT8U data)
{
    if (fifo->occupy >= fifo->deepth) {
		return FALSE;
    }
    uint32_t x=API_EnterCirtical();
    *fifo->wp++ = data;
    if (fifo->wp >= fifo->limit) {
		fifo->wp = fifo->array;
	}
    fifo->occupy++;
    API_ExitCirtical(x);
    return TRUE;
}
/**
 * @brief 向FIFO写入多个数据
 * @param fifo FIFO结构体指针
 * @param data 要写入的数据指针
 * @param dataSize 要写入的数据数量
 * @return 成功返回true，否则返回false
 */
BOOLEAN FIFO_Writes(FIFO *fifo, INT8U *data, INT16U dataSize)
{
    if (dataSize > (fifo->deepth - fifo->occupy)) {
		return FALSE;
	}
    uint32_t x=API_EnterCirtical();
    for (; dataSize > 0; dataSize--) { 
       *fifo->wp++ = *data++;
       if (fifo->wp >= fifo->limit){
		   fifo->wp = fifo->array;
	   }
       fifo->occupy++;
    }
    API_ExitCirtical(x);
    return TRUE;
}
/**
 * @brief 检查FIFO是否为空
 * @param fifo FIFO结构体指针
 * @return 如果FIFO为空，返回true，否则返回false
 */
inline  BOOLEAN FIFO_Empty(FIFO *fifo)
{
    if (fifo->occupy == 0){
		return true;
	} else {
		return false;
	}
}
 /**
 * @brief 从FIFO读取数据
 * @param fifo FIFO结构体指针
 * @param data 用于存储数据的指针
 * @return 如果FIFO不为空，返回true，否则返回false
 */
BOOLEAN FIFO_Read(FIFO *fifo, INT8U *data)
{
    if (fifo->occupy == 0) {
		return false;
	}
    uint32_t x=API_EnterCirtical();
    *data = *fifo->rp++;
    if (fifo->rp >= fifo->limit) {
		fifo->rp = fifo->array;
	}
    fifo->occupy--;
    API_ExitCirtical(x);
    return true;
}
/**
 * @brief 从FIFO读取多个数据
 * @param fifo FIFO结构体指针
 * @param data 用于存储数据的指针
 * @param dataSize 要读取的数据数量
 * @param readLen 实际读取到的数据数量
 * @return 如果FIFO不为空，返回true，否则返回false
 */
BOOLEAN FIFO_ReadN(FIFO *fifo, INT8U *data, INT16U dataSize, INT16U *readLen)
{
    if (fifo->occupy == 0) {
		return false;
	}
    uint32_t x=API_EnterCirtical();
    if (dataSize < fifo->occupy) {
        *readLen = dataSize;
    } else {
        *readLen = fifo->occupy;
    }
    INT16U tmpLen = *readLen;
    while (tmpLen--) {
        *data++ = *fifo->rp++;
        if (fifo->rp >= fifo->limit) {
            fifo->rp = fifo->array;
        }
    }
    fifo->occupy -= *readLen;
    API_ExitCirtical(x);
    return true;
}

inline uint32_t API_EnterCirtical( void )
{
    uint32_t primask_bit = __get_PRIMASK();
    __set_PRIMASK(1);
    return primask_bit;
}
/*-----------------------------------------------------------*/

inline void API_ExitCirtical( uint32_t x)
{
    __set_PRIMASK(x);
}


