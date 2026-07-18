#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define ITU_SCALECOVERFLOW_SLIDE_CHECK_FACTOR 4 // slide must >= widget->rect.width(or widget->rect.height) / factor

static const char  scaleCoverFlowName[] = "ITUScaleCoverFlow";

extern int         CoverFlowGetVisibleChildCount (ITUCoverFlow * coverflow);
extern ITUWidget * CoverFlowGetVisibleChild (ITUCoverFlow * coverflow, int index);

float              ituScaleCoverflowScaleFactor (ITUWidget * widget, int targetW)
{
    if (widget)
    {
        float factor = 1.0f;
        if (widget->org_rect.width > 0)
        {
            factor = ((float)targetW) / ((float)widget->org_rect.width);
        }
        // if (factor < 0.0)
        //	(void)printf("ituScaleCoverflowScaleFactor error!\n");
        // else
        //	(void)printf("widget %s cal factor %.3f\n", widget->name, factor);
        return factor;
    }
    return 0.0f;
}

void ituScaleCoverflowSetChildSize (ITUWidget * widget, float factor)
{
    if (widget)
    {
        ITCTree *   tree       = (ITCTree *)widget;
        ITUWidget * parent     = (ITUWidget *)tree->parent;
        float       posFactorX = (float)widget->org_rect.x / parent->org_rect.width;
        float       posFactorY = (float)widget->org_rect.y / parent->org_rect.height;

        if (widget->type == ITU_TEXT)
        {
            ITUText * tc               = (ITUText *)widget;
            int       scaledFontWidth  = (int)(tc->org_fontWidth * factor);
            int       scaledFontHeight = (int)(tc->org_fontHeight * factor);
            ituTextSetFontWidth(tc, scaledFontWidth);
            ituTextSetFontHeight(tc, scaledFontHeight);
        }
        widget->rect.width  = (int)(widget->org_rect.width * factor);
        widget->rect.height = (int)(widget->org_rect.height * factor);
        widget->dirty       = true;
        widget->rect.x      = (int)(parent->rect.width * posFactorX);
        widget->rect.y      = (int)(parent->rect.height * posFactorY);

        ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
    }
}

void ituScaleCoverflowScaleWidgetChild (ITUWidget * widget, float factor)
{
    ITCTree * node = (ITCTree *)widget;

    for (node = node->child; node; node = node->sibling)
    {
        ITUWidget *       obj   = (ITUWidget *)node;
        const ITUWidget * child = (ITUWidget *)(node->child);

        if (child)
        {
            if (obj->type != ITU_BUTTON)
            {
                ituScaleCoverflowScaleWidgetChild(obj, factor);
            }
        }

        ituScaleCoverflowSetChildSize(obj, factor);
        // (void)printf("widget %s scale factor -> %.3f\n", obj->name, factor);
    }
    return;
}

static void ituWidgetSetPositionFix (ITUCoverFlow * coverflow, ITUWidget * child, int x, int y, int index)
{
    ITUScaleCoverFlow * scalecoverflow = (ITUScaleCoverFlow *)coverflow;
    int                 pos_shift      = scalecoverflow->concentration;
    int                 count          = CoverFlowGetVisibleChildCount(coverflow);

    assert(scalecoverflow);
    return;
    if (coverflow->focusIndex < 0)
    {
        coverflow->focusIndex = 0;
    }
    else if (coverflow->focusIndex > (count - 1))
    {
        coverflow->focusIndex = count - 1;
    }

    if (index == coverflow->focusIndex)
    {
        ituWidgetSetPosition(child, x, y);
    }
    else
    {
        ITUWidget * childf = CoverFlowGetVisibleChild(coverflow, coverflow->focusIndex);
        int         fix;

        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            fix = ((child->rect.y > childf->rect.y) ? (-pos_shift) : (pos_shift));
        }
        else
        {
            fix = ((child->rect.x > childf->rect.x) ? (-pos_shift) : (pos_shift));
        }

        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            ituWidgetSetPosition(child, x, y + fix);
        }
        else
        {
            ituWidgetSetPosition(child, x + fix, y);
        }
    }
}

