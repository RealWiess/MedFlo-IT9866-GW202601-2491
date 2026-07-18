#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "itu_cfg.h"
#include "ite/itp.h"
#include "ite/itu.h"
#include "ite/itv.h"
#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
#include "ite/itv_2way.h"
#endif
#include "gfx/gfx.h"
#include "itu_private.h"
#ifdef ITU_ENABLE_DEFER_DESTROY
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/queue.h"
#endif

#define MAX_ROTATE_BUFFER_COUNT        2
#define MAX_DEFER_DESTROY_COUNT        2048

#define SMTK_RGB565(r, g, b)           (((b) >> 3) | (((g) >> 2) << 5) | (((r) >> 3) << 11))

#define OUT_OF_RANGE(p, start, length) (((p) < (start)) || ((p) >= ((start) + (length))))

static ITURotation  m2dRotation = ITU_ROT_0;

static bool         gM2dInit    = false;
static uint32_t     gRotateBuffer[MAX_ROTATE_BUFFER_COUNT];
static uint8_t      gRotateBufIdx = 0;
static ITUSurface * gRoateSurf;

#ifdef CFG_M2D_HUD_ENABLE
static uint32_t *        tmp_dst = NULL;

extern volatile uint32_t gBlock_W;
extern volatile uint32_t gBlock_H;
extern volatile uint32_t gW_num;
extern volatile uint32_t gH_num;
extern volatile uint32_t gBlock_num;
extern volatile uint32_t gHUDAddrDataNum;

extern int gTempHudIndex;
extern uint32_t * gpHudTableData[3];

static GFXSurface * tempHudRotateSurf;
static bool         bMicroAngle = false;
static bool         gbHudOff = false;
#endif

#ifdef ITU_ENABLE_DEFER_DESTROY

typedef void (*DeferDestroyFunc)(void * pData);

typedef struct
{
    void *           pData;
    DeferDestroyFunc destroyFunc;
} DeferDestroyData;

typedef struct
{
    DeferDestroyData * pDestroyData;
    int                count;
} DestroyDataArray;

typedef struct
{
    void * pData;
    int    blitId;
} DeferDestroyEntry;

static QueueHandle_t    deferQueue                                       = NULL;
static int              gBlitId                                          = 0;

static bool             gbFreeDeferEntry                                 = false;
static int              gDeleteBlitId                                    = 0;

static DeferDestroyData gptTmpDeferDestroyArray[MAX_DEFER_DESTROY_COUNT] = {0};
static int              gTmpDeferDestroyCount                            = 0;

    #define MAX_QUEUE_ENTRY 32

static void blitIdIntrCallback (void)
{
    gbFreeDeferEntry = true;
    #if (CFG_CHIP_FAMILY == 9880)
    gDeleteBlitId = ithReadRegA(0xE0700000 + 0x98);
    #else
    gDeleteBlitId = ithReadRegA(0xB0700000 + 0x98);
    #endif
    #if 0
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    DeferDestroyEntry tEntry;

    while(xQueueReceiveFromISR(deferQueue, &tEntry,&xHigherPriorityTaskWoken))
    {
        ituFtFreeDestroyData(tEntry.pData);
        #if (CFG_CHIP_FAMILY == 9880)
        if (tEntry.blitId == ithReadRegA(0xE0700000 + 0x98))
        #else
        if (tEntry.blitId == ithReadRegA(0xB0700000 + 0x98))
        #endif
        {
            break;
        }
    }
    #endif
}

int ituAddDeferDestroyData (void * pData, void * destroyFunc)
{
    if (gTmpDeferDestroyCount < MAX_DEFER_DESTROY_COUNT)
    {
        gptTmpDeferDestroyArray[gTmpDeferDestroyCount].pData       = pData;
        gptTmpDeferDestroyArray[gTmpDeferDestroyCount].destroyFunc = (DeferDestroyFunc)destroyFunc;
        gTmpDeferDestroyCount++;
        return 1;
    }
    else
    {
        (void)printf("itu_freetype_cache.c(%d), out of destroy avaiable size.\n", __LINE__);
        if (destroyFunc && pData)
        {
            (*((DeferDestroyFunc)destroyFunc))(pData);
        }
    }
    return 0;
}

int ituFtGetDeferDestroyData (void ** pData)
{
    DestroyDataArray * ptDestroyDataArray = NULL;
    int                i                  = 0;
    if (gTmpDeferDestroyCount)
    {
        ptDestroyDataArray = malloc(sizeof(DestroyDataArray));
        if (!ptDestroyDataArray)
        {
            goto error;
        }
        ptDestroyDataArray->pDestroyData = malloc(sizeof(DeferDestroyData) * gTmpDeferDestroyCount);
        ptDestroyDataArray->count        = gTmpDeferDestroyCount;
        if (!ptDestroyDataArray->pDestroyData)
        {
            goto error;
        }

        (void)memcpy(ptDestroyDataArray->pDestroyData, gptTmpDeferDestroyArray,
               sizeof(DeferDestroyData) * gTmpDeferDestroyCount);
        *pData                = ptDestroyDataArray;
        // (void)printf("destroy count: %u\n", gTmpDeferDestroyCount);
        gTmpDeferDestroyCount = 0;
        return 1;
    }
    else
    {
        return 0;
    }
error:
    if (ptDestroyDataArray)
    {
        free(ptDestroyDataArray);
    }

    for (i = 0; i < gTmpDeferDestroyCount; i++)
    {
        (*gptTmpDeferDestroyArray[i].destroyFunc)(gptTmpDeferDestroyArray[i].pData);
    }
    gTmpDeferDestroyCount = 0;
    return 0;
}

int ituFtFreeDestroyData (void * pData)
{
    DestroyDataArray * ptFreeDataArray = pData;
    int                i               = 0;
    if (ptFreeDataArray)
    {
        for (i = 0; i < ptFreeDataArray->count; i++)
        {
            DeferDestroyData * ptFreeData = &ptFreeDataArray->pDestroyData[i];
            if (ptFreeData->pData)
            {
                if (ptFreeData->destroyFunc)
                {
                    (*ptFreeData->destroyFunc)(ptFreeData->pData);
                }
                else
                {
                    free(ptFreeData->pData);
                }
            }
        }
        if (ptFreeDataArray->pDestroyData)
        {
            free(ptFreeDataArray->pDestroyData);
        }
        free(ptFreeDataArray);
    }
    return 0;
}

#endif

void _SetClipRectRegion (ITUSurface * surf, int x, int y, int w, int h)
{
    int          left    = surf->clipping.x;
    int          top     = surf->clipping.y;
    int          right   = (surf->clipping.x + surf->clipping.width - 1);
    int          bottom  = (surf->clipping.y + surf->clipping.height - 1);
    M2dSurface * m2dSurf = (M2dSurface *)surf;

    if (right >= (x + w))
    {
        right = x + w - 1;
    }
    if (bottom >= (y + h))
    {
        bottom = y + h - 1;
    }
    if (left < x)
    {
        left = x;
    }
    if (top < y)
    {
        top = y;
    }

    gfxSurfaceSetClip(m2dSurf->m2dSurf, left, top, right, bottom);
}

static void _SWColorFill (ITUSurface * surf, int x, int y, int w, int h, ITUColor * color)
{
    if (surf->format == ITU_RGB565)
    {
        int        i, j;
        uint16_t * ptr = (uint16_t *)ituLockSurface(surf, x, y, w, h);

        if (ptr)
        {
            for (i = 0; i < h; i++)
            {
                if (surf->flags & ITU_CLIPPING)
                {
                    if (OUT_OF_RANGE(y + i, surf->clipping.y, surf->clipping.height))
                    {
                        continue;
                    }
                }

                for (j = 0; j < w; j++)
                {
                    uint16_t dc, r, g, b;

                    if (surf->flags & ITU_CLIPPING)
                    {
                        if (OUT_OF_RANGE(x + j, surf->clipping.x, surf->clipping.width))
                        {
                            continue;
                        }
                    }
                    dc = ptr[surf->width * i + j];
                    r  = ((((dc & 0xf800) >> 11) << 3) * (255 - color->alpha) + color->red * color->alpha) / 255;
                    g  = ((((dc & 0x07e0) >> 5) << 2) * (255 - color->alpha) + color->green * color->alpha) / 255;
                    b  = (((dc & 0x001f) << 3) * (255 - color->alpha) + color->blue * color->alpha) / 255;
                    ptr[surf->width * i + j] = ITH_RGB565(r, g, b);
                }
            }
        }

        ituUnlockSurface(surf);
    }
    else if (surf->format == ITU_ARGB1555)
    {
        int        i, j;
        uint16_t * ptr = (uint16_t *)ituLockSurface(surf, x, y, w, h);

        if (ptr)
        {
            for (i = 0; i < h; i++)
            {
                if (surf->flags & ITU_CLIPPING)
                {
                    if (OUT_OF_RANGE(y + i, surf->clipping.y, surf->clipping.height))
                    {
                        continue;
                    }
                }

                for (j = 0; j < w; j++)
                {
                    if (surf->flags & ITU_CLIPPING)
                    {
                        if (OUT_OF_RANGE(x + j, surf->clipping.x, surf->clipping.width))
                        {
                            continue;
                        }
                    }
                    ptr[surf->width * i + j] = ITH_ARGB1555(color->alpha, color->red, color->green, color->blue);
                }
            }
        }

        ituUnlockSurface(surf);
    }
    else if (surf->format == ITU_ARGB4444)
    {
        int        i, j;
        uint16_t * ptr = (uint16_t *)ituLockSurface(surf, x, y, w, h);

        if (ptr)
        {
            for (i = 0; i < h; i++)
            {
                if (surf->flags & ITU_CLIPPING)
                {
                    if (OUT_OF_RANGE(y + i, surf->clipping.y, surf->clipping.height))
                    {
                        continue;
                    }
                }

                for (j = 0; j < w; j++)
                {
                    if (surf->flags & ITU_CLIPPING)
                    {
                        if (OUT_OF_RANGE(x + j, surf->clipping.x, surf->clipping.width))
                        {
                            continue;
                        }
                    }
                    ptr[surf->width * i + j] = ITH_ARGB4444(color->alpha, color->red, color->green, color->blue);
                }
            }
        }

        ituUnlockSurface(surf);
    }
    else if (surf->format == ITU_ARGB8888)
    {
        int        i, j;
        uint32_t * ptr = (uint32_t *)ituLockSurface(surf, x, y, w, h);

        if (ptr)
        {
            for (i = 0; i < h; i++)
            {
                if (surf->flags & ITU_CLIPPING)
                {
                    if (OUT_OF_RANGE(y + i, surf->clipping.y, surf->clipping.height))
                    {
                        continue;
                    }
                }

                for (j = 0; j < w; j++)
                {
                    if (surf->flags & ITU_CLIPPING)
                    {
                        if (OUT_OF_RANGE(x + j, surf->clipping.x, surf->clipping.width))
                        {
                            continue;
                        }
                    }
                    ptr[surf->width * i + j] = ITH_ARGB8888(color->alpha, color->red, color->green, color->blue);
                }
            }
        }

        ituUnlockSurface(surf);
    }
}

