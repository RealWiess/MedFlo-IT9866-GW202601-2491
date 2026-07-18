#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define ITUCIRCLEPROGRESSBAR_RTC  //define this to use the rotate to cache

#define ITUCIRCLEPROGRESSBAR_MAX_ITEM  15  //The maximum amount of circleprogressbar
#define ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM  20  //The maximum amount of cache block

static char cpbNameID[ITUCIRCLEPROGRESSBAR_MAX_ITEM][ITU_WIDGET_NAME_SIZE] = { { "" } };
static char cpbChanged[ITUCIRCLEPROGRESSBAR_MAX_ITEM] = { 0 };
static int cpbRangeSep[ITUCIRCLEPROGRESSBAR_MAX_ITEM] = { 0 };
static ITUSurface* cpbStepSurf[ITUCIRCLEPROGRESSBAR_MAX_ITEM][ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM] = { { NULL } };
static ITUIcon* cpbRangeIcon[ITUCIRCLEPROGRESSBAR_MAX_ITEM][2] = { { NULL } };

static const char circleProgressBarName[] = "ITUCircleProgressBar";

static int        ituCircleProgressBarGetNameID (const ITUWidget * widget)
{
    int i = 0;

    if (widget == NULL)
    {
        return 0;
    }

    for (; i < ITUCIRCLEPROGRESSBAR_MAX_ITEM; i++)
    {
        if (strlen(cpbNameID[i]) == 0)
        {
            strlcpy(cpbNameID[i], widget->name, sizeof(cpbNameID[i]));
            // (void)printf("[CPB][%s NameID %d]\n", widget->name, i);
            return i;
        }
        else
        {
            if (strcmp(cpbNameID[i], widget->name) == 0)
            {
                // (void)printf("[CPB][%s NameID %d]\n", widget->name, i);
                return i;
            }
        }
    }

    return 0;
}

void ituCircleProgressBarFlushPointer(ITUWidget* widget, int currNameID, int value)
{
    ITUCircleProgressBar* bar = (ITUCircleProgressBar*)widget;

    if (!cpbRangeIcon[currNameID][1])
    {
        return;
    }

    if (bar)
    {
        if (bar->progressIcon)
        {
            bar->progressIcon->widget.visible = 1;
        }
        if (bar->progressIcon && cpbRangeIcon[currNameID][0]) //&& cpbRangeIcon[currNameID][1])
        {
            int  tid             = 0;
            bool bCheckChangeReq = false;
            if ((value >= cpbRangeSep[currNameID]) && (bar->progressIcon != cpbRangeIcon[currNameID][1]))
            {
                tid             = 1;
                bCheckChangeReq = true;
            }
            if ((value < cpbRangeSep[currNameID]) && (bar->progressIcon != cpbRangeIcon[currNameID][0]))
            {
                tid             = 0;
                bCheckChangeReq = true;
            }

            if (bCheckChangeReq)
            {
                bar->progressIcon->widget.visible = 0;
                bar->progressIcon                 = cpbRangeIcon[currNameID][tid];
                bar->progressIcon->widget.visible = 1;
            }

            if (!bar->progressIcon->surf)
            {
                const ITUWidget * pointerWidget = (ITUWidget *)bar->progressIcon;
                if (pointerWidget->flags & ITU_EXTERNAL)
                {
                    ituIconLoadExternalDataAPI(bar->progressIcon);
                }
                else
                {
                    ituIconLoadStaticData(bar->progressIcon);
                }
            }
        }
    }
}

void ituCircleProgressBarStepSurfClear(ITUWidget* widget)
{
    if (widget)
    {
        ITUCircleProgressBar * bar = (ITUCircleProgressBar *)widget;
        int                    id  = ituCircleProgressBarGetNameID(widget);
        if (id >= 0)
        {
            int i = 0;
            for (i = 0; i < ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM; i++)
            {
                if (cpbStepSurf[id][i])
                {
                    ituSurfaceRelease(cpbStepSurf[id][i]);
                    cpbStepSurf[id][i] = NULL;
                }
            }
        }
        if (bar->cacheSurf)
        {
            ituDestroySurface(bar->cacheSurf);
            bar->cacheSurf = NULL;
        }
    }
}

