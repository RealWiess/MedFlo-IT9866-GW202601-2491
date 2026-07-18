/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file surface.c
 *  GFX application layer API function file.
 *
 * @author Awin Huang
 * @version 1.0
 * @date 2014-05-16
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <malloc.h>
#include "ite/itp.h"
#include "gfx.h"
#include "driver.h"
#include "gfx_math.h"
#include "gfx_mem.h"
#include "msg.h"
#include "hw.h"
#include "ith/ith_cmdq.h"

// =============================================================================
//                              Extern Reference
// =============================================================================
// extern GFX_DRIVER* gfxGetDriver();

// =============================================================================
//                              Macro Definition
// =============================================================================
#undef MIN
#undef MAX
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

#define GENERAL_ALGORITHM

#define REG_FILL_SRC_SURFACE(regs, srcSurface, srcX, srcY, srcWidth, srcHeight) \
    {   \
        regs->regSRC.data   = (uint32_t)srcSurface->imageData;    \
        regs->regSPR.data   = srcSurface->pitch;  \
        regs->regSRCXY.data = ((((uint32_t)srcX) & GFX_REG_SRCXY_SRCX_MASK) << GFX_REG_SRCXY_SRCX_OFFSET) | ((((uint32_t)srcY) & GFX_REG_SRCXY_SRCY_MASK) << GFX_REG_SRCXY_SRCY_OFFSET);    \
        regs->regSHWR.data  = ((((uint32_t)srcWidth) & GFX_REG_SHWR_SRCWIDTH_MASK) << GFX_REG_SHWR_SRCWIDTH_OFFSET) | ((((uint32_t)srcHeight) & GFX_REG_SHWR_SRCHEIGHT_MASK) << GFX_REG_SHWR_SRCHEIGHT_OFFSET);  \
        regs->regCR2.data  &= ~GFX_REG_CR2_SRCFORMAT_MASK;   \
        regs->regCR2.data   |= ((uint32_t)srcSurface->format << GFX_REG_CR2_SRCFORMAT_OFFSET);  \
    }

#define REG_FILL_DST_SURFACE(regs, dstSurface, dstX, dstY, dstWidth, dstHeight) \
    {   \
        regs->regDST.data   = (uint32_t)dstSurface->imageData;    \
        regs->regDPR.data   = dstSurface->pitch;  \
        regs->regDSTXY.data = ((((uint32_t)dstX) & GFX_REG_SRCXY_SRCX_MASK) << GFX_REG_SRCXY_SRCX_OFFSET) | ((((uint32_t)dstY) & GFX_REG_SRCXY_SRCY_MASK) << GFX_REG_SRCXY_SRCY_OFFSET);    \
        regs->regDHWR.data  = ((((uint32_t)dstWidth) & GFX_REG_SHWR_SRCWIDTH_MASK) << GFX_REG_SHWR_SRCWIDTH_OFFSET) | ((((uint32_t)dstHeight) & GFX_REG_SHWR_SRCHEIGHT_MASK) << GFX_REG_SHWR_SRCHEIGHT_OFFSET);  \
        regs->regCR2.data  &= ~GFX_REG_CR2_DSTFORMAT_MASK;   \
        regs->regCR2.data   |= ((uint32_t)dstSurface->format << GFX_REG_CR2_DSTFORMAT_OFFSET);  \
    }

#define REG_FILL_MASK_SURFACE(regs, dstSurface, maskX, maskY, maskWidth, maskHeight) \
    {   \
        if ((dstSurface != 0) && dstSurface->maskEnable && (dstSurface->maskSurface != 0)) \
        {   \
            regs->regMASK.data   = (uint32_t)dstSurface->maskSurface->imageData; \
            regs->regMPR.data    = dstSurface->maskSurface->pitch; \
            regs->regMASKXY.data    = ((((uint32_t)maskX) & GFX_REG_MASKXY_MASKX_MASK) << GFX_REG_MASKXY_MASKX_OFFSET) | ((((uint32_t)maskY) & GFX_REG_MASKXY_MASKY_MASK) << GFX_REG_MASKXY_MASKY_OFFSET);    \
            regs->regMHWR.data      = ((((uint32_t)maskWidth) & GFX_REG_MHWR_MASKWIDTH_MASK) << GFX_REG_MHWR_MASKWIDTH_OFFSET) | ((((uint32_t)maskHeight) & GFX_REG_MHWR_MASKHEIGHT_MASK) << GFX_REG_MHWR_MASKHEIGHT_OFFSET);  \
            regs->regCR2.data   &= ~GFX_REG_CR2_MSKFORMAT_MASK; \
            regs->regCR2.data       |= ((uint32_t)dstSurface->maskSurface->format << GFX_REG_CR2_MSKFORMAT_OFFSET); \
            regs->regCR1.data   |= GFX_REG_CR1_MASK_EN; \
        }   \
    }

#define REG_FILL_DST_CLIP(regs, clipSX, clipSY, clipEX, clipEY) \
    {   \
        regs->regPXCR.data  = ((((uint32_t)clipSX) & GFX_REG_PXCR_CLIPXSTART_MASK) << GFX_REG_PXCR_CLIPXSTART_OFFSET) | ((((uint32_t)clipEX) & GFX_REG_PXCR_CLIPXEND_MASK) << GFX_REG_PXCR_CLIPXEND_OFFSET); \
        regs->regPYCR.data  = ((((uint32_t)clipSY) & GFX_REG_PYCR_CLIPYSTART_MASK) << GFX_REG_PYCR_CLIPYSTART_OFFSET) | ((((uint32_t)clipEY) & GFX_REG_PYCR_CLIPYEND_MASK) << GFX_REG_PYCR_CLIPYEND_OFFSET); \
    }

#define REG_FILL_SET_ITM(regs, mtx) \
    {   \
        regs->regITMR00.data    = (int32_t)(mtx.m[0][0] * (1 << 19)); \
        regs->regITMR01.data    = (int32_t)(mtx.m[0][1] * (1 << 19)); \
        regs->regITMR02.data    = (int32_t)(mtx.m[0][2] * (1 << 19)); \
        regs->regITMR10.data    = (int32_t)(mtx.m[1][0] * (1 << 19)); \
        regs->regITMR11.data    = (int32_t)(mtx.m[1][1] * (1 << 19)); \
        regs->regITMR12.data    = (int32_t)(mtx.m[1][2] * (1 << 19)); \
        regs->regITMR20.data    = (int32_t)(mtx.m[2][0] * (1 << 19)); \
        regs->regITMR21.data    = (int32_t)(mtx.m[2][1] * (1 << 19)); \
        regs->regITMR22.data    = (int32_t)(mtx.m[2][2] * (1 << 19)); \
    }

#define REG_FILL_FGCOLOR(regs, color)           \
    {                                               \
        regs->regFGCOLOR.data =                     \
        (color.a << GFX_REG_FGCOLOR_A_OFFSET) \
        | (color.r << GFX_REG_FGCOLOR_R_OFFSET) \
        | (color.g << GFX_REG_FGCOLOR_G_OFFSET) \
        | (color.b << GFX_REG_FGCOLOR_B_OFFSET); \
    }

#define REG_FILL_ROP3(regs, ropValue)                   \
    {                                                       \
        regs->regCAR.data &= ~GFX_REG_CAR_ROP_ROP3_MASK;    \
        regs->regCAR.data   |= (uint32_t)ropValue;                      \
    }

#define REG_FILL_BLENDING(regs, enableAlpha, enableConstantAlpha, constantAlphaValue)   \
    {                                                           \
        if (enableAlpha == true) {                              \
            regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;     \
        }                                                       \
        if (enableConstantAlpha == true) {                      \
            regs->regCR1.data   |= GFX_REG_CR1_ALPHABLEND_EN;     \
            regs->regCR1.data   |= GFX_REG_CR1_CONSTALPHA_EN;     \
            regs->regCAR.data   |= (constantAlphaValue & GFX_REG_CAR_CONSTALPHA_MASK) << GFX_REG_CAR_CONSTALPHA_OFFSET;    \
        }                                                       \
    }

#define REG_FILL_LINEDRAW_STARTXY(x, y) \
    {                                                           \
        regs->regSRCXY.data = ((((uint32_t)x) & GFX_REG_SRCXY_SRCX_MASK) << GFX_REG_SRCXY_SRCX_OFFSET) | (((uint32_t)y) & GFX_REG_SRCXY_SRCY_MASK); \
    }

#define REG_FILL_LINEDRAW_ENDXY(x, y) \
    {                                                           \
        regs->regLNER.data = ((((uint32_t)x) & GFX_REG_LNEXY_LNEX_MASK) << GFX_REG_LNEXY_LNEX_OFFSET) | (((uint32_t)y) & GFX_REG_LNEXY_LNEY_MASK); \
    }

// =============================================================================
//                              Structure Definition
// =============================================================================

// =============================================================================
//                              Global Data Definition
// =============================================================================
#ifdef CFG_M2D_HUD_ENABLE
extern volatile uint32_t    gBlock_W;
extern volatile uint32_t    gBlock_H;
extern volatile uint32_t    gW_num;
extern volatile uint32_t    gH_num;
extern volatile uint32_t    gBlock_num;
extern volatile uint32_t    gHUDAddrDataNum;

extern GFX_MATRIX           * transform_m;
extern GFX_DST_SURFACE_INFO * dstInfo;
extern GFX_DST_SURFACE_INFO * srcInfo;
extern uint32_t             * HUDAddrData;
extern uint32_t             * gpHudTableData[3];
extern int                  gTempHudIndex;

uint32_t                    num         = 0;

static uint32_t             addrDataNum = 0;
static FILE                 * fd        = NULL;
#endif
// =============================================================================
//                              Private Function Declaration
// =============================================================================
static bool
_SurfaceBoundaryCheck (
    GFXSurface  * surface,
    int         clipX,
    int         clipY,
    int         clipWidth,
    int         clipHeight);

static GFXSurface *
_gfxCreateSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXColorFormat  format);

static GFXMaskSurface *
_gfxCreateMaskSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXMaskFormat   format);

// TODO
#if 0
static bool
_SurfaceDrawSurface (
    GFXSurface  * dstSurface,
    int32_t     dstX,
    int32_t     dstY,
    int32_t     dstWidth,
    int32_t     dstHeight,
    GFXSurface  * srcSurface,
    int32_t     srcX,
    int32_t     srcY,
    int32_t     srcWidth,
    int32_t     srcHeight,
    GFXSurface  * maskSurface,
    bool        enableMask,
    int32_t     maskX,
    int32_t     maskY,
    int32_t     maskWidth,
    int32_t     maskHeight,
    bool        enableSrcColorKey,
    GFXColor    srcColorKey,
    bool        enableDstColorKey,
    GFXColor    dstColorKey,
    bool        enableAlphablending,
    bool        enableConstantAlpha,
    GFXColor    contantAlphaValue,
    float       rotateDegree);
#endif

