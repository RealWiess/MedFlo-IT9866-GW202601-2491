/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Remote IR functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "openrtos/FreeRTOS.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "ir/ir.h"

// #define DEBUG_MSG

static IR_OBJ CFG_IR_OBJ[4] = {
    INIT_IR_OBJECT(0),
    INIT_IR_OBJECT(1),
    INIT_IR_OBJECT(2),
    INIT_IR_OBJECT(3),
};

static IR_OBJ * pIrPortObj[4]    = {NULL};

static IR_OBJ   IR_STATIC_OBJ[6] = {};

IR_OBJ *itNewIrObj (ITHIrPort port, IR_OBJ * pIrObj)
{
    if (!pIrPortObj[IR_JUDGE_PORT(port)])
    {
        pIrPortObj[IR_JUDGE_PORT(port)] = &IR_STATIC_OBJ[IR_JUDGE_PORT(port)];
    }
    IR_OBJ *pObj      = pIrPortObj[IR_JUDGE_PORT(port)];

    pObj->port         = pIrObj->port;
    pObj->RxIntr       = pIrObj->RxIntr;
    pObj->TxGpio       = pIrObj->TxGpio;
    pObj->RxGpio       = pIrObj->RxGpio;
    pObj->precision    = pIrObj->precision;
    pObj->irRxMod      = pIrObj->irRxMod;
    pObj->irTxMod      = pIrObj->irTxMod;
    pObj->irRxSample   = pIrObj->irRxSample;
    pObj->intrHandler  = pIrObj->intrHandler;
    pObj->TxChName     = pIrObj->TxChName;
    pObj->TxChannelReq = pIrObj->TxChannelReq;
    pObj->TxChannel    = pIrObj->TxChannel;
    pObj->RxBufFull    = pIrObj->RxBufFull;
    pObj->TxBuffer     = pIrObj->TxBuffer;
    pObj->xRxStrBuf    = pIrObj->xRxStrBuf;
    return pObj;
}

#if 0
static void _IrTxSend(ITHIrPort port, int code)
{
    ithIrTxTransmit(port, code);
}
#endif

static int IrPricDivGcd_, Ir1MDivGcd_; // PRECISON and 1M divided by gcd

static int irCalcGcd_ (int i, int j)
{
    return (j == 0 ? i : irCalcGcd_(j, (i % j)));
}

static void irCalcPricAnd1MGcd_ (int precision)
{
    int gcd       = irCalcGcd_(1000000, precision);
    // printf("gcd: %d, precision: %d\n", gcd, precision);
    IrPricDivGcd_ = precision / gcd;
    Ir1MDivGcd_   = 1000000 / gcd;
    // printf("IrPricDivGcd_: %d, Ir1MDivGcd_: %d\n", IrPricDivGcd_, Ir1MDivGcd_);
}

static void IrRxIntrHandler_ (void * arg)
{
    ITHIrPort     port                     = (ITHIrPort)arg;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    IR_OBJ *      ir_obj                   = pIrPortObj[IR_JUDGE_PORT(port)];
    uint32_t      signal                   = 0;

    // signal in 1 microsecend
    signal                                 = ithIrProbe(ir_obj->port) * Ir1MDivGcd_ / IrPricDivGcd_;

    if (signal > 0)
    {
        if (xStreamBufferSendFromISR(ir_obj->xRxStrBuf, &signal, sizeof(uint32_t), &xHigherPriorityTaskWoken) ==
            pdFALSE)
        {
            ir_obj->RxBufFull = 1;
            (void)ithPrintf("IR%d buffer full\n", IR_JUDGE_PORT(port));
        }
    }
}

//=============================================================================
//                              Public Function Definition
//=============================================================================
int iteIrGetFreqFast (ITHIrPort port)
{
    int cycle = ithGetFreqFast(port);
    if (cycle)
    {
        return PRECISION / cycle;
    }
    else
    {
        return 0;
    }
}

