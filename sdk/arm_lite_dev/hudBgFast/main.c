#include <stdarg.h>
#include <string.h>
#include "ite/ith.h"
#include "ith/ith_cmdq.h"
#include "arm_lite_dev/hudBgFast/hudBgFast.h"



ITHCmdQ* ithCmdQ = NULL;
ITHCmdQ* ithCmdQ1 = NULL;

static uint32_t cmdQBase, *currPtr;
static uint32_t cmdQBase1, *currPtr1;

static uint32_t    *gTableID;
static uint32_t    *gHudTableIndex;
static uint32_t    *gpHudTableData[3] = {0};

static uint32_t   gBlock_W            = 0;
static uint32_t   gBlock_H            = 0;
static uint32_t   gW_num              = 0;
static uint32_t   gH_num              = 0;
static uint32_t   gBlock_num          = 0;
static uint32_t   gHUDAddrDataNum     = 0;
static uint32_t   hudAllblockDowarp   = 0;

#define HUD_ALLBLOCK_DOWARP

static void hudInvalidateDCacheData()
{
#ifdef CFG_CPU_WB
    ithInvalidateDCacheRange((void *)ithCmdQ,  sizeof(ITHCmdQ));
    ithInvalidateDCacheRange((void *)ithCmdQ1, sizeof(ITHCmdQ));
    ithInvalidateDCacheRange((void *)currPtr,  sizeof(uint32_t));
    ithInvalidateDCacheRange((void *)currPtr1, sizeof(uint32_t));
    ithInvalidateDCacheRange((void *)gTableID, sizeof(uint32_t));
    ithInvalidateDCacheRange((void *)gHudTableIndex, sizeof(uint32_t));
#endif
}

static void SetWritePointer(uint32_t ptr, ITHCmdQPortOffset portOffset)
{
    // ASSERT(ITH_IS_ALIGNED(ptr, sizeof (uint64_t)));
    // ASSERT(ptr != 0);

    ithWriteRegMaskA(ITH_CMDQ_BASE + ITH_CMDQ_WR_REG + portOffset, ptr, ITH_CMDQ_WR_MASK);
}

static void ithCmdInsertSkipCheck2(ITHCmdQPortOffset portOffset, uint8_t* pInput, uint32_t dataSize)
{
    uint32_t    cmdQBufferBase = 0;
    uint32_t    currWriteAddr = 0;
    uint32_t    remainSize = 0;
    uint32_t*   pCurrWritePtr = NULL;

    if (portOffset == ITH_CMDQ0_OFFSET)
    {
        cmdQBufferBase = cmdQBase;
        currWriteAddr = cmdQBufferBase + *currPtr;
        remainSize = ithCmdQ->size - *currPtr;
        pCurrWritePtr = currPtr;
    }
    else if (portOffset == ITH_CMDQ1_OFFSET)
    {
        cmdQBufferBase = cmdQBase1;
        currWriteAddr = cmdQBufferBase + *currPtr1;
        remainSize = ithCmdQ1->size - *currPtr1;
        pCurrWritePtr = currPtr1;
    }

    if (remainSize >= dataSize)
    {
        (void)memcpy((void*)currWriteAddr, pInput, dataSize);
        ithFlushDCacheRange((void*)currWriteAddr, dataSize);
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
        (void)memcpy((void*)currWriteAddr, pInput, remainSize);
        (void)memcpy((void*)cmdQBufferBase, &pInput[remainSize], (dataSize - remainSize));
        ithFlushDCacheRange((void*)currWriteAddr, remainSize);
        ithFlushDCacheRange((void*)cmdQBufferBase, (dataSize - remainSize));
        ithFlushMemBuffer();
        *pCurrWritePtr = dataSize - remainSize;
    }
    SetWritePointer(*pCurrWritePtr, portOffset);
}

static uint32_t gfxGetHUDTable(void)
{
    return *gTableID;
}