void ituScaleCoverflowLayoutCal (ITUWidget * widget, int offset)
{
    ITUCoverFlow *      coverflow      = (ITUCoverFlow *)widget;
    ITUScaleCoverFlow * scalecoverflow = (ITUScaleCoverFlow *)widget;
    ITUWidget *         child          = NULL;
    int                 i, factor, width, height, x, y, orgWidth, orgHeight, fcX, fcY;
    int                 count  = CoverFlowGetVisibleChildCount(coverflow);
    int *               posArr = (int *)malloc(sizeof(int) * count);
    bool                hWay   = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) ? (false) : (true);
    float               step;
    int                 dragDistUnit = 0;
    int                 fcID         = 0;
    int                 localOffset  = offset;

    orgWidth                         = scalecoverflow->itemWidth;
    orgHeight                        = scalecoverflow->itemHeight;
    // the default position of center item
    fcX                              = (widget->rect.width / 2) - (orgWidth / 2);
    fcY                              = (widget->rect.height / 2) - (orgHeight / 2);
    for (i = 0; i < count; i++)
    {
        if (hWay)
        {
            posArr[i] = (widget->rect.width / 2) - (orgWidth / 2) - ((coverflow->focusIndex - i) * orgWidth);
        }
        else
        {
            posArr[i] = (widget->rect.height / 2) - (orgHeight / 2) - ((coverflow->focusIndex - i) * orgHeight);
        }
    }

    dragDistUnit = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) ? (localOffset / scalecoverflow->itemHeight)
                                                                        : (localOffset / scalecoverflow->itemWidth);
    fcID         = coverflow->focusIndex - dragDistUnit;

    // (void)printf("fcid %d  ddunit %d offset %d\n", fcID, dragDistUnit, localOffset);

    localOffset  = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) ? (localOffset % scalecoverflow->itemHeight)
                                                                        : (localOffset % scalecoverflow->itemWidth);

    // step initialize
    if (hWay)
    {
        step = (float)(abs(localOffset)) / (float)(scalecoverflow->itemWidth);
    }
    else
    {
        step = (float)(abs(localOffset)) / (float)(scalecoverflow->itemHeight);
    }
    step = step - 1;
    step = step * step * step + 1;

    // 0 <-- focusIndex - 1
    for (i = fcID - 1; i >= 0; i--)
    {
        int dindex = abs(i - fcID);
        factor     = (int)(100 - (100 - scalecoverflow->factor) * step);
        child      = CoverFlowGetVisibleChild(coverflow, i);

        width      = orgWidth * scalecoverflow->factor / 100;
        height     = orgHeight * scalecoverflow->factor / 100;

        // the default position of current child(i);
        if (hWay)
        {
            x = posArr[i] + offset;
            y = fcY;
        }
        else
        {
            x = fcX;
            y = posArr[i] + offset;
        }

        if (dindex == 0)
        {
            width  = orgWidth * factor / 100;
            height = orgHeight * factor / 100;
        }
        else if (dindex == 1)
        {
            if (localOffset > 0)
            {
                factor = (100 - factor + scalecoverflow->factor);
                width  = orgWidth * factor / 100;
                height = orgHeight * factor / 100;
            }
            // else
            //	factor = scalecoverflow->factor;
        }
        // else
        //	factor = scalecoverflow->factor;

        x = x + (orgWidth - width) / 2;
        y = y + (orgHeight - height) / 2;

        if (scalecoverflow->concentration)
        {
            if (dindex != 0)
            {
                if (hWay)
                {
                    x += (scalecoverflow->concentration * dindex);
                }
                else
                {
                    y += (scalecoverflow->concentration * dindex);
                }
            }
        }

        ituWidgetSetPosition(child, x, y);
        // ituWidgetSetPositionFix(coverflow, child, x, y, i);
        ituWidgetSetDimension(child, width, height);
        ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
        child->flags &= ~ITU_CLIP_DISABLED;
        child->flags &= ~ITU_PROGRESS;
        // (void)printf("1[child %d] [%d][%d][%d][%d] [factor %d]\n", i, child->rect.x, child->rect.y, width, height, factor);
    }

    // focusIndex + 1 --> count - 1
    for (i = fcID + 1; i <= count - 1; i++)
    {

        int dindex = abs(i - fcID); // abs(i - fcID - 1);
        factor     = (int)(100 - (100 - scalecoverflow->factor) * step);
        child      = CoverFlowGetVisibleChild(coverflow, i);

        width      = orgWidth * scalecoverflow->factor / 100;
        height     = orgHeight * scalecoverflow->factor / 100;

        // the default position of current child(i);
        if (hWay)
        {
            x = posArr[i] + offset;
            y = fcY;
        }
        else
        {
            x = fcX;
            y = posArr[i] + offset;
        }

        /*if (dindex == 0)
        {
            if (offset < 0)
            {
                width = orgWidth  * factor / 100;
                height = orgHeight  * factor / 100;
            }
        }*/

        if (dindex == 0)
        {
            width  = orgWidth * factor / 100;
            height = orgHeight * factor / 100;
        }
        else if (dindex == 1)
        {
            if (localOffset < 0)
            {
                factor = (100 - factor + scalecoverflow->factor);
                width  = orgWidth * factor / 100;
                height = orgHeight * factor / 100;
            }
            // else
            //	factor = scalecoverflow->factor;
        }
        // else
        //	factor = scalecoverflow->factor;

        x = x + (orgWidth - width) / 2;
        y = y + (orgHeight - height) / 2;

        if (scalecoverflow->concentration)
        {
            if (hWay)
            {
                x -= (scalecoverflow->concentration * (dindex + 1));
            }
            else
            {
                y -= (scalecoverflow->concentration * (dindex + 1));
            }
        }

        ituWidgetSetPosition(child, x, y);
        // ituWidgetSetPositionFix(coverflow, child, x, y, i);
        ituWidgetSetDimension(child, width, height);
        ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
        child->flags &= ~ITU_CLIP_DISABLED;
        child->flags &= ~ITU_PROGRESS;
        // (void)printf("2[child %d] [%d][%d][%d][%d] [factor %d]\n", i, child->rect.x, child->rect.y, width, height, factor);
    }

    // focus here
    child = CoverFlowGetVisibleChild(coverflow, fcID);
    if (child)
    {
        factor = (int)(100 - (100 - scalecoverflow->factor) * step);

        // the default position of current child(i);
        if (hWay)
        {
            x = posArr[fcID] + offset;
            y = fcY;
        }
        else
        {
            x = fcX;
            y = posArr[fcID] + offset;
        }

        width  = orgWidth * factor / 100;
        height = orgHeight * factor / 100;

        x      = x + (orgWidth - width) / 2;
        y      = y + (orgHeight - height) / 2;

        ituWidgetSetPosition(child, x, y);
        ituWidgetSetDimension(child, width, height);
        ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
        child->flags &= ~ITU_CLIP_DISABLED;
        child->flags &= ~ITU_PROGRESS;
    }

    free(posArr);
}

//get the original position for each child
void ituScaleCoverflowCalOP(ITUWidget* widget, ITUWidget* child, int* x, int* y)
{
    ITUScaleCoverFlow* scf = (ITUScaleCoverFlow*)widget;
    ITUCoverFlow* coverflow = (ITUCoverFlow*)widget;
    if ((widget != NULL) && (child != NULL))
    {
        *x = *x - ((scf->itemWidth - child->rect.width) / 2);
        *y = *y - ((scf->itemHeight - child->rect.height) / 2);
    }
}

