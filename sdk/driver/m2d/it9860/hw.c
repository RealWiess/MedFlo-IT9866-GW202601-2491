/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file hw.c
 *  GFX hardware layer API function file.
 *
 * @author Awin Huang
 * @version 1.0
 * @date 2014-05-28
 */

#include <pthread.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include "hw.h"
#include "ite/ith.h"
#include "msg.h"
#include "../../include/ite/itp.h"
#include "gfx.h"
#include "gfx_math.h"
#include "arm_lite_dev/armlite_dev_device.h"
#include "arm_lite_dev/hudBgFast/hudBgFast.h"
#include "ith/ith_cmdq.h"
// =============================================================================
//                              Compile Option
// =============================================================================
#ifndef CFG_CHIP_FT
    #define GFX_USE_CMDQ
#endif
// #define GFX_USE_COMQ_BURST
// #define GFX_USE_INTERRUPT

// =============================================================================
//                              Extern Reference
// =============================================================================
#ifdef CFG_M2D_HUD_ENABLE
volatile uint32_t   gBlock_W            = 0;
volatile uint32_t   gBlock_H            = 0;
volatile uint32_t   gW_num              = 0;
volatile uint32_t   gH_num              = 0;
volatile uint32_t   gBlock_num          = 0;
volatile uint32_t   gHUDAddrDataNum     = 0;

uint32_t            gTableID            = 0;
uint32_t            * gpHudTableData[3] = {0};
uint32_t            gHudTableIndex      = 0;
uint32_t            Hud2DCmdTable[64]   = {0};
GFXHUDRotateType    gRotType            = GFX_HUD_ROT0;
#ifndef HUD_ADVANCE_ROTATE_90
#ifdef HUD_ALLBLOCK_DOWARP
int                 gTempHudIndex = 2; //add hud source gap down and left pixel
#else
int                 gTempHudIndex = 0;
#endif
#else
int                 gTempHudIndex = 0; //add hud source gap down and left pixel
#endif
static float        gAngle              = 0;
uint32_t            * HUDAddrData       = 0;


extern GFX_DST_SURFACE_INFO * dstInfo;
extern GFX_DST_SURFACE_INFO * srcInfo;
extern GFX_MATRIX           * transform_m;
extern uint32_t             num;
#endif
// =============================================================================
//                              Macro Definition
// =============================================================================

// =============================================================================
//                              Structure Definition
// =============================================================================

// =============================================================================
//                              Global Data Definition
// =============================================================================
static GFX_HW_DEVICE GfxHwDevice = {
    {
        {GFX_REG_SRC_ADDR,     0     },
        {GFX_REG_SRCXY_ADDR,   0     },
        {GFX_REG_SHWR_ADDR,    0     },
        {GFX_REG_SPR_ADDR,     0     },
        {GFX_REG_MASK_ADDR,    0     },
        {GFX_REG_MASKXY_ADDR,  0     },
        {GFX_REG_MHWR_ADDR,    0     },
        {GFX_REG_MPR_ADDR,     0     },
        {GFX_REG_DST_ADDR,     0     },
        {GFX_REG_DSTXY_ADDR,   0     },
        {GFX_REG_DHWR_ADDR,    0     },
        {GFX_REG_DPR_ADDR,     0     },
        {GFX_REG_PXCR_ADDR,    0     },
        {GFX_REG_PYCR_ADDR,    0     },
        {GFX_REG_LNEXY_ADDR,   0     },
        {GFX_REG_ITMR00_ADDR,  0     },
        {GFX_REG_ITMR01_ADDR,  0     },
        {GFX_REG_ITMR02_ADDR,  0     },
        {GFX_REG_ITMR10_ADDR,  0     },
        {GFX_REG_ITMR11_ADDR,  0     },
        {GFX_REG_ITMR12_ADDR,  0     },
        {GFX_REG_ITMR20_ADDR,  0     },
        {GFX_REG_ITMR21_ADDR,  0     },
        {GFX_REG_ITMR22_ADDR,  0     },
        {GFX_REG_FGCOLOR_ADDR, 0     },
        {GFX_REG_LINE_P1_ADDR, 0     },
        {GFX_REG_CAR_ADDR,     0     },
        {GFX_REG_SAFE_ADDR,    0     },
        {GFX_REG_CR1_ADDR,     0     },
        {GFX_REG_CR2_ADDR,     0     },
        {GFX_REG_CR3_ADDR,     0     },
        {GFX_REG_CMD_ADDR,     0     },

        {GFX_REG_ICR_ADDR,     0     },
        {GFX_REG_ISR_ADDR,     0     },
        {GFX_REG_ID1_ADDR,     0     },
        {GFX_REG_ID2_ADDR,     0     },
        {GFX_REG_ID3_ADDR,     0     },
        {GFX_REG_IDM1_ADDR,    0     },
        {GFX_REG_IDM2_ADDR,    0     },
        {GFX_REG_IDM3_ADDR,    0     }
    }
};

