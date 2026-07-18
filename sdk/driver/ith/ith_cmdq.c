/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL command queue functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include "ith_cfg.h"
#include "../../include/ite/itp.h"
#include "arm_lite_dev/armlite_dev_device.h"
#include "arm_lite_dev/hudBgFast/hudBgFast.h"
#include "gfx/gfx.h"
#include "ith/ith_cmdq.h"

/* Constant definitions */
#define CMDQ_UNIT_SIZE 1024         // 1k

ITHCmdQ         * ithCmdQ = NULL;
ITHCmdQ         * ithCmdQ1 = NULL;

static uint32_t cmdQBase, currPtr, waitSize;
static uint32_t cmdQBase1, currPtr1, waitSize1;
#if defined(CMDQ_IRQ_ENABLE)
static sem_t    cmdqMutex;
static uint32_t gLastSize = 0;
static uint32_t gNum = 0;
static uint32_t gIndex = 0;
static uint32_t gInputCmd = 0;
static uint32_t gCmdQBufferBase = 0;
#endif

static uint32_t
GetReadPointer (
    ITHCmdQPortOffset portOffset)
{
    uint32_t readPtr;

    // first time read value is old
    readPtr = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_RD_REG + portOffset) & ITH_CMDQ_RD_MASK);
    readPtr = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_RD_REG + portOffset) & ITH_CMDQ_RD_MASK);

    return readPtr;
}

static uint32_t
GetWritePointer (
    ITHCmdQPortOffset portOffset)
{
    uint32_t writePtr;

    writePtr = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG + portOffset) & ITH_CMDQ_WR_MASK);

    return writePtr;
}

static void
SetWritePointer (
    uint32_t            ptr,
    ITHCmdQPortOffset   portOffset)
{
    ASSERT(ITH_IS_ALIGNED(ptr, sizeof (uint64_t)));
    ASSERT(ptr < CFG_CMDQ_SIZE);

    ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG + portOffset, ptr, ITH_CMDQ_WR_MASK);
}

static void
FillNullCommands (
    ITHCmdQPortOffset portOffset)
{
    uint32_t size, count, i, * ptr;

    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        size = ithCmdQ->size - currPtr;
        if (size == 0)
        {
            return;
        }

        count   = size / sizeof(uint32_t);

        ptr     = (uint32_t *)(cmdQBase + currPtr);
    }
    else
    {
        size = ithCmdQ1->size - currPtr1;
        if (size == 0)
        {
            return;
        }

        count   = size / sizeof(uint32_t);

        ptr     = (uint32_t *)(cmdQBase1 + currPtr1);
    }

    for (i = 0; i < count; ++i)
    {
        ptr[i] = 0;
    }

    ithFlushMemBuffer();
}

/*
 * Case 1: readPtr <= writePtr <= currPtr <= ithCmdQ->size
 * Case 2: currPtr <= readPtr <= writePtr <= ithCmdQ->size
 * Case 3: writePtr <= currPtr <= readPtr <= ithCmdQ->size
 */
