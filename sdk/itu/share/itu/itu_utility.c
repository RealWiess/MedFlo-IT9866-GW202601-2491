#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include "ite/itp.h"
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#ifdef CFG_BUILD_M2D
#include "gfx/gfx.h"
#endif

#pragma warning(disable : 4996)

#ifndef M2dSurface
typedef struct
{
    ITUSurface surf;
#ifdef CFG_M2D_ENABLE
    GFXSurface* m2dSurf;
#endif
} M2dSurface;
#endif

unsigned int ituFormat2Bpp (ITUPixelFormat format)
{
    switch (format)
    {
        case ITU_RGB565:
        case ITU_ARGB1555:
        case ITU_ARGB4444:
        case ITU_RGB565A8:
            return 16;
            break;

        case ITU_ARGB8888:
            return 32;
            break;

        case ITU_A8:
        case ITU_MASK_A8:
            return 8;
            break;

        case ITU_MONO:
            return 1;
            break;

        default:
            return 0;
    }
}

void ituFocusWidgetImpl (ITUWidget * widget)
{
    assert(ituScene);
    ITU_ASSERT_THREAD();

    if (widget)
    {
        if (ituScene->focused && ituScene->focused != widget)
        {
            ituWidgetSetActive(ituScene->focused, false);
            ituWidgetUpdate(ituScene->focused, ITU_EVENT_LAYOUT, ITU_ACTION_FOCUS, 0, 0);
            ituScene->focused = NULL;
        }
        ituWidgetSetActive(widget, true);
        ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, ITU_ACTION_FOCUS, 0, 0);
        ituScene->focused = widget;
    }
    else
    {
        if (ituScene->focused)
        {
            ituWidgetSetActive(ituScene->focused, false);
            ituWidgetUpdate(ituScene->focused, ITU_EVENT_LAYOUT, ITU_ACTION_FOCUS, 0, 0);
            ituScene->focused = NULL;
        }
    }
}

void ituDirtyWidgetImpl (ITUWidget * widget, bool dirty)
{
    ITCTree * node;
    ITU_ASSERT_THREAD();

    for (node = widget->tree.child; node; node = node->sibling)
    {
        ITUWidget * child = (ITUWidget *)node;
        ituDirtyWidgetImpl(child, dirty);
    }
    ituWidgetSetDirty(widget, dirty);
}

void ituUnPressWidgetImpl (ITUWidget * widget)
{
    ITCTree * node;
    ITU_ASSERT_THREAD();

    for (node = widget->tree.child; node; node = node->sibling)
    {
        ITUWidget * child = (ITUWidget *)node;
        ituUnPressWidgetImpl(child);
    }
    if (widget->type == ITU_BUTTON || widget->type == ITU_CHECKBOX || widget->type == ITU_RADIOBOX ||
        widget->type == ITU_POPUPBUTTON)
    {
        ituWidgetSetActive(widget, false);

        if (ituButtonIsPressed(widget))
        {
            ituButtonSetPressed((ITUButton *)widget, false);
        }

        if (widget->type == ITU_CHECKBOX || widget->type == ITU_RADIOBOX)
        {
            ituCheckBoxSetChecked((ITUCheckBox *)widget, ituCheckBoxIsChecked((ITUCheckBox *)widget));
        }
    }
    else if (widget->type == ITU_SPRITEBUTTON)
    {
        ITUSpriteButton * sb = (ITUSpriteButton *)widget;
        sb->pressed          = false;
    }
    else
    {
        /* do nothing */
    }
}

ITUWidget * ituFindWidgetChildImpl (ITUWidget * widget, const char * name)
{
    ITCTree * node;

    if (strcmp(name, widget->name) == 0)
    {
        return widget;
    }

    for (node = widget->tree.child; node; node = node->sibling)
    {
        ITUWidget * result = ituFindWidgetChildImpl((ITUWidget *)node, name);
        if (result)
        {
            return result;
        }
    }
    return NULL;
}