// =============================================================================
//                              Private Data Declaration
// =============================================================================
static volatile bool    __hwCRC_en      = false;
static volatile bool    __hwCRC_update  = false;
#ifdef GFX_USE_INTERRUPT
static sem_t            gfxMutex;
#endif

static gfxBlitIdCallback    gppfCallback[LAST_GFX_ID]   = {0};
static int                  gbEnableIdInterrupt         = 0;

// =============================================================================
//                              Private Function Declaration
// =============================================================================
bool
_HwEngineMmioFire (
    GFX_HW_DEVICE * dev);

bool
_HwEngineCmdQFire (
    GFX_HW_DEVICE * dev);

#ifdef CFG_M2D_HUD_ENABLE
bool
_HwEngineCmdQFireHUD (
    GFX_HW_DEVICE * dev);
#endif

#ifdef GFX_USE_INTERRUPT
static void
_IntrHandler (
    void * arg);

bool
_IntrCmdQSingleFire (
    void);
#endif

static void
_BlitIdIntrHandler (
    void * arg)
{
    uint32_t regVal = ithReadRegA(GFX_REG_ISR_ADDR);// clear

    // ID 0
    if (regVal & 0x1)
    {
        if (gppfCallback[0])
        {
            (*gppfCallback[0])();
        }
    }
    // ID 1
    if (regVal & 0x2)
    {
        if (gppfCallback[1])
        {
            (*gppfCallback[1])();
        }
    }
    // ID 1
    if (regVal & 0x4)
    {
        if (gppfCallback[2])
        {
            (*gppfCallback[2])();
        }
    }
}

// =============================================================================
//                              Public Function Definition
// =============================================================================
GFX_HW_DEVICE *
gfxHwInitialize ()
{
    int         i;
    uint32_t    reg = 0;

#ifdef GFX_USE_INTERRUPT

    ithIntrDisableIrq(ITH_INTR_2D);
    ithReadRegA(GFX_REG_ISR_ADDR);// clear
    ithIntrClearIrq(ITH_INTR_2D);

    ithIntrRegisterHandlerIrq(ITH_INTR_2D, _IntrHandler, NULL);
    ithIntrEnableIrq(ITH_INTR_2D);

    // interrupt enable
    ithWriteRegA(GFX_REG_ICR_ADDR, 0x1);

    sem_init(&gfxMutex, 0, 0);
#endif

    // reset 2D, enable 2D clock
    reg = ithReadRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG);

    ithWriteRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0x42af8000 | reg);

#ifndef WIN32
    for (i = 0; i < 100; i++)
    {
        asm volatile ("");
    }
#endif
    ithWriteRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0x02af8000 | reg);

    return &GfxHwDevice;
}

void
gfxHwRegisterBlitIdCallback (
    int                 id,
    gfxBlitIdCallback   pfCallback)
{
    if (id < LAST_GFX_ID)
    {
        gppfCallback[id] = pfCallback;
    }

    if (!gbEnableIdInterrupt)
    {
        ithIntrDisableIrq(ITH_INTR_2D);
        ithReadRegA(GFX_REG_ISR_ADDR);// clear
        ithIntrClearIrq(ITH_INTR_2D);

        ithIntrRegisterHandlerIrq(ITH_INTR_2D, _BlitIdIntrHandler, NULL);
        ithIntrEnableIrq(ITH_INTR_2D);
        gbEnableIdInterrupt = 1;
    }
}