bool ituScaleCoverFlowUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool                result         = false;
    ITUCoverFlow *      coverflow      = (ITUCoverFlow *)widget;
    ITUScaleCoverFlow * scalecoverflow = (ITUScaleCoverFlow *)widget;
    assert(scalecoverflow);

    if (ev == ITU_EVENT_TIMER && coverflow->inc)
    {
        result = true;
    }
    else if (((ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDERIGHT) &&
              ((coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) == 0)) ||
             ((ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDEDOWN) &&
              (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)))
    {
        int checkSlideDist = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                 ? (widget->rect.height / ITU_SCALECOVERFLOW_SLIDE_CHECK_FACTOR)
                                 : (widget->rect.width / ITU_SCALECOVERFLOW_SLIDE_CHECK_FACTOR);
        if ((arg1 > 0) && (arg1 < checkSlideDist))
        {
            return true;
        }
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        if (scalecoverflow->itemWidth > 0 || scalecoverflow->itemHeight > 0 || scalecoverflow->itemPos > 0)
        {
            int i, count = CoverFlowGetVisibleChildCount(coverflow);

            for (i = 0; i < count; ++i)
            {
                ITUWidget * child = CoverFlowGetVisibleChild(coverflow, i);

                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    // ituWidgetSetX(child, scalecoverflow->itemPos);
                    ituWidgetSetPositionFix(coverflow, child, scalecoverflow->itemPos, child->rect.y, i);
                }
                else
                {
                    // ituWidgetSetY(child, scalecoverflow->itemPos);
                    ituWidgetSetPositionFix(coverflow, child, child->rect.x, scalecoverflow->itemPos, i);
                }

                ituWidgetSetDimension(child, scalecoverflow->itemWidth, scalecoverflow->itemHeight);
                ituScaleCoverflowScaleWidgetChild(child,
                                                  ituScaleCoverflowScaleFactor(child, scalecoverflow->itemWidth));
            }

            if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                coverflow->min_change_dist_factor = 1;
            }
        }
    }

    if (result && (!(widget->flags & ITU_UNDRAGGING)))
    {
        int count = CoverFlowGetVisibleChildCount(coverflow);

        if ((coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE) || (coverflow->inc))
        {
            int i = 0;
            for (; i < count; ++i)
            {
                ITUWidget * child = CoverFlowGetVisibleChild(coverflow, i);

                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if (i == coverflow->focusIndex)
                    {
                        ituWidgetSetX(child, scalecoverflow->itemPos);
                    }
                    else
                    {
                        ituWidgetSetX(child, scalecoverflow->itemPos +
                                                 (child->org_rect.width - scalecoverflow->itemWidth) / 2);
                    }
                }
                else
                {
                    if (i == coverflow->focusIndex)
                    {
                        ituWidgetSetY(child, scalecoverflow->itemPos);
                    }
                    else
                    {
                        ituWidgetSetY(child, scalecoverflow->itemPos +
                                                 (child->org_rect.height - scalecoverflow->itemHeight) / 2);
                    }
                }

                ituWidgetSetDimension(child, scalecoverflow->itemWidth, scalecoverflow->itemHeight);
                ituScaleCoverflowScaleWidgetChild(child,
                                                  ituScaleCoverflowScaleFactor(child, scalecoverflow->itemWidth));
            }
        }
    }

    result |= ituCoverFlowUpdate(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_TIMER)
    {
        if ((coverflow->inc == 0) && (coverflow->frame == 0) && scalecoverflow->draggable)
        {
            if (!(widget->flags & ITU_DRAGGABLE))
            {
                widget->flags |= ITU_DRAGGABLE;
                // (void)printf("reset not draggable!\n");
            }
        }

        if (coverflow->inc != 0)
        {
            int i, count = CoverFlowGetVisibleChildCount(coverflow);

            if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                if ((coverflow->focusIndex == 0) && (coverflow->inc > 0))
                {
                    coverflow->inc = 0;
                }
                else if ((coverflow->focusIndex == (count - 1)) && (coverflow->inc < 0))
                {
                    coverflow->inc = 0;
                }
                else
                { //<<< TIMER for non-cycle >>>
                    int avail_count, index, orgWidth, orgHeight, x, y;
                    int scalWidth, scalHeight;
                    int frame = coverflow->frame - 1;

                    if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        avail_count = (widget->rect.height / scalecoverflow->itemHeight);
                    }
                    else
                    {
                        avail_count = (widget->rect.width / scalecoverflow->itemWidth);
                    }

                    index     = coverflow->focusIndex - (avail_count / 2) - 1;
                    orgWidth  = scalecoverflow->itemWidth;
                    orgHeight = scalecoverflow->itemHeight;

                    for (i = 0; i < (avail_count + 2); i++)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverflow, index);

                        if (child == NULL)
                        {
                            index++;
                            continue;
                        }

                        if (index == coverflow->focusIndex)
                        {
                            float step = (float)frame / coverflow->totalframe;
                            int   factor_dx, factor_dy, dx, dy;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                factor_dx = (int)((0.5 - (float)scalecoverflow->factor / 200.0) * orgWidth / 2.0);
                                factor_dy =
                                    (int)((0.5 - (float)scalecoverflow->factor / 200.0) * orgHeight / 2.0) + orgHeight;
                            }
                            else
                            {
                                if (coverflow->inc > 0)
                                {
                                    factor_dx = orgWidth + ((100 - scalecoverflow->factor) * orgWidth / 200);
                                }
                                else
                                {
                                    factor_dx = (orgWidth * scalecoverflow->factor / 100) +
                                                ((100 - scalecoverflow->factor) * orgWidth / 200);
                                }

                                factor_dy = (100 - scalecoverflow->factor) * orgHeight / 200;
                            }

                            dx = (int)(factor_dx * step);
                            dy = (int)(factor_dy * step);

                            scalWidth =
                                (int)((100.0 - ((100 - scalecoverflow->factor) * step)) * (float)orgWidth / 100.0);
                            scalHeight =
                                (int)((100.0 - ((100 - scalecoverflow->factor) * step)) * (float)orgHeight / 100.0);

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                x = (widget->rect.width / 2 - orgWidth / 2) + dx;
                                y = (widget->rect.height / 2 - orgHeight / 2) +
                                    dy * ((coverflow->inc > 0) ? (1) : (-1));
                            }
                            else
                            {
                                x = (widget->rect.width / 2 - orgWidth / 2) +
                                    (dx * ((coverflow->inc > 0) ? (1) : (-1)));
                                y = dy;
                            }

                            // ituWidgetSetPosition(child, x, y);
                            ituWidgetSetPositionFix(coverflow, child, x, y, index);
                            ituWidgetSetDimension(child, scalWidth, scalHeight);
                            ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, scalWidth));
                        }
                        else if (((coverflow->focusIndex - index) == 1) && (coverflow->inc > 0))
                        { // the item (focus - 1) that will become focus
                            float step = (float)frame / coverflow->totalframe;
                            int   factor_dx, factor_dy, dx, dy;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                factor_dx =
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgWidth / 100.0) / 2);
                                factor_dy =
                                    (int)(orgHeight * scalecoverflow->factor / 100) +
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgHeight / 100.0) / 2);
                            }
                            else
                            {
                                factor_dx =
                                    (int)(orgWidth * scalecoverflow->factor / 100) +
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgWidth / 100.0) / 2);
                                factor_dy =
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgHeight / 100.0) / 2);
                            }

                            dx         = (int)(factor_dx * step);
                            dy         = (int)(factor_dy * step);

                            scalWidth  = (int)((100.0 - (scalecoverflow->factor * (1.0 - step))) * orgWidth / 100.0);
                            scalHeight = (int)((100.0 - (scalecoverflow->factor * (1.0 - step))) * orgHeight / 100.0);

                            if (coverflow->inc > 0)
                            {
                                child->flags &= ~ITU_PROGRESS;
                            }
                            else
                            {
                            }
                            // child->flags |= ITU_CLIP_DISABLED;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                x = (orgWidth / 2 - (orgWidth * scalecoverflow->factor / 100) / 2) - dx;
                                y = (widget->rect.height / 2 - orgHeight / 2) - factor_dy + dy;
                            }
                            else
                            {
                                x = (widget->rect.width / 2 - orgWidth / 2) - factor_dx + dx;
                                y = (orgHeight / 2 - (orgHeight * scalecoverflow->factor / 100) / 2) - dy;
                            }

                            // ituWidgetSetPosition(child, x, y);
                            ituWidgetSetPositionFix(coverflow, child, x, y, index);
                            ituWidgetSetDimension(child, scalWidth, scalHeight);
                            ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, scalWidth));
                        }
                        else if (((coverflow->focusIndex - index) == -1) && (coverflow->inc < 0))
                        { // the item (focus + 1) that will become focus
                            float step = (float)frame / coverflow->totalframe;
                            int   factor_dx, factor_dy, dx, dy;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                factor_dx =
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgWidth / 100.0) / 2);
                                factor_dy = (int)(orgHeight * (100 - scalecoverflow->factor) / 200) + orgHeight;
                            }
                            else
                            {
                                factor_dx = (int)(orgWidth * (100 - scalecoverflow->factor) / 200) + orgWidth;
                                factor_dy =
                                    (int)(((float)(100 - scalecoverflow->factor) * (float)orgHeight / 100.0) / 2);
                            }

                            dx         = (int)(factor_dx * step);
                            dy         = (int)(factor_dy * step);

                            scalWidth  = (int)((100.0 - (scalecoverflow->factor * (1.0 - step))) * orgWidth / 100.0);
                            scalHeight = (int)((100.0 - (scalecoverflow->factor * (1.0 - step))) * orgHeight / 100.0);

                            if (coverflow->inc > 0)
                            {
                                child->flags &= ~ITU_PROGRESS;
                            }
                            else
                            {
                            }
                            // child->flags |= ITU_CLIP_DISABLED;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                x = (orgWidth / 2 - (orgWidth * scalecoverflow->factor / 100) / 2) - dx;
                                y = (widget->rect.height / 2 + orgHeight / 2) +
                                    (orgHeight * (100 - scalecoverflow->factor) / 200) - dy;
                            }
                            else
                            {
                                x = (widget->rect.width / 2 + orgWidth / 2) +
                                    (orgWidth * (100 - scalecoverflow->factor) / 200) - dx;
                                y = (orgHeight / 2 - (orgHeight * scalecoverflow->factor / 100) / 2) - dy;
                            }

                            // ituWidgetSetPosition(child, x, y);
                            ituWidgetSetPositionFix(coverflow, child, x, y, index);
                            ituWidgetSetDimension(child, scalWidth, scalHeight);
                            ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, scalWidth));
                        }
                        else if (coverflow->inc > 0)
                        { // press (<<<)    motion -> -> ->
                            int   item_dist = coverflow->focusIndex - index;
                            float step      = (float)frame / coverflow->totalframe;
                            int   factor_dx, factor_dy, dx, dy;
                            int   obj_w = scalecoverflow->factor * orgWidth / 100;
                            int   obj_h = scalecoverflow->factor * orgHeight / 100;

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                factor_dx = 0;
                                factor_dy = orgHeight;
                            }
                            else
                            {
                                factor_dx = orgWidth;
                                factor_dy = 0;
                            }

                            dx = (int)(factor_dx * step);
                            dy = (int)(factor_dy * step);

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                x = (orgWidth - obj_w) / 2;
                                y = (widget->rect.width / 2 - orgWidth / 2) - (item_dist * orgHeight) +
                                    ((orgHeight - obj_h) / 2) + dy;
                            }
                            else
                            {
                                x = (widget->rect.width / 2 - orgWidth / 2) - (item_dist * orgWidth) +
                                    ((orgWidth - obj_w) / 2) + dx;
                                y = (orgHeight - obj_h) / 2;
                            }

                            ituWidgetSetDimension(child, obj_w, obj_h);
                            ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, obj_w));
                            // ituWidgetSetPosition(child, x, y);
                            ituWidgetSetPositionFix(coverflow, child, x, y, index);
                        }
                        else
                        {
                            int   item_dist = coverflow->focusIndex - index;
                            float step      = (float)frame / coverflow->totalframe;
                            int   factor_dx, factor_dy, dx, dy;
                            int   obj_w = (int)((float)scalecoverflow->factor * orgWidth / 100);
                            int   obj_h = (int)((float)scalecoverflow->factor * orgHeight / 100);

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                factor_dx = 0;
                                factor_dy = orgHeight;
                            }
                            else
                            {
                                factor_dx = orgWidth;
                                factor_dy = 0;
                            }

                            dx = (int)(factor_dx * step);
                            dy = (int)(factor_dy * step);

                            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                x = (orgWidth - obj_w) / 2;
                                y = (widget->rect.height / 2 - orgHeight / 2) - (item_dist * orgHeight) +
                                    ((orgHeight - obj_h) / 2) - dy;
                            }
                            else
                            {
                                x = (widget->rect.width / 2 - orgWidth / 2) - (item_dist * orgWidth) +
                                    ((orgWidth - obj_w) / 2) - dx;
                                y = (orgHeight - obj_h) / 2;
                            }

                            ituWidgetSetDimension(child, obj_w, obj_h);
                            ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, obj_w));
                            // ituWidgetSetPosition(child, x, y);
                            ituWidgetSetPositionFix(coverflow, child, x, y, index);

                            // if (index == 2)
                            //	(void)printf("[child %d] [%d, %d] [%d, %d]\n", index, x, y, obj_w, obj_h);
                        }

                        // if (index == 0)
                        // (void)printf("child [%d] y %d\n", index, child->rect.y);
                        index++;
                    }
                }
            }
            else if (coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
            {
                int orgWidth = scalecoverflow->itemWidth;
                int orgHeight = scalecoverflow->itemHeight;
                int factor, width, height, x, y, frame = coverflow->frame - 1;
                        float step = (float)frame / coverflow->totalframe;
                        step       = step - 1;
                        step       = step * step * step + 1;
                for (i = 0; i < count; ++i)
                    {
                    ITUWidget* child = CoverFlowGetVisibleChild(coverflow, i);
                    int i_next = ((coverflow->focusIndex + 1) >= count) ? (0) : (coverflow->focusIndex + 1);
                    int i_prev = ((coverflow->focusIndex - 1) < 0) ? (count - 1) : (coverflow->focusIndex - 1);
                    if (i == coverflow->focusIndex)
                        {
                        factor = (int)(100 - (100 - scalecoverflow->factor) * step);
                        }
                    else if ((i == i_next) && (coverflow->inc < 0))
                        {
                        factor = (int)(scalecoverflow->factor + (100 - scalecoverflow->factor) * step);
                            }
                    else if ((i == i_prev) && (coverflow->inc > 0))
                            {
                                factor     = (int)(scalecoverflow->factor + (100 - scalecoverflow->factor) * step);
                            }
                            else
                            {
                                factor = scalecoverflow->factor;
                            }

                        width  = orgWidth * factor / 100;
                        height = orgHeight * factor / 100;

                        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                        {
                        x = (widget->rect.width / 2 - orgWidth / 2) + ((orgWidth - width) / 2);
                        y = child->rect.y + ((orgHeight - height) / 2);
                            }
                            else
                            {
                        x = child->rect.x + ((orgWidth - width) / 2);
                        y = (widget->rect.height / 2 - orgHeight / 2) + ((orgHeight - height) / 2);
                            }
                        ituWidgetSetDimension(child, width, height);
                    ituWidgetSetPosition(child, x, y);
                        ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
                    }
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEDOWN)
    {
        int x = arg2 - widget->rect.x;
        int y = arg3 - widget->rect.y;
        int i = 0, count = CoverFlowGetVisibleChildCount(coverflow);
        for (i = 0; i < count; i++)
        {
            ITUWidget* child = CoverFlowGetVisibleChild(coverflow, i);
            scalecoverflow->layoutMemoX[i] = child->rect.x - ((scalecoverflow->itemWidth - child->rect.width) / 2);
            scalecoverflow->layoutMemoY[i] = child->rect.y - ((scalecoverflow->itemHeight - child->rect.height) / 2);
            // (void)printf("child %d,  %d, %d\n", i, child->rect.x, child->rect.y);
        }

        if (scalecoverflow->draggable && (ituWidgetIsEnabled(widget)) && (ituWidgetIsInside(widget, x, y)))
        {
            widget->flags |= ITU_DRAGGABLE;
        }

        if ((widget->flags & ITU_DRAGGABLE) && (ituWidgetIsEnabled(widget)) && (ituWidgetIsInside(widget, x, y)))
        {
            coverflow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
            coverflow->frame = 0;

            scalecoverflow->currentFocusIndex = coverflow->focusIndex;

            widget->flags |= ITU_DRAGGING;
            ituScene->dragged = widget;
        }
    }
    else if (ev == ITU_EVENT_MOUSEUP)
    {
        if (widget->flags & ITU_DRAGGABLE)
        {
            if (widget->flags & ITU_UNDRAGGING)
            {
                //return true;
            }
            //coverflow->inc   = 0;
            //coverflow->frame = 0;

            widget->flags |= ITU_UNDRAGGING;
            ituScene->dragged = NULL;

            // ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
            // widget->flags &= ~ITU_DRAGGABLE;

            if (scalecoverflow->currentFocusIndex != coverflow->focusIndex)
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                // workaround for coverflow use same item size to calculate distance but scalecoverflow not.
                if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                {
                    ITUWidget * childf = NULL;
                    bool        hway   = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) ? (false) : (true);

                    if (coverflow->focusIndex < 0)
                    {
                        coverflow->focusIndex = 0;
                    }
                    else if (coverflow->focusIndex > itcTreeGetChildCount(coverflow) - 1)
                    {
                        coverflow->focusIndex = itcTreeGetChildCount(coverflow) - 1;
                    }

                    childf = itcTreeGetChildAt(coverflow, coverflow->focusIndex);

                    if (hway)
                    {
                        int cfx   = ituWidgetGetX(childf) + (ituWidgetGetWidth(childf) / 2);
                        int index = (cfx - (ituWidgetGetWidth(scalecoverflow) / 2)) / childf->org_rect.width;
                        if ((coverflow->focusIndex - index) >= 0 &&
                            (coverflow->focusIndex - index) <= itcTreeGetChildCount(coverflow) - 1)
                        {
                            coverflow->focusIndex -= index;
                        }

                        childf = itcTreeGetChildAt(coverflow, coverflow->focusIndex);
                        cfx    = ituWidgetGetX(childf) + (ituWidgetGetWidth(childf) / 2);

                        if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) > 0 &&
                            coverflow->focusIndex > 0)
                        {
                            ITUWidget * child0 = itcTreeGetChildAt(coverflow, coverflow->focusIndex - 1);
                            if (abs(cfx - (ituWidgetGetWidth(scalecoverflow) / 2)) >=
                                abs(ituWidgetGetX(child0) + (ituWidgetGetWidth(child0) / 2) -
                                    (ituWidgetGetWidth(scalecoverflow) / 2)))
                            {
                                coverflow->focusIndex -= 1;
                            }
                        }
                        else if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) < 0 &&
                                 coverflow->focusIndex < itcTreeGetChildCount(coverflow) - 1)
                        {
                            ITUWidget * child1 = itcTreeGetChildAt(coverflow, coverflow->focusIndex + 1);
                            if (abs(cfx - (ituWidgetGetWidth(scalecoverflow) / 2)) >=
                                abs(ituWidgetGetX(child1) + (ituWidgetGetWidth(child1) / 2) -
                                    (ituWidgetGetWidth(scalecoverflow) / 2)))
                            {
                                coverflow->focusIndex += 1;
                            }
                        }
                    }
                    else
                    {
                        int cfy   = ituWidgetGetY(childf) + (ituWidgetGetHeight(childf) / 2);
                        int index = (cfy - (ituWidgetGetHeight(scalecoverflow) / 2)) / childf->org_rect.height;
                        if ((coverflow->focusIndex - index) >= 0 &&
                            (coverflow->focusIndex - index) <= itcTreeGetChildCount(coverflow) - 1)
                        {
                            coverflow->focusIndex -= index;
                        }

                        childf = itcTreeGetChildAt(coverflow, coverflow->focusIndex);
                        cfy    = ituWidgetGetY(childf) + (ituWidgetGetHeight(childf) / 2);

                        if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) > 0 &&
                            coverflow->focusIndex > 0)
                        {
                            ITUWidget * child0 = itcTreeGetChildAt(coverflow, coverflow->focusIndex - 1);
                            if (abs(cfy - (ituWidgetGetHeight(scalecoverflow) / 2)) >=
                                abs(ituWidgetGetY(child0) + (ituWidgetGetHeight(child0) / 2) -
                                    (ituWidgetGetHeight(scalecoverflow) / 2)))
                            {
                                coverflow->focusIndex -= 1;
                            }
                        }
                        else if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) < 0 &&
                                 coverflow->focusIndex < itcTreeGetChildCount(coverflow) - 1)
                        {
                            ITUWidget * child1 = itcTreeGetChildAt(coverflow, coverflow->focusIndex + 1);
                            if (abs(cfy - (ituWidgetGetHeight(scalecoverflow) / 2)) >=
                                abs(ituWidgetGetY(child1) + (ituWidgetGetHeight(child1) / 2) -
                                    (ituWidgetGetHeight(scalecoverflow) / 2)))
                            {
                                coverflow->focusIndex += 1;
                            }
                        }
                    }

                    // if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) > 2)
                    //	coverflow->focusIndex -= (coverflow->focusIndex - scalecoverflow->currentFocusIndex - 2);
                    // else if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) < -2)
                    //	coverflow->focusIndex -= (coverflow->focusIndex - scalecoverflow->currentFocusIndex + 2);
                    // else if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) ==  2)
                    //	coverflow->focusIndex -= (coverflow->focusIndex - scalecoverflow->currentFocusIndex - 1);
                    // else if ((coverflow->focusIndex - scalecoverflow->currentFocusIndex) == -2)
                    //	coverflow->focusIndex -= (coverflow->focusIndex - scalecoverflow->currentFocusIndex + 1);

                    if (ituWidgetIsInside(widget, x, y))
                    {
                        ituExecActions(widget, coverflow->actions, ITU_EVENT_CHANGED, coverflow->focusIndex);
                    }
                }
            }
            else
            {
                coverflow->inc   = 0;
                coverflow->frame = 0;
            }
            ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
        }
    }
    else if (ev == ITU_EVENT_MOUSEMOVE)
    {

        if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGING))
        {
            int   i, orgWidth, orgHeight, count = CoverFlowGetVisibleChildCount(coverflow);
            int   neib;
            float step;
            int   xm = arg2 - widget->rect.x;
            int   ym = arg3 - widget->rect.y;
            // (void)printf("move f %d, org %d\n", coverflow->focusIndex, scalecoverflow->currentFocusIndex);
            // fix for drag outside widget
            if (!(ituWidgetIsInside(scalecoverflow, xm, ym)))
            {
                ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
                // ituWidgetUpdate(scalecoverflow, ITU_EVENT_MOUSEDOWN, 0, arg2, arg3);
                return true;
            }

            if (scalecoverflow->currentFocusIndex != coverflow->focusIndex)
            {
                ituExecActions(widget, coverflow->actions, ITU_EVENT_CHANGED, coverflow->focusIndex);
                ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
                ituWidgetUpdate(scalecoverflow, ITU_EVENT_MOUSEDOWN, 0, arg2, arg3);
            }

            if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
            {
                scalecoverflow->offsetx = 0;
                scalecoverflow->offsety = ym - coverflow->touchPos;

                if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                {
                    if (((coverflow->focusIndex == 0) && (scalecoverflow->offsety > 0)) ||
                        ((coverflow->focusIndex == (count - 1)) && (scalecoverflow->offsety < 0)))
                    {
                        ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
                        return true;
                    }
                }

                step = (float)(abs(scalecoverflow->offsety)) / (float)(scalecoverflow->itemHeight);

                neib = ((scalecoverflow->offsety >= 0) ? (1) : (-1));
                neib = coverflow->focusIndex - neib;

                if (neib < 0)
                {
                    neib = count - 1;
                }
                else if (neib >= count)
                {
                    neib = 0;
                }
            }
            else
            {
                scalecoverflow->offsetx = xm - coverflow->touchPos;
                scalecoverflow->offsety = 0;

                if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                {
                    if (((coverflow->focusIndex == 0) && (scalecoverflow->offsetx > 0)) ||
                        ((coverflow->focusIndex == (count - 1)) && (scalecoverflow->offsetx < 0)))
                    {
                        ituWidgetUpdate(scalecoverflow, ITU_EVENT_LAYOUT, 0, 0, 0);
                        return true;
                    }
                }

                step = (float)(abs(scalecoverflow->offsetx)) / (float)(scalecoverflow->itemWidth);

                neib = ((scalecoverflow->offsetx >= 0) ? (1) : (-1));
                neib = coverflow->focusIndex - neib;

                if (neib < 0)
                {
                    neib = count - 1;
                }
                else if (neib >= count)
                {
                    neib = 0;
                }
            }

            // (void)printf("focus is %d neib is %d, tpos %d\n", coverflow->focusIndex, neib, coverflow->touchPos);

            orgWidth  = scalecoverflow->itemWidth;
            orgHeight = scalecoverflow->itemHeight;

            if (coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
            {
                int factor, width, height;
                int   offset = (scalecoverflow->offsetx == 0) ? (scalecoverflow->offsety) : (scalecoverflow->offsetx);
                step   = step - 1;
                step   = step * step * step + 1;

                for (i = 0; i < count; ++i)
                {
                    ITUWidget * child = CoverFlowGetVisibleChild(coverflow, i);
                    int i_next = ((coverflow->focusIndex + 1) >= count) ? (0) : (coverflow->focusIndex + 1);
                    int i_prev = ((coverflow->focusIndex - 1) < 0) ? (count - 1) : (coverflow->focusIndex - 1);
                    int x = child->rect.x;
                    int y = child->rect.y;
                    if (i == coverflow->focusIndex)
                    {
                        factor = (int)(100 - (100 - scalecoverflow->factor) * step);
                    }
                    else if ((i == i_next) && (offset < 0))
                    {
                        factor = (int)(scalecoverflow->factor + (100 - scalecoverflow->factor) * step);
                    }
                    else if ((i == i_prev) && (offset > 0))
                        {
                        factor = (int)(scalecoverflow->factor + (100 - scalecoverflow->factor) * step);
                            }
                    else
                        {
                        factor = scalecoverflow->factor;
                    }

                    width = orgWidth * factor / 100;
                    height = orgHeight * factor / 100;

                    //ituScaleCoverflowCalOP(widget, child, &x, &y);

                    if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        x = (widget->rect.width / 2 - orgWidth / 2) + ((orgWidth - width) / 2);
                        y = y + ((orgHeight - height) / 2);
                    }
                    else
                    {
                        x = child->rect.x + ((orgWidth - width) / 2);
                        y = (widget->rect.height - height) / 2;
                        //x = x + ((orgWidth - width) / 2);
                        //y = (widget->rect.height / 2 - orgHeight / 2) + ((orgHeight - height) / 2);
                    }
                    //ituWidgetSetDimension(child, orgWidth, orgHeight);
                    ituWidgetSetDimension(child, width, height);
                    ituWidgetSetPosition(child, x, y);
                    ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
                }
            }
            else
            {
                int * posArr = (int *)malloc(sizeof(int) * count);
                bool  hWay   = (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) ? (false) : (true);
                int   offset = (scalecoverflow->offsetx == 0) ? (scalecoverflow->offsety) : (scalecoverflow->offsetx);
                bool  unDrag = false;
                // the default position of center item
                int   fcX    = (widget->rect.width / 2) - (orgWidth / 2);
                int   fcY    = (widget->rect.height / 2) - (orgHeight / 2);
                for (i = 0; i < count; i++)
                {
                    if (hWay)
                    {
                        posArr[i] =
                            (widget->rect.width / 2) - (orgWidth / 2) - ((coverflow->focusIndex - i) * orgWidth);
                    }
                    else
                    {
                        posArr[i] =
                            (widget->rect.height / 2) - (orgHeight / 2) - ((coverflow->focusIndex - i) * orgHeight);
                    }
                }
                // check the drag distance limit
                if (hWay)
                {
                    if ((offset > 0) && (posArr[0] + offset) >= fcX)
                    {
                        unDrag = true;
                    }
                    else if ((offset < 0) && (posArr[count - 1] + offset) <= fcX)
                    {
                        unDrag = true;
                    }
                }
                else
                {
                    if ((offset > 0) && (posArr[0] + offset) >= fcY)
                    {
                        unDrag = true;
                    }
                    else if ((offset < 0) && (posArr[count - 1] + offset) <= fcY)
                    {
                        unDrag = true;
                    }
                }
                free(posArr);

                if (unDrag)
                {
                    ituWidgetUpdate(scalecoverflow, ITU_EVENT_MOUSEUP, 0, 0, 0);
                    return true;
                }

                ituScaleCoverflowLayoutCal(widget, offset);
                return true;
            }
        }
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        int orgWidth, orgHeight, count = CoverFlowGetVisibleChildCount(coverflow);

        if (scalecoverflow->itemWidth == 0 && scalecoverflow->itemHeight == 0 && scalecoverflow->itemPos == 0)
        {
            ITUWidget * child          = CoverFlowGetVisibleChild(coverflow, coverflow->focusIndex);

            if (child != NULL)
            {
                scalecoverflow->itemWidth = child->rect.width;
                scalecoverflow->itemHeight = child->rect.height;

                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    scalecoverflow->itemPos = child->rect.x;
                    coverflow->orgItemSize = child->rect.height;
                }
                else
                {
                    scalecoverflow->itemPos = child->rect.y;
                    coverflow->orgItemSize = child->rect.width;
                }

                if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                {
                    coverflow->min_change_dist_factor = 1;
                }
            }
        }
        orgWidth  = scalecoverflow->itemWidth;
        orgHeight = scalecoverflow->itemHeight;

        if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
        {
            ituScaleCoverflowLayoutCal(widget, 0);
        }
        else // if (coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
        {
            int i = 0;
            for (; i < count; ++i)
            {
                ITUWidget* child = CoverFlowGetVisibleChild(coverflow, i);
                int x, y, factor, width, height;
                int i_next = ((coverflow->focusIndex + 1) >= count) ? (0) : (coverflow->focusIndex + 1);
                int i_prev = ((coverflow->focusIndex - 1) < 0) ? (count - 1) : (coverflow->focusIndex - 1);
                if (i == coverflow->focusIndex)
                    {
                        factor = 100;
                    }
                    else
                    {
                        factor = scalecoverflow->factor;
                    }

                    width  = orgWidth * factor / 100;
                    height = orgHeight * factor / 100;

                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    x = (widget->rect.width / 2 - orgWidth / 2) + ((orgWidth - width) / 2);
                    y = child->rect.y + ((orgHeight - height) / 2);
                }
                else
                {
                    x = child->rect.x + ((orgWidth - width) / 2);
                    y = (widget->rect.height / 2 - orgHeight / 2) + ((orgHeight - height) / 2);
            }
                    ituWidgetSetDimension(child, width, height);
                ituWidgetSetPosition(child, x, y);
                    ituScaleCoverflowScaleWidgetChild(child, ituScaleCoverflowScaleFactor(child, width));
            }
        }
    }

    return widget->visible ? result : false;
}

void ituScaleCoverFlowDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    ITUCoverFlow * coverflow = (ITUCoverFlow *)widget;
    ITURectangle   prevClip;
    ITURectangle * rect = (ITURectangle *)&widget->rect;
    ITCTree *      node;
    assert(widget);
    assert(dest);

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    x += widget->rect.x;
    y += widget->rect.y;
    alpha = alpha * widget->alpha / 255;

    if (alpha)
    {
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, ITU_ARGB8888, NULL, 0);
        if (surf)
        {
            ituColorFill(surf, 0, 0, rect->width, rect->height, &widget->color);

            if (alpha == 255)
            {
                ituBitBlt(dest, x, y, rect->width, rect->height, surf, 0, 0);
            }
            else
            {
                ituAlphaBlend(dest, x, y, rect->width, rect->height, surf, 0, 0, alpha);
            }

            ituDestroySurface(surf);
        }
    }

    if (!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
    {
        ITUWidget * fChild = (ITUWidget *)widget->tree.child;

        for (node = widget->tree.child; node; node = node->sibling)
        {
            ITUWidget * child = (ITUWidget *)node;
            if ((child->rect.width) > (fChild->rect.width))
            {
                fChild = child;
            }
        }

        for (node = widget->tree.child; node; node = node->sibling)
        {
            ITUWidget * child = (ITUWidget *)node;
            if (child->visible && !(child->flags & ITU_PROGRESS) && !(child->flags & ITU_CLIP_DISABLED) &&
                ituWidgetIsOverlapClipping(child, dest, x, y))
            {
                if (child != fChild)
                {
                    ituWidgetDraw(node, dest, x, y, alpha);
                }
            }

            child->dirty = false;
        }
        ituWidgetDraw(fChild, dest, x, y, alpha);

        /*
        for (node = widget->tree.child; node; node = node->sibling)
        {
        ITUWidget* child = (ITUWidget*)node;
        if (child->visible && !(child->flags & ITU_PROGRESS) && !(child->flags & ITU_CLIP_DISABLED) &&
        ituWidgetIsOverlapClipping(child, dest, x, y)) ituWidgetDraw(node, dest, x, y, alpha);

        child->dirty = false;
        }

        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
        avail_count = (widget->rect.height / scalecoverflow->itemHeight);
        }
        else
        {
        avail_count = (widget->rect.width / scalecoverflow->itemWidth);
        }

        index = coverflow->focusIndex - (avail_count / 2) - 1;

        for (i = 0; i < (avail_count + 2); i++)
        {
        ITUWidget* child = CoverFlowGetVisibleChild(coverflow, index);

        if (child == NULL)
        {
        index++;
        continue;
        }

        if (child->visible && !(child->flags & ITU_PROGRESS) && !(child->flags & ITU_CLIP_DISABLED) &&
        ituWidgetIsOverlapClipping(child, dest, x, y)) ituWidgetDraw(child, dest, x, y, alpha);

        child->dirty = false;

        index++;
        }
        */
    }
    else
    {
        int count  = CoverFlowGetVisibleChildCount(coverflow);
        int count2 = count / 2;
        int i, index;
        for (node = widget->tree.child; node; node = node->sibling)
        {
            ITUWidget * child = (ITUWidget *)node;
            if (child->visible && !(child->flags & ITU_PROGRESS) && (child->flags & ITU_CLIP_DISABLED) &&
                ituWidgetIsOverlapClipping(child, dest, x, y))
            {
                ituWidgetDraw(node, dest, x, y, alpha);
            }

            child->dirty = false;
        }

        if ((coverflow->inc < 0 && coverflow->frame < coverflow->totalframe / 2) ||
            (coverflow->inc >= 0 && coverflow->frame >= coverflow->totalframe / 2))
        {
            count2++;
        }

        if (coverflow->focusIndex - count2 >= 0)
        {
            index = coverflow->focusIndex - count2;
        }
        else
        {
            index = count - 1 - (count2 - 1 - coverflow->focusIndex);
        }

        // (void)printf("focus=%d start=%d ", coverflow->focusIndex, index);

        if (index < 0)
        {
            index = 0;
        }

        for (i = 0; i < count2; ++i)
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverflow, index);

            if (child->visible && !(child->flags & ITU_PROGRESS) && !(child->flags & ITU_CLIP_DISABLED) &&
                ituWidgetIsOverlapClipping(child, dest, x, y))
            {
                ituWidgetDraw(child, dest, x, y, alpha);
            }

            child->dirty = false;

            if (++index >= count)
            {
                index = 0;
            }
        }

        count2 = count - count2;

        if (coverflow->focusIndex + count2 - 1 < count)
        {
            index = coverflow->focusIndex + count2 - 1;
        }
        else
        {
            index = count2 - 1 - (count - coverflow->focusIndex);
        }

        // (void)printf("end=%d\n", index);
        if (index < 0)
        {
            index = 0;
        }

        for (i = 0; i < count2; ++i)
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverflow, index);

            if (child->visible && !(child->flags & ITU_PROGRESS) && !(child->flags & ITU_CLIP_DISABLED) &&
                ituWidgetIsOverlapClipping(child, dest, x, y))
            {
                ituWidgetDraw(child, dest, x, y, alpha);
            }

            child->dirty = false;

            if (--index < 0)
            {
                index = count - 1;
            }
        }
    }

    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    ituDirtyWidget(widget, false);
}