static int
WaitAvailableSize (
    uint32_t            size,
    ITHCmdQPortOffset   portOffset)
{
    uint32_t timeout;

    if (portOffset == ITH_CMDQ0_OFFSET) // CMDQ0
    {
        if (currPtr + size >= ithCmdQ->size)
        {
            // Cannot be case 2, else locking size > queue size
            ASSERT(GetWritePointer(portOffset) <= currPtr);

            timeout = ITH_CMDQ_LOOP_TIMEOUT * 10;
            do
            {
                uint32_t readPtr = GetReadPointer(portOffset);

                // Wait read pointer <= current pointer (case 3 -> case 1)
                if (readPtr <= currPtr)
                {
                    break;
                }

                ITH_LOG_DBG("CMDQ busy1: 0x%X > 0x%X\r\n", readPtr, currPtr );
                //    ithTaskYield();
                (void)usleep(100);
            } while (--timeout);

            if (timeout == 0)
            {
                ITH_LOG_ERR("Wait available1 timeout, size: %d\r\n", size );

#ifdef CFG_ITH_DBG
                ithCmdQStats();
#endif

                return __LINE__;
            }

            // Fill null commands in the end of command queue
            FillNullCommands(portOffset);

            // Reset current pointer to zero
            currPtr = 0;
        }

        // Should currPtr + size < writePtr when in case 2
        ASSERT((GetWritePointer(portOffset) <= currPtr) || (currPtr + size < GetWritePointer(portOffset)));

        timeout = ITH_CMDQ_LOOP_TIMEOUT * 10;
        do
        {
            uint32_t readPtr = GetReadPointer(portOffset);

            // Wait read pointer <= current pointer (case 3 -> case 1) or
            // read pointer - current pointer > required size (case 2 or case 3)
            if ((readPtr <= currPtr) || (readPtr - currPtr > size))
            {
                return 0;
            }

            ITH_LOG_DBG("CMDQ busy2: 0x%X > 0x%X\r\n", readPtr, currPtr );
            //    ithTaskYield();
            (void)usleep(100);
        } while (--timeout);

        ITH_LOG_ERR("Wait available2 timeout, size: %d\r\n", size );
#ifdef CFG_ITH_DBG
        ithCmdQStats();
#endif
    }
    else // CMDQ1
    {
        if (currPtr1 + size >= ithCmdQ1->size)
        {
            // Cannot be case 2, else locking size > queue size
            ASSERT(GetWritePointer(portOffset) <= currPtr1);

            timeout = ITH_CMDQ_LOOP_TIMEOUT * 10;
            do
            {
                uint32_t readPtr1 = GetReadPointer(portOffset);

                // Wait read pointer <= current pointer (case 3 -> case 1)
                if (readPtr1 <= currPtr1)
                {
                    break;
                }

                ITH_LOG_DBG("CMDQ1 busy1: 0x%X > 0x%X\r\n", readPtr1, currPtr1 );
                //    ithTaskYield();
                (void)usleep(100);
            } while (--timeout);

            if (timeout == 0)
            {
                ITH_LOG_ERR("Wait available1 timeout, size: %d\r\n", size );

#ifdef CFG_ITH_DBG
                ithCmdQStats();
#endif

                return __LINE__;
            }

            // Fill null commands in the end of command queue
            FillNullCommands(portOffset);

            // Reset current pointer to zero
            currPtr1 = 0;
        }

        // Should currPtr + size < writePtr when in case 2
        ASSERT((GetWritePointer(portOffset) <= currPtr1) || (currPtr1 + size < GetWritePointer(portOffset)));

        timeout = ITH_CMDQ_LOOP_TIMEOUT * 10;
        do
        {
            uint32_t readPtr1 = GetReadPointer(portOffset);

            // Wait read pointer <= current pointer (case 3 -> case 1) or
            // read pointer - current pointer > required size (case 2 or case 3)
            if ((readPtr1 <= currPtr1) || (readPtr1 - currPtr1 > size))
            {
                return 0;
            }

            ITH_LOG_DBG("CMDQ1 busy2: 0x%X > 0x%X\r\n", readPtr1, currPtr1 );
            //    ithTaskYield();
            (void)usleep(100);
        } while (--timeout);

        ITH_LOG_ERR("Wait available2 timeout, size: %d\r\n", size );
    }

#ifdef CFG_ITH_DBG
    ithCmdQStats();
#endif

    return __LINE__;
}

#if defined(CMDQ_IRQ_ENABLE)

static void
cmdqIntrHandler(
    void * arg)
{
    ITHCmdQPortOffset portOffset = (ITHCmdQPortOffset)arg;

    if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + portOffset) & (0x1 << 11))
    {
        //(void)printf("CMDQ INT!!!\n");
        uint32_t cmdqSize = ithCmdQ->size;
        uint32_t cmdqSizeBound = ithCmdQ->size - 8;
        //printf("(gIndex:%d gNum:%d gInputCmd:0x%x ithCmdQ->addr:0x%x\n", gIndex, gNum, gInputCmd, ithCmdQ->addr);
        //printf("2D:0x%x read:0x%x writw:0x%x SR1:0x%x\n", ithReadRegA(0xB07000C4), GetReadPointer(portOffset), GetWritePointer(portOffset), ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + portOffset));

        uint32_t value;
        uint32_t value2;
        value = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG));
        value2 = ithReadRegA(0xB07000C4);
        if (value & (0x1 << 4))
        {
            printf("value:0x%x\n", value);

            if (value2 & (0x1 << 0))
                printf("2D:0x%x\n", value2);

            printf("ithCmdQ->addr:0x%x\n", ithCmdQ->addr);
        }


        if (gIndex < gNum)
        {
            ithCmdQ->addr = gInputCmd + cmdqSizeBound*gIndex;
            ithCmdQReset(portOffset);
#if BYTE_ORDER == BIG_ENDIAN
            ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, portOffset);