static void hudProcessInitCmd(void)
{
    HUD_INIT_DATA *ptInitData = (HUD_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
    ithCmdQ = (ITHCmdQ*)(ptInitData->ithCmdQ);
    ithCmdQ1 = (ITHCmdQ*)(ptInitData->ithCmdQ1);
    cmdQBase = (ptInitData->cmdQBase);
    currPtr = (uint32_t*)(ptInitData->currPtr);
    cmdQBase1 = (ptInitData->cmdQBase1);
    currPtr1 = (uint32_t*)(ptInitData->currPtr1);
    gTableID = (uint32_t*)(ptInitData->gTableID);
    gHudTableIndex = (uint32_t*)(ptInitData->gHudTableIndex);

    gpHudTableData[0] = (uint32_t*)(ptInitData->gpHudTableData1);
    gpHudTableData[1] = (uint32_t*)(ptInitData->gpHudTableData2);
    gpHudTableData[2] = (uint32_t*)(ptInitData->gpHudTableData3);
    gBlock_W = ptInitData->gBlock_W;
    gBlock_H = ptInitData->gBlock_H;
    gW_num = ptInitData->gW_num;
    gH_num = ptInitData->gH_num;
    gBlock_num = gW_num * gH_num;
    gHUDAddrDataNum = (14 * (gBlock_num - 1) + 32) * 2;
}

static void hudProcessInitCmdqCmd(void)
{
    HUD_INIT_DATA *ptInitData = (HUD_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
    ithCmdQ = (ITHCmdQ*)(ptInitData->ithCmdQ);
    ithCmdQ1 = (ITHCmdQ*)(ptInitData->ithCmdQ1);
    cmdQBase = (ptInitData->cmdQBase);
    currPtr = (uint32_t*)(ptInitData->currPtr);
    cmdQBase1 = (ptInitData->cmdQBase1);
    currPtr1 = (uint32_t*)(ptInitData->currPtr1);
}

static void hudProcessInitHwCmd(void)
{
    HUD_INIT_DATA *ptInitData = (HUD_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
    gTableID = (uint32_t*)(ptInitData->gTableID);
    gHudTableIndex = (uint32_t*)(ptInitData->gHudTableIndex);

    gpHudTableData[0] = (uint32_t*)(ptInitData->gpHudTableData1);
    gpHudTableData[1] = (uint32_t*)(ptInitData->gpHudTableData2);
    gpHudTableData[2] = (uint32_t*)(ptInitData->gpHudTableData3);
    gBlock_W = ptInitData->gBlock_W;
    gBlock_H = ptInitData->gBlock_H;
    gW_num = ptInitData->gW_num;
    gH_num = ptInitData->gH_num;
    gBlock_num = gW_num * gH_num;
    gHUDAddrDataNum = (14 * (gBlock_num - 1) + 32) * 2;
}

static void hudProcessSkipBgFastCmd(void)
{
    HUD_CONTEXT *ptOpusCtxt = (HUD_CONTEXT*) CMD_DATA_BUFFER_OFFSET;
    uint16_t* buf = (uint16_t*)ptOpusCtxt->dst;

    int         bottomLineIndex;
        #ifndef HUD_ALLBLOCK_DOWARP
    int         wLoop = (gW_num - 1) / 4;
        #else
    int         wLoop = gW_num - 1;
        #endif
    int         gCounter = 0;
    int         bgTotal = 0;
    int         TotalBlock = gBlock_num;
    int         i, j;
    int         vpBufCurIndex;
    int         totalCount              = 0;

    uint32_t    * pActiveHUDAddrData    = gpHudTableData[*gHudTableIndex];
    uint32_t    * pHudSrcCurrentPos     = &pActiveHUDAddrData[64];


        #ifndef HUD_ALLBLOCK_DOWARP
    bottomLineIndex = (CFG_LCD_WIDTH/16*CFG_LCD_HEIGHT/4) -1;
        #else
    bottomLineIndex = gBlock_num - 1;
        #endif

    // ithCmdQLock(ITH_CMDQ0_OFFSET);
    //First entry.
    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pActiveHUDAddrData, 64 * 4);

    if (((gBlock_W == 16) && (gBlock_H == 4)))
    {
        // Remain entry of first column
        for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
        {
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + gW_num])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        --bottomLineIndex;

        for (i = 0; i < wLoop; i++)
        {
            // expand loop to reduce time.
            // entry of last line
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #endif

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
        #ifndef HUD_ALLBLOCK_DOWARP
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
        #else
                if (true)
        #endif
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }
        #ifndef HUD_ALLBLOCK_DOWARP
        // remain 1 column loops.
        // entry of last line
        if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
        {
            ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
            gCounter++;
        }
        pHudSrcCurrentPos += 28;
        for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
        {
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        #endif
    }
    else if ((gBlock_W == 16) && (gBlock_H == 8))
    {
        // Remain entry of first column
        for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
        {
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex - gW_num])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        --bottomLineIndex;

        for (i = 0; i < wLoop; i++)
        {
            // expand loop to reduce time.
            // entry of last line
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #endif

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
        #ifndef HUD_ALLBLOCK_DOWARP
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
        #else
                if (true)
        #endif
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }

        #ifndef HUD_ALLBLOCK_DOWARP
        // remain 1 column loops.
        // entry of last line
        if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
        {
            ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
            gCounter++;
        }
        pHudSrcCurrentPos += 28;
        for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
        {
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        #endif
    }
    else if ((gBlock_W == 32) && (gBlock_H == 16))
    {
        // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        bottomLineIndex -= 1;
        // Remain entry of first column
        for (j = 0, vpBufCurIndex = bottomLineIndex - 8 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 8 * gW_num)
        {
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex - 2 * gW_num] ||
                buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 6 * gW_num])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        bottomLineIndex -= 2;
        // (void)printf("bottomLineIndex:%d vpBufCurIndex:%d\n", bottomLineIndex, vpBufCurIndex);
        for (i = 0; i < wLoop; i++)
        {
            // expand loop to reduce time.
            // entry of last line
        #ifndef HUD_ALLBLOCK_DOWARP
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3])
        #else
            if (true)
        #endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 8 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 8 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #endif

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 8 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 8 * gW_num)
            {
        #ifndef HUD_ALLBLOCK_DOWARP
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3])
        #else
                if (true)
        #endif
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 8 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 8 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 8 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 8 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;
            // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        }
    }
    else if ((gBlock_W == 32) && (gBlock_H == 32))
    {
        // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        bottomLineIndex -= 1;
        // Remain entry of first column
        for (j = 0, vpBufCurIndex = bottomLineIndex - 16 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 16 * gW_num)
        {
#ifndef HUD_ALLBLOCK_DOWARP
            if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex - 2 * gW_num] ||
                buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 6 * gW_num] ||
                buf[vpBufCurIndex - 8 * gW_num] || buf[vpBufCurIndex + 10 * gW_num] || buf[vpBufCurIndex + 12 * gW_num] || buf[vpBufCurIndex - 10 * gW_num] ||
                buf[vpBufCurIndex + 14 * gW_num] || buf[vpBufCurIndex + 16 * gW_num] || buf[vpBufCurIndex - 12 * gW_num] || buf[vpBufCurIndex - 14 * gW_num])
