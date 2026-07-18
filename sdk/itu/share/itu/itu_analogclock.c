#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char aclkName[] = "ITUAnalogClock";

bool ituAnalogClockUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool result;
    ITUAnalogClock* aclk = (ITUAnalogClock*) widget;
    assert(aclk);

    result = ituIconUpdate(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_TIMER)
    {
        struct timeval tv;
        struct tm* tm = NULL;

        gettimeofday(&tv, NULL);
        tm = localtime(&tv.tv_sec);

        if (tm != NULL)
        {
            if ((NULL != aclk->secondIcon) && (tm->tm_sec != aclk->second))
            {
                aclk->second = tm->tm_sec;
                widget->dirty = true;
                result = true;
            }
            if (tm->tm_min != aclk->minute)
            {
                aclk->minute = tm->tm_min;
                widget->dirty = true;
                result = true;
            }
            if (tm->tm_hour != aclk->hour)
            {
                aclk->hour = tm->tm_hour;
                widget->dirty = true;
                result = true;
            }
        }
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        if ((NULL == aclk->hourIcon) && (aclk->hourName[0] != '\0'))
        {
            aclk->hourIcon = (ITUIcon*) ituSceneFindWidget(ituScene, aclk->hourName);
        }
        if ((NULL == aclk->minuteIcon) && (aclk->minuteName[0] != '\0'))
        {
            aclk->minuteIcon = (ITUIcon*) ituSceneFindWidget(ituScene, aclk->minuteName);
        }
        if ((NULL == aclk->secondIcon) && (aclk->secondName[0] != '\0'))
        {
            aclk->secondIcon = (ITUIcon*) ituSceneFindWidget(ituScene, aclk->secondName);
        }
        widget->dirty = true;
        result = true;
    }
    else
    {
        /* do nothing */
    }
    return result;
}

void ituAnalogClockDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    ITUAnalogClock* aclk = (ITUAnalogClock*)widget;
    ITURectangle prevClip;
    float angle;
    ITCTree* child = widget->tree.child;
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    uint8_t alphaLocal = alpha * widget->alpha / 255;
#endif

    widget->tree.child = NULL;
    ituBackgroundDraw(widget, dest, x, y, alpha);
    widget->tree.child = child;

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    x += widget->rect.x;
    y += widget->rect.y;

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] AnalogClockDraw but SurfacePool not ready yet!\n");
        return;
    }

    if ((NULL != aclk->hourIcon) && (alphaLocal > 0))
    {
        ituSceneSetD2D(ituScene, widget);
        if (widget->flags & ITU_EXTERNAL)
        {
            ituIconLoadExternalDataAPI(aclk->hourIcon);
        }
        else
        {
            ituIconLoadStaticData(aclk->hourIcon);
        }
    }
#endif

    if ((NULL != aclk->hourIcon) && (NULL != aclk->hourIcon->surf))
    {
        float hour;

        if (aclk->hour > 12)
        {
            hour = (float)(aclk->hour - 12);
        }
        else
        {
            hour = (float)aclk->hour;
        }

        hour += aclk->minute / 60.0f;

        angle = 360.0f / 12.0f * hour;
#ifdef CFG_M2D_ENABLE
        ituRotate(dest, x + aclk->hourIcon->widget.rect.x, y + aclk->hourIcon->widget.rect.y, aclk->hourIcon->surf, aclk->hourX, aclk->hourY, angle, 1.0f, 1.0f);
#else
        ituRotate(dest, x + aclk->hourIcon->widget.rect.x + aclk->hourX, y + aclk->hourIcon->widget.rect.y + aclk->hourY, aclk->hourIcon->surf, aclk->hourX, aclk->hourY, angle, 1.0f, 1.0f);
#endif
        ituWidgetSetDirty(aclk->hourIcon, false);
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if ((NULL != aclk->minuteIcon) && (alphaLocal > 0))
    {
        ituSceneSetD2D(ituScene, widget);
        if (widget->flags & ITU_EXTERNAL)
        {
            ituIconLoadExternalDataAPI(aclk->minuteIcon);
        }
        else
        {
            ituIconLoadStaticData(aclk->minuteIcon);
        }
    }
#endif

    if ((NULL != aclk->minuteIcon) && (NULL != aclk->minuteIcon->surf))
    {
        angle = 360.0f / 60.0f * aclk->minute;
#ifdef CFG_M2D_ENABLE
        ituRotate(dest, x + aclk->minuteIcon->widget.rect.x, y + aclk->minuteIcon->widget.rect.y, aclk->minuteIcon->surf, aclk->minuteX, aclk->minuteY, angle, 1.0f, 1.0f);
#else
        ituRotate(dest, x + aclk->minuteIcon->widget.rect.x + aclk->minuteX, y + aclk->minuteIcon->widget.rect.y + aclk->minuteY, aclk->minuteIcon->surf, aclk->minuteX, aclk->minuteY, angle, 1.0f, 1.0f);
#endif
        ituWidgetSetDirty(aclk->minuteIcon, false);
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if ((NULL != aclk->secondIcon) && (alphaLocal > 0))
    {
        ituSceneSetD2D(ituScene, widget);
        if (widget->flags & ITU_EXTERNAL)
        {
            ituIconLoadExternalDataAPI(aclk->secondIcon);
        }
        else
        {
            ituIconLoadStaticData(aclk->secondIcon);
        }
    }
#endif

    if ((NULL != aclk->secondIcon) && (NULL != aclk->secondIcon->surf))
    {
        angle = 360.0f / 60.0f * aclk->second;
#ifdef CFG_M2D_ENABLE
        ituRotate(dest, x + aclk->secondIcon->widget.rect.x, y + aclk->secondIcon->widget.rect.y, aclk->secondIcon->surf, aclk->secondX, aclk->secondY, angle, 1.0f, 1.0f);
#else
        ituRotate(dest, x + aclk->secondIcon->widget.rect.x + aclk->secondX, y + aclk->secondIcon->widget.rect.y + aclk->secondY, aclk->secondIcon->surf, aclk->secondX, aclk->secondY, angle, 1.0f, 1.0f);
#endif
        ituWidgetSetDirty(aclk->secondIcon, false);
    }
    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
}

void ituAnalogClockInit(ITUAnalogClock* aclk)
{
    assert(aclk);
    ITU_ASSERT_THREAD();

    (void)memset(aclk, 0, sizeof (ITUAnalogClock));

    ituBackgroundInit(&aclk->bg);

    ituWidgetSetType(aclk, ITU_ANALOGCLOCK);
    ituWidgetSetName(aclk, aclkName);
    ituWidgetSetUpdate(aclk, ituAnalogClockUpdate);
    ituWidgetSetDraw(aclk, ituAnalogClockDraw);
}

void ituAnalogClockLoad(ITUAnalogClock* aclk, uint32_t base)
{
    assert(aclk);

    ituBackgroundLoad(&aclk->bg, base);

    if (NULL != aclk->hourIcon)
    {
        aclk->hourIcon = (ITUIcon *)((uint32_t)aclk->hourIcon + base);
    }

    if (NULL != aclk->minuteIcon)
    {
        aclk->minuteIcon = (ITUIcon *)((uint32_t)aclk->minuteIcon + base);
    }

    if (NULL != aclk->secondIcon)
    {
        aclk->secondIcon = (ITUIcon *)((uint32_t)aclk->secondIcon + base);
    }

    ituWidgetSetUpdate(aclk, ituAnalogClockUpdate);
    ituWidgetSetDraw(aclk, ituAnalogClockDraw);
}