static ITUSurface * M2dCreateSurface (int w, int h, int pitch, ITUPixelFormat format, const uint8_t * bitmap,
                                      unsigned int flags)
{
    GFXColorFormat gfxformat = GFX_COLOR_UNKNOWN;
    GFXMaskFormat  maskFormat;
    M2dSurface *   surface    = calloc(1, sizeof(M2dSurface));
    GFXSurface *   gfxSurface = NULL;

    if (!surface)
    {
        (void)printf("Out of memory: %d\n", sizeof(M2dSurface));
        return NULL;
    }

    surface->surf.width  = w;
    surface->surf.height = h;
    surface->surf.pitch  = (pitch) ? pitch : (w * ituFormat2Bpp(format) / 8);
    surface->surf.format = format;
    surface->surf.flags  = flags;
    ituSetColor(&surface->surf.fgColor, 255, 255, 255, 255);

    if (format == ITU_RGB565)
    {
        gfxformat = GFX_COLOR_RGB565;
    }
    else if (format == ITU_ARGB1555)
    {
        gfxformat = GFX_COLOR_ARGB1555;
    }
    else if (format == ITU_ARGB4444)
    {
        gfxformat = GFX_COLOR_ARGB4444;
    }
    else if (format == ITU_ARGB8888)
    {
        gfxformat = GFX_COLOR_ARGB8888;
    }
    else if (format == ITU_MASK_A8)
    {
        gfxformat  = GFX_COLOR_ARGB8888;
        maskFormat = GFX_MASK_A_8;
    }
    else
    {
        gfxformat = GFX_COLOR_RGB565;
    }

    if (bitmap)
    {
        surface->surf.addr = (uint32_t)bitmap;
    }
    else
    {
        unsigned int size  = (surface->surf.pitch + 4) * (h + 1);

        surface->surf.addr = itpVmemAlloc(size);
        if (!surface->surf.addr)
        {
            ITU_LOG_ERR("Out of VRAM: %d\n", size);
            free(surface);
            return NULL;
        }
#ifdef CFG_CPU_WB
        // The mapped memory area may be used by CPU and related cache may be dirty before.
        // Therefore, we need to invalidate the cache range to ensure no related cache flush while engine
        // writing this area simultaneously.
        ithInvalidateDCacheRange((void *)surface->surf.addr, size);
#endif
    }

    if (format == ITU_A8)
    {
        if (flags == ITU_STATIC)
        {
            gfxSurface = gfxCreateSurfaceByPointer(w, h, surface->surf.pitch, GFX_COLOR_A_8,
                                                   (uint8_t *)surface->surf.addr, h * surface->surf.pitch);
        }
        else
        {
            gfxSurface = gfxCreateSurfaceByBuffer(w, h, surface->surf.pitch, GFX_COLOR_A_8,
                                                  (uint8_t *)surface->surf.addr, h * surface->surf.pitch);
        }

        if (!gfxSurface)
        {
            ITU_LOG_ERR("gfxCreateSurfaceByBuffer fail!\n");
            free(surface);
            return NULL;
        }

        surface->m2dSurf = gfxSurface;
    }
    else if (format == ITU_MASK_A8)
    {
        gfxSurface = gfxCreateSurface((w + 3) / 4, h, (int)((w + 3) / 4) * 4, gfxformat);

        if (!gfxSurface)
        {
            ITU_LOG_ERR("gfxCreateSurface fail!\n");
            free(surface);
            return NULL;
        }

        surface->m2dSurf              = gfxSurface;
        surface->m2dSurf->format      = GFX_COLOR_A_8;
        surface->m2dSurf->maskSurface = gfxCreateMaskSurface(w, h, (int)((w + 3) / 4) * 4, maskFormat);
    }
    else
    {
        gfxSurface = gfxCreateSurfaceByPointer(w, h, surface->surf.pitch, gfxformat, (uint8_t *)surface->surf.addr,
                                               h * surface->surf.pitch);

        if (!gfxSurface)
        {
            ITU_LOG_ERR("gfxCreateSurfaceByPointer fail!\n");
            free(surface);
            return NULL;
        }

        surface->m2dSurf = gfxSurface;
    }

    return (ITUSurface *)surface;
}

static void M2dDestroySurface (ITUSurface * surf)
{
    M2dSurface * m2dSurf;

    if (surf)
    {
        m2dSurf = (M2dSurface *)surf;
    }
    else
    {
        (void)printf("M2dDestroySurface surf is not exist\n");
        return;
    }

    if (m2dSurf && m2dSurf->m2dSurf->maskSurface && m2dSurf->m2dSurf->format == GFX_COLOR_A_8)
    {
        gfxDestroyMaskSurface(m2dSurf->m2dSurf->maskSurface);
    }

    if (m2dSurf && m2dSurf->m2dSurf)
    {
        gfxDestroySurface(m2dSurf->m2dSurf);
    }
    // #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (surf && !(surf->flags & ITU_STATIC))
    {
        itpVmemFree(surf->addr);
    }
    // #endif

    if (surf)
    {
        free(surf);
    }
}

static void M2dReplaceSurface (ITUSurface * surf, int w, int h, int pitch, ITUPixelFormat format)
{
    M2dSurface *   m2dSurface = (M2dSurface *)surf;
    GFXColorFormat gfxformat  = GFX_COLOR_UNKNOWN;

    m2dSurface->surf.width    = w;
    m2dSurface->surf.height   = h;
    m2dSurface->surf.pitch    = (pitch) ? pitch : (w * ituFormat2Bpp(format) / 8);
    m2dSurface->surf.format   = format;

    if (format == ITU_RGB565)
    {
        gfxformat = GFX_COLOR_RGB565;
    }
    else if (format == ITU_ARGB1555)
    {
        gfxformat = GFX_COLOR_ARGB1555;
    }
    else if (format == ITU_ARGB4444)
    {
        gfxformat = GFX_COLOR_ARGB4444;
    }
    else if (format == ITU_ARGB8888)
    {
        gfxformat = GFX_COLOR_ARGB8888;
    }
    else
    {
        gfxformat = GFX_COLOR_RGB565;
    }

    m2dSurface->m2dSurf->width  = w;
    m2dSurface->m2dSurf->height = h;
    m2dSurface->m2dSurf->pitch  = (pitch) ? pitch : (w * ituFormat2Bpp(format) / 8);
    m2dSurface->m2dSurf->format = gfxformat;
}

static ITUMaskSurface * M2dCreateMaskSurface (int w, int h, int pitch, ITUMaskFormat format, uint8_t * buffer,
                                              uint32_t bufferLength)
{
    GFXMaskFormat    maskFormat = GFX_MASK_UNKNOWN;
    GFXMaskSurface * imgSurf    = 0;

    if (format == ITU_MASK_A_8)
    {
        maskFormat = GFX_MASK_A_8;
        ;
    }
    else if (format == ITU_MASK_A_4)
    {
        maskFormat = GFX_MASK_A_4;
    }
    else if (format == ITU_MASK_A_2)
    {
        maskFormat = GFX_MASK_A_2;
    }
    else if (format == ITU_MASK_A_1)
    {
        maskFormat = GFX_MASK_A_1;
    }

    imgSurf = gfxCreateMaskSurfaceByBuffer(w, h, pitch, maskFormat, buffer, bufferLength);

    return (ITUMaskSurface *)imgSurf;
}

static void M2dDestroyMaskSurface (ITUMaskSurface * surf)
{
    M2dSurface * m2dSurf = (M2dSurface *)surf;

    gfxDestroyMaskSurface(m2dSurf->m2dSurf->maskSurface);
}

static void M2dSetMaskSurface (ITUSurface * surface, ITUMaskSurface * maskSurface, bool enable)
{
    gfxSetMaskSurface((GFXSurface *)surface, (GFXMaskSurface *)maskSurface, enable);
}

static uint8_t * M2dLockSurface (ITUSurface * surf, int x, int y, int w, int h)
{
    uint32_t     vram_addr;
    M2dSurface * m2dSurface = (M2dSurface *)surf;
    gfxwaitEngineIdle();

    surf->lockSize = surf->pitch * h;

    switch (surf->format)
    {
        case ITU_A8:
        case ITU_MASK_A8:
        case ITU_MONO:
            vram_addr = (uint32_t)(m2dSurface->m2dSurf->imageData + surf->pitch * y + (x + 7) / 8);
            break;
        case ITU_ARGB8888:
            vram_addr = (uint32_t)(m2dSurface->m2dSurf->imageData + surf->pitch * y + x * 4);
            break;
        default:
            vram_addr = (uint32_t)(m2dSurface->m2dSurf->imageData + surf->pitch * y + x * 2);
            break;
    }

#ifndef SPEED_UP
    surf->lockAddr = (uint8_t *)ithMapVram(vram_addr, surf->lockSize, ITH_VRAM_READ | ITH_VRAM_WRITE);
#else
    surf->lockAddr       = (uint8_t *)ithMapVram(vram_addr, surf->lockSize, /*ITH_VRAM_READ |*/ ITH_VRAM_WRITE);
#endif

    ithInvalidateDCacheRange((void *)(surf->lockAddr), surf->lockSize);

    return surf->lockAddr;
}

