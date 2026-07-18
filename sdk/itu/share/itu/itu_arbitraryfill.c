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

static const char arbitraryfillName[] = "ITUArbitraryFill";

void ituArbitraryFillExit(ITUWidget* widget)
{
    ITUArbitraryFill* arbf = (ITUArbitraryFill*)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    ituIconExit((ITUWidget *)(&arbf->icon));
}

bool ituArbitraryFillClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUArbitraryFill* arbf = (ITUArbitraryFill*)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUArbitraryFill));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUArbitraryFill));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    if ((!(widget->flags & ITU_EXTERNAL)) && (arbf->icon.staticSurf))
    {
        ITUArbitraryFill* newArbf = (ITUArbitraryFill*)*cloned;

        if (arbf->icon.staticSurf)
        {
            ITUSurface * surf = arbf->icon.staticSurf;
            if (surf->flags & ITU_COMPRESSED)
            {
                newArbf->icon.surf = NULL;
            }
            else
            {
                newArbf->icon.surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                      (const uint8_t *)surf->addr, surf->flags);
            }
        }

        ituWidgetUpdate(newArbf, ITU_EVENT_LOAD, 0, 0, 0);
    }
    return ituWidgetCloneImpl(widget, cloned);
}

void ituArbitraryFillInit(ITUArbitraryFill* arbf)
{
    assert(arbf);
    ITU_ASSERT_THREAD();

    (void)memset(arbf, 0, sizeof(ITUArbitraryFill));

    ituIconInit(&arbf->icon);

    ituWidgetSetType(arbf, ITU_ARBITRARYFILL);
    ituWidgetSetName(arbf, arbitraryfillName);
    ituWidgetSetClone(arbf, ituArbitraryFillClone);
    ituWidgetSetDraw(arbf, ituArbitraryFillDraw);
}

void ituArbitraryFillLoad(ITUArbitraryFill* arbf, uint32_t base)
{
    const ITUWidget* widget = (ITUWidget*)arbf;
    assert(arbf);

    ituIconLoad(&arbf->icon, base);

    if (!(widget->flags & ITU_EXTERNAL))
    {
        if (arbf->staticPathSurf)
        {
            ITUSurface* surf = (ITUSurface*)(base + (uint32_t)arbf->staticPathSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                arbf->staticPathSurf = NULL;
            }
            else
            {
                arbf->staticPathSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                    (const uint8_t*)surf->addr, surf->flags);
            }

            arbf->staticPathSurf = surf;
        }
    }

    ituWidgetSetClone(arbf, ituArbitraryFillClone);
    ituWidgetSetDraw(arbf, ituArbitraryFillDraw);
}

ITUSurface* ituArbitraryFillLoadImg(ITUArbitraryFill* arbf, ITUSurface** surf, ITUSurface* staticSurf)
{
    ITUWidget* widget = (ITUWidget*)arbf;
    ITUSurface* surfSrc = NULL;
    ITUSurface* result = (surf == NULL) ? (NULL) : (*surf);
    assert(arbf);
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
        ITULayer* layer = ituGetLayer(widget);
        surfSrc = ituLayerLoadExternalSurface(layer, (uint32_t)staticSurf);
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
                (const uint8_t*)surfSrc->addr, surfSrc->flags);
        }
    }

    return result;
}

static void ituArbitraryFillMixMask(ITUSurface* destSurf, ITUSurface* maskSurf)
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

            for (yy = 0; yy < maskSurf->height - 1; yy++)
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