#else
            if (true)
#endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        bottomLineIndex -= 2;
        // (void)printf("bottomLineIndex:%d vpBufCurIndex:%d\n", bottomLineIndex, vpBufCurIndex);
        for (i = 0; i < wLoop; i++)
        {
            // expand loop to reduce time.
            // entry of last line
#ifndef HUD_ALLBLOCK_DOWARP
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3] ||
                
                buf[bottomLineIndex - 8 * gW_num] || buf[bottomLineIndex - 8 * gW_num + 1] || buf[bottomLineIndex - 10 * gW_num] || buf[bottomLineIndex - 10 * gW_num + 1] ||
                buf[bottomLineIndex - 8 * gW_num + 2] || buf[bottomLineIndex - 8 * gW_num + 3] || buf[bottomLineIndex - 10 * gW_num + 2] || buf[bottomLineIndex - 10 * gW_num + 3] ||
                buf[bottomLineIndex - 12 * gW_num] || buf[bottomLineIndex - 12 * gW_num + 1] || buf[bottomLineIndex - 12 * gW_num + 2] || buf[bottomLineIndex - 12 * gW_num + 3] ||
                buf[bottomLineIndex - 14 * gW_num] || buf[bottomLineIndex - 14 * gW_num + 1] || buf[bottomLineIndex - 14 * gW_num + 2] || buf[bottomLineIndex - 14 * gW_num + 3])