static void M2dUnlockSurface (ITUSurface * surf)
{
#ifndef SPEED_UP
    #ifdef CFG_CPU_WB
    gfxwaitEngineIdle();
    ithUnmapVram(surf->lockAddr, surf->lockSize);
    #endif
    ithFlushDCacheRange((void *)(surf->lockAddr), surf->lockSize);
    ithFlushMemBuffer();
#endif
}

static void M2dSetRotation (ITURotation rot)
{
    ITUSurface * lcdSurf;
    M2dSurface * screenSurf;

    if (m2dRotation == rot)
    {
        return;
    }

    if (gRotateBuffer[0])
    {
        itpVmemFree(gRotateBuffer[0]);
        gRotateBuffer[0] = 0;
    }

    if (gRoateSurf && rot != ITU_ROT_0)
    {
        M2dDestroySurface(gRoateSurf);
    }

#ifdef CFG_BUILD_ITV
    itv_set_rotation(rot);
#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
    itv_set_rotation_2way(rot);
#endif    
#endif

    lcdSurf = ituGetDisplaySurface();
    assert(lcdSurf);
    screenSurf = (M2dSurface *)lcdSurf;

    switch (rot)
    {
        default:
            lcdSurf->width  = ithLcdGetWidth();
            lcdSurf->height = ithLcdGetHeight();
            lcdSurf->pitch  = ithLcdGetPitch();
            break;

        case ITU_ROT_90:
        case ITU_ROT_270:
            lcdSurf->width  = ithLcdGetHeight();
            lcdSurf->height = ithLcdGetWidth();
            lcdSurf->pitch  = ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth();
            break;
    }

    m2dRotation = rot;

    // create rotation buffer and sruface
    if (!gRotateBuffer[0] && m2dRotation != ITU_ROT_0)
    {
        uint32_t       i;
        uint32_t       size;
        uint32_t       pitch;
        uint32_t       tiling_block;
        ITUPixelFormat format;

        if (ithIsTilingModeOn() == 0)
        {
            pitch        = ithTilingPitch();
            tiling_block = 2048;
        }
        else
        {
            pitch        = lcdSurf->pitch;
            tiling_block = 16;
        }

        size             = pitch * lcdSurf->height * MAX_ROTATE_BUFFER_COUNT;

        gRotateBuffer[0] = itpVmemAlignedAlloc(tiling_block, size);

        assert(gRotateBuffer[0]);

        for (i = 1; i < MAX_ROTATE_BUFFER_COUNT; ++i)
        {
            gRotateBuffer[i] = gRotateBuffer[i - 1] + pitch * lcdSurf->height;
        }

        lcdSurf->addr = screenSurf->surf.addr = gRotateBuffer[0];

        gfxSurfaceSetSurfaceBaseAddress(screenSurf->m2dSurf, screenSurf->surf.addr);

        switch (ithLcdGetFormat())
        {
            case ITH_LCD_RGB565:
                format = ITU_RGB565;
                break;
            case ITH_LCD_ARGB1555:
                format = ITU_ARGB1555;
                break;
            case ITH_LCD_ARGB4444:
                format = ITU_ARGB4444;
                break;
            case ITH_LCD_ARGB8888:
                format = ITU_ARGB8888;
                break;
            default: // ITH_LCD_YUV
                format = ITU_MONO;
        }

        gRoateSurf = M2dCreateSurface(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), format,
                                      (uint8_t *)ithLcdGetBaseAddrA(), ITU_STATIC);

        gfxSurfaceSetWidth(screenSurf->m2dSurf, lcdSurf->width);
        gfxSurfaceSetHeight(screenSurf->m2dSurf, lcdSurf->height);
        gfxSurfaceSetPitch(screenSurf->m2dSurf, pitch);
    }
}

static void M2dDrawGlyph (ITUSurface * surf, int x, int y, ITUGlyphFormat format, const uint8_t * bitmap, int w, int h)
{
    M2dSurface *  destSurf   = (M2dSurface *)surf;
    GFXColor      color      = {0};
    GFXSurfaceSrc srcSurface = {0};
    GFXSurface    tmpSrcSurf = {0};

    color.a                  = surf->fgColor.alpha;
    color.r                  = surf->fgColor.red;
    color.g                  = surf->fgColor.green;
    color.b                  = surf->fgColor.blue;

    tmpSrcSurf.imageData     = (uint8_t *)bitmap;
    tmpSrcSurf.width         = w;
    tmpSrcSurf.height        = h;
    tmpSrcSurf.quality       = GFX_QUALITY_BETTER;

    if (format == ITU_GLYPH_1BPP)
    {
        tmpSrcSurf.pitch  = (w + 7) / 8;
        tmpSrcSurf.format = GFX_COLOR_A_1;
    }
    else if (format == ITU_GLYPH_4BPP)
    {
        tmpSrcSurf.pitch  = (w + 1) / 2;
        tmpSrcSurf.format = GFX_COLOR_A_4;
    }
    else if (format == ITU_GLYPH_8BPP)
    {
        tmpSrcSurf.pitch  = w;
        tmpSrcSurf.format = GFX_COLOR_A_8;
    }

    tmpSrcSurf.imageDataLength = tmpSrcSurf.pitch * tmpSrcSurf.height;

    srcSurface.srcSurface      = &tmpSrcSurf; // bitmap;
    srcSurface.srcX            = 0;
    srcSurface.srcY            = 0;
    srcSurface.srcWidth        = w;
    srcSurface.srcHeight       = h;

    // (void)printf("x:%d,y:%d,w:%d,h:%d\n",x,y,w,h);
    // (void)printf("a:%d,r:%d,g:%d,b:%d\n",color.a,color.r,color.g,color.b);

    if (surf->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(surf, x, y, w, h);
    }

    gfxSurfaceDrawGlyph(destSurf->m2dSurf, x, y, &srcSurface, color);

    if (surf->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }

    // if (w > 64 || h > 64)       // workaround draw large font bug
    //     gfxwaitEngineIdle();
}

static void M2dBitBlt (ITUSurface * dest, int dx, int dy, int w, int h, ITUSurface * src, int sx, int sy)
{
    GFXSurfaceSrc srcSurface = {0};
    M2dSurface *  destSurf   = (M2dSurface *)dest;
    M2dSurface *  srcSurf    = (M2dSurface *)src;

    int           width, height;
    int           destX, destY;
    int           srcX, srcY;

    width  = (w > src->width) ? src->width : w;
    height = (h > src->height) ? src->height : h;

    if (dx < 0)
    {
        srcX  = sx - dx;
        destX = 0;
    }
    else
    {
        srcX  = sx;
        destX = dx;
    }

    if (dy < 0)
    {
        srcY  = sy - dy;
        destY = 0;
    }
    else
    {
        srcY  = sy;
        destY = dy;
    }

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, destX, destY, width, height);
    }

    srcSurface.srcSurface = srcSurf->m2dSurf;
    srcSurface.srcX       = srcX;
    srcSurface.srcY       = srcY;
    srcSurface.srcWidth   = width;
    srcSurface.srcHeight  = height;

    if (src->flags & ITU_DITHER)
    {
        srcSurface.srcSurface->flags = 1;
    }
    else
    {
        srcSurface.srcSurface->flags = 0;
    }

    // if ((dest->flags & 0xC0000000) == 0xC0000000)
    //{
    //     _SWBitBlt(dest, dx, dy, w, h, src, sx, sy);
    // }
    //  workaround hw bitblt with alpha bug
    (void)memcpy(&destSurf->m2dSurf->fgColor, &destSurf->surf.fgColor, sizeof(GFXColor));
    if ((dest->flags & 0x80000000) == 0x80000000)
    {
        gfxSurfaceBitblt(destSurf->m2dSurf, destX, destY, &srcSurface);
        // dest->flags |= 0x40000000;
    }
    else
    {
        gfxSurfaceAlphaBlendEx(destSurf->m2dSurf, destX, destY, &srcSurface, false, 0xFF);
    }

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dStretchBlt (ITUSurface * dest, int dx, int dy, int dw, int dh, ITUSurface * src, int sx, int sy, int sw,
                           int sh)
{
    M2dSurface *  destSurf   = (M2dSurface *)dest;
    M2dSurface *  srcSurf    = (M2dSurface *)src;
    GFXSurfaceDst dstSurface = {0};
    GFXSurfaceSrc srcSurface = {0};

    dstSurface.dstSurface    = destSurf->m2dSurf;
    dstSurface.dstX          = dx;
    dstSurface.dstY          = dy;
    dstSurface.dstWidth      = dw;
    dstSurface.dstHeight     = dh;

    srcSurface.srcSurface    = srcSurf->m2dSurf;
    srcSurface.srcX          = sx;
    srcSurface.srcY          = sy;
    srcSurface.srcWidth      = sw;
    srcSurface.srcHeight     = sh;

    (void)memcpy(&destSurf->m2dSurf->fgColor, &destSurf->surf.fgColor, sizeof(GFXColor));
    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dw, dh);
    }

    gfxSurfaceStrectch(&dstSurface, &srcSurface);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dAlphaBlend (ITUSurface * dest, int dx, int dy, int w, int h, ITUSurface * src, int sx, int sy,
                           uint8_t alpha)
{
    M2dSurface *  destSurf   = (M2dSurface *)dest;
    M2dSurface *  srcSurf    = (M2dSurface *)src;
    GFXSurfaceSrc srcSurface = {0};

    srcSurface.srcSurface    = srcSurf->m2dSurf;
    srcSurface.srcX          = sx;
    srcSurface.srcY          = sy;
    srcSurface.srcWidth      = w;
    srcSurface.srcHeight     = h;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, w, h);
    }

    if (src->flags & ITU_DITHER)
    {
        srcSurface.srcSurface->flags = 1;
    }
    else
    {
        srcSurface.srcSurface->flags = 0;
    }

    (void)memcpy(&destSurf->m2dSurf->fgColor, &destSurf->surf.fgColor, sizeof(GFXColor));
    if (alpha != 0xFF)
    {
        gfxSurfaceAlphaBlendEx(destSurf->m2dSurf, dx, dy, &srcSurface, true, alpha);
    }
    else
    {
        gfxSurfaceAlphaBlendEx(destSurf->m2dSurf, dx, dy, &srcSurface, false, 0xFF);
    }

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dColorFill (ITUSurface * surf, int x, int y, int w, int h, ITUColor * color)
{
    M2dSurface *  destSurf   = (M2dSurface *)surf;
    GFXColor      fillColor  = {0};
    GFXSurfaceDst dstSurface = {0};

    if (w == 1 && h == 1)
    {
        _SWColorFill(surf, x, y, w, h, color);
    }
    else
    {
        fillColor.a           = color->alpha;
        fillColor.r           = color->red;
        fillColor.g           = color->green;
        fillColor.b           = color->blue;

        dstSurface.dstSurface = destSurf->m2dSurf;
        dstSurface.dstX       = x;
        dstSurface.dstY       = y;
        dstSurface.dstWidth   = w;
        dstSurface.dstHeight  = h;

        if (surf->flags & ITU_CLIPPING)
        {
            _SetClipRectRegion(surf, x, y, w, h);
        }

        // (void)printf("surf->format:%d\n",surf->format);
        // (void)printf("surface->width:%d,surface->height:%d,x:%d,y:%d,w:%d,h:%d\n",surface.width,surface.height,x,y,w,h);
        gfxSurfaceFillColor(&dstSurface, fillColor);

        if (surf->flags & ITU_CLIPPING)
        {
            gfxSurfaceUnSetClip(destSurf->m2dSurf);
        }
    }
}

