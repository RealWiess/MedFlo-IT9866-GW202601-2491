#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#ifdef CFG_BUILD_M2D
#include "gfx/gfx.h"
#endif

#ifndef M2dSurface
typedef struct
{
    ITUSurface surf;
#ifdef CFG_M2D_ENABLE
    GFXSurface* m2dSurf;
#endif
} M2dSurface;
#endif

static const char spinnerName[] = "ITUSpinner";

void ituSpinnerExit(ITUWidget* widget)
{
    ITUSpinner* spin = (ITUSpinner*)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    //clear surf when surface link from iconlistbox target
    if (spin->bImgILB != NULL)
    {
        spin->icon.surf = NULL;
    }
    else
    {
        ituIconExit((ITUWidget *)(&spin->icon));
    }

    if (spin->surfSpA != NULL)
    {
        ituSurfaceRelease(spin->surfSpA);
        spin->surfSpA = NULL;
    }

    if (spin->surfSpB != NULL)
    {
        ituSurfaceRelease(spin->surfSpB);
        spin->surfSpB = NULL;
    }

    if (spin->surfSpC != NULL)
    {
        ituSurfaceRelease(spin->surfSpC);
        spin->surfSpC = NULL;
    }

    if (spin->surfMk != NULL)
    {
        ituSurfaceRelease(spin->surfMk);
        spin->surfMk = NULL;
    }

    if (spin->destTmp != NULL)
    {
        ituSurfaceRelease(spin->destTmp);
        spin->surfMk = NULL;
    }

    if (spin->srcTmp != NULL)
    {
        ituSurfaceRelease(spin->srcTmp);
        spin->surfMk = NULL;
    }
}

bool ituSpinnerClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUSpinner* spin = (ITUSpinner*)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUSpinner));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUSpinner));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    if ((!(widget->flags & ITU_EXTERNAL)) && (spin->icon.staticSurf || spin->staticSpASurf || spin->staticSpBSurf ||
                                              spin->staticSpCSurf || spin->staticMkSurf))
    {
        ITUSpinner* newSpin = (ITUSpinner*)*cloned;

        if (spin->icon.staticSurf)
        {
            ITUSurface * surf = spin->icon.staticSurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newSpin->icon.surf = NULL;
            }
            else
            {
                newSpin->icon.surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                      (const uint8_t *)surf->addr, surf->flags);
            }
        }

        if (spin->staticSpASurf)
        {
            ITUSurface * surf = spin->staticSpASurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newSpin->surfSpA = NULL;
            }
            else
            {
                newSpin->surfSpA = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                    (const uint8_t *)surf->addr, surf->flags);
            }
        }
        if (spin->staticSpBSurf)
        {
            ITUSurface * surf = spin->staticSpBSurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newSpin->surfSpB = NULL;
            }
            else
            {
                newSpin->surfSpB = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                    (const uint8_t *)surf->addr, surf->flags);
            }
        }
        if (spin->staticSpCSurf)
        {
            ITUSurface * surf = spin->staticSpCSurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newSpin->surfSpC = NULL;
            }
            else
            {
                newSpin->surfSpC = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                    (const uint8_t *)surf->addr, surf->flags);
            }
        }

        if (spin->staticMkSurf)
        {
            ITUSurface * surf = spin->staticMkSurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newSpin->surfMk = NULL;
            }
            else
            {
                newSpin->surfMk = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                   (const uint8_t *)surf->addr, surf->flags);
            }
        }

        ituWidgetUpdate(newSpin, ITU_EVENT_LOAD, 0, 0, 0);
    }
    return ituWidgetCloneImpl(widget, cloned);
}

void ituSpinnerInit(ITUSpinner* spin)
{
    assert(spin);
    ITU_ASSERT_THREAD();

    (void)memset(spin, 0, sizeof(ITUSpinner));

    ituIconInit(&spin->icon);

    ituWidgetSetType(spin, ITU_SPINNER);
    ituWidgetSetName(spin, spinnerName);
    ituWidgetSetClone(spin, ituSpinnerClone);
    ituWidgetSetDraw(spin, ituSpinnerDraw);
}

