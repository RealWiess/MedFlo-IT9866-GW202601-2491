#include <string.h>
#include "itu_cfg.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "ite/itu.h"
#ifdef CFG_M2D_ENABLE
#include "gfx/gfx.h"
#endif
#include "itu_private.h"

static LcdSurface lcdSurface;

#ifdef CFG_M2D_HUD_ENABLE
uint8_t *                gpHudTmpBuffer[3] = {0};
extern volatile uint32_t gBlock_W;
extern volatile uint32_t gBlock_H;
extern volatile uint32_t gW_num;
extern volatile uint32_t gH_num;
#endif

#define SMTK_RGB565(r, g, b) (((b) >> 3) | (((g) >> 2) << 5) | (((r) >> 3) << 11))

static bool         gLcdInit = false;

static ITUSurface * LcdGetDisplaySurface (void)
{
    return (ITUSurface *)&lcdSurface;
}

void ituLcdInit (void)
{

    if (gLcdInit)
    {
        return;
    }

#ifdef CFG_M2D_HUD_ENABLE
    GFXHUDRotateType HUDRotateType = GFX_HUD_ROT0;    
    uint32_t type = CFG_HUD_ROTATE_TYPE;

    switch (type)
    {
    case 0:
        HUDRotateType = GFX_HUD_ROT0;
        break;
    case 1:
        HUDRotateType = GFX_HUD_ROT90;
        break;
    case 2:
        HUDRotateType = GFX_HUD_ROT180;
        break;
    case 3:
        HUDRotateType = GFX_HUD_ROT270;
        break;
    case 4:
        HUDRotateType = GFX_HUD_MIRROR;
        break;
    case 5:
        HUDRotateType = GFX_HUD_FLIP;
        break;
    default: 
        HUDRotateType = GFX_HUD_ROT0;
        break;
    }
    printf("HUDRotateType:%d\n", HUDRotateType);
    gfxHUDInit(CFG_BLOCK_WIDTH, CFG_BLOCK_HEIGHT, HUDRotateType);
#endif

    ituGetDisplaySurface = LcdGetDisplaySurface;

    if (lcdSurface.surf.addr == 0)
    {
        ITUPixelFormat format;

        (void)memset(&lcdSurface, 0, sizeof(lcdSurface));

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
                break;
        }

#ifdef CFG_M2D_HUD_ENABLE
        lcdSurface.surf.width  = gBlock_W * gW_num;     // ithLcdGetWidth();
        lcdSurface.surf.height = gBlock_H * gH_num;     // ithLcdGetHeight();
        lcdSurface.surf.pitch  = gBlock_W * gW_num * 2; // ithLcdGetPitch();
        lcdSurface.surf.format = format;
        lcdSurface.surf.flags  = ITU_STATIC;

    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        lcdSurface.pUiBuffer[0] = (uint8_t *)itpVmemAlloc(lcdSurface.surf.width * lcdSurface.surf.height *
                                                          (lcdSurface.surf.pitch / lcdSurface.surf.width));
        lcdSurface.pUiBuffer[1] = (uint8_t *)itpVmemAlloc(lcdSurface.surf.width * lcdSurface.surf.height *
                                                          (lcdSurface.surf.pitch / lcdSurface.surf.width));
        lcdSurface.pUiBuffer[2] = (uint8_t *)itpVmemAlloc(lcdSurface.surf.width * lcdSurface.surf.height *
                                                          (lcdSurface.surf.pitch / lcdSurface.surf.width));
    #else
        lcdSurface.pUiBuffer[0] = (uint8_t *)itpVmemAlloc(lcdSurface.surf.width * lcdSurface.surf.height *
                                                          (lcdSurface.surf.pitch / lcdSurface.surf.width));
        lcdSurface.pUiBuffer[1] = (uint8_t *)itpVmemAlloc(lcdSurface.surf.width * lcdSurface.surf.height *
                                                          (lcdSurface.surf.pitch / lcdSurface.surf.width));
    #endif

    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        if (ithLcdGetFlip() == 0)
        {
            lcdSurface.surf.addr = (uint32_t)lcdSurface.pUiBuffer[1];
        }
        else if (ithLcdGetFlip() == 1)
        {
            lcdSurface.surf.addr = (uint32_t)lcdSurface.pUiBuffer[2];
        }
        else if (ithLcdGetFlip() == 2)
        {
            lcdSurface.surf.addr = (uint32_t)lcdSurface.pUiBuffer[0];
        }
        else
        {
            /* do nothing */
        }
    #else
        lcdSurface.surf.addr =
            (ithLcdGetFlip() == 0) ? (uint32_t)lcdSurface.pUiBuffer[1] : (uint32_t)lcdSurface.pUiBuffer[0];
    #endif

#else
        // Get width, height, pitch setting from LCD register
        // [ToDo] Could we set the UI size different from the LCD width and
        //        height?
        lcdSurface.surf.width  = ithLcdGetWidth();
        lcdSurface.surf.height = ithLcdGetHeight();
        lcdSurface.surf.pitch  = ithLcdGetPitch();
        lcdSurface.surf.format = format;
        lcdSurface.surf.flags  = ITU_STATIC;

    #ifndef CFG_M2D_ENABLE
        lcdSurface.surf.addr   = ithLcdGetBaseAddrA();
    #else

        #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        if (ithLcdGetFlip() == 0)
        {
            lcdSurface.surf.addr = ithLcdGetBaseAddrB();
        }
        else if (ithLcdGetFlip() == 1)
        {
            lcdSurface.surf.addr = ithLcdGetBaseAddrC();
        }
        else if (ithLcdGetFlip() == 2)
        {
            lcdSurface.surf.addr = ithLcdGetBaseAddrA();
        }
        else
        {
            /* do nothing */
        }
        #else
        lcdSurface.surf.addr = (ithLcdGetFlip() == 0) ? ithLcdGetBaseAddrB() : ithLcdGetBaseAddrA();
        #endif

    #endif
#endif
        ituSetColor(&lcdSurface.surf.fgColor, 255, 255, 255, 255);
    }

