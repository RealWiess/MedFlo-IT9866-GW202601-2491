#ifndef CANBUSLITE_H
#define CANBUSLITE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "arm_lite_dev/armlite_dev_device.h"
#include "can_bus/it9860/can_api.h"


//=============================================================================
//                Constant Definition
//=============================================================================
#define RX_MAX_DATA_BUFFER          120// sw fifo number 120
#define RX_MAX_DATA_BUFFER_SIZE     (12 * 1024)
#define CAN_RX_BUFFER_OFFSET        (540 * 1024)
#define CAN_TX_BUFFER_OFFSET        RISC1_IMAGE_SIZE

#define CAN_LITE_ERROR_REG          USER_DEFINE_REG27
#define CAN_LITE_RX_RD_PTR_REG      USER_DEFINE_REG28
#define CAN_LITE_RX_WR_PTR_REG      USER_DEFINE_REG29

//input command list
#define CAN_LITE_OPEN_ID              1
#define CAN_LITE_CLOSED_ID            2
#define CAN_LITE_READ_ID              3
#define CAN_LITE_WRITE_ID             4
#define CAN_LITE_GETINFO_ID           5


//status bit
#define CAN_LITE_TX_BUSY         (1 << 0x0)
#define CAN_LITE_TX_TIMEOUT      (1 << 0x1)
#define CAN_LITE_TX_FIFO_FULL    (1 << 0x2)
#define CAN_LITE_SW_FIFO_FULL    (1 << 0x3)
#define CAN_LITE_HW_FIFO_FULL    (1 << 0x4)


//engine status
typedef enum {
    CANBUS_UNKNOWN = 0,
	CANBUS_OPEN,
	CANBUS_CLOSE,
} ARMLITE_CANBUS_STATUS;

typedef struct
{
	unsigned short start; //read ptr
	unsigned short end;   //write ptr
	unsigned short count; //current data count
	unsigned int endAddr;
} ARMLITE_RING_BUFFER;

typedef struct
{
	CAN_TXOBJ txObj;
	unsigned char data[DLC_MAX];
} ARMLITE_TX_OBJ;


typedef struct
{
	unsigned char TEC; //Transmit Error Count
	unsigned char REC; // Receive Error Count
	unsigned char KindOfError; //Kind of Error
	unsigned char ErrorStatus; //Active ,Passive ,Bus off
} ARMLITE_CAN_INFO;

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================


//=============================================================================
//                Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif





