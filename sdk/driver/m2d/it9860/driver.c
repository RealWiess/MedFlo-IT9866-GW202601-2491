/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file driver.c
 *  GFX API function file.
 *
 * @author Awin Huang
 * @version 1.0
 * @date 2014-05-28
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include "gfx.h"
#include "hw.h"
#include "driver.h"
#include "gfx_math.h"
#include "ite/ith.h"
#include "msg.h"
#include "arm_lite_dev/armlite_dev_device.h"
#include "arm_lite_dev/hudBgFast/hudBgFast.h"

#ifdef CFG_BUILD_DEWARP
#include "dewarp.h"
#endif


// =============================================================================
//                              Extern Reference
// =============================================================================
#ifdef CFG_M2D_HUD_ENABLE
extern volatile uint32_t    gBlock_W;
extern volatile uint32_t    gBlock_H;
extern volatile uint32_t    gW_num;
extern volatile uint32_t    gH_num;
extern volatile uint32_t    gBlock_num;
extern volatile uint32_t    gHUDAddrDataNum;
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
static GFX_DRIVER GfxDriver = {PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP};

#ifdef CFG_LCD_PQ_TUNING
static ISP_DEVICE           gIspDev;
static MMP_ISP_CORE_INFO    coreInfo;
static MMP_ISP_SHARE        inputInfo;
static MMP_ISP_OUTPUT_INFO  outInfo;
#endif

#ifdef CPU2
extern uint32_t *gpHudTableData[3];//gpHudTableData[3][gHUDAddrDataNum];
extern uint32_t gHudTableIndex;
extern uint32_t gTableID;
#endif
// =============================================================================
//                              Private Function Declaration
// =============================================================================
static void
_gfxDriverReset (
    GFX_DRIVER * driver);

static GFXSurface *
_gfxDriverCreateDisplaySurface (
    uint32_t index);

// =============================================================================
//                              Public Function Definition
// =============================================================================
bool
gfxInitialize ()
{
    GFX_FUNC_ENTRY;

    // enable 2D/CMDQ clock
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0x02af8000, 0x02af8000);

#ifdef CFG_LCD_PQ_TUNING
    mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
    coreInfo.EnPreview      = false;
    coreInfo.PreScaleSel    = MMP_ISP_PRESCALE_NORMAL;
    mmpIspSetCore(gIspDev, &coreInfo);
    mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
    inputInfo.addrY         = ithLcdGetBaseAddrA();
    inputInfo.width         = ithLcdGetWidth();
    inputInfo.height        = ithLcdGetHeight();
    inputInfo.pitchY        = ithLcdGetPitch();
    inputInfo.format        = MMP_ISP_IN_RGB565;
    outInfo.width           = ithLcdGetWidth();
    outInfo.height          = ithLcdGetHeight();
    outInfo.pitchRGB        = ithLcdGetPitch();
    outInfo.format          = MMP_ISP_OUT_DITHER565A;
    outInfo.addrRGB         = ithLcdGetBaseAddrA();
    mmpIspSetOutputWindow(gIspDev, &outInfo);
    mmpIspSetVideoWindow(gIspDev, 0, 0, ithLcdGetWidth(), ithLcdGetHeight());
    mmpIspEnableCMDQ(gIspDev);
#endif

#ifndef CMDQ_IRQ_ENABLE
    ithLcdEnableHwFlip();
#endif

    if (GfxDriver.refCount == 0)
    {
        // Do this first, reset refCount inside.
        _gfxDriverReset(&GfxDriver);

        GfxDriver.refCount          = 1;
        GfxDriver.hwDev             = gfxHwInitialize();
        // Get display surface
        GfxDriver.displaySurfaceA   = _gfxDriverCreateDisplaySurface(0);
        GfxDriver.displaySurfaceB   = _gfxDriverCreateDisplaySurface(1);
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        GfxDriver.displaySurfaceC   = _gfxDriverCreateDisplaySurface(2);
#endif

#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        if (ithLcdGetFlip() == 0)
        {
            GfxDriver.displaySurfaceIndex = 1;
        }
        else if (ithLcdGetFlip() == 1)
        {
            GfxDriver.displaySurfaceIndex = 2;
        }
        else
        {
            GfxDriver.displaySurfaceIndex = 0;
        }
#else
        if (ithLcdGetFlip() == 0)
        {
            GfxDriver.displaySurfaceIndex = 1;
        }
        else
        {
            GfxDriver.displaySurfaceIndex = 0;
        }
#endif
    }
    else
    {
        GfxDriver.refCount++;
    }

#ifdef CFG_M2D_HUD_ENABLE

    #ifdef CFG_HUD_AUTO_GENERATE
    static struct timeval   tvStart = {0, 0}, tvEnd = {0, 0};
    int                     nTemp;

    gettimeofday(&tvStart, NULL);
    {   
        #ifdef CFG_BUILD_DEWARP
        char *warpTable="E:/800_480/warpTable.txt";
        if ((access(warpTable, F_OK) != -1)){
            Table destination;
            int err=0;
            float *arraywarp=table_read(warpTable);
            if(arraywarp!=NULL){
                err =HUDdewarpTable(NULL,arraywarp,&destination);
                printf("err = %d\n",err);
                if (destination.data!=NULL){
                    gfxGetCoordinate(NULL, NULL, (uint16_t*)destination.data, gfxGetHudRotateType());
                    free(destination.data);
                }
                free(arraywarp);   
            }
        }else
        #endif
        {
            gfxGetCoordinate(0, CFG_PRIVATE_DRIVE ":/hud/destination.txt", 0, gfxGetHudRotateType());
        }
    }
    gettimeofday(&tvEnd, NULL);
    nTemp = itpTimevalDiff(&tvStart, &tvEnd);
    (void)printf("HUD time:%d ms\n", nTemp);    // HUD time:162 ms
    gettimeofday(&tvStart, NULL);
    gfxHUDPreProcessing();
    gettimeofday(&tvEnd, NULL);
    nTemp = itpTimevalDiff(&tvStart, &tvEnd);
    (void)printf("HUD time:%d ms\n", nTemp);    // HUD time : 43 ms
    // HUD time:334 ms -> 205 ms
    #endif

#endif

    GFX_FUNC_LEAVE;

    return true;
}

void
gfxRegisterBlitIdCallback (
    int     objId,
    void    * pfCallback)
{
    gfxHwRegisterBlitIdCallback(objId, (gfxBlitIdCallback)pfCallback);
}

void
gfxTerminate ()
{
    GFX_FUNC_ENTRY;
    assert(GfxDriver.refCount > 0);

#ifdef CFG_LCD_PQ_TUNING
    mmpIspResetEngine(gIspDev);
    mmpIspTerminate(&gIspDev);
#endif

    if (--GfxDriver.refCount == 0)
    {
        _gfxDriverReset(&GfxDriver);
        gfxHwTerminate(GfxDriver.hwDev);

        // Terminate LCD buffer here!
        gfxDestroySurface(GfxDriver.displaySurfaceA);
        gfxDestroySurface(GfxDriver.displaySurfaceB);
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        gfxDestroySurface(GfxDriver.displaySurfaceC);
#endif
    }

    GFX_FUNC_LEAVE;
}

uint8_t
gfxGetDispSurfIndex (
    void)
{
    return GfxDriver.displaySurfaceIndex;
}

uint8_t
gfxGetBackSurfIndex (
    void)
{
    uint8_t surf_index;

#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    if (GfxDriver.displaySurfaceIndex == 0)
    {
        surf_index = 2;
    }
    else if (GfxDriver.displaySurfaceIndex == 1)
    {
        surf_index = 0;
    }
    else
    {
        surf_index = 1;
    }
#else
    surf_index = !GfxDriver.displaySurfaceIndex;
#endif
    return surf_index;
}