void ituScreenshot (ITUSurface * surf, char * filepath)
{
    int pos, w, h;

    ITU_ASSERT_THREAD();

    pos = strlen(filepath) - 3;
    if (pos < 0)
    {
        pos = 0;
    }

    w = surf->width;
    h = surf->height;

    if (ituScene->rotation == ITU_ROT_90 || ituScene->rotation == ITU_ROT_270)
    {
        surf->width  = h;
        surf->height = w;
    }
    else
    {
#ifdef CFG_ENABLE_ROTATE
        surf->width  = h;
        surf->height = w;
#endif // CFG_ENABLE_ROTATE
    }

    if (stricmp(&filepath[pos], "jpg") == 0)
    {
        ituJpegSaveFile(surf, filepath);
    }
    else if (stricmp(&filepath[pos], "png") == 0)
    {
        ituPngSaveFile(surf, filepath);
    }
    else
    {
        FILE * fp = fopen(filepath, "wb");
        if (fp)
        {
            int       size = surf->width * surf->height * 3;
            uint8_t * dest = malloc(size);
            if (dest)
            {
                uint8_t * src;

                if (surf == ituGetDisplaySurface())
                {
                    uint32_t addr;

                    switch (ithLcdGetFlip())
                    {
                        case 0:
                            addr = ithLcdGetBaseAddrA();
                            break;

                        case 1:
                            addr = ithLcdGetBaseAddrB();
                            break;

                        default:
                            addr = ithLcdGetBaseAddrC();
                            break;
                    }
                    src = (uint8_t *)ithMapVram(addr, surf->lockSize, ITH_VRAM_READ);
                }

                snprintf(dest, size, "P6\n%d\n%d\n255\n", surf->width, surf->height);
                fwrite(dest, 1, strlen(dest), fp);

                if (surf->format == ITU_ARGB8888)
                {
                    int hLocal;
                    for (hLocal = 0; hLocal < surf->height; hLocal++)
                    {
                        int             i, j;
                        const uint8_t * ptr = src + surf->width * 4 * hLocal;

                        // color trasform from ARGB8888 to RGB888
                        for (i = (surf->width - 1) * 4, j = (surf->width - 1) * 3; i >= 0 && j >= 0; i -= 4, j -= 3)
                        {
                            dest[surf->width * hLocal * 3 + j + 0] = ptr[i + 2];
                            dest[surf->width * hLocal * 3 + j + 1] = ptr[i + 1];
                            dest[surf->width * hLocal * 3 + j + 2] = ptr[i + 0];
                        }
                    }
                }
                else if (surf->format == ITU_RGB565)
                {
                    int hLocal;
                    for (hLocal = 0; hLocal < surf->height; hLocal++)
                    {
                        int             i, j;
                        const uint8_t * ptr = src + surf->width * 2 * hLocal;

                        // color trasform from RGB565 to RGB888
                        for (i = (surf->width - 1) * 2, j = (surf->width - 1) * 3; i >= 0 && j >= 0; i -= 2, j -= 3)
                        {
                            dest[surf->width * hLocal * 3 + j + 0] = ((ptr[i + 1]) & 0xf8) + ((ptr[i + 1] >> 5) & 0x07);
                            dest[surf->width * hLocal * 3 + j + 1] =
                                ((ptr[i + 0] >> 3) & 0x1c) + ((ptr[i + 1] << 5) & 0xe0) + ((ptr[i + 1] >> 1) & 0x3);
                            dest[surf->width * hLocal * 3 + j + 2] =
                                ((ptr[i + 0] << 3) & 0xf8) + ((ptr[i + 0] >> 2) & 0x07);
                        }
                    }
                }
                else
                {
                    /* do nothing */
                }
                fwrite(dest, 1, size, fp);
                free(dest);
            }
            else
            {
                ITU_LOG_ERR("out of memory: %d.\n", size);
            }
            fclose(fp);
        }
        else
        {
            ITU_LOG_ERR("open %s fail.\n", filepath);
        }
    }
    surf->width  = w;
    surf->height = h;
}

void ituScreenshotRect (ITUSurface * surf, int x, int y, int w, int h, char * filepath)
{
    int          pos;
    ITUSurface * tempSurf = NULL;

    ITU_ASSERT_THREAD();

    pos = strlen(filepath) - 3;
    if (pos < 0)
    {
        pos = 0;
    }

    if (w == 0)
    {
        w = surf->width;
    }

    if (h == 0)
    {
        h = surf->height;
    }

    tempSurf = ituCreateSurface(w, h, 0, surf->format, NULL, 0);
    if (!tempSurf)
    {
        goto end;
    }

    ituBitBlt(tempSurf, 0, 0, tempSurf->width, tempSurf->height, surf, x, y);

    if (stricmp(&filepath[pos], "jpg") == 0)
    {
        ituJpegSaveFile(tempSurf, filepath);
    }
    else if (stricmp(&filepath[pos], "png") == 0)
    {
        ituPngSaveFile(tempSurf, filepath);
    }
    else
    {
        /* do nothing */
    }

end:
    if (tempSurf)
    {
        ituDestroySurface(tempSurf);
    }
}

ITULayer * ituGetLayer (ITUWidget * widget)
{
    ITUWidget * parent = (ITUWidget *)widget->tree.parent;

    ITU_ASSERT_THREAD();

    while (parent)
    {
        if (parent->type == ITU_LAYER)
        {
            return (ITULayer *)parent;
        }

        parent = (ITUWidget *)parent->tree.parent;
    }
    return NULL;
}

