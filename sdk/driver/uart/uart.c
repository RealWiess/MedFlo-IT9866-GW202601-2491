/** @file
* PAL UART functions.
*
* @author Jim Tan
* @version 1.0
* @date 2011-2012
* @copyright ITE Tech. Inc. All Rights Reserved.
*/
#ifndef _WIN32
#include <errno.h>
#if LWIP_COMPAT_SOCKETS
#include <sys/socket.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart/uart.h"
#include "uart/uart_intr.h"
#include "uart/uart_dma.h"
#include "uart/uart_fifo.h"

static UART_OBJ UART_CFG_UART_OBJ[6] =
{
	INIT_UART_OBJ_DEFAULT(0),
	INIT_UART_OBJ_DEFAULT(1),
	INIT_UART_OBJ_DEFAULT(2),
	INIT_UART_OBJ_DEFAULT(3),
	INIT_UART_OBJ_DEFAULT(4),
	INIT_UART_OBJ_DEFAULT(5),
};

static UART_OBJ UART_STATIC_OBJ[6] = {};

static UART_OBJ* pUartPortObj[6] = { NULL };

static UART_OBJ* pUartDbgObj;
static int UartDbgPutchar(int c);
static void UartSwtichMode(ITHUartPort port, int mode);
static void UartObjDelete(UART_OBJ **pUartObj);

UART_OBJ* iteNewUartObj(ITHUartPort port, UART_OBJ *pUartObj)
{
	UART_OBJ *pObj = &UART_STATIC_OBJ[uart_judge_port(port)];

	pObj->port = pUartObj->port;
	pObj->parity = pUartObj->parity;
	pObj->txPin = pUartObj->txPin;
	pObj->rxPin = pUartObj->rxPin;
	pObj->baud = pUartObj->baud;
	pObj->timeout = pUartObj->timeout;
	pObj->mode = pUartObj->mode;
	pObj->forDbg = pUartObj->forDbg;
	pObj->pMode = pObj;
	pObj->uart_init = NULL;
	pObj->uart_dele = UartObjDelete;
	pObj->uart_send = NULL;
	pObj->uart_read = NULL;

	return pObj;
}

static void UartObjDelete(UART_OBJ **pUartObj)
{
	(*pUartObj)->uart_dele = NULL;
	*pUartObj = NULL;
}

void iteUartInit(ITHUartPort port, UART_OBJ* pUartObj)
{
	int port_num = uart_judge_port(port);
	if (pUartPortObj[port_num] != NULL)
		ithPrintf("\nAlert: UART port %d init more than once.\n", port_num);
	if (pUartObj == NULL)
		pUartObj = &UART_CFG_UART_OBJ[port_num];
	switch (pUartObj->mode)
	{
	case UART_INTR_MODE:
		pUartPortObj[port_num] = iteNewUartIntrObj(port, pUartObj);
		break;
	case UART_DMA_MODE:
		pUartPortObj[port_num] = iteNewUartDmaObj(port, pUartObj);
		break;
	case UART_FIFO_MODE:
		pUartPortObj[port_num] = iteNewUartFifoObj(port, pUartObj);
		break;
	}

	if (pUartPortObj[port_num]->uart_init(pUartPortObj[port_num]))
	{
		ithPrintf("UART port%d init failed!\n", port_num);
		pUartPortObj[port_num]->uart_dele(&pUartPortObj[port_num]);
		return;
	}

	if (pUartPortObj[port_num]->forDbg)
	{
		pUartDbgObj = pUartPortObj[port_num];
		ithPutcharFunc = UartDbgPutchar;
	}
}

void iteUartTerminate(ITHUartPort port)
{
	UART_OBJ *pObj = pUartPortObj[uart_judge_port(port)];
	pObj->uart_dele(&pObj);
}

void iteUartReset(ITHUartPort port, UART_OBJ* pUartObj)
{
	if (pUartPortObj[uart_judge_port(port)] != NULL)
		iteUartTerminate(port);
	iteUartInit(port, pUartObj);
}

void iteUartOpen(ITHUartPort port, ITHUartParity Parity)
{
	/* Wait for removing. */
}

int iteUartWrite(ITHUartPort port, char *ptr, int len)
{
	UART_OBJ *pObj = pUartPortObj[uart_judge_port(port)];

	if (pObj->uart_send)
	{
		return pObj->uart_send(pObj, ptr, len);
	}
	else
	{
		return 0;
	}
}

int iteUartRead(ITHUartPort port, char *ptr, int len)
{
	UART_OBJ *pObj = pUartPortObj[uart_judge_port(port)];
	if (pObj->uart_read)
	{
		return pObj->uart_read(pObj, ptr, len);
	}
	else
	{
		return 0;
	}
}

void iteUartRegisterCallBack(ITHUartPort port, void *ptr)
{
	if(pUartPortObj[uart_judge_port(port)]->mode != UART_INTR_MODE)
	{
		ithPrintf("UART%d not in intr mode, %s failed!\n", uart_judge_port(port), __FUNCTION__);
		return;
	}
	((UART_INTR_OBJ*)(pUartPortObj[uart_judge_port(port)]->pMode))->itpUartDeferIntrHandler = (ITPPendFunction)ptr;
}

void iteUartRegisterDeferCallBack(ITHUartPort port, void *ptr)
{
	if(pUartPortObj[uart_judge_port(port)]->mode != UART_INTR_MODE)
	{
		ithPrintf("UART%d not in intr mode, %s failed!\n", uart_judge_port(port), __FUNCTION__);
		return;
	}
	((UART_INTR_OBJ*)(pUartPortObj[uart_judge_port(port)]->pMode))->UartDeferIntrOn = 1;
	((UART_INTR_OBJ*)(pUartPortObj[uart_judge_port(port)]->pMode))->itpUartDeferIntrHandler = (ITPPendFunction)ptr;
}

void iteUartSetTimeout(ITHUartPort port, uint32_t ptr)
{
	UART_OBJ *pObj = pUartPortObj[uart_judge_port(port)];
	pObj->timeout = ptr;
}

void iteUartSetGpio(ITHUartPort port, ITHUartConfig *pUartConfig)
{
	/* Wait for removing. */
}

void iteUartStopDMA(ITHUartPort port)
{
	UartSwtichMode(port, UART_FIFO_MODE);
}

static int UartDbgPutchar(int c)
{
	while (ithUartIsTxFull(pUartDbgObj->port));
	ithUartPutChar(pUartDbgObj->port, (char)c);
	return c;
}

static void UartSwtichMode(ITHUartPort port, int mode)
{
	UART_STATIC_OBJ[uart_judge_port(port)].mode = mode;

	iteUartReset(port, &UART_STATIC_OBJ[uart_judge_port(port)]);
}
#endif