#endif

            SetWritePointer(cmdqSizeBound, portOffset);
            //printf("(SetWritePointer 1\n");

            ithCmdQEnableIntr(portOffset);


            gIndex++;
        }
        else if (gLastSize && (gIndex == gNum))
        {
            ithCmdQ->addr = gInputCmd + cmdqSizeBound*gIndex;

            ithCmdQReset(portOffset);
#if BYTE_ORDER == BIG_ENDIAN
            ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, portOffset);
#endif

            SetWritePointer(gLastSize, portOffset);
            //printf("(SetWritePointer 2\n");

            ithCmdQEnableIntr(portOffset);


            gIndex++;
        }
        else if (gIndex > gNum)
        {
            //printf("(SetWritePointer end\n");
            itpSemPostFromISR(&cmdqMutex);
            ithClearRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_IR1_REG + portOffset, ITH_CMDQ_INT_EN_BIT);
            ithIntrDisableIrq(ITH_INTR_CMDQ);
            ithIntrClearIrq(ITH_INTR_CMDQ);
            gIndex = 0;

        }
    }
    else
    {
        printf("ERROR CMDQ INT!!!\n");
    }


    ithCmdQClearIntr(portOffset);
}
#endif

int
ithCmdQWaitEmpty (
    ITHCmdQPortOffset portOffset)
{
    int         ret = 0;
//#if defined(CMDQ_IRQ_ENABLE)
    int         res = 0;
//#else
    uint32_t    timeout;
//#endif

    ithCmdQLock(portOffset);

//#if defined(CMDQ_IRQ_ENABLE)
//    res = itpSemWaitTimeout(&cmdqMutex, 5000);
//    if (res)
//#else
    timeout = ITH_CMDQ_LOOP_TIMEOUT;
    do
    {
        if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + portOffset) & (0x1 << ITH_CMDQ_ALLIDLE_BIT))
        {
            break;
        }

        (void)usleep(1000);
    } while (--timeout);
    if (timeout == 0)
//#endif

    {
        ITH_LOG_ERR("Wait empty timeout\r\n" );

#ifdef CFG_ITH_DBG
        ithCmdQStats();
#endif

        ret = __LINE__;
    }
    ithCmdQUnlock(portOffset);
    return ret;
}

void
ithCmdQInit (
    ITHCmdQ             * cmdQ,
    ITHCmdQPortOffset   portOffset)
{
    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        ASSERT(cmdQ);
        ASSERT(cmdQ->addr);
        ASSERT(cmdQ->size);

        ithCmdQ     = cmdQ;
        cmdQBase    = (uint32_t)ithMapVram(cmdQ->addr, cmdQ->size, ITH_VRAM_WRITE);
        currPtr     = 0;
        waitSize    = 0;
    }
    else
    {
        ASSERT(cmdQ);
        ASSERT(cmdQ->addr);
        ASSERT(cmdQ->size);

        ithCmdQ1    = cmdQ;
        cmdQBase1   = (uint32_t)ithMapVram(cmdQ->addr, cmdQ->size, ITH_VRAM_WRITE);
        currPtr1    = 0;
        waitSize1   = 0;
    }
#if defined(CMDQ_IRQ_ENABLE)
    sem_init(&cmdqMutex, 0, 0);
#endif
}