void ituPreloadFontCache (ITUWidget * widget, ITUSurface * surf)
{
#ifdef CFG_ITU_FT_CACHE_SIZE
    ITCTree * node;

    ITU_ASSERT_THREAD();

    for (node = widget->tree.child; node; node = node->sibling)
    {
        ITUWidget * child = (ITUWidget *)node;
        ituPreloadFontCache(child, surf);
    }

    switch (widget->type)
    {
        case ITU_TEXT:
        case ITU_SCROLLTEXT:
            {
                ITUText * text   = (ITUText *)widget;
                char *    string = NULL;

                if (text->string != NULL)
                {
                    string = text->string;
                }
                else if (text->stringSet != NULL)
                {
                    string = text->stringSet->strings[text->lang];
                }
                else
                {
                    /* do nothing */
                }

                if (string != NULL)
                {
                    if (string[0] != '\0')
                    {
                        if (text->fontHeight > 0)
                        {
                            ituFtSetFontSize(text->fontWidth, text->fontHeight);
                        }

                        ituFtDrawText(surf, 0, 0, string);
                    }
                }
            }
            break;

        case ITU_TEXTBOX:
            {
                ITUTextBox * textbox = (ITUTextBox *)widget;
                ITUText *    text    = &textbox->text;
                char *       string  = NULL;

                if (text->string != NULL)
                {
                    string = text->string;
                }
                else if (text->stringSet != NULL)
                {
                    string = text->stringSet->strings[text->lang];
                }
                else
                {
                    /* do nothing */
                }

                if (string != NULL)
                {
                    if (string[0] != '\0')
                    {
                        if (text->fontHeight > 0)
                        {
                            ituFtSetFontSize(text->fontWidth, text->fontHeight);
                        }

                        ituFtDrawText(surf, 0, 0, string);
                    }
                }
            }
            break;

        case ITU_BUTTON:
        case ITU_CHECKBOX:
        case ITU_RADIOBOX:
        case ITU_POPUPBUTTON:
            {
                ITUButton * button = (ITUButton *)widget;
                ITUText *   text   = &button->text;
                char *      string = NULL;

                if (text->string != NULL)
                {
                    string = text->string;
                }
                else if (text->stringSet != NULL)
                {
                    string = text->stringSet->strings[text->lang];
                }
                else
                {
                    /* do nothing */
                }

                if (string != NULL)
                {
                    if (string[0] != '\0')
                    {
                        if (text->fontHeight > 0)
                        {
                            ituFtSetFontSize(text->fontWidth, text->fontHeight);
                        }

                        ituFtDrawText(surf, 0, 0, string);
                    }
                }
            }
            break;
    }
#endif // CFG_ITU_FT_CACHE_SIZE
}

void ituDrawGlyphEmpty (ITUSurface * surf, int x, int y, ITUGlyphFormat format, const uint8_t * bitmap, int w, int h)
{
    // DO NOTHING
}

ITUWidget * ituGetVarTarget (int index)
{
    ITUVariable * var;
    ITUWidget *   target = NULL;
    assert(ituScene);
    ITU_ASSERT_THREAD();

    var = &ituScene->variables[index];

    if (var->cachedTarget)
    {
        target = var->cachedTarget;
    }
    else if (var->target[0] != '\0')
    {
        ITUWidget * widget = ituSceneFindWidget(ituScene, var->target);
        if (widget)
        {
            target            = widget;
            var->cachedTarget = (void *)target;
        }
    }
    else
    {
        /* do nothing */
    }
    return target;
}

void ituSetVarTarget (int index, ITUWidget * target)
{
    ITUVariable * var;

    assert(index >= 0);
    assert(index < ITU_VARIABLE_SIZE);
    if (index < 0 || index >= ITU_VARIABLE_SIZE)
    {
        ITU_LOG_ERR("incorrect index: %d\n", index);
        return;
    }
    var = &ituScene->variables[index];

    if (target)
    {
        strlcpy(var->target, target->name, sizeof(var->target));
    }
    else
    {
        var->target[0] = '\0';
    }

    var->cachedTarget = target;
}

void ituSetVarParam (int index, char * param)
{
    ITUVariable * var;

    assert(index >= 0);
    assert(index < ITU_VARIABLE_SIZE);
    if (index < 0 || index >= ITU_VARIABLE_SIZE)
    {
        ITU_LOG_ERR("incorrect index: %d\n", index);
        return;
    }
    var = &ituScene->variables[index];

    if (param)
    {
        strlcpy(var->param, param, sizeof(var->param));
    }
    else
    {
        var->param[0] = '\0';
    }
}

char * ituGetVarParam (int index)
{
    ITUVariable * var;
    assert(ituScene);
    ITU_ASSERT_THREAD();

    var = &ituScene->variables[index];
    return var->param;
}

void ituAssertThread (const char * file)
{
    assert(ituScene);
    if (!pthread_equal(ituScene->threadID, pthread_self()))
    {
        ITU_LOG_ERR("itu thread assertion fail: %s\n", file);
        sleep(1);
        abort();
    }
}