int iteIrGetFreqSlow (ITHIrPort port)
{
    int cycle = ithGetFreqSlow(port);
    if (cycle)
    {
        return PRECISION / cycle;
    }
    else
    {
        return 0;
    }
}

int iteIrGetFreqAvg (ITHIrPort port)
{
#if (CFG_CHIP_FAMILY == 9860)
    printf("Do not use %s\n", __func__); // 9860 not ready
#elif (CFG_CHIP_FAMILY == 9830)
    int cycle = ithGetFreqAvg(port);
    if (cycle)
    {
        return PRECISION / cycle;
    }
#endif
    return 0;
}

int iteIrGetFreqNew (ITHIrPort port)
{
    // IR_OBJ *ir_obj = &CFG_IR_OBJ[IR_JUDGE_PORT(port)];
    IR_OBJ * ir_obj = pIrPortObj[IR_JUDGE_PORT(port)];
    int      cycle  = ithGetFreqNew(port) - 2;
    // (void)ithPrintf("new cycle: %d, ir_obj->precision: %d\n", cycle, ir_obj->precision);
    if (cycle > 0)
    {
        return ir_obj->precision / cycle; // PRECISION / cycle;
    }
    else
    {
        return 0;
    }
}

int iteIrGetHighDCFast (ITHIrPort port)
{
    return ithGetHighDCFast(port);
}

int iteIrGetLowDCFast (ITHIrPort port)
{
    return ithGetLowDCFast(port);
}

int iteIrGetHighDCSlow (ITHIrPort port)
{
    return ithGetHighDCSlow(port);
}

int iteIrGetLowDCSlow (ITHIrPort port)
{
    return ithGetLowDCSlow(port);
}

int iteIrGetHighDCAvg (ITHIrPort port)
{
    return ithGetHighDCAvg(port);
}

int iteIrGetLowDCAvg (ITHIrPort port)
{
    return ithGetLowDCAvg(port);
}

int iteIrGetHighDCNew (ITHIrPort port)
{
    return ithGetHighDCNew(port);
}

int iteIrGetLowDCNew (ITHIrPort port)
{
    return ithGetLowDCNew(port);
}

void iteIrClearClkSample (ITHIrPort port)
{
    ithSetRegBitA(port + ITH_IR_RX_MOD_FILTER_REG, ITH_IR_RX_MOD_FILTER_RST_BIT);
    ithClearRegBitA(port + ITH_IR_RX_MOD_FILTER_REG, ITH_IR_RX_MOD_FILTER_RST_BIT);
}