static void ituCircleProgressBarExit (ITUWidget * widget)
{
    assert(widget);
    ITU_ASSERT_THREAD();

    ituCircleProgressBarStepSurfClear(widget);
    (void)printf("[CircleProgressBar][Exit][%s]\n", widget->name);
}

void ituCircleProgressBarPrepareStepCache (ITUWidget * widget)
{
    ITUCircleProgressBar * bar                  = (ITUCircleProgressBar *)widget;
    int                    activeCacheItemCount;
    int                    currNameID           = ituCircleProgressBarGetNameID(widget);
    int                    i;
    ITUSurface *           dest                 = ituGetDisplaySurface();

    if ((widget == NULL) || (bar->cacheCount == 0) || ((currNameID < 0) || (currNameID >= ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM)))
    {
        return;
    }
    else
    {
        activeCacheItemCount = (bar->maxValue / bar->cacheCount);
    }

    if ((bar->maxValue % bar->cacheCount) == 0)
    {
        activeCacheItemCount--;
    }

    for (i = 0; i < activeCacheItemCount; i++)
    {
        if (!cpbStepSurf[currNameID][i]) // ITU_ARGB8888
        {
            cpbStepSurf[currNameID][i] =
                ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
        }

        if (cpbStepSurf[currNameID][i])
        {
            if (!bar->bg.icon.surf)
            { // under develop for no bg image case
                int destx = 0;
                int desty = 0;
                ituWidgetGetGlobalPosition(widget, &destx, &desty);
                ituColorFill(cpbStepSurf[currNameID][i], 0, 0, widget->rect.width, widget->rect.height, &widget->color);
                ituBitBlt(cpbStepSurf[currNameID][i], 0, 0, widget->rect.width, widget->rect.height, dest, destx,
                          desty);
            }
        }
    }

    // (void)printf("[CPB_Layout][%d][%s][cacheCount %d][maxValue %d][Angle %d to %d]\n", currNameID, widget->name,
    // bar->cacheCount, bar->maxValue, bar->startAngle, bar->endAngle);
    for (i = 0; i <= bar->maxValue; ++i)
    {
        int   currIdx = i / bar->cacheCount;
        float angle   = 0.0;

        // if ((bar->maxValue - i) <= bar->cacheCount)
        //   break;

        ituCircleProgressBarFlushPointer(widget, currNameID, i);

        if (i >= (activeCacheItemCount * bar->cacheCount))
        {
            break;
        }

        if ((i % bar->cacheCount) == 0)
        {
            if (currIdx > 0)
            {
                ituBitBlt(cpbStepSurf[currNameID][currIdx], 0, 0, widget->rect.width, widget->rect.height,
                          cpbStepSurf[currNameID][currIdx - 1], 0, 0);
            }
            // (void)printf("[CPB-prepare-step-cache][%d][%s][%d]\n", currNameID, widget->name, currIdx);
        }

        if (bar->endAngle > bar->startAngle)
        {
            angle = bar->startAngle + (bar->endAngle - bar->startAngle) * i / (float)bar->maxValue;
        }
        else if (bar->endAngle < bar->startAngle)
        {
            angle = bar->startAngle - (bar->startAngle - bar->endAngle) * i / (float)bar->maxValue;
        }
        else
        {
            /* do nothing */
        }

#ifdef CFG_M2D_ENABLE
        // ituRotate(cpbStepSurf[currNameID][currIdx], widget->rect.width / 2, (widget->rect.height / 2) -
        // bar->progressIcon->surf->height, bar->progressIcon->surf, 0, bar->progressIcon->surf->height,
        // angle, 1.0f, 1.0f);
        ituTransform(cpbStepSurf[currNameID][currIdx], bar->progressIcon->widget.rect.x,
                     bar->progressIcon->widget.rect.y, bar->progressIcon->surf->width, bar->progressIcon->surf->height,
                     bar->progressIcon->surf, 0, 0, bar->progressIcon->surf->width, bar->progressIcon->surf->height, 0,
                     bar->progressIcon->surf->height, 1.0f, 1.0f, (float)angle, ITU_TILE_FILL, true, true, 255);
#else
        ituRotate(cpbStepSurf[currNameID][currIdx], widget->rect.width / 2, widget->rect.height / 2,
                  bar->progressIcon->surf, 0, bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
#endif
    }
}

bool ituCircleProgressBarUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool result = false;
    ITUCircleProgressBar* bar = (ITUCircleProgressBar*)widget;
    assert(bar);

    if (ev == ITU_EVENT_LAYOUT)
    {
        //ituCircleProgressBarExit(widget);
        int currNameID = ituCircleProgressBarGetNameID(widget);

        if (currNameID >= 0)
        {

            if (bar->cacheCount > 0)
            {
                if (((bar->maxValue - 1) / bar->cacheCount) > ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM)
                {
                    bar->cacheCount = (bar->maxValue - 1) / ITUCIRCLEPROGRESSBAR_MAX_CACHE_ITEM;
                }
            }

            if (!bar->progressIcon && (bar->progressName[0] != '\0'))
            {
                bar->progressIcon = (ITUIcon *)ituSceneFindWidget(ituScene, bar->progressName);
            }
            if ((bar->progressIcon) && (!cpbRangeIcon[currNameID][0]))
            {
                cpbRangeIcon[currNameID][0] = bar->progressIcon;
            }

            if (!bar->percentText && (bar->percentName[0] != '\0'))
            {
                bar->percentText = (ITUText *)ituSceneFindWidget(ituScene, bar->percentName);
            }

            if (bar->percentText)
            {
                char buf[5] = {0};

                snprintf(buf, sizeof(buf), "%i", bar->value);
                ituTextSetString(bar->percentText, buf);

                result = widget->dirty = true;
            }

            //someone use draw before layout test
            if ((bar->cacheSurf) && (!cpbStepSurf[currNameID][0]))
            {
                ituCircleProgressBarPrepareStepCache(widget);
            }

            if (!bar->cacheSurf && bar->cacheCount > 0 && bar->progressIcon && bar->progressIcon->surf && (bar->endAngle != bar->startAngle))
            {
                cpbChanged[currNameID] = 1;

                if ((bar->cacheCount <= bar->maxValue) && (bar->cacheCount > 0))
                {
                    int destx = 0;
                    int desty = 0;
                    ITUSurface* dest = ituGetDisplaySurface();
                    ituWidgetGetGlobalPosition(widget, &destx, &desty);

                    bar->cacheSurf =
                        ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
                    if (bar->cacheSurf)
                    {
                        if (bar->bg.icon.surf)
                        {
                            ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, dest, destx,
                                      desty);
                            ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, bar->bg.icon.surf,
                                      0, 0);
                        }
                        else
                        {
                            ituColorFill(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, &widget->color);
                        }
                    }

                    ituCircleProgressBarPrepareStepCache(widget);

                    ituCircleProgressBarFlushPointer(widget, currNameID, bar->value); //reset to current value status
                }
            }
        }
    }
    else if ((ev == ITU_EVENT_RELEASE) || (ev == ITU_EVENT_LEAVE))
    {
        ituCircleProgressBarExit(widget);
    }
    else
    {
        /* do nothing */
    }
    result |= ituIconUpdate(widget, ev, arg1, arg2, arg3);
    return widget->visible ? result : false;
}

void ituCircleProgressBarDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    ITUCircleProgressBar* bar = (ITUCircleProgressBar*)widget;
    ITURectangle prevClip = { 0, 0, 0, 0 };
    const ITURectangle* rect = &widget->rect;
    int destx = 0, desty = 0;
    uint8_t desta = 0;
    uint8_t progressIconVisible = 0;

    if (bar->progressIcon)
    {
        progressIconVisible = bar->progressIcon->widget.visible;
        bar->progressIcon->widget.visible = false;
    }

    if (bar->cacheCount == 0)
    {
        ituBackgroundDraw(widget, dest, x, y, alpha);
    }

    if (bar->progressIcon)
    {
        bar->progressIcon->widget.visible = progressIconVisible;
    }

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255;

    if (desta > 0)
    {
        if (bar->progressIcon && bar->progressIcon->surf && bar->value >= 0 && (bar->endAngle != bar->startAngle))
        {
            float angle      = 0.0;
            int   i          = 0;
            int   currNameID = ituCircleProgressBarGetNameID(widget);

            if (currNameID >= 0)
            {
                if ((bar->cacheCount > 0) && (bar->maxValue > 0))
                {
                    if (!bar->cacheSurf) // ITU_ARGB8888
                    {
                        bar->cacheSurf =
                            ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
                    }

                    // someone use draw before layout test
                    if ((bar->cacheSurf) && (!cpbStepSurf[currNameID][0]))
                    {
                        ituCircleProgressBarPrepareStepCache(widget);
                    }

                    if (bar->cacheSurf)
                    {
#ifndef ITUCIRCLEPROGRESSBAR_RTC
                        if (bar->bg.icon.surf)
                        {
                            ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, bar->bg.icon.surf,
                                      0, 0);
                        }
                        else
                        {
                            ituColorFill(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, &widget->color);
                        }
#else
                        if (cpbChanged[currNameID])
                        {
                            if (bar->bg.icon.surf)
                            {
                                ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height, dest, destx,
                                          desty);
                                ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height,
                                          bar->bg.icon.surf, 0, 0);
                            }
                            else
                            {
                                ituColorFill(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height,
                                             &widget->color);
                            }
                        }
#endif
                    }

                    if ((bar->value > 0) && (bar->value >= bar->cacheCount) && (bar->value <= bar->maxValue))
                    {
                        // int currNameID = ituCircleProgressBarGetNameID(widget);
                        int currIdx = bar->value / bar->cacheCount;

                        if ((bar->value == bar->maxValue) && ((bar->maxValue % bar->cacheCount) == 0))
                        {
                            currIdx--;
                        }

                        i = currIdx * bar->cacheCount;

                        if ((currIdx > 0) && (cpbStepSurf[currNameID][currIdx - 1]))
                        {
#ifndef ITUCIRCLEPROGRESSBAR_RTC
                            ituBitBlt(dest, destx, desty, widget->rect.width, widget->rect.height,
                                      cpbStepSurf[currNameID][currIdx - 1], 0, 0);
#else
                            if (cpbChanged[currNameID])
                            {
                                ituBitBlt(bar->cacheSurf, 0, 0, widget->rect.width, widget->rect.height,
                                          cpbStepSurf[currNameID][currIdx - 1], 0, 0);
                            }
#endif
                            // (void)printf("[CPB][%d][%s][draw step cache %d]\n", currNameID, widget->name, currIdx - 1);
                        }
                    }
                }

                // (void)printf("[CPB][%s][cacheCount %d]draw value %d to %d\n", widget->name, bar->cacheCount, i,
                // bar->value);

                for (; i <= bar->value; ++i)
                {
                    ituCircleProgressBarFlushPointer(widget, currNameID, i);

#ifdef ITUCIRCLEPROGRESSBAR_RTC
                    if (bar->cacheCount > 0)
                    {
                        if ((cpbChanged[currNameID] == 0) && bar->cacheSurf)
                        {
                            ituBitBlt(dest, rect->x + x, rect->y + y, widget->rect.width, widget->rect.height,
                                      bar->cacheSurf, 0, 0);
                            break;
                        }
                    }
#endif

                    if (bar->endAngle > bar->startAngle)
                    {
                        angle = bar->startAngle + (bar->endAngle - bar->startAngle) * i / (float)bar->maxValue;
                    }
                    else if (bar->endAngle < bar->startAngle)
                    {
                        angle = bar->startAngle - (bar->startAngle - bar->endAngle) * i / (float)bar->maxValue;
                    }
                    else
                    {
                        /* do nothing */
                    }

                    if (desta == 255)
                    {
#ifdef CFG_M2D_ENABLE
    #ifndef ITUCIRCLEPROGRESSBAR_RTC
                        // ituRotate(dest, destx + bar->progressIcon->widget.rect.x, desty +
                        // bar->progressIcon->widget.rect.y, bar->progressIcon->surf, 0,
                        // bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                        ituTransform(dest, destx + bar->progressIcon->widget.rect.x,
                                     desty + bar->progressIcon->widget.rect.y, bar->progressIcon->surf->width,
                                     bar->progressIcon->surf->height, bar->progressIcon->surf, 0, 0,
                                     bar->progressIcon->surf->width, bar->progressIcon->surf->height, 0,
                                     bar->progressIcon->surf->height, 1.0f, 1.0f, (float)angle, ITU_TILE_FILL, true, true, 255);
    #else
                        if (bar->cacheSurf)
                        {
                            // ituRotate(bar->cacheSurf, widget->rect.width / 2, (widget->rect.height / 2) -
                            // bar->progressIcon->surf->height, bar->progressIcon->surf, 0,
                            // bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                            ituTransform(bar->cacheSurf, bar->progressIcon->widget.rect.x,
                                         bar->progressIcon->widget.rect.y, bar->progressIcon->surf->width,
                                         bar->progressIcon->surf->height, bar->progressIcon->surf, 0, 0,
                                         bar->progressIcon->surf->width, bar->progressIcon->surf->height, 0,
                                         bar->progressIcon->surf->height, 1.0f, 1.0f, (float)angle, ITU_TILE_FILL, true, true, 255);
                            ituBitBlt(dest, rect->x + x, rect->y + y, widget->rect.width, widget->rect.height,
                                      bar->cacheSurf, 0, 0);
                        }
                        else
                        {
                            // ituRotate(dest, destx + bar->progressIcon->widget.rect.x, desty +
                            // bar->progressIcon->widget.rect.y, bar->progressIcon->surf, 0,
                            // bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                            ituTransform(dest, destx + bar->progressIcon->widget.rect.x,
                                         desty + bar->progressIcon->widget.rect.y, bar->progressIcon->surf->width,
                                         bar->progressIcon->surf->height, bar->progressIcon->surf, 0, 0,
                                         bar->progressIcon->surf->width, bar->progressIcon->surf->height, 0,
                                         bar->progressIcon->surf->height, 1.0f, 1.0f, (float)angle, ITU_TILE_FILL, true, true, 255);
                        }
    #endif