void
gfxSetDispSurfIndex (
    void)
{
    uint32_t lcd_index = ithLcdGetFlip();
    GFX_FUNC_ENTRY;
    gfxLock();

    switch (lcd_index)
    {
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        case 0:
            GfxDriver.displaySurfaceIndex = 1;
            break;

        case 1:
            GfxDriver.displaySurfaceIndex = 2;
            break;

        case 2:
            GfxDriver.displaySurfaceIndex = 0;
            break;

#else
        default:
        case 0:
            GfxDriver.displaySurfaceIndex = 1;
            break;

        case 1:
            GfxDriver.displaySurfaceIndex = 0;
            break;
#endif
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;
}

GFXSurface *
gfxGetDisplaySurface ()
{
    GFXSurface * currentDisplaySurface = NULL;
    GFX_FUNC_ENTRY;

    gfxLock();
    
    if (GfxDriver.displaySurfaceIndex == 0)
    {
        currentDisplaySurface = GfxDriver.displaySurfaceA;
    }
    else if (GfxDriver.displaySurfaceIndex == 1)
    {
        currentDisplaySurface = GfxDriver.displaySurfaceB;
    }
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    else
    {
        currentDisplaySurface = GfxDriver.displaySurfaceC;
    }
#endif

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return currentDisplaySurface;
}

GFXSurface *
gfxGetBackSurface ()
{
    GFXSurface * currentDisplaySurface = NULL;
    GFX_FUNC_ENTRY;

    gfxLock();

#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    if (GfxDriver.displaySurfaceIndex == 0)
    {
        currentDisplaySurface = GfxDriver.displaySurfaceC;
    }
    else if (GfxDriver.displaySurfaceIndex == 1)
    {
        currentDisplaySurface = GfxDriver.displaySurfaceA;
    }
    else
    {
        currentDisplaySurface = GfxDriver.displaySurfaceB;
    }
#else
    if (!GfxDriver.displaySurfaceIndex)
    {
        currentDisplaySurface = GfxDriver.displaySurfaceB;
    }
    else
    {
        currentDisplaySurface = GfxDriver.displaySurfaceA;
    }
#endif

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return currentDisplaySurface;
}

#ifndef CFG_CHIP_FT

bool
gfxFlip ()
{
    GFX_FUNC_ENTRY;
    gfxLock();

    #ifdef CFG_LCD_PQ_TUNING
    if (GfxDriver.displaySurfaceIndex == 0)
    {
        inputInfo.addrY = ithLcdGetBaseAddrA();
        outInfo.addrRGB = ithLcdGetBaseAddrA();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }
    else if (GfxDriver.displaySurfaceIndex == 1)
    {
        inputInfo.addrY = ithLcdGetBaseAddrB();
        outInfo.addrRGB = ithLcdGetBaseAddrB();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }
    else
    {
        inputInfo.addrY = ithLcdGetBaseAddrC();
        outInfo.addrRGB = ithLcdGetBaseAddrC();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }

    gfxwaitEngineIdle();

    pthread_mutex_lock(&ISP_CORE_0_MUTEX);
    mmpIspPlayImageProcess(gIspDev, &inputInfo);
    gfxwaitEngineIdle();
    mmpIspWaitEngineIdle(gIspDev);
    pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
    ithCmdQFlip(GfxDriver.displaySurfaceIndex, ITH_CMDQ0_OFFSET);
    #else

    #if defined(CMDQ_IRQ_ENABLE)
    ithLcdSwFlip(GfxDriver.displaySurfaceIndex);
    #else
    ithCmdQFlip(GfxDriver.displaySurfaceIndex, ITH_CMDQ0_OFFSET);
    #endif

    #endif

    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    GfxDriver.displaySurfaceIndex   = (GfxDriver.displaySurfaceIndex + 1) % 3;
    #else
    GfxDriver.displaySurfaceIndex   = !GfxDriver.displaySurfaceIndex;
    #endif

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return true;
}
#endif

#ifdef CFG_LCD_PQ_TUNING

bool
gfxPQTuning (
    MMP_ISP_COLOR_CTRL  colorCtrl,
    int                 value)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    gfxLock();

    if (mmpIspSetColorCtrl(gIspDev, colorCtrl, value) == ISP_SUCCESS)
    {
        result = true;
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}
#endif

#ifndef CFG_CHIP_FT

bool
gfxFlipWitBlitId (
    int objId,
    int val)
{
    GFX_FUNC_ENTRY;
    gfxLock();
    if (objId < LAST_GFX_ID)
    {
        uint32_t * cmdqAddr = NULL;

        cmdqAddr = ithCmdQWaitSize(ITH_CMDQ_SINGLE_CMD_SIZE * 2, ITH_CMDQ0_OFFSET);

        switch (objId)
        {
            case GFX_ID1:
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ICR_ADDR, 0x1);
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ID1_ADDR, val);
                break;

            case GFX_ID2:
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ICR_ADDR, 0x2);
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ID2_ADDR, val);
                break;

            case GFX_ID3:
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ICR_ADDR, 0x4);
                ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ID3_ADDR, val);
                break;
        }
        ithCmdQFlush(cmdqAddr, ITH_CMDQ0_OFFSET);
    }

    #ifdef CFG_LCD_PQ_TUNING
    if (GfxDriver.displaySurfaceIndex == 0)
    {
        inputInfo.addrY = ithLcdGetBaseAddrA();
        outInfo.addrRGB = ithLcdGetBaseAddrA();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }
    else if (GfxDriver.displaySurfaceIndex == 1)
    {
        inputInfo.addrY = ithLcdGetBaseAddrB();
        outInfo.addrRGB = ithLcdGetBaseAddrB();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }
    else
    {
        inputInfo.addrY = ithLcdGetBaseAddrC();
        outInfo.addrRGB = ithLcdGetBaseAddrC();
        mmpIspSetOutputWindow(gIspDev, &outInfo);
    }

    gfxwaitEngineIdle();

    pthread_mutex_lock(&ISP_CORE_0_MUTEX);
    mmpIspPlayImageProcess(gIspDev, &inputInfo);
    gfxwaitEngineIdle();
    mmpIspWaitEngineIdle(gIspDev);
    pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
    ithCmdQFlip(GfxDriver.displaySurfaceIndex, ITH_CMDQ0_OFFSET);
    #else

    #if defined(CMDQ_IRQ_ENABLE)
    ithLcdSwFlip(GfxDriver.displaySurfaceIndex);
    #else
    ithCmdQFlip(GfxDriver.displaySurfaceIndex, ITH_CMDQ0_OFFSET);
    #endif

    #endif

    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    GfxDriver.displaySurfaceIndex   = (GfxDriver.displaySurfaceIndex + 1) % 3;
    #else
    GfxDriver.displaySurfaceIndex   = !GfxDriver.displaySurfaceIndex;
    #endif

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return true;
}
#endif