void ituSpinnerLoad(ITUSpinner* spin, uint32_t base)
{
    const ITUWidget* widget = (ITUWidget*)spin;
    assert(spin);

    ituIconLoad(&spin->icon, base);

    if (!(widget->flags & ITU_EXTERNAL))
    {
        if (spin->staticSpASurf)
        {
            ITUSurface * surf = (ITUSurface *)(base + (uint32_t)spin->staticSpASurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                spin->surfSpA = NULL;
            }
            else
            {
                spin->surfSpA = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                 (const uint8_t *)surf->addr, surf->flags);
            }

            spin->staticSpASurf = surf;
        }

        if (spin->staticSpBSurf)
        {
            ITUSurface * surf = (ITUSurface *)(base + (uint32_t)spin->staticSpBSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                spin->surfSpB = NULL;
            }
            else
            {
                spin->surfSpB = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                 (const uint8_t *)surf->addr, surf->flags);
            }

            spin->staticSpBSurf = surf;
        }

        if (spin->staticSpCSurf)
        {
            ITUSurface * surf = (ITUSurface *)(base + (uint32_t)spin->staticSpCSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                spin->surfSpC = NULL;
            }
            else
            {
                spin->surfSpC = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                 (const uint8_t *)surf->addr, surf->flags);
            }

            spin->staticSpCSurf = surf;
        }

        if (spin->staticMkSurf)
        {
            ITUSurface * surf = (ITUSurface *)(base + (uint32_t)spin->staticMkSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                spin->surfMk = NULL;
            }
            else
            {
                spin->surfMk = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                (const uint8_t *)surf->addr, surf->flags);
            }

            spin->staticMkSurf = surf;
        }
    }



    ituWidgetSetClone(spin, ituSpinnerClone);
    ituWidgetSetDraw(spin, ituSpinnerDraw);
}

ITUSurface* ituSpinnerLoadImg(ITUSpinner* spin, ITUSurface** surf, ITUSurface* staticSurf)
{
    ITUWidget* widget = (ITUWidget*)spin;
    ITUSurface* surfSrc = NULL;
    ITUSurface* result = (surf == NULL) ? (NULL) : (*surf);
    assert(spin);
    assert(ituScene);

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] SpinnerDraw but SurfacePool not ready yet!\n");
        return result;
    }
    if (!staticSurf || (widget->flags & ITU_EXTERNAL_IMAGE))
    {
        return result;
    }
#else
    if (!staticSurf || (surf && *surf) || (widget->flags & ITU_EXTERNAL_IMAGE))
    {
        return result;
    }
#endif

    if (widget->flags & ITU_EXTERNAL)
    {
        ITULayer * layer = ituGetLayer(widget);
        surfSrc          = ituLayerLoadExternalSurface(layer, (uint32_t)staticSurf);
    }
    else
    {
        surfSrc = staticSurf;
    }

    if (surfSrc)
    {
        if (surfSrc->flags & ITU_COMPRESSED)
        {
            bool drawReq = ituSceneClearD2D(ituScene);
            ituSceneCheckD2D(ituScene, drawReq);
            result = ituSurfaceDecompress(surfSrc);
        }
        else
        {
            result = ituCreateSurface(surfSrc->width, surfSrc->height, surfSrc->pitch, surfSrc->format,
                                      (const uint8_t *)surfSrc->addr, surfSrc->flags);
        }
    }

    return result;
}

void ituSpinnerSetBgAngle(ITUWidget* widget, int angle)
{
    ITUSpinner * spin = (ITUSpinner *)widget;
    assert(spin);
    if ((angle >= 0) && (angle <= 360))
    {
        spin->bgAngle = (angle == 360) ? (0) : (angle);
    }
}

void ituSpinnerSetMkAngle(ITUWidget* widget, int angle)
{
    ITUSpinner * spin = (ITUSpinner *)widget;
    assert(spin);
    if ((angle >= 0) && (angle <= 360))
    {
        spin->mkAngle = (angle == 360) ? (0) : (angle);
    }
}

