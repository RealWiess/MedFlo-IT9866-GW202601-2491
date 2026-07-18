#ifndef _WIN32
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart_dma.h"

static ITHIntr uartIthIntrBase[6] =
{
    ITH_INTR_UART0,
    ITH_INTR_UART1,
    ITH_INTR_UART2,
    ITH_INTR_UART3,
    ITH_INTR_UART4,
    ITH_INTR_UART5,
};

static void
uartIntrHandler (
    void * arg)
{
    UART_OBJ    * pUartObj  = (UART_OBJ *)arg;
    uint32_t    err_status  = ithReadRegA((uint32_t)pUartObj->port + ITH_UART_LSR_REG);

    (void)ithPrintf("irq: 0x%X\n", err_status);
    if ((err_status & ITH_UART_LSR_OVERRUN) != 0U)
    {
        (void)ithPrintf("OVERRUN ERR at 0x%X\n", pUartObj->port);
    }
    if ((err_status & ITH_UART_LSR_PARITY) != 0U)
    {
        (void)ithPrintf("PARITY ERR at 0x%X\n", pUartObj->port);
    }
    if ((err_status & ITH_UART_LSR_FRAMING) != 0U)
    {
        (void)ithPrintf("FRAMING ERR at 0x%X\n", pUartObj->port);
    }
    if ((err_status & ITH_UART_LSR_DATA_ERR) != 0U)
    {
        (void)ithPrintf("DATA ERR at 0x%X\n", pUartObj->port);
    }

    ithWriteRegA((uint32_t)pUartObj->port + ITH_UART_FCR_REG, (0x21U | (0x1U << ITH_UART_FCR_RXFIFO_RESET_BIT)));
}

static void
dma_write_isr (
    int         ch,
    void        * arg,
    uint32_t    int_status)
{
    sem_t * sem = (sem_t *)arg;

    if ((int_status & (uint32_t)ITH_DMA_INTS_ERR) != 0U)
    {
        (void)ithPrintf(" dma ch%d error \n", ch);
    }
    if ((int_status & (uint32_t)ITH_DMA_INTS_WDT) != 0U)
    {
        (void)ithPrintf(" dma ch%d watchdog timeout \n", ch);
    }
    if ((int_status & (uint32_t)ITH_DMA_INTS_ABT) != 0U)
    {
        (void)ithPrintf(" dma ch%d abort \n", ch);
    }

    itpSemPostFromISR(sem);
}

static UART_DMA_BASE UartDmaChBase[6] =
{
    UART_DMA_CH_BASE(0),
    UART_DMA_CH_BASE(1),
    UART_DMA_CH_BASE(2),
    UART_DMA_CH_BASE(3),
    UART_DMA_CH_BASE(4),
    UART_DMA_CH_BASE(5),
};

static UART_DMA_OBJ UART_STATIC_DMA_OBJ[6] = {};

static int
UartDmaObjInit (
    UART_OBJ * pUartObj);
static int
UartDmaObjSend (
    UART_OBJ    * pUartObj,
    char        * ptr,
    int         len);
static int
UartDmaObjRead (
    UART_OBJ    * pUartObj,
    char        * ptr,
    int         len);
static void
UartDmaObjTerminate (
    UART_OBJ ** pUartObj);

UART_OBJ *
iteNewUartDmaObj (
    ITHUartPort port,
    UART_OBJ    * pUartObj)
{
    UART_OBJ * pObj = iteNewUartObj(port, pUartObj);
    if (pObj == NULL)
    {
        return NULL;
    }

    UART_DMA_OBJ * pUartDmaObj = &UART_STATIC_DMA_OBJ[uart_judge_port((uint32_t)port)];

    pObj->pMode = pUartDmaObj;

    /* initialising pMode class members*/
    int prot_num = (int)uart_judge_port((uint32_t)pObj->port);
    pUartDmaObj->pRxChName  = UartDmaChBase[prot_num].pRxChName;
    pUartDmaObj->pTxChName  = UartDmaChBase[prot_num].pTxChName;
    pUartDmaObj->txReqNum   = (int32_t)UartDmaChBase[prot_num].txReqNum;
    pUartDmaObj->rxReqNum   = (int32_t)UartDmaChBase[prot_num].rxReqNum;
    pUartDmaObj->rxChNum    = 0;
    pUartDmaObj->txChNum    = 0;
    pUartDmaObj->pDmaSrc    = NULL;
    pUartDmaObj->pDmaBuf    = NULL;
    pUartDmaObj->wtIndex    = 0;
    pUartDmaObj->rdIndex    = 0;
    pUartDmaObj->LLPCtxt    = NULL;

    /* Changing base class interface to access pMode class functions*/
    pObj->uart_init              = &UartDmaObjInit;
    pObj->uart_send              = &UartDmaObjSend;
    pObj->uart_read              = &UartDmaObjRead;
    pObj->uart_dele              = &UartDmaObjTerminate;

    return pObj;
}