void
ithCmdQExit (
    ITHCmdQPortOffset portOffset)
{
    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        ithUnmapVram((void *)cmdQBase, ithCmdQ->size);
        ithCmdQ = NULL;
    }
    else
    {
        ithUnmapVram((void *)cmdQBase1, ithCmdQ1->size);
        ithCmdQ1 = NULL;
    }
}

void
ithCmdQReset (
    ITHCmdQPortOffset portOffset)
{
    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        ASSERT(ithCmdQ);
    }
    else
    {
        ASSERT(ithCmdQ1);
    }

    // Enable command queue clock
    ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_EN_M2CLK_BIT);
    ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_EN_N2CLK_BIT);
    // ithSetRegBitA(ITH_HOST_BASE + ITH_EN_MMIO_REG, ITH_EN_CQ_MMIO_BIT);

    // Reset command queue engine
    ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_CQ_RST_BIT);
    ithDelay(1);
    ithClearRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_CQ_RST_BIT);

    // Initialize command queue registers
    if (ithCmdQ)
    {
        // CMDQ0
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG, ithCmdQ->addr << ITH_CMDQ_BASE_BIT, ITH_CMDQ_BASE_MASK);
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_LEN_REG, ithCmdQ->size / CMDQ_UNIT_SIZE - 1, ITH_CMDQ_LEN_MASK);
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG, 0 << ITH_CMDQ_WR_BIT, ITH_CMDQ_WR_MASK);
    }

    if (ithCmdQ1)
    {
        // CMDQ1
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG + ITH_CMDQ_BASE_OFFSET, ithCmdQ1->addr << ITH_CMDQ_BASE_BIT, ITH_CMDQ_BASE_MASK);
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_LEN_REG + ITH_CMDQ_BASE_OFFSET, ithCmdQ1->size / CMDQ_UNIT_SIZE - 1, ITH_CMDQ_LEN_MASK);
        ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG + ITH_CMDQ_BASE_OFFSET, 0 << ITH_CMDQ_WR_BIT, ITH_CMDQ_WR_MASK);
    }
}

uint32_t *
ithCmdQWaitSize (
    uint32_t            size,
    ITHCmdQPortOffset   portOffset)
{
    ASSERT(size > 0);
    ASSERT(ITH_IS_ALIGNED(size, sizeof(uint64_t)));

    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        ASSERT(ithCmdQ);

        currPtr = GetWritePointer(portOffset);

        // Wait command queue's size is available
        if (WaitAvailableSize(size, portOffset) != 0)
        {
            uint32_t timeout = 5000;
            // Wait 2D engine idle
            while ((ithReadRegA(0xB07000C4) & (0x1 << 0)) && timeout != 0)
            {
                usleep(200);
                timeout--;
            }
            if (timeout == 0)
            {
                ITH_LOG_ERR("2D Wait engine idle timeout\n");

                // reset 2D
                ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);
                usleep(100);
                ithClearRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);
                usleep(100);

                ithCmdQReset(portOffset);
#if BYTE_ORDER == BIG_ENDIAN
                ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, portOffset);
#endif
                currPtr = GetWritePointer(portOffset);
                if (WaitAvailableSize(size, portOffset) != 0)
                {
                    ITH_LOG_ERR("ithCmdQWaitSize return NULL 2\n");
                    for (;;)
                    {
                    }
                    return NULL;
                }
            }
            else
            {
                ITH_LOG_ERR("ithCmdQWaitSize return NULL\n");
                for (;;)
                {
                }
                return NULL;
            }

        }

        ASSERT(ITH_IS_ALIGNED(currPtr, sizeof(uint64_t)));

        waitSize = size;

        return (uint32_t *)(cmdQBase + currPtr);
    }
    else
    {
        ASSERT(ithCmdQ1);

        currPtr1 = GetWritePointer(portOffset);

        // Wait command queue's size is available
        if (WaitAvailableSize(size, portOffset) != 0)
        {
            uint32_t timeout = 5000;
            // Wait 2D engine idle
            while ((ithReadRegA(0xB07000C4) & (0x1 << 0)) && timeout != 0)
            {
                usleep(200);
                timeout--;
            }
            if (timeout == 0)
            {
                ITH_LOG_ERR("2D Wait engine idle timeout\n");

                // reset 2D
                ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);
                usleep(100);
                ithClearRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);
                usleep(100);

                ithCmdQReset(portOffset);
#if BYTE_ORDER == BIG_ENDIAN
                ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, portOffset);