void
gfxHwTerminate (
    GFX_HW_DEVICE * dev)
{
    // disable 2D clock
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0, (0x1 << 15) | (0x1 << 17) | (0x1 << 19) | (0x1 << 23));
}

bool
gfxHwEngineFire (
    GFX_HW_DEVICE * dev)
{
    GFX_FUNC_ENTRY;

    if (true)
    {
        GFX_HW_REGS * regs = &dev->regs;

        // Disable merge for testing
        // regs->regSAFE.data |= 0x00f00000;
        // regs->regSAFE.data |= 0x00000800;

        // Disable read reorder for 9860 hw bug
        regs->regSAFE.data  |= GFX_REG_SAFE_RDREORDER_EN;
        // Disable write reorder for 9860 hw bug
        regs->regSAFE.data  |= GFX_REG_SAFE_WRREORDER_EN;
#ifdef CFG_M2D_HUD_ENABLE
        // Enable boundary interpolation
        regs->regSAFE.data  |= GFX_REG_SAFE_BOUNDARY_INTERPOLATION;
#endif
        // Enable extend by zero for color depth extend
        regs->regCR1.data   |= 0x00004000;
    }

    if (__hwCRC_en)
    {
        GFX_HW_REGS * regs = &dev->regs;
        if (__hwCRC_update)
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_RDCRC_UPD |
                GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_WRCRC_UPD |
                GFX_REG_CR1_ALUCRC_EN | GFX_REG_CR1_ALUCRC_UPD );
        }
        else
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_ALUCRC_EN);
        }
        __hwCRC_update = false;
    }

#ifndef GFX_USE_CMDQ
    return _HwEngineMmioFire(dev);
#else
    return _HwEngineCmdQFire(dev);
#endif
    GFX_FUNC_LEAVE;
}

#ifdef CFG_M2D_HUD_ENABLE

bool
gfxHwEngineFireHUD (
    GFX_HW_DEVICE * dev)
{
    GFX_FUNC_ENTRY;

    if (true)
    {
        GFX_HW_REGS * regs = &dev->regs;

        // Disable read reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_RDREORDER_EN;
        // Disable write reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_WRREORDER_EN;

    #ifdef CFG_M2D_HUD_ENABLE
        // Enable boundary interpolation
        regs->regSAFE.data |= GFX_REG_SAFE_BOUNDARY_INTERPOLATION;
    #endif

        // Enable extend by zero for color depth extend
        regs->regCR1.data |= 0x00004000;
    }

    if (__hwCRC_en)
    {
        GFX_HW_REGS * regs = &dev->regs;
        if (__hwCRC_update)
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_RDCRC_UPD |
                GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_WRCRC_UPD |
                GFX_REG_CR1_ALUCRC_EN | GFX_REG_CR1_ALUCRC_UPD);
        }
        else
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_ALUCRC_EN);
        }
        __hwCRC_update = false;
    }

    return _HwEngineCmdQFireHUD(dev);

    GFX_FUNC_LEAVE;
}
#endif

void
gfxHwReset (
    GFX_HW_DEVICE * dev)
{
    GFX_HW_REGS * regs      = &dev->regs;
    GFX_HW_REG  * currReg   = &regs->regSRC;
    GFX_HW_REG  * endReg    = &regs->regIDM3;

    while (currReg <= endReg)
    {
        currReg->data = 0;
        currReg++;
    }

    regs->regITMR00.data    = 0x80000;
    regs->regITMR11.data    = 0x80000;
    regs->regITMR22.data    = 0x80000;
}