void iteIrInit (ITHIrPort port, IR_OBJ * pUserDeployIrObj)
{
    int      port_num = IR_JUDGE_PORT(port);
    IR_OBJ * ir_obj;
    if (pUserDeployIrObj == NULL)
    {
        pIrPortObj[port_num] = &CFG_IR_OBJ[IR_JUDGE_PORT(port)];
    }
    else
    {
        pIrPortObj[port_num] = itNewIrObj(port, pUserDeployIrObj);
    }

    ir_obj = pIrPortObj[port_num];

    irCalcPricAnd1MGcd_(ir_obj->precision);

    // IR-RX init
    ithIrRxInit(ir_obj->port, ir_obj->RxGpio, 0, ir_obj->precision);
    // IR-TX init
    ithIrTxInit(ir_obj->port, ir_obj->TxGpio, 0, ir_obj->precision);
    
    if (!ir_obj->irRxSample)
    {
        ir_obj->xRxStrBuf = xStreamBufferCreate(STR_BUF_LEN * sizeof(uint32_t), 1);
        ir_obj->RxBufFull = 0;

        ithEnterCritical();
        // Init IR-RX(remote control) interrupt
        ithIntrDisableIrq(ir_obj->RxIntr);
        ithIntrClearIrq(ir_obj->RxIntr);
        if (!ir_obj->intrHandler)
        {
            ithIntrRegisterHandlerIrq(ir_obj->RxIntr, IrRxIntrHandler_, (void *)ir_obj->port);
        }
        else
        {
            ithIntrRegisterHandlerIrq(ir_obj->RxIntr, ir_obj->intrHandler, (void *)ir_obj->port);
        }
        ithIntrEnableIrq(ir_obj->RxIntr);

        ithIrRxCtrlEnable(ir_obj->port, ITH_IR_INT);
        ithIrRxIntrCtrlEnable(ir_obj->port, ITH_IR_DATA);
        ithExitCritical();
    }

    ir_obj->TxChannel = ithDmaRequestCh(ir_obj->TxChName, ITH_DMA_CH_PRIO_HIGHEST, NULL, NULL);

    if (ir_obj->TxChannel < 0 || ir_obj->TxChannel >= ITH_DMA_CH_NUM)
    {
        (void)ithPrintf("Error: ir_obj->TxChannel is out of valid range: %d\n", ir_obj->TxChannel);
        return;
    }
    else
    {
        ithDmaReset(ir_obj->TxChannel);
    }

    ir_obj->TxBuffer = (uint32_t *)itpVmemAlloc(IR_DMA_BUFFER_SIZE * sizeof(uint32_t));

    if (ir_obj->TxBuffer == NULL)
    {
        (void)ithPrintf("Alloc IR DMA buffer fail\n");
    }

#ifdef ENABLE_IR_RX_MOD
    // Set Rx Modulation Filter
    if (ir_obj->irRxMod)
    {
        (void)ithPrintf("Modulation initial\n");
        // freq:20~60k.(80M/PreScale/freq.) for it9860, PreScale=798(80M)
        ithIrRxSetModFilter(ir_obj->port, (ir_obj->precision - 1) / 60000, (ir_obj->precision - 1) / 20000);
        ithIrRxMode(ir_obj->port, ITH_IR_RX_MODFILTER); // Enable modulation filter
    }
#endif
    ithIrRxCtrlEnable(ir_obj->port, ITH_IR_EN);

#ifdef ENABLE_IR_TX_MOD
    // Set Tx Modulation Freq.
    if (ir_obj->irTxMod)
    {
        int WCLK = ithGetBusClock();
        //(WCLK/SampleRate), SampleRate=39KHz
        ithIrTxSetModFreq(ir_obj->port, WCLK / 39000);
    }
#endif
    ithIrTxCtrlEnable(ir_obj->port, ITH_IR_EN);

#ifdef ENABLE_IR_RX_SAMPLE
    if (ir_obj->irRxSample)
    {
        // Init Clock Sample function
        ithClkSampleInit(ir_obj->port, ir_obj->RxGpio, 0, ir_obj->precision);
        // Set Min & Max Filter
        // ithIrRxSetModFilter(ir_obj->port, 0, 10000);//0x6800, 0x6A00); //ex: 27000Hz=0x6978
        // Enable Clock Sample function
        ithIrRxMode(ir_obj->port, ITH_SAMPLE_CLK);
        (void)ithPrintf("####ir rx sample enable\n");
    }
#endif
}

int iteIrRead (ITHIrPort port, uint32_t * ptr, int len)
{
    IR_OBJ * ir_obj = pIrPortObj[IR_JUDGE_PORT(port)];

    return xStreamBufferReceive(ir_obj->xRxStrBuf, ptr, len * sizeof(uint32_t), 0) / sizeof(uint32_t);
}