#else
    #ifndef ITUCIRCLEPROGRESSBAR_RTC
                        ituRotate(dest, destx + bar->progressIcon->widget.rect.x,
                                  desty + bar->progressIcon->widget.rect.y + bar->progressIcon->surf->height,
                                  bar->progressIcon->surf, 0, bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
    #else
                        if (bar->cacheSurf)
                        {
                            ituRotate(bar->cacheSurf, widget->rect.width / 2, (widget->rect.height / 2),
                                      bar->progressIcon->surf, 0, bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                            ituBitBlt(dest, rect->x + x, rect->y + y, widget->rect.width, widget->rect.height,
                                      bar->cacheSurf, 0, 0);
                        }
                        else
                        {
                            ituRotate(dest, destx + bar->progressIcon->widget.rect.x,
                                      desty + bar->progressIcon->widget.rect.y + bar->progressIcon->surf->height,
                                      bar->progressIcon->surf, 0, bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                        }
    #endif
#endif
                    }
                    else
                    {
#ifdef CFG_M2D_ENABLE
                        if (bar->cacheSurf)
                        {
                            ituTransform(bar->cacheSurf, bar->progressIcon->widget.rect.x,
                                         bar->progressIcon->widget.rect.y, bar->progressIcon->surf->width,
                                         bar->progressIcon->surf->height, bar->progressIcon->surf, 0, 0,
                                         bar->progressIcon->surf->width, bar->progressIcon->surf->height, 0,
                                         bar->progressIcon->surf->height, 1.0f, 1.0f, (float)angle, ITU_TILE_FILL, true, true, 255);
                            ituAlphaBlend(dest, rect->x + x, rect->y + y, widget->rect.width, widget->rect.height,
                                          bar->cacheSurf, 0, 0, desta);
                        }
#else
                        if (bar->cacheSurf)
                        {
                            ituRotate(bar->cacheSurf, bar->progressIcon->widget.rect.x,
                                      bar->progressIcon->widget.rect.y + bar->progressIcon->surf->height,
                                      bar->progressIcon->surf, 0, bar->progressIcon->surf->height, angle, 1.0f, 1.0f);
                            ituAlphaBlend(dest, rect->x + x, rect->y + y, widget->rect.width, widget->rect.height,
                                          bar->cacheSurf, 0, 0, desta);
                        }
#endif
                    }
                    // (void)printf("[CPB]draw value %d --> %d\n", i, bar->value);
                }
                ituCircleProgressBarFlushPointer(widget, currNameID, bar->value); // reset to current value status

                if (cpbChanged[currNameID])
                {
                    cpbChanged[currNameID] = 0;
                }

                ituWidgetSetDirty(bar->progressIcon, false);
            }
        }
    }
    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    ituDirtyWidget(bar, false);
}

void ituCircleProgressBarOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    assert(widget);

    switch (action)
    {
    case ITU_ACTION_GOTO:
        ituCircleProgressBarSetValue((ITUCircleProgressBar*)widget, atoi(param));
        break;

    default:
        ituWidgetOnActionImpl(widget, action, param);
        break;
    }
}

void ituCircleProgressBarInit(ITUCircleProgressBar* bar)
{
    assert(bar);
    ITU_ASSERT_THREAD();

    (void)memset(bar, 0, sizeof (ITUCircleProgressBar));

    ituBackgroundInit(&bar->bg);

    ituWidgetSetType(bar, ITU_CIRCLEPROGRESSBAR);
    ituWidgetSetName(bar, circleProgressBarName);
    ituWidgetSetExit(bar, ituCircleProgressBarExit);
    ituWidgetSetUpdate(bar, ituCircleProgressBarUpdate);
    ituWidgetSetDraw(bar, ituCircleProgressBarDraw);
    ituWidgetSetOnAction(bar, ituCircleProgressBarOnAction);
}

void ituCircleProgressBarLoad(ITUCircleProgressBar* bar, uint32_t base)
{
    assert(bar);

    ituBackgroundLoad(&bar->bg, base);

    if (bar->progressIcon)
    {
    bar->progressIcon = (ITUIcon *)((uint32_t)bar->progressIcon + base);
    }

    if (bar->percentText)
    {
    bar->percentText = (ITUText *)((uint32_t)bar->percentText + base);
    }

    ituWidgetSetExit(bar, ituCircleProgressBarExit);
    ituWidgetSetUpdate(bar, ituCircleProgressBarUpdate);
    ituWidgetSetDraw(bar, ituCircleProgressBarDraw);
    ituWidgetSetOnAction(bar, ituCircleProgressBarOnAction);
}