void
gfxLock (
    void)
{
    pthread_mutex_lock(&GfxDriver.mutex);
}

void
gfxUnlock (
    void)
{
    pthread_mutex_unlock(&GfxDriver.mutex);
}

GFX_DRIVER *
gfxGetDriver ()
{
    return &GfxDriver;
}

#ifdef CFG_M2D_HUD_ENABLE

void
gfxHUDPreProcessing (
    void)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    int             i, j;
    GFXSurfaceSrc   src                 = {0};
    GFXSurfaceDst   surface             = {0};
    GFXSurface      * currDisplaySurf   = gfxGetDisplaySurface();
    uint32_t        * addr              = (uint32_t *)currDisplaySurf->imageData;

    src.srcSurface      = currDisplaySurf;
    src.srcX            = 0;
    src.srcY            = 0;
    src.srcWidth        = gBlock_W;
    src.srcHeight       = gBlock_H;

    surface.dstSurface  = currDisplaySurf;
    surface.dstX        = 0;
    surface.dstY        = 0;
    surface.dstWidth    = ithLcdGetWidth();    
    surface.dstHeight   = ithLcdGetHeight();   

    #ifdef HUD_HORIZONTAL
    for (j = 0; j < gH_num; j++)
    {
        for (i = 0; i < gW_num; i++)
        {
            currDisplaySurf->imageData = (uint8_t *)(addr + gBlock_W * 2 * gW_num * gBlock_H * j + gBlock_W * 2 * i);

            gfxSurfaceTransformWithMatrixPreProcessing(
                &surface,
                &src);
        }
    }
    #else
    for (i = 0; i < gW_num; i++)
    {
        for (j = 0; j < gH_num; j++)
        {
            currDisplaySurf->imageData = (uint8_t *)(addr + gBlock_W * 2 * gW_num * gBlock_H * j + gBlock_W * 2 * i);

            gfxSurfaceTransformWithMatrixPreProcessing(
                &surface,
                &src);
        }
    }
    #endif

    currDisplaySurf->imageData = (uint8_t *)addr;

    gfxUnlock();
    GFX_FUNC_LEAVE;
}