int iteIrWrite (ITHIrPort port, uint32_t * ptr, int len)
{
    if ((len - IR_FIFO_LIMIT) > IR_DMA_BUFFER_SIZE)
    {
        (void)ithPrintf("IR write data exceed buffer size! IR_DMA_BUFFER_SIZE: %d, len= %d\n", IR_DMA_BUFFER_SIZE, len);
        return 0;
    }
    ithIrTxCtrlDisable(port, 0);
    int i;
    for (i = 0; (i < len) && (i != IR_FIFO_LIMIT); i++)
    {
        ithIrTxTransmit(port, *(ptr + i));
    }

    if (i == IR_FIFO_LIMIT)
    {
        IR_OBJ * ir_obj         = pIrPortObj[IR_JUDGE_PORT(port)];
        int      remainDataSize = (len - i) * sizeof(uint32_t);

        memcpy(ir_obj->TxBuffer, ptr + i, remainDataSize);

        ithDmaSetSrcAddr(ir_obj->TxChannel, (uint32_t)ir_obj->TxBuffer);
        ithDmaSetDstAddr(ir_obj->TxChannel, ir_obj->port + ITH_IR_TX_DATA_REG);

        ithDmaSetRequest(ir_obj->TxChannel, ITH_DMA_NORMAL_MODE, ITH_DMA_MEM, ITH_DMA_HW_HANDSHAKE_MODE,
                         ir_obj->TxChannelReq);
        ithDmaSetSrcParams(ir_obj->TxChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_INC, ITH_DMA_MASTER_0);
        ithDmaSetDstParams(ir_obj->TxChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_FIX, ITH_DMA_MASTER_1);

        ithDmaSetTxSize(ir_obj->TxChannel, remainDataSize);
        ithDmaSetBurst(ir_obj->TxChannel, ITH_DMA_BURST_1);

        ithDmaStart(ir_obj->TxChannel);
    }
    ithIrTxCtrlEnable(port, 0);
    return 0;
}

void iteIrDeinit(ITHIrPort port)
{
    int port_num = IR_JUDGE_PORT(port);
    IR_OBJ *ir_obj = pIrPortObj[port_num];

    if (ir_obj == NULL) {
        ithPrintf("IR object not initialized\n");
        return;
    }

    if (!ir_obj->irRxSample)
    {
        ithEnterCritical();

        // Disable and clear IR-RX interrupt
        ithIntrDisableIrq(ir_obj->RxIntr);
        ithIntrClearIrq(ir_obj->RxIntr);

        // Disable IR-RX and IR-TX controls
        ithIrRxCtrlDisable(ir_obj->port, ITH_IR_INT);
        ithIrRxIntrCtrlDisable(ir_obj->port, ITH_IR_DATA);
        ithIrRxCtrlDisable(ir_obj->port, ITH_IR_EN);
        ithIrTxCtrlDisable(ir_obj->port, ITH_IR_EN);

        ithExitCritical();
        // Delete stream buffer
        if (ir_obj->xRxStrBuf)
        {
            vStreamBufferDelete(ir_obj->xRxStrBuf);
            ir_obj->xRxStrBuf = NULL;
        }
    }

#ifdef ENABLE_IR_RX_SAMPLE
    if (ir_obj->irRxSample)
    {
        // Disable Clock Sample function
        ithIrRxMode(ir_obj->port, ITH_IR_RX_DEFAULT);
    }
#endif

#ifdef ENABLE_IR_TX_MOD
    // Reset Tx Modulation Freq.
    if (ir_obj->irTxMod)
    {
        ithIrTxSetModFreq(ir_obj->port, 0);
    }
#endif

#ifdef ENABLE_IR_RX_MOD
    // Disable Rx Modulation Filter
    if (ir_obj->irRxMod)
    {
        ithIrRxMode(ir_obj->port, ITH_IR_RX_DEFAULT); //Disable modulation filter
    }
#endif


    // Free DMA channel and buffer
    if (ir_obj->TxChannel != -1)
    {
        ithDmaFreeCh(ir_obj->TxChannel);
        ir_obj->TxChannel = -1;
    }

    if (ir_obj->TxBuffer)
    {
        itpVmemFree((uint32_t)ir_obj->TxBuffer);
        ir_obj->TxBuffer = NULL;
    }

    ithIrRxDeinit(ir_obj->port);
    ithIrTxDeinit(ir_obj->port);

    pIrPortObj[port_num] = NULL;
}