bool
gfxwaitEngineIdle (
    void)
{
#ifndef CFG_CHIP_FT

    uint32_t timeout = 5000;

    if (true)    // FIXME

    {
    #ifdef GFX_USE_INTERRUPT
        _IntrCmdQSingleFire();

        int result = 0;

        result = itpSemWaitTimeout(&gfxMutex, 20);
        if (result)
        {
            /* Time out, fail! */
            (void)printf("Time out, fail!\n");
            return false;
        }
        else
        {
            (void)printf("gfxwaitEngineIdle success!\n");
            return true;
        }
    #else

        #ifdef GFX_USE_CMDQ
        ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
        #endif
        // Wait engine idle
        while ((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY) && timeout != 0)
        {
            usleep(200);
            timeout--;
        }
    #endif
    }
    else // for test gfx CRC
    {
    #ifdef GFX_USE_CMDQ
        while (true)
        {
            if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) & (0x1UL << ITH_CMDQ_ALLIDLE_BIT))
            {
                break;
            }

            usleep(1000);
        }
    #endif
        while (ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY)
        {
            usleep(200);
        }
    }

    if (timeout == 0)
    {
        (void)printf("2D Wait engine idle timeout\n");

        // reset 2D
        ithSetRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);
        usleep(100);
        ithClearRegBitA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 30);

        return false;
    }
    else
    {
        return true;
    }
#else
    return true;
#endif
}

bool
gfxHwCRCInitialize ()
{
    uint32_t status;
    status = ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_CRC32_IMPL;

    if (status)
    {
        __hwCRC_update  = true;
        __hwCRC_en      = true;
        return true;
    }
    else
    {
        __hwCRC_update  = false;
        __hwCRC_en      = false;
        return false;
    }
}

// #ifndef CFG_CHIP_FT
bool
gfxHWCRCGetValue (
    uint32_t    * rdcrc,
    uint32_t    * wrcrc,
    uint32_t    * alucrc)
{
    uint32_t status;
    status = ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_CRC32_IMPL;

    if (status && __hwCRC_en)
    {
        // gfxwaitEngineIdle();
        while (true)
        {
            if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) & (0x1UL << ITH_CMDQ_ALLIDLE_BIT))
            {
                break;
            }

            usleep(1000);
        }

        while (ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY)
        {
            usleep(200);
        }

        *rdcrc  = ithReadRegA(GFX_REG_RDCRC_ADDR);
        *wrcrc  = ithReadRegA(GFX_REG_WRCRC_ADDR);
        *alucrc = ithReadRegA(GFX_REG_ALUCRC_ADDR);

        return true;
    }
    else
    {
        return false;
    }
}
// #endif

#ifdef CFG_M2D_HUD_ENABLE

void
gfxHUDInit (
    uint32_t    blockW,
    uint32_t    blockH,
    GFXHUDRotateType rotType)
{
    gBlock_W        = blockW;
    gBlock_H        = blockH;

    #ifdef HUD_ADVANCE_ROTATE_90
    gW_num          = 800 / gBlock_W;
    gH_num          = 480 / gBlock_H;
    #else
    gW_num          = CFG_LCD_WIDTH / gBlock_W;     
    gH_num          = CFG_LCD_HEIGHT / gBlock_H;  
    gRotType        = rotType;
    #endif

    gBlock_num      = gW_num * gH_num; 
#if defined(CMDQ_IRQ_ENABLE)
    // Enable CMDQ_IRQ_ENABLE need to add a useless 2D command
    gHUDAddrDataNum = (14 * (gBlock_num - 1) + 32 + 1) * 2;
#else
    gHUDAddrDataNum = (14 * (gBlock_num - 1) + 32) * 2;
#endif

    if (NULL != gpHudTableData[0])
    {
        free(gpHudTableData[0]);
    }
    if (NULL != gpHudTableData[1])
    {
        free(gpHudTableData[1]);
    }
    if (NULL != gpHudTableData[2])
    {
        free(gpHudTableData[2]);
    }

    gpHudTableData[0]   = malloc(gHUDAddrDataNum * sizeof(uint32_t));
    gpHudTableData[1]   = malloc(gHUDAddrDataNum * sizeof(uint32_t));
    gpHudTableData[2]   = malloc(gHUDAddrDataNum * sizeof(uint32_t));

    if (NULL != dstInfo)
    {
        free(dstInfo);
    }
    if (NULL != srcInfo)
    {
        free(srcInfo);
    }
    dstInfo = malloc(gBlock_num * sizeof(GFX_DST_SURFACE_INFO));
    srcInfo = malloc(gBlock_num * sizeof(GFX_DST_SURFACE_INFO));

    if (NULL != transform_m)
    {
        free(transform_m);
    }
    transform_m = malloc(gBlock_num * sizeof(GFX_MATRIX));

    #ifdef HUD_INVERSE
    num         = gBlock_num - 1;
    #else
    num         = 0;
    #endif

    #ifndef CFG_HUD_AUTO_GENERATE
    if (NULL != HUDAddrData)
    {
        free(HUDAddrData);
    }
    HUDAddrData = malloc(gHUDAddrDataNum * sizeof(uint32_t));
    #endif

#ifdef CPU2
    HUD_INIT_DATA tInitData = { 0 };

    ithCmdEnableCPU2();

    tInitData.gTableID = (uint32_t)(&gTableID);
    tInitData.gHudTableIndex = (uint32_t)(&gHudTableIndex);
    tInitData.gpHudTableData = (uint32_t)(gpHudTableData);
    tInitData.gpHudTableData1 = (uint32_t)(gpHudTableData[0]);
    tInitData.gpHudTableData2 = (uint32_t)(gpHudTableData[1]);
    tInitData.gpHudTableData3 = (uint32_t)(gpHudTableData[2]);
    tInitData.gBlock_W = gBlock_W;
    tInitData.gBlock_H = gBlock_H;
    tInitData.gW_num = gW_num;
    tInitData.gH_num = gH_num;
    ioctl(ITP_DEVICE_ARMLITE, ITP_IOCTL_HUD_INIT_HW_PARAM, &tInitData);
#endif

}