static int
UartDmaObjInit (
    UART_OBJ * pUartObj)
{
    UART_DMA_OBJ * pUartDmaObj = pUartObj->pMode;

    /* Set the required protocol. */
    ithUartReset(pUartObj->port, (uint32_t)pUartObj->baud, pUartObj->parity, 1, 8);

    ithUartSetMode(pUartObj->port, ITH_UART_DEFAULT, pUartObj->txPin, pUartObj->rxPin);

    ithEnterCritical();
    /* Enable the Rx interrupts.  The Tx interrupts are not enabled
    until there are characters to be transmitted. */
    ithIntrDisableIrq(uartIthIntrBase[uart_judge_port((uint32_t)pUartObj->port)]);
    (void)ithUartClearIntr(pUartObj->port);
    ithIntrClearIrq(uartIthIntrBase[uart_judge_port((uint32_t)pUartObj->port)]);

    ithIntrSetTriggerModeIrq(uartIthIntrBase[uart_judge_port((uint32_t)pUartObj->port)], ITH_INTR_LEVEL);
    ithIntrRegisterHandlerIrq(uartIthIntrBase[uart_judge_port((uint32_t)pUartObj->port)], &uartIntrHandler, (void *)pUartObj);
    ithUartEnableIntr(pUartObj->port, ITH_UART_RECV_STATUS);

    /* Enable the interrupts. */
    ithIntrEnableIrq(uartIthIntrBase[uart_judge_port((uint32_t)pUartObj->port)]);
    ithExitCritical();

    pUartDmaObj->rdIndex = 0U;
    pUartDmaObj->wtIndex = 0U;

    pUartDmaObj->rxChNum = ithDmaRequestCh(pUartDmaObj->pRxChName, ITH_DMA_CH_PRIO_LOW, NULL, NULL);
    if (pUartDmaObj->rxChNum >= 0)
    {
        ithDmaReset(pUartDmaObj->rxChNum);
    }
    else
    {
        return -1;
    }

    (void)sem_init(&pUartDmaObj->txSem, 0, 1);
    pUartDmaObj->txChNum = ithDmaRequestCh(pUartDmaObj->pTxChName, ITH_DMA_CH_PRIO_HIGH, &dma_write_isr, &pUartDmaObj->txSem);
    if (pUartDmaObj->txChNum >= 0)
    {
        ithDmaReset(pUartDmaObj->txChNum);
    }
    else
    {
        return -1;
    }

    pUartDmaObj->pDmaSrc = (uint8_t *)itpVmemAlloc(UART_DMA_BUF_SIZE);
    pUartDmaObj->pDmaBuf = (uint8_t *)itpVmemAlloc(UART_DMA_BUF_SIZE);

    if ((pUartDmaObj->pDmaBuf == NULL) || (pUartDmaObj->pDmaSrc == NULL))
    {
        (void)ithPrintf("Alloc DMA buffer fail\n");
    }
    else
    {
        LLP_t * llpaddr = NULL;
        pUartDmaObj->LLPCtxt    = (LLP_t *)itpVmemAlloc(sizeof(LLP_t) + 32U);
        llpaddr                 = (LLP_t *)(((uint32_t)pUartDmaObj->LLPCtxt + 0x1FU) & ~(0x1FU));
        llpaddr->SrcAddr        = (uint32_t)pUartObj->port;
        llpaddr->DstAddr        = (uint32_t)pUartDmaObj->pDmaBuf;
        llpaddr->LLP            = (uint32_t)llpaddr;
        llpaddr->TotalSize      = UART_DMA_BUF_SIZE;
        llpaddr->Control        = 0x00210000U;

        ithDmaSetSrcAddr(pUartDmaObj->rxChNum, (uint32_t)pUartObj->port);
        ithDmaSetDstAddr(pUartDmaObj->rxChNum, (uint32_t)pUartDmaObj->pDmaBuf);
        ithDmaSetRequest(pUartDmaObj->rxChNum, ITH_DMA_HW_HANDSHAKE_MODE, (uint32_t)pUartDmaObj->rxReqNum, ITH_DMA_NORMAL_MODE, ITH_DMA_MEM);

        ithDmaSetSrcParamsA(pUartDmaObj->rxChNum, ITH_DMA_WIDTH_8, ITH_DMA_CTRL_FIX);
        ithDmaSetDstParamsA(pUartDmaObj->rxChNum, ITH_DMA_WIDTH_8, ITH_DMA_CTRL_INC);
        ithDmaSetTxSize((int32_t)pUartDmaObj->rxChNum, UART_DMA_BUF_SIZE);
        ithDmaSetBurst(pUartDmaObj->rxChNum, ITH_DMA_BURST_1);
        ithDmaSetLLPAddr(pUartDmaObj->rxChNum, (uint32_t)llpaddr);
        ithDmaStart(pUartDmaObj->rxChNum);
    }

    return 0;
}