#endif
                currPtr1 = GetWritePointer(portOffset);
                if (WaitAvailableSize(size, portOffset) != 0)
                {
                    ITH_LOG_ERR("ithCmdQWaitSize return NULL 2\n");
                    for (;;)
                    {
                    }
                    return NULL;
                }
            }
            else
            {
                ITH_LOG_ERR("ithCmdQWaitSize return NULL\n");
                for (;;)
                {
                }
                return NULL;
            }
        }

        ASSERT(ITH_IS_ALIGNED(currPtr1, sizeof(uint64_t)));

        waitSize1 = size;

        return (uint32_t *)(cmdQBase1 + currPtr1);
    }
}

void
ithCmdQFlush (
    uint32_t            * ptr,
    ITHCmdQPortOffset   portOffset)
{
    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        uint32_t    cmdsPtr     = cmdQBase + currPtr;
        uint32_t    cmdsSize    = (uint32_t)ptr - cmdsPtr;

#ifdef DEBUG
        if (cmdsSize != waitSize)
        {
            ITH_LOG_ERR("CmdQ cmdsSize %d != waitSize %d\r\n", cmdsSize, waitSize );
        }

#endif // DEBUG

        ASSERT(cmdsSize <= waitSize);

        // Flush cache
        ithFlushMemBuffer();

        currPtr     += cmdsSize;
        SetWritePointer(currPtr, portOffset);
        waitSize    = 0;
    }
    else
    {
        uint32_t    cmdsPtr1    = cmdQBase1 + currPtr1;
        uint32_t    cmdsSize1   = (uint32_t)ptr - cmdsPtr1;

#ifdef DEBUG
        if (cmdsSize1 != waitSize1)
        {
            ITH_LOG_ERR("CmdQ1 cmdsSize1 %d != waitSize1 %d\r\n", cmdsSize1, waitSize1 );
        }

#endif // DEBUG

        ASSERT(cmdsSize1 <= waitSize1);

        // Flush cache
        ithFlushMemBuffer();

        currPtr1    += cmdsSize1;
        SetWritePointer(currPtr1, portOffset);
        waitSize1   = 0;
    }

    // Check whether decode fail
#ifdef DEBUG
    if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + portOffset) & (0x1 << ITH_CMDQ_CMQFAIL_BIT))
    {
        ITH_LOG_ERR("CmdQ decode fail\r\n" );
        ithCmdQStats();
        ASSERT(0);
    }
#endif // DEBUG
}

void
ithCmdQFlip (
    unsigned int        index,
    ITHCmdQPortOffset   portOffset)
{
    uint32_t * ptr;

    ithCmdQLock(portOffset);
    ptr = ithCmdQWaitSize(ITH_CMDQ_SINGLE_CMD_SIZE, portOffset);
    ITH_CMDQ_SINGLE_CMD(ptr, ITH_CMDQ_BASE + ITH_CMDQ_FLIPIDX_REG + portOffset, index);
    ithCmdQFlush(ptr, portOffset);
    ithCmdQUnlock(portOffset);
}

void
ithCmdQStats (
    void)
{
    uint32_t    baseAddr, writePtr, readPtr, addr;
    uint32_t    baseAddr1, writePtr1, readPtr1, addr1;
    char        ctrlBits[33], statusBits[33];
    char        ctrlBits1[33], statusBits1[33];

    // CMDQ0
    PRINTF("CMDQ 0 SW: addr=0x%X,size=%d,base=0x%X,mutex=0x%X\r\n",
        ithCmdQ->addr,
        ithCmdQ->size,
        cmdQBase,
        (uint32_t)ithCmdQ->mutex);

    baseAddr    = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG) & ITH_CMDQ_BASE_MASK) >> ITH_CMDQ_BASE_BIT;

    writePtr    = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG) & ITH_CMDQ_WR_MASK) >> ITH_CMDQ_WR_BIT;

    readPtr     = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_RD_REG) & ITH_CMDQ_RD_MASK) >> ITH_CMDQ_RD_BIT;

    ithU32tob(ctrlBits, ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_CR_REG));
    ithU32tob(statusBits, ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG));

    (void)ithPrintf("CMDQ 0 HW: addr=0x%X,len=%d,writePtr=0x%X,ctl=%s,readPtr=0x%X,sr1=%s\r\n",
        baseAddr,
        (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_LEN_REG) & ITH_CMDQ_LEN_MASK) >> ITH_CMDQ_LEN_BIT,
        writePtr,
        &ctrlBits[sizeof(uint32_t) * 8 - 16],
        readPtr,
        &statusBits[sizeof(uint32_t) * 8 - 16]);

    ithPrintRegA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG, (0x3C) + sizeof(uint32_t));