void ituScaleCoverFlowInit (ITUScaleCoverFlow * scaleCoverFlow, ITULayout layout)
{
    assert(scaleCoverFlow);
    ITU_ASSERT_THREAD();

    (void)memset(scaleCoverFlow, 0, sizeof(ITUScaleCoverFlow));

    ituCoverFlowInit(&scaleCoverFlow->coverFlow, layout);

    ituWidgetSetType(scaleCoverFlow, ITU_SCALECOVERFLOW);
    ituWidgetSetName(scaleCoverFlow, scaleCoverFlowName);
    ituWidgetSetUpdate(scaleCoverFlow, ituScaleCoverFlowUpdate);
    ituWidgetSetDraw(scaleCoverFlow, ituScaleCoverFlowDraw);
    ituWidgetSetOnAction(scaleCoverFlow, ituScaleCoverFlowOnAction);
}

void ituScaleCoverFlowLoad (ITUScaleCoverFlow * scaleCoverFlow, uint32_t base)
{
    assert(scaleCoverFlow);

    if (!(scaleCoverFlow->coverFlow.coverFlowFlags & ITU_COVERFLOW_CYCLE))
    {
        scaleCoverFlow->coverFlow.boundaryAlign = 0;
    }

    if (scaleCoverFlow->draggable)
    {
        // ITUWidget* widget = (ITUWidget*)(&(scaleCoverFlow->coverFlow));
        // widget->flags |= ITU_DRAGGABLE;
        scaleCoverFlow->orgFocusIndex = scaleCoverFlow->coverFlow.focusIndex;
    }

    ituCoverFlowLoad(&scaleCoverFlow->coverFlow, base);
    ituWidgetSetUpdate(scaleCoverFlow, ituScaleCoverFlowUpdate);
    ituWidgetSetDraw(scaleCoverFlow, ituScaleCoverFlowDraw);
    ituWidgetSetOnAction(scaleCoverFlow, ituScaleCoverFlowOnAction);
}

void ituScaleCoverFlowOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    ITUScaleCoverFlow * scalecoverFlow = (ITUScaleCoverFlow *)widget;
    assert(scalecoverFlow);

    switch (action)
    {
        case ITU_ACTION_PREV:
            widget->flags &= ~ITU_DRAGGABLE;
            ituCoverFlowPrev((ITUCoverFlow *)widget);
            break;

        case ITU_ACTION_NEXT:
            widget->flags &= ~ITU_DRAGGABLE;
            ituCoverFlowNext((ITUCoverFlow *)widget);
            break;

        default:
            ituCoverFlowOnAction(widget, action, param);
            break;
    }
}

void ituScaleCoverFlowNext (ITUScaleCoverFlow * scalecoverflow)
{
    ituCoverFlowNext((ITUCoverFlow *)scalecoverflow);
}

void ituScaleCoverFlowPrev (ITUScaleCoverFlow * scalecoverflow)
{
    ituCoverFlowPrev((ITUCoverFlow *)scalecoverflow);
}