static int
UartDmaObjSend (
    UART_OBJ    * pUartObj,
    char        * ptr,
    int         len)
{
    UART_DMA_OBJ * pUartDmaObj = pUartObj->pMode;

    if (pUartDmaObj->pDmaSrc == NULL)
    {
        return 0;
    }

    if (itpSemWaitTimeout(&pUartDmaObj->txSem, UART_DMA_MEM_TIMEOUT) == -1)
    {
        (void)ithPrintf("UART%d dma timeout!\n", uart_judge_port((uint32_t)pUartObj->port));
        ithDmaDumpReg(pUartDmaObj->txChNum);
        goto timeout;
    }

    (void)memcpy(pUartDmaObj->pDmaSrc, ptr, (uint32_t)len);
    ithDmaSetSrcAddr(pUartDmaObj->txChNum, (uint32_t)pUartDmaObj->pDmaSrc);
    ithDmaSetDstAddr(pUartDmaObj->txChNum, (uint32_t)pUartObj->port);
    ithDmaSetRequest(pUartDmaObj->txChNum, ITH_DMA_NORMAL_MODE, ITH_DMA_MEM, ITH_DMA_HW_HANDSHAKE_MODE, (uint32_t)pUartDmaObj->txReqNum);

    ithDmaSetSrcParamsA(pUartDmaObj->txChNum, ITH_DMA_WIDTH_8, ITH_DMA_CTRL_INC);
    ithDmaSetDstParamsA(pUartDmaObj->txChNum, ITH_DMA_WIDTH_8, ITH_DMA_CTRL_FIX);
    ithDmaSetTxSize(pUartDmaObj->txChNum, (uint32_t)len);
    ithDmaSetBurst(pUartDmaObj->txChNum, ITH_DMA_BURST_1);

    ithDmaStart(pUartDmaObj->txChNum);

    return len;

timeout:
    return 0;
}

