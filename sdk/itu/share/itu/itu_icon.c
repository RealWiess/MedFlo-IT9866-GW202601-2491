#include <assert.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#include "ite/itp.h"
#ifdef CFG_BUILD_M2D
    #include "gfx/gfx.h"
#endif

static const char iconName[] = "ITUIcon";

#pragma warning(disable : 4028)

#ifndef M2dSurface
typedef struct
{
    ITUSurface surf;
    #ifdef CFG_M2D_ENABLE
    GFXSurface * m2dSurf;
    #endif
} M2dSurface;
#endif

void ituIconExit (ITUWidget * widget)
{
    ITUIcon * icon = (ITUIcon *)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (icon->filePath)
    {
        free(icon->filePath);
        icon->filePath = NULL;
    }

    if (widget->flags & ITU_LOADED)
    {
        if (icon->loadedSurf)
        {
            ituSurfaceRelease(icon->loadedSurf);
            icon->loadedSurf = NULL;
        }
        widget->flags &= ~ITU_LOADED;
    }

    if (icon->surf)
    {
        ituSurfaceRelease(icon->surf);
        icon->surf = NULL;
    }

    if (icon->iconflags & ITUICON_CORNER_MASK_LINKING)
    {
        icon->maskTL = NULL;
        icon->maskTR = NULL;
        icon->maskBL = NULL;
        icon->maskBR = NULL;
    }
    else
    {
        if (icon->maskTL)
        {
            ituSurfaceRelease(icon->maskTL);
            icon->maskTL = NULL;
        }
        if (icon->maskTR)
        {
            ituSurfaceRelease(icon->maskTR);
            icon->maskTR = NULL;
        }
        if (icon->maskBL)
        {
            ituSurfaceRelease(icon->maskBL);
            icon->maskBL = NULL;
        }
        if (icon->maskBR)
        {
            ituSurfaceRelease(icon->maskBR);
            icon->maskBR = NULL;
        }
    }

    if (icon->iconflags & ITUICON_CORNER_MASK_ACTIVE)
    {
        icon->iconflags &= ~ITUICON_CORNER_MASK_ACTIVE;
    }

    ituWidgetExitImpl(widget);
}

bool ituIconClone (ITUWidget * widget, ITUWidget ** cloned)
{
    ITUIcon * icon = (ITUIcon *)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUIcon));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUIcon));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    if (!(icon->widget.flags & ITU_EXTERNAL) && icon->staticSurf)
    {
        ITUIcon *    newIcon = (ITUIcon *)*cloned;
        ITUSurface * surf    = icon->staticSurf;
        if (surf->flags & ITU_COMPRESSED)
        {
            newIcon->surf = NULL;
        }
        else
        {
            newIcon->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                             (const uint8_t *)surf->addr, surf->flags);
        }

        ituWidgetUpdate(newIcon, ITU_EVENT_LOAD, 0, 0, 0);
    }
    return ituWidgetCloneImpl(widget, cloned);
}

static void IconLoadExternalData (ITUIcon * icon, ITULayer * layer) //
{
    ITUWidget *  widget = (ITUWidget *)icon;
    ITUSurface * surf;
    ITUSurface * surfTL;
    ITUSurface * surfTR;
    ITUSurface * surfBL;
    ITUSurface * surfBR;
    bool         drawReq = ituSceneClearD2D(ituScene);

    assert(widget);

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (!icon->staticSurf || !(widget->flags & ITU_EXTERNAL))
    {
        return;
    }
#else
    if (!icon->staticSurf || icon->surf || !(widget->flags & ITU_EXTERNAL))
    {
        return;
    }
#endif

    if (!layer)
    {
        layer = ituGetLayer(widget);
    }

    surf = ituLayerLoadExternalSurface(layer, (uint32_t)icon->staticSurf);
    if (surf)
    {
        if (surf->flags & ITU_COMPRESSED)
        {
            ituSceneCheckD2D(ituScene, drawReq);
            icon->surf = ituSurfaceDecompress(surf);
        }
        else
        {
            icon->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                          (const uint8_t *)surf->addr, surf->flags);
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    // corner mask not support for dcps in place mode.
    return;
#endif

    // load corner mask surface
    if (icon->staticMaskTL && !icon->maskTL)
    {
        surfTL = ituLayerLoadExternalSurface(layer, (uint32_t)icon->staticMaskTL);
        if (surfTL)
        {
            if (surfTL->flags & ITU_COMPRESSED)
            {
                icon->maskTL = ituSurfaceDecompress(surfTL);
            }
            else
            {
                icon->maskTL = ituCreateSurface(surfTL->width, surfTL->height, surfTL->pitch, surfTL->format,
                                                (const uint8_t *)surfTL->addr, surfTL->flags);
            }
        }
    }
    if (icon->staticMaskTR && !icon->maskTR)
    {
        surfTR = ituLayerLoadExternalSurface(layer, (uint32_t)icon->staticMaskTR);
        if (surfTR)
        {
            if (surfTR->flags & ITU_COMPRESSED)
            {
                icon->maskTR = ituSurfaceDecompress(surfTR);
            }
            else
            {
                icon->maskTR = ituCreateSurface(surfTR->width, surfTR->height, surfTR->pitch, surfTR->format,
                                                (const uint8_t *)surfTR->addr, surfTR->flags);
            }
        }
    }
    if (icon->staticMaskBL && !icon->maskBL)
    {
        surfBL = ituLayerLoadExternalSurface(layer, (uint32_t)icon->staticMaskBL);
        if (surfBL)
        {
            if (surfBL->flags & ITU_COMPRESSED)
            {
                icon->maskBL = ituSurfaceDecompress(surfBL);
            }
            else
            {
                icon->maskBL = ituCreateSurface(surfBL->width, surfBL->height, surfBL->pitch, surfBL->format,
                                                (const uint8_t *)surfBL->addr, surfBL->flags);
            }
        }
    }
    if (icon->staticMaskBR && !icon->maskBR)
    {
        surfBR = ituLayerLoadExternalSurface(layer, (uint32_t)icon->staticMaskBR);
        if (surfBR)
        {
            if (surfBR->flags & ITU_COMPRESSED)
            {
                icon->maskBR = ituSurfaceDecompress(surfBR);
            }
            else
            {
                icon->maskBR = ituCreateSurface(surfBR->width, surfBR->height, surfBR->pitch, surfBR->format,
                                                (const uint8_t *)surfBR->addr, surfBR->flags);
            }
        }
    }
}

void ituIconLoadExternalDataAPI (ITUIcon * icon)
{
    IconLoadExternalData(icon, NULL);
}

static void IconLoadImage (ITUIcon * icon, char * path)
{
    ITUWidget *    widget = (ITUWidget *)icon;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf   = NULL;
    char           filepath[PATH_MAX];
    char *         ptr;

    assert(widget);

    if ((!(widget->flags & ITU_EXTERNAL_IMAGE)) || (icon->surf != NULL))
    {
        return;
    }

    //workaround for running external image mode under 983X
    if (path == NULL)
    {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        char* lastPath = (char*)ituWidgetGetCustomData(widget);
        if (lastPath != NULL)
        {
            strlcpy(filepath, lastPath, sizeof(filepath));
        }
#endif
    }
    else
    {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        char* pathBackup = (char*)strdup(path);
        char* lastPath = (char*)ituWidgetGetCustomData(widget);
        if (lastPath != NULL)
        {
            free(lastPath);
        }
        ituWidgetSetCustomData(widget, pathBackup);
#endif
        strlcpy(filepath, path, sizeof(filepath));
    }

    strlcat(filepath, widget->name, sizeof(filepath));

    ptr = strrchr(filepath, '.');
    if (ptr)
    {
        ptr++;
        if (stricmp(ptr, "jpg") == 0)
        {
            if (widget->flags & ITU_FIT_TO_RECT)
            {
                surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
            }
            else if (widget->flags & ITU_CUT_BY_RECT)
            {
                surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
            }
            else
            {
                surf = ituJpegLoadFile(0, 0, filepath, 0);
            }
        }
        else if (stricmp(ptr, "png") == 0)
        {
            if (widget->flags & ITU_FIT_TO_RECT)
            {
                surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
            }
            else if (widget->flags & ITU_CUT_BY_RECT)
            {
                surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
            }
            else
            {
                surf = ituPngLoadFile(0, 0, filepath, 0);
            }
        }
    }

    if (surf)
    {
        //workaround for running external image mode under 983X
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        icon->surf = surf;
#else
        ituIconReleaseSurface(icon);
        icon->surf = surf;
#endif
    }
}

bool ituIconUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool      result = false;
    ITUIcon * icon   = (ITUIcon *)widget;
    assert(icon);

    if (ev == ITU_EVENT_LOAD)
    {
        if (widget->flags & ITU_EXTERNAL)
        {
            IconLoadExternalData(icon, NULL);
        }
        else
        {
            if ((ITU_USE_SPRITE_COMMON_BUF > 0) && (arg1 == ITU_USE_SPRITE_COMMON_BUF))
            {
                if (icon->surf)
                {
                    icon->surf = NULL;
                }
                icon->staticSurf->flags |= ITU_SPRITE_BUF;
            }
            ituIconLoadStaticData(icon);
        }

        result = true;
    }
    else if (ev == ITU_EVENT_LOAD_EXTERNAL)
    {
        IconLoadExternalData(icon, (ITULayer *)arg1);
        result = true;
    }
    else if (ev == ITU_EVENT_LOAD_IMAGE)
    {
        IconLoadImage(icon, (char *)arg1);
        result = true;
    }
    else if (ev == ITU_EVENT_RELEASE)
    {
        ituIconReleaseSurface(icon);
        result = true;
    }
    else if (ev == ITU_EVENT_TIMER)
    {
        if (widget->flags & ITU_LOADED)
        {
            if (icon->filePath)
            {
                free(icon->filePath);
                icon->filePath = NULL;
            }

            if (icon->loadedSurf)
            {
                if (icon->surf)
                {
                    ituSurfaceRelease(icon->surf);
                }

                icon->surf       = icon->loadedSurf;
                icon->loadedSurf = NULL;
            }
            widget->flags &= ~ITU_LOADED;
            result = true;
        }
    }
    result |= ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);
    return result;
}

void ituIconDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int            destx, desty;
    uint8_t        desta;
    ITURectangle   prevClip;
    ITUIcon *      icon = (ITUIcon *)widget;
    ITURectangle * rect = &widget->rect;
    assert(icon);
    assert(dest);

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL) //
    {
        (void)printf("[SurfacePool] IconDraw but SurfacePool not ready yet!\n");
        return;
    }

    if (widget->visible && (widget->alpha > 0) && (alpha > 0))
    {
        ituSceneSetD2D(ituScene, widget);
        if (widget->flags & ITU_EXTERNAL)
        {
            IconLoadExternalData(icon, NULL);
        }
        else
        {
            ituIconLoadStaticData(icon);
        }
    }
#endif

    if (icon->surf == NULL)
    {
        ituWidgetSetDirty(widget, false);
        return;
    }
    else if (icon->maskTL || icon->maskTR || icon->maskBL || icon->maskBR)
    {
        if (!(icon->iconflags & ITUICON_CORNER_MASK_ACTIVE))
        {
            if (icon->staticSurf != NULL)
            {
                if ((icon->staticSurf->cornerMaskScale.topLeft
                    * icon->staticSurf->cornerMaskScale.topRight
                    * icon->staticSurf->cornerMaskScale.bottomLeft
                    * icon->staticSurf->cornerMaskScale.bottomRight) <= 0)
                {
                    icon->staticSurf->cornerMaskScale.topLeft = 1.0f;
                    icon->staticSurf->cornerMaskScale.topRight = 1.0f;
                    icon->staticSurf->cornerMaskScale.bottomLeft = 1.0f;
                    icon->staticSurf->cornerMaskScale.bottomRight = 1.0f;
                    icon->staticSurf->cornerMaskScale.dxTL = icon->staticSurf->cornerMaskScale.dyTL = 0;
                    icon->staticSurf->cornerMaskScale.dxTR = icon->staticSurf->cornerMaskScale.dyTR = 0;
                    icon->staticSurf->cornerMaskScale.dxBL = icon->staticSurf->cornerMaskScale.dyBL = 0;
                    icon->staticSurf->cornerMaskScale.dxBR = icon->staticSurf->cornerMaskScale.dyBR = 0;
                }
                icon->surf->cornerMaskScale = icon->staticSurf->cornerMaskScale;
            }
            else
            {
                icon->surf->cornerMaskScale.topLeft = 1.0f;
                icon->surf->cornerMaskScale.topRight = 1.0f;
                icon->surf->cornerMaskScale.bottomLeft = 1.0f;
                icon->surf->cornerMaskScale.bottomRight = 1.0f;
                icon->surf->cornerMaskScale.dxTL = icon->surf->cornerMaskScale.dyTL = 0;
                icon->surf->cornerMaskScale.dxTR = icon->surf->cornerMaskScale.dyTR = 0;
                icon->surf->cornerMaskScale.dxBL = icon->surf->cornerMaskScale.dyBL = 0;
                icon->surf->cornerMaskScale.dxBR = icon->surf->cornerMaskScale.dyBR = 0;
            }

            if (!(icon->surf->flags & ITU_MASKED))
            {
                //default use cached decompressed data when use same static image
                //will make all related(same static surf) icon all corner mask marked
                //ituCornerRender(widget, NULL, NULL, NULL, NULL, NULL);

                //workaround below but/with performance reducing
                ITUSurface* cloneSurf = ituCreateSurface(icon->surf->width, icon->surf->height, 0, icon->surf->format, NULL, 0);
                ITUColor emptyColor = { 0,0,0,0 };
                ituColorFill(cloneSurf, 0, 0, cloneSurf->width, cloneSurf->height, &emptyColor);
                ituBitBlt(cloneSurf, 0, 0, icon->surf->width, icon->surf->height, icon->surf, 0, 0);
                cloneSurf->cornerMaskScale = icon->staticSurf->cornerMaskScale;
                ituCornerRender(NULL, cloneSurf, icon->maskTL, icon->maskTR, icon->maskBL, icon->maskBR);
                ituIconReleaseSurface(icon);
                icon->surf = cloneSurf;
            }
        }
    }
    else
    {
        //nothing
    }

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->color.alpha / 255;
    desta = desta * widget->alpha / 255;

    if (widget->angle % 90 == 0)
    {
        ituWidgetSetClipping(widget, dest, x, y, &prevClip);
    }

    if (desta > 0)
    {
        if ((icon->use_rotate_mask > 0) && (widget->flags & ITU_STRETCH) && (widget->angle == 0) &&
            (widget->transformType == ITU_TRANSFORM_NONE))
        {
            int    ow               = icon->surf->width;
            int    oh               = icon->surf->height;
            int    w                = ((ow % 2) ? (ow + 1) : (ow));
            int    h                = ((oh % 2) ? (oh + 1) : (oh));
            int    w2               = w / 2;
            int    h2               = h / 2;
            double pi               = 0.0174532925;

            bool   impossible_angle = false;

            int    v_angle1         = icon->start_rm_angle;
            int    v_angle2         = icon->end_rm_angle;
            int    loopCount        = 1; // default 1

            if ((v_angle2 <= v_angle1) && (v_angle1 > 0) && (v_angle2 > 0))
            {
                impossible_angle = true;
            }
            else if ((v_angle2 >= v_angle1) && (v_angle1 < 0) && (v_angle2 < 0))
            {
                impossible_angle = true;
            }
            else
            {
                if (v_angle1 < 0)
                {
                    v_angle1 += 360;
                }
                if (v_angle2 <= 0)
                {
                    v_angle2 += 360;
                }
                if (v_angle1 > 360)
                {
                    v_angle1 -= 360;
                }
                if (v_angle2 > 360)
                {
                    v_angle2 -= 360;
                }
                if (v_angle2 < v_angle1)
                {
                    loopCount++;
                }
            }

            // rotate range judgement
            if (impossible_angle)
            {
                (void)printf("[Icon %s]wrong rotate mask angle setting!\n", widget->name);
            }
            else
            {
                ITUSurface * S1              = ituCreateSurface(w2, h2, 0, icon->surf->format, NULL, 0);
                ITUSurface * S2              = ituCreateSurface(w2, h2, 0, icon->surf->format, NULL, 0);
                ITUSurface * S3              = ituCreateSurface(w2, h2, 0, icon->surf->format, NULL, 0);
                ITUSurface * S4              = ituCreateSurface(w2, h2, 0, icon->surf->format, NULL, 0);
                ITUSurface * SF              = ituCreateSurface(w, h, 0, icon->surf->format, NULL, 0);
                bool         quadFourDone[5] = {(loopCount > 1) ? (true) : (false), false, false, false, false};
                ITUColor     emptycolor      = {0, 0, 0, 0};

                ituColorFill(SF, 0, 0, w, h, &emptycolor);
                ituBitBlt(SF, 0, 0, w, h, dest, destx, desty);
                bool looped = false;
                while (loopCount > 0)
                {
                    v_angle1 = icon->start_rm_angle;
                    v_angle2 = icon->end_rm_angle;
                    if (v_angle1 < 0)
                    {
                        v_angle1 += 360;
                    }
                    if (v_angle2 <= 0)
                    {
                        v_angle2 += 360;
                    }
                    if (v_angle1 > 360)
                    {
                        v_angle1 -= 360;
                    }
                    if (v_angle2 > 360)
                    {
                        v_angle2 -= 360;
                    }

                    loopCount--;

                    if (v_angle2 < v_angle1)
                    {
                        if (loopCount > 0)
                        {
                            v_angle2 = 360;
                        }
                        else
                        {
                            v_angle1 = 0;
                        }
                    }

                    // for quadrant 1
                    if (!quadFourDone[1] || looped)
                    {
                        if ((v_angle1 >= 90) && (v_angle2 <= 360))
                        {
                            ituColorFill(S1, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S1, 0, 0, w2, h2, dest, destx + w2, desty);
                            ituBitBlt(S1, 0, 0, w2, h2, icon->surf, w2, 0);
                        }
                        else if ((v_angle1 == 0) && (v_angle2 >= 90))
                        {
                            ituColorFill(S1, 0, 0, w2, h2, &emptycolor);
                            quadFourDone[1] = true;
                        }
                        else if ((v_angle1 < 90) || (v_angle2 < 90))
                        {
                            int    i, j;
                            double a1 = ((v_angle1 == 0) ? (0.0) : (tan(pi * v_angle1)));
                            double a2 = ((v_angle2 >= 90) ? (0.0) : (tan(pi * v_angle2)));
                            ituColorFill(S1, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S1, 0, 0, w2, h2, dest, destx + w2, desty);
                            ituBitBlt(S1, 0, 0, w2, h2, icon->surf, w2, 0);
                            for (j = 0; j < h2; j++)
                            {
                                int dline   = 0;
                                int start_i = -1;
                                for (i = 0; i < w2; i++)
                                {
                                    double dy = h2 - j;
                                    double dx = i;
                                    double da = dx / dy;

                                    if (((da <= a2) || (a2 == 0.0)) && (da >= a1))
                                    {
                                        dline++;
                                        if (start_i < 0)
                                        {
                                            start_i = i;
                                        }
                                    }
                                }
                                if (dline)
                                {
                                    ituColorFill(S1, start_i, j, dline, 1, &emptycolor);
                                }
                            }
                            quadFourDone[1] = true;
                            if (quadFourDone[0] && v_angle2 <= 90)
                            {
                                continue;
                            }
                        }
                    }

                    // for quadrant 2
                    if (!quadFourDone[2] || looped)
                    {
                        // for double check
                        if ((v_angle2 <= 90) && looped)
                        {
                            loopCount--;
                            continue;
                        }

                        if ((v_angle1 >= 180) || (v_angle2 <= 90))
                        {
                            ituColorFill(S2, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S2, 0, 0, w2, h2, dest, destx + w2, desty + h2);
                            ituBitBlt(S2, 0, 0, w2, h2, icon->surf, w2, h2);
                        }
                        else if ((v_angle1 <= 90) && (v_angle2 >= 180))
                        {
                            ituColorFill(S2, 0, 0, w2, h2, &emptycolor);
                            quadFourDone[2] = true;
                        }
                        else if ((v_angle1 > 90 && v_angle1 < 180) || (v_angle2 > 90 && v_angle2 < 180))
                        {
                            int    i, j;
                            double a1 = ((v_angle1 <= 90) ? (0.0) : (tan(pi * (v_angle1 - 90))));
                            double a2 = ((v_angle2 >= 180) ? (0.0) : (tan(pi * (v_angle2 - 90))));
                            ituColorFill(S2, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S2, 0, 0, w2, h2, dest, destx + w2, desty + h2);
                            ituBitBlt(S2, 0, 0, w2, h2, icon->surf, w2, h2);
                            for (j = 0; j < h2; j++)
                            {
                                int dline   = 0;
                                int start_i = -1;
                                for (i = 0; i < w2; i++)
                                {
                                    double dy = i;
                                    double dx = j;
                                    double da = dx / dy;

                                    if (((da <= a2) || (a2 == 0.0)) && (da >= a1))
                                    {
                                        dline++;
                                        if (start_i < 0)
                                        {
                                            start_i = i;
                                        }
                                    }
                                }
                                if (dline)
                                {
                                    ituColorFill(S2, start_i, j, dline, 1, &emptycolor);
                                }
                            }
                            quadFourDone[2] = true;
                            if (quadFourDone[0] && v_angle2 <= 180)
                            {
                                continue;
                            }
                        }
                    }

                    // for quadrant 3
                    if (!quadFourDone[3] || looped)
                    {
                        // for double check
                        if ((v_angle2 <= 180) && looped)
                        {
                            loopCount--;
                            continue;
                        }

                        if ((v_angle1 >= 270) || (v_angle2 <= 180))
                        {
                            ituColorFill(S3, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S3, 0, 0, w2, h2, dest, destx, desty + h2);
                            ituBitBlt(S3, 0, 0, w2, h2, icon->surf, 0, h2);
                        }
                        else if ((v_angle1 <= 180) && (v_angle2 >= 270))
                        {
                            ituColorFill(S3, 0, 0, w2, h2, &emptycolor);
                            quadFourDone[3] = true;
                        }
                        else if ((v_angle1 > 180 && v_angle1 < 270) || (v_angle2 > 180 && v_angle2 < 270))
                        {
                            int    i, j;
                            double a1 = ((v_angle1 <= 180) ? (0.0) : (tan(pi * (v_angle1 - 180))));
                            double a2 = ((v_angle2 >= 270) ? (0.0) : (tan(pi * (v_angle2 - 180))));
                            ituColorFill(S3, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S3, 0, 0, w2, h2, dest, destx, desty + h2);
                            ituBitBlt(S3, 0, 0, w2, h2, icon->surf, 0, h2);
                            for (j = 0; j < h2; j++)
                            {
                                int dline   = 0;
                                int start_i = -1;
                                for (i = 0; i < w2; i++)
                                {
                                    double dy = j;
                                    double dx = w2 - i;
                                    double da = dx / dy;

                                    if (((da <= a2) || (a2 == 0.0)) && (da >= a1))
                                    {
                                        dline++;
                                        if (start_i < 0)
                                        {
                                            start_i = i;
                                        }
                                    }
                                }
                                if (dline)
                                {
                                    ituColorFill(S3, start_i, j, dline, 1, &emptycolor);
                                }
                            }
                            quadFourDone[3] = true;
                            if (quadFourDone[0] && v_angle2 <= 270)
                            {
                                continue;
                            }
                        }
                    }

                    // for quadrant 4
                    if (!quadFourDone[4] || looped)
                    {
                        // for double check
                        if ((v_angle2 <= 270) && looped)
                        {
                            loopCount--;
                            continue;
                        }

                        if (v_angle1 >= 0 && v_angle2 <= 270)
                        {
                            ituColorFill(S4, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S4, 0, 0, w2, h2, dest, destx, desty);
                            ituBitBlt(S4, 0, 0, w2, h2, icon->surf, 0, 0);
                        }
                        else if ((v_angle1 <= 270) && (v_angle2 >= 360))
                        {
                            ituColorFill(S4, 0, 0, w2, h2, &emptycolor);
                            quadFourDone[4] = true;
                        }
                        else if ((v_angle1 > 270 && v_angle1 < 360) || (v_angle2 > 270 && v_angle2 < 360))
                        {
                            int    i, j;
                            double a1 = ((v_angle1 <= 270) ? (0.0) : (tan(pi * (v_angle1 - 270))));
                            double a2 = ((v_angle2 >= 360) ? (0.0) : (tan(pi * (v_angle2 - 270))));
                            ituColorFill(S4, 0, 0, w2, h2, &emptycolor);
                            ituBitBlt(S4, 0, 0, w2, h2, dest, destx, desty);
                            ituBitBlt(S4, 0, 0, w2, h2, icon->surf, 0, 0);
                            for (j = 0; j < h2; j++)
                            {
                                int dline   = 0;
                                int start_i = -1;
                                for (i = 0; i < w2; i++)
                                {
                                    double dy = w2 - i;
                                    double dx = h2 - j;
                                    double da = dx / dy;

                                    if (((da <= a2) || (a2 == 0.0)) && (da >= a1))
                                    {
                                        dline++;
                                        if (start_i < 0)
                                        {
                                            start_i = i;
                                        }
                                    }
                                }
                                if (dline)
                                {
                                    ituColorFill(S4, start_i, j, dline, 1, &emptycolor);
                                }
                            }
                            quadFourDone[4] = true;
                        }
                    }

                    looped = true;
                }

                ituAlphaBlend(SF, w2, 0, w2, h2, S1, 0, 0, desta);
                ituAlphaBlend(SF, w2, h2, w2, h2, S2, 0, 0, desta);
                ituAlphaBlend(SF, 0, h2, w2, h2, S3, 0, 0, desta);
                ituAlphaBlend(SF, 0, 0, w2, h2, S4, 0, 0, desta);

                ituStretchBlt(dest, destx, desty, rect->width, rect->height, SF, 0, 0, SF->width, SF->height);

                ituDestroySurface(S1);
                ituDestroySurface(S2);
                ituDestroySurface(S3);
                ituDestroySurface(S4);
                ituDestroySurface(SF);
            }
        }
        else if (desta == 255)
        {
            if (widget->flags & ITU_STRETCH)
            {
#ifndef CFG_M2D_ENABLE
                if (widget->angle == 0)
                {
                    if (widget->transformType == ITU_TRANSFORM_NONE)
                    {
                        ituStretchBlt(dest, destx, desty, rect->width, rect->height, icon->surf, 0, 0,
                                      icon->surf->width, icon->surf->height);
                    }
                    else
                    {
                        ITUSurface * surf =
                            ituCreateSurface(icon->surf->width, icon->surf->height, 0, dest->format, NULL, 0);
                        if (surf)
                        {
                            int w = (icon->surf->width - icon->surf->width * widget->transformX / 100) / 2;
                            int h = (icon->surf->height - icon->surf->height * widget->transformY / 100) / 2;

                            ituStretchBlt(surf, 0, 0, icon->surf->width, icon->surf->height, dest, destx, desty,
                                          rect->width, rect->height);

                            switch (widget->transformType)
                            {
                                case ITU_TRANSFORM_TURN_LEFT:
                                    ituTransformBlt(surf, 0, 0, icon->surf, 0, 0, icon->surf->width, icon->surf->height,
                                                    w, h, icon->surf->width - w, 0, icon->surf->width - w,
                                                    icon->surf->height, w, icon->surf->height - h, true,
                                                    ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_TOP:
                                    ituTransformBlt(surf, 0, 0, icon->surf, 0, 0, icon->surf->width, icon->surf->height,
                                                    w, h, icon->surf->width - w, h, icon->surf->width,
                                                    icon->surf->height - h, 0, icon->surf->height - h, true,
                                                    ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_RIGHT:
                                    ituTransformBlt(surf, 0, 0, icon->surf, 0, 0, icon->surf->width, icon->surf->height,
                                                    w, 0, icon->surf->width - w, h, icon->surf->width - w,
                                                    icon->surf->height - h, w, icon->surf->height, false,
                                                    ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_BOTTOM:
                                    ituTransformBlt(surf, 0, 0, icon->surf, 0, 0, icon->surf->width, icon->surf->height,
                                                    0, h, icon->surf->width, h, icon->surf->width - w,
                                                    icon->surf->height - h, w, icon->surf->height - h, false,
                                                    ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;
                            }
                            ituStretchBlt(dest, destx, desty, rect->width, rect->height, surf, 0, 0, surf->width,
                                          surf->height);
                            ituDestroySurface(surf);
                        }
                    }
                }
                else
                {
                    float scaleX, scaleY;
                    if (widget->angle == 90 || widget->angle == 270)
                    {
                        scaleX = (float)rect->width / icon->surf->height;
                        scaleY = (float)rect->height / icon->surf->width;
                    }
                    else
                    {
                        scaleX = (float)rect->width / icon->surf->width;
                        scaleY = (float)rect->height / icon->surf->height;
                    }
                    ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, icon->surf,
                              icon->surf->width / 2, icon->surf->height / 2, (float)widget->angle, scaleX, scaleY);
                }
#else
                float scaleX = (float)rect->width / icon->surf->width;
                float scaleY = (float)rect->height / icon->surf->height;

                ituTransform(dest, destx, desty, rect->width, rect->height, icon->surf, 0, 0, icon->surf->width,
                             icon->surf->height, icon->surf->width / 2, icon->surf->height / 2, scaleX, scaleY,
                             (float)widget->angle, 0, true, true, desta);
#endif
            }
            else
            {
                if (widget->angle == 0)
                {
                    if (widget->transformType == ITU_TRANSFORM_NONE)
                    {
                        ituBitBlt(dest, destx, desty, rect->width, rect->height, icon->surf, 0, 0);
                    }
                    else
                    {
                        int w = (rect->width - rect->width * widget->transformX / 100) / 2;
                        int h = (rect->height - rect->height * widget->transformY / 100) / 2;

                        switch (widget->transformType)
                        {
                            case ITU_TRANSFORM_TURN_LEFT:
                                ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, rect->width, rect->height, w, h,
                                                rect->width - w, 0, rect->width - w, rect->height, w, rect->height - h,
                                                true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                break;

                            case ITU_TRANSFORM_TURN_TOP:
                                ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, rect->width, rect->height, w, h,
                                                rect->width - w, h, rect->width, rect->height - h, 0, rect->height - h,
                                                true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                break;

                            case ITU_TRANSFORM_TURN_RIGHT:
                                ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, rect->width, rect->height, w, 0,
                                                rect->width - w, h, rect->width - w, rect->height - h, w, rect->height,
                                                false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                break;

                            case ITU_TRANSFORM_TURN_BOTTOM:
                                ituTransformBlt(dest, destx, desty, icon->surf, 0, 0, rect->width, rect->height, 0, h,
                                                rect->width, h, rect->width - w, rect->height - h, w, rect->height - h,
                                                false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                break;
                        }
                    }
                }
                else
                {
#ifndef CFG_M2D_ENABLE
                    ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, icon->surf,
                              icon->surf->width / 2, icon->surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
                    // workaround for rotate locksurface outside empty under rtos
                    if ((icon->surf->width != rect->width) || (icon->surf->height != rect->height))
                    {
                        int fitSide =
                            (icon->surf->width > icon->surf->height) ? (icon->surf->width) : (icon->surf->height);
                        ITUSurface * tmpSurf    = ituCreateSurface(fitSide, fitSide, 0, icon->surf->format, NULL, 0);
                        ITUColor     emptycolor = {0, 0, 0, 0};
                        ituColorFill(tmpSurf, 0, 0, fitSide, fitSide, &emptycolor);
                        ituBitBlt(tmpSurf, (fitSide - icon->surf->width) / 2, (fitSide - icon->surf->height) / 2,
                                  icon->surf->width, icon->surf->height, icon->surf, 0, 0);
                        ituRotate(dest, destx - (fitSide - rect->width) / 2, desty - (fitSide - rect->height) / 2,
                                  tmpSurf, tmpSurf->width / 2, tmpSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
                        ituSurfaceRelease(tmpSurf);
                    }
                    else
                    {
                        ituRotate(dest, destx, desty, icon->surf, icon->surf->width / 2, icon->surf->height / 2,
                                  (float)widget->angle, 1.0f, 1.0f);
                    }
#endif
                }
            }
        }
        else
        {
            if (widget->flags & ITU_STRETCH)
            {
#ifndef CFG_M2D_ENABLE
                ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);

                    if (widget->angle == 0)
                    {
                        ituStretchBlt(surf, 0, 0, rect->width, rect->height, icon->surf, 0, 0, icon->surf->width,
                                      icon->surf->height);
                    }
                    else
                    {
                        float scaleX = (float)rect->width / icon->surf->width;
                        float scaleY = (float)rect->height / icon->surf->height;

                        ituRotate(surf, rect->width / 2, rect->height / 2, icon->surf, icon->surf->width / 2,
                                  icon->surf->height / 2, (float)widget->angle, scaleX, scaleY);
                    }
                    ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                    ituDestroySurface(surf);
                }
#else
                float scaleX = (float)rect->width / icon->surf->width;
                float scaleY = (float)rect->height / icon->surf->height;

                ituTransform(dest, destx, desty, rect->width, rect->height, icon->surf, 0, 0, icon->surf->width,
                             icon->surf->height, icon->surf->width / 2, icon->surf->height / 2, scaleX, scaleY,
                             (float)widget->angle, 0, true, true, desta);
#endif
            }
            else
            {
                ituAlphaBlend(dest, destx, desty, rect->width, rect->height, icon->surf, 0, 0, desta);
            }
        }
    }

    if (widget->angle % 90 == 0)
    {
        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    }

    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}

void ituIconInit (ITUIcon * icon)
{
    assert(icon);
    ITU_ASSERT_THREAD();

    (void)memset(icon, 0, sizeof(ITUIcon));

    ituWidgetInit(&icon->widget);

    ituWidgetSetType(icon, ITU_ICON);
    ituWidgetSetName(icon, iconName);
    ituWidgetSetExit(icon, ituIconExit);
    ituWidgetSetClone(icon, ituIconClone);
    ituWidgetSetUpdate(icon, ituIconUpdate);
    ituWidgetSetDraw(icon, ituIconDraw);
}

void ituIconLoad (ITUIcon * icon, uint32_t base)
{
    assert(icon);

    ituWidgetLoad((ITUWidget *)icon, base);
    ituWidgetSetExit(icon, ituIconExit);
    ituWidgetSetClone(icon, ituIconClone);
    ituWidgetSetUpdate(icon, ituIconUpdate);
    ituWidgetSetDraw(icon, ituIconDraw);

    if (!(icon->widget.flags & ITU_EXTERNAL) && icon->staticSurf)
    {
        ITUSurface * surf = (ITUSurface *)(base + (uint32_t)icon->staticSurf);
        if (surf->flags & ITU_COMPRESSED)
        {
            icon->surf = NULL;
        }
        else
        {
            icon->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                          (const uint8_t *)surf->addr, surf->flags);
        }

        icon->staticSurf = surf;

        // for corner mask surface
        if (icon->staticMaskTL)
        {
            ITUSurface * surfTL = (ITUSurface *)(base + (uint32_t)icon->staticMaskTL);
            if (surfTL->flags & ITU_COMPRESSED)
            {
                icon->maskTL = NULL;
            }
            else
            {
                icon->maskTL = ituCreateSurface(surfTL->width, surfTL->height, surfTL->pitch, surfTL->format,
                                                (const uint8_t *)surfTL->addr, surfTL->flags);
            }
            icon->staticMaskTL = surfTL;
        }
        if (icon->staticMaskTR)
        {
            ITUSurface * surfTR = (ITUSurface *)(base + (uint32_t)icon->staticMaskTR);
            if (surfTR->flags & ITU_COMPRESSED)
            {
                icon->maskTR = NULL;
            }
            else
            {
                icon->maskTR = ituCreateSurface(surfTR->width, surfTR->height, surfTR->pitch, surfTR->format,
                                                (const uint8_t *)surfTR->addr, surfTR->flags);
            }
            icon->staticMaskTR = surfTR;
        }
        if (icon->staticMaskBL)
        {
            ITUSurface * surfBL = (ITUSurface *)(base + (uint32_t)icon->staticMaskBL);
            if (surfBL->flags & ITU_COMPRESSED)
            {
                icon->maskBL = NULL;
            }
            else
            {
                icon->maskBL = ituCreateSurface(surfBL->width, surfBL->height, surfBL->pitch, surfBL->format,
                                                (const uint8_t *)surfBL->addr, surfBL->flags);
            }
            icon->staticMaskBL = surfBL;
        }
        if (icon->staticMaskBR)
        {
            ITUSurface * surfBR = (ITUSurface *)(base + (uint32_t)icon->staticMaskBR);
            if (surfBR->flags & ITU_COMPRESSED)
            {
                icon->maskBR = NULL;
            }
            else
            {
                icon->maskBR = ituCreateSurface(surfBR->width, surfBR->height, surfBR->pitch, surfBR->format,
                                                (const uint8_t *)surfBR->addr, surfBR->flags);
            }
            icon->staticMaskBR = surfBR;
        }
    }
}

bool ituIconLoadJpegData (ITUIcon * icon, uint8_t * data, int size)
{
    ITUWidget *    widget = (ITUWidget *)icon;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf;
    bool           result = false;

    assert(icon);
    ITU_ASSERT_THREAD();

    if (widget->flags & ITU_LOADING)
    {
        widget->flags &= ~ITU_LOADING;
        icon->loadedSurf = NULL;
    }
    else if (widget->flags & ITU_LOADED)
    {
        if (icon->filePath)
        {
            free(icon->filePath);
            icon->filePath = NULL;
        }

        if (icon->loadedSurf)
        {
            ituSurfaceRelease(icon->loadedSurf);
            icon->loadedSurf = NULL;
        }
        widget->flags &= ~ITU_LOADED;
    }
    else
    {
        free(icon->filePath);
        icon->filePath = NULL;

        if (widget->flags & ITU_FIT_TO_RECT)
        {
            surf = ituJpegLoad(rect->width, rect->height, data, size, ITU_FIT_TO_RECT);
        }
        else if (widget->flags & ITU_CUT_BY_RECT)
        {
            surf = ituJpegLoad(rect->width, rect->height, data, size, ITU_CUT_BY_RECT);
        }
        else
        {
            surf = ituJpegLoad(0, 0, data, size, 0);
        }

        if (surf)
        {
            if (icon->surf)
            {
                ituSurfaceRelease(icon->surf);
            }

            icon->surf = surf;
            result     = true;
        }
        ituWidgetSetDirty(icon, true);
    }
    return result;
}

bool ituIconLoadJpegAlphaData (ITUIcon * icon, uint8_t * data, int size)
{
    ITUWidget *    widget = (ITUWidget *)icon;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf   = NULL;
    bool           result = false;

    assert(icon);
    ITU_ASSERT_THREAD();

    if (widget->flags & ITU_LOADING)
    {
        widget->flags &= ~ITU_LOADING;
        icon->loadedSurf = NULL;
    }
    else if (widget->flags & ITU_LOADED)
    {
        if (icon->filePath)
        {
            free(icon->filePath);
            icon->filePath = NULL;
        }

        if (icon->loadedSurf)
        {
            ituSurfaceRelease(icon->loadedSurf);
            icon->loadedSurf = NULL;
        }
        widget->flags &= ~ITU_LOADED;
    }
    else
    {
        free(icon->filePath);
        icon->filePath = NULL;

#if defined(CFG_JPEG_HW_ENABLE)
        /*   for YC modify*/
        if (widget->flags & ITU_FIT_TO_RECT)
        {
            surf = ituJpegLoadToARGB8888(rect->width, rect->height, data, size, ITU_FIT_TO_RECT);
        }
        else if (widget->flags & ITU_CUT_BY_RECT)
        {
            surf = ituJpegLoadToARGB8888(rect->width, rect->height, data, size, ITU_CUT_BY_RECT);
        }
        else
        {
            surf = ituJpegLoadToARGB8888(0, 0, data, size, 0);
        }
#endif

        if (surf != NULL)
        {
            ITUSurface * preSurf = NULL;
#ifdef CFG_M2D_ENABLE
            ITUSurface * dest  = ituGetDisplaySurface();
            ITUColor     color = {255, 0, 0, 0};
            preSurf            = ituCreateSurface(surf->width, surf->height, 0, ITU_ARGB8888, NULL, 0);
            if ((preSurf != NULL) && (dest != NULL))
            {
                int gx = 0, gy = 0;
                ituWidgetGetGlobalPosition(widget, &gx, &gy);
                ituColorFill(preSurf, 0, 0, preSurf->width, preSurf->height, &color);
                ituBitBlt(preSurf, 0, 0, preSurf->width, preSurf->height, dest, gx, gy);
                ituAlphaBlendSelDstAlpha(preSurf, 0, 0, surf);
            }
#endif
            if (icon->surf)
            {
                ituSurfaceRelease(icon->surf);
            }

            if (preSurf != NULL)
            {
                icon->surf = preSurf;
                ituSurfaceRelease(surf);
                icon->surf->flags |= ITU_JPEGALPHAED;
                surf = NULL;
            }
            else
            {
                icon->surf = surf;
            }

            result = true;
        }
        ituWidgetSetDirty(icon, true);
    }
    return result;
}

static void * IconLoadJpegFileTask (void * arg)
{
    ITUWidget *    widget = (ITUWidget *)arg;
    ITUIcon *      icon   = (ITUIcon *)widget;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf;
    char           filepath[PATH_MAX];
    assert(widget);

    ITU_LOG_DBG("+IconLoadJpegFileTask\n");

    if (!icon->filePath)
    {
        goto end;
    }

    strlcpy(filepath, icon->filePath, sizeof(filepath));

    if (widget->flags & ITU_FIT_TO_RECT)
    {
        surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
    }
    else if (widget->flags & ITU_CUT_BY_RECT)
    {
        surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
    }
    else
    {
        surf = ituJpegLoadFile(0, 0, filepath, 0);
    }

    icon->loadedSurf = surf;
    if (surf)
    {
        widget->flags |= ITU_LOADED;
    }

end:
    widget->flags &= ~ITU_LOADING;

    ITU_LOG_DBG("-IconLoadJpegFileTask\n");
    return NULL;
}
#define FILELIST_TASK_STACK_SIZE (48 * 1024)
void ituIconLoadJpegFile (ITUIcon * icon, char * filepath)
{
    ITUWidget * widget = (ITUWidget *)icon;
    assert(widget);
    ITU_ASSERT_THREAD();

    if ((widget->flags & ITU_LOADING) || (widget->flags & ITU_LOADED))
    {
        return;
    }

    if (strlen(filepath) > 0)
    {
        pthread_t      task;
        pthread_attr_t attr;

        assert(!icon->filePath);
        icon->filePath = strdup(filepath);

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
        widget->flags |= ITU_LOADING;
        pthread_create(&task, &attr, IconLoadJpegFileTask, icon);
    }
}

void ituIconLoadJpegFileSync (ITUIcon * icon, char * filepath)
{
    ITUWidget * widget = (ITUWidget *)icon;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (strlen(filepath) > 0)
    {
        ITURectangle * rect = &widget->rect;
        ITUSurface *   surf;

        if (widget->flags & ITU_FIT_TO_RECT)
        {
            surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
        }
        else if (widget->flags & ITU_CUT_BY_RECT)
        {
            surf = ituJpegLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
        }
        else
        {
            surf = ituJpegLoadFile(0, 0, filepath, 0);
        }

        if (surf)
        {
            if (icon->surf)
            {
                ituSurfaceRelease(icon->surf);
            }

            icon->surf = surf;
        }
    }
}

bool ituIconLoadPngData (ITUIcon * icon, uint8_t * data, int size)
{
    ITUWidget *    widget = (ITUWidget *)icon;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf;
    bool           result = false;

    assert(icon);
    ITU_ASSERT_THREAD();

    if (widget->flags & ITU_LOADING)
    {
        widget->flags &= ~ITU_LOADING;
        icon->loadedSurf = NULL;
    }
    else if (widget->flags & ITU_LOADED)
    {
        if (icon->filePath)
        {
            free(icon->filePath);
            icon->filePath = NULL;
        }

        if (icon->loadedSurf)
        {
            ituSurfaceRelease(icon->loadedSurf);
            icon->loadedSurf = NULL;
        }
        widget->flags &= ~ITU_LOADED;
    }
    else
    {
        free(icon->filePath);
        icon->filePath = NULL;

        if (widget->flags & ITU_FIT_TO_RECT)
        {
            surf = ituPngLoad(rect->width, rect->height, data, size, ITU_FIT_TO_RECT);
        }
        else if (widget->flags & ITU_CUT_BY_RECT)
        {
            surf = ituPngLoad(rect->width, rect->height, data, size, ITU_CUT_BY_RECT);
        }
        else
        {
            surf = ituPngLoad(0, 0, data, size, 0);
        }

        if (surf)
        {
            if (icon->surf)
            {
                ituSurfaceRelease(icon->surf);
            }

            icon->surf = surf;
            result     = true;
        }
        ituWidgetSetDirty(icon, true);
    }
    return result;
}

static void * IconLoadPngFileTask (void * arg)
{
    ITUWidget *    widget = (ITUWidget *)arg;
    ITUIcon *      icon   = (ITUIcon *)widget;
    ITURectangle * rect   = &widget->rect;
    ITUSurface *   surf;
    char           filepath[PATH_MAX];
    assert(widget);

    ITU_LOG_DBG("+IconLoadPngFileTask\n");

    if (!icon->filePath)
    {
        goto end;
    }

    strlcpy(filepath, icon->filePath, sizeof(filepath));

    if (widget->flags & ITU_FIT_TO_RECT)
    {
        surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
    }
    else if (widget->flags & ITU_CUT_BY_RECT)
    {
        surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
    }
    else
    {
        surf = ituPngLoadFile(0, 0, filepath, 0);
    }

    icon->loadedSurf = surf;
    if (surf)
    {
        widget->flags |= ITU_LOADED;
    }

end:
    widget->flags &= ~ITU_LOADING;

    ITU_LOG_DBG("-IconLoadPngFileTask\n");
    return NULL;
}

void ituIconLoadPngFile (ITUIcon * icon, char * filepath)
{
    ITUWidget * widget = (ITUWidget *)icon;
    assert(widget);
    ITU_ASSERT_THREAD();

    if ((widget->flags & ITU_LOADING) || (widget->flags & ITU_LOADED))
    {
        return;
    }

    if (strlen(filepath) > 0)
    {
        pthread_t      task;
        pthread_attr_t attr;

        assert(!icon->filePath);
        icon->filePath = strdup(filepath);

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
        widget->flags |= ITU_LOADING;
        pthread_create(&task, &attr, IconLoadPngFileTask, icon);
    }
}

void ituIconLoadPngFileSync (ITUIcon * icon, char * filepath)
{
    ITUWidget * widget = (ITUWidget *)icon;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (strlen(filepath) > 0)
    {
        ITURectangle * rect = &widget->rect;
        ITUSurface *   surf;

        if (widget->flags & ITU_FIT_TO_RECT)
        {
            surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_FIT_TO_RECT);
        }
        else if (widget->flags & ITU_CUT_BY_RECT)
        {
            surf = ituPngLoadFile(rect->width, rect->height, filepath, ITU_CUT_BY_RECT);
        }
        else
        {
            surf = ituPngLoadFile(0, 0, filepath, 0);
        }

        if (surf)
        {
            if (icon->surf)
            {
                ituSurfaceRelease(icon->surf);
            }

            icon->surf = surf;
        }
    }
}