void
gfxSelectHudTable (
    uint32_t    srcAddr,
    uint32_t    dstAddr)
{
    int i = 0;

    for (i = 0; i < 3; i++)
    {
        if (gpHudTableData[i][17] == dstAddr)
        {
            gHudTableIndex = i;
        }
    }
}

void
gfxSkipBgFast (
    uint16_t * buf)
{
    #ifdef CPU2

    HUD_CONTEXT tCtxData = {0};
    
    // usleep(100 * 1000);
    ithCmdFlushDCacheCPU2();
    ithFlushDCacheRange(buf, ithLcdGetWidth() / 16 * 2 * ithLcdGetHeight() / 4);
    ithFlushDCacheRange(gpHudTableData[0], gHUDAddrDataNum);
    ithFlushDCacheRange(gpHudTableData[1], gHUDAddrDataNum);
    ithFlushDCacheRange(gpHudTableData[2], gHUDAddrDataNum);
    ithFlushDCacheRange(&gTableID, sizeof(uint32_t));
    ithFlushDCacheRange(&gHudTableIndex, sizeof(uint32_t));
    ithFlushMemBuffer();
    tCtxData.dst = (uint32_t)buf;
    ioctl(ITP_DEVICE_ARMLITE, ITP_IOCTL_HUD_SKIP_BG_FAST, &tCtxData);

    // (void)printf("Test is done...................\n");
    #else
    int         bottomLineIndex;
        #ifndef HUD_ALLBLOCK_DOWARP
    int         wLoop = (gW_num - 1) / 4;
        #else
    int         wLoop = gW_num - 1;
        #endif
    int         gCounter = 0;
    int         bgTotal = 0;
    int         TotalBlock = gBlock_num;
    int         i, j = 0;
    int         vpBufCurIndex = 0;
    int         totalCount = 0;
    bool        bLastLine = false;

    uint32_t    * pActiveHUDAddrData    = gpHudTableData[gHudTableIndex];
    uint32_t    * pHudSrcCurrentPos     = &pActiveHUDAddrData[64];


    #ifndef HUD_ALLBLOCK_DOWARP
    #ifdef HUD_ADVANCE_ROTATE_90
    bottomLineIndex = 5999;
    #else
    bottomLineIndex = (CFG_LCD_WIDTH/16*CFG_LCD_HEIGHT/4) -1;
    #endif
    if (0U != ((gW_num - 1) % 4U))
    {
        bLastLine = true;
    }
    #else
    bottomLineIndex = gBlock_num - 1;
    #endif

#if defined(CMDQ_IRQ_ENABLE)

    ithCmdQLock(ITH_CMDQ0_OFFSET);
    //printf("table size:0x%x gpHudTableData:0x%x\n", gHUDAddrDataNum * sizeof(uint32_t), gpHudTableData[gHudTableIndex]);
    ithCmdIntrCheck(ITH_CMDQ0_OFFSET, (uint8_t *)gpHudTableData[gHudTableIndex], gHUDAddrDataNum * sizeof(uint32_t));
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

#else
    ithCmdQLock(ITH_CMDQ0_OFFSET);
    // First entry.
    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pActiveHUDAddrData, 64 * 4);

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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }
        #ifndef HUD_ALLBLOCK_DOWARP
        if (bLastLine)
        {
            // remain 1 column loops.
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
        #ifndef HUD_ALLBLOCK_DOWARP
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #ifndef HUD_ALLBLOCK_DOWARP
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;

            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }

        #ifndef HUD_ALLBLOCK_DOWARP
        if (bLastLine)
        {
            // remain 1 column loops.
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - gW_num] || buf[bottomLineIndex - gW_num + 1])// if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }
            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - 2 * gW_num; j < gH_num - 1; j++, vpBufCurIndex -= 2 * gW_num)
            {
                if (buf[vpBufCurIndex] || buf[vpBufCurIndex + 1] || buf[vpBufCurIndex + gW_num] || buf[vpBufCurIndex + gW_num + 1] ||
                    buf[vpBufCurIndex + 2 * gW_num] || buf[vpBufCurIndex + 2 * gW_num + 1] || buf[vpBufCurIndex - gW_num] || buf[vpBufCurIndex - gW_num + 1])
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;
            // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        }
        #ifndef HUD_ALLBLOCK_DOWARP
        if (bLastLine)
        {
            // remain 1 column loops.
            // entry of last line
            if (buf[bottomLineIndex] || buf[bottomLineIndex + 1] || buf[bottomLineIndex - 2 * gW_num] || buf[bottomLineIndex - 2 * gW_num + 1] ||
                buf[bottomLineIndex + 2] || buf[bottomLineIndex + 3] || buf[bottomLineIndex - 2 * gW_num + 2] || buf[bottomLineIndex - 2 * gW_num + 3] ||
                buf[bottomLineIndex - 4 * gW_num] || buf[bottomLineIndex - 4 * gW_num + 1] || buf[bottomLineIndex - 4 * gW_num + 2] || buf[bottomLineIndex - 4 * gW_num + 3] ||
                buf[bottomLineIndex - 6 * gW_num] || buf[bottomLineIndex - 6 * gW_num + 1] || buf[bottomLineIndex - 6 * gW_num + 2] || buf[bottomLineIndex - 6 * gW_num + 3])
            {
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
        }
        #endif
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
        #endif
                pHudSrcCurrentPos += 28;
            }
            bottomLineIndex -= 2;
            // (void)printf("bottomLineIndex:%d\n", bottomLineIndex);
        }

        #ifndef HUD_ALLBLOCK_DOWARP
        if (bLastLine)
        {
            // remain 1 column loops.
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }
                pHudSrcCurrentPos += 28;
            }
        }
        #endif
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
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
                ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                gCounter++;
            }

            pHudSrcCurrentPos += 28;
            for (j = 0, vpBufCurIndex = bottomLineIndex - gW_num; j < gH_num - 1; j++, vpBufCurIndex -= gW_num)
            {
                if (true)
                {
                    ithCmdInsertSkipCheck(ITH_CMDQ0_OFFSET, (uint8_t *)pHudSrcCurrentPos, 28 * 4);
                    gCounter++;
                }

                pHudSrcCurrentPos += 28;
            }
            --bottomLineIndex;
        }
    }
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);
    // (void)printf("gCounter:%d \n", gCounter);
    #endif