static void M2dColorFillBlend (ITUSurface * surf, int x, int y, int w, int h, ITUColor * color, bool enableAlpha,
                               bool enableConstantAlpha, uint8_t constantAlphaValue)
{
    M2dSurface *  destSurf         = (M2dSurface *)surf;
    GFXColor      fillColor        = {0};
    GFXAlphaBlend alphaBlend       = {0};
    GFXSurfaceDst dstSurface       = {0};

    fillColor.a                    = color->alpha;
    fillColor.r                    = color->red;
    fillColor.g                    = color->green;
    fillColor.b                    = color->blue;

    dstSurface.dstSurface          = destSurf->m2dSurf;
    dstSurface.dstX                = x;
    dstSurface.dstY                = y;
    dstSurface.dstWidth            = w;
    dstSurface.dstHeight           = h;

    alphaBlend.enableAlpha         = enableAlpha;
    alphaBlend.enableConstantAlpha = enableConstantAlpha;
    alphaBlend.constantAlphaValue  = constantAlphaValue;

    if (surf->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(surf, x, y, w, h);
    }

    gfxSurfaceFillColorWithBlend(&dstSurface, fillColor, &alphaBlend);

    if (surf->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dRotate (ITUSurface * dest, int dx, int dy, ITUSurface * src, int cx, int cy, float angle, float scaleX,
                       float scaleY)
{
    M2dSurface *  destSurf         = (M2dSurface *)dest;
    M2dSurface *  srcSurf          = (M2dSurface *)src;
    GFXSurfaceDst dstSurface       = {0};
    GFXSurfaceSrc srcSurface       = {0};
    GFXAlphaBlend alphaBlend       = {0};

    dstSurface.dstSurface          = destSurf->m2dSurf;
    dstSurface.dstX                = dx;
    dstSurface.dstY                = dy;
    dstSurface.dstWidth            = dest->width;
    dstSurface.dstHeight           = dest->height;

    srcSurface.srcSurface          = srcSurf->m2dSurf;
    srcSurface.srcX                = 0;
    srcSurface.srcY                = 0;
    srcSurface.srcWidth            = src->width;
    srcSurface.srcHeight           = src->height;

    alphaBlend.enableAlpha         = true;
    alphaBlend.enableConstantAlpha = false;
    alphaBlend.constantAlphaValue  = 0;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dest->clipping.x, dest->clipping.y, dest->width, dest->height);
    }

    gfxSurfaceTransform(&dstSurface, &srcSurface, cx, cy, scaleX, scaleY, angle, ITU_TILE_FILL, GFX_ROP3_SRCCOPY,
                        &alphaBlend);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dTransform (ITUSurface * dest, int dx, int dy, int dw, int dh, ITUSurface * src, int sx, int sy, int sw,
                          int sh, int cx, int cy, float scaleWidth, float scaleHeight, float angle,
                          ITUTileMode tilemode, bool enableAlpha, bool enableConstantAlpha, uint8_t constantAlphaValue)
{
    M2dSurface *  destSurf         = (M2dSurface *)dest;
    M2dSurface *  srcSurf          = (M2dSurface *)src;
    GFXSurfaceDst dstSurface       = {0};
    GFXSurfaceSrc srcSurface       = {0};
    GFXAlphaBlend alphaBlend       = {0};

    dstSurface.dstSurface          = destSurf->m2dSurf;
    dstSurface.dstX                = dx;
    dstSurface.dstY                = dy;
    dstSurface.dstWidth            = dw;
    dstSurface.dstHeight           = dh;

    srcSurface.srcSurface          = srcSurf->m2dSurf;
    srcSurface.srcX                = sx;
    srcSurface.srcY                = sy;
    srcSurface.srcWidth            = sw;
    srcSurface.srcHeight           = sh;

    alphaBlend.enableAlpha         = enableAlpha;
    alphaBlend.enableConstantAlpha = enableConstantAlpha;
    alphaBlend.constantAlphaValue  = constantAlphaValue;

    if (dest->clipping.x + dest->clipping.width >= dest->width || ((dest->flags & ITU_CLIPPING) && angle == 0))
    {
        _SetClipRectRegion(dest, dx, dy, dw, dh);
    }

    gfxSurfaceTransform(&dstSurface, &srcSurface, cx, cy, scaleWidth, scaleHeight, angle, tilemode, GFX_ROP3_SRCCOPY,
                        &alphaBlend);

    if (dest->clipping.x + dest->clipping.width >= dest->width || ((dest->flags & ITU_CLIPPING) && angle == 0))
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

/**
 * Fill a rectangle in the specified gradient color.
 *
 * @param destDisplay       handle to destination display context.
 * @param destX             x-coordinate of destination upper-left corner.
 * @param destY             y-coordinate of destination upper-left corner.
 * @param destWidth         width of destination rectangle.
 * @param destHeight        height of destination rectangle.
 * @param initColor         the initial color(RGB565)
 * @param endColor          the end color(RGB565)
 * @param direction         the direction of gradient fill
 *
 * @return MMP_RESULT_SUCCESS if succeed, error codes of MMP_RESULT_ERROR
 *         otherwise.
 */

static void M2dGradientFill (ITUSurface * surf, int x, int y, int w, int h, ITUColor * initcolor, ITUColor * endcolor,
                             ITUGradientMode mode)
{
    M2dSurface *  destSurf   = (M2dSurface *)surf;
    GFXColor      color      = {0};
    GFXColor      color_end  = {0};
    GFXSurfaceDst dstSurface = {0};

    color.a                  = initcolor->alpha;
    color.r                  = initcolor->red;
    color.g                  = initcolor->green;
    color.b                  = initcolor->blue;

    color_end.a              = endcolor->alpha;
    color_end.r              = endcolor->red;
    color_end.g              = endcolor->green;
    color_end.b              = endcolor->blue;

    dstSurface.dstSurface    = destSurf->m2dSurf;
    dstSurface.dstX          = x;
    dstSurface.dstY          = y;
    dstSurface.dstWidth      = w;
    dstSurface.dstHeight     = h;

    if (surf->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(surf, x, y, w, h);
    }

    if (mode == ITU_GF_NONE)
    {
        gfxSurfaceFillColor(&dstSurface, color);
    }
    else
    {
        GFXGradDir gfxMode = GFX_GRAD_UNKNOWN;

        if (mode == ITU_GF_HORIZONTAL)
        {
            gfxMode = GFX_GRAD_H;
        }
        else if (mode == ITU_GF_VERTICAL)
        {
            gfxMode = GFX_GRAD_V;
        }
        else if (mode == ITU_GF_FORWARD_DIAGONAL || mode == ITU_GF_BACKWARD_DIAGONAL)
        {
            gfxMode = GFX_GRAD_BOTH;
        }

        if (destSurf->m2dSurf->format == GFX_COLOR_A_8)
        {
            GFXSurface * surface = dstSurface.dstSurface;

            surface->format      = GFX_COLOR_ARGB8888;
            dstSurface.dstWidth = surface->width = (w + 3) / 4;
            gfxSurfaceGradientFill(&dstSurface, color, color_end, gfxMode);
            if (surface->maskSurface != NULL)
            {
                gfxDestroyMaskSurface(surface->maskSurface);
            }
            surface->maskSurface = (GFXMaskSurface*)surface;

            surface->maskSurface->width           = w;
            surface->maskSurface->height          = surface->height;
            surface->maskSurface->pitch           = surface->pitch;
            surface->maskSurface->format          = GFX_MASK_A_8;
            surface->maskSurface->imageData       = surface->imageData;
            surface->maskSurface->imageDataLength = surface->imageDataLength;
            surface->maskSurface->imageDataOwner  = surface->imageDataOwner;
        }
        else
        {
            gfxSurfaceGradientFill(&dstSurface, color, color_end, gfxMode);
        }
    }

    if (surf->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dGradientFillBlend (ITUSurface * surf, int x, int y, int w, int h, ITUColor * initcolor,
                                  ITUColor * endcolor, ITUGradientMode mode, bool enableAlpha, bool enableConstantAlpha,
                                  uint8_t constantAlphaValue)
{
    M2dSurface *  destSurf   = (M2dSurface *)surf;
    GFXColor      color      = {0};
    GFXColor      color_end  = {0};
    GFXAlphaBlend alphaBlend = {0};
    GFXSurfaceDst dstSurface = {0};

    color.a                  = initcolor->alpha;
    color.r                  = initcolor->red;
    color.g                  = initcolor->green;
    color.b                  = initcolor->blue;

    color_end.a              = endcolor->alpha;
    color_end.r              = endcolor->red;
    color_end.g              = endcolor->green;
    color_end.b              = endcolor->blue;

    dstSurface.dstSurface    = destSurf->m2dSurf;
    dstSurface.dstX          = x;
    dstSurface.dstY          = y;
    dstSurface.dstWidth      = w;
    dstSurface.dstHeight     = h;

    if (surf->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(surf, x, y, w, h);
    }

    if (mode == ITU_GF_NONE)
    {
        gfxSurfaceFillColor(&dstSurface, color);
    }
    else
    {
        GFXGradDir gfxMode = GFX_GRAD_UNKNOWN;

        if (mode == ITU_GF_HORIZONTAL)
        {
            gfxMode = GFX_GRAD_H;
        }
        else if (mode == ITU_GF_VERTICAL)
        {
            gfxMode = GFX_GRAD_V;
        }
        else if (mode == ITU_GF_FORWARD_DIAGONAL || mode == ITU_GF_BACKWARD_DIAGONAL)
        {
            gfxMode = GFX_GRAD_BOTH;
        }

        alphaBlend.enableAlpha         = enableAlpha;
        alphaBlend.enableConstantAlpha = enableConstantAlpha;
        alphaBlend.constantAlphaValue  = constantAlphaValue;

        gfxSurfaceGradientFillWithBlend(&dstSurface, color, color_end, gfxMode, &alphaBlend);
    }

    if (surf->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

#ifdef CFG_M2D_HUD_ENABLE
void M2dUpdateHudAddr (HudSurface * ptHudSurf)
{
    #ifndef CFG_HUD_AUTO_GENERATE
    // extern uint32_t gpHudTableData1[3][gHUDAddrDataNum];
    // extern uint32_t gpHudTableData2[3][gHUDAddrDataNum];
    // extern uint32_t gpHudTableData3[3][gHUDAddrDataNum];
    extern uint32_t * HUDAddrData;
    #endif

    int i, j;
    int hudDataIndex = 0;
    #ifdef HUD_INVERSE
    int n = gBlock_num - 2; // 5998;
    #else
    int n         = 0;
    #endif

    ituHudOn();

    #ifndef CFG_HUD_AUTO_GENERATE
    {
        (void)memcpy(gpHudTableData[0], HUDAddrData, gHUDAddrDataNum * sizeof(uint32_t));
        (void)memcpy(gpHudTableData[1], HUDAddrData, gHUDAddrDataNum * sizeof(uint32_t));
        (void)memcpy(gpHudTableData[2], HUDAddrData, gHUDAddrDataNum * sizeof(uint32_t));
    }
    #endif

    (void)printf("gpHudTableData[0][30]:0x%x, gpHudTableData[0][31]:0x%x\n", gpHudTableData[0][30], gpHudTableData[0][31]);

    uint32_t srcAddr = 0;
    uint32_t dstAddr = 0;

    gpHudTableData[0][7] = gpHudTableData[1][7] = gpHudTableData[2][7] = ((gBlock_W * gW_num) + gTempHudIndex) * 2; // source pitch

    for (hudDataIndex = 0; hudDataIndex < 3; hudDataIndex++)
    {
    #ifndef HUD_ADVANCE_ROTATE_90
        if (1)//(gfxGetHUDAngle() != 0.0)
            srcAddr = (uint32_t)tempHudRotateSurf->imageData + (((gBlock_W * gW_num) + gTempHudIndex) * 2 * gTempHudIndex);
        else
            srcAddr = (uint32_t)ptHudSurf->pUiBuffer[hudDataIndex];
    #else
        srcAddr = (uint32_t)ptHudSurf->pUiBuffer[hudDataIndex];
    #endif

        if (hudDataIndex == 0)
        {
            dstAddr = ithLcdGetBaseAddrA();
        }
        else if (hudDataIndex == 1)
        {
            dstAddr = ithLcdGetBaseAddrB();
        }
        else
        {
            dstAddr = ithLcdGetBaseAddrC();
        }


    #ifdef HUD_INVERSE
        n = gBlock_num - 2;
    #else
        n       = 0;
    #endif

        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
            {
    #ifdef HUD_INVERSE
                if (i == gW_num - 1 && j == gH_num - 1)
    #else
                if (i == 0 && j == 0)
    #endif
                {
                    gpHudTableData[hudDataIndex][1] = srcAddr + (((gBlock_W * gW_num) + gTempHudIndex) * 2 * gBlock_H * j) + (gBlock_W * 2 * i) + (gTempHudIndex * 2);
                }
                else
                {
                    gpHudTableData[hudDataIndex][65 + 28 * n] = srcAddr + (((gBlock_W * gW_num) + gTempHudIndex) * 2 * gBlock_H * j) + (gBlock_W * 2 * i) + (gTempHudIndex * 2);

    #ifdef HUD_INVERSE
                    n--;
    #else
                    n++;
    #endif
                }
            }
        }

    #ifdef HUD_INVERSE
        n = gBlock_num - 2;
    #else
        n = 0;
    #endif
        for (i = 0; i < gW_num; i++)
        {
            for (j = 0; j < gH_num; j++)
            {
    #ifdef HUD_INVERSE
                if (i == gW_num - 1 && j == gH_num - 1)
    #else
                if (i == 0 && j == 0)
    #endif
                {
                    gpHudTableData[hudDataIndex][17] = dstAddr;
                }
                else
                {
                    gpHudTableData[hudDataIndex][67 + 28 * n] = dstAddr;

    #ifdef HUD_INVERSE
                    n--;
    #else
                    n++;
    #endif
                }
            }
        }
    }
}
#endif

static void M2dHWFlip (ITUSurface * surf)
{
    ITUSurface * lcdSurf;
#ifdef CFG_M2D_HUD_ENABLE
    HudSurface * srcSurf = (HudSurface *)surf;
#else
    M2dSurface * srcSurf = (M2dSurface *)surf;
#endif

    lcdSurf = ituGetDisplaySurface();

#if (CFG_CHIP_FAMILY == 9880)
    if (m2dRotation != ITU_ROT_0)
    {
        M2dSurface * m2dtempSurf;
        float        angle = 0.0;
        int          dX    = 0;
        int          dY    = 0;
        int          cX    = 0;
        int          cY    = 0;

        switch (m2dRotation)
        {
            case ITU_ROT_90:
                angle = 90.0f;
                dX    = ithLcdGetWidth();
                dY    = 0;
                cX    = 0;
                cY    = 0;
                break;
            case ITU_ROT_180:
                angle = 180.0f;
                dX    = ithLcdGetWidth();
                dY    = ithLcdGetHeight();
                cX    = 0;
                cY    = 0;
                break;
            case ITU_ROT_270:
                angle = 270.0f;
                dX    = 0;
                dY    = ithLcdGetHeight();
                cX    = 0;
                cY    = 0;
                break;
            default:
                angle = 0;
                break;
        }

        m2dtempSurf = (M2dSurface *)gRoateSurf;

        if (gfxGetDispSurfIndex() == 0)
        {
            gfxSurfaceSetSurfaceBaseAddress(m2dtempSurf->m2dSurf, ithLcdGetBaseAddrA());
        }
        else if (gfxGetDispSurfIndex() == 1)
        {
            gfxSurfaceSetSurfaceBaseAddress(m2dtempSurf->m2dSurf, ithLcdGetBaseAddrB());
        }
        else
        {
            gfxSurfaceSetSurfaceBaseAddress(m2dtempSurf->m2dSurf, ithLcdGetBaseAddrC());
        }

        gfxSurfaceSetQuality(m2dtempSurf->m2dSurf, GFX_QUALITY_FASTER);

        M2dRotate((ITUSurface *)m2dtempSurf, dX, dY, (ITUSurface *)srcSurf, cX, cY, angle, 1.0f, 1.0f);

        gfxwaitEngineIdle();
    #ifdef ITU_ENABLE_DEFER_DESTROY
        if (gbFreeDeferEntry)
        {
            ithEnterCritical();
            gbFreeDeferEntry = false;
            ithExitCritical();

            DeferDestroyEntry tEntry       = {0};
            uint32_t          deleteBlitId = gDeleteBlitId;
            while (xQueueReceive(deferQueue, &tEntry, 0))
            {
                ituFtFreeDestroyData(tEntry.pData);
                if (tEntry.blitId == deleteBlitId)
                {
                    break;
                }
            }
        }

        DeferDestroyEntry tDestryEntry = {0};
        if (ituFtGetDeferDestroyData(&tDestryEntry.pData))
        {
            ++gBlitId;
            tDestryEntry.blitId = gBlitId;
            xQueueSend(deferQueue, &tDestryEntry, 0);
            gfxFlipWitBlitId(0, gBlitId);
        }
        else
        {
            gfxFlip();
        }
    #else
        gfxFlip();
    #endif

        gRotateBufIdx = (gRotateBufIdx + 1) % MAX_ROTATE_BUFFER_COUNT;

        if (lcdSurf)
        {
            lcdSurf->addr = gRotateBuffer[gRotateBufIdx];

            gfxSurfaceSetSurfaceBaseAddress(srcSurf->m2dSurf, lcdSurf->addr);
        }
    }
    else
#endif
    {
#ifdef CFG_M2D_HUD_ENABLE

        if (ituHudIsOff() == false)
        {
            GFXSurfaceSrc src        = {0};
            GFXSurfaceDst surface    = {0};
            GFXColor      fillColor  = {255, 0, 0, 0}; // a, r, g, b
            GFXAlphaBlend alphaBlend = {0};

    #ifndef HUD_ADVANCE_ROTATE_90
            if (1)//(gfxGetHUDAngle() != 0.0)
            {
                if (bMicroAngle == false)
                {
                    M2dUpdateHudAddr(srcSurf);
                    bMicroAngle = true;                    
                }
                surface.dstSurface = tempHudRotateSurf;
                surface.dstX       = 0;
                surface.dstY       = 0;
                surface.dstWidth   = ithLcdGetWidth() + gTempHudIndex;
                surface.dstHeight  = ithLcdGetHeight() + gTempHudIndex * 2;
                gfxSurfaceFillColor(&surface, fillColor);

                surface.dstSurface             = tempHudRotateSurf;
                surface.dstX                   = gTempHudIndex;
                surface.dstY                   = gTempHudIndex;
                surface.dstWidth               = ithLcdGetWidth() + gTempHudIndex;
                surface.dstHeight              = ithLcdGetHeight() + gTempHudIndex * 2;

                src.srcSurface                 = srcSurf->m2dSurf;
                src.srcX                       = 0;
                src.srcY                       = 0;
                src.srcWidth                   = ithLcdGetWidth();
                src.srcHeight                  = ithLcdGetHeight();

                alphaBlend.enableAlpha         = true;
                alphaBlend.enableConstantAlpha = false;
                alphaBlend.constantAlphaValue  = 0;

                // (void)printf("gfxGetHUDAngle():%f\n", gfxGetHUDAngle());
                if (gfxGetHUDAngle() != 0.0)
                {
                    gfxSurfaceSetClip(tempHudRotateSurf, 0, 0, ithLcdGetWidth() - 1 + gTempHudIndex,
                        ithLcdGetHeight() - 1 + gTempHudIndex * 2);
                    gfxSurfaceTransform(&surface, &src, ithLcdGetWidth() / 2, ithLcdGetHeight() / 2, 1.0, 1.0,
                        gfxGetHUDAngle(), ITU_TILE_FILL, GFX_ROP3_SRCCOPY, &alphaBlend);
                    gfxSurfaceUnSetClip(tempHudRotateSurf);
                }
                else
                {
                    gfxSurfaceBitblt(tempHudRotateSurf, gTempHudIndex, gTempHudIndex, &src);
                }

            }
            else
            {
                if (bMicroAngle == true)
                {
                    M2dUpdateHudAddr(srcSurf);
                    bMicroAngle = false;
                }
            }
    #endif
            gfxSelectHudTable((uint32_t)srcSurf->m2dSurf->imageData, (uint32_t)srcSurf->lcdDstSurf->imageData);

            //#ifndef HUD_ADVANCE_ROTATE_90
            //        // #ifndef HUD_ALLBLOCK_DOWARP
            //        if (1)//(gfxGetHUDAngle() != 0.0)
            //            mmpIspHUDScale((uint32_t)tmp_dst, (uint32_t)tempHudRotateSurf->imageData);
            //        else
            //            mmpIspHUDScale((uint32_t)tmp_dst, (uint32_t)srcSurf->m2dSurf->imageData);
            //        // #endif
            //#else
            //        mmpIspHUDScale((uint32_t)tmp_dst, (uint32_t)srcSurf->m2dSurf->imageData);
            //#endif
#ifdef HUD_ALLBLOCK_DOWARP
            gfxwaitEngineIdle();
#endif

            surface.dstSurface = srcSurf->lcdDstSurf;
            surface.dstX       = 0;
            surface.dstY       = 0;
            surface.dstWidth   = ithLcdGetWidth();
            surface.dstHeight  = ithLcdGetHeight();
            gfxSurfaceFillColor(&surface, fillColor);       

            gfxSkipBgFast((uint16_t *)tmp_dst);

        }
#endif

#ifdef ITU_ENABLE_DEFER_DESTROY
        if (gbFreeDeferEntry)
        {
            ithEnterCritical();
            gbFreeDeferEntry = false;
            ithExitCritical();

            DeferDestroyEntry tEntry       = {0};
            uint32_t          deleteBlitId = gDeleteBlitId;
            while (xQueueReceive(deferQueue, &tEntry, 0))
            {
                ituFtFreeDestroyData(tEntry.pData);
                if (tEntry.blitId == deleteBlitId)
                {
                    break;
                }
            }
        }

        DeferDestroyEntry tDestryEntry = {0};
        if (ituFtGetDeferDestroyData(&tDestryEntry.pData))
        {
            ++gBlitId;
            tDestryEntry.blitId = gBlitId;
            xQueueSend(deferQueue, &tDestryEntry, 0);
            gfxFlipWitBlitId(0, gBlitId);
        }
        else
        {
            gfxFlip();
        }
#else
        gfxFlip();
#endif

#ifdef CFG_M2D_HUD_ENABLE
        if (ituHudIsOff())
#endif
        {
            if (lcdSurf)
            {
                if (gfxGetDispSurfIndex() == 0)
                {
                    lcdSurf->addr = ithLcdGetBaseAddrA();
                }
                else if (gfxGetDispSurfIndex() == 1)
                {
                    lcdSurf->addr = ithLcdGetBaseAddrB();
                }
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                else if (gfxGetDispSurfIndex() == 2)
                {
                    lcdSurf->addr = ithLcdGetBaseAddrC();
                }
#endif
                gfxSurfaceSetSurfaceBaseAddress(srcSurf->m2dSurf, lcdSurf->addr);
            }
        }
#ifdef CFG_M2D_HUD_ENABLE
        else
        {
            if (gfxGetDispSurfIndex() == 0)
            {
                srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[0]; 
                srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrA();
            }
            else if (gfxGetDispSurfIndex() == 1)
            {
                srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[1];
                srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrB();
            }
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
            else if (gfxGetDispSurfIndex() == 2)
            {
                srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[2]; 
                srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrC();
            }
    #endif
        }
#endif
    }

#ifdef CFG_LCD_PQ_TUNING
    itv_update_vidSurf_anchor();
#endif
}

static void M2dProjection (ITUSurface * dest, int dx, int dy, int dw, int dh, ITUSurface * src, int sx, int sy, int sw,
                           int sh, float scaleWidth, float scaleHeight, float degree, float FOV, float pivotX,
                           bool enableAlpha, bool enableConstantAlpha, uint8_t constantAlphaValue)
{
    GFXSurfaceDst dstSurface       = {0};
    GFXSurfaceSrc srcSurface       = {0};
    GFXAlphaBlend alphaBlend       = {0};

    M2dSurface *  destSurf         = (M2dSurface *)dest;
    M2dSurface *  srcSurf          = (M2dSurface *)src;

    dstSurface.dstSurface          = destSurf->m2dSurf;
    dstSurface.dstX                = dx;
    dstSurface.dstY                = dy;
    dstSurface.dstWidth            = dw;
    dstSurface.dstHeight           = dh;

    srcSurface.srcSurface          = srcSurf->m2dSurf;
    srcSurface.srcX                = sx;
    srcSurface.srcY                = sy;
    srcSurface.srcWidth            = sw;
    srcSurface.srcHeight           = sh;

    alphaBlend.enableAlpha         = enableAlpha;
    alphaBlend.enableConstantAlpha = enableConstantAlpha;
    alphaBlend.constantAlphaValue  = constantAlphaValue;

    gfxSurfaceProjection(&dstSurface, &srcSurface, scaleWidth, scaleHeight, degree, FOV, pivotX, &alphaBlend);
}

static void M2dDrawLine (ITUSurface * surf, int32_t fromX, int32_t fromY, int32_t toX, int32_t toY,
                         ITUColor * lineColor, int32_t lineWidth)
{
    int32_t dx1 = abs(fromX - toX);
    int32_t dy1 = abs(fromY - toY);
    float   d   = (dx1 + dy1) == 0 ? 1.0f : sqrtf((float)dx1 * dx1 + (float)dy1 * dy1);
    int     p2  = (int)(d * (lineWidth + 1) / 2);

    // (void)printf("p2:%d d:%f\n", p2, d);

    if (p2 >= 7500) // p2 >= 8192
    {
        M2dSurface *  destSurf   = (M2dSurface *)surf;
        GFXColor      fillColor  = {0};
        GFXAlphaBlend alphaBlend = {0};
        GFXSurfaceDst dstSurface = {0};

        fillColor.a              = lineColor->alpha;
        fillColor.r              = lineColor->red;
        fillColor.g              = lineColor->green;
        fillColor.b              = lineColor->blue;

        int   dx = abs(toX - fromX), sx = fromX < toX ? 1 : -1;
        int   dy = abs(toY - fromY), sy = fromY < toY ? 1 : -1;
        int   err = dx - dy, e2, x2, y2; /* error value e_xy */
        float wd  = (float)lineWidth;
        float ed  = dx + dy == 0 ? 1.0f : sqrtf((float)dx * dx + (float)dy * dy);
        float alpha;

        for (wd = (wd + 1) / 2;;)
        { /* pixel loop */
            alpha                          = 255 - MAX(0, 255 * (abs(err - dx + dy) / ed - wd + 1));

            dstSurface.dstSurface          = destSurf->m2dSurf;
            dstSurface.dstX                = fromX;
            dstSurface.dstY                = fromY;
            dstSurface.dstWidth            = 1;
            dstSurface.dstHeight           = 1;

            alphaBlend.enableAlpha         = true;
            alphaBlend.enableConstantAlpha = true;
            alphaBlend.constantAlphaValue  = (uint8_t)alpha;

            gfxSurfaceFillColorWithBlend(&dstSurface, fillColor, &alphaBlend);

            e2 = err;
            x2 = fromX;
            if (2 * e2 >= -dx)
            { /* x step */
                for (e2 += dy, y2 = fromY; e2 < ed * wd && (toY != y2 || dx > dy); e2 += dx)
                {
                    alpha                          = 255 - MAX(0, 255 * (abs(e2) / ed - wd + 1));

                    dstSurface.dstSurface          = destSurf->m2dSurf;
                    dstSurface.dstX                = fromX;
                    dstSurface.dstY                = y2 += sy;
                    dstSurface.dstWidth            = 1;
                    dstSurface.dstHeight           = 1;

                    alphaBlend.enableAlpha         = true;
                    alphaBlend.enableConstantAlpha = true;
                    alphaBlend.constantAlphaValue  = (uint8_t)alpha;

                    gfxSurfaceFillColorWithBlend(&dstSurface, fillColor, &alphaBlend);
                }

                if (fromX == toX)
                {
                    break;
                }

                e2 = err;
                err -= dy;
                fromX += sx;
            }
            if (2 * e2 <= dy)
            { /* y step */
                for (e2 = dx - e2; e2 < ed * wd && (toX != x2 || dx < dy); e2 += dy)
                {
                    alpha                          = 255 - MAX(0, 255 * (abs(e2) / ed - wd + 1));

                    dstSurface.dstSurface          = destSurf->m2dSurf;
                    dstSurface.dstX                = x2 += sx;
                    dstSurface.dstY                = fromY;
                    dstSurface.dstWidth            = 1;
                    dstSurface.dstHeight           = 1;

                    alphaBlend.enableAlpha         = true;
                    alphaBlend.enableConstantAlpha = true;
                    alphaBlend.constantAlphaValue  = (uint8_t)alpha;

                    gfxSurfaceFillColorWithBlend(&dstSurface, fillColor, &alphaBlend);
                }

                if (fromY == toY)
                {
                    break;
                }

                err += dx;
                fromY += sy;
            }
        }
    }
    else
    {
        M2dSurface * destSurf = (M2dSurface *)surf;

        gfxSurfaceDrawLine(destSurf->m2dSurf, fromX, fromY, toX, toY, (GFXColor *)lineColor, lineWidth);
    }
}

static void M2dDrawCurve (ITUSurface * surf, ITUPoint * point1, ITUPoint * point2, ITUPoint * point3, ITUPoint * point4,
                          ITUColor * lineColor, int32_t lineWidth)
{
    M2dSurface * destSurf = (M2dSurface *)surf;

    gfxSurfaceDrawCurve(destSurf->m2dSurf, (GFXCoordinates *)point1, (GFXCoordinates *)point2, (GFXCoordinates *)point3,
                        (GFXCoordinates *)point4, (GFXColor *)lineColor, lineWidth);
}

// static ITUSurface* M2dGetDisplaySurface(void)
//{
//     return (ITUSurface*)gfxGetDisplaySurface();
// }

static void M2dFlipSync (int * points, int count)
{
    GFXSurface *  currDisplaySurf = NULL;
    GFXSurface *  prevDisplaySurf = NULL;
    GFXSurfaceSrc Surf            = {0};
    int           i;

    prevDisplaySurf = gfxGetBackSurface();
    currDisplaySurf = gfxGetDisplaySurface();

    Surf.srcSurface = prevDisplaySurf;

    if (points == NULL || count == 0)
    {
        Surf.srcX      = 0;
        Surf.srcY      = 0;
        Surf.srcWidth  = ithLcdGetWidth();
        Surf.srcHeight = ithLcdGetHeight();
        gfxSurfaceBitblt(currDisplaySurf, 0, 0, &Surf);
    }
    else
    {
        for (i = 0; i < count; ++i)
        {
            int * rect     = &points[i * 4];
            Surf.srcX      = rect[0];
            Surf.srcY      = rect[1];
            Surf.srcWidth  = rect[2];
            Surf.srcHeight = rect[3];
            gfxSurfaceBitblt(currDisplaySurf, rect[0], rect[1], &Surf);
        }
    }
}

static void M2dTransformBlt (ITUSurface * dest, int dx, int dy, ITUSurface * src, int sx, int sy, int sw, int sh,
                             int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool bInverse,
                             ITUPageFlowType type, ITUTransformType transformType)
{
    M2dSurface * destSurf = (M2dSurface *)dest;
    M2dSurface * srcSurf  = (M2dSurface *)src;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dest->width, dest->height);
    }

    if (x0 > sw / 3 || y0 > sh / 4 || y1 > sh / 4)
    {
        (void)printf("Out of 2D transformBlt boundary\n");
        return;
    }

    gfxSurfaceTransformBlt(destSurf->m2dSurf, dx, dy, srcSurf->m2dSurf, sx, sy, sw, sh, x0, y0, x1, y1, x2, y2, x3, y3,
                           bInverse, type, transformType);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dReflected (ITUSurface * dest, int dx, int dy, ITUSurface * src, int sx, int sy, int reflectedWidth,
                          int reflectedHeight, ITUSurface * masksurf)
{
    M2dSurface * destSurf    = (M2dSurface *)dest;
    M2dSurface * srcSurf     = (M2dSurface *)src;
    M2dSurface * maskSurface = (M2dSurface *)masksurf;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dest->width, dest->height);
    }

    if (masksurf)
    {
        if (maskSurface->m2dSurf->maskSurface)
        {
            gfxSetMaskSurface(destSurf->m2dSurf, maskSurface->m2dSurf->maskSurface, true);
        }
    }

    gfxSurfaceReflected(destSurf->m2dSurf, dx, dy, srcSurf->m2dSurf, sx, sy, reflectedWidth, reflectedHeight);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dShearing(ITUSurface * dest, int dx, int dy, ITU_RECTANGLE coordinate, ITUSurface * src, ITUColor* color)
{
    M2dSurface *  destSurf   = (M2dSurface *)dest;
    M2dSurface *  srcSurf    = (M2dSurface *)src;
    GFXSurfaceDst dstSurface = {0};
    GFXSurfaceSrc srcSurface = {0};
    GFX_RECTANGLE position   = {0};
	GFXColor      fillColor = { 0 };

    fillColor.a = color->alpha;
    fillColor.r = color->red;
    fillColor.g = color->green;
    fillColor.b = color->blue;

    dstSurface.dstSurface    = destSurf->m2dSurf;
    dstSurface.dstX          = dx;
    dstSurface.dstY          = dy;
    dstSurface.dstWidth      = dest->width;
    dstSurface.dstHeight     = dest->height;

    srcSurface.srcSurface    = srcSurf->m2dSurf;
    srcSurface.srcX          = 0;
    srcSurface.srcY          = 0;
    srcSurface.srcWidth      = src->width;
    srcSurface.srcHeight     = src->height;

    position.x0              = coordinate.x0;
    position.y0              = coordinate.y0;
    position.x1              = coordinate.x1;
    position.y1              = coordinate.y1;
    position.x2              = coordinate.x2;
    position.y2              = coordinate.y2;
    position.x3              = coordinate.x3;
    position.y3              = coordinate.y3;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dest->width, dest->height);
    }

    gfxSurfaceShearing(&dstSurface, position, &srcSurface, fillColor);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

static void M2dColorFillWithShearing(ITUSurface * dest, int dx, int dy, int w, int h, ITU_RECTANGLE coordinate, ITUColor * color)
{
    M2dSurface *   destSurf = (M2dSurface *)dest;
    GFXColor       fillColor = { 0 };
    GFXSurfaceDst  dstSurface = { 0 };
    GFXSurfaceSrc  srcSurface = { 0 };
    GFX_RECTANGLE  position = { 0 };
    GFXColorFormat gfxformat = GFX_COLOR_UNKNOWN;
    GFXSurface *   gfxSurface = NULL;

    switch (dest->format)
    {
    case ITU_RGB565:
        gfxformat = GFX_COLOR_RGB565;
        break;
    case ITU_ARGB1555:
        gfxformat = GFX_COLOR_ARGB1555;
        break;
    case ITU_ARGB4444:
        gfxformat = GFX_COLOR_ARGB4444;
        break;
    case ITU_ARGB8888:
        gfxformat = GFX_COLOR_ARGB8888;
        break;
    default:
        gfxformat = GFX_COLOR_RGB565;
        break;
    }

    gfxSurface = gfxCreateSurface(w, h, dest->pitch, gfxformat);

    if (!gfxSurface)
    {
        ITU_LOG_ERR("gfxCreateSurface fail!\n");
    }

    fillColor.a = color->alpha;
    fillColor.r = color->red;
    fillColor.g = color->green;
    fillColor.b = color->blue;

    dstSurface.dstSurface = gfxSurface;
    dstSurface.dstX = 0;
    dstSurface.dstY = 0;
    dstSurface.dstWidth = w;
    dstSurface.dstHeight = h;

    gfxSurfaceFillColor(&dstSurface, fillColor);

    dstSurface.dstSurface = destSurf->m2dSurf;
    dstSurface.dstX = dx;
    dstSurface.dstY = dy;
    dstSurface.dstWidth = dest->width;
    dstSurface.dstHeight = dest->height;

    srcSurface.srcSurface = gfxSurface;
    srcSurface.srcX = 0;
    srcSurface.srcY = 0;
    srcSurface.srcWidth = w;
    srcSurface.srcHeight = h;

    position.x0 = coordinate.x0;
    position.y0 = coordinate.y0;
    position.x1 = coordinate.x1;
    position.y1 = coordinate.y1;
    position.x2 = coordinate.x2;
    position.y2 = coordinate.y2;
    position.x3 = coordinate.x3;
    position.y3 = coordinate.y3;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dest->width, dest->height);
    }

    gfxSurfaceShearing(&dstSurface, position, &srcSurface, fillColor);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }

    if (gfxSurface)
    {
        gfxDestroySurface(gfxSurface);
    }
}

