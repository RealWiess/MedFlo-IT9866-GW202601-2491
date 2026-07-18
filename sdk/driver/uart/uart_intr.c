#ifndef _WIN32
#include <stdlib.h>
#include <stdio.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart_intr.h"

pthread_mutex_t _mutexRead = PTHREAD_MUTEX_INITIALIZER;

static ITHIntr _UartIthIntrBase[6] =
{
    ITH_INTR_UART0,
    ITH_INTR_UART1,
    ITH_INTR_UART2,
    ITH_INTR_UART3,
    ITH_INTR_UART4,
    ITH_INTR_UART5,
};

static uint8_t _UartFifoDepth[6] =
{
    128, 16, 16, 16, 8, 8,
};

static uint8_t _UartTxBuff[6][128] = { 0 };

static UART_INTR_OBJ UART_STATIC_INTR_OBJ[6] = {};

static int UartIntrObjInit(UART_OBJ *pUartObj);
static int UartIntrObjSend(UART_OBJ *pUartObj, char *ptr, int len);
static int UartIntrObjRead(UART_OBJ *pUartObj, char *ptr, int len);
static void UartIntrObjTerminate(UART_OBJ **pUartObj);

UART_OBJ* iteNewUartIntrObj(ITHUartPort port, UART_OBJ *pUartObj)
{
    UART_OBJ* pObj = iteNewUartObj(port, pUartObj);
    if (pObj == NULL)
        return NULL;

    UART_INTR_OBJ* pUartIntrObj = &UART_STATIC_INTR_OBJ[uart_judge_port(port)];

    pObj->pMode = pUartIntrObj;

    /*initialising pMode class members*/
    pUartIntrObj->Intr = _UartIthIntrBase[uart_judge_port(pObj->port)];

    pUartIntrObj->xRxStrBuf = NULL;
    pUartIntrObj->xTxStrBuf = NULL;

    pUartIntrObj->uartFifoDepth = _UartFifoDepth[uart_judge_port(pObj->port)];
    pUartIntrObj->UartTxBuff = _UartTxBuff[uart_judge_port(pObj->port)];

    pUartIntrObj->UartDeferIntrOn = 0;
    pUartIntrObj->itpUartDeferIntrHandler = NULL;

    /*Changing base class interface to access pMode class functions*/
    pObj->uart_init = UartIntrObjInit;
    pObj->uart_send = UartIntrObjSend;
    pObj->uart_read = UartIntrObjRead;
    pObj->uart_dele = UartIntrObjTerminate;

    return pObj;
}

static void _UartDefaultIntrHandler(void* arg1, uint32_t arg2)
{
    /* DO NOTHING*/
}

static void _UartIntrHandler(void *arg)
{
    UART_OBJ* pUartObj = (UART_OBJ*)arg;
    UART_INTR_OBJ* pUartIntrObj = pUartObj->pMode;

    uint32_t status = ithUartClearIntr(pUartObj->port) & 0xF;
    signed char cChar = 0;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    uint8_t i, txCount = 0;

    if(pUartObj->parity)
    {
        if(ithUartIsRxParityError(pUartObj->port))
            ithPrintf("UART%d: parity error!\n",uart_judge_port(pUartObj->port));
        if (ithUartIsRxDataError(pUartObj->port))
            ithPrintf("UART%d: parity or framing error or break indication active!\n",uart_judge_port(pUartObj->port));
    }
    switch (status)
    {
    case UART_INTR_CRT:
        ithPrintf("UART%d timeout clear fifo\n", uart_judge_port(pUartObj->port));
        while(ithUartIsRxReady(pUartObj->port))
        {
            ithUartGetChar(pUartObj->port);
        }
        ithUartGetChar(pUartObj->port); // read again to clear UART INTR
        break;

    case UART_INTR_RDR:
        while(ithUartIsRxReady(pUartObj->port))
        {
            cChar = ithUartGetChar(pUartObj->port);

            if (xStreamBufferSendFromISR(pUartIntrObj->xRxStrBuf, &cChar, 1, &xHigherPriorityTaskWoken) == pdFALSE)
            {
                ithPrintf("UART%d: rx queue full!\n", uart_judge_port(pUartObj->port));
            }
            if (pUartIntrObj->UartDeferIntrOn)
                itpPendFunctionCallFromISR(pUartIntrObj->itpUartDeferIntrHandler, NULL, 0);
            else
            {
                if(pUartIntrObj->itpUartDeferIntrHandler)
                    pUartIntrObj->itpUartDeferIntrHandler(NULL, 0);
            }
        }
        break;

    case UART_INTR_THE:
        txCount = xStreamBufferReceiveFromISR(pUartIntrObj->xTxStrBuf, pUartIntrObj->UartTxBuff, pUartIntrObj->uartFifoDepth, &xHigherPriorityTaskWoken);
        if(txCount < pUartIntrObj->uartFifoDepth || xStreamBufferBytesAvailable(pUartIntrObj->xTxStrBuf) == 0)
        {
            ithUartDisableIntr(pUartObj->port, ITH_UART_TX_READY);
        }
        for(i = 0; i < txCount; i++)
        {
            ithUartPutChar(pUartObj->port, pUartIntrObj->UartTxBuff[i]);
        }
        break;
    
    default:
        break;
    }
}