void ituIconLoadStaticData (ITUIcon * icon)
{
    const ITUWidget * widget = (ITUWidget *)icon;
    ITUSurface *      surf;
    bool              drawReq = ituSceneClearD2D(ituScene);

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (widget->flags & ITU_EXTERNAL_IMAGE)
    {
        IconLoadImage(icon, NULL);
        return;
    }

    if (!icon->staticSurf || (widget->flags & ITU_EXTERNAL))
    {
        return;
    }
#else
    if (!icon->staticSurf || icon->surf || (widget->flags & ITU_EXTERNAL) || (widget->flags & ITU_EXTERNAL_IMAGE))
    {
        return;
    }
#endif

    surf = icon->staticSurf;

    if (surf->flags & ITU_COMPRESSED)
    {
        ituSceneCheckD2D(ituScene, drawReq);
        icon->surf = ituSurfaceDecompress(surf);
    }
    else
    {
        icon->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t *)surf->addr,
                                      surf->flags);
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    // corner mask not support for dcps in place mode.
    return;
#endif

    // load corner mask surface
    if (icon->staticMaskTL && !icon->maskTL)
    {
        ITUSurface * surfTL = icon->staticMaskTL;
        if (surfTL->flags & ITU_COMPRESSED)
        {
            icon->maskTL = ituSurfaceDecompress(surfTL);
        }
        else
        {
            icon->maskTL = ituCreateSurface(surfTL->width, surfTL->height, surfTL->pitch, surfTL->format,
                                            (const uint8_t *)surfTL->addr, surfTL->flags);
        }
    }
    if (icon->staticMaskTR && !icon->maskTR)
    {
        ITUSurface * surfTR = icon->staticMaskTR;
        if (surfTR->flags & ITU_COMPRESSED)
        {
            icon->maskTR = ituSurfaceDecompress(surfTR);
        }
        else
        {
            icon->maskTR = ituCreateSurface(surfTR->width, surfTR->height, surfTR->pitch, surfTR->format,
                                            (const uint8_t *)surfTR->addr, surfTR->flags);
        }
    }
    if (icon->staticMaskBL && !icon->maskBL)
    {
        ITUSurface * surfBL = icon->staticMaskBL;
        if (surfBL->flags & ITU_COMPRESSED)
        {
            icon->maskBL = ituSurfaceDecompress(surfBL);
        }
        else
        {
            icon->maskBL = ituCreateSurface(surfBL->width, surfBL->height, surfBL->pitch, surfBL->format,
                                            (const uint8_t *)surfBL->addr, surfBL->flags);
        }
    }
    if (icon->staticMaskBR && !icon->maskBR)
    {
        ITUSurface * surfBR = icon->staticMaskBR;
        if (surfBR->flags & ITU_COMPRESSED)
        {
            icon->maskBR = ituSurfaceDecompress(surfBR);
        }
        else
        {
            icon->maskBR = ituCreateSurface(surfBR->width, surfBR->height, surfBR->pitch, surfBR->format,
                                            (const uint8_t *)surfBR->addr, surfBR->flags);
        }
    }
}