#ifdef CFG_M2D_ENABLE
    // create m2d surface
    if (!lcdSurface.m2dSurf)
    {
        GFXColorFormat gfxformat = GFX_COLOR_UNKNOWN;

        switch (ithLcdGetFormat())
        {
            case ITH_LCD_RGB565:
                gfxformat = GFX_COLOR_RGB565;
                break;
            case ITH_LCD_ARGB1555:
                gfxformat = GFX_COLOR_ARGB1555;
                break;
            case ITH_LCD_ARGB4444:
                gfxformat = GFX_COLOR_ARGB4444;
                break;
            case ITH_LCD_ARGB8888:
                gfxformat = GFX_COLOR_ARGB8888;
                break;
            default: // ITH_LCD_YUV
                gfxformat = GFX_COLOR_UNKNOWN;
        }

    #ifdef CFG_M2D_HUD_ENABLE
        lcdSurface.m2dSurf =
            gfxCreateSurfaceByPointer(gBlock_W * gW_num, gBlock_H * gH_num, gBlock_W * gW_num * 2, gfxformat,
                                      (uint8_t *)lcdSurface.surf.addr, gBlock_H * gH_num * gBlock_W * gW_num * 2);
    #else
        lcdSurface.m2dSurf =
            gfxCreateSurfaceByPointer(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), gfxformat,
                                      (uint8_t *)lcdSurface.surf.addr, ithLcdGetHeight() * ithLcdGetPitch());
    #endif
    }
    #ifdef CFG_M2D_HUD_ENABLE
    // create lcd dst surface
    if (!lcdSurface.lcdDstSurf)
    {
        GFXColorFormat gfxformat = GFX_COLOR_UNKNOWN;
        uint32_t       dstAddr   = 0;

        switch (ithLcdGetFormat())
        {
            case ITH_LCD_RGB565:
                gfxformat = GFX_COLOR_RGB565;
                break;
            case ITH_LCD_ARGB1555:
                gfxformat = GFX_COLOR_ARGB1555;
                break;
            case ITH_LCD_ARGB4444:
                gfxformat = GFX_COLOR_ARGB4444;
                break;
            case ITH_LCD_ARGB8888:
                gfxformat = GFX_COLOR_ARGB8888;
                break;
            default: // ITH_LCD_YUV
                gfxformat = GFX_COLOR_UNKNOWN;
        }

        #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        if (ithLcdGetFlip() == 0)
        {
            dstAddr = ithLcdGetBaseAddrB();
        }
        else if (ithLcdGetFlip() == 1)
        {
            dstAddr = ithLcdGetBaseAddrC();
        }
        else if (ithLcdGetFlip() == 2)
        {
            dstAddr = ithLcdGetBaseAddrA();
        }
        else
        {
            /* do nothing */
        }
        #else
        dstAddr = (ithLcdGetFlip() == 0) ? ithLcdGetBaseAddrB() : ithLcdGetBaseAddrA();
        #endif
        lcdSurface.lcdDstSurf =
            gfxCreateSurfaceByPointer(ithLcdGetWidth(), ithLcdGetHeight(), ithLcdGetPitch(), gfxformat,
                                      (uint8_t *)dstAddr, ithLcdGetHeight() * ithLcdGetPitch());
    }

    #endif
#endif

    gLcdInit = true;
}

void ituLcdExit (void)
{
    // TODO: IMPLEMENT
#ifdef CFG_M2D_ENABLE
    gfxDestroySurface(lcdSurface.m2dSurf);
#endif
    gLcdInit = false;
}
