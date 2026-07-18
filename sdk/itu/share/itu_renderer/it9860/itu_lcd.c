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

#ifdef CFG_M2D_HUD_ENABLE
static GFXHUDRotateType GetHUDRotateType (uint32_t type)
{
    GFXHUDRotateType hud_rotate_type;

    switch (type)
    {
        case 1:
            hud_rotate_type = GFX_HUD_ROT90;
            break;
        case 2:
            hud_rotate_type = GFX_HUD_ROT180;
            break;
        case 3:
            hud_rotate_type = GFX_HUD_ROT270;
            break;
        case 4:
            hud_rotate_type = GFX_HUD_MIRROR;
            break;
        case 5:
            hud_rotate_type = GFX_HUD_FLIP;
            break;

        case 0:
        default:
            return GFX_HUD_ROT0;
    }

    return hud_rotate_type;
}

static void AllocUiBuffer (
    uint8_t ** p_ui_buffer, 
    int        count, 
    int        width,
    int        height, 
    int        pitch)
{
    int size = width * height * (pitch / width);
    for (int i = 0; i < count; ++i)
    {
        p_ui_buffer[i] = (uint8_t *)itpVmemAlloc(size);
    }
}
#endif

#if defined(CFG_M2D_ENABLE) || defined(CFG_M2D_HUD_ENABLE)
static GFXColorFormat GetGFXColorFormat (int lcd_format)
{
    GFXColorFormat color_format;
    switch (lcd_format)
    {
        case ITH_LCD_RGB565:
            color_format = GFX_COLOR_RGB565;
            break;
        case ITH_LCD_ARGB1555:
            color_format = GFX_COLOR_ARGB1555;
            break;
        case ITH_LCD_ARGB4444:
            color_format = GFX_COLOR_ARGB4444;
            break;
        case ITH_LCD_ARGB8888:
            color_format = GFX_COLOR_ARGB8888;
            break;
        default: // ITH_LCD_YUV
            color_format = GFX_COLOR_UNKNOWN;
            break;
    }
    return color_format;
}
#endif

void ituLcdInit (void)
{

    if (gLcdInit)
    {
        return;
    }

#ifdef CFG_M2D_HUD_ENABLE
    GFXHUDRotateType HUDRotateType = GetHUDRotateType(CFG_HUD_ROTATE_TYPE);
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
        AllocUiBuffer(lcdSurface.pUiBuffer, 3, lcdSurface.surf.width, lcdSurface.surf.height, lcdSurface.surf.pitch);
    #else
        AllocUiBuffer(lcdSurface.pUiBuffer, 2, lcdSurface.surf.width, lcdSurface.surf.height, lcdSurface.surf.pitch);
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
        GFXColorFormat gfxformat = GetGFXColorFormat(ithLcdGetFormat());

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
        GFXColorFormat gfxformat = GetGFXColorFormat(ithLcdGetFormat());
        uint32_t       dstAddr   = 0;

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