static void M2dMirror (ITUSurface * dest, int dx, int dy, ITUSurface * src, int sx, int sy)
{
    M2dSurface * destSurf = (M2dSurface *)dest;
    M2dSurface * srcSurf  = (M2dSurface *)src;

    if (dest->flags & ITU_CLIPPING)
    {
        _SetClipRectRegion(dest, dx, dy, dest->width, dest->height);
    }

    gfxSurfaceMirror(destSurf->m2dSurf, dx, dy, srcSurf->m2dSurf, sx, sy);

    if (dest->flags & ITU_CLIPPING)
    {
        gfxSurfaceUnSetClip(destSurf->m2dSurf);
    }
}

#ifdef CFG_M2D_HUD_ENABLE
#ifndef CFG_HUD_AUTO_GENERATE
extern uint32_t * HUDAddrData;

void ituUpdateHudCmdTable (uint32_t * cmdTable, uint32_t size)
{
    HudSurface * srcSurf = NULL;
    ITUSurface * lcdSurf = NULL;
    lcdSurf = ituGetDisplaySurface();

    if (lcdSurf)
    {
        srcSurf = (HudSurface *)lcdSurf;
    }

    (void)memcpy(HUDAddrData, cmdTable, size);
    (void)printf("HUDAddrData[30]:0x%x, HUDAddrData[31]:0x%x\n", HUDAddrData[30], HUDAddrData[31]);

    if (srcSurf)
    {
        M2dUpdateHudAddr(srcSurf);
    }
}
#else

