/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL interrupt handler functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"
#ifdef __OPENRTOS__
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/task.h"
    #include "openrtos/portmacro.h"
#endif
#ifdef __SM32__
    #include "openrtos/sm32/port_spr_defs.h"
#endif

#define ITH_INTR_MAX (64)

static ITHIntrHandler irqHandlerTable[ITH_INTR_MAX];
static ITHIntrHandler fiqHandlerTable[ITH_INTR_MAX];
static void *         irqArgTable[ITH_INTR_MAX]; // the argument to be passed to the irq handler
static void *         fiqArgTable[ITH_INTR_MAX]; // the argument to be passed to the fiq handler

static void           intrDefaultHandler (void * arg)
{
    // DO NOTHING
}

void ithIntrInit (void)
{
    int i;

    for (i = 0; i < ITH_INTR_MAX; ++i)
    {
        irqHandlerTable[i] = fiqHandlerTable[i] = intrDefaultHandler;
    }
}

void ithIntrRegisterHandlerIrq (ITHIntr intr, ITHIntrHandler handler, void * arg)
{
    if (intr < ITH_INTR_MAX)
    {
        ithEnterCritical();
        irqHandlerTable[intr] = handler ? handler : intrDefaultHandler;
        irqArgTable[intr]     = arg;
        ithExitCritical();
    }
}

#ifdef __arm__
void ithIntrDoIrq (void) __attribute__((naked)) __attribute__((__optimize__ ("-fno-stack-protector")));
#endif
void ithIntrDoIrq (void)
{
#ifdef __arm__
    portSAVE_CONTEXT();
#endif

#if defined(CFG_DBG_TRACE_ANALYZER) && defined(CFG_DBG_TRACE)
    // vTraceStoreISRBegin(1);
#endif

    // WARNING - Do not use local (stack) variables here.  Use globals if you must!

    register uint32_t intrNum  = 0U;
    register uint32_t intrSrc1 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_IRQ1_STATUS_REG); // read irq1 source
    register uint32_t intrSrc2 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_IRQ2_STATUS_REG); // read irq2 source

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc1 != 0U)
    {
        if ((intrSrc1 & 0xFFFFU) == 0U)
        {
            intrNum += 16U;
            intrSrc1 >>= 16U;
        }

        if ((intrSrc1 & 0xFFU) == 0U)
        {
            intrNum += 8U;
            intrSrc1 >>= 8U;
        }

        if ((intrSrc1 & 0xFU) == 0U)
        {
            intrNum += 4U;
            intrSrc1 >>= 4U;
        }

        if ((intrSrc1 & 0x3U) == 0U)
        {
            intrNum += 2U;
            intrSrc1 >>= 2U;
        }

        if ((intrSrc1 & 0x1U) == 0U)
        {
            intrNum += 1U;
            intrSrc1 >>= 1U;
        }

        ithIntrClearIrq(intrNum);

        // Get the vector and process the interrupt
        irqHandlerTable[intrNum](irqArgTable[intrNum]);

        intrSrc1 &= ~((uint32_t)1U);
    }

    intrNum = 32U;

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc2 != 0U)
    {
        if ((intrSrc2 & 0xFFFFU) == 0U)
        {
            intrNum += 16U;
            intrSrc2 >>= 16U;
        }

        if ((intrSrc2 & 0xFFU) == 0U)
        {
            intrNum += 8U;
            intrSrc2 >>= 8U;
        }

        if ((intrSrc2 & 0xFU) == 0U)
        {
            intrNum += 4U;
            intrSrc2 >>= 4U;
        }

        if ((intrSrc2 & 0x3U) == 0U)
        {
            intrNum += 2U;
            intrSrc2 >>= 2U;
        }

        if ((intrSrc2 & 0x1U) == 0U)
        {
            intrNum += 1U;
            intrSrc2 >>= 1U;
        }

        ithIntrClearIrq(intrNum);

        // Get the vector and process the interrupt
        irqHandlerTable[intrNum](irqArgTable[intrNum]);

        intrSrc2 &= ~((uint32_t)1U);
    }

