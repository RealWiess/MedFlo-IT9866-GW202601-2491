#ifndef SW_UART_H
#define SW_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "alt_cpu/alt_cpu_device.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define MAX_CMD_DATA_BUFFER_SIZE    640
#define CMD_DATA_BUFFER_OFFSET      (16 * 1024 - MAX_CMD_DATA_BUFFER_SIZE)

#define MAX_UART_BUFFER_SIZE 		500

#define INIT_CMD_ID                 1
#define WRITE_DATA_CMD_ID			2
#define READ_DATA_CMD_ID			3

#define ITP_IOCTL_INIT_DBG_UART		                 ITP_IOCTL_CUSTOM_CTL_ID1
#define ITP_IOCTL_INIT_UART_PARAM	     			 ITP_IOCTL_CUSTOM_CTL_ID2

enum UartParity
{
	NONE,
	ODD,
	EVEN,
};


//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct
{
	unsigned long cpuClock;
    unsigned long baudrate;
    unsigned long uartTxGpio;
	unsigned long uartRxGpio;
	unsigned long parity;
} SW_UART_INIT_DATA;

typedef struct
{
	volatile int len;
	uint8_t pWriteBuffer[MAX_UART_BUFFER_SIZE];
    uint8_t *pWriteDataBuffer;
} SW_UART_WRITE_DATA;

typedef struct
{
	volatile int len;
	uint8_t pReadBuffer[MAX_UART_BUFFER_SIZE];
    uint8_t *pReadDataBuffer;
} SW_UART_READ_DATA;



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