int ituUpdateHudDstTable(char * srcfilepath, char * dstfilepath, uint16_t * dstBuf, uint32_t blockW, uint32_t blockH)
{
    bool result;
    int state;
    HudSurface * srcSurf = NULL;
    ITUSurface * lcdSurf = NULL;

    if ((dstfilepath == NULL) && (dstBuf == NULL))
    {
        printf("HUD dstfilepath or dstBuf is NULL\n");
        return HUD_TABLE_PATH_FAIL;
    }
    if ((blockW == 0) || (blockH == 0) || ((CFG_LCD_WIDTH % blockW) != 0) || ((CFG_LCD_HEIGHT % blockH) != 0))
    {
        printf("HUD block size is invalid\n");
        return HUD_BLOCK_SIZE_INVALID;
    }

    lcdSurf = ituGetDisplaySurface();

    if (lcdSurf)
    {
        srcSurf = (HudSurface *)lcdSurf;
    }

    gfxwaitEngineIdle();

    gfxHUDInit(blockW, blockH, GFX_HUD_ROT0);

    result = gfxGetCoordinate(srcfilepath, dstfilepath, dstBuf, gfxGetHudRotateType());

    gfxHUDPreProcessing();

    if (srcSurf)
    {
        M2dUpdateHudAddr(srcSurf);
    }

    if (result)
    {
        state = HUD_TABLE_UPDATE_SUCCESS;
        printf("HUD table update sussess!\n");
    }
    else
    {
        state = HUD_TABLE_FOPEN_FAIL;
        printf("HUD table fopen fail!\n");
    }

    return state;
}
#endif