#define FORWARD_SIZE 16
    addr = (readPtr > FORWARD_SIZE)? readPtr - FORWARD_SIZE : 0;

    ithPrintVram32(baseAddr + addr, FORWARD_SIZE);

    // CMDQ1
    PRINTF("CMDQ 1 SW: addr=0x%X,size=%d,base=0x%X,mutex=0x%X\r\n",
        ithCmdQ1->addr,
        ithCmdQ1->size,
        cmdQBase1,
        (uint32_t)ithCmdQ1->mutex);

    baseAddr1   = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG + ITH_CMDQ_BASE_OFFSET) & ITH_CMDQ_BASE_MASK) >> ITH_CMDQ_BASE_BIT;

    writePtr1   = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG + ITH_CMDQ_BASE_OFFSET) & ITH_CMDQ_WR_MASK) >> ITH_CMDQ_WR_BIT;

    readPtr1    = (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_RD_REG + ITH_CMDQ_BASE_OFFSET) & ITH_CMDQ_RD_MASK) >> ITH_CMDQ_RD_BIT;

    ithU32tob(ctrlBits1, ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_CR_REG + ITH_CMDQ_BASE_OFFSET));
    ithU32tob(statusBits1, ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ_BASE_OFFSET));

    (void)ithPrintf("CMDQ 1 HW: addr=0x%X,len=%d,writePtr=0x%X,ctl=%s,readPtr=0x%X,sr1=%s\r\n",
        baseAddr1,
        (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_LEN_REG + ITH_CMDQ_BASE_OFFSET) & ITH_CMDQ_LEN_MASK) >> ITH_CMDQ_LEN_BIT,
        writePtr1,
        &ctrlBits1[sizeof(uint32_t) * 8 - 16],
        readPtr1,
        &statusBits1[sizeof(uint32_t) * 8 - 16]);

    ithPrintRegA(ITH_CMDQ_BASE + ITH_CMDQ_BASE_REG + ITH_CMDQ_BASE_OFFSET, 0x3C + sizeof(uint32_t));

#define FORWARD_SIZE 16
    addr1 = (readPtr1 > FORWARD_SIZE)? readPtr1 - FORWARD_SIZE : 0;

    ithPrintVram32(baseAddr1 + addr1, FORWARD_SIZE);
}

void
ithCmdQEnableClock (
    void)
{
    // Enable N5CLK
    // ithSetRegBitH(ITH_ISP_CLK2_REG, ITH_EN_N5CLK_BIT);

    // Enable command queue clock
    ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_EN_M2CLK_BIT);
}

void
ithCmdQDisableClock (
    void)
{
    ithClearRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, ITH_EN_M2CLK_BIT);

    // if ((ithReadRegH(ITH_ISP_CLK2_REG) & (0x1 << ITH_EN_ICLK_BIT)) == 0)
    //    ithClearRegBitH(ITH_ISP_CLK2_REG, ITH_EN_N5CLK_BIT);   // disable N5 clock safely
}

void
ithCmdQSetTripleBuffer (
    ITHCmdQPortOffset portOffset)
{
    ithSetRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_CR_REG + portOffset, ITH_CMDQ_FLIPBUFMODE_BIT);
}

/**
 * Enables specified command queue controls.
 *
 * @param ctrl the controls to enable.
 */
