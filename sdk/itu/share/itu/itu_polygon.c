#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char polygonName[] = "ITUPolygon";

void ituPolygonExit (ITUWidget * widget)
{
    ITUPolygon * pg = (ITUPolygon *)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (pg->cacheSurf != NULL)
    {
        ituSurfaceRelease(pg->cacheSurf);
        pg->cacheSurf = NULL;
    }

    ituIconExit(widget);
}

bool ituPolygonCheckPointInside (ITUPoint p, ITUPoint * polygon, int polygonCount)
{
    int  i = 0, j = polygonCount - 1;
    int  minX   = polygon[0].x;
    int  maxX   = polygon[0].x;
    int  minY   = polygon[0].y;
    int  maxY   = polygon[0].y;
    bool inside = false;
    if ((polygonCount < 3) || (polygonCount > ITU_POLYGON_MAX_POINT_COUNT))
    {
        return false;
    }

    for (; i < polygonCount; i++)
    {
        ITUPoint pt = polygon[i];
        minX        = (pt.x < minX) ? (pt.x) : (minX);
        maxX        = (pt.x > maxX) ? (pt.x) : (maxX);
        minY        = (pt.y < minY) ? (pt.y) : (minY);
        maxY        = (pt.y > maxY) ? (pt.y) : (maxY);
    }

    if ((p.x < minX) || (p.x > maxX) || (p.y < minY) || (p.y > maxY))
    {
        return false;
    }

    for (i = 0; i < polygonCount; j = i++)
    {
        int dnum   = (polygon[j].y - polygon[i].y);
        int calNum = polygon[i].x;
        if (dnum != 0)
        {
            calNum += ((polygon[j].x - polygon[i].x) * (p.y - polygon[i].y) / dnum);
            if (((polygon[i].y > p.y) != (polygon[j].y > p.y)) && (p.x < calNum))
            {
                inside = !inside;
            }
        }
        else
        {
            continue;
        }
    }

    return inside;
}

bool ituPolygonUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool         result = false;
    ITUPolygon * pg     = (ITUPolygon *)widget;
    assert(widget);

    result = ituIconUpdate(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_LAYOUT)
    {
        if (pg->cacheSurf != NULL)
        {
            ituSurfaceRelease(pg->cacheSurf);
            pg->cacheSurf = NULL;
        }
    }

    return widget->visible ? result : false;
}

void ituPolygonDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int             destx = 0, desty = 0;
    ITURectangle    prevClip   = {0, 0, 0, 0};
    ITUBackground * bg         = (ITUBackground *)widget;
    ITUPolygon *    pg         = (ITUPolygon *)widget;
    ITURectangle *  rect       = (ITURectangle *)&widget->rect;
    int             pointCount = pg->pointCount;
    int             i = 0, j = 0;
    assert(pg);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;

    ituBackgroundDraw(widget, dest, x, y, alpha);

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    for (j = 0; j < rect->height; j++)
    {
        int a1 = -1, dev = 0;
        for (i = 0; i < rect->width; i++)
        {
            ITUPoint   lp   = {i, j};
            ITUPoint * pArr = &pg->pointArray[0];
            if (ituPolygonCheckPointInside(lp, pArr, pointCount))
            {
                if (a1 < 0)
                {
                    a1 = i;
                }
                else
                {
                    dev++;
                }
            }
        }

        if (a1 >= 0)
        {
            int rx = destx + a1;
            int ry = desty + j;

            if (bg->graidentMode == ITU_GF_NONE)
            {
                if (dev > 0)
                {
                    ituDrawLine(dest, rx, ry, rx + dev, ry, &pg->fillColor, 1);
                }
                else
                {
                    ituColorFill(dest, rx, ry, 1, 1, &pg->fillColor);
                }
            }
            else
            {
                if (dev > 0)
                {
                    ituGradientFill(dest, rx, ry, dev, 1, &pg->fillColor, &bg->graidentColor, bg->graidentMode);
                }
                else
                {
                    ituColorFill(dest, rx, ry, 1, 1, &pg->fillColor);
                }
            }
        }
    }

    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
}

void ituPolygonInit (ITUPolygon * pg)
{
    assert(pg);
    ITU_ASSERT_THREAD();

    (void)memset((void *)pg, 0, sizeof(ITUPolygon));

    ituBackgroundInit(&pg->bg);

    ituWidgetSetType(pg, ITU_POLYGON);
    ituWidgetSetName(pg, polygonName);
    ituWidgetSetExit(pg, ituPolygonExit);
    ituWidgetSetUpdate(pg, ituPolygonUpdate);
    ituWidgetSetDraw(pg, ituPolygonDraw);
}

void ituPolygonLoad (ITUPolygon * pg, uint32_t base)
{
    assert(pg);

    ituBackgroundLoad(&pg->bg, base);
    ituWidgetSetExit(pg, ituPolygonExit);
    ituWidgetSetUpdate(pg, ituPolygonUpdate);
    ituWidgetSetDraw(pg, ituPolygonDraw);
}

void ituPolygonSetPointCount (ITUPolygon * pg, int count)
{
    if (pg != NULL)
    {
        if ((count >= 3) && (count <= ITU_POLYGON_MAX_POINT_COUNT))
        {
            pg->pointCount = count;
        }
    }
}

void ituPolygonSetPoint (ITUPolygon * pg, int * pointX, int * pointY, int count)
{
    if (pg != NULL)
    {
        if ((count >= 3) && (count <= ITU_POLYGON_MAX_POINT_COUNT))
        {
            pg->pointCount = count;

            if ((pointX != NULL) && (pointY != NULL))
            {
                int i = 0;
                for (; i < count; i++)
                {
                    pg->xPoints[i] = pg->pointArray[i].x = pointX[i];
                    pg->yPoints[i] = pg->pointArray[i].y = pointY[i];
                }
            }
        }
    }
}