#endif
}

void
gfxUpdateSrcAddr (
    uint32_t    srcAddr,
    uint32_t    dstAddr)
{
    int i, j;
    int n = gBlock_num - 2;

    if (srcAddr != 0)
    {
    #ifdef HUD_HORIZONTAL
        for (j = 0; j < gH_num; j++)
        {
            for (i = 0; i < gW_num; i++)
    #else
        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
    #endif
            {
                if ((i == gW_num - 1) && (j == gH_num - 1))
                {
                    gpHudTableData[0][1] = srcAddr + gBlock_W * 2 * gW_num * gBlock_H * j + gBlock_W * 2 * i;
                }
                else
                {
                    gpHudTableData[0][65 + 28 * n] = srcAddr + gBlock_W * 2 * gW_num * gBlock_H * j + gBlock_W * 2 * i;
                    n--;
                }
            }
        }
    }

    if (dstAddr != 0)
    {
        n = gBlock_num - 2;
    #ifdef HUD_HORIZONTAL
        for (j = 0; j < gH_num; j++)
        {
            for (i = 0; i < gW_num; i++)
    #else
        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
    #endif
            {
                if ((i == gW_num - 1) && (j == gH_num - 1))
                {
                    gpHudTableData[0][17] = dstAddr;
                }
                else
                {
                    gpHudTableData[0][67 + 28 * n] = dstAddr;
                    n--;
                }
            }
        }
    }
}