#else
            if (true)
#endif
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
#ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 16 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 16 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3] ||
                    
                    
                    buf[vpBufCurIndex - 8 * gW_num] || buf[vpBufCurIndex - 8 * gW_num + 1] || buf[vpBufCurIndex + 10 * gW_num] || buf[vpBufCurIndex + 10 * gW_num + 1] ||
                    buf[vpBufCurIndex + 12 * gW_num] || buf[vpBufCurIndex + 12 * gW_num + 1] || buf[vpBufCurIndex - 10 * gW_num] || buf[vpBufCurIndex - 10 * gW_num + 1] ||

                    buf[vpBufCurIndex - 8 * gW_num + 2] || buf[vpBufCurIndex - 8 * gW_num + 3] || buf[vpBufCurIndex + 10 * gW_num + 2] || buf[vpBufCurIndex + 10 * gW_num + 3] ||
                    buf[vpBufCurIndex + 12 * gW_num + 2] || buf[vpBufCurIndex + 12 * gW_num + 3] || buf[vpBufCurIndex - 10 * gW_num + 2] || buf[vpBufCurIndex - 10 * gW_num + 3] ||

                    buf[vpBufCurIndex + 14 * gW_num] || buf[vpBufCurIndex + 14 * gW_num + 1] || buf[vpBufCurIndex + 14 * gW_num + 2] || buf[vpBufCurIndex + 14 * gW_num + 3] ||
                    buf[vpBufCurIndex + 16 * gW_num] || buf[vpBufCurIndex + 16 * gW_num + 1] || buf[vpBufCurIndex + 16 * gW_num + 2] || buf[vpBufCurIndex + 16 * gW_num + 3] ||

                    buf[vpBufCurIndex - 12 * gW_num] || buf[vpBufCurIndex - 12 * gW_num + 1] || buf[vpBufCurIndex - 12 * gW_num + 2] || buf[vpBufCurIndex - 12 * gW_num + 3] ||
                    buf[vpBufCurIndex - 14 * gW_num] || buf[vpBufCurIndex - 14 * gW_num + 1] || buf[vpBufCurIndex - 14 * gW_num + 2] || buf[vpBufCurIndex - 14 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3] ||
                
                buf[bottomLineIndex - 8 * gW_num] || buf[bottomLineIndex - 8 * gW_num + 1] || buf[bottomLineIndex - 10 * gW_num] || buf[bottomLineIndex - 10 * gW_num + 1] ||
                buf[bottomLineIndex - 8 * gW_num + 2] || buf[bottomLineIndex - 8 * gW_num + 3] || buf[bottomLineIndex - 10 * gW_num + 2] || buf[bottomLineIndex - 10 * gW_num + 3] ||
                buf[bottomLineIndex - 12 * gW_num] || buf[bottomLineIndex - 12 * gW_num + 1] || buf[bottomLineIndex - 12 * gW_num + 2] || buf[bottomLineIndex - 12 * gW_num + 3] ||
                buf[bottomLineIndex - 14 * gW_num] || buf[bottomLineIndex - 14 * gW_num + 1] || buf[bottomLineIndex - 14 * gW_num + 2] || buf[bottomLineIndex - 14 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
#endif

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 16 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 16 * gW_num)
            {
#ifndef HUD_ALLBLOCK_DOWARP
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3] ||
                    
                    buf[vpBufCurIndex - 8 * gW_num] || buf[vpBufCurIndex - 8 * gW_num + 1] || buf[vpBufCurIndex + 10 * gW_num] || buf[vpBufCurIndex + 10 * gW_num + 1] ||
                    buf[vpBufCurIndex + 12 * gW_num] || buf[vpBufCurIndex + 12 * gW_num + 1] || buf[vpBufCurIndex - 10 * gW_num] || buf[vpBufCurIndex - 10 * gW_num + 1] ||

                    buf[vpBufCurIndex - 8 * gW_num + 2] || buf[vpBufCurIndex - 8 * gW_num + 3] || buf[vpBufCurIndex + 10 * gW_num + 2] || buf[vpBufCurIndex + 10 * gW_num + 3] ||
                    buf[vpBufCurIndex + 12 * gW_num + 2] || buf[vpBufCurIndex + 12 * gW_num + 3] || buf[vpBufCurIndex - 10 * gW_num + 2] || buf[vpBufCurIndex - 10 * gW_num + 3] ||

                    buf[vpBufCurIndex + 14 * gW_num] || buf[vpBufCurIndex + 14 * gW_num + 1] || buf[vpBufCurIndex + 14 * gW_num + 2] || buf[vpBufCurIndex + 14 * gW_num + 3] ||
                    buf[vpBufCurIndex + 16 * gW_num] || buf[vpBufCurIndex + 16 * gW_num + 1] || buf[vpBufCurIndex + 16 * gW_num + 2] || buf[vpBufCurIndex + 16 * gW_num + 3] ||

                    buf[vpBufCurIndex - 12 * gW_num] || buf[vpBufCurIndex - 12 * gW_num + 1] || buf[vpBufCurIndex - 12 * gW_num + 2] || buf[vpBufCurIndex - 12 * gW_num + 3] ||
                    buf[vpBufCurIndex - 14 * gW_num] || buf[vpBufCurIndex - 14 * gW_num + 1] || buf[vpBufCurIndex - 14 * gW_num + 2] || buf[vpBufCurIndex - 14 * gW_num + 3])