void ituSpinnerSetAngle(ITUWidget* widget, int angle, int spIndex)
{
    ITUSpinner* spin = (ITUSpinner*)widget;
    assert(spin);
    if ((angle >= 0) && (angle <= 360) && (spIndex >= 0) && (spIndex < 3))
    {
        spin->spAngle = (angle == 360) ? (0) : (angle);
        spin->spIndex = spIndex;
        //(void)printf("[Spinner %s] spAngle %d, spIndex %d\n", widget->name, spin->spAngle, spin->spIndex);
    }
}

static void ituSpinnerMixMask(ITUSurface* destSurf, ITUSurface* maskSurf)
{
    if ((destSurf == NULL) || (maskSurf == NULL))
    {
        return;
    }
    else if ((destSurf->format != ITU_ARGB8888) || (maskSurf->format != ITU_ARGB8888))
    {
        return;
    }
    else
    {
        int xx, yy;
        uint8_t* PM = ituLockSurface(maskSurf, 0, 0, maskSurf->width, maskSurf->height);
        uint8_t* PD = ituLockSurface(destSurf, 0, 0, destSurf->width, destSurf->height);

        if ((PM == NULL) || (PD == NULL))
        {
            return;
        }
        else
        {
            const uint32_t* ptr = (uint32_t*)PM;
            uint32_t* dptr = (uint32_t*)PD;

#ifdef CFG_M2D_ENABLE
            GFXSurfaceSrc srcSurface = { 0 };
            const M2dSurface* msurf = (M2dSurface*)maskSurf;
            M2dSurface* micon = (M2dSurface*)destSurf;
            GFX_ROP3 ROP3 = GFX_ROP3_SRCINVERT;
            srcSurface.srcSurface = msurf->m2dSurf;
            srcSurface.srcX = 0;
            srcSurface.srcY = 0;
            srcSurface.srcWidth = maskSurf->width;
            srcSurface.srcHeight = maskSurf->height;
            gfxSurfaceBitbltWithRop(micon->m2dSurf, 0, 0, &srcSurface, ROP3);
#else
            
            for (yy = 0; yy < maskSurf->height; yy++)
            {
                if (yy >= destSurf->height)
                {
                    break;
                }
                for (xx = 0; xx < maskSurf->width; xx++)
                {
                    uint32_t src = ptr[maskSurf->width * yy + xx];
                    uint32_t aa = ((src & 0xFF000000) >> 24);

                    if (xx >= destSurf->width)
                    {
                        break;
                    }

                    if (aa > 0x10) // 70 constant alpha (0->dither->not 0) filter
                    {
                        dptr[maskSurf->width * yy + xx] = 0;
                    }
                }
            }
#endif
            ituUnlockSurface(maskSurf);
            ituUnlockSurface(destSurf);
        }
    }
}

void ituSpinnerDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx = 0, desty = 0;
    uint8_t desta = 0;
    ITURectangle prevClip = { 0, 0, 0, 0 };
    ITUSpinner* spin = (ITUSpinner*)widget;
    ITUIcon* icon = &spin->icon;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    bool fuckmode = true;
    assert(spin);
    assert(dest);
    assert(icon);

    if (!ituWidgetIsVisible(widget) || (alpha == 0))
    {
        return;
    }
    
    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255;

    if (desta > 0)
    {
        ITUSurface * spinTarget = NULL;

        if (spin->maskMode)
        {
            if (spin->destTmp == NULL)
            {
                spin->destTmp = ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
            }

            if (spin->srcTmp == NULL)
            {
                spin->srcTmp = ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
            }
        }

        if (spin->maskMode)
        {
            if (spin->destTmp != NULL)
            {
                ituColorFill(spin->destTmp, 0, 0, spin->destTmp->width, spin->destTmp->height, &widget->color);
            }
            else
            {
                return;
            }

            if (spin->srcTmp != NULL)
            {
                ITUColor ec = { 0,0,0,0 };
                ituColorFill(spin->srcTmp, 0, 0, spin->srcTmp->width, spin->srcTmp->height, &ec);
            }
            else
            {
                return;
            }
        }
        else
        {
            if (widget->color.alpha > 0)
            {
                ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
            }
        }

        icon->surf = ituSpinnerLoadImg(spin, &icon->surf, icon->staticSurf);
        if (icon->surf != NULL)
        {
            if (spin->maskMode)
            {
                ituTransform(spin->destTmp, 0, 0, icon->surf->width, icon->surf->height, icon->surf, 0, 0,
                    icon->surf->width, icon->surf->height, icon->surf->width / 2, icon->surf->height / 2, 1.0f,
                    1.0f, (float)spin->bgAngle, 0, true, true, 255);
            }
            else
            {
                if (spin->bgAngle == 0)
                {
                    if (desta == 255)
                    {
                        ituBitBlt(dest, destx, desty, icon->surf->width, icon->surf->height, icon->surf, 0, 0);
                    }
                    else
                    {
                        ituAlphaBlend(dest, destx, desty, icon->surf->width, icon->surf->height, icon->surf, 0, 0, desta);
                    }
                }
                else
                {
                    ituTransform(dest, destx, desty, icon->surf->width, icon->surf->height, icon->surf, 0, 0,
                        icon->surf->width, icon->surf->height, icon->surf->width / 2, icon->surf->height / 2, 1.0f,
                        1.0f, (float)spin->bgAngle, 0, true, true, desta);
                }
            }
        }

        spin->surfSpA = ituSpinnerLoadImg(spin, &spin->surfSpA, spin->staticSpASurf);
        spin->surfSpB = ituSpinnerLoadImg(spin, &spin->surfSpB, spin->staticSpBSurf);
        spin->surfSpC = ituSpinnerLoadImg(spin, &spin->surfSpC, spin->staticSpCSurf);

        if (spin->spIndex == 0)
        {
            spinTarget = spin->surfSpA;
        }
        else if (spin->spIndex == 1)
        {
            spinTarget = spin->surfSpB;
        }
        else
        {
            spinTarget = spin->surfSpC;
        }

        if (spinTarget)
        {
            if (spin->maskMode)
            {
                ituTransform(spin->srcTmp, 0, 0, spinTarget->width, spinTarget->height, spinTarget, 0, 0,
                    spinTarget->width, spinTarget->height, spinTarget->width / 2, spinTarget->height / 2, 1.0f,
                    1.0f, (float)spin->spAngle, 0, true, true, 255);
            }
            else
            {
                if (spin->spAngle == 0)
                {
                    if (desta == 255)
                    {
                        ituBitBlt(dest, destx, desty, spinTarget->width, spinTarget->height, spinTarget, 0, 0);
                    }
                    else
                    {
                        ituAlphaBlend(dest, destx, desty, spinTarget->width, spinTarget->height, spinTarget, 0, 0, desta);
                    }
                }
                else
                {
                    ituTransform(dest, destx, desty, spinTarget->width, spinTarget->height, spinTarget, 0, 0,
                        spinTarget->width, spinTarget->height, spinTarget->width / 2, spinTarget->height / 2, 1.0f,
                        1.0f, (float)spin->spAngle, 0, true, true, desta);
                }
            }
        }

        if (spin->maskMode)
        {
            ituSpinnerMixMask(spin->destTmp, spin->srcTmp);
            if (desta == 255)
            {
                ituBitBlt(dest, destx, desty, spin->destTmp->width, spin->destTmp->height, spin->destTmp, 0, 0);
            }
            else
            {
                ituAlphaBlend(dest, destx, desty, spin->destTmp->width, spin->destTmp->height, spin->destTmp, 0, 0, desta);
            }
        }

        spin->surfMk = ituSpinnerLoadImg(spin, &spin->surfMk, spin->staticMkSurf);
        if (spin->surfMk)
        {
            if (spin->mkAngle == 0)
            {
                if (desta == 255)
                {
                    ituBitBlt(dest, destx, desty, spin->surfMk->width, spin->surfMk->height, spin->surfMk, 0, 0);
                }
                else
                {
                    ituAlphaBlend(dest, destx, desty, spin->surfMk->width, spin->surfMk->height, spin->surfMk, 0, 0,
                                  desta);
                }
            }
            else
            {
                ituTransform(dest, destx, desty, spin->surfMk->width, spin->surfMk->height, spin->surfMk, 0, 0,
                             spin->surfMk->width, spin->surfMk->height, spin->surfMk->width / 2,
                             spin->surfMk->height / 2, 1.0f, 1.0f, (float)spin->mkAngle, 0, true, true, desta);
            }
        }
    }
}
