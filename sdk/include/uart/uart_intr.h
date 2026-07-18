#include "uart/uart.h"

/* User could change Interrupt buffer size by editing project Kconfig */
#ifdef CFG_UART_INTR_BUF_SIZE 
#define UART_INTR_BUF_SIZE CFG_UART_INTR_BUF_SIZE 
#else
#define UART_INTR_BUF_SIZE 16384
#endif

#define UART_INTR_CRT (0x0C) // Character Reception Timeout
#define UART_INTR_LSR (0x06) // Line Status Register
#define UART_INTR_RDR (0x04) // Receive Data Ready
#define UART_INTR_THE (0x02) // Transmit Holding Empty

typedef struct _UART_INTR_OBJ UART_INTR_OBJ;

typedef struct _UART_INTR_OBJ
{
	UART_OBJ* pBaseObj;
	ITHIntr Intr;
	StreamBufferHandle_t xRxStrBuf;
	StreamBufferHandle_t xTxStrBuf;
	uint8_t UartDeferIntrOn;
	uint8_t	uartFifoDepth;
	uint8_t *UartTxBuff;
	ITPPendFunction	itpUartDeferIntrHandler;
}UART_INTR_OBJ;

UART_OBJ* iteNewUartIntrObj(ITHUartPort port, UART_OBJ *uart_obj);