void ituArbitraryFillFF(ITUWidget* widget, int addStep)
{
    ITUArbitraryFill* arbf = (ITUArbitraryFill*)widget;
    ITUIcon* icon = &arbf->icon;
    ITUColor emptyColor = { 0,0,0,0 };
    int vFrom = arbf->current;
    int vTo = arbf->current + addStep;

    if (arbf == NULL)
    {
        return;
    }
    else if ((icon == NULL) || (arbf->surfPath == NULL))
    {
        return;
    }
    else
    {
        if ((arbf->value < 0) || (arbf->value >= 100) || (addStep <= 0))
        {
            return;
        }
    }

    if (arbf->srcTmp == NULL)
    {
        arbf->srcTmp = ituCreateSurface(arbf->surfPath->width, arbf->surfPath->height, 0
            , arbf->surfPath->format, 0, 0);
        if (arbf->srcTmp == NULL)
        {
            return;
        }
        ituColorFill(arbf->srcTmp, 0, 0, arbf->srcTmp->width, arbf->srcTmp->height, &emptyColor);
        ituAlphaBlend(arbf->srcTmp, 0, 0, arbf->surfPath->width, arbf->surfPath->height, arbf->surfPath, 0, 0, 255);
    }

    if (arbf->destTmp == NULL)
    {
        arbf->destTmp = ituCreateSurface(icon->surf->width, icon->surf->height, 0
            , icon->surf->format, 0, 0);
        if (arbf->destTmp == NULL)
        {
            return;
        }
        ituColorFill(arbf->destTmp, 0, 0, arbf->srcTmp->width, arbf->srcTmp->height, &emptyColor);
        ituAlphaBlend(arbf->destTmp, 0, 0, icon->surf->width, icon->surf->height, icon->surf, 0, 0, 255);
    }
    else
    {
        ituAlphaBlend(arbf->destTmp, 0, 0, icon->surf->width, icon->surf->height, icon->surf, 0, 0, 255);
    }

    if (arbf->destTmp != NULL)
    {
        int vId = (vFrom * arbf->count) / 100;
        int vTo = vFrom + addStep;
        if (vTo > 100)
        {
            vTo = 100;
        }
        arbf->current = vTo;

        vTo = (vTo * arbf->count) / 100;

        if (addStep > 0)
        {
            ITUPoint p1 = { arbf->pathAX[vId],arbf->pathAY[vId] };
            ITUPoint p2 = { arbf->pathAX[vTo],arbf->pathAY[vTo] };
            ITUPoint p3 = { arbf->pathBX[vTo],arbf->pathBY[vTo] };
            ITUPoint p4 = { arbf->pathBX[vId],arbf->pathBY[vId] };
#ifdef CFG_M2D_ENABLE
            int minX = ITH_MIN(ITH_MIN(ITH_MIN(p1.x, p2.x), p3.x), p4.x);
            int minY = ITH_MIN(ITH_MIN(ITH_MIN(p1.y, p2.y), p3.y), p4.y);
            int maxX = ITH_MAX(ITH_MAX(ITH_MAX(p1.x, p2.x), p3.x), p4.x);
            int maxY = ITH_MAX(ITH_MAX(ITH_MAX(p1.y, p2.y), p3.y), p4.y);
            ITU_RECTANGLE rectSrc = {
                (float)p1.x, (float)p1.y, (float)p3.x, (float)p3.y,
                (float)p4.x, (float)p4.y, (float)p2.x, (float)p2.y
            };
            ituColorFillWithShearing(arbf->srcTmp, minX, minY, (maxX - minX), (maxY - minY), rectSrc, &emptyColor);
#else
            ituArbitrarySWFill(arbf->srcTmp, &p1, &p2, &p3, &p4, &emptyColor);
#endif
        }
    }
}

void ituArbitraryFillDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx = 0, desty = 0;
    uint8_t desta = 0;
    ITURectangle prevClip = { 0, 0, 0, 0 };
    ITUArbitraryFill* arbf = (ITUArbitraryFill*)widget;
    ITUIcon* icon = &arbf->icon;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    ITUColor emptyColor = { 0,0,0,0 };
    assert(arbf);
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
        if (widget->color.alpha > 0)
        {
            ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
        }

        arbf->surfPath = ituArbitraryFillLoadImg(arbf, &arbf->surfPath, arbf->staticPathSurf);

        icon->surf = ituArbitraryFillLoadImg(arbf, &icon->surf, icon->staticSurf);
        // Check source for debug
        /*
        if (icon->surf != NULL)
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
        */

        while (arbf->current < arbf->value)//(arbf->value > 0)//use mask mode for now
        {
            ituArbitraryFillFF(widget, arbf->fillstep);
            //test code
            //arbf->value = 0;
            //ituArbitraryFillFF(widget, 6);
            //ituArbitraryFillMixMask(arbf->destTmp, arbf->srcTmp);
            
            if (arbf->current > arbf->value)
                arbf->current = arbf->value;
        }

        ituArbitraryFillMixMask(arbf->destTmp, arbf->srcTmp);

        if (arbf->destTmp != NULL)
        {
            if (desta == 255)
            {
                ituBitBlt(dest, destx, desty, arbf->destTmp->width, arbf->destTmp->height, arbf->destTmp, 0, 0);
            }
            else
            {
                ituAlphaBlend(dest, destx, desty, arbf->destTmp->width, arbf->destTmp->height, arbf->destTmp, 0, 0, desta);
            }
        }
    }
}