void ituSetClipping (ITUSurface * surf, int x, int y, int width, int height, ITURectangle * prevClip)
{
    assert(surf);
    assert(prevClip);
    ITU_ASSERT_THREAD();

    (void)memcpy(prevClip, &surf->clipping, sizeof(ITURectangle));

    if (surf->flags & ITU_CLIPPING)
    {
        int left, top, right, bottom, r1, r2;
        int cx, cy, cw, ch;

        left   = x > prevClip->x ? x : prevClip->x;

        r1     = x + width;
        r2     = prevClip->x + prevClip->width;
        right  = r1 < r2 ? r1 : r2;

        r1     = y + height;
        r2     = prevClip->y + prevClip->height;
        bottom = r1 < r2 ? r1 : r2;

        top    = y > prevClip->y ? y : prevClip->y;

        cx     = left;
        cy     = top;
        cw     = right - left;
        ch     = bottom - top;

        if (cw < 0)
        {
            cw = 0;
        }

        if (ch < 0)
        {
            ch = 0;
        }

        ituSurfaceSetClipping(surf, cx, cy, cw, ch);
    }
    else
    {
        ituSurfaceSetClipping(surf, x, y, width, height);
    }
}

void * ituMalloc (size_t size)
{
    return malloc(size);
}

void ituFree (void * ptr)
{
    free(ptr);
}

bool ituTouchIsInsideWidget (ITUWidget * widget, int x, int y)
{
    ITU_ASSERT_THREAD();

    if (!widget->visible)
    {
        return false;
    }
    else
    {
        int absX1 = 0, absY1 = 0, absX2, absY2;
        int bw = widget->bound.width ? widget->bound.width : widget->rect.width;
        int bh = widget->bound.height ? widget->bound.height : widget->rect.height;
        ituWidgetGetGlobalPosition(widget, &absX1, &absY1);
        absX1 += (widget->bound.x != 0) ? (widget->bound.x) : (0);
        absY1 += (widget->bound.y != 0) ? (widget->bound.y) : (0);
        absX2 = absX1 + bw;
        absY2 = absY1 + bh;
        if ((x >= absX1) && (y >= absY1) && (x <= absX2) && (y <= absY2))
        {
            return true;
        }
        return false;
    }
}

void ituBitmapFileGetSize (char * filepath, int * width, int * height)
{
    FILE * fp = fopen(filepath, "rb");
    if (fp)
    {
        unsigned char data[64] = {0};
        fseek(fp, 0, SEEK_SET);
        fread(data, 1, 64, fp);
        *width  = ((unsigned int)data[18]) + ((unsigned int)data[19] << 8);
        *height = ((unsigned int)data[22]) + ((unsigned int)data[23] << 8);
        fclose(fp);
    }
}

void ituBitmapFileLoad(char* filepath, ITUSurface* surf)
{
    if ((filepath == NULL) || (surf == NULL))
    {
        return;
    }
    else if (surf->format != ITU_ARGB8888)
    {
        return;
    }
    else
    {
        FILE* fp = fopen(filepath, "rb");
        if (fp != NULL)
        {
            char headerInfo[54];
            int i, j, width, height;
            uint32_t* destPtr = NULL;
            unsigned char data[64] = { 0 };
            fseek(fp, 0, SEEK_SET);
            fread(data, 1, 64, fp);
            width = ((unsigned int)data[18]) + ((unsigned int)data[19] << 8);
            height = ((unsigned int)data[22]) + ((unsigned int)data[23] << 8);
            fseek(fp, 0, SEEK_SET);
            fread(&headerInfo, sizeof(char) * 54, 1, fp);

            destPtr = (uint32_t*)ituLockSurface(surf, 0, 0, width, height);
            for (i = height - 1; i >= 0; i--)
            {
                for (j = 0; j < width; j++)
                {
                    uint8_t cc[3];
                    fread(&cc[0], 1, 1, fp);
                    fread(&cc[1], 1, 1, fp);
                    fread(&cc[2], 1, 1, fp);
                    destPtr[((width * i) + j)] = (uint32_t)ITH_ARGB8888(255, cc[2], cc[1], cc[0]);
                }
            }

            ituUnlockSurface(surf);
            fclose(fp);
        }
    }
}

bool ituSurfaceCopy(ITUSurface* surfSrc, int sx, int sy, int sw, int sh, ITUSurface* surfDst, int dx, int dy)
{
    ITU_ASSERT_THREAD();
    if ((surfSrc == NULL) || (surfDst == NULL))
    {
        return false;
    }
    else
    {
        if (((sx + sw) > surfSrc->width) || ((sy + sh) > surfSrc->height) || ((dx + sw) > surfDst->width) || ((dy + sh) > surfDst->height) || (surfSrc->format != surfDst->format))
        {
            return false;
        }
        else
        {
            if (surfSrc->format == ITU_RGB565)
            {
                int dw = 0, dh = 0;
                uint16_t* srcPtr = (uint16_t*)ituLockSurface(surfSrc, sx, sy, sw, sh);
                uint16_t* dstPtr = (uint16_t*)ituLockSurface(surfDst, dx, dy, sw, sh);
                for (; dh < sh; dh++)
                {
                    for (; dw < sw; dw++)
                    {
                        dstPtr[(surfDst->width * dh) + dw] = srcPtr[(surfSrc->width * dh) + dw];
                    }
                }
                ituUnlockSurface(surfSrc);
                ituUnlockSurface(surfDst);
            }
            else if (surfSrc->format == ITU_ARGB8888)
            {
                int dw = 0, dh = 0;
                uint32_t* srcPtr = (uint32_t*)ituLockSurface(surfSrc, sx, sy, sw, sh);
                uint32_t* dstPtr = (uint32_t*)ituLockSurface(surfDst, dx, dy, sw, sh);
                for (; dh < sh; dh++)
                {
                    for (; dw < sw; dw++)
                    {
                        dstPtr[(surfDst->width * dh) + dw] = srcPtr[(surfSrc->width * dh) + dw];
                    }
                }
                ituUnlockSurface(surfSrc);
                ituUnlockSurface(surfDst);
            }
            else
            {
                return false;
            }

            return true;
        }
    }
    return false;
}

