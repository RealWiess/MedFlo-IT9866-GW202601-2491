#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char bgName[] = "ITUBackground";

bool ituBackgroundClone(ITUWidget* widget, ITUWidget** cloned)
{
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUBackground* newObj = (ITUBackground*)malloc(sizeof(ITUBackground));
        ITUBackground* oldObj = (ITUBackground*)widget;
        ITUWidget* newWidget = (ITUWidget*)newObj;

        if (newWidget == NULL)
        {
            return false;
        }
        else
        {
            (void)memcpy(newObj, oldObj, sizeof(ITUBackground));
            newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
            *cloned = newWidget;
        }
    }

    return ituIconClone(widget, cloned);
}

//prepare local surface to trans effect
void ituBackgroundLocalTransEffect(ITUWidget* widget, ITUSurface* surf)
{
    const ITUBackground* bg = (ITUBackground*)widget;
    if (bg && surf)
    {
        const ITUIcon* icon = &bg->icon;
        if (icon)
        {
            if (icon->surf)
            {
                int w = (surf->width - surf->width * widget->transformX / 100) / 2;
                int h = (surf->height - surf->height * widget->transformY / 100) / 2;
                ITUSurface* surfTransSrc = ituCreateSurface(surf->width, surf->height, 0, icon->surf->format, NULL, 0);
                if (surfTransSrc)
                {
                    if (widget->flags & ITU_STRETCH)
                    {
#ifndef CFG_M2D_ENABLE
                        ituStretchBlt(surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height, icon->surf, 0, 0, icon->surf->width, icon->surf->height);
#else
                        float scaleX = (float)surfTransSrc->width / (float)bg->icon.surf->width;
                        float scaleY = (float)surfTransSrc->height / (float)bg->icon.surf->height;
                        ituTransform(
                            surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height,
                            bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height,
                            bg->icon.surf->width / 2, bg->icon.surf->height / 2,
                            scaleX, scaleY, 0.0f,
                            ITU_TILE_FILL, true, true, 255);
#endif
                    }
                    else
                        ituBitBlt(surfTransSrc, 0, 0, icon->surf->width, icon->surf->height, icon->surf, 0, 0);

                    switch (widget->transformType)
                    {
                    case ITU_TRANSFORM_TURN_LEFT:
                        ituTransformBlt(surf, 0, 0, surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height, w, h, surf->width - w, 0, surf->width - w, surf->height, w, surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                        break;

                    case ITU_TRANSFORM_TURN_TOP:
                        ituTransformBlt(surf, 0, 0, surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height, w, h, surf->width - w, h, surf->width, surf->height - h, 0, surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                        break;

                    case ITU_TRANSFORM_TURN_RIGHT:
                        ituTransformBlt(surf, 0, 0, surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height, w, 0, surf->width - w, h, surf->width - w, surf->height - h, w, surf->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                        break;

                    case ITU_TRANSFORM_TURN_BOTTOM:
                        ituTransformBlt(surf, 0, 0, surfTransSrc, 0, 0, surfTransSrc->width, surfTransSrc->height, 0, h, surf->width, h, surf->width - w, surf->height - h, w, surf->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                        break;

                    default:
                        /* do nothing */
                        break;
                    }
                    ituDestroySurface(surfTransSrc);
                }
            }
        }
    }
}

//trans effect to dest directly
void ituBackgroundDestTransEffect(ITUWidget* widget, ITUSurface* dest, int destx, int desty)
{
    const ITUBackground* bg = (ITUBackground*)widget;
    if (bg && dest)
    {
        const ITUIcon* icon = &bg->icon;
        if (icon)
        {
            if (icon->surf)
            {
                int w = (icon->surf->width - icon->surf->width * widget->transformX / 100) / 2;
                int h = (icon->surf->height - icon->surf->height * widget->transformY / 100) / 2;
                switch (widget->transformType)
                {
                case ITU_TRANSFORM_TURN_LEFT:
                    ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, icon->surf->width, icon->surf->height, w, h, icon->surf->width - w, 0, icon->surf->width - w, icon->surf->height, w, icon->surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                    break;

                case ITU_TRANSFORM_TURN_TOP:
                    ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, icon->surf->width, icon->surf->height, w, h, icon->surf->width - w, h, icon->surf->width, icon->surf->height - h, 0, icon->surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                    break;

                case ITU_TRANSFORM_TURN_RIGHT:
                    ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, icon->surf->width, icon->surf->height, w, 0, icon->surf->width - w, h, icon->surf->width - w, icon->surf->height - h, w, icon->surf->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                    break;

                case ITU_TRANSFORM_TURN_BOTTOM:
                    ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, icon->surf->width, icon->surf->height, 0, h, icon->surf->width, h, icon->surf->width - w, icon->surf->height - h, w, icon->surf->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void ituBackgroundDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx = 0, desty = 0;
    uint8_t desta = 0;
    ITURectangle prevClip = { 0, 0, 0, 0 };
    ITUBackground* bg = (ITUBackground*)widget;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    ITUIcon* icon = &bg->icon;
    ITUSurface* bgSurf = NULL;
    bool bgSurfUsed = false;
    int orgWidth = 0;
    int orgHeight = 0;
    ITUColor emptyColor = { 0,0,0,0 };
    assert(bg);
    assert(dest);

    //cover for user wrong callback
    orgWidth = bg->orgWidth;
    orgHeight = bg->orgHeight;
    if (widget->type != ITU_BACKGROUND)
    {
        int checkType[] = {
            ITU_BUTTON,ITU_PROGRESSBAR,ITU_KEYBOARD,ITU_TRACKBAR,ITU_DIGITALCLOCK,ITU_ANALOGCLOCK,ITU_CALENDAR,
            ITU_CIRCLEPROGRESSBAR,ITU_SCROLLBAR,ITU_METER,ITU_COLORPICKER,ITU_BACKGROUNDBUTTON,ITU_RIPPLEBACKGROUND,
            ITU_CURVE,ITU_WHEELBACKGROUND,ITU_WAVEBACKGROUND,ITU_OSCILLOSCOPE,ITU_TABLEBAR,ITU_TABLEGRID,
            ITU_CHECKBOX,ITU_RADIOBOX,ITU_POPUPRADIOBOX,ITU_POPUPBUTTON
        };
        int i = 0, rer = 0, checkMax = sizeof(checkType) / sizeof(int);

        for (; i < checkMax; i++)
        {
            if ((int)widget->type == checkType[i])
            {
                rer++;
                break;
            }
        }

        if (rer == 0)
        {
            (void)printf("[Warning]%s call from %s (%d,%d,%d,%d)\n", __func__, widget->name, rect->x, rect->y, rect->width, rect->height);
            ituWidgetDrawImpl(widget, dest, x, y, alpha);
            return;
        }
    }


    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255; // widget alpha x alpha = final alpha

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] BackgroundDraw but SurfacePool not ready yet!\n");
        return;
    }

    if (icon != NULL)
    {
        if ((icon->staticSurf != NULL) && (widget->alpha > 0) && (alpha > 0))
        {
            if (widget->visible)
                ituSceneSetD2D(ituScene, widget);
            if (widget->flags & ITU_EXTERNAL)
                ituIconLoadExternalDataAPI(icon);
            else
                ituIconLoadStaticData(icon);
        }
    }
#endif

    //check mask link here
    if (icon != NULL)
    {
        if ((icon->surf != NULL) && (!(icon->iconflags & ITUICON_CORNER_MASK_ACTIVE)))
        {
            ituCornerRender(widget, NULL, NULL, NULL, NULL, NULL);
        }
    }

    if (widget->angle == 0 && !((widget->flags & ITU_STRETCH) && widget->tree.child && (orgWidth > 0) && (orgHeight > 0) && (orgWidth != rect->width || orgHeight != rect->height)))
    {
        ituWidgetSetClipping(widget, dest, x, y, &prevClip);
    }

    // drawing requirement (alpha > 0) and (widget->alpha > 0)
    if ((alpha * widget->alpha) > 0)
    {
        bool colorFillCheck = false;
        bool hasIconSurf = false;
        if (icon != NULL)
        {
            if (icon->surf != NULL)
            {
                hasIconSurf = true;
            }
        }

        //check requirement for colorfill
        if (hasIconSurf)
        {
            int currentRectW = rect->width;
            int currentRectH = rect->height;

            if ((!(widget->flags & ITU_STRETCH)) &&
                (bg->icon.surf->width < currentRectW || bg->icon.surf->height < currentRectH))
            {
                colorFillCheck = true;
            }
            else
            {
                if ((icon->surf->format == ITU_ARGB8888) && (widget->color.alpha > 0))
                {
                    colorFillCheck = true;
                }
            }
        }
        else
        {
            if (widget->color.alpha > 0)
            {
                colorFillCheck = true;
            }
        }

        if (bgSurf == NULL)
        {
            //prepare bgSurf to work for reasons:
            //case1. has orgWidth/orgHeight but current size change (use orginal rect to work)
            //case2. alpha-transform
            //case3. has stretch flag and icon but icon size is different with rect(desta!=255 or rotate)
            //case4. without icon but rotate with bgColor.alpha(> 0)
            //if ((orgWidth != rect->width || orgHeight != rect->height) && (orgWidth > 0) && (orgHeight > 0))
            ITUPixelFormat bgFormat = ITU_ARGB8888;
            if (((orgWidth != rect->width || orgHeight != rect->height) && (orgWidth > 0) &&
                 (orgHeight > 0))) //&& (!hasIconSurf || colorFillCheck))
            {
                bgSurf = ituCreateSurface(orgWidth, orgHeight, 0, bgFormat, NULL, 0);
            }
            else if ((desta != 255) && (widget->transformType != ITU_TRANSFORM_NONE))
            {
                bgSurf = ituCreateSurface(rect->width, rect->height, 0, bgFormat, NULL, 0);
            }
            else if (hasIconSurf && ((desta != 255)|| (widget->angle != 0)) && (widget->flags & ITU_STRETCH))
            {
                if ((bg->icon.surf->width != rect->width) || (bg->icon.surf->height != rect->height))
                {
                bgSurf = ituCreateSurface(rect->width, rect->height, 0, bgFormat, NULL, 0);
                }
            }
            else if ((!hasIconSurf) && (widget->color.alpha > 0) && ((widget->angle > 0) || (desta < 255) ))
            {
                bgSurf = ituCreateSurface(rect->width, rect->height, 0, bgFormat, NULL, 0);
            }
            else if ((!hasIconSurf) && (widget->alpha > 0) && (widget->alpha < 255))
            {
                bgSurf = ituCreateSurface(rect->width, rect->height, 0, bgFormat, NULL, 0);
            }
            else
            {
                /* do nothing */
            }
        }
        if (bgSurf != NULL)
        {
            bgSurfUsed = true;
            if (!colorFillCheck)
            {
                colorFillCheck = true;
            }
             //ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &widget->color);
        }

        if (colorFillCheck)
        {
            if (bgSurfUsed)
            {
                // #ifdef CFG_M2D_ENABLE // workaround hw bitblt with alpha issue for 9850
                //     bgSurf->flags |= 0x80000000;
                // #endif
                ITUColor* targetColor = (widget->color.alpha == 0) ? (&emptyColor) : (&widget->color);
                if (bg->graidentMode == ITU_GF_NONE)
                {
                    ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, targetColor);
                }
                else
                {
                    ituGradientFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, targetColor, &bg->graidentColor,
                        bg->graidentMode);
                }
            }
            else if (widget->color.alpha > 0) // colorfill to dest directly for this case
            {
                if (bg->graidentMode == ITU_GF_NONE)
                {
                    ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
                }
                else
                {
                    ituGradientFill(dest, destx, desty, rect->width, rect->height, &widget->color, &bg->graidentColor,
                                    bg->graidentMode);
                }
            }
            else
            {
                /* do nothing */
            }
        }

        if (bgSurfUsed)
        {
            bool scaledToUseHW = false;
            //draw icon here (to bgSurf)
            if (hasIconSurf)
            {
                if (widget->transformType == ITU_TRANSFORM_NONE)
                {
                    if ((widget->flags & ITU_STRETCH) && ((bgSurf->width != bg->icon.surf->width) || (bgSurf->height != bg->icon.surf->height)))
                    {
#ifndef CFG_M2D_ENABLE
                        ituStretchBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
#else
                        float scaleX = (float)bgSurf->width / (float)bg->icon.surf->width;
                        float scaleY = (float)bgSurf->height / (float)bg->icon.surf->height;
                        ituTransform(
                            bgSurf, 0, 0, bgSurf->width, bgSurf->height,
                            bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height,
                            bg->icon.surf->width / 2, bg->icon.surf->height / 2,
                            scaleX, scaleY, 0.0f,
                            ITU_TILE_FILL, true, true, 255);
#endif
                    }
                    else
                    {
                        int validW = (bgSurf->width < bg->icon.surf->width) ? (bgSurf->width) : (bg->icon.surf->width);
                        int validH = (bgSurf->height < bg->icon.surf->height) ? (bgSurf->height) : (bg->icon.surf->height);
                        if (desta == 255)
                        ituBitBlt(bgSurf, 0, 0, validW, validH, bg->icon.surf, 0, 0);
                        else
                        ituAlphaBlend(bgSurf, 0, 0, validW, validH, bg->icon.surf, 0, 0, desta);
                    }
                }
                else
                {
                    ituBackgroundLocalTransEffect(widget, bgSurf);
                }
            }

            do
            {
                ITCTree* node = widget->tree.child;
                for (; node; node = node->sibling)
                {
                    ITUWidget* child = (ITUWidget*)node;
                    if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
                        ituWidgetDraw(child, bgSurf, 0, 0, alpha);
                    child->dirty = false;
                }
            } while (false);

            if ((bgSurf->width != rect->width) || (bgSurf->height != rect->height))
            {
#ifndef CFG_M2D_ENABLE
                ITUSurface* scaledSurf = ituCreateSurface(rect->width, rect->height, 0, bgSurf->format, NULL, 0);
                if (scaledSurf)
                {
                    if (colorFillCheck)
                        ituColorFill(scaledSurf, 0, 0, scaledSurf->width, scaledSurf->height, &widget->color);

                    ituStretchBlt(scaledSurf, 0, 0, scaledSurf->width, scaledSurf->height, bgSurf, 0, 0, bgSurf->width, bgSurf->height);
                    ituDestroySurface(bgSurf);
                    bgSurf = scaledSurf;
                }
#else
                scaledToUseHW = true;
#endif
            }


            //for angle case
#ifndef CFG_M2D_ENABLE
            if (widget->angle == 0)
            {
                if (desta == 255)
                {
                    ituBitBlt(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0);
                }
                else
                {
                    ituAlphaBlend(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0, desta);
                }
            }
            else
            {
                if (desta < 255) //use surfLocal to make alpha work(win32 ituRotate can't use alpha)
                {
                    uint32_t* srcPtr = (uint32_t*)ituLockSurface(bgSurf, 0, 0, bgSurf->width, bgSurf->height);
                    if (srcPtr != NULL)
                    {
                        int xx, yy;
                        uint32_t sc;
                        uint32_t tc;
                        for (yy = 0; yy < bgSurf->height; yy++)
                        {
                            for (xx = 0; xx < bgSurf->width; xx++)
                            {
                                sc = srcPtr[bgSurf->width * yy + xx];
                                tc = (sc >> 24UL) * desta / 255;
                                tc = (((255 & 0xFF000000U) >> 24U) * (255U - tc) + ((sc & 0xFF000000U) >> 24U) * tc) / 255U;
                                sc = (sc & 0x00FFFFFFU) | (tc << 24U);
                                srcPtr[bgSurf->width * yy + xx] = sc;
                            }
                        }
                        ituUnlockSurface(bgSurf);
                    }
                }
                //else
                ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, bgSurf, bgSurf->width / 2, bgSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
            }
#else
            do
            {
                float scaleWidth = 1.0f;
                float scaleHeight = 1.0f;
                if (scaledToUseHW)
                {
                    if (bgSurf->width > 0)
                    {
                        scaleWidth = (float)rect->width / (float)bgSurf->width;
                    }
                    if (bgSurf->height > 0)
                    {
                        scaleHeight = (float)rect->height / (float)bgSurf->height;
                    }
                }
                ituTransform(
                    dest, destx, desty, rect->width, rect->height,
                    bgSurf, 0, 0, bgSurf->width, bgSurf->height,
                    bgSurf->width / 2, bgSurf->height / 2,
                    scaleWidth, scaleHeight, (float)widget->angle,
                    ITU_TILE_FILL, true, true, desta);
            } while (false);
#endif
        }
        else
        {
            if (hasIconSurf)
            {
                //(widget->transformType != ITU_TRANSFORM_NONE)
                //for bgSurf case2 --> no bgSurf here --> two case here: (trans with alpha255) or (no-trans with alpha)
                if (widget->transformType == ITU_TRANSFORM_NONE)
                {
                    int validW = (rect->width < bg->icon.surf->width) ? (rect->width) : (bg->icon.surf->width);
                    int validH = (rect->height < bg->icon.surf->height) ? (rect->height) : (bg->icon.surf->height);
                    if (desta == 255)
                    {
                        if ((widget->flags & ITU_STRETCH) && ((bg->icon.surf->width != rect->width) || (bg->icon.surf->height != rect->height)))
                        {
#ifndef CFG_M2D_ENABLE
                            ituStretchBlt(dest, destx, desty, rect->width, rect->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
#else
                            float scaleX = (float)rect->width / (float)bg->icon.surf->width;
                            float scaleY = (float)rect->height / (float)bg->icon.surf->height;
                            ituTransform(
                                dest, destx, desty, rect->width, rect->height,
                                bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height,
                                bg->icon.surf->width / 2, bg->icon.surf->height / 2,
                                scaleX, scaleY, 0.0f,
                                ITU_TILE_FILL, true, true, 255);
#endif
                        }
                        else
                        {
                            if (widget->angle == 0)
                            {
                                ituBitBlt(dest, destx, desty, validW, validH, bg->icon.surf, 0, 0);
                            }
                            else
                            {
                                ituTransform(
                                    dest, destx, desty, bg->icon.surf->width, bg->icon.surf->height,
                                    bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height,
                                    bg->icon.surf->width / 2, bg->icon.surf->height / 2,
                                    1.0f, 1.0f, widget->angle,
                                    ITU_TILE_FILL, true, true, 255);
                            }
                        }
                    }
                    else
                    {
                        ituAlphaBlend(dest, destx, desty, validW, validH, bg->icon.surf, 0, 0, desta);
                    }
                }
                else
                {
                    ituBackgroundDestTransEffect(widget, dest, destx, desty);
                }
            }
        }
        if (bgSurf)
        {
            ituDestroySurface(bgSurf);
        }
    }

    if (widget->angle == 0 && !((widget->flags & ITU_STRETCH) && widget->tree.child && (orgWidth > 0) &&
                                (orgHeight > 0) && (orgWidth != rect->width || orgHeight != rect->height)))
    {
        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    }

    if (!bgSurfUsed) // continue draw child when normal flow (without bgSurf)
    {
        ituWidgetDrawImpl(widget, dest, x, y, alpha);
    }

    if (widget->type == ITU_BACKGROUND)
    {
        ITCTree* node = widget->tree.child;
        for (; node; node = node->sibling)
        {
            ITUWidget* child = (ITUWidget*)node;
            if (child->visible && child->type == ITU_CLIPPER)
            {
                ituClipperPostDraw(child, dest, destx, desty, alpha);
            }
        }
    }
}

void ituBackgroundInit(ITUBackground* bg)
{
    assert(bg);
    ITU_ASSERT_THREAD();

    (void)memset(bg, 0, sizeof(ITUBackground));

    ituIconInit(&bg->icon);

    ituWidgetSetType(bg, ITU_BACKGROUND);
    ituWidgetSetName(bg, bgName);
    ituWidgetSetClone(bg, ituBackgroundClone);
    ituWidgetSetDraw(bg, ituBackgroundDraw);
}

void ituBackgroundLoad(ITUBackground* bg, uint32_t base)
{
    assert(bg);

    ituIconLoad(&bg->icon, base);
    ituWidgetSetClone(bg, ituBackgroundClone);
    ituWidgetSetDraw(bg, ituBackgroundDraw);
}