static int UartIntrObjInit(UART_OBJ *pUartObj)
{
    UART_INTR_OBJ* pUartIntrObj = pUartObj->pMode;

    /* Set the required protocol. */
    ithUartReset(pUartObj->port, pUartObj->baud, pUartObj->parity, 1, 8);

    ithUartSetMode(pUartObj->port, ITH_UART_DEFAULT, pUartObj->txPin, pUartObj->rxPin);

    if (!pUartIntrObj->xTxStrBuf && !pUartIntrObj->xRxStrBuf)
    {
        pUartIntrObj->xRxStrBuf = xStreamBufferCreate(UART_INTR_BUF_SIZE, 1);
        pUartIntrObj->xTxStrBuf = xStreamBufferCreate(UART_INTR_BUF_SIZE, 1);
    }

    pUartIntrObj->UartDeferIntrOn = 0;
    pUartIntrObj->itpUartDeferIntrHandler = _UartDefaultIntrHandler;

    ithEnterCritical();
    /* Enable the Rx interrupts.  The Tx interrupts are not enabled
    until there are characters to be transmitted. */
    ithIntrDisableIrq(pUartIntrObj->Intr);
    ithUartClearIntr(pUartObj->port);
    ithIntrClearIrq(pUartIntrObj->Intr);

    ithIntrSetTriggerModeIrq(pUartIntrObj->Intr, ITH_INTR_LEVEL);
    ithIntrRegisterHandlerIrq(pUartIntrObj->Intr, _UartIntrHandler, (void *)pUartObj);
    ithUartEnableIntr(pUartObj->port, ITH_UART_RX_READY);

    /* Enable the interrupts. */
    ithIntrEnableIrq(pUartIntrObj->Intr);
    ithExitCritical();

    return 0;
}

static int UartIntrObjSend(UART_OBJ *pUartObj, char *ptr, int len)
{
    if(len > UART_INTR_BUF_SIZE || len == 0)
    {
        return 0;
    }
    UART_INTR_OBJ* pUartIntrObj = pUartObj->pMode;
    int count = 0;
    ithEnterCritical();
    count = xStreamBufferSend(pUartIntrObj->xTxStrBuf, ptr, len, 0);
    if(count > 0)
    {
        ithUartEnableIntr(pUartObj->port, ITH_UART_TX_READY);
    }
    ithExitCritical();
    return count;
}

static int UartIntrObjRead(UART_OBJ *pUartObj, char *ptr, int len)
{
    if(len > UART_INTR_BUF_SIZE || len == 0)
    {
        return 0;
    }
    UART_INTR_OBJ* pUartIntrObj = pUartObj->pMode;
    int count = 0;
    pthread_mutex_lock(&_mutexRead);
    count = xStreamBufferReceive(pUartIntrObj->xRxStrBuf, ptr, len, 0);
    pthread_mutex_unlock(&_mutexRead);
    return count;
}

static void UartIntrObjTerminate(UART_OBJ **pUartObj)
{
    UART_INTR_OBJ* pUartIntrObj = (*pUartObj)->pMode;

    ithUartClearMode((*pUartObj)->port, ITH_UART_DEFAULT, (*pUartObj)->txPin, (*pUartObj)->rxPin);
    if (pUartIntrObj->xTxStrBuf && pUartIntrObj->xRxStrBuf)
    {
        /* Delete the queues used to hold Rx and Tx characters. */
        vStreamBufferDelete(pUartIntrObj->xRxStrBuf);
        vStreamBufferDelete(pUartIntrObj->xTxStrBuf);
    }

    pUartIntrObj->UartDeferIntrOn = 0;
    pUartIntrObj->itpUartDeferIntrHandler = NULL;

    ithEnterCritical();

    ithIntrDisableIrq(pUartIntrObj->Intr);
    ithUartClearIntr((*pUartObj)->port);
    ithIntrClearIrq(pUartIntrObj->Intr);

    ithExitCritical();

    pUartIntrObj->Intr = -1;

    pUartIntrObj->xRxStrBuf = NULL;
    pUartIntrObj->xTxStrBuf = NULL;

    pUartIntrObj->UartDeferIntrOn = 0;
    pUartIntrObj->uartFifoDepth = 0;
    pUartIntrObj->UartTxBuff = NULL;
    pUartIntrObj->itpUartDeferIntrHandler = NULL;

    (*pUartObj)->uart_init = NULL;
    (*pUartObj)->uart_dele = NULL;
    (*pUartObj)->uart_send = NULL;
    (*pUartObj)->uart_read = NULL;

    *pUartObj = NULL;
}
#endif