bool ituHudIsOff(void)
{
    return gbHudOff;
}

void ituHudOff(void)
{
    gbHudOff = true;
}

void ituHudOn(void)
{
    gbHudOff = false;
}
#endif

void ituM2dInit (void)
{
    ITUSurface *   lcdSurf = NULL;
#ifdef CFG_M2D_HUD_ENABLE
    HudSurface *   srcSurf = NULL;
    GFXColorFormat format  = GFX_COLOR_UNKNOWN;
    uint32_t       size    = 0;
#else
    M2dSurface *   srcSurf = NULL;
#endif

    if (gM2dInit)
    {
        return;
    }

    gfxInitialize();

#ifdef ITU_ENABLE_DEFER_DESTROY
    gfxRegisterBlitIdCallback(0, blitIdIntrCallback);
    if (!deferQueue)
    {
        deferQueue = xQueueCreate(MAX_QUEUE_ENTRY, sizeof(DeferDestroyEntry));
    }
#endif
    lcdSurf = ituGetDisplaySurface();

#ifndef CFG_M2D_HUD_ENABLE
    if (lcdSurf)
    {
        srcSurf = (M2dSurface *)lcdSurf;
    }

#else
    if (lcdSurf)
    {
        srcSurf = (HudSurface *)lcdSurf;
    }

    size    = ithLcdGetWidth() / 16 * 2 * ithLcdGetHeight() / 4;
    tmp_dst = malloc(size);
    (void)memset(tmp_dst, 0xFF, sizeof(size));

    switch (ithLcdGetFormat())
    {
        case ITH_LCD_RGB565:
            format = GFX_COLOR_RGB565;
            break;
        case ITH_LCD_ARGB1555:
            format = GFX_COLOR_ARGB1555;
            break;
        case ITH_LCD_ARGB4444:
            format = GFX_COLOR_ARGB4444;
            break;
        case ITH_LCD_ARGB8888:
            format = GFX_COLOR_ARGB8888;
            break;
        default:
            format = GFX_COLOR_RGB565;
    }
    tempHudRotateSurf = gfxCreateSurface(ithLcdGetWidth() + gTempHudIndex, ithLcdGetHeight() + gTempHudIndex * 2, (ithLcdGetWidth() + gTempHudIndex)*2, format);

    
#endif

    // ituGetDisplaySurface  = M2dGetDisplaySurface; //add
    // ITUSurface* lcdSurf  = ituGetDisplaySurface();
    ituCreateSurface      = M2dCreateSurface;
    ituDestroySurface     = M2dDestroySurface;
    ituCreateMaskSurface  = M2dCreateMaskSurface;  // add
    ituDestroyMaskSurface = M2dDestroyMaskSurface; // add
    ituSetMaskSurface     = M2dSetMaskSurface;     // add

    ituLockSurface        = M2dLockSurface;
    ituUnlockSurface      = M2dUnlockSurface;
    ituDrawGlyph          = M2dDrawGlyph;
    ituDrawLine           = M2dDrawLine;  // add
    ituDrawCurve          = M2dDrawCurve; // add

    ituBitBlt             = M2dBitBlt;
    ituStretchBlt         = M2dStretchBlt; // add
    ituAlphaBlend         = M2dAlphaBlend;
    ituColorFill          = M2dColorFill;
    ituColorFillBlend     = M2dColorFillBlend; // add

    ituFlip               = M2dHWFlip;

    ituSetRotation        = M2dSetRotation;
    ituRotate             = M2dRotate;
    ituTransform          = M2dTransform;

    ituGradientFill       = M2dGradientFill;
    ituGradientFillBlend  = M2dGradientFillBlend; // add

    ituProjection         = M2dProjection;        // add
    ituTransformBlt       = M2dTransformBlt;      // add
    ituReflected          = M2dReflected;

    ituFlipSync           = M2dFlipSync;
    ituReplaceSurface     = M2dReplaceSurface;
    ituShearing           = M2dShearing;
	ituMirror                = M2dMirror;

	ituColorFillWithShearing = M2dColorFillWithShearing;
	
    m2dRotation           = ITU_ROT_0;

#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    ithCmdQSetTripleBuffer(ITH_CMDQ0_OFFSET);
#endif

#ifdef CFG_M2D_HUD_ENABLE
    #ifndef CFG_HUD_AUTO_GENERATE
    FILE * fd = NULL;

    if (fd == 0)
    {
        fd = fopen(CFG_PRIVATE_DRIVE ":/hud/HUDAddrData[].txt", "r");
        if (fd == 0)
        {
            (void)printf("HUDAddrData[].txt file is not exist\n");
        }
    }
    if (fd)
    {
        uint32_t * buffer;
        uint32_t   i = 0;

        buffer       = (uint32_t *)malloc(gHUDAddrDataNum * sizeof(uint32_t));

        while (fscanf(fd, "%x", &buffer[i]) > 0)
        {
            i++;
        }
        if (i != gHUDAddrDataNum)
        {
            (void)printf("[ERROR] HUD File Incorrect!!!\n");
            while (1);
        }

        (void)printf(" buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2],
               buffer[3]);

        ituUpdateHudCmdTable(buffer, gHUDAddrDataNum * sizeof(uint32_t));

        fclose(fd);
        free(buffer);
    }

    #else
    M2dUpdateHudAddr(srcSurf);
    #endif
    if (gfxGetDispSurfIndex() == 0)
    {
        srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[0];
        srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrA();
    }
    else if (gfxGetDispSurfIndex() == 1)
    {
        srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[1];
        srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrB();
    }
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
    else if (gfxGetDispSurfIndex() == 2)
    {
        srcSurf->m2dSurf->imageData    = srcSurf->pUiBuffer[2];
        srcSurf->lcdDstSurf->imageData = (uint8_t *)ithLcdGetBaseAddrC();
    }
    #endif
#else
    if (lcdSurf)
    {
        if (gfxGetDispSurfIndex() == 0)
        {
            lcdSurf->addr = ithLcdGetBaseAddrA();
        }
        else if (gfxGetDispSurfIndex() == 1)
        {
            lcdSurf->addr = ithLcdGetBaseAddrB();
        }
        else if (gfxGetDispSurfIndex() == 2)
        {
            lcdSurf->addr = ithLcdGetBaseAddrC();
        }

        gfxSurfaceSetSurfaceBaseAddress(srcSurf->m2dSurf, lcdSurf->addr);
    }
#endif

    gM2dInit = true;
}