void ituSurfaceStretch (ITUSurface * surf, int width, int height)
{
    assert(surf);
    ITU_ASSERT_THREAD();

    if ((surf->width != width) && (surf->height != height) && (width > 0) && (height > 0))
    {
        ITUSurface * tmpSurf = ituCreateSurface(width, height, 0, surf->format, NULL, 0);
        if (tmpSurf)
        {
            int          y = 0, x = 0;
            int          pitch   = width * ituFormat2Bpp(surf->format) / 8;
            unsigned int size    = pitch * height;
            uint8_t *    ptrDest = NULL;
            uint8_t *    ptrSrc  = NULL;
            uint32_t *   destPtr = (uint32_t *)ituLockSurface(tmpSurf, 0, 0, width, height);
            uint32_t *   srcPtr  = (uint32_t *)ituLockSurface(surf, 0, 0, surf->width, surf->height);
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    uint32_t sc;
                    int      xx, yy;

                    xx                              = x * surf->width / width;
                    yy                              = y * surf->height / height;
                    sc                              = srcPtr[surf->width * yy + xx];
                    destPtr[tmpSurf->width * y + x] = sc;
                }
            }
            ituUnlockSurface(surf);
            ituUnlockSurface(tmpSurf);

            itpVmemFree(surf->addr);
            surf->addr = itpVmemAlloc(size);

            ituReplaceSurface(surf, width, height, pitch, surf->format);

            ptrDest = (uint8_t *)ithMapVram(surf->addr, size, ITH_VRAM_WRITE);
            ptrSrc  = (uint8_t *)ithMapVram(tmpSurf->addr, size, ITH_VRAM_READ);
            (void)memcpy(ptrDest, ptrSrc, size);
            ithUnmapVram(ptrDest, size);
            ithUnmapVram(ptrSrc, size);
            ituDestroySurface(tmpSurf);
        }
    }
}

void ituTreeLoopExec(ITUWidget* fSrc, ITUWidget* fDst)
{
    if ((fSrc != NULL) && (fDst != NULL))
    {
        ITCTree* treeSrc = (ITCTree*)fSrc;
        ITCTree* treeDst = (ITCTree*)fDst;
        ITCTree* child = treeSrc->child;

        for (; child;)
        {
            ITUWidget* Obj = (ITUWidget*)child;
            ITUWidget* cloned = NULL;
            if (ituWidgetClone(Obj, &cloned))
            {
                if (cloned != NULL)
                {
                    ituWidgetAdd(fDst, cloned);
                    ituTreeLoopExec(Obj, cloned);
                }

                child = child->sibling;
                continue;
            }
            break;
        }
    }
}

bool ituTreeClone(ITUWidget* widget, ITUWidget** cloned)
{
    if ((widget != NULL) && (cloned != NULL))
    {
        if (ituWidgetClone(widget, cloned))
        {
            ituTreeLoopExec(widget, *cloned);
            return true;
        }
    }

    return false;
}

char* wchar_to_char(const wchar_t* pwchar, size_t* size)
{
    int currentCharIndex = 0;
    char currentChar = pwchar[currentCharIndex];

    while (currentChar != '\0')
    {
        currentCharIndex++;
        currentChar = pwchar[currentCharIndex];
    }

    const int charCount = currentCharIndex + 1;

    char* filePathC = (char*)malloc(sizeof(char) * charCount);

    if (filePathC != NULL)
    {

        for (int i = 0; i < charCount; i++)
        {
            char character = pwchar[i];

            *filePathC = character;

            filePathC += sizeof(char);

        }
        filePathC += '\0';

        filePathC -= (sizeof(char) * charCount);

    }

    *size = (sizeof(char) * charCount);

    return filePathC;
}