void
ithCmdQCtrlEnable (
    ITHCmdQCtrl         ctrl,
    ITHCmdQPortOffset   portOffset)
{
    ithSetRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_CR_REG + portOffset, ctrl);
}

/**
 * Disables specified command queue controls.
 *
 * @param ctrl the controls to disable.
 */
void
ithCmdQCtrlDisable (
    ITHCmdQCtrl         ctrl,
    ITHCmdQPortOffset   portOffset)
{
    ithClearRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_CR_REG + portOffset, ctrl);
}

int
ithGetCmdAvailableSize (
    ITHCmdQPortOffset portOffset)
{
    uint32_t    remainSize      = 0;
    uint32_t    * pCurrWritePtr = NULL;
    uint32_t    readPtr         = GetReadPointer(portOffset);

    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        remainSize      = ithCmdQ->size - currPtr;
        pCurrWritePtr   = &currPtr;
    }
    else if (portOffset == ITH_CMDQ1_OFFSET)
    {
        remainSize      = ithCmdQ1->size - currPtr1;
        pCurrWritePtr   = &currPtr1;
    }

    if (readPtr > *pCurrWritePtr)
    {
        return (readPtr - *pCurrWritePtr);
    }
    else
    {
        return (remainSize + readPtr);
    }
}

#if defined(CMDQ_IRQ_ENABLE)
void
ithCmdIntrCheck (
    ITHCmdQPortOffset   portOffset,
    uint8_t             * pInput,
    uint32_t            dataSize)
{
    uint32_t cmdqSize = ithCmdQ->size;
    uint32_t cmdqSizeBound = ithCmdQ->size - 8;  
    int      res = 0;
    uint32_t *ptr;

    //printf("---------cmdQBufferBase:0x%x cmdQBase:0x%x\n", cmdQBufferBase, cmdQBase);
    //printf("currPtr:0x%x rdPtr:0x%x cmdqSize:0x%x\n", GetWritePointer(portOffset), GetReadPointer(portOffset), cmdqSize);

    gCmdQBufferBase = ithCmdQ->addr;

    gLastSize = dataSize % cmdqSizeBound;
    gNum = dataSize / cmdqSizeBound;
    //printf("pInput:0x%x dataSize:0x%x lastSize:0x%x num:%d\n", pInput, dataSize, gLastSize, gNum);
    gInputCmd = (uint32_t)pInput;


    // Enable CMDQ_IRQ_ENABLE need to add a useless 2D command
    ptr = ithCmdQWaitSize(8, portOffset);
    ITH_CMDQ_SINGLE_CMD(ptr, 0xB07000A8, 1);
    ithCmdQFlush(ptr, portOffset);


    ithCmdQEnableIntr(portOffset);

    sem_wait(&cmdqMutex);
    //res = itpSemWaitTimeout(&cmdqMutex, 500); //sem_wait(&cmdqMutex);// 
    //if (res)
    //    printf("Cmdq Wait empty timeout\n");

    ithCmdQ->addr = gCmdQBufferBase;
    ithCmdQ->size = cmdqSize;
    ithCmdQReset(portOffset);
#if BYTE_ORDER == BIG_ENDIAN
    ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, portOffset);
#endif
}
#endif


void
ithCmdInsertSkipCheck (
    ITHCmdQPortOffset   portOffset,
    uint8_t             * pInput,
    uint32_t            dataSize)
{
    uint32_t    cmdQBufferBase  = 0;
    uint32_t    currWriteAddr   = 0;
    uint32_t    remainSize      = 0;
    uint32_t    * pCurrWritePtr = NULL;

    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        cmdQBufferBase  = cmdQBase;
        currWriteAddr   = cmdQBufferBase + currPtr;
        remainSize      = ithCmdQ->size - currPtr;
        pCurrWritePtr   = &currPtr;
    }
    else if (portOffset == ITH_CMDQ1_OFFSET)
    {
        cmdQBufferBase  = cmdQBase1;
        currWriteAddr   = cmdQBufferBase + currPtr1;
        remainSize      = ithCmdQ1->size - currPtr1;
        pCurrWritePtr   = &currPtr1;
    }

    if (remainSize >= dataSize)
    {
        (void)memcpy((void *)currWriteAddr, pInput, dataSize);
        ithFlushMemBuffer();

        if (remainSize == dataSize)
        {
            *pCurrWritePtr = 0;
        }
        else
        {
            *pCurrWritePtr += dataSize;
        }
    }
    else
    {
        (void)memcpy((void *)currWriteAddr, pInput, remainSize);
        (void)memcpy((void *)cmdQBufferBase, &pInput[remainSize], (dataSize - remainSize));
        ithFlushMemBuffer();
        *pCurrWritePtr = dataSize - remainSize;
    }
    SetWritePointer(*pCurrWritePtr, portOffset);
}