#if defined(CFG_DBG_TRACE_ANALYZER) && defined(CFG_DBG_TRACE)
    // vTraceStoreISREnd(0);
#endif

#ifdef __SM32__
    mtspr(SPR_PICSR, 0); // clear interrupt status: all modules have level interrupts, which have to be cleared by
                         // software, thus this is safe, since non processed interrupts will get re-asserted soon enough
#endif
#ifdef __arm__
    portRESTORE_CONTEXT();
#endif
}

void ithIntrRegisterHandlerFiq (ITHIntr intr, ITHIntrHandler handler, void * arg)
{
    if (intr < ITH_INTR_MAX)
    {
        ithEnterCritical();
        fiqHandlerTable[intr] = handler ? handler : intrDefaultHandler;
        fiqArgTable[intr]     = arg;
        ithExitCritical();
    }
}

#ifdef __arm__
void ithIntrDoFiq (void) __attribute__((naked)) __attribute__((__optimize__ ("-fno-stack-protector")));
#endif
void ithIntrDoFiq (void)
{
#ifdef __arm__
    portSAVE_CONTEXT();
#endif

#if defined(CFG_DBG_TRACE_ANALYZER) && defined(CFG_DBG_TRACE)
    // vTraceStoreISRBegin(1);
#endif

    // WARNING - Do not use local (stack) variables here.  Use globals if you must!

    register uint32_t intrNum  = 0U;
    register uint32_t intrSrc1 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_FIQ1_STATUS_REG); // read fiq1 source
    register uint32_t intrSrc2 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_FIQ2_STATUS_REG); // read fiq2 source

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc1 != 0U)
    {
        if ((intrSrc1 & 0xFFFFU) == 0U)
        {
            intrNum += 16U;
            intrSrc1 >>= 16U;
        }

        if ((intrSrc1 & 0xFFU) == 0U)
        {
            intrNum += 8U;
            intrSrc1 >>= 8U;
        }

        if ((intrSrc1 & 0xFU) == 0U)
        {
            intrNum += 4U;
            intrSrc1 >>= 4U;
        }

        if ((intrSrc1 & 0x3U) == 0U)
        {
            intrNum += 2U;
            intrSrc1 >>= 2U;
        }

        if ((intrSrc1 & 0x1U) == 0U)
        {
            intrNum += 1U;
            intrSrc1 >>= 1U;
        }

        ithIntrClearFiq(intrNum);

        // Get the vector and process the interrupt
        fiqHandlerTable[intrNum](fiqArgTable[intrNum]);

        intrSrc1 &= ~((uint32_t)1U);
    }

    intrNum = 32U;

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc2 != 0U)
    {
        if ((intrSrc2 & 0xFFFFU) == 0U)
        {
            intrNum += 16U;
            intrSrc2 >>= 16U;
        }

        if ((intrSrc2 & 0xFFU) == 0U)
        {
            intrNum += 8U;
            intrSrc2 >>= 8U;
        }

        if ((intrSrc2 & 0xFU) == 0U)
        {
            intrNum += 4U;
            intrSrc2 >>= 4U;
        }

        if ((intrSrc2 & 0x3U) == 0U)
        {
            intrNum += 2U;
            intrSrc2 >>= 2U;
        }

        if ((intrSrc2 & 0x1U) == 0U)
        {
            intrNum += 1U;
            intrSrc2 >>= 1U;
        }

        ithIntrClearFiq(intrNum);

        // Get the vector and process the interrupt
        fiqHandlerTable[intrNum](fiqArgTable[intrNum]);

        intrSrc2 &= ~((uint32_t)1U);
    }

#if defined(CFG_DBG_TRACE_ANALYZER) && defined(CFG_DBG_TRACE)
    // vTraceStoreISREnd(0);
#endif

#ifdef __SM32__
    mtspr(SPR_PICSR, 0); // clear interrupt status: all modules have level interrupts, which have to be cleared by
                         // software, thus this is safe, since non processed interrupts will get re-asserted soon enough
#endif
#ifdef __arm__
    portRESTORE_CONTEXT();
#endif
}