void ituCornerMaskLinkSetup(ITUWidget* widget, ITUCornerMaskScale* maskScale, ITUIcon* iconSrc, bool enable)
{
    if ((widget != NULL) && (iconSrc != NULL))
    {
        ITUIcon* icon;
        if (widget->type == ITU_ICON)
        {
            icon = (ITUIcon*)widget;
        }
        else if (widget->type == ITU_BACKGROUND)
        {
            ITUBackground* bg = (ITUBackground*)widget;
            icon = &bg->icon;
        }
        else if (widget->type == ITU_BUTTON)
        {
            ITUButton* btn = (ITUButton*)widget;
            icon = &btn->bg.icon;
        }
        else
        {
            icon = NULL;
        }

        if (enable)
        {
            if (icon != NULL)
            {
                ITUSurface* surf = icon->staticSurf;

                icon->maskTL = iconSrc->maskTL;
                icon->maskTR = iconSrc->maskTR;
                icon->maskBL = iconSrc->maskBL;
                icon->maskBR = iconSrc->maskBR;

                icon->staticMaskTL = iconSrc->staticMaskTL;
                icon->staticMaskTR = iconSrc->staticMaskTR;
                icon->staticMaskBL = iconSrc->staticMaskBL;
                icon->staticMaskBR = iconSrc->staticMaskBR;

                if (surf != NULL)
                {
                    surf->cornerMaskScale.topLeft = 1.0f;
                    surf->cornerMaskScale.topRight = 1.0f;
                    surf->cornerMaskScale.bottomLeft = 1.0f;
                    surf->cornerMaskScale.bottomRight = 1.0f;
                    surf->cornerMaskScale.dxTL = surf->cornerMaskScale.dyTL = 0;
                    surf->cornerMaskScale.dxTR = surf->cornerMaskScale.dyTR = 0;
                    surf->cornerMaskScale.dxBL = surf->cornerMaskScale.dyBL = 0;
                    surf->cornerMaskScale.dxBR = surf->cornerMaskScale.dyBR = 0;

                    if (maskScale != NULL)
                    {
                        surf->cornerMaskScale.topLeft = maskScale->topLeft;
                        surf->cornerMaskScale.topRight = maskScale->topRight;
                        surf->cornerMaskScale.bottomLeft = maskScale->bottomLeft;
                        surf->cornerMaskScale.bottomRight = maskScale->bottomRight;
                        surf->cornerMaskScale.dxTL = maskScale->dxTL;
                        surf->cornerMaskScale.dyTL = maskScale->dyTL;
                        surf->cornerMaskScale.dxTR = maskScale->dxTR;
                        surf->cornerMaskScale.dyTR = maskScale->dyTR;
                        surf->cornerMaskScale.dxBL = maskScale->dxBL;
                        surf->cornerMaskScale.dyBL = maskScale->dyBL;
                        surf->cornerMaskScale.dxBR = maskScale->dxBR;
                        surf->cornerMaskScale.dyBR = maskScale->dyBR;
                    }
                }

                icon->iconflags |= ITUICON_CORNER_MASK_LINKING;
            }
        }
        else
        {
            if (icon != NULL)
            {
                ITUSurface* surf = icon->staticSurf;
                if (surf != NULL)
                {
                    surf->cornerMaskScale.topLeft = 1.0f;
                    surf->cornerMaskScale.topRight = 1.0f;
                    surf->cornerMaskScale.bottomLeft = 1.0f;
                    surf->cornerMaskScale.bottomRight = 1.0f;
                    surf->cornerMaskScale.dxTL = surf->cornerMaskScale.dyTL = 0;
                    surf->cornerMaskScale.dxTR = surf->cornerMaskScale.dyTR = 0;
                    surf->cornerMaskScale.dxBL = surf->cornerMaskScale.dyBL = 0;
                    surf->cornerMaskScale.dxBR = surf->cornerMaskScale.dyBR = 0;
                }

                icon->maskTL = NULL;
                icon->maskTR = NULL;
                icon->maskBL = NULL;
                icon->maskBR = NULL;

                icon->staticMaskTL = NULL;
                icon->staticMaskTR = NULL;
                icon->staticMaskBL = NULL;
                icon->staticMaskBR = NULL;

                icon->iconflags &= ~ITUICON_CORNER_MASK_LINKING;
                ituIconReleaseSurface(icon);
                ituIconLoadStaticData(icon);
            }
        }
    }
}