void ituM2dExit (void)
{
    // TODO: IMPLEMENT
    ITUSurface * lcdSurf = ituGetDisplaySurface();
    // M2dSurface *srcSurf = (M2dSurface *)lcdSurf;

    // (void)printf("lcdSurf->addr:0x%x  GetA:0x%x GetB:0x%x
    // GetC:0x%x\n",lcdSurf->addr,ithLcdGetBaseAddrA(),ithLcdGetBaseAddrB(),ithLcdGetBaseAddrC());

    gfxwaitEngineIdle();

    ithLcdDisableHwFlip();

    if (lcdSurf)
    {
        if (lcdSurf->addr != ithLcdGetBaseAddrA())
        {
            lcdSurf->addr = ithLcdGetBaseAddrA();
        }
        else
        {
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
            ithLcdSwFlip(0);
#else
            ithLcdSwFlip(0);
            while (ithLcdGetFlip() != 0)
            {
                (void)usleep(500);
            }
#endif
        }
    }

#ifdef CFG_M2D_HUD_ENABLE
    if (tmp_dst)
    {
        free(tmp_dst);
    }

    if (tempHudRotateSurf)
    {
        free(tempHudRotateSurf);
    }

#endif
    // gfxSurfaceSetSurfaceBaseAddress(
    //     srcSurf->m2dSurf,
    //     lcdSurf->addr);

    // (void)printf("lcdSurf->addr:0x%x\n",lcdSurf->addr);
    gfxTerminate();

    if (gRotateBuffer[0])
    {
        itpVmemFree(gRotateBuffer[0]);
        gRotateBuffer[0] = 0;
    }
    gM2dInit = false;
}