static int
UartDmaObjRead (
    UART_OBJ    * pUartObj,
    char        * ptr,
    int         len)
{
    UART_DMA_OBJ    * pUartDmaObj = pUartObj->pMode;
    uint32_t        lasttime = 0, timeout_val = pUartObj->timeout;
    uint32_t        dataSize, cpySize = 0;

    if (pUartDmaObj->pDmaBuf == NULL)
    {
        return 0;
    }

    if (timeout_val > 0U)
    {
        lasttime = itpGetTickCount();
    }

retry:
    pUartDmaObj->wtIndex = UART_DMA_BUF_SIZE - ithReadRegA(ITH_DMA_BASE + ITH_DMA_SIZE_CH(pUartDmaObj->rxChNum));

    if (pUartDmaObj->wtIndex != pUartDmaObj->rdIndex)
    {
        if (pUartObj->parity != ITH_UART_NONE)
        {
            if (ithUartIsRxParityError(pUartObj->port))
            {
                (void)ithPrintf("UART%d: parity error!\n", uart_judge_port((uint32_t)pUartObj->port));
            }
            if (ithUartIsRxDataError(pUartObj->port))
            {
                (void)ithPrintf("UART%d: parity or framing error or break indication active!\n", uart_judge_port((uint32_t)pUartObj->port));
            }
        }
        if (pUartDmaObj->wtIndex < pUartDmaObj->rdIndex)
        {
            dataSize = UART_DMA_BUF_SIZE + pUartDmaObj->wtIndex - pUartDmaObj->rdIndex;
        }
        else
        {
            dataSize = pUartDmaObj->wtIndex - pUartDmaObj->rdIndex;
        }

        if ((timeout_val > 0U) && (dataSize < len))
        {
            if (itpGetTickDuration(lasttime) < timeout_val)
            {
                (void)usleep(1);
                goto retry;
            }
        }

        cpySize = ((uint32_t)len > dataSize) ? dataSize : (uint32_t)len;

        if (pUartDmaObj->rdIndex + cpySize > UART_DMA_BUF_SIZE) /* boundary happened*/
        {
            uint32_t left = UART_DMA_BUF_SIZE - pUartDmaObj->rdIndex, waitForRead = 0;
            ithInvalidateDCacheRange(pUartDmaObj->pDmaBuf + pUartDmaObj->rdIndex, left);
            (void)memcpy(ptr, pUartDmaObj->pDmaBuf + pUartDmaObj->rdIndex, left);

            waitForRead             = cpySize - left;
            ithInvalidateDCacheRange(pUartDmaObj->pDmaBuf, waitForRead);
            (void)memcpy(ptr + left, pUartDmaObj->pDmaBuf, waitForRead);
            pUartDmaObj->rdIndex    = waitForRead;
        }
        else
        {
            ithInvalidateDCacheRange(pUartDmaObj->pDmaBuf + pUartDmaObj->rdIndex, cpySize);
            (void)memcpy(ptr, pUartDmaObj->pDmaBuf + pUartDmaObj->rdIndex, cpySize);
            pUartDmaObj->rdIndex += cpySize;
        }
    }
    return (int32_t)cpySize;
}

static void
UartDmaObjTerminate (
    UART_OBJ ** pUartObj)
{
    UART_DMA_OBJ * pUartDmaObj = (*pUartObj)->pMode;

    ithUartClearMode((*pUartObj)->port, ITH_UART_DEFAULT, (*pUartObj)->txPin, (*pUartObj)->rxPin);

    ithUartDisableDmaMode2((*pUartObj)->port);

    ithDmaAbort(pUartDmaObj->rxChNum);
    while (ithDmaIsBusy(pUartDmaObj->rxChNum) == 1)
    {
        (void)ithPrintf("DMA read channel busy\n");
    }
    ithDmaFreeCh(pUartDmaObj->rxChNum);

    ithDmaAbort(pUartDmaObj->txChNum);
    while (ithDmaIsBusy(pUartDmaObj->txChNum) == 1)
    {
        (void)ithPrintf("DMA write channel busy\n");
    }
    ithDmaFreeCh(pUartDmaObj->txChNum);

    itpVmemFree((uint32_t)pUartDmaObj->LLPCtxt);
    itpVmemFree((uint32_t)pUartDmaObj->pDmaSrc);
    itpVmemFree((uint32_t)pUartDmaObj->pDmaBuf);

    (void)sem_destroy(&pUartDmaObj->txSem);

    pUartDmaObj->pRxChName  = "";
    pUartDmaObj->pTxChName  = "";
    pUartDmaObj->txReqNum   = -1;
    pUartDmaObj->rxReqNum   = -1;
    pUartDmaObj->rxChNum    = 0;
    pUartDmaObj->txChNum    = 0;
    pUartDmaObj->pDmaSrc    = NULL;
    pUartDmaObj->pDmaBuf    = NULL;
    pUartDmaObj->wtIndex    = 0;
    pUartDmaObj->rdIndex    = 0;
    pUartDmaObj->LLPCtxt    = NULL;

    (*pUartObj)->uart_init  = NULL;
    (*pUartObj)->uart_dele  = NULL;
    (*pUartObj)->uart_send  = NULL;
    (*pUartObj)->uart_read  = NULL;

    *pUartObj               = NULL;
}
#endif