// void
// gfxHUDPreProcessingFish(GFXSurface* src, GFXSurface* dst)
// {
//    GFX_FUNC_ENTRY;
//    gfxLock();
//
//    GFXSurfaceSrc srcSurface = { 0 };
//    GFXSurfaceDst dstSurface = { 0 };
//    int i, j = 0;
//
//    for (i = 0; i < gW_num; i++)
//    {
//        for (j = 0; j < gH_num; j++)
//        {
//            dstSurface.dstSurface = dst;
//            dstSurface.dstX = 0;
//            dstSurface.dstY = 0;
//            dstSurface.dstWidth = ithLcdGetWidth();//800;
//            dstSurface.dstHeight = ithLcdGetHeight();//480;
//
//            srcSurface.srcSurface = src;
//            srcSurface.srcX = 0;
//            srcSurface.srcY = 0;
//            srcSurface.srcWidth = 1280;//
//            srcSurface.srcHeight = 960;//
//
//            gfxSurfaceTransformWithMatrixPreProcessingFish(
//                &dstSurface,
//                &srcSurface);
//        }
//    }
//
//    gfxUnlock();
//    GFX_FUNC_LEAVE;
// }
#endif

// =============================================================================
//                              Private Function Definition
// =============================================================================
void
_gfxDriverReset (
    GFX_DRIVER * driver)
{
    uint8_t     * driverPtr     = (uint8_t *)driver + sizeof(pthread_mutex_t);
    uint32_t    driverPtrSize   = sizeof(GFX_DRIVER) - sizeof(pthread_mutex_t);

    memset(driverPtr, 0x00, driverPtrSize);
}

GFXSurface *
_gfxDriverCreateDisplaySurface (
    uint32_t index)
{
    GFXSurface      * surface = NULL;
    GFXColorFormat  format;

    switch (ithLcdGetFormat())
    {
        case ITH_LCD_RGB565: format     = GFX_COLOR_RGB565;
            break;

        case ITH_LCD_ARGB1555: format   = GFX_COLOR_ARGB1555;
            break;

        case ITH_LCD_ARGB4444: format   = GFX_COLOR_ARGB4444;
            break;

        case ITH_LCD_ARGB8888: format   = GFX_COLOR_ARGB8888;
            break;

        default: // ITH_LCD_YUV
            format = GFX_COLOR_UNKNOWN;
    }

    switch (index)
    {
        case 0:
            surface = gfxCreateSurfaceByPointer(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), format, (uint8_t *)ithLcdGetBaseAddrA(), ithLcdGetHeight() * ithLcdGetPitch());
            break;

        case 1:
            surface = gfxCreateSurfaceByPointer(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), format, (uint8_t *)ithLcdGetBaseAddrB(), ithLcdGetHeight() * ithLcdGetPitch());
            break;
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)

        case 2:
            surface = gfxCreateSurfaceByPointer(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), format, (uint8_t *)ithLcdGetBaseAddrC(), ithLcdGetHeight() * ithLcdGetPitch());
            break;

#endif
        default:
            assert(0);
            break;
    }

    return surface;
}