#else
                if (true)
#endif
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
#ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3] ||
                
                buf[bottomLineIndex - 8 * gW_num] || buf[bottomLineIndex - 8 * gW_num + 1] || buf[bottomLineIndex - 10 * gW_num] || buf[bottomLineIndex - 10 * gW_num + 1] ||
                buf[bottomLineIndex - 8 * gW_num + 2] || buf[bottomLineIndex - 8 * gW_num + 3] || buf[bottomLineIndex - 10 * gW_num + 2] || buf[bottomLineIndex - 10 * gW_num + 3] ||
                buf[bottomLineIndex - 12 * gW_num] || buf[bottomLineIndex - 12 * gW_num + 1] || buf[bottomLineIndex - 12 * gW_num + 2] || buf[bottomLineIndex - 12 * gW_num + 3] ||
                buf[bottomLineIndex - 14 * gW_num] || buf[bottomLineIndex - 14 * gW_num + 1] || buf[bottomLineIndex - 14 * gW_num + 2] || buf[bottomLineIndex - 14 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 16 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 16 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3] ||
                    
                    buf[vpBufCurIndex - 8 * gW_num] || buf[vpBufCurIndex - 8 * gW_num + 1] || buf[vpBufCurIndex + 10 * gW_num] || buf[vpBufCurIndex + 10 * gW_num + 1] ||
                    buf[vpBufCurIndex + 12 * gW_num] || buf[vpBufCurIndex + 12 * gW_num + 1] || buf[vpBufCurIndex - 10 * gW_num] || buf[vpBufCurIndex - 10 * gW_num + 1] ||

                    buf[vpBufCurIndex - 8 * gW_num + 2] || buf[vpBufCurIndex - 8 * gW_num + 3] || buf[vpBufCurIndex + 10 * gW_num + 2] || buf[vpBufCurIndex + 10 * gW_num + 3] ||
                    buf[vpBufCurIndex + 12 * gW_num + 2] || buf[vpBufCurIndex + 12 * gW_num + 3] || buf[vpBufCurIndex - 10 * gW_num + 2] || buf[vpBufCurIndex - 10 * gW_num + 3] ||

                    buf[vpBufCurIndex + 14 * gW_num] || buf[vpBufCurIndex + 14 * gW_num + 1] || buf[vpBufCurIndex + 14 * gW_num + 2] || buf[vpBufCurIndex + 14 * gW_num + 3] ||
                    buf[vpBufCurIndex + 16 * gW_num] || buf[vpBufCurIndex + 16 * gW_num + 1] || buf[vpBufCurIndex + 16 * gW_num + 2] || buf[vpBufCurIndex + 16 * gW_num + 3] ||

                    buf[vpBufCurIndex - 12 * gW_num] || buf[vpBufCurIndex - 12 * gW_num + 1] || buf[vpBufCurIndex - 12 * gW_num + 2] || buf[vpBufCurIndex - 12 * gW_num + 3] ||
                    buf[vpBufCurIndex - 14 * gW_num] || buf[vpBufCurIndex - 14 * gW_num + 1] || buf[vpBufCurIndex - 14 * gW_num + 2] || buf[vpBufCurIndex - 14 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3] ||
                
                buf[bottomLineIndex - 8 * gW_num] || buf[bottomLineIndex - 8 * gW_num + 1] || buf[bottomLineIndex - 10 * gW_num] || buf[bottomLineIndex - 10 * gW_num + 1] ||
                buf[bottomLineIndex - 8 * gW_num + 2] || buf[bottomLineIndex - 8 * gW_num + 3] || buf[bottomLineIndex - 10 * gW_num + 2] || buf[bottomLineIndex - 10 * gW_num + 3] ||
                buf[bottomLineIndex - 12 * gW_num] || buf[bottomLineIndex - 12 * gW_num + 1] || buf[bottomLineIndex - 12 * gW_num + 2] || buf[bottomLineIndex - 12 * gW_num + 3] ||
                buf[bottomLineIndex - 14 * gW_num] || buf[bottomLineIndex - 14 * gW_num + 1] || buf[bottomLineIndex - 14 * gW_num + 2] || buf[bottomLineIndex - 14 * gW_num + 3])
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 16 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 16 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] ||
                    buf[vpBufCurIndex + 4 * gW_num] || buf[vpBufCurIndex + 4 * gW_num + 1] || buf[vpBufCurIndex - 2 * gW_num] || buf[vpBufCurIndex - 2 * gW_num + 1] ||

                    buf[vpBufCurIndex + 2] || buf[vpBufCurIndex + 3] || buf[vpBufCurIndex + 2 * gW_num + 2] || buf[vpBufCurIndex + 2 * gW_num + 3] ||
                    buf[vpBufCurIndex + 4 * gW_num + 2] || buf[vpBufCurIndex + 4 * gW_num + 3] || buf[vpBufCurIndex - 2 * gW_num + 2] || buf[vpBufCurIndex - 2 * gW_num + 3] ||

                    buf[vpBufCurIndex + 6 * gW_num] || buf[vpBufCurIndex + 6 * gW_num + 1] || buf[vpBufCurIndex + 6 * gW_num + 2] || buf[vpBufCurIndex + 6 * gW_num + 3] ||
                    buf[vpBufCurIndex + 8 * gW_num] || buf[vpBufCurIndex + 8 * gW_num + 1] || buf[vpBufCurIndex + 8 * gW_num + 2] || buf[vpBufCurIndex + 8 * gW_num + 3] ||

                    buf[vpBufCurIndex - 4 * gW_num] || buf[vpBufCurIndex - 4 * gW_num + 1] || buf[vpBufCurIndex - 4 * gW_num + 2] || buf[vpBufCurIndex - 4 * gW_num + 3] ||
                    buf[vpBufCurIndex - 6 * gW_num] || buf[vpBufCurIndex - 6 * gW_num + 1] || buf[vpBufCurIndex - 6 * gW_num + 2] || buf[vpBufCurIndex - 6 * gW_num + 3] ||
                    
                    buf[vpBufCurIndex - 8 * gW_num] || buf[vpBufCurIndex - 8 * gW_num + 1] || buf[vpBufCurIndex + 10 * gW_num] || buf[vpBufCurIndex + 10 * gW_num + 1] ||
                    buf[vpBufCurIndex + 12 * gW_num] || buf[vpBufCurIndex + 12 * gW_num + 1] || buf[vpBufCurIndex - 10 * gW_num] || buf[vpBufCurIndex - 10 * gW_num + 1] ||

                    buf[vpBufCurIndex - 8 * gW_num + 2] || buf[vpBufCurIndex - 8 * gW_num + 3] || buf[vpBufCurIndex + 10 * gW_num + 2] || buf[vpBufCurIndex + 10 * gW_num + 3] ||
                    buf[vpBufCurIndex + 12 * gW_num + 2] || buf[vpBufCurIndex + 12 * gW_num + 3] || buf[vpBufCurIndex - 10 * gW_num + 2] || buf[vpBufCurIndex - 10 * gW_num + 3] ||

                    buf[vpBufCurIndex + 14 * gW_num] || buf[vpBufCurIndex + 14 * gW_num + 1] || buf[vpBufCurIndex + 14 * gW_num + 2] || buf[vpBufCurIndex + 14 * gW_num + 3] ||
                    buf[vpBufCurIndex + 16 * gW_num] || buf[vpBufCurIndex + 16 * gW_num + 1] || buf[vpBufCurIndex + 16 * gW_num + 2] || buf[vpBufCurIndex + 16 * gW_num + 3] ||

                    buf[vpBufCurIndex - 12 * gW_num] || buf[vpBufCurIndex - 12 * gW_num + 1] || buf[vpBufCurIndex - 12 * gW_num + 2] || buf[vpBufCurIndex - 12 * gW_num + 3] ||
                    buf[vpBufCurIndex - 14 * gW_num] || buf[vpBufCurIndex - 14 * gW_num + 1] || buf[vpBufCurIndex - 14 * gW_num + 2] || buf[vpBufCurIndex - 14 * gW_num + 3])
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;
            // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        }
    }
    else
    {
        wLoop           = gW_num - 1;
        bottomLineIndex = gBlock_num - 1;

        // Remain entry of first column
        for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
        {
            if (true)
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
        }
        --bottomLineIndex;

        for (i = 0; i < wLoop; i++)
        {
            // expand loop to reduce time.
            // entry of last line
            if (true)
            {
                ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (true)
                {
                    ithCmdInsertSkipCheck2(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }

                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }
    }
    // ithCmdQUnlock(ITH_CMDQ0_OFFSET);
    // (void)printf("gCounter:%d \n", gCounter);
}

int main(int argc, char **argv)
{
    int inputCmd = 0;

    while(1)
    {
        inputCmd = ARMLITE_COMMAND_REG_READ(REQUEST_CMD_REG);
        if (inputCmd && ARMLITE_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)
        {
            #ifndef CFG_CPU_WB
            ithInvalidateDCache();
            #endif
            switch(inputCmd)
            {
                case INIT_CMD_ID:
                #ifdef CFG_CPU_WB
                    ithInvalidateDCacheRange((void *)CMD_DATA_BUFFER_OFFSET, sizeof(HUD_INIT_DATA));
                #endif
                    hudProcessInitCmd();
                    break;
                case INIT_CMD_CMDQ_ID:
                #ifdef CFG_CPU_WB
                    ithInvalidateDCacheRange((void *)CMD_DATA_BUFFER_OFFSET, sizeof(HUD_INIT_DATA));
                #endif
                    hudProcessInitCmdqCmd();
                    break;
                case INIT_CMD_HW_ID:
                #ifdef CFG_CPU_WB
                    ithInvalidateDCacheRange((void *)CMD_DATA_BUFFER_OFFSET, sizeof(HUD_INIT_DATA));
                #endif
                    hudProcessInitHwCmd();
                    break;
                case SKIP_BG_FAST_CMD_ID:
                #ifdef CFG_CPU_WB
                    ithInvalidateDCacheRange((void*) CMD_DATA_BUFFER_OFFSET, sizeof(HUD_CONTEXT));
                #endif
                    hudInvalidateDCacheData();
                    hudProcessSkipBgFastCmd();
                    break;
                default:
                    break;
            }
            ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, (uint16_t) inputCmd);
        }
    }
    
    return 0;
}