void ituCircleProgressBarSetValue (ITUCircleProgressBar * bar, int value)
{
    assert(bar);
    ITU_ASSERT_THREAD();

    if (value < 0 || value > bar->maxValue)
    {
        ITU_LOG_WARN("incorrect value: %d\n", value);
        return;
    }

    if (bar->value != value)
    {
        int currNameID = ituCircleProgressBarGetNameID((const ITUWidget *)bar);
        if (currNameID >= 0)
        {
            cpbChanged[currNameID] = 1;
        }
    }

    bar->value = value;

    ituWidgetUpdate(bar, ITU_EVENT_LAYOUT, 0, 0, 0);
    ituWidgetSetDirty(bar, true);
}

void ituCircleProgressBarSetAngle(ITUCircleProgressBar* bar, int start, int end)
{
    assert(bar);
    ITU_ASSERT_THREAD();

    if (start < 0 || end < 0)
    {
        ITU_LOG_WARN("incorrect value: %d, %d\n", start, end);
        return;
    }

    bar->startAngle = start;
    bar->endAngle = end;
    if (bar->cacheSurf)
    {
        ituDestroySurface(bar->cacheSurf);
        bar->cacheSurf = NULL;
    }
    ituWidgetUpdate(bar, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituCircleProgressBarSetRangeIcon(ITUCircleProgressBar* bar, ITUIcon* icon, int value)
{
    assert(bar);
    ITU_ASSERT_THREAD();

    if (icon)
    {
        ITUWidget* widget = (ITUWidget*)bar;
        int currNameID = ituCircleProgressBarGetNameID(widget);
        if (currNameID >= 0)
        {
            cpbRangeIcon[currNameID][1] = icon;
            cpbRangeSep[currNameID]     = value;
            ituCircleProgressBarStepSurfClear(widget);
            ituWidgetUpdate(bar, ITU_EVENT_LAYOUT, 0, 0, 0);
        }

    }
}