void
gfxSetHud2DCmdTable (
    GFX_HW_DEVICE * dev)
{
    GFX_HW_REGS * regs      = &dev->regs;
    GFX_HW_REG  * currReg   = &regs->regSRC;
    GFX_HW_REG  * endReg    = &regs->regICR;
    uint32_t    i           = 0;

    while (currReg < endReg)
    {
        Hud2DCmdTable[i]    = currReg->addr;
        i++;
        Hud2DCmdTable[i]    = currReg->data;
        i++;
        currReg++;
    }

    // for (i = 0; i < 64; i += 2)
    //    (void)printf("0x%08x, 0x%08x\n", Hud2DCmdTable[i], Hud2DCmdTable[i+1]);
}

void
gfxGetHud2DCmdTable (
    uint32_t * cmdTable)
{
    uint32_t i = 0;

    for (i = 0; i < 64; i++)
    {
        cmdTable[i] = Hud2DCmdTable[i];
    }
}

float
gfxGetHUDAngle (
    void)
{
    return (float)gAngle;
}

void
gfxSetHUDAngle (
    float angle)
{
    // (void)printf("gAngle:%f, angle:%f\n", gAngle, angle);
    gAngle = angle;
    // (void)printf("gAngle:%f, angle:%f\n",gAngle, angle);
}

GFXHUDRotateType
gfxGetHudRotateType(
    void)
{
    // (void)printf("gRotType:%d\n", gRotType);
    return gRotType;
}

#endif

// =============================================================================
//                              Private Function Definition
// =============================================================================
bool
_HwEngineMmioFire (
    GFX_HW_DEVICE * dev)
{
    GFX_HW_REGS * regs      = &dev->regs;
    GFX_HW_REG  * currReg   = &regs->regSRC;
    GFX_HW_REG  * endReg    = &regs->regICR;

    // Wait engine idle
    while ((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY))
    {
    }

    while (currReg < endReg)
    {
        ithWriteRegA(currReg->addr, currReg->data);
        // ithPrintf("0x%08x, 0x%08x\n", currReg->addr, currReg->data);
        currReg++;
    }

    // gfxSetHud2DCmdTable(dev);

    return true;
}

#ifndef CFG_CHIP_FT

bool
_HwEngineCmdQSingleFire (
    GFX_HW_DEVICE * dev)
{
    bool        result          = true;
    GFX_HW_REGS * regs          = &dev->regs;
    GFX_HW_REG  * currReg       = &regs->regSRC;
    GFX_HW_REG  * endReg        = &regs->regCMD;
    uint32_t    requestCmdQLen  = 0;
    uint32_t    * cmdqAddr      = NULL;

    ithCmdQLock(ITH_CMDQ0_OFFSET);

    requestCmdQLen  = (((uint32_t)endReg - (uint32_t)currReg) / sizeof(GFX_HW_REG) + 1) * 8;
    cmdqAddr        = ithCmdQWaitSize(requestCmdQLen, ITH_CMDQ0_OFFSET);

    if (cmdqAddr)
    {
        while (currReg <= endReg)
        {
            ITH_CMDQ_SINGLE_CMD(cmdqAddr, currReg->addr, currReg->data);
            //ithPrintf("0x%08x, 0x%08x,\n", currReg->addr, currReg->data);

            currReg++;
        }

        ithCmdQFlush(cmdqAddr, ITH_CMDQ0_OFFSET);
    }

    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

    return result;
}

    #ifdef CFG_M2D_HUD_ENABLE