void
ithCmdEnableCPU2 (
    void)
{
#ifdef CFG_M2D_HUD_ENABLE
    #ifdef CPU2
    int             armLiteEngineType   = ARMLITE_HUD_BG_FAST;
    HUD_INIT_DATA   tInitData           = {0};

    // Load Pattern Generator Engine on CPU2
    ioctl(ITP_DEVICE_ARMLITE, ITP_IOCTL_ARMLITE_SWITCH_ENG, &armLiteEngineType);
    ioctl(ITP_DEVICE_ARMLITE, ITP_IOCTL_INIT, NULL);
    (void)usleep(100 * 1000);
    (void)printf("Init CPU2 Parameters\n");
    tInitData.ithCmdQ   = (uint32_t)ithCmdQ;
    tInitData.ithCmdQ1  = (uint32_t)ithCmdQ1;
    tInitData.cmdQBase  = cmdQBase;
    tInitData.currPtr   = (uint32_t)(&currPtr);
    tInitData.cmdQBase1 = cmdQBase1;
    tInitData.currPtr1  = (uint32_t)(&currPtr1);
    ioctl(ITP_DEVICE_ARMLITE, ITP_IOCTL_HUD_INIT_CMDQ_PARAM, &tInitData);
    #endif
#endif
}

void
ithCmdFlushDCacheCPU2 (
    void)
{
#ifdef CFG_M2D_HUD_ENABLE
    #ifdef CPU2
    if (ithCmdQ) {ithFlushDCacheRange(ithCmdQ, sizeof(ITHCmdQ));}
    if (ithCmdQ1) {ithFlushDCacheRange(ithCmdQ1, sizeof(ITHCmdQ));}
    ithFlushDCacheRange(&currPtr, sizeof(uint32_t));
    ithFlushDCacheRange(&currPtr1, sizeof(uint32_t));
    #endif
#endif
}

#if defined(CMDQ_IRQ_ENABLE)

/**
 * Clears the interrupt of command queue.
 */
void
ithCmdQClearIntr (
    ITHCmdQPortOffset portOffset)
{
    ithSetRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_IR1_REG + portOffset, ITH_CMDQ_INTCLR_BIT);
    ithClearRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_IR1_REG + portOffset, ITH_CMDQ_INTCLR_BIT);
}

/**
 * enable the interrupt of command queue.
 */
void
ithCmdQEnableIntr (
    ITHCmdQPortOffset portOffset)
{
    // select cmdq interrupt mode
    ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_IR1_REG + portOffset, 0x1 << ITH_CMDQ_INT_MODE_BIT, 0xF << ITH_CMDQ_INT_MODE_BIT);

    // enable cmdq IRQ
    ithIntrDisableIrq(ITH_INTR_CMDQ);
    //ithCmdQClearIntr(portOffset);
    ithIntrClearIrq(ITH_INTR_CMDQ);

    ithIntrRegisterHandlerIrq(ITH_INTR_CMDQ, cmdqIntrHandler, (void *)portOffset);
    ithIntrSetTriggerModeIrq(ITH_INTR_CMDQ, ITH_INTR_EDGE);
    ithIntrSetTriggerLevelIrq(ITH_INTR_CMDQ, ITH_INTR_HIGH_RISING);
    ithIntrEnableIrq(ITH_INTR_CMDQ);

    // enable cmdq interrupt
    ithSetRegBitA(ITH_CMDQ_BASE + ITH_CMDQ_IR1_REG + portOffset, ITH_CMDQ_INT_EN_BIT);
}
#endif