void ituCornerRender(ITUWidget* widget, ITUSurface* surfDest, ITUSurface* maskTL, ITUSurface* maskTR, ITUSurface* maskBL, ITUSurface* maskBR)
{
    ITUSurface* dest = NULL;
    ITUIcon* icon = NULL;
    ITUSurface* mTL = maskTL;
    ITUSurface* mTR = maskTR;
    ITUSurface* mBL = maskBL;
    ITUSurface* mBR = maskBR;
    ITUButton* btn = NULL;

    if ((widget == NULL) && (surfDest == NULL))
    {
        return;
    }

    if (widget != NULL)
    {
        if (widget->type == ITU_ICON)
        {
            icon = (ITUIcon*)widget;
        }
        else if (widget->type == ITU_BACKGROUND)
        {
            ITUBackground* bg = (ITUBackground*)widget;
            icon = &bg->icon;
        }
        else if (widget->type == ITU_BUTTON)
        {
            btn = (ITUButton*)widget;
            icon = &btn->bg.icon;
        }
        else
        {
            icon = NULL;
        }

        if (icon != NULL)
        {
            dest = icon->surf;
        }
    }
    else
    {
        dest = surfDest;
    }

    if (dest == NULL)
    {
        return;
    }
    else if (dest->format != ITU_ARGB8888)
    {
        return;
    }
    else
    {
        if (icon != NULL)
        {
            mTL = (icon->maskTL != NULL) ? (icon->maskTL) : (maskTL);
            mTR = (icon->maskTR != NULL) ? (icon->maskTR) : (maskTR);
            mBL = (icon->maskBL != NULL) ? (icon->maskBL) : (maskBL);
            mBR = (icon->maskBR != NULL) ? (icon->maskBR) : (maskBR);
        }
    }

    if ((mTL != NULL) || (mTR != NULL) || (mBL != NULL) || (mBR != NULL))
    {
        bool working = true;
        int loopCount = 0;
        while (working)
        {
            if (icon != NULL)
            {
                if (icon->staticSurf != NULL)
                {
                    dest->cornerMaskScale = icon->staticSurf->cornerMaskScale;
                }
            }

            if (!(dest->flags & ITU_MASKED))
            {
                ITUSurface* surf = ituCreateSurface(dest->width, dest->height, 0, ITU_ARGB8888, NULL, 0);
                if (surf != NULL)
                {
                    ITUColor ec = { 0, 0, 0, 0 };
#ifdef CFG_M2D_ENABLE
                    GFXSurfaceSrc      srcSurface = { 0 };
                    const M2dSurface* msurf = (M2dSurface*)surf;
                    M2dSurface* micon = (M2dSurface*)dest;
                    GFX_ROP3           ROP3;
                    srcSurface.srcSurface = msurf->m2dSurf;
#endif
                    ituColorFill(surf, 0, 0, surf->width, surf->height, &ec);
                    if (mTL != NULL)
                    {
                        float scaleX = dest->cornerMaskScale.topLeft;
                        float scaleY = dest->cornerMaskScale.topLeft;
                        ituTransform(surf, 0, 0, surf->width, surf->height, mTL, 0, 0, mTL->width, mTL->height, 0, 0, scaleX, scaleY,
                            0, 0, true, false, 0xFF);
                    }
                    if (mTR != NULL)
                    {
                        float scaleX = dest->cornerMaskScale.topRight;
                        float scaleY = dest->cornerMaskScale.topRight;
                        ituTransform(surf, surf->width - (mTR->width * scaleX), 0, surf->width, surf->height,
                            mTR, 0, 0, mTR->width, mTR->height, 0, 0, scaleX, scaleY, 0, 0, true, false, 0xFF);

                    }
                    if (mBL != NULL)
                    {
                        float scaleX = dest->cornerMaskScale.bottomLeft;
                        float scaleY = dest->cornerMaskScale.bottomLeft;
                        ituTransform(surf, 0, surf->height - (mBL->height * scaleY), surf->width, surf->height,
                            mBL, 0, 0, mBL->width, mBL->height, 0, 0, scaleX, scaleY, 0, 0, true, false, 0xFF);
                    }
                    if (mBR != NULL)
                    {
                        float scaleX = dest->cornerMaskScale.bottomRight;
                        float scaleY = dest->cornerMaskScale.bottomRight;
                        ituTransform(surf, surf->width - (mBR->width * scaleX), surf->height - (mBR->height * scaleY), surf->width, surf->height,
                            mBR, 0, 0, mBR->width, mBR->height, 0, 0, scaleX, scaleY, 0, 0, true, false, 0xFF);
                    }

#ifndef CFG_M2D_ENABLE
                    //combined mask
                    ituBitBlt(dest, 0, 0, surf->width, surf->height, surf, 0, 0);

                    if (dest && surf && (dest->width == surf->width) && (dest->height == surf->height))
                    {
                        int              xx, yy;
                        const uint32_t* ptr = (uint32_t*)ituLockSurface(surf, 0, 0, surf->width, surf->height);
                        uint32_t* dptr = (uint32_t*)ituLockSurface(dest, 0, 0, dest->width, dest->height);
                        for (yy = 0; yy < surf->height; yy++)
                        {
                            for (xx = 0; xx < surf->width; xx++)
                            {
                                uint32_t src = ptr[surf->width * yy + xx];
                                uint32_t aa = ((src & 0xFF000000) >> 24);
                                // uint32_t rr = ((src & 0x00FF0000) >> 16);
                                // uint32_t gg = ((src & 0x0000FF00) >> 8);
                                // uint32_t bb = (src & 0x000000FF);
                                if (aa > 0x70) // constant alpha (0->dither->not 0) filter
                                {
                                    dptr[surf->width * yy + xx] = 0;
                                }
                            }
                        }
                        ituUnlockSurface(surf);
                        ituUnlockSurface(dest);
                    }
#else
                    srcSurface.srcX = 0;
                    srcSurface.srcY = 0;
                    srcSurface.srcWidth = surf->width;
                    srcSurface.srcHeight = surf->height;
                    ROP3 = GFX_ROP3_SRCINVERT;
                    gfxSurfaceBitbltWithRop(micon->m2dSurf, 0, 0, &srcSurface, ROP3);
#endif
                    ituDestroySurface(surf);
                    dest->flags |= ITU_MASKED;
                }
            }

            if (btn != NULL)
            {
                //case 1: pressSurf
                if (loopCount == 0)
                {
                    loopCount++;
                    dest = btn->pressSurf;
                    if (dest != NULL)
                    {
                        continue;
                    }
                }
                //case 2: focusSurf
                if (loopCount == 1)
                {
                    dest = btn->focusSurf;
                    if (dest != NULL)
                    {
                        continue;
                    }
                }

                break;
            }
            else
            {
                break;
            }

            working = false;
        }
    }
    return;
}