void ituIconReleaseSurface (ITUIcon * icon)
{
    ITUWidget * widget = (ITUWidget *)icon;
    ITU_ASSERT_THREAD();

    if (!(widget->flags & ITU_EXTERNAL))
    {
        if (icon->staticSurf && ITU_USE_SPRITE_COMMON_BUF)
        {
            if (icon->staticSurf->flags & ITU_SPRITE_BUF)
            {
                icon->surf = NULL;
                return;
            }
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    //workaround for running external image mode under 983X
    if (widget->flags & ITU_EXTERNAL_IMAGE)
    {
        return;
    }
#endif

    if (icon->surf)
    {
        ituSurfaceRelease(icon->surf);
        icon->surf = NULL;

        if (icon->iconflags & ITUICON_CORNER_MASK_LINKING)
        {
            icon->maskTL = NULL;
            icon->maskTR = NULL;
            icon->maskBL = NULL;
            icon->maskBR = NULL;
        }
        else
        {
            if (icon->maskTL)
            {
                ituSurfaceRelease(icon->maskTL);
                icon->maskTL = NULL;
            }
            if (icon->maskTR)
            {
                ituSurfaceRelease(icon->maskTR);
                icon->maskTR = NULL;
            }
            if (icon->maskBL)
            {
                ituSurfaceRelease(icon->maskBL);
                icon->maskBL = NULL;
            }
            if (icon->maskBR)
            {
                ituSurfaceRelease(icon->maskBR);
                icon->maskBR = NULL;
            }
        }

        if (icon->iconflags & ITUICON_CORNER_MASK_ACTIVE)
        {
            icon->iconflags &= ~ITUICON_CORNER_MASK_ACTIVE;
        }

        if (widget->flags & ITU_EXTERNAL)
        {
            ITULayer * layer = ituGetLayer(widget);
            if (icon->staticSurf)
            {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                if (widget->flags & ITU_EXTERNAL_IN_PLACE)
                {
                    widget->flags &= ~ITU_EXTERNAL_IN_PLACE;
                }
#endif
                ituLayerReleaseExternalSurface(layer);
            }
        }
    }
}

void ituIconLinkSurface (ITUIcon * icon, ITUIcon * src)
{
    assert(icon);
    assert(src);
    ITU_ASSERT_THREAD();

    if (icon->surf)
    {
        ituSurfaceRelease(icon->surf);
        icon->surf = NULL;
        if (icon->iconflags & ITUICON_CORNER_MASK_ACTIVE)
        {
            icon->iconflags &= ~ITUICON_CORNER_MASK_ACTIVE;
        }
    }

    if (src->surf)
    {
        ITUSurface * parentSurf = (ITUSurface *)src->surf->parent;

        if (parentSurf && (parentSurf->flags & ITU_STATIC))
        {
            assert(parentSurf->flags & ITU_COMPRESSED);
            assert(parentSurf->lockSize);

            if ((!(src->widget.flags & ITU_EXTERNAL) && src->staticSurf) ||
                ((src->widget.flags & ITU_EXTERNAL) && (icon->widget.flags & ITU_EXTERNAL)))
            {
                icon->staticSurf = src->staticSurf;
            }
            parentSurf->lockSize++;
            icon->surf = src->surf;
        }
        else if (!(src->widget.flags & ITU_EXTERNAL) && src->staticSurf)
        {
            ITUSurface * staticSurf = src->staticSurf;
            assert((staticSurf->flags & ITU_COMPRESSED) == 0);
            icon->surf = ituCreateSurface(staticSurf->width, staticSurf->height, staticSurf->pitch, staticSurf->format,
                                          (const uint8_t *)staticSurf->addr, staticSurf->flags);
        }
        else
        {
            icon->surf = src->surf;
        }
    }
}

void ituIconRmSetEnable (ITUIcon * icon, bool enable)
{
    if (icon)
    {
        icon->use_rotate_mask = (enable) ? (1) : (0);
        ituDirtyWidget(icon, true);
        ituWidgetUpdate(icon, ITU_EVENT_LAYOUT, 0, 0, 0);
    }
}

bool ituIconRmIsEnable (const ITUIcon * icon)
{
    if (icon)
    {
        if (icon->use_rotate_mask > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool ituIconRmSetAngle (ITUIcon * icon, int start_angle, int end_angle)
{
    if (icon)
    {
        bool check_angle = false;

        if (end_angle <= start_angle)
        {
            return false;
        }
        else if (start_angle < 0)
        {
            if ((360 - end_angle) <= (-start_angle))
            {
                return false;
            }
            else
            {
                check_angle = true;
            }
        }
        else if (end_angle > 360)
        {
            if ((end_angle - 360) >= start_angle)
            {
                return false;
            }
            else
            {
                check_angle = true;
            }
        }
        else
        {
            check_angle = true;
        }

        if (check_angle)
        {
            icon->start_rm_angle = start_angle;
            icon->end_rm_angle   = end_angle;
            if (icon->use_rotate_mask > 0)
            {
                ituIconRmSetEnable(icon, true);
            }
        }
        return check_angle;
    }
    else
    {
        return false;
    }
}