bool
_HwEngineCmdQSingleFireHUD (
    GFX_HW_DEVICE * dev)
{
    bool        result          = true;

    uint32_t    lastSize        = 0;
    uint32_t    * cmdqAddr      = NULL;
    uint32_t    i;
    uint32_t    offsetSize      = 32000;
    uint32_t    * pHUDAddrData  = NULL;
    uint32_t    num             = 0;
    uint32_t    fireSize;

    pHUDAddrData = gpHudTableData[0]; 

        #ifndef GFX_USE_CMDQ
    // Wait engine idle
    while ((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY))
    {
    }

    for (i = 0; i < gHUDAddrDataNum; i += 2)
    {
        if (gpHudTableData[0][i] == GFX_REG_BASE_ADDR)
        {
            while ((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY))
            {
            }
        }

        ithWriteRegA(gpHudTableData[0][i], gpHudTableData[0][i + 1]);
        // ithPrintf("0x%08x, 0x%08x,\n", gpHudTableData[0][i], gpHudTableData[0][i+1]);
    }
        #else

    lastSize    = sizeof(gpHudTableData[0]) % offsetSize;
    num         = sizeof(gpHudTableData[0]) / offsetSize;

    ithCmdQLock(ITH_CMDQ0_OFFSET);
    for (i = 0; i < num; i++)
    {
        // (void)printf("(HUDAddrData):0x%x\n", sizeof(HUDAddrData));//0xa4190 ////(14*5999+32)*2 = 168036
        cmdqAddr = ithCmdQWaitSize(offsetSize, ITH_CMDQ0_OFFSET);
        memcpy(cmdqAddr, pHUDAddrData + i * offsetSize / 4, offsetSize);
        ithCmdQFlush(cmdqAddr + offsetSize / 4, ITH_CMDQ0_OFFSET);
    }

    cmdqAddr = ithCmdQWaitSize(lastSize, ITH_CMDQ0_OFFSET);
    memcpy(cmdqAddr, pHUDAddrData + i * offsetSize / 4, lastSize);
    ithCmdQFlush(cmdqAddr + lastSize / 4, ITH_CMDQ0_OFFSET);

    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

        #endif
    return result;
}
    #endif
#endif

bool
_HwEngineCmdQBurstFire (
    GFX_HW_DEVICE * dev)
{
    bool result = false;

    return result;
}

#ifndef CFG_CHIP_FT

bool
_HwEngineCmdQFire (
    GFX_HW_DEVICE * dev)
{
    bool result = false;

    #ifndef GFX_USE_COMQ_BURST
    result  = _HwEngineCmdQSingleFire(dev);
    #else
    result  = _HwEngineCmdQBurstFire(dev);
    #endif

    return result;
}
    #ifdef CFG_M2D_HUD_ENABLE

bool
_HwEngineCmdQFireHUD (
    GFX_HW_DEVICE * dev)
{
    bool result = false;

    result = _HwEngineCmdQSingleFireHUD(dev);

    return result;
}
    #endif
#endif

#ifdef GFX_USE_INTERRUPT

static void
_IntrHandler (
    void * arg)
{
    ithReadRegA(GFX_REG_ISR_ADDR);// clear
    itpSemPostFromISR(&gfxMutex);
    // (void)printf("GFX isr!\n");
}

    #ifndef CFG_CHIP_FT

bool
_IntrCmdQSingleFire (
    void)
{
    bool        result          = true;
    uint32_t    requestCmdQLen  = 0;
    uint32_t    * cmdqAddr      = NULL;

    ithCmdQLock();

    cmdqAddr = ithCmdQWaitSize(8, ITH_CMDQ0_OFFSET);

    ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ID1_ADDR, 0x1);

    ithCmdQFlush(cmdqAddr, ITH_CMDQ0_OFFSET);

    ithCmdQUnlock();

    return result;
}
    #endif

#endif