static bool isPOL(ITUPoint* p, ITUPoint* p1, ITUPoint* p2)
{
    int dt = ((p1->x - p2->x) * (p1->x - p2->x)) + ((p1->y - p2->y) * (p1->y - p2->y));
    int d1 = ((p1->x - p->x) * (p1->x - p->x)) + ((p1->y - p->y) * (p1->y - p->y));
    int d2 = ((p->x - p2->x) * (p->x - p2->x)) + ((p->y - p2->y) * (p->y - p2->y));
    return  (dt == (d1 + d2)) ? (true) : (false);
}

typedef struct {
    float x;
    float y;
} FloatPoint;

static bool willIntersect(ITUPoint* pp, ITUPoint* pa, ITUPoint* pb)
{
    FloatPoint p, p1, p2;
    float result;
    p.x = (float)pp->x;
    p.y = (float)pp->y;
    p1.x = (float)pa->x;
    p1.y = (float)pa->y;
    p2.x = (float)pb->x;
    p2.y = (float)pb->y;
    if (p1.y > p2.y)
    {
        FloatPoint tmp = p1;
        p1 = p2;
        p2 = tmp;
    }
    if ((p.y == p1.y) || (p.y == p2.y))
    {
        p.y += 0.0001f;
    }
    if ((p.y < p1.y) || (p.y > p2.y))
    {
        return false;
    }
    if (p2.y == p1.y)
    {
        return false;
    }
    result = (((p.y - p1.y) * (p2.x - p1.x)) / (p2.y - p1.y)) + p1.x;
    return (result > p.x);
}

void ituArbitrarySWFill(ITUSurface* dest, ITUPoint* p1, ITUPoint* p2, ITUPoint* p3, ITUPoint* p4, ITUColor* color)
{
    int minX = ITH_MIN(ITH_MIN(ITH_MIN(p1->x, p2->x), p3->x), p4->x) - 5;
    int maxX = ITH_MAX(ITH_MAX(ITH_MAX(p1->x, p2->x), p3->x), p4->x);
    int minY = ITH_MIN(ITH_MIN(ITH_MIN(p1->y, p2->y), p3->y), p4->y) - 5;
    int maxY = ITH_MAX(ITH_MAX(ITH_MAX(p1->y, p2->y), p3->y), p4->y);
    int x, y;
    
    //int init_x = minX + 1;
    for (y = minY; y <= maxY; y++)
    {
        int count = 0;
        int startX = minX, dw = 0;
        bool foundFirstInSide = false;
        for (x = minX; x <= maxX; x++)
        {
            ITUPoint pt = { x, y };
            if (isPOL(&pt, p1, p2))
            {
                foundFirstInSide = true;
                continue;
            }
            if (isPOL(&pt, p2, p3))
            {
                foundFirstInSide = true;
                continue;
            }
            if (isPOL(&pt, p3, p4))
            {
                foundFirstInSide = true;
                continue;
            }
            if (isPOL(&pt, p4, p1))
            {
                foundFirstInSide = true;
                continue;
            }
            count = 0;
            if (willIntersect(&pt, p1, p2))
            {
                count++;
            }
            if (willIntersect(&pt, p2, p3))
            {
                count++;
            }
            if (willIntersect(&pt, p3, p4))
            {
                count++;
            }
            if (willIntersect(&pt, p4, p1))
            {
                count++;
            }

            if (foundFirstInSide)
            {
                if ((count % 2) == 0)
                {
                    dw = x - startX;
                    break;
                }
            }
            else
            {
                if ((count % 2) == 1)
                    foundFirstInSide = true;
            }
        }
        
        if (dw > 0)
        {
            ituColorFill(dest, startX, y, dw, 2, color);
            //printf("(%d,%d)----w: %d\n", startX, y, dw);
        }

    }
}