static bool
_SurfaceBitblt (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceBitbltSelDstAlpha (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceTransform (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    GFXTileMode     tilemode,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceFillColor (
    GFXSurfaceDst   * surface,
    GFXColor        color,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceGradientFill (
    GFXSurfaceDst   * surface,
    GFXColor        initcolor,
    GFXColor        endcolor,
    GFXGradDir      dir,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceProjection (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    float           FOV,
    float           pivotX,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceTransformBlt (
    GFXSurface          * dest,
    int32_t             dx,
    int32_t             dy,
    GFXSurface          * src,
    int32_t             sx,
    int32_t             sy,
    int32_t             sw,
    int32_t             sh,
    int32_t             x0,
    int32_t             y0,
    int32_t             x1,
    int32_t             y1,
    int32_t             x2,
    int32_t             y2,
    int32_t             x3,
    int32_t             y3,
    bool                bInverse,
    GFXPageFlowType     type,
    GFXTransformType    transformType);

static void
_setFGColor (
    GFXSurface  * dstSurface,
    GFXColor    color);

static bool
_SurfaceDrawLine (
    GFXSurface      * surface,
    int32_t         fromX,
    int32_t         fromY,
    int32_t         toX,
    int32_t         toY,
    GFXColor        * lineColor,
    int32_t         lineWidth,
    GFXAlphaBlend   * alphaBlend);

static bool
_SurfaceDrawPixelAA (
    GFXSurface  * surface,
    int         x,
    int         y,
    GFXColor    * color,
    int         lineWidth);

static bool
_SurfaceReflected (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy,
    int32_t     reflectedWidth,
    int32_t     reflectedHeight);

static bool
_SurfaceMirror (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy);

static bool
_SurfaceShearing (
    GFXSurfaceDst   * dstSurface,
    GFX_RECTANGLE   coordinate,
    GFXSurfaceSrc   * srcSurface,
    GFXAlphaBlend *alphaBlend);

#ifdef CFG_M2D_HUD_ENABLE
static bool
_SurfaceTransformWithMatrix (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface);

static bool
_SurfaceTransformWithMatrixPreProcessing (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface);

static bool
_SurfaceTransformWithMatrixPreProcessingFish (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface);
#endif

static bool
_SurfaceTransformGetVersion (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface);

// =============================================================================
//                              Public Function Definition
// =============================================================================
GFXSurface *
gfxCreateSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXColorFormat  format)
{
    GFXSurface * surface = NULL;

    GFX_FUNC_ENTRY;
    gfxLock();

    surface = _gfxCreateSurface(width, height, pitch, format);
    if (surface == NULL)
    {
        GFX_ERROR_MSG("Create surface fail.\n");
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return surface;
}

GFXSurface *
gfxCreateSurfaceByBuffer (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXColorFormat  format,
    uint8_t         * buffer,
    uint32_t        bufferLength)
{
    GFXSurface * surface = NULL;

    GFX_FUNC_ENTRY;
    gfxLock();
    do
    {
        uint8_t * mappedSysRam = NULL;
        surface = _gfxCreateSurface(width, height, pitch, format);
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Create surface fail.\n");
            break;
        }

        // coverity[NULL_FIELD: FALSE]
        mappedSysRam = (uint8_t *)ithMapVram((uint32_t)surface->imageData, bufferLength, ITH_VRAM_WRITE);
        (void)memcpy(mappedSysRam, buffer, surface->imageDataLength);
        ithFlushDCacheRange((void *)mappedSysRam, bufferLength);
        ithFlushMemBuffer();
        ithUnmapVram((void *)mappedSysRam, bufferLength);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return surface;
}

GFXSurface *
gfxCreateSurfaceByPointer (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXColorFormat  format,
    uint8_t         * alreadyExistPtr,
    uint32_t        ptrLength)
{
    GFXSurface * surface = NULL;

    GFX_FUNC_ENTRY;
    gfxLock();

    do
    {
#ifdef WIN32
        surface = (GFXSurface *)malloc(sizeof(GFXSurface));
#else
        surface = (GFXSurface *)memalign(sizeof(int), sizeof(GFXSurface));
#endif
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Create surface fail.\n");
            break;
        }
        surface->width              = (int32_t)width;
        surface->height             = (int32_t)height;
        surface->pitch              = pitch;
        surface->format             = format;
        surface->imageDataLength    = ptrLength;
        surface->imageData          = alreadyExistPtr;
        surface->imageDataOwner     = false;
        surface->maskEnable         = false;
        surface->maskSurface        = NULL;
        surface->clipEnable         = false;
        surface->quality            = GFX_QUALITY_BETTER;
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return surface;
}

void
gfxDestroySurface (
    GFXSurface * surface)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    if (surface != NULL)
    {
        (void)gfxwaitEngineIdle();
        if ((surface->imageDataOwner == true)
            && (surface->imageData != 0))
        {
            // (void)printf("Gfx free %x %d\n", surface->imageData, surface->imageDataOwner);
            gfxVmemFree(surface->imageData);
            surface->imageData = NULL;
        }
        free(surface);
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;
}

int
gfxSurfaceGetWidth (
    GFXSurface * surface)
{
    int w = 0;

    GFX_FUNC_ENTRY;

    if (surface != NULL)
    {
        w = surface->width;
    }

    GFX_FUNC_LEAVE;

    return w;
}

int
gfxSurfaceGetHeight (
    GFXSurface * surface)
{
    int h = 0;

    GFX_FUNC_ENTRY;

    if (surface != NULL)
    {
        h = surface->height;
    }

    GFX_FUNC_LEAVE;

    return h;
}

GFXColorFormat
gfxSurfaceGetFormat (
    GFXSurface * surface)
{
    GFXColorFormat format = GFX_COLOR_UNKNOWN;

    GFX_FUNC_ENTRY;

    if (surface != NULL)
    {
        format = surface->format;
    }

    GFX_FUNC_LEAVE;

    return format;
}

void
gfxSurfaceSetSurfaceBaseAddress (
    GFXSurface  * surface,
    uint32_t    addr)
{
    surface->imageData = (void *)addr;
}

void
gfxSurfaceSetWidth (
    GFXSurface  * surface,
    uint32_t    width)
{
    surface->width = (int32_t)width;
}

void
gfxSurfaceSetHeight (
    GFXSurface  * surface,
    uint32_t    height)
{
    surface->height = (int32_t)height;
}

void
gfxSurfaceSetPitch (
    GFXSurface  * surface,
    uint32_t    pitch)
{
    surface->pitch = pitch;
}

bool
gfxSurfaceDrawGlyph (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFXColor        color)
{
    bool            result;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    do
    {
        int32_t actualSrcWidth  = srcSurface->srcWidth;
        int32_t actualSrcHeight = srcSurface->srcHeight;

        if (srcSurface->srcSurface != NULL)
        {
            if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
            {
                actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
            }

            if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
            {
                actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
            }
        }

        {
            if ((dstX + actualSrcWidth) > dstSurface->width)
            {
                actualSrcWidth = dstSurface->width - dstX;
            }

            if ((dstY + actualSrcHeight) > dstSurface->height)
            {
                actualSrcHeight = dstSurface->height - dstY;
            }
        }

        _setFGColor(dstSurface, color);
        // (void)printf("dstX:%d,dstY:%d\n",dstX,dstY);
        srcSurface->srcWidth    = actualSrcWidth;
        srcSurface->srcHeight   = actualSrcHeight;

        alphaBlend.enableAlpha  = true;             // Enable Alpha Blending

        if (color.a == 0xFF)
        {
            alphaBlend.enableConstantAlpha  = false;
            alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care
        }
        else
        {
            alphaBlend.enableConstantAlpha  = true;
            alphaBlend.constantAlphaValue   = color.a;
        }

        result = _SurfaceBitblt(
            dstSurface,
            dstX, dstY,
            srcSurface,
            GFX_ROP3_SRCCOPY,
            &alphaBlend);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceBitblt (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface)
{
    return gfxSurfaceBitbltWithRop(
        dstSurface,
        dstX,
        dstY,
        srcSurface,
        GFX_ROP3_SRCCOPY);
}

bool
gfxSurfaceBitbltWithRop (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFX_ROP3        rop)
{
    bool            result;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    do
    {
        int32_t actualSrcWidth  = srcSurface->srcWidth;
        int32_t actualSrcHeight = srcSurface->srcHeight;

        if (srcSurface->srcSurface != NULL)
        {
            if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
            {
                actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
            }

            if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
            {
                actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
            }
        }

        {
            if ((dstX + actualSrcWidth) > dstSurface->width)
            {
                actualSrcWidth = dstSurface->width - dstX;
            }

            if ((dstY + actualSrcHeight) > dstSurface->height)
            {
                actualSrcHeight = dstSurface->height - dstY;
            }
        }

        srcSurface->srcWidth            = actualSrcWidth;
        srcSurface->srcHeight           = actualSrcHeight;

        alphaBlend.enableAlpha          = false;    // No alpha blending
        alphaBlend.enableConstantAlpha  = false;
        alphaBlend.constantAlphaValue   = 0;        // No constant alpha, 0 for don't care

        result                          = _SurfaceBitblt(
            dstSurface,
            dstX, dstY,
            srcSurface,
            rop,
            &alphaBlend);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceTransform (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    GFXTileMode     tilemode,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend)
{
    bool    result          = false;
    int32_t actualSrcWidth  = srcSurface->srcWidth;
    int32_t actualSrcHeight = srcSurface->srcHeight;

    GFX_FUNC_ENTRY;

    if (dstSurface->dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    srcSurface->srcWidth    = actualSrcWidth;
    srcSurface->srcHeight   = actualSrcHeight;

    result                  = _SurfaceTransform(
        dstSurface,
        srcSurface,
        refX,
        refY,
        scaleWidth,
        scaleHeight,
        degree,
        tilemode,
        rop,
        alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceBitbltWithRotate (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           degree)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    GFXSurfaceDst   DstSurface      = {0};
    GFXAlphaBlend   alphaBlend      = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    srcSurface->srcWidth            = actualSrcWidth;
    srcSurface->srcHeight           = actualSrcHeight;

    DstSurface.dstSurface           = dstSurface;
    DstSurface.dstX                 = dstX;
    DstSurface.dstY                 = dstY;
    DstSurface.dstWidth             = dstSurface->width;
    DstSurface.dstHeight            = dstSurface->height;

    alphaBlend.enableAlpha          = false;    // No alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;        // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        &DstSurface,
        srcSurface,
        refX,
        refY,
        1.0f,
        1.0f,
        degree,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceStrectch (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    int32_t         actualDstWidth  = dstSurface->dstWidth;
    int32_t         actualDstHeight = dstSurface->dstHeight;

    float           scaleWidth      = (float) dstSurface->dstWidth / srcSurface->srcWidth;
    float           scaleHeight     = (float) dstSurface->dstHeight / srcSurface->srcHeight;

    GFXAlphaBlend   alphaBlend      = {0};
    GFXSurface      * surf          = 0;

    GFX_FUNC_ENTRY;

    if (dstSurface->dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    if (dstSurface->dstSurface != NULL)
    {
        if ((dstSurface->dstX + dstSurface->dstWidth) > dstSurface->dstSurface->width)
        {
            actualDstWidth = dstSurface->dstSurface->width - dstSurface->dstX;
        }

        if ((dstSurface->dstY + dstSurface->dstHeight) > dstSurface->dstSurface->height)
        {
            actualDstHeight = dstSurface->dstSurface->height - dstSurface->dstY;
        }
    }

    srcSurface->srcWidth    = actualSrcWidth;
    srcSurface->srcHeight   = actualSrcHeight;
    dstSurface->dstWidth    = actualDstWidth;
    dstSurface->dstHeight   = actualDstHeight;

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX != 0) || (srcSurface->srcY != 0))
        {
            uint8_t bpp         = 0;
            uint8_t * lcdAddr   = 0;
            int     size        = ((int32_t)srcSurface->srcSurface->pitch) * actualSrcHeight;

            if (srcSurface->srcSurface->format == GFX_COLOR_ARGB8888)
            {
                bpp = 4;
            }
            else
            {
                bpp = 2;
            }

            lcdAddr                 = srcSurface->srcSurface->imageData + (srcSurface->srcY * srcSurface->srcSurface->pitch) + (srcSurface->srcX * bpp);

            gfxUnlock();
            surf                    = gfxCreateSurfaceByPointer((uint32_t)actualSrcWidth, (uint32_t)actualSrcHeight, srcSurface->srcSurface->pitch, srcSurface->srcSurface->format, (uint8_t *)lcdAddr, (uint32_t)size);
            gfxLock();
            srcSurface->srcSurface  = surf;
            srcSurface->srcX        = 0;
            srcSurface->srcY        = 0;
        }
    }

    alphaBlend.enableAlpha          = true; // No alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        dstSurface,
        srcSurface,
        0,
        0,
        (float)scaleWidth,
        (float)scaleHeight,
        0.0f,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);   // no alpha & const alpha

    if (surf != NULL)
    {
        gfxUnlock();
        GFX_FUNC_LEAVE;
        gfxDestroySurface(surf);
    }
    else
    {
        gfxUnlock();
        GFX_FUNC_LEAVE;
    }

    return result;
}

bool
gfxSurfaceStrectchWithRotate (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           degree)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    int32_t         actualDstWidth  = dstSurface->dstWidth;
    int32_t         actualDstHeight = dstSurface->dstHeight;

    float           scaleWidth      = (float) dstSurface->dstWidth / srcSurface->srcWidth;
    float           scaleHeight     = (float) dstSurface->dstHeight / srcSurface->srcHeight;

    GFXAlphaBlend   alphaBlend      = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface->dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    if (dstSurface->dstSurface != NULL)
    {
        if ((dstSurface->dstX + dstSurface->dstWidth) > dstSurface->dstSurface->width)
        {
            actualDstWidth = dstSurface->dstSurface->width - dstSurface->dstX;
        }

        if ((dstSurface->dstY + dstSurface->dstHeight) > dstSurface->dstSurface->height)
        {
            actualDstHeight = dstSurface->dstSurface->height - dstSurface->dstY;
        }
    }

    srcSurface->srcWidth            = actualSrcWidth;
    srcSurface->srcHeight           = actualSrcHeight;
    dstSurface->dstWidth            = actualDstWidth;
    dstSurface->dstHeight           = actualDstHeight;

    alphaBlend.enableAlpha          = true; // No alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        dstSurface,
        srcSurface,
        0,
        0,
        (float)scaleWidth,
        (float)scaleHeight,
        degree,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);   // no alpha & const alpha

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceAlphaBlend (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface)
{
    bool            result;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    do
    {
        int32_t actualSrcWidth  = srcSurface->srcWidth;
        int32_t actualSrcHeight = srcSurface->srcHeight;

        if (srcSurface->srcSurface != NULL)
        {
            if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
            {
                actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
            }

            if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
            {
                actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
            }
        }

        {
            if ((dstX + actualSrcWidth) > dstSurface->width)
            {
                actualSrcWidth = dstSurface->width - dstX;
            }

            if ((dstY + actualSrcHeight) > dstSurface->height)
            {
                actualSrcHeight = dstSurface->height - dstY;
            }
        }

        srcSurface->srcWidth            = actualSrcWidth;
        srcSurface->srcHeight           = actualSrcHeight;

        alphaBlend.enableAlpha          = true; // use alpha blending
        alphaBlend.enableConstantAlpha  = false;
        alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

        result                          = _SurfaceBitblt(
            dstSurface,
            dstX, dstY,
            srcSurface,
            GFX_ROP3_SRCCOPY,
            &alphaBlend);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceAlphaBlendEx (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    bool            enableConstantAlpha,
    uint8_t         constantAlphaValue)
{
    bool            result;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    do
    {
        int32_t actualSrcWidth  = srcSurface->srcWidth;
        int32_t actualSrcHeight = srcSurface->srcHeight;

        if (srcSurface->srcSurface != NULL)
        {
            if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
            {
                actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
            }

            if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
            {
                actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
            }
        }

        {
            if ((dstX + actualSrcWidth) > dstSurface->width)
            {
                actualSrcWidth = dstSurface->width - dstX;
            }

            if ((dstY + actualSrcHeight) > dstSurface->height)
            {
                actualSrcHeight = dstSurface->height - dstY;
            }
        }

        srcSurface->srcWidth            = actualSrcWidth;
        srcSurface->srcHeight           = actualSrcHeight;

        alphaBlend.enableAlpha          = true; // use alpha blending
        alphaBlend.enableConstantAlpha  = enableConstantAlpha;
        alphaBlend.constantAlphaValue   = constantAlphaValue;

        result                          = _SurfaceBitblt(
            dstSurface,
            dstX, dstY,
            srcSurface,
            GFX_ROP3_SRCCOPY,
            &alphaBlend);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceAlphaBlendSelDstAlpha (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface)
{
    bool            result;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    do
    {
        int32_t actualSrcWidth  = srcSurface->srcWidth;
        int32_t actualSrcHeight = srcSurface->srcHeight;

        if (srcSurface->srcSurface != NULL)
        {
            if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
            {
                actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
            }

            if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
            {
                actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
            }
        }

        {
            if ((dstX + actualSrcWidth) > dstSurface->width)
            {
                actualSrcWidth = dstSurface->width - dstX;
            }

            if ((dstY + actualSrcHeight) > dstSurface->height)
            {
                actualSrcHeight = dstSurface->height - dstY;
            }
        }

        srcSurface->srcWidth            = actualSrcWidth;
        srcSurface->srcHeight           = actualSrcHeight;

        alphaBlend.enableAlpha = false; // use alpha blending
        alphaBlend.enableConstantAlpha  = false;
        alphaBlend.constantAlphaValue = 0xff;    // No constant alpha, 0 for don't care

        result                          = _SurfaceBitbltSelDstAlpha(
            dstSurface,
            dstX, dstY,
            srcSurface,
            GFX_ROP3_SRCCOPY,
            &alphaBlend);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceAlphaBlendWithRotate (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           degree)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    GFXSurfaceDst   DstSurface      = {0};
    GFXAlphaBlend   alphaBlend      = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    srcSurface->srcWidth            = actualSrcWidth;
    srcSurface->srcHeight           = actualSrcHeight;

    DstSurface.dstSurface           = dstSurface;
    DstSurface.dstX                 = dstX;
    DstSurface.dstY                 = dstY;
    DstSurface.dstWidth             = dstSurface->width;
    DstSurface.dstHeight            = dstSurface->height;

    alphaBlend.enableAlpha          = true; // enable alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        &DstSurface,
        srcSurface,
        refX,
        refY,
        1.0f,
        1.0f,
        degree,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceStrectchAlphaBlend (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    int32_t         actualDstWidth  = dstSurface->dstWidth;
    int32_t         actualDstHeight = dstSurface->dstHeight;

    float           scaleWidth      = (float) dstSurface->dstWidth / srcSurface->srcWidth;
    float           scaleHeight     = (float) dstSurface->dstHeight / srcSurface->srcHeight;
    GFXAlphaBlend   alphaBlend      = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface->dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    if (dstSurface->dstSurface != NULL)
    {
        if ((dstSurface->dstX + dstSurface->dstWidth) > dstSurface->dstSurface->width)
        {
            actualDstWidth = dstSurface->dstSurface->width - dstSurface->dstX;
        }

        if ((dstSurface->dstY + dstSurface->dstHeight) > dstSurface->dstSurface->height)
        {
            actualDstHeight = dstSurface->dstSurface->height - dstSurface->dstY;
        }
    }

    srcSurface->srcWidth            = actualSrcWidth;
    srcSurface->srcHeight           = actualSrcHeight;
    dstSurface->dstWidth            = actualDstWidth;
    dstSurface->dstHeight           = actualDstHeight;

    alphaBlend.enableAlpha          = true; // use alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        dstSurface,
        srcSurface,
        0,
        0,
        (float)scaleWidth,
        (float)scaleHeight,
        0.0f,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceStrectchAlphaBlendWithRotate (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           degree)
{
    bool            result          = false;
    int32_t         actualSrcWidth  = srcSurface->srcWidth;
    int32_t         actualSrcHeight = srcSurface->srcHeight;
    int32_t         actualDstWidth  = dstSurface->dstWidth;
    int32_t         actualDstHeight = dstSurface->dstHeight;

    float           scaleWidth      = (float) dstSurface->dstWidth / srcSurface->srcWidth;
    float           scaleHeight     = (float) dstSurface->dstHeight / srcSurface->srcHeight;

    GFXAlphaBlend   alphaBlend      = {0};

    GFX_FUNC_ENTRY;

    if (dstSurface->dstSurface != NULL)
    {
        // Check
        if (_SurfaceBoundaryCheck(dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (srcSurface->srcSurface != NULL)
    {
        if (_SurfaceBoundaryCheck(srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight) == false)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    gfxLock();

    if (srcSurface->srcSurface != NULL)
    {
        if ((srcSurface->srcX + srcSurface->srcWidth) > srcSurface->srcSurface->width)
        {
            actualSrcWidth = srcSurface->srcSurface->width - srcSurface->srcX;
        }

        if ((srcSurface->srcY + srcSurface->srcHeight) > srcSurface->srcSurface->height)
        {
            actualSrcHeight = srcSurface->srcSurface->height - srcSurface->srcY;
        }
    }

    if (dstSurface->dstSurface != NULL)
    {
        if ((dstSurface->dstX + dstSurface->dstWidth) > dstSurface->dstSurface->width)
        {
            actualDstWidth = dstSurface->dstSurface->width - dstSurface->dstX;
        }

        if ((dstSurface->dstY + dstSurface->dstHeight) > dstSurface->dstSurface->height)
        {
            actualDstHeight = dstSurface->dstSurface->height - dstSurface->dstY;
        }
    }

    srcSurface->srcWidth            = actualSrcWidth;
    srcSurface->srcHeight           = actualSrcHeight;
    dstSurface->dstWidth            = actualDstWidth;
    dstSurface->dstHeight           = actualDstHeight;

    alphaBlend.enableAlpha          = true; // use alpha blending
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;    // No constant alpha, 0 for don't care

    result                          = _SurfaceTransform(
        dstSurface,
        srcSurface,
        0,
        0,
        (float)scaleWidth,
        (float)scaleHeight,
        degree,
        GFX_TILE_FILL,
        GFX_ROP3_SRCCOPY,
        &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceFillColor (
    GFXSurfaceDst   * dstSurface,
    GFXColor        color)
{
    bool            result      = false;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;
    if (dstSurface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    alphaBlend.enableAlpha          = false;
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;

    result                          = _SurfaceFillColor(dstSurface,
                                                       color,
                                                       &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceFillColorWithBlend(
    GFXSurfaceDst   * dstSurface,
    GFXColor        color,
    GFXAlphaBlend   * alphaBlend)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    if (dstSurface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    result = _SurfaceFillColor(dstSurface,
                               color,
                               alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceGradientFill (
    GFXSurfaceDst   *dstSurface,
    GFXColor        initcolor,
    GFXColor        endcolor,
    GFXGradDir      dir)
{
    bool            result      = false;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;
    if (dstSurface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    alphaBlend.enableAlpha          = false;
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;

    result                          = _SurfaceGradientFill(dstSurface,
                                                          initcolor,
                                                          endcolor,
                                                          dir,
                                                          &alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceGradientFillWithBlend (
    GFXSurfaceDst   *dstSurface,
    GFXColor        initcolor,
    GFXColor        endcolor,
    GFXGradDir      dir,
    GFXAlphaBlend   * alphaBlend)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    if (dstSurface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    result = _SurfaceGradientFill(dstSurface,
                                  initcolor,
                                  endcolor,
                                  dir,
                                  alphaBlend);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceProjection (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    float           FOV,
    float           pivotX,
    GFXAlphaBlend   * alphaBlend)
{
    bool result = false;

    if (dstSurface == NULL || srcSurface == NULL)
    {
        return false;
    }

    result = _SurfaceProjection(
        dstSurface,
        srcSurface,
        scaleWidth,
        scaleHeight,
        degree,
        FOV,
        pivotX,
        GFX_ROP3_SRCCOPY,
        alphaBlend);

    return result;
}

bool
gfxSurfaceTransformBlt (
    GFXSurface          * dest,
    int32_t             dx,
    int32_t             dy,
    GFXSurface          * src,
    int32_t             sx,
    int32_t             sy,
    int32_t             sw,
    int32_t             sh,
    int32_t             x0,
    int32_t             y0,
    int32_t             x1,
    int32_t             y1,
    int32_t             x2,
    int32_t             y2,
    int32_t             x3,
    int32_t             y3,
    bool                bInverse,
    GFXPageFlowType     type,
    GFXTransformType    transformType)
{
    bool result = false;

    if (dest == NULL || src == NULL)
    {
        return false;
    }

    result = _SurfaceTransformBlt(
        dest,
        dx,
        dy,
        src,
        sx,
        sy,
        sw,
        sh,
        x0, y0,
        x1, y1,
        x2, y2,
        x3, y3,
        bInverse,
        type,
        transformType);

    return result;
}

bool
gfxSurfaceDrawLine(
    GFXSurface  * dstSurface,
    int32_t     fromX,
    int32_t     fromY,
    int32_t     toX,
    int32_t     toY,
    GFXColor    * lineColor,
    int32_t     lineWidth)
{
    bool            result      = false;
    GFXAlphaBlend   alphaBlend  = {0};

    GFX_FUNC_ENTRY;
    if (dstSurface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    if (lineColor->a == 0xFF)
    {
        alphaBlend.enableConstantAlpha  = false;
        alphaBlend.constantAlphaValue   = 0;  // No constant alpha, 0 for don't care
    }
    else
    {
        alphaBlend.enableConstantAlpha  = true;
        alphaBlend.constantAlphaValue   = lineColor->a;
    }

    if ((fromX == toX) || (fromY == toY))
    {
        GFXSurfaceDst   surface  = {0};
        GFXColor        color    = {0};

        color.a                 = lineColor->a;
        color.r                 = lineColor->r;
        color.g                 = lineColor->g;
        color.b                 = lineColor->b;

        surface.dstSurface   = dstSurface;
        surface.dstX         = fromX;
        surface.dstY         = fromY;
        surface.dstWidth     = toX - fromX;
        surface.dstHeight    = toY - fromY;

        if (fromX == toX)
        {
            surface.dstWidth = lineWidth;
        }
        if (fromY == toY)
        {
            surface.dstHeight = lineWidth;
        }

        result = _SurfaceFillColor(&surface,
            color,
            &alphaBlend);
    }
    else
    {
        result = _SurfaceDrawLine(dstSurface,
            fromX,
            fromY,
            toX,
            toY,
            lineColor,
            lineWidth,
            &alphaBlend);
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceDrawCurve (
    GFXSurface      * surface,
    GFXCoordinates  * point1,
    GFXCoordinates  * point2,
    GFXCoordinates  * point3,
    GFXCoordinates  * point4,
    GFXColor        * lineColor,
    int32_t         lineWidth)
{
    bool    result = false;
    float   x, y = 0.0f;
    float   t = 0.0f;

    GFX_FUNC_ENTRY;
    if (surface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    for (t = 0.0f; t <= 1.0f; t += 0.01f)
    {
        x = (-t * t * t + 3 * t * t - 3 * t + 1) * point1->x
            + (3 * t * t * t - 6 * t * t + 3 * t) * point2->x
            + (-3 * t * t * t + 3 * t * t) * point3->x
            + (t * t * t) * point4->x;

        y = (-t * t * t + 3 * t * t - 3 * t + 1) * point1->y
            + (3 * t * t * t - 6 * t * t + 3 * t) * point2->y
            + (-3 * t * t * t + 3 * t * t) * point3->y
            + (t * t * t) * point4->y;

        result = _SurfaceDrawPixelAA(surface, (int)round(x), (int)round(y), lineColor, lineWidth);
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

void
gfxSurfaceSetColor (
    GFXSurface  * surface,
    GFXColor    color)
{
    _setFGColor(surface, color);
}

void
gfxSurfaceSetQuality (
    GFXSurface      * surface,
    GFXQualityMode  quality)
{
    if (surface != NULL)
    {
        surface->quality = quality;
    }
}

void
gfxSurfaceSetClip (
    GFXSurface  * surface,
    int         x0,
    int         y0,
    int         x1,
    int         y1)
{
    if (surface != NULL)
    {
        surface->clipSet.left   = x0;
        surface->clipSet.top    = y0;
        surface->clipSet.right  = x1;
        surface->clipSet.bottom = y1;
        surface->clipEnable     = true;
    }
}

void
gfxSurfaceUnSetClip (
    GFXSurface * surface)
{
    if (surface != NULL)
    {
        surface->clipEnable = false;
    }
}

GFXMaskSurface *
gfxCreateMaskSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXMaskFormat   format)
{
    GFXMaskSurface * surface = NULL;

    GFX_FUNC_ENTRY;
    gfxLock();

    surface = _gfxCreateMaskSurface(width, height, pitch, format);
    if (surface == NULL)
    {
        GFX_ERROR_MSG("Create surface fail.\n");
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return surface;
}

GFXMaskSurface *
gfxCreateMaskSurfaceByBuffer (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXMaskFormat   format,
    uint8_t         * buffer,
    uint32_t        bufferLength)
{
    GFXMaskSurface * surface = NULL;

    GFX_FUNC_ENTRY;
    gfxLock();

    do
    {
        uint8_t * mappedSysRam = NULL;
        surface = _gfxCreateMaskSurface(width, height, pitch, format);
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Create surface fail.\n");
            break;
        }

        // coverity[NULL_FIELD: FALSE]
        mappedSysRam = (uint8_t *)ithMapVram((uint32_t)surface->imageData, bufferLength, ITH_VRAM_WRITE);
        (void)memcpy(mappedSysRam, buffer, surface->imageDataLength);
        ithFlushDCacheRange((void *)mappedSysRam, bufferLength);
        ithFlushMemBuffer();
        ithUnmapVram((void *)mappedSysRam, bufferLength);
    } while (false);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return surface;
}

void
gfxDestroyMaskSurface (
    GFXMaskSurface * surface)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    if (surface != NULL)
    {
        (void)gfxwaitEngineIdle();
        if ((surface->imageDataOwner == true)
            && (surface->imageData != 0))
        {
            gfxVmemFree(surface->imageData);
            surface->imageData = NULL;
        }
        free(surface);
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;
}

void
gfxSetMaskSurface (
    GFXSurface      * surface,
    GFXMaskSurface  * maskSurface,
    bool            enable)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    if (surface != NULL)
    {
        surface->maskEnable     = enable;
        surface->maskSurface    = maskSurface;
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;
}

bool
gfxSurfaceReflected (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy,
    int32_t     reflectedWidth,
    int32_t     reflectedHeight)
{
    bool result;

    GFX_FUNC_ENTRY;

    if ((dest == NULL) || (src == NULL))
    {
        return false;
    }

    gfxLock();

    result = _SurfaceReflected(
        dest,
        dx,
        dy,
        src,
        sx,
        sy,
        reflectedWidth,
        reflectedHeight);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceMirror (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy)
{
    bool result;

    GFX_FUNC_ENTRY;

    if ((dest == NULL) || (src == NULL))
    {
        return false;
    }

    gfxLock();

    result = _SurfaceMirror(
        dest,
        dx,
        dy,
        src,
        sx,
        sy);

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceTransformGetVersion (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    gfxLock();

    result = _SurfaceTransformGetVersion(dstSurface, srcSurface);

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

#ifdef CFG_M2D_HUD_ENABLE

bool
gfxSurfaceTransformWithMatrix (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    gfxLock();

    result = _SurfaceTransformWithMatrix(dstSurface, srcSurface);

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

// Pre-processing
bool
gfxSurfaceTransformWithMatrixPreProcessing (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool result = false;

    GFX_FUNC_ENTRY;
    gfxLock();

    result = _SurfaceTransformWithMatrixPreProcessing(dstSurface, srcSurface);

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

#if 0
// bool
// gfxSurfaceTransformWithMatrixPreProcessingFish(
//    GFXSurfaceDst *dstSurface,
//    GFXSurfaceSrc *srcSurface)
// {
//    bool result = false;
//
//    GFX_FUNC_ENTRY;
//    gfxLock();
//
//    result = _SurfaceTransformWithMatrixPreProcessingFish(dstSurface, srcSurface);
//
//
//    gfxUnlock();
//
//    GFX_FUNC_LEAVE;
//
//    return result;
// }
#endif

static bool
_SurfaceTransformWithMatrix (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;
    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;

    int32_t     new_dstX;
    int32_t     new_dstY;
    int32_t     new_dstWidth;
    int32_t     new_dstHeight;

    gfxHwReset(driver->hwDev);
    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));

    inverseMtx.m[0][0]  = transform_m[num].m[0][0];
    inverseMtx.m[0][1]  = transform_m[num].m[0][1], inverseMtx.m[0][2] = transform_m[num].m[0][2];
    inverseMtx.m[1][0]  = transform_m[num].m[1][0], inverseMtx.m[1][1] = transform_m[num].m[1][1], inverseMtx.m[1][2] = transform_m[num].m[1][2];
    inverseMtx.m[2][0]  = transform_m[num].m[2][0], inverseMtx.m[2][1] = transform_m[num].m[2][1], inverseMtx.m[2][2] = transform_m[num].m[2][2];

    new_dstX            = dstInfo[num].new_dstX;
    new_dstY            = dstInfo[num].new_dstY;
    new_dstWidth        = dstInfo[num].new_dstWidth;
    new_dstHeight       = dstInfo[num].new_dstHeight;

    // (void)printf("(%d,%d) (%d,%d)\n", new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    {
        REG_FILL_DST_CLIP(regs,
            0,
            0,
            ithLcdGetWidth() - 1,
            ithLcdGetHeight() - 1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    if (dstSurface->dstSurface != NULL)
    {
        regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;
        // regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
    }

    // Fire
    regs->regCMD.data = GFX_REG_CMD_ROP3;

    gfxHwEngineFireHUD(driver->hwDev);

    #ifdef HUD_INVERSE
    if (num == 0)
    {
        num = gBlock_num - 1;
    }
    else
    {
        num--;
    }

    #else
    num++;

    if (num == gBlock_num)
    {
        num = 0;
    }
    #endif
    return result;
}

static bool
_SurfaceTransformWithMatrixPreProcessing (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;
    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;

    int32_t     new_dstX;
    int32_t     new_dstY;
    int32_t     new_dstWidth;
    int32_t     new_dstHeight;

    gfxHwReset(driver->hwDev);
    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));

    inverseMtx.m[0][0]  = transform_m[num].m[0][0];
    inverseMtx.m[0][1]  = transform_m[num].m[0][1], inverseMtx.m[0][2] = transform_m[num].m[0][2];
    inverseMtx.m[1][0]  = transform_m[num].m[1][0], inverseMtx.m[1][1] = transform_m[num].m[1][1], inverseMtx.m[1][2] = transform_m[num].m[1][2];
    inverseMtx.m[2][0]  = transform_m[num].m[2][0], inverseMtx.m[2][1] = transform_m[num].m[2][1], inverseMtx.m[2][2] = transform_m[num].m[2][2];

    new_dstX        = dstInfo[num].new_dstX;
    new_dstY        = dstInfo[num].new_dstY;
    new_dstWidth    = dstInfo[num].new_dstWidth;
    new_dstHeight   = dstInfo[num].new_dstHeight;

    // (void)printf("(%d,%d) (%d,%d)\n", new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    {
        REG_FILL_DST_CLIP(regs,
            0,
            0,
            ithLcdGetWidth() - 1,
            ithLcdGetHeight() - 1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    if (dstSurface->dstSurface != NULL)
    {
        regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;
        // regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
    }

    // Fire
    if (new_dstWidth >= 3 && new_dstHeight >= 3)
    {
        regs->regCMD.data = GFX_REG_CMD_ROP3;
    }

    // dump registers
    if (1)
    {
        // Disable read reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_RDREORDER_EN;
        // Disable write reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_WRREORDER_EN;

        // Enable boundary interpolation
        regs->regSAFE.data  |= GFX_REG_SAFE_BOUNDARY_INTERPOLATION;

        // Enable extend by zero for color depth extend
        regs->regCR1.data   |= 0x00004000;

        GFX_HW_DEVICE   * dev       = driver->hwDev;
        GFX_HW_REGS     * regs      = &dev->regs;
        GFX_HW_REG      * currReg   = &regs->regSRC;
        GFX_HW_REG      * endReg    = &regs->regCMD;

    #ifndef CFG_HUD_AUTO_GENERATE
        char    buf[64]             = {0};
        int     size                = 0;

        if (fd == 0)
        {
            fd = fopen("A:/HUDAddrData[].txt", "w");
            if (fd == 0)
            {
                (void)printf("HUDAddrData[].txt file is not exist\n");
                return result;
            }
        }
    #endif

        while (currReg <= endReg)
        {
    #ifdef HUD_INVERSE
            if (num == gBlock_num - 1)
    #else
            if (num == 0)
    #endif
            {
    #ifndef CFG_HUD_AUTO_GENERATE

                if (currReg->addr == GFX_REG_SPR_ADDR /*|| currReg->addr == 0xb070002c*/)
                {
                    currReg->data = ((gBlock_W * gW_num) + gTempHudIndex) * 2; // source pitch
                }
                size = sprintf(buf, "0x%08x, 0x%08x,\n", currReg->addr, currReg->data);
                fwrite(buf, 1, size, fd);
    #else

                if (currReg->addr == GFX_REG_SPR_ADDR /*|| currReg->addr == 0xb070002c*/)
                {
                    currReg->data = ((gBlock_W * gW_num) + gTempHudIndex) * 2; // source pitch
                }
                // if (currReg->addr == 0xb070008C /*|| currReg->addr == 0xb070002c*/)
                //    currReg->data = 0x0; //first block not fire to avoid flicker

                // (void)printf("0x%08x, 0x%08x,\n", currReg->addr, currReg->data);

                gpHudTableData[0][addrDataNum]      = currReg->addr;
                gpHudTableData[0][addrDataNum + 1]  = currReg->data;
                gpHudTableData[1][addrDataNum]      = currReg->addr;
                gpHudTableData[1][addrDataNum + 1]  = currReg->data;
                gpHudTableData[2][addrDataNum]      = currReg->addr;
                gpHudTableData[2][addrDataNum + 1]  = currReg->data;
                addrDataNum                         += 2;
    #endif
            }
            else
            {
                if (currReg->addr == GFX_REG_SRC_ADDR || currReg->addr == GFX_REG_DST_ADDR || currReg->addr == GFX_REG_DSTXY_ADDR || currReg->addr == GFX_REG_DHWR_ADDR || currReg->addr == GFX_REG_ITMR00_ADDR || currReg->addr == GFX_REG_ITMR01_ADDR
                    || currReg->addr == GFX_REG_ITMR02_ADDR || currReg->addr == GFX_REG_ITMR10_ADDR || currReg->addr == GFX_REG_ITMR11_ADDR || currReg->addr == GFX_REG_ITMR12_ADDR || currReg->addr == GFX_REG_ITMR20_ADDR || currReg->addr == GFX_REG_ITMR21_ADDR
                    || currReg->addr == GFX_REG_ITMR22_ADDR || currReg->addr == GFX_REG_CMD_ADDR)

                {
    #ifndef CFG_HUD_AUTO_GENERATE
                    size                                = sprintf(buf, "0x%08x, 0x%08x,\n", currReg->addr, currReg->data);
                    fwrite(buf, 1, size, fd);
    #else
                    gpHudTableData[0][addrDataNum]      = currReg->addr;
                    gpHudTableData[0][addrDataNum + 1]  = currReg->data;
                    gpHudTableData[1][addrDataNum]      = currReg->addr;
                    gpHudTableData[1][addrDataNum + 1]  = currReg->data;
                    gpHudTableData[2][addrDataNum]      = currReg->addr;
                    gpHudTableData[2][addrDataNum + 1]  = currReg->data;
                    addrDataNum                         += 2;
    #endif
                }
            }

            currReg++;
        }
    }

    #if defined(CMDQ_IRQ_ENABLE)
    // Enable CMDQ_IRQ_ENABLE need to add a useless 2D command
    gpHudTableData[0][addrDataNum] = 0xB07000A8;
    gpHudTableData[0][addrDataNum + 1] = 0x00000001;
    gpHudTableData[1][addrDataNum] = 0xB07000A8;
    gpHudTableData[1][addrDataNum + 1] = 0x00000001;
    gpHudTableData[2][addrDataNum] = 0xB07000A8;
    gpHudTableData[2][addrDataNum + 1] = 0x00000001;
    #endif

    #ifdef HUD_INVERSE
    if (num == 0)
    {
        num         = gBlock_num - 1;
        addrDataNum = 0;
        #ifndef CFG_HUD_AUTO_GENERATE
        fclose(fd);
        #endif
    }
    else
    {
        num--;
    }
    #else
    if (num < gBlock_num - 1)
    {
        num++;
    }
    else
    {
        num         = 0;
        addrDataNum = 0;
        #ifndef CFG_HUD_AUTO_GENERATE
        fclose(fd);
        #endif
    }
    #endif
    return result;
}

#if 0
static bool
_SurfaceTransformWithMatrixPreProcessingFish (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;
    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;

    int32_t     new_dstX;
    int32_t     new_dstY;
    int32_t     new_dstWidth;
    int32_t     new_dstHeight;
    int32_t     new_srcX;
    int32_t     new_srcY;
    int32_t     new_srcWidth;
    int32_t     new_srcHeight;

    gfxHwReset(driver->hwDev);
    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));

    mtx.m[0][0]     = transform_m[num].m[0][0];
    mtx.m[0][1]     = transform_m[num].m[0][1], mtx.m[0][2] = transform_m[num].m[0][2];
    mtx.m[1][0]     = transform_m[num].m[1][0], mtx.m[1][1] = transform_m[num].m[1][1], mtx.m[1][2] = transform_m[num].m[1][2];
    mtx.m[2][0]     = transform_m[num].m[2][0], mtx.m[2][1] = transform_m[num].m[2][1], mtx.m[2][2] = transform_m[num].m[2][2];

    new_dstX        = dstInfo[num].new_dstX;
    new_dstY        = dstInfo[num].new_dstY;
    new_dstWidth    = dstInfo[num].new_dstWidth;
    new_dstHeight   = dstInfo[num].new_dstHeight;

    new_srcX        = srcInfo[num].new_dstX;
    new_srcY        = srcInfo[num].new_dstY;
    new_srcWidth    = srcInfo[num].new_dstWidth;
    new_srcHeight   = srcInfo[num].new_dstHeight;

    GFXSurface  * surf      = 0;
    uint8_t     bpp         = 0;
    uint8_t     * lcdAddr   = 0;

    if (srcSurface->srcSurface != NULL)
    {
        int size = srcSurface->srcSurface->pitch * new_srcHeight;

        if (srcSurface->srcSurface->format == GFX_COLOR_ARGB8888)
        {
            bpp = 4;
        }
        else
        {
            bpp = 2;
        }

        lcdAddr                 = srcSurface->srcSurface->imageData + (new_srcY * 1280 * 2) + (new_srcX * 2);
        surf                    = gfxCreateSurfaceByPointer(new_srcWidth, new_srcHeight, srcSurface->srcSurface->pitch, srcSurface->srcSurface->format, (uint8_t *)lcdAddr, size);
        srcSurface->srcSurface  = surf;
    }

    // (void)printf("dst(%d,%d) (%d,%d)\n", new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }
    // (void)printf("src(%d,%d) (%d,%d)\n", new_srcX, new_srcY, new_srcWidth, new_srcHeight);
    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    {
        REG_FILL_DST_CLIP(regs,
            0,
            0,
            ithLcdGetWidth() - 1,
            ithLcdGetHeight() - 1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // About blending
    // REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    //// Foreground Color
    // REG_FILL_FGCOLOR(regs, dstSurface->dstSurface->fgColor);

    // Mask
    // REG_FILL_MASK_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    if (dstSurface->dstSurface != NULL)
    {
        regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;
        // regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
    }

    // Fire
    regs->regCMD.data = GFX_REG_CMD_ROP3;

    // dump registers
    if (1)
    {
        // Disable read reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_RDREORDER_EN;
        // Disable write reorder
        regs->regSAFE.data  |= GFX_REG_SAFE_WRREORDER_EN;

        // Enable boundary interpolation
        regs->regSAFE.data  |= GFX_REG_SAFE_BOUNDARY_INTERPOLATION;

        // Enable extend by zero for color depth extend
        regs->regCR1.data   |= 0x00004000;

        GFX_HW_DEVICE   * dev       = driver->hwDev;
        GFX_HW_REGS     * regs      = &dev->regs;
        GFX_HW_REG      * currReg   = &regs->regSRC;
        GFX_HW_REG      * endReg    = &regs->regCMD;

        while (currReg <= endReg)
        {
    #ifdef HUD_INVERSE
            if (num == gBlock_num - 1)
    #else
            if (num == 0)
    #endif
            {
                if (currReg->addr == 0xb070000C /*|| currReg->addr == 0xb070002c*/)
                {
                    currReg->data = 1280 * 2;// gBlock_W * gW_num * 2; //source pitch
                }
                // (void)printf("0x%08x, 0x%08x,\n", currReg->addr, currReg->data);
                gpHudTableData[0][addrDataNum]      = currReg->addr;
                gpHudTableData[0][addrDataNum + 1]  = currReg->data;
                gpHudTableData[1][addrDataNum]      = currReg->addr;
                gpHudTableData[1][addrDataNum + 1]  = currReg->data;
                gpHudTableData[2][addrDataNum]      = currReg->addr;
                gpHudTableData[2][addrDataNum + 1]  = currReg->data;
                addrDataNum                         += 2;
            }
            else
            {
                if ((currReg->addr == 0xb0700000) || (currReg->addr == 0xb0700020) || (currReg->addr == 0xb0700024) || (currReg->addr == 0xb0700028) || (currReg->addr == 0xb0700040) || (currReg->addr == 0xb0700044)
                    || (currReg->addr == 0xb0700048) || (currReg->addr == 0xb070004c) || (currReg->addr == 0xb0700050) || (currReg->addr == 0xb0700054) || (currReg->addr == 0xb0700058) || (currReg->addr == 0xb070005c)
                    || (currReg->addr == 0xb0700060) || (currReg->addr == 0xb070008c))
                {
                    gpHudTableData[0][addrDataNum]      = currReg->addr;
                    gpHudTableData[0][addrDataNum + 1]  = currReg->data;
                    gpHudTableData[1][addrDataNum]      = currReg->addr;
                    gpHudTableData[1][addrDataNum + 1]  = currReg->data;
                    gpHudTableData[2][addrDataNum]      = currReg->addr;
                    gpHudTableData[2][addrDataNum + 1]  = currReg->data;
                    addrDataNum                         += 2;
                    // (void)printf("0x%08x, 0x%08x,\n", currReg->addr, currReg->data);
                }
            }
            currReg++;
        }
    }
    #ifdef HUD_INVERSE
    if (num == 0)
    {
        num         = gBlock_num - 1;
        addrDataNum = 0;
    }
    else
    {
        num--;
    }
    #else
    if (num < gBlock_num - 1)
    {
        num++;
    }
    else
    {
        num         = 0;
        addrDataNum = 0;
    }
    #endif

    if (surf != NULL)
    {
        gfxDestroySurface(surf);
    }

    return result;
}
#endif
#endif

bool
gfxSurfaceShearing (
    GFXSurfaceDst   * dstSurface,
    GFX_RECTANGLE   coordinate,
    GFXSurfaceSrc *srcSurface,
    GFXColor      color)
{
    bool result = false;
    GFXAlphaBlend alphaBlend = { 0 };

    GFX_FUNC_ENTRY;
    gfxLock();

    _setFGColor(dstSurface->dstSurface, color);

    alphaBlend.enableAlpha = true; // Enable Alpha Blending

    if (color.a == 0xFF)
    {
        alphaBlend.enableConstantAlpha = false;
        alphaBlend.constantAlphaValue = 0;    // No constant alpha, 0 for don't care
    }
    else if (color.a == 0x0)
    {
        if (color.r == 0x0 && color.g == 0x0 && color.b == 0x0)
            alphaBlend.enableAlpha = false;
        alphaBlend.enableConstantAlpha = false;
        alphaBlend.constantAlphaValue = 0;
    }
    else
    {
        alphaBlend.enableConstantAlpha = true;
        alphaBlend.constantAlphaValue = color.a;
    }

    result = _SurfaceShearing(dstSurface, coordinate, srcSurface, &alphaBlend);

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

bool
gfxSurfaceLightCarpet (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    float           recgtangle,
    float           radius)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    bool            result      = false;
    GFXSurfaceDst   surface     = {0};
    GFXSurfaceSrc   imgsurface  = {0};
    GFX_RECTANGLE   position;
    GFXAlphaBlend   alphaBlend  = {0};

    int     imgH;
    int     block_w, block_h, block_total;
    float   R1, R2;
    int     j;
    float   i;
    float   degree_start, degree_end, degree_block;
    double  rad;

    imgH    = srcSurface->srcHeight;
    block_w = srcSurface->srcWidth;
    block_h = 2;

    if (recgtangle == 0.0f)
    {
        R1 = R2 = degree_start = degree_end = 0.0f;
    }
    else
    {
        if (recgtangle > 0.0f)
        {
            R1              = radius + (srcSurface->srcWidth / 2);
            R2              = radius - (srcSurface->srcWidth / 2);
            degree_start    = 180.0f + recgtangle;
            degree_end      = 240.0f + recgtangle;
        }
        else
        {
            R1              = radius + (srcSurface->srcWidth / 2);
            R2              = radius - (srcSurface->srcWidth / 2);
            degree_start    = 360.0f + recgtangle;
            degree_end      = 300.0f + recgtangle;
        }
    }
    block_total     = imgH / block_h;
    degree_block    = (degree_end - degree_start) / block_total;

    // (void)printf("R1:%f R2:%f %f\n", R1, R2, radius);
    // (void)printf("%f %d\n", degree_block, block_total);

    for (i = degree_start, j = 0; i < degree_end && j < block_total; i += degree_block, j++)
    {
        rad                 = (i * GFX_PI / 180.0f);

        surface.dstSurface  = dstSurface->dstSurface;
        surface.dstX        = dstSurface->dstX;

        {
            surface.dstY = dstSurface->dstY - ((j + 1) * block_h);
        }

        surface.dstWidth        = dstSurface->dstWidth;
        surface.dstHeight       = dstSurface->dstHeight;

        imgsurface.srcSurface   = srcSurface->srcSurface;
        imgsurface.srcX         = 0;
        imgsurface.srcY         = imgH - ((j + 1) * block_h);
        imgsurface.srcWidth     = block_w;
        imgsurface.srcHeight    = block_h;

        if (recgtangle == 0.0f)
        {
            position.x0 = 0.0f;
            position.y0 = 0.0f;
            position.x1 = 0.0f;
            position.y1 = (float)block_h;
            position.x2 = (float)block_w;
            position.y2 = (float)block_h;
            position.x3 = (float)block_w;
            position.y3 = 0.0f;
        }
        else if (recgtangle > 0.0f)                                                             // turn right
        {
            float shift = 0.0f;

            if (j == 0)
            {
                position.x0 = R1 * cos(rad + (degree_block * GFX_PI / 180.0f)) + R1;
                position.y0 = R1 * sin(rad + (degree_block * GFX_PI / 180.0f)) - shift;
                position.x1 = R1 * cos(rad) + R1;
                position.y1 = R1 * sin(rad) + block_h;
            }
            else
            {
                position.x1 = position.x0;
                position.y1 = position.y0 + block_h + shift,
                position.x0 = R1 * cos(rad + (degree_block * GFX_PI / 180.0f)) + R1;
                position.y0 = R1 * sin(rad + (degree_block * GFX_PI / 180.0f));
            }

            if (j == 0)
            {
                position.x2 = R2 * cos(rad) + R2 + block_w;
                position.y2 = R2 * sin(rad) + block_h;
                position.x3 = R2 * cos(rad + (degree_block * GFX_PI / 180.0f)) + R2 + block_w;
                position.y3 = R2 * sin(rad + (degree_block * GFX_PI / 180.0f)) - shift;
            }
            else
            {
                position.x2 = position.x3;
                position.y2 = position.y3 + block_h + shift;
                position.x3 = R2 * cos(rad + (degree_block * GFX_PI / 180.0f)) + R2 + block_w;
                position.y3 = R2 * sin(rad + (degree_block * GFX_PI / 180.0f));
            }

            // (void)printf("(%f %f)(%f %f)(%f %f)(%f %f)\n", position.x0, position.y0, position.x1, position.y1, position.x2, position.y2, position.x3, position.y3);
        }
        else // turn left
        {
            float shift = 0.0f;

            if (j == 0)
            {
                position.x0 = R1 * cos(rad + (degree_block * GFX_PI / 180.0f));
                position.y0 = R1 * sin(rad + (degree_block * GFX_PI / 180.0f)) - shift;
                position.x1 = R1 * cos(rad);
                position.y1 = R1 * sin(rad) + block_h;
            }
            else
            {
                position.x1 = position.x0;
                position.y1 = position.y0 + block_h + shift;
                position.x0 = R1 * cos(rad + (degree_block * GFX_PI / 180.0f));
                position.y0 = R1 * sin(rad + (degree_block * GFX_PI / 180.0f));
            }

            if (j == 0)
            {
                position.x2 = R2 * cos(rad);
                position.y2 = R2 * sin(rad) + block_h;
                position.x3 = R2 * cos(rad + (degree_block * GFX_PI / 180.0f));
                position.y3 = R2 * sin(rad + (degree_block * GFX_PI / 180.0f)) - shift;
            }
            else
            {
                position.x2 = position.x3;
                position.y2 = position.y3 + block_h + shift;
                position.x3 = R2 * cos(rad + (degree_block * GFX_PI / 180.0f));
                position.y3 = R2 * sin(rad + (degree_block * GFX_PI / 180.0f));
            }

            // (void)printf("%d (%f %f)(%f %f)(%f %f)(%f %f)\n", j, position.x0, position.y0, position.x1, position.y1, position.x2, position.y2, position.x3, position.y3);
        }

        alphaBlend.enableAlpha = false; // disable Alpha Blending
        alphaBlend.enableConstantAlpha = false;
        alphaBlend.constantAlphaValue = 0;    // No constant alpha, 0 for don't care

        gfxSurfaceSetClip(dstSurface->dstSurface, 0, 0, dstSurface->dstWidth - 1, dstSurface->dstHeight - 1);
        result = _SurfaceShearing(&surface, position, &imgsurface, &alphaBlend);
        gfxSurfaceUnSetClip(dstSurface->dstSurface);
    }

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

// Shearing Graphic Display Controller
bool
gfxSurfaceShearingGDC (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    GFX_COODS       srcCoods,
    GFX_COODS       destCoods)
{
    GFX_FUNC_ENTRY;
    gfxLock();

    bool            result = false;
    GFXSurfaceDst   surface = {0};
    GFXSurfaceSrc   imgsurface = {0};
    GFX_RECTANGLE   position;
    int             block_w, block_h, block_num;
    int             i, j;
    GFXAlphaBlend   alphaBlend = { 0 };

    if ((srcCoods.cloums != destCoods.cloums) || (srcCoods.row != destCoods.row))
    {
        return result;
    }

    // (void)printf("coods num %d %d %d %d\n", sizeof(*srcCoods.coods), sizeof(GFX_RECTANGLE), sizeof(float), srcCoods.cloums*srcCoods.row);

    block_w     = srcCoods.coods[0].x3 - srcCoods.coods[0].x0;
    block_h     = srcCoods.coods[0].y1 - srcCoods.coods[0].y0;
    block_num   = (srcCoods.cloums / 2) * (srcCoods.row / 2);

    // (void)printf("block (%d %d) block_num:%d\n", block_w, block_h, block_num);
    // (void)printf("src (%f %f) (%f %f) (%f %f) (%f %f)\n", srcCoods.coods[0].x0, srcCoods.coods[0].y0, srcCoods.coods[0].x1, srcCoods.coods[0].y1, srcCoods.coods[0].x2, srcCoods.coods[0].y2, srcCoods.coods[0].x3, srcCoods.coods[0].y3);
    // (void)printf("dest(%f %f) (%f %f) (%f %f) (%f %f)\n", destCoods.coods[0].x0, destCoods.coods[0].y0, destCoods.coods[0].x1, destCoods.coods[0].y1, destCoods.coods[0].x2, destCoods.coods[0].y2, destCoods.coods[0].x3, destCoods.coods[0].y3);

    for (i = (srcCoods.row / 2) - 1; i >= 0; i--)
    {
        for (j = (srcCoods.cloums / 2) - 1; j >= 0 / 2; j--)
        {
            surface.dstSurface      = dstSurface->dstSurface;
            surface.dstX            = dstSurface->dstX + (i * block_w);
            surface.dstY            = dstSurface->dstY + (j * block_h);
            surface.dstWidth        = dstSurface->dstWidth;
            surface.dstHeight       = dstSurface->dstHeight;

            imgsurface.srcSurface   = srcSurface->srcSurface;
            imgsurface.srcX         = i * block_w;
            imgsurface.srcY         = j * block_h;
            imgsurface.srcWidth     = block_w;
            imgsurface.srcHeight    = block_h;

            block_num--;

            {
                position.x0 = destCoods.coods[block_num].x0;
                position.y0 = destCoods.coods[block_num].y0;
                position.x1 = destCoods.coods[block_num].x1;
                position.y1 = destCoods.coods[block_num].y1 + block_h;
                position.x2 = destCoods.coods[block_num].x2 + block_w;
                position.y2 = destCoods.coods[block_num].y2 + block_h;
                position.x3 = destCoods.coods[block_num].x3 + block_w;
                position.y3 = destCoods.coods[block_num].y3;

                // (void)printf("%d(%f %f)(%f %f)(%f %f)(%f %f)\n", block_num, position.x0, position.y0, position.x1, position.y1, position.x2, position.y2, position.x3, position.y3);
            }

            alphaBlend.enableAlpha = false; // disable Alpha Blending
            alphaBlend.enableConstantAlpha = false;
            alphaBlend.constantAlphaValue = 0;    // No constant alpha, 0 for don't care

            gfxSurfaceSetClip(dstSurface->dstSurface, 0, 0, dstSurface->dstWidth - 1, dstSurface->dstHeight - 1);
            result = _SurfaceShearing(&surface, position, &imgsurface, &alphaBlend);
            gfxSurfaceUnSetClip(dstSurface->dstSurface);
        }
    }

    gfxUnlock();

    GFX_FUNC_LEAVE;

    return result;
}

// =============================================================================
//                              Private Function Definition
// =============================================================================
static bool
_SurfaceBoundaryCheck (
    GFXSurface  * surface,
    int         clipX,
    int         clipY,
    int         clipWidth,
    int         clipHeight)
{
    bool result = false;

#if 0
    do
    {
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Invalud destination surface.\n");
            break;
        }
        if (clipX >= surface->width)
        {
            GFX_ERROR_MSG("Invalud start X(%d). Over surface's width(%d).\n", clipX, surface->width);
            break;
        }
        if (clipY >= surface->height)
        {
            GFX_ERROR_MSG("Invalud start Y(%d). Over surface's height(%d).\n", clipY, surface->height);
            break;
        }
        if ((clipX + clipWidth) <= 0)
        {
            GFX_ERROR_MSG("Selected X(%d) and width(%d) out of boundary.\n", clipX, clipWidth);
            break;
        }
        if ((clipY + clipHeight) <= 0)
        {
            GFX_ERROR_MSG("Selected Y(%d) and height(%d) out of boundary.\n", clipY, clipHeight);
            break;
        }

        result = true;
    } while (false);
#else
    result = true;
#endif
    return result;
}

static GFXSurface *
_gfxCreateSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXColorFormat  format)
{
    GFXSurface  * surface   = NULL;
    bool        result      = false;

    do
    {
#ifdef WIN32
        surface = (GFXSurface *)malloc(sizeof(GFXSurface));
#else
        surface = (GFXSurface *)memalign(sizeof(int), sizeof(GFXSurface));
#endif
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Out of memory.\n");
            break;
        }

        surface->width              = (int32_t)width;
        surface->height             = (int32_t)height;
        surface->pitch              = pitch;
        surface->format             = format;
        surface->imageDataLength    = pitch * height;
        surface->imageData          = gfxVmemAlloc(surface->imageDataLength);
        surface->imageDataOwner     = true;
        surface->maskEnable         = false;
        surface->maskSurface        = NULL;
        surface->clipEnable         = false;
        surface->quality            = GFX_QUALITY_BETTER;
        if (surface->imageData == NULL)
        {
            GFX_ERROR_MSG("Out of memory.\n");
            break;
        }

#ifdef CFG_CPU_WB
        // The mapped memory area may be used by CPU and related cache may be
        // dirty before. Therefore, we need to invalidate the cache range to
        // ensure no related cache flush while engine writing this area
        // simultaneously.
        ithInvalidateDCacheRange((void *)surface->imageData,
                                 surface->imageDataLength);
#endif

        result = true;
    } while (false);

    if (result == false)
    {
        if (surface != NULL)
        {
            free(surface);
            surface = NULL;
        }
    }

    return surface;
}

static GFXMaskSurface *
_gfxCreateMaskSurface (
    uint32_t        width,
    uint32_t        height,
    uint32_t        pitch,
    GFXMaskFormat   format)
{
    GFXMaskSurface * surface = NULL;
    bool             result  = false;

    do
    {
#ifdef WIN32
        surface = (GFXMaskSurface *)malloc(sizeof(GFXMaskSurface));
#else
        surface =
            (GFXMaskSurface *)memalign(sizeof(int), sizeof(GFXMaskSurface));
#endif
        if (surface == NULL)
        {
            GFX_ERROR_MSG("Out of memory.\n");
            break;
        }

        surface->width           = (int32_t)width;
        surface->height          = (int32_t)height;
        surface->pitch           = pitch;
        surface->format          = format;
        surface->imageDataLength = pitch * height;
        surface->imageData       = gfxVmemAlloc(surface->imageDataLength);
        surface->imageDataOwner  = true;
        if (surface->imageData == NULL)
        {
            GFX_ERROR_MSG("Out of memory.\n");
            break;
        }
#ifdef CFG_CPU_WB
        // The mapped memory area may be used by CPU and related cache may be
        // dirty before. Therefore, we need to invalidate the cache range to
        // ensure no related cache flush while engine writing this area
        // simultaneously.
        ithInvalidateDCacheRange((void *)surface->imageData,
                                 surface->imageDataLength);
#endif

        result = true;
    } while (false);

    if (result == false)
    {
        if (surface != NULL)
        {
            free(surface);
            surface = NULL;
        }
    }

    return surface;
}

// TODO
#if 0

static bool
_SurfaceDrawSurface (
    GFXSurface  * dstSurface,
    int32_t     dstX,
    int32_t     dstY,
    int32_t     dstWidth,
    int32_t     dstHeight,
    GFXSurface  * srcSurface,
    int32_t     srcX,
    int32_t     srcY,
    int32_t     srcWidth,
    int32_t     srcHeight,
    GFXSurface  * maskSurface,
    bool        enableMask,
    int32_t     maskX,
    int32_t     maskY,
    int32_t     maskWidth,
    int32_t     maskHeight,
    bool        enableSrcColorKey,
    GFXColor    srcColorKey,
    bool        enableDstColorKey,
    GFXColor    dstColorKey,
    bool        enableAlphablending,
    bool        enableConstantAlpha,
    GFXColor    contantAlphaValue,
    float       rotateDegree)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    regs->regSRC.data   = (uint32_t)srcSurface->imageData;
    regs->regMASK.data  = (enableMask == true) ? (uint32_t)maskSurface->imageData : 0;
    regs->regSPR.data   = srcSurface->pitch;
    regs->regMPR.data   = (enableMask == true) ? maskSurface->pitch : 0;
    regs->regSRCXY.data = ((srcX & GFX_REG_SRCXY_SRCX_MASK) << GFX_REG_SRCXY_SRCX_OFFSET) | (srcY & GFX_REG_SRCXY_SRCY_MASK);
    regs->regSHWR.data  = ((srcWidth & GFX_REG_SHWR_SRCWIDTH_MASK) << GFX_REG_SHWR_SRCWIDTH_OFFSET) | (srcHeight & GFX_REG_SHWR_SRCHEIGHT_MASK);
    regs->regDST.data   = (uint32_t)dstSurface->imageData;
    regs->regDSTXY.data = ((dstX & GFX_REG_DSTXY_DSTX_MASK) << GFX_REG_DSTXY_DSTX_OFFSET) | (dstY & GFX_REG_DSTXY_DSTY_MASK);
    regs->regDHWR.data  = ((dstWidth & GFX_REG_DHWR_DSTWIDTH_MASK) << GFX_REG_DHWR_DSTWIDTH_OFFSET) | (dstHeight & GFX_REG_DHWR_DSTHEIGHT_MASK);
    regs->regDPR.data   = dstSurface->pitch;
    regs->regPXCR.data  = (dstWidth - 1) & GFX_REG_PXCR_CLIPXEND_MASK;
    regs->regPYCR.data  = (dstHeight - 1) & GFX_REG_PYCR_CLIPYEND_MASK;

    // Set src & dst surface format

    // Enable clipping here!

    if (enableMask == true)
    {
        // Enable mask

        // Setup mask data format
    }

    if (rotateDegree > 0)
    {
        // Setup ITMR

        // Calculate rotate boundary, reset clipping range here!

        // Enable Transform here!

        // Enable perspective here, if needed!

        // Set interpolation mode

        // Set texture tile mode
    }

    // Setup Color key
    if (enableSrcColorKey == true)
    {
        // Set transparent mode
    }
    if (enableDstColorKey == true)
    {
        // Set transparent mode
    }

    if (enableAlphablending == true)
    {
        // Enable alpha blending
    }

    if (enableConstantAlpha == true)
    {
        // Enable constant alpha
    }

    // Setup constant alpha
    if (enableConstantAlpha == true)
    {
    }

    // Enable dithering

    return result;
}
#endif

static bool
_SurfaceBitblt (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    gfxHwReset(driver->hwDev);

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    if (dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    if (dstSurface != NULL && dstSurface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
                          dstSurface->clipSet.left,
                          dstSurface->clipSet.top,
                          dstSurface->clipSet.right,
                          dstSurface->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // ROP3
    REG_FILL_ROP3(regs, rop);

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    if (dstSurface != NULL)
    {
        REG_FILL_FGCOLOR(regs, dstSurface->fgColor);
    }
    // Mask
    if (dstSurface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, dstSurface, dstX, dstY, dstSurface->width, dstSurface->height);
    }

    // FIXME
/*
    if ((srcSurface->format == GFX_COLOR_A_8) ||
        (srcSurface->format == GFX_COLOR_A_4) ||
        (srcSurface->format == GFX_COLOR_A_2) ||
        (srcSurface->format == GFX_COLOR_A_1) ||
        (srcSurface->format != dstSurface->format)) {
        regs->regSAFE.data |= GFX_REG_SAFE_PIXELIZE_EN;
    }
 */
    if (srcSurface->srcSurface != NULL)
    {
        if (srcSurface->srcSurface->flags != 0)
        {
            regs->regCR1.data &= ~(GFX_REG_CR1_MANUALDITHER_EN);    // disable manual control (auto detect dither)
        }
        else
        {
            regs->regCR1.data |= GFX_REG_CR1_MANUALDITHER_EN;       // enable manual control (disable dither)
        }
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceBitbltSelDstAlpha (
    GFXSurface      * dstSurface,
    int32_t         dstX,
    int32_t         dstY,
    GFXSurfaceSrc   * srcSurface,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    gfxHwReset(driver->hwDev);

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    if (dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface, dstX, dstY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    if (dstSurface != NULL && dstSurface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
            dstSurface->clipSet.left,
            dstSurface->clipSet.top,
            dstSurface->clipSet.right,
            dstSurface->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // ROP3
    REG_FILL_ROP3(regs, rop);

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    if (dstSurface != NULL)
    {
        REG_FILL_FGCOLOR(regs, dstSurface->fgColor);
    }

    // Mask
    if (dstSurface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, dstSurface, dstX, dstY, dstSurface->width, dstSurface->height);
    }

    if (srcSurface->srcSurface != NULL && srcSurface->srcSurface->flags)
    {
        regs->regCR1.data &= ~(GFX_REG_CR1_MANUALDITHER_EN);    // disable manual control (auto detect dither)
    }
    else
    {
        regs->regCR1.data |= GFX_REG_CR1_MANUALDITHER_EN;       // enable manual control (disable dither)
    }

    regs->regCR1.data   |= GFX_REG_CR1_ALPHA_SEL_DST;

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceTransform (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    int32_t         refX,
    int32_t         refY,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    GFXTileMode     tilemode,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result = false;
    GFX_DRIVER  * driver = gfxGetDriver();
    GFX_HW_REGS * regs = &driver->hwDev->regs;
    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;
    int32_t     degree_i = (int32_t)fabsf(degree);
    int32_t     new_dstX, new_dstY, new_dstWidth, new_dstHeight;
    int32_t     clip_x0 = 0, clip_y0 = 0, clip_x1 = 0, clip_y1 = 0;
    bool        extra_clip = false;

    // (void)printf("dest (%d, %d)(%d, %d)\n", dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight);
    // (void)printf("src (%d, %d)(%d, %d)\n", srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);

    gfxHwReset(driver->hwDev);

    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));

    gfxMatrixIdentify(&mtx);

    gfxMatrixTranslate(&mtx, dstSurface->dstX, dstSurface->dstY);

    if ((scaleWidth != 1.0) || (scaleHeight != 1.0))
    {
        gfxMatrixScale(&mtx, scaleWidth, scaleHeight);
    }

    if ((refX != 0) || (refY != 0))
    {
        gfxMatrixTranslate(&mtx, refX, refY);
    }

    if (degree != 0.0)
    {
        gfxMatrixRotate(&mtx, degree);
    }

    // (void)printf("%f %f %f\n",mtx.m[0][0],mtx.m[0][1],mtx.m[0][2]);
    // (void)printf("%f %f %f\n",mtx.m[1][0],mtx.m[1][1],mtx.m[1][2]);
    // (void)printf("%f %f %f\n\n",mtx.m[2][0],mtx.m[2][1],mtx.m[2][2]);

    if ((refX != 0) || (refY != 0))
    {
        gfxMatrixTranslate(&mtx, -refX, -refY);
    }

    if (tilemode == GFX_TILE_FILL)   // fill mode
    {
        float   topLX, topLY, topRX, topRY;
        float   botLX, botLY, botRX, botRY;
        int32_t topX, topY;
        int32_t botX, botY;

        // Find the top left and bottom right coordinate
        {
            gfxMatrixTransform(&mtx, srcSurface->srcX, srcSurface->srcY, &topLX, &topLY);
            gfxMatrixTransform(&mtx, srcSurface->srcX + srcSurface->srcWidth, srcSurface->srcY, &topRX, &topRY);
            gfxMatrixTransform(&mtx, srcSurface->srcX, srcSurface->srcY + srcSurface->srcHeight, &botLX, &botLY);
            gfxMatrixTransform(&mtx, srcSurface->srcX + srcSurface->srcWidth, srcSurface->srcY + srcSurface->srcHeight, &botRX, &botRY);

            topX    = (int32_t)floorf(MIN(MIN(topLX, topRX), MIN(botLX, botRX))) - 1;
            topY    = (int32_t)floorf(MIN(MIN(topLY, topRY), MIN(botLY, botRY))) - 1;
            botX    = (int32_t)ceilf(MAX(MAX(topLX, topRX), MAX(botLX, botRX))) + 1;
            botY    = (int32_t)ceilf(MAX(MAX(topLY, topRY), MAX(botLY, botRY))) + 1;

            // (void)printf("topL (%f, %f), topR (%f, %f)\n", topLX, topLY, topRX, topRY);
            // (void)printf("botL (%f, %f), botR (%f, %f)\n", botLX, botLY, botRX, botRY);
            // (void)printf("top (%d, %d), bot (%d, %d)\n", topX, topY, botX, botY);
#ifndef CFG_ENABLE_ROTATE
            topX = (topX < 0) ? 0 : topX;
            topY = (topY < 0) ? 0 : topY;
            topX = topX >= CFG_LCD_WIDTH ? CFG_LCD_WIDTH : topX;
            topY = topY >= CFG_LCD_HEIGHT ? CFG_LCD_HEIGHT : topY;
            botX = (botX < 0) ? 0 : botX;
            botY = (botY < 0) ? 0 : botY;
            botX = botX >= CFG_LCD_WIDTH ? CFG_LCD_WIDTH - 1 : botX;
            botY = botY >= CFG_LCD_HEIGHT ? CFG_LCD_HEIGHT - 1 : botY;
#endif
        }

        gfxMatrixIdentify(&mtx);

        // Adjust the coordinate
        gfxMatrixTranslate(&mtx, dstSurface->dstX - topX, dstSurface->dstY - topY);

        if ((scaleWidth != 1.0) || (scaleHeight != 1.0))
        {
            gfxMatrixScale(&mtx, scaleWidth, scaleHeight);
        }

        if ((refX != 0) || (refY != 0))
        {
            gfxMatrixTranslate(&mtx, refX, refY);
        }

        if ((topX < 0) || (topY < 0) || (botX > dstSurface->dstSurface->width) || (botY > dstSurface->dstSurface->height))
        {
            clip_x0     = (topX < 0) ? 0 : topX;
            clip_y0     = (topY < 0) ? 0 : topY;
            clip_x1     = (botX > dstSurface->dstSurface->width) ? dstSurface->dstSurface->width : botX;
            clip_y1     = (botY > dstSurface->dstSurface->height) ? dstSurface->dstSurface->height :  botY;
            extra_clip  = true;
        }

        if (degree != 0.0)
        {
            gfxMatrixRotate(&mtx, degree);
        }

        if ((refX != 0) || (refY != 0))
        {
            gfxMatrixTranslate(&mtx, -refX, -refY);
        }

        new_dstX        = topX;
        new_dstY        = topY;
        new_dstWidth    = botX - topX + 1;
        new_dstHeight   = botY - topY + 1;
    }
    else     // pad, repeat, reflect mode
    {
        new_dstX        = dstSurface->dstX;
        new_dstY        = dstSurface->dstY;
        new_dstWidth    = dstSurface->dstWidth;
        new_dstHeight   = dstSurface->dstHeight;
    }

    // (void)printf("new dst (%d, %d)(%d, %d)\n", new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    if (extra_clip)
    {
        if (dstSurface->dstSurface != NULL && dstSurface->dstSurface->clipEnable)
        {
            clip_x0 = MAX(clip_x0, dstSurface->dstSurface->clipSet.left);
            clip_y0 = MAX(clip_y0, dstSurface->dstSurface->clipSet.top);
            clip_x1 = MIN(clip_x1, dstSurface->dstSurface->clipSet.right);
            clip_y1 = MIN(clip_y1, dstSurface->dstSurface->clipSet.bottom);
        }
        REG_FILL_DST_CLIP(regs,
                          clip_x0,
                          clip_y0,
                          clip_x1,
                          clip_y1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
        // (void)printf("Clip top (%d, %d), bot (%d, %d)\n", clip_x0, clip_y0, clip_x1, clip_y1);
    }
    else
    {
        if (dstSurface->dstSurface != NULL && dstSurface->dstSurface->clipEnable)
        {
            REG_FILL_DST_CLIP(
                regs,
                dstSurface->dstSurface->clipSet.left,
                dstSurface->dstSurface->clipSet.top,
                dstSurface->dstSurface->clipSet.right,
                dstSurface->dstSurface->clipSet.bottom);
            regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
        }
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, rop);

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_FGCOLOR(regs, dstSurface->dstSurface->fgColor);
        // Mask
        REG_FILL_MASK_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    // Control 1
    regs->regCR1.data |= GFX_REG_CR1_TRANSFORM_EN
                         | ((tilemode << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    if (dstSurface->dstSurface != NULL)
    {
        if (dstSurface->dstSurface->quality == GFX_QUALITY_BETTER)
        {
            regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;

            // enable alpha blending to do the edge anti-aliasing
            if ((tilemode == GFX_TILE_FILL) && ((degree_i != 0) && (degree_i != 90) && (degree_i != 180) && (degree_i != 270)))
            {
                regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
            }
        }
    }

    // Temp solution for rotate LCD application to speed up the performance
    // if quality set faster mode, disable alphablending
    if (dstSurface->dstSurface != NULL && (dstSurface->dstSurface->quality == GFX_QUALITY_FASTER))
    {
        regs->regCR1.data = regs->regCR1.data & ~(GFX_REG_CR1_ALPHABLEND_EN);
    }

    // Change the scan direction to increase the hit rate of cache
    // (it only supports after IT970 project)
    if (((degree_i > 45 * 1) && (degree_i < 45 * 3)) || ((degree_i > 45 * 5) && (degree_i < 45 * 7)))
    {
        regs->regCR1.data |= GFX_REG_CR1_ROWMAJOR;
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceFillColor (
    GFXSurfaceDst   * surface,
    GFXColor        color,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    gfxHwReset(driver->hwDev);

    // Surface
    if (surface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, surface->dstSurface, surface->dstX, surface->dstY, surface->dstWidth, surface->dstHeight);
    }

    // Clipping
    if (surface->dstSurface != NULL && surface->dstSurface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
                          surface->dstSurface->clipSet.left,
                          surface->dstSurface->clipSet.top,
                          surface->dstSurface->clipSet.right,
                          surface->dstSurface->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    REG_FILL_FGCOLOR(regs, color);

    // Mask
    if (surface->dstSurface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, surface->dstSurface, surface->dstX, surface->dstY, surface->dstWidth, surface->dstHeight);
    }

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_PATCOPY);

    if (surface->dstSurface != NULL)
    {
        // Control Bits
        regs->regCR1.data   |= GFX_REG_CR1_TILEMODE_FILL;
        regs->regCR2.data   |=
            (surface->dstSurface->format << GFX_REG_CR2_DSTFORMAT_OFFSET)
            | (surface->dstSurface->format << GFX_REG_CR2_SRCFORMAT_OFFSET);
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceGradientFill (
    GFXSurfaceDst   * surface,
    GFXColor        initcolor,
    GFXColor        endcolor,
    GFXGradDir      dir,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    if ((surface->dstWidth <= 0) || (surface->dstHeight <= 0))
    {
        return true;
    }

    gfxHwReset(driver->hwDev);

    if (surface->dstSurface != NULL)
    {
        // Surface
        REG_FILL_DST_SURFACE(regs, surface->dstSurface, surface->dstX, surface->dstY, surface->dstWidth, surface->dstHeight);
    }

    // Clipping
    if (surface->dstSurface != NULL && surface->dstSurface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
                          surface->dstSurface->clipSet.left,
                          surface->dstSurface->clipSet.top,
                          surface->dstSurface->clipSet.right,
                          surface->dstSurface->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Mask
    if (surface->dstSurface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, surface->dstSurface, surface->dstX, surface->dstY, surface->dstWidth, surface->dstHeight);
    }

    // Initial Color
    REG_FILL_FGCOLOR(regs, initcolor);

    // Gradient Direction
    switch (dir)
    {
        case GFX_GRAD_H: regs->regCR2.data      |= GFX_REG_CR2_GRADMODE_H;
            break;

        case GFX_GRAD_V: regs->regCR2.data      |= GFX_REG_CR2_GRADMODE_V;
            break;

        case GFX_GRAD_BOTH: regs->regCR2.data   |= GFX_REG_CR2_GRADMODE_BI;
            break;

        default: result                         = false;
            goto end;
    }

    // Color Delta
    if (dir == GFX_GRAD_H)
    {
        regs->regITMR00.data = ((((int32_t)endcolor.a - (int32_t)initcolor.a) * (1 << 12)) / surface->dstWidth) * (1 << 4);
        regs->regITMR01.data = ((((int32_t)endcolor.r - (int32_t)initcolor.r) * (1 << 12)) / surface->dstWidth) * (1 << 4);
        regs->regITMR02.data = ((((int32_t)endcolor.g - (int32_t)initcolor.g) * (1 << 12)) / surface->dstWidth) * (1 << 4);
        regs->regITMR10.data = ((((int32_t)endcolor.b - (int32_t)initcolor.b) * (1 << 12)) / surface->dstWidth) * (1 << 4);
    }

    if (dir == GFX_GRAD_V)
    {
        regs->regITMR11.data = ((((int32_t)endcolor.a - (int32_t)initcolor.a) * (1 << 12)) / surface->dstHeight) * (1 << 4);
        regs->regITMR12.data = ((((int32_t)endcolor.r - (int32_t)initcolor.r) * (1 << 12)) / surface->dstHeight) * (1 << 4);
        regs->regITMR20.data = ((((int32_t)endcolor.g - (int32_t)initcolor.g) * (1 << 12)) / surface->dstHeight) * (1 << 4);
        regs->regITMR21.data = ((((int32_t)endcolor.b - (int32_t)initcolor.b) * (1 << 12)) / surface->dstHeight) * (1 << 4);
    }

    if (dir == GFX_GRAD_BOTH)
    {
        regs->regITMR00.data = ((((int32_t)endcolor.a - (int32_t)initcolor.a) * (1 << 11)) / surface->dstWidth) * (1 << 4);
        regs->regITMR01.data = ((((int32_t)endcolor.r - (int32_t)initcolor.r) * (1 << 11)) / surface->dstWidth) * (1 << 4);
        regs->regITMR02.data = ((((int32_t)endcolor.g - (int32_t)initcolor.g) * (1 << 11)) / surface->dstWidth) * (1 << 4);
        regs->regITMR10.data = ((((int32_t)endcolor.b - (int32_t)initcolor.b) * (1 << 11)) / surface->dstWidth) * (1 << 4);

        regs->regITMR11.data = ((((int32_t)endcolor.a - (int32_t)initcolor.a) * (1 << 11)) / surface->dstHeight) * (1 << 4);
        regs->regITMR12.data = ((((int32_t)endcolor.r - (int32_t)initcolor.r) * (1 << 11)) / surface->dstHeight) * (1 << 4);
        regs->regITMR20.data = ((((int32_t)endcolor.g - (int32_t)initcolor.g) * (1 << 11)) / surface->dstHeight) * (1 << 4);
        regs->regITMR21.data = ((((int32_t)endcolor.b - (int32_t)initcolor.b) * (1 << 11)) / surface->dstHeight) * (1 << 4);
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_GRADIENT_FILL;

    result              = gfxHwEngineFire(driver->hwDev);

end:
    return result;
}

static bool
_SurfaceProjection (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface,
    float           scaleWidth,
    float           scaleHeight,
    float           degree,
    float           FOV,
    float           pivotX,
    GFX_ROP3        rop,
    GFXAlphaBlend   * alphaBlend)
{
    // degree: angle in radius on which the image is rotating (image rotates on the Y axis)
    // FOV: Field of view, ratio between the projection plane and the actual object
    // pivotX: Pivot point in X; indicates which point in x to use for rotation.

    bool            result      = false;
    GFX_DRIVER      * driver    = gfxGetDriver();
    GFX_HW_REGS     * regs      = &driver->hwDev->regs;

    GFX_RECTANGLE   d;
    GFX_RECTANGLE   s;
    GFX_MATRIX      mtx;
    GFX_MATRIX      inverseMtx;

    float           Xw;         // Holds the proportional width due to the rotation
    float           Yo;         // Holds the proportional height on the reduced side
    float           Rx;         // Delta width ( Rx + Xw = width)
    float           Po, Pon;    // ratio of reduction affected by X pivot point offset
    float           rad = (degree * GFX_PI / 180.0f);

    gfxHwReset(driver->hwDev);

    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));
    gfxMatrixIdentify(&mtx);

    Xw = cos(rad) * srcSurface->srcWidth;                                       // transformed width

    if (Xw >= 0)                                                                // width delta
    {
        Rx = srcSurface->srcWidth - Xw;
    }
    else
    {
        Rx = srcSurface->srcWidth + Xw;
    }

    Yo  = sin(rad) * (srcSurface->srcHeight / 2) * (FOV * GFX_PI / 180.0f);     // height reduction due to horizontal angle
    Po  = pivotX / srcSurface->srcWidth;                                        // ratio of reduction affected by X pivot point offset
    Pon = 1.0f - Po;                                                            // inverse ..

#if 1
    s.x0    = 0.0f;
    s.y0    = (float)srcSurface->srcHeight;
    s.x1    = (float)srcSurface->srcWidth;
    s.y1    = (float)srcSurface->srcHeight;
    s.x2    = (float)srcSurface->srcWidth;
    s.y2    = 0.0f;
    s.x3    = 0.0f;
    s.y3    = 0.0f;

    d.x0    = (Po * Rx);
    d.y0    = srcSurface->srcHeight + (Po * Yo);
    d.x1    = (Po * Rx) + Xw;
    d.y1    = srcSurface->srcHeight - (Pon * Yo);
    d.x2    = (Po * Rx) + Xw;
    d.y2    = (Pon * Yo);
    d.x3    = (Po * Rx);
    d.y3    = (Po * -Yo);
#else
    d.x0    = 0;
    d.y0    = dstSurface->dstHeight;
    d.x1    = dstSurface->dstWidth;
    d.y1    = dstSurface->dstHeight;
    d.x2    = dstSurface->dstWidth;
    d.y2    = 0;
    d.x3    = 0;
    d.y3    = 0;

    s.x0    = (Po * Rx);
    s.y0    = srcSurface->srcHeight + (Po * Yo);
    s.x1    = (Po * Rx) + Xw;
    s.y1    = srcSurface->srcHeight - (Pon * Yo);
    s.x2    = (Po * Rx) + Xw;
    s.y2    = (Pon * Yo);
    s.x3    = (Po * Rx);
    s.y3    = (Po * -Yo);
#endif

    (void)gfxMatrixWarpQuadToQuad(d, s, &mtx, false);

    mtx.m[0][2] = (float)dstSurface->dstX;
    mtx.m[1][2] = (float)dstSurface->dstY;

    if ((scaleWidth != 1.0) || (scaleHeight != 1.0))
    {
        mtx.m[0][0] = mtx.m[0][0] * scaleWidth;
        mtx.m[1][1] = mtx.m[1][1] * scaleHeight;
    }

    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, 0, 0, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, rop);

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_FGCOLOR(regs, dstSurface->dstSurface->fgColor);
    }

    // Mask
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, dstSurface->dstSurface, dstSurface->dstX, dstSurface->dstY, dstSurface->dstWidth, dstSurface->dstHeight);
    }

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    if (dstSurface->dstSurface != NULL)
    {
        if (dstSurface->dstSurface->quality == GFX_QUALITY_BETTER)
        {
            regs->regCR1.data   |= GFX_REG_CR1_INTERPOLATE_EN;
            regs->regCR1.data   |= GFX_REG_CR1_ALPHABLEND_EN;
        }

        // Clipping
        if (dstSurface->dstSurface->clipEnable)
        {
            REG_FILL_DST_CLIP(regs,
                dstSurface->dstSurface->clipSet.left,
                dstSurface->dstSurface->clipSet.top,
                dstSurface->dstSurface->clipSet.right,
                dstSurface->dstSurface->clipSet.bottom);
            regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
        }
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceTransformBlt (
    GFXSurface          * dest,
    int32_t             dx,
    int32_t             dy,
    GFXSurface          * src,
    int32_t             sx,
    int32_t             sy,
    int32_t             sw,
    int32_t             sh,
    int32_t             x0,
    int32_t             y0,
    int32_t             x1,
    int32_t             y1,
    int32_t             x2,
    int32_t             y2,
    int32_t             x3,
    int32_t             y3,
    bool                bInverse,
    GFXPageFlowType     type,
    GFXTransformType    transformType)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    GFX_MATRIX  mtx;
    GFX_MATRIX  mtx_tmp;
    GFX_MATRIX  inverseMtx;

    float       Xw = 0.0f, w = 0.0f, h = 0.0f, offsetx = 0.0f, offsety = 0.0f;
    float       offsetx0 = 0.0f, offsetx1 = 0.0f, offsety0 = 0.0f, offsety1 = 0.0f;
    // int32_t       minX = 0;
    int32_t     minY            = 0;
    int32_t     newDestWidth    = 0;
    int32_t     newDestheight   = 0;
    // int32_t       topX = 0, topY = 0, botX = 0, botY = 0;

    gfxHwReset(driver->hwDev);

    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));
    gfxMatrixIdentify(&mtx);
    gfxMatrixIdentify(&mtx_tmp);

    if ((dest == 0) || (src == 0))
    {
        return result;
    }

    newDestWidth    = dest->width - dx;
    newDestheight   = dest->height - dy;

    w       = (float)src->width;
    h       = (float)src->height;

    offsetx = (float)(src->width - abs(x2));
    if (offsetx == 0)
    {
        Xw = (float)x1 - (float)x3;

        if (Xw >= 0)
        {
            offsetx = (float)src->width - Xw;
        }
        else
        {
            offsetx = (float)src->width + Xw;
        }
    }

    // minX = MIN(x0, x1);
    minY = MIN(y0, y1);

    if (minY < 0)
    {
        gfxMatrixTranslate(&mtx, 0, -(minY));
    }

    if ((transformType == GFX_TRANSFORM_TURN_LEFT) || (transformType == GFX_TRANSFORM_TURN_RIGHT))  // horizontal
    {
#ifdef GENERAL_ALGORITHM
        if ((y0 == 0) && (y3 == h))                                                                 // left y0=0 y3=h
        {
            offsetx0        = (float)(src->width - abs(x2));
            offsetx1        = (float)x0;

            offsety0        = (float)y2 - (float)y3;
            offsety1        = (float)y1 - (float)y0;

            // (void)printf("offset y(%f,%f) x(%f %f)\n", offsety0, offsety1, offsetx0, offsetx1);
            mtx_tmp.m[0][0] = ((w - offsetx0) / w) * (1 + ((offsety0 - offsety1) / (offsety1 - offsety0 - h))) - (offsetx1 / w);
            mtx_tmp.m[0][1] = 0.0f;
            mtx_tmp.m[0][2] = offsetx1;
            mtx_tmp.m[1][0] = (offsety1 / w) * (1 + ((offsety0 - offsety1) / (offsety1 - offsety0 - h)));
            mtx_tmp.m[1][1] = 1.0f;
            mtx_tmp.m[1][2] = 0.0f;
            mtx_tmp.m[2][0] = (1 / w) * ((offsety0 - offsety1) / (offsety1 - offsety0 - h));
            mtx_tmp.m[2][1] = 0.0f;
            mtx_tmp.m[2][2] = 1.0f;

            if ((type == GFX_PAGEFLOW_FOLD) && (y1 != 0))
            {
                newDestWidth = x1 / 2;
            }
        }
#else
        if (bInverse)
        {
            if (((type == GFX_PAGEFLOW_FOLD) || (type == GFX_PAGEFLOW_FOLD2)) && (y1 == 0))
            {
                offsety = (float)y0;

                // (void)printf("offset 1(%f,%f)\n", offsetx, offsety);
                // (void)printf("bInverse minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1 - (offsetx / w) - ((2 * offsety * (w - offsetx)) / (w * h));
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = 0.0f;
                mtx_tmp.m[1][0] = (-offsety) / w;
                mtx_tmp.m[1][1] = 1 - ((2 * offsety) / h);
                mtx_tmp.m[1][2] = offsety;
                mtx_tmp.m[2][0] = (-2 * offsety) / (w * h);
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;
            }
            else if (type == GFX_PAGEFLOW_QUADRANGLE)
            {
                float offsety0, offsety1;

                offsety0        = (float)y2 - (float)y3;
                offsety1        = (float)y1 - (float)y0;

                // (void)printf("offset y(%f,%f)\n", offsety0, offsety1);
                mtx_tmp.m[0][0] = 1 + ((offsety1 - offsety0) / (h + offsety0 - offsety1));
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = 0.0f;
                mtx_tmp.m[1][0] = (offsety1 / w) * (1 + ((offsety1 - offsety0) / (h + offsety0 - offsety1)));
                mtx_tmp.m[1][1] = 1.0f;
                mtx_tmp.m[1][2] = 0.0f;
                mtx_tmp.m[2][0] = (1 / w) * ((offsety1 - offsety0) / (h + offsety0 - offsety1));
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;
            }
            else
            {
                offsety = (float)(-y1);

                // (void)printf("offset 2(%f,%f)\n", offsetx, offsety);
                // (void)printf("bInverse minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1 - (offsetx / w) - ((2 * offsety * (w - offsetx)) / (w * (h + (2 * offsety))));
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = 0.0f;
                mtx_tmp.m[1][0] = ((2 * offsety * offsety) / (w * (h + (2 * offsety)))) - (offsety / w);
                mtx_tmp.m[1][1] = 1.0f;
                mtx_tmp.m[1][2] = 0.0f;
                mtx_tmp.m[2][0] = -(2 * offsety) / (w * (h + (2 * offsety)));
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;

                if (type == GFX_PAGEFLOW_FOLD)
                {
                    newDestWidth = x1 / 2;
                }
            }
        }
#endif
#ifdef GENERAL_ALGORITHM
        else if ((y1 == 0) && (y2 == h)) // right y1=0 y2=h
        {
            offsetx0        = (float)x0;
            offsetx1        = (float)(src->width - abs(x2));

            offsety0        = (float)y3 - (float)y2;
            offsety1        = (float)y0 - (float)y1;

            // (void)printf("offset2 y(%f,%f) x(%f %f)\n", offsety0, offsety1, offsetx0, offsetx1);
            mtx_tmp.m[0][0] = 1 - (offsetx0 / w) - (offsetx1 / w) + (((offsety0 - offsety1) * (w - offsetx1)) / (w * h));
            mtx_tmp.m[0][1] = 0.0f;
            mtx_tmp.m[0][2] = offsetx0;
            mtx_tmp.m[1][0] = -(offsety1 / w);
            mtx_tmp.m[1][1] = 1 + ((offsety0 - offsety1) / h);
            mtx_tmp.m[1][2] = offsety1;
            mtx_tmp.m[2][0] = (offsety0 - offsety1) / (w * h);
            mtx_tmp.m[2][1] = 0.0f;
            mtx_tmp.m[2][2] = 1.0f;

            if ((type == GFX_PAGEFLOW_FOLD) && (y0 == 0))
            {
                newDestWidth = (x1 - x0) / 2 + x0;
            }
        }
#else
        else // no bInverse
        {
            if (((type == GFX_PAGEFLOW_FOLD) || (type == GFX_PAGEFLOW_FOLD2)) && (y0 == 0))
            {
                offsety = (float)y1;

                // (void)printf("offset 3(%f,%f)\n", offsetx, offsety);
                // (void)printf("minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1 - (offsetx / w) - ((2 * offsety) / ((2 * offsety) - h));
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = offsetx;
                mtx_tmp.m[1][0] = (offsety / w) - ((2 * offsety * offsety) / (w * ((2 * offsety) - h)));
                mtx_tmp.m[1][1] = 1.0f;
                mtx_tmp.m[1][2] = 0.0f;
                mtx_tmp.m[2][0] = -(2 * offsety) / (w * ((2 * offsety) - h));
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;

                if (type == GFX_PAGEFLOW_FOLD)
                {
                    newDestWidth = (x1 - x0) / 2 + x0;
                }
            }
            else if (type == GFX_PAGEFLOW_QUADRANGLE)
            {
                float offsety0, offsety1;

                offsety0        = (float)y3 - (float)y2;
                offsety1        = (float)y0 - (float)y1;

                // (void)printf("offset y(%f,%f)\n", offsety0, offsety1);
                mtx_tmp.m[0][0] = 1 + ((offsety0 - offsety1) / h);
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = 0.0f;
                mtx_tmp.m[1][0] = -(offsety1) / w;
                mtx_tmp.m[1][1] = 1 + ((offsety0 - offsety1) / h);
                mtx_tmp.m[1][2] = offsety1;
                mtx_tmp.m[2][0] = (offsety0 - offsety1) / (w * h);
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;
            }
            else
            {
                offsety = (float)(-y0);

                // (void)printf("offset 4(%f,%f)\n", offsetx, offsety);
                // (void)printf("minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1 - (offsetx / w) + ((2 * offsety) / h);
                mtx_tmp.m[0][1] = 0.0f;
                mtx_tmp.m[0][2] = offsetx;
                mtx_tmp.m[1][0] = (offsety / w);
                mtx_tmp.m[1][1] = 1 + ((2 * offsety) / h);
                mtx_tmp.m[1][2] = (-offsety);
                mtx_tmp.m[2][0] = (2 * offsety) / (w * h);
                mtx_tmp.m[2][1] = 0.0f;
                mtx_tmp.m[2][2] = 1.0f;
            }
        }
#endif
    }
    else if ((transformType == GFX_TRANSFORM_TURN_TOP) || (transformType == GFX_TRANSFORM_TURN_BOTTOM)) // vertical
    {
        if (bInverse)                                                                                   // Turn to top side
        {
            if (type == GFX_PAGEFLOW_FOLD2)
            {
                offsety = (float)y0;

                // (void)printf("vertical offset 0(%f,%f)\n", offsetx, offsety);
                // (void)printf("bInverse minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1 - (2 * offsetx / w);
                mtx_tmp.m[0][1] = -(offsetx / h);
                mtx_tmp.m[0][2] = offsetx;
                mtx_tmp.m[1][0] = 0.0f;
                mtx_tmp.m[1][1] = 1 - ((2 * offsety) / h) - ((2 * offsetx * (h - offsety)) / (w * h));
                mtx_tmp.m[1][2] = offsety;
                mtx_tmp.m[2][0] = 0.0f;
                mtx_tmp.m[2][1] = -((2 * offsetx) / (w * h));
                mtx_tmp.m[2][2] = 1.0f;
            }
        }
        else // Turn to bottom side
        {
            if (type == GFX_PAGEFLOW_FOLD2)
            {
                offsety = (float)y1;

                // (void)printf("vertical offset 1(%f,%f)\n", offsetx, offsety);
                // (void)printf("minX:%d, minY:%d\n", minX, minY);
                mtx_tmp.m[0][0] = 1.0f;
                mtx_tmp.m[0][1] = (offsetx / h) + ((2 * offsetx * offsetx) / (h * (w - (2 * offsetx))));
                mtx_tmp.m[0][2] = 0.0f;
                mtx_tmp.m[1][0] = 0.0f;
                mtx_tmp.m[1][1] = 1 - ((2 * offsety) / h) + ((2 * offsetx * (h - offsety)) / (h * (w - (2 * offsetx))));
                mtx_tmp.m[1][2] = offsety;
                mtx_tmp.m[2][0] = 0.0f;
                mtx_tmp.m[2][1] = ((2 * offsetx) / (h * (w - (2 * offsetx))));
                mtx_tmp.m[2][2] = 1.0f;
            }
        }

        // minX = 0;
        minY = 0;
    }

    gfxMatrixMultiply(&mtx, &mtx_tmp);

    if (type == GFX_PAGEFLOW_FOLD)
    {
        sw = src->width;
    }

#ifndef GENERAL_ALGORITHM
    if (!offsetx && !offsety && (type != GFX_PAGEFLOW_QUADRANGLE))
#else
    if (!offsetx0 && !offsetx1 && !offsety0 && !offsety1)
#endif
    {
        newDestWidth    = w;
        newDestheight   = h;
    }

    {
        REG_FILL_DST_SURFACE(regs, dest, dx, dy + minY, newDestWidth, newDestheight - minY);
    }

    {
        REG_FILL_SRC_SURFACE(regs, src, sx, sy, sw, sh);
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // About blending
    REG_FILL_BLENDING(regs, 0, 0, 0);

    // Mask
    {
        REG_FILL_MASK_SURFACE(regs, dest, dx, dy + minY, newDestWidth, newDestheight - minY);
    }

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    {
        if (dest->quality == GFX_QUALITY_BETTER)
        {
            regs->regCR1.data   |= GFX_REG_CR1_INTERPOLATE_EN;
            regs->regCR1.data   |= GFX_REG_CR1_ALPHABLEND_EN;
        }
    }

    // Clipping
    if (dest->clipEnable)
    {
        dest->clipSet.top = dest->clipSet.top + minY;

        if (dest->clipSet.top < 0)
        {
            dest->clipSet.top = 0;
        }

        REG_FILL_DST_CLIP(regs,
            dest->clipSet.left,
            dest->clipSet.top,
            dest->clipSet.right,
            dest->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static void
_setFGColor (
    GFXSurface  * dstSurface,
    GFXColor    color)
{
    if (dstSurface != NULL)
    {
        dstSurface->fgColor.a   = color.a;
        dstSurface->fgColor.r   = color.r;
        dstSurface->fgColor.g   = color.g;
        dstSurface->fgColor.b   = color.b;
    }
}

static bool
_SurfaceDrawLine (
    GFXSurface      * surface,
    int32_t         fromX,
    int32_t         fromY,
    int32_t         toX,
    int32_t         toY,
    GFXColor        * lineColor,
    int32_t         lineWidth,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;

    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    gfxHwReset(driver->hwDev);

    // FIXME
    if (surface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, surface, (int32_t)0, (int32_t)0, surface->width, surface->height);
    }

    // Clipping
    if (surface != NULL && surface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
                          surface->clipSet.left,
                          surface->clipSet.top,
                          surface->clipSet.right,
                          surface->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // Start x,y
    REG_FILL_LINEDRAW_STARTXY(fromX, fromY);

    // End x,y
    REG_FILL_LINEDRAW_ENDXY(toX, toY);

    // Line Color
    REG_FILL_FGCOLOR(regs, (*lineColor));

    // About blending
    REG_FILL_BLENDING(regs, true, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Mask
    if (surface != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, surface, 0, 0, surface->width, surface->height);
    }

    if ((lineWidth == 1) && surface != NULL && (surface->quality == GFX_QUALITY_FASTER))
    {
        regs->regCR3.data |= GFX_REG_CR3_LINE_TYPE0;
    }
    else
    {
        int32_t dx  = abs(fromX - toX);
        int32_t dy  = abs(fromY - toY);
        float   d   = ((dx + dy) == 0) ? 1.0f : sqrtf((float) dx * dx + (float) dy * dy);
        int     p2  = (int)(d * (lineWidth + 1) / 2);
        int     p1  = (int)((1 << 20) / d);

        if (surface != NULL && (surface->quality == GFX_QUALITY_BETTER))
        {
            regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
        }

        regs->regCR3.data       |= GFX_REG_CR3_LINE_TYPE1;
        regs->regCR3.data       |= ((lineWidth & GFX_REG_CR3_LINE_WIDTH_MASK) << GFX_REG_CR3_LINE_WIDTH_OFFSET) |
                             ((p2 & GFX_REG_CR3_LINE_P2_MASK) << GFX_REG_CR3_LINE_P2_OFFSET);

        regs->regLINEP1.data    = p1 & GFX_REG_LINE_P1_MASK;
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_LINE_DRAW;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceDrawPixelAA (
    GFXSurface  * surface,
    int         x,
    int         y,
    GFXColor    * color,
    int         lineWidth)
{
    bool            result = false;
    GFXAlphaBlend   alphaBlend = {0};
    GFXSurfaceDst   dstSurface = {0};
    GFXColor        pixelcolor = {0};
    float           d, alpha;
    int32_t         x1, y1, x2, y2;
    int32_t         px, py;
    int32_t         cr;

    GFX_FUNC_ENTRY;
    if (surface == NULL)
    {
        GFX_ERROR_MSG("Invalid destination surface.\n");
        return false;
    }

    gfxLock();

    alphaBlend.enableAlpha          = true;
    alphaBlend.enableConstantAlpha  = false;
    alphaBlend.constantAlphaValue   = 0;

    alpha                           = (float)(2 * (color->a) / lineWidth);
    if (alpha > 255)
    {
        alpha = (float)255;
    }

    pixelcolor.r    = color->r;
    pixelcolor.g    = color->g;
    pixelcolor.b    = color->b;

    // radius = 1.1;
    cr              = floor(lineWidth / 1.5);

    x1              = floor(x - lineWidth + 0.3);
    y1              = floor(y - lineWidth + 0.3);
    x2              = floor(x + lineWidth - 0.3);
    y2              = floor(y + lineWidth - 0.3);

    // (void)printf("cr:%d, x1:%d,y1:%d,x2:%d,y2:%d\n", cr, x1, y1, x2, y2);
    for (py = y1, px = x1; py <= y2 && px <= x2; py++, px++)
    {
        d   = sqrt(pow(px - x, 2) + pow(py - y, 2));
        d   = d / lineWidth;
        // (void)printf("d:%f\n",d);
        if (d < 1)
        {
            d                       = pow(d, cr);

            dstSurface.dstSurface   = surface;
            dstSurface.dstX         = px;
            dstSurface.dstY         = py;
            dstSurface.dstWidth     = lineWidth;
            dstSurface.dstHeight    = lineWidth;

            pixelcolor.a            = floor(alpha * (1 - d));
            // (void)printf("pixelcolor.a:%d\n", pixelcolor.a);
            result                  = _SurfaceFillColor(&dstSurface, pixelcolor, &alphaBlend);
        }
    }

    gfxUnlock();
    GFX_FUNC_LEAVE;

    return result;
}

static bool
_SurfaceReflected (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy,
    int32_t     reflectedWidth,
    int32_t     reflectedHeight)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;

    gfxHwReset(driver->hwDev);

    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));
    gfxMatrixIdentify(&mtx);

    mtx.m[0][0] = 1.0f;
    mtx.m[0][1] = 0.0f;
    mtx.m[0][2] = 0.0f;
    mtx.m[1][0] = 0.0f;
    mtx.m[1][1] = -1.0f;
    mtx.m[1][2] = (float)reflectedHeight;
    mtx.m[2][0] = 0.0f;
    mtx.m[2][1] = 0.0f;
    mtx.m[2][2] = 1.0f;

    if (dest != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dest, dx, dy, reflectedWidth, reflectedHeight);
    }

    if (src != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, src, sx, sy, reflectedWidth, reflectedHeight);
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // About blending
    REG_FILL_BLENDING(regs, 0, 0, 0);

    // Mask
    if (dest != NULL)
    {
        REG_FILL_MASK_SURFACE(regs, dest, 0, 0, reflectedWidth, reflectedHeight);
    }

    // Control 1
    regs->regCR1.data |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);


    // Clipping
    if (dest != NULL && dest->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
            dest->clipSet.left,
            dest->clipSet.top,
            dest->clipSet.right,
            dest->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceMirror (
    GFXSurface  * dest,
    int32_t     dx,
    int32_t     dy,
    GFXSurface  * src,
    int32_t     sx,
    int32_t     sy)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;

    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;

    gfxHwReset(driver->hwDev);

    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));
    gfxMatrixIdentify(&mtx);

    if (src != NULL)
    {
        mtx.m[0][0] = -1.0f;
        mtx.m[0][1] = 0.0f;
        mtx.m[0][2] = (float)src->width;
        mtx.m[1][0] = 0.0f;
        mtx.m[1][1] = 1.0f;
        mtx.m[1][2] = 0.0f;
        mtx.m[2][0] = 0.0f;
        mtx.m[2][1] = 0.0f;
        mtx.m[2][2] = 1.0f;
    }

    if (dest != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dest, dx, dy, dest->width, dest->height);
    }

    if (src != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, src, sx, sy, src->width, src->height);
    }

    (void)gfxMatrixInverse(&mtx, &inverseMtx);
    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // About blending
    REG_FILL_BLENDING(regs, true, 0, 0);

    // Control 1
    regs->regCR1.data |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    // Clipping
    if (dest != NULL && dest->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
            dest->clipSet.left,
            dest->clipSet.top,
            dest->clipSet.right,
            dest->clipSet.bottom);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    // Fire
    regs->regCMD.data   = GFX_REG_CMD_ROP3;

    result              = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceShearing (
    GFXSurfaceDst   * dstSurface,
    GFX_RECTANGLE   coordinate,
    GFXSurfaceSrc   * srcSurface,
    GFXAlphaBlend   * alphaBlend)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;
    GFX_MATRIX  mtx;
    GFX_MATRIX  inverseMtx;
    GFX_MATRIX  transform;

    int32_t     dx      = dstSurface->dstX;
    int32_t     dy      = dstSurface->dstY;
    int32_t     srcW    = srcSurface->srcWidth;
    int32_t     srcH    = srcSurface->srcHeight;

    int32_t new_dstX;
    int32_t new_dstY;
    int32_t new_dstWidth;
    int32_t new_dstHeight;

    float   dx1, dx2, dx3, dy1, dy2, dy3;
    float   denominator;

    float   u3, v1;
    float   x0, x1, x2, x3, y0, y1, y2, y3;

    gfxHwReset(driver->hwDev);
    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));
    gfxMatrixIdentify(&mtx);
    gfxMatrixIdentify(&transform);

    u3                                      = (float)srcW;
    v1                                      = (float)srcH;

    x0                                      = coordinate.x0;
    x1                                      = coordinate.x1;
    x2                                      = coordinate.x2;
    x3                                      = coordinate.x3;
    y0                                      = coordinate.y0;
    y1                                      = coordinate.y1;
    y2                                      = coordinate.y2;
    y3                                      = coordinate.y3;

    dx1                                     = x3 - x2;
    dx2                                     = x1 - x2;
    dx3                                     = x0 - x3 + x2 - x1;
    dy1                                     = y3 - y2;
    dy2                                     = y1 - y2;
    dy3                                     = y0 - y3 + y2 - y1;

    denominator                             = dx1 * dy2 - dx2 * dy1;

    if (denominator == 0.0f)
    {
        transform.m[2][0]   = (dx3 * dy2 - dx2 * dy3) / u3;
        transform.m[2][1]   = (dx1 * dy3 - dx3 * dy1) / v1;
    }
    else
    {
        transform.m[2][0]   = (dx3 * dy2 - dx2 * dy3) / u3 / denominator;
        transform.m[2][1]   = (dx1 * dy3 - dx3 * dy1) / v1 / denominator;
    }

    transform.m[0][0]   = (x3 - x0) / u3 + (transform.m[2][0] * x3);
    transform.m[0][1]   = (x1 - x0) / v1 + (transform.m[2][1] * x1);
    transform.m[0][2]   = x0;
    transform.m[1][2]   = y0;
    transform.m[1][0]   = (y3 - y0) / u3 + (transform.m[2][0] * y3);
    transform.m[1][1]   = (y1 - y0) / v1 + (transform.m[2][1] * y1);
    transform.m[2][2]   = 1.0f;

    if (transform.m[0][0] == 0.0f)
    {
        transform.m[0][0] = 1.0f;
    }
    if (transform.m[1][1] == 0.0f)
    {
        transform.m[1][1] = 1.0f;
    }

    mtx.m[0][0] = 1.0f;
    mtx.m[0][1] = 0.0f;
    mtx.m[0][2] = (float)dx;
    mtx.m[1][0] = 0.0f;
    mtx.m[1][1] = 1.0f;
    mtx.m[1][2] = (float)dy;
    mtx.m[2][0] = 0.0f;
    mtx.m[2][1] = 0.0f;
    mtx.m[2][2] = 1.0f;

    gfxMatrixMultiply(&mtx, &transform);

    {
        float   topLX, topLY, topRX, topRY;
        float   botLX, botLY, botRX, botRY;
        int32_t topX;
        int32_t topY;
        int32_t botX;
        int32_t botY;

        // Find the top left and bottom right coordinate
        topLX       = (mtx.m[0][2]) / (mtx.m[2][2]);
        topLY       = (mtx.m[1][2]) / (mtx.m[2][2]);

        topRX       = (srcW * mtx.m[0][0] + mtx.m[0][2]) /
                (srcW * mtx.m[2][0] + mtx.m[2][2]);
        topRY       = (srcW * mtx.m[1][0] + mtx.m[1][2]) /
                (srcW * mtx.m[2][0] + mtx.m[2][2]);

        botLX       = (srcH * mtx.m[0][1] + mtx.m[0][2]) /
                (srcH * mtx.m[2][1] + mtx.m[2][2]);
        botLY       = (srcH * mtx.m[1][1] + mtx.m[1][2]) /
                (srcH * mtx.m[2][1] + mtx.m[2][2]);

        botRX       = (srcW * mtx.m[0][0] + srcH * mtx.m[0][1] + mtx.m[0][2]) /
                (srcW * mtx.m[2][0] + srcH * mtx.m[2][1] + mtx.m[2][2]);
        botRY       = (srcW * mtx.m[1][0] + srcH * mtx.m[1][1] + mtx.m[1][2]) /
                (srcW * mtx.m[2][0] + srcH * mtx.m[2][1] + mtx.m[2][2]);

        topX        = (int32_t)floorf(MIN(MIN(topLX, topRX), MIN(botLX, botRX))) - 1;
        topY        = (int32_t)floorf(MIN(MIN(topLY, topRY), MIN(botLY, botRY))) - 1;
        botX        = (int32_t)ceilf(MAX(MAX(topLX, topRX), MAX(botLX, botRX))) + 1;
        botY        = (int32_t)ceilf(MAX(MAX(topLY, topRY), MAX(botLY, botRY))) + 1;

        {
            topX = (topX < 0) ? 0 : topX;
            topY = (topY < 0) ? 0 : topY;
            topX = topX >= CFG_LCD_WIDTH ? CFG_LCD_WIDTH : topX;
            topY = topY >= CFG_LCD_HEIGHT ? CFG_LCD_HEIGHT : topY;
            botX = (botX < 0) ? 0 : botX;
            botY = (botY < 0) ? 0 : botY;
            botX = botX >= CFG_LCD_WIDTH ? CFG_LCD_WIDTH - 1 : botX;
            botY = botY >= CFG_LCD_HEIGHT ? CFG_LCD_HEIGHT - 1 : botY;
        }

        mtx.m[0][0] = 1.0f;
        mtx.m[0][1] = 0.0f;
        mtx.m[0][2] = (float)(dx - topX);
        mtx.m[1][0] = 0.0f;
        mtx.m[1][1] = 1.0f;
        mtx.m[1][2] = (float)(dy - topY);
        mtx.m[2][0] = 0.0f;
        mtx.m[2][1] = 0.0f;
        mtx.m[2][2] = 1.0f;

        gfxMatrixMultiply(&mtx, &transform);

        (void)gfxMatrixInverse(&mtx, &inverseMtx);

        new_dstX        = topX;
        new_dstY        = topY;
        new_dstWidth    = botX - topX + 1;
        new_dstHeight   = botY - topY + 1;
    }

    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    if ((dstSurface->dstSurface != NULL) && dstSurface->dstSurface->clipEnable)
    {
        REG_FILL_DST_CLIP(regs,
            0,
            0,
            CFG_LCD_WIDTH - 1,
            CFG_LCD_HEIGHT - 1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // About blending
    REG_FILL_BLENDING(regs, alphaBlend->enableAlpha, alphaBlend->enableConstantAlpha, alphaBlend->constantAlphaValue);

    // Foreground Color
    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_FGCOLOR(regs, dstSurface->dstSurface->fgColor);
    }

    // Mask
    //REG_FILL_MASK_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    if (dstSurface->dstSurface != NULL)
    {
        regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;
        // regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
    }

    // Fire
    regs->regCMD.data = GFX_REG_CMD_ROP3;

    result = gfxHwEngineFire(driver->hwDev);

    return result;
}

static bool
_SurfaceTransformGetVersion (
    GFXSurfaceDst   * dstSurface,
    GFXSurfaceSrc   * srcSurface)
{
    bool        result      = false;
    GFX_DRIVER  * driver    = gfxGetDriver();
    GFX_HW_REGS * regs      = &driver->hwDev->regs;
    GFX_MATRIX  inverseMtx;

    int32_t     new_dstX;
    int32_t     new_dstY;
    int32_t     new_dstWidth;
    int32_t     new_dstHeight;

    gfxHwReset(driver->hwDev);
    (void)memset((void *)&inverseMtx, 0, sizeof(GFX_MATRIX));

    inverseMtx.m[0][0]  = 1.469833f;
    inverseMtx.m[0][1]  = -0.008197f;
    inverseMtx.m[0][2]  = -2.378002f;
    inverseMtx.m[1][0]  = 0.000000f;
    inverseMtx.m[1][1]  = 0.483428f;
    inverseMtx.m[1][2]  = -0.483428f;
    inverseMtx.m[2][0]  = -0.001127f;
    inverseMtx.m[2][1]  = 0.003209f;
    inverseMtx.m[2][2]  = 0.998622f;

    new_dstX        = 0;    // dstInfo[num].new_dstX;
    new_dstY        = -1;   // dstInfo[num].new_dstY;
    new_dstWidth    = 15;   // dstInfo[num].new_dstWidth;
    new_dstHeight   = 12;   // dstInfo[num].new_dstHeight;

    if (dstSurface->dstSurface != NULL)
    {
        REG_FILL_DST_SURFACE(regs, dstSurface->dstSurface, new_dstX, new_dstY, new_dstWidth, new_dstHeight);
    }

    if (srcSurface->srcSurface != NULL)
    {
        REG_FILL_SRC_SURFACE(regs, srcSurface->srcSurface, srcSurface->srcX, srcSurface->srcY, srcSurface->srcWidth, srcSurface->srcHeight);
    }

    // Clipping
    {
        REG_FILL_DST_CLIP(regs,
            0,
            0,
            16 - 1,
            4 - 1);
        regs->regCR1.data |= GFX_REG_CR1_CLIP_EN;
    }

    REG_FILL_SET_ITM(regs, inverseMtx);

    // ROP3
    REG_FILL_ROP3(regs, GFX_ROP3_SRCCOPY);

    // Control 1
    regs->regCR1.data   |= GFX_REG_CR1_TRANSFORM_EN
        | ((GFX_TILE_FILL << GFX_REG_CR1_TILEMODE_OFFSET) & GFX_REG_CR1_TILEMODE_MASK);

    regs->regCR1.data   |= GFX_REG_CR1_PERSPECTIVE_EN;

    regs->regSAFE.data  |= GFX_REG_SAFE_BOUNDARY_INTERPOLATION;

    if (dstSurface->dstSurface != NULL)
    {
        regs->regCR1.data |= GFX_REG_CR1_INTERPOLATE_EN;
        // regs->regCR1.data |= GFX_REG_CR1_ALPHABLEND_EN;
    }

    // Fire
    regs->regCMD.data = GFX_REG_CMD_ROP3;

    result = gfxHwEngineFire(driver->hwDev);

    return result;
}
