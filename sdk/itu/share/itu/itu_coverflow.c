#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/time.h>
#include "ite/itp.h"
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define COVERFLOW_FAST_SLIDE_TIMECHECK   140
#define ITU_COVERFLOW_ANYSTOP            0x10
#define ITU_COVERFLOW_ANYBOUNCE          0x20
#define ITU_COVERFLOW_ANYBOUNCE1         0x40
#define ITU_COVERFLOW_ANYBOUNCE2         0x80
#define ITU_BOUNCE_1                     0x1000
#define ITU_BOUNCE_2                     0x2000

#define COVERFLOW_FACTOR                 10
#define COVERFLOW_PROCESS_STAGE1         0.2f
#define COVERFLOW_PROCESS_STAGE2         0.4f
#define COVERFLOW_OVERLAP_MAX_PERCENTAGE 80
#define COVERFLOW_SMOOTH_LASTFRAME_COUNT 5

// local debug related
#define COVERFLOW_DEBUG_FOCUSINDEX       0
#define COVERFLOW_DEBUG_SETXY            0
#define COVERFLOW_LOCAL_DEBUG            0

static const char coverFlowName[] = "ITUCoverFlow";

int save_div(int a, int b)
{
    int sum = ((a + b) / 2);

    if ((a + b) % 2 != 0)
    {
        sum += 1;
    }

    return sum;
}

bool ituCoverFlowClone(ITUWidget* widget, ITUWidget** cloned)
{
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        void* newObj = malloc(sizeof(ITUCoverFlow));
        const void* srcObj = (const void*)widget;
        ITUWidget* newWidget = (ITUWidget*)newObj;
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newObj, srcObj, sizeof(ITUCoverFlow));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    return ituWidgetCloneImpl(widget, cloned);
}

int CoverFlowGetVisibleChildCount (ITUCoverFlow * coverflow)
{
    int             i     = 0;
    const ITCTree * tree  = (ITCTree *)coverflow;
    ITCTree *       child;

    if (tree != NULL)
    {
        for (child = tree->child; child; child = child->sibling)
        {
            const ITUWidget * widget = (ITUWidget *)child;
            if (widget->visible)
            {
                i++;
            }
        }
    }

    return i;
}

ITUWidget * CoverFlowGetVisibleChild (ITUCoverFlow * coverflow, int index)
{
    int      i = 0;
    ITCTree* child;
    ITCTree* tree = (ITCTree*)coverflow;
    assert(tree);

    for (child = tree->child; child; child = child->sibling)
    {
        ITUWidget * widget = (ITUWidget *)child;

        if (widget->visible)
        {
            if (i++ == index)
            {
                return widget;
            }
        }
    }
    return NULL;
}

int CoverFlowSaveDiv (int fa, int fb)
{
    if (fb == 0)
    {
        return 1;
    }
    else
    {
        return (fa / fb);
    }
}

float CoverFlowSaveDivF (float fa, float fb)
{
    if (fb == 0.0f)
    {
        return 1.0f;
    }
    else
    {
        return (fa / fb);
    }
}

float CoverFlowAniStepCal (ITUCoverFlow * coverFlow)
{
    if (coverFlow != NULL)
    {
        int   way  = (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) ? (-1) : (1);
        float step = 0.0f;

        if (coverFlow->totalframe == 0)
        {
            coverFlow->totalframe = 10;
        }

        if (way > 0)
        {
            step = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
        }
        else
        {
            step = CoverFlowSaveDiv((float)(coverFlow->totalframe - coverFlow->frame), (float)coverFlow->totalframe);
        }

        return step;
    }

    return 0.0f;
}

int CoverFlowCheckBoundaryTouch (ITUWidget * widget)
{
    ITUCoverFlow * coverFlow = (ITUCoverFlow *)widget;
    ITUWidget *    childbase = CoverFlowGetVisibleChild(coverFlow, 0);
    int            count     = CoverFlowGetVisibleChildCount(coverFlow);
    int            base_size, orgW, orgH;
    int            max_neighbor_item;
    int            max_width_item;
    int            result = 0;

    if (childbase == NULL)
    {
        return 0;
    }
    else
    {
        orgW = childbase->rect.width;
        orgH = childbase->rect.height;
    }

    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
    {
        base_size = ((coverFlow->orgItemSize > 0) ? (coverFlow->orgItemSize) : (orgH)) - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
    }
    else
    {
        base_size = ((coverFlow->orgItemSize > 0) ? (coverFlow->orgItemSize) : (orgW)) - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
    }

    max_neighbor_item = (CoverFlowSaveDiv(widget->rect.width, base_size) - 1) / 2;
    max_width_item    = CoverFlowSaveDiv(widget->rect.width, base_size);

    if (max_neighbor_item == 0)
    {
        max_neighbor_item++;
    }

    if (coverFlow->focusIndex >= max_neighbor_item)
    {
        if ((count >= (max_neighbor_item * 2 + 1)) && ((count - coverFlow->focusIndex - 1) < max_neighbor_item))
        {
            result = ITU_BOUNCE_2;
        }
        else
        {
            if ((count >= (max_neighbor_item * 2 + 1)) && ((count - coverFlow->focusIndex - 1) < max_width_item))
            {
                result = ITU_BOUNCE_2;
            }
        }
    }
    else
    {
        result = ITU_BOUNCE_1;
    }

    if (result != 0)
    {
        coverFlow->bounceFlag = result;
    }

    return result;
}

void CoverFlowFlushQueue (ITUWidget * widget, ITUCoverFlow * coverFlow, int count, int widget_size, int base_size)
{
    if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
    {
        int i = 0;
        bool no_queue = true;
        bool foundEmpty = false, foundWork = false;
        coverFlow->frame = 0;
        ituCoverFlowOnCoverChanged(coverFlow, widget);

        //shift left and force clean the last
        do
        {
            foundEmpty = foundWork = false;
            for (i = 0; i < COVERFLOW_MAX_PROCARR_SIZE; i++)
            {
                if (coverFlow->procArr[i] == 0)
                {
                    foundEmpty = true;
                    i++;
                    break;
                }
            }
            if (foundEmpty)
            {
                for (; i < COVERFLOW_MAX_PROCARR_SIZE; i++)
                {
                    if (coverFlow->procArr[i] != 0)
                    {
                        foundWork = true;
                        break;
                    }
                }
            }
            if (foundEmpty && foundWork)
            {
                for (i = 1; i < COVERFLOW_MAX_PROCARR_SIZE; i++)
                {
                    if (coverFlow->procArr[i - 1] == 0)
                    {
                        coverFlow->procArr[i - 1] = coverFlow->procArr[i];
                        coverFlow->procArr[i] = 0;
                    }
                }
            }
        } while (foundEmpty && foundWork);

        if (coverFlow->procArr[0] != 0)
        {
            no_queue = false;
        }

        if (no_queue)
        {
            coverFlow->inc = 0;
            widget->flags &= ~ITU_UNDRAGGING;
            ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
        }
        else
        {
            bool boundary_touch = false;

            if (coverFlow->boundaryAlign)
            {
                int max_neighbor_item = (CoverFlowSaveDiv(widget_size, base_size) - 1) / 2;

                coverFlow->slideCount = 0;

                if (max_neighbor_item == 0)
                {
                    max_neighbor_item++;
                }

                if (coverFlow->focusIndex >= max_neighbor_item)
                {
                    if ((count >= (max_neighbor_item * 2 + 1)) && ((count - coverFlow->focusIndex - 1) < max_neighbor_item))
                    {
                        boundary_touch = true;
                    }
                    else
                    {
                        ITUWidget* cf = CoverFlowGetVisibleChild(coverFlow, count - 1);
                        if (cf != NULL)
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                if ((cf->rect.y + cf->rect.height) <= widget_size)
                                {
                                    boundary_touch = true;
                                }
                            }
                            else
                            {
                                if ((cf->rect.x + cf->rect.width) <= widget_size)
                                {
                                    boundary_touch = true;
                                }
                            }
                        }
                    }
                }
                else
                    boundary_touch = true;
            }

            if (!boundary_touch)
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if (coverFlow->procArr[0] < 0)
                    {
                        ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEDOWN, 0, widget->rect.x, widget->rect.y);
                    }
                    else if (coverFlow->procArr[0] > 0)
                    {
                        ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEUP, 0, widget->rect.x, widget->rect.y);
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    if (coverFlow->procArr[0] < 0)
                    {
                        ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDERIGHT, 0, widget->rect.x, widget->rect.y);
                    }
                    else if (coverFlow->procArr[0] > 0)
                    {
                        ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDELEFT, 0, widget->rect.x, widget->rect.y);
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                for (i = 1; i < COVERFLOW_MAX_PROCARR_SIZE; i++)
                {
                    coverFlow->procArr[i - 1] = coverFlow->procArr[i];
                }
            }
        }

        ituScene->dragged = NULL;
    }
}

void CoverFlowCleanQueue (ITUCoverFlow * coverflow)
{
    int i;

    for (i = COVERFLOW_MAX_PROCARR_SIZE - 1; i >= 0; i--)
    {
        coverflow->procArr[i] = 0;
    }
}

void ituCoverFlowSetXY (ITUCoverFlow * coverFlow, int index, int xy, int line)
{
    const ITUWidget * coverflowwidget = (ITUWidget *)coverFlow;
    ITUWidget *       child           = CoverFlowGetVisibleChild(coverFlow, index);
    if (child != NULL)
    {
        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            ituWidgetSetY(child, xy);

            if (COVERFLOW_DEBUG_SETXY)
            {
                (void)printf("[Debug][Coverflow %s][Child %d][SetY %d][%d]\n", coverflowwidget->name, index, xy, line);
            }
        }
        else
        {
            ituWidgetSetX(child, xy);

            if (COVERFLOW_DEBUG_SETXY)
            {
                (void)printf("[Debug][Coverflow %s][Child %d][SetX %d][%d]\n", coverflowwidget->name, index, xy, line);
            }
        }
    }
}

void CoverFlowLayout (ITUWidget * widget)
{
    ITUCoverFlow * coverFlow = (ITUCoverFlow *)widget;

    if (coverFlow != NULL)
    {
        const int   count = CoverFlowGetVisibleChildCount(coverFlow);
        int         base_size;
        ITUWidget * childbase = CoverFlowGetVisibleChild(coverFlow, 0);
        assert(childbase != NULL);

        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            base_size = childbase->rect.height - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
        }
        else
        {
            base_size = childbase->rect.width - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
        }

        if (count > 0)
        {
            if (coverFlow->focusIndex > (count - 1))
            {
                coverFlow->focusIndex = count - 1;
            }
            else if (coverFlow->focusIndex < 0)
            {
                coverFlow->focusIndex = 0;
            }
            else
            {
                /* do nothing */
            }

            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int i, index, count2;

                    count2 = count / 2 + 1;
                    index  = coverFlow->focusIndex;

                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fy = widget->rect.height / 2 - child->rect.height / 2;
                            fy += i * child->rect.height;
                            ituCoverFlowSetXY(coverFlow, index, fy, __LINE__);
                        }

                        if (index >= count - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }

                    count2 = count - count2;
                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fy = widget->rect.height / 2 - child->rect.height / 2;
                            fy -= count2 * child->rect.height;
                            fy += i * child->rect.height;
                            ituCoverFlowSetXY(coverFlow, index, fy, __LINE__);
                        }

                        if (index >= count - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }
                }
                else //[LAYOUT][Vertical][non-cycle]
                {
                    int i = 0;
                    for (; i < count; ++i)
                    {
                        int         fy    = widget->rect.height / 2 - base_size / 2;

                        if (coverFlow->boundaryAlign)
                        {
                            int max_neighbor_item = (CoverFlowSaveDiv(widget->rect.height, base_size) - 1) / 2;
                            int max_height_item   = CoverFlowSaveDiv(widget->rect.height, base_size);
                            fy                    = 0;

                            if (max_neighbor_item == 0)
                            {
                                max_neighbor_item++;
                            }

                            if (coverFlow->focusIndex > 0) //>= max_neighbor_item)
                            {
                                // if ((count >= (max_neighbor_item * 2 + 1)) && ((count - coverFlow->focusIndex - 1) <
                                // max_neighbor_item))
                                if ((count >= (max_neighbor_item * 2 + 1)) &&
                                    ((count - coverFlow->focusIndex - 1) < max_height_item))
                                {
                                    fy = widget->rect.height - (count * base_size);
                                }
                                else
                                {
                                    fy -= base_size * coverFlow->focusIndex;
                                }
                            }
                            else
                            {
                                fy = 0;
                            }
                        }
                        else
                        {
                            fy -= base_size * coverFlow->focusIndex;
                        }

                        if (coverFlow->overlapsize > 0)
                        {
                            fy += i * base_size;
                            ituCoverFlowSetXY(coverFlow, i, fy - coverFlow->overlapsize, __LINE__);
                        }
                        else
                        {
                            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                            if (child != NULL)
                            {
                                fy += i * child->rect.height;
                                ituCoverFlowSetXY(coverFlow, i, fy, __LINE__);
                            }
                        }
                    }
                }
            }
            else
            {
                //[LAYOUT][Horizontal][cycle]
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int i, index, count2;

                    count2 = count / 2 + 1;
                    index  = coverFlow->focusIndex;

                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fx = widget->rect.width / 2 - child->rect.width / 2;
                            fx += i * child->rect.width;
                            ituCoverFlowSetXY(coverFlow, index, fx, __LINE__);
                        }

                        if (index >= count - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }

                    count2 = count - count2;
                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fx = widget->rect.width / 2 - child->rect.width / 2;
                            fx -= count2 * child->rect.width;
                            fx += i * child->rect.width;
                            ituCoverFlowSetXY(coverFlow, index, fx, __LINE__);
                        }

                        if (index >= count - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }
                }
                else //[LAYOUT][Horizontal][non-cycle]
                {
                    int i = 0;
                    for (; i < count; ++i)
                    {
                        int         fx    = widget->rect.width / 2 - base_size / 2;

                        if (coverFlow->boundaryAlign)
                        {
                            int max_neighbor_item = (CoverFlowSaveDiv(widget->rect.width, base_size) - 1) / 2;
                            int max_width_item    = CoverFlowSaveDiv(widget->rect.width, base_size);

                            fx                    = 0;

                            if (max_neighbor_item == 0)
                            {
                                max_neighbor_item++;
                            }

                            if (coverFlow->focusIndex > 0)
                            {
                                if ((count >= (max_neighbor_item * 2 + 1)) &&
                                    ((count - coverFlow->focusIndex - 1) < max_width_item))
                                {
                                    fx = widget->rect.width - (count * base_size);
                                }
                                else
                                {
                                    fx -= base_size * coverFlow->focusIndex;
                                }
                            }
                            else
                            {
                                fx = 0;
                            }
                        }
                        else
                        {
                            fx -= base_size * coverFlow->focusIndex;
                        }

                        if (coverFlow->overlapsize > 0)
                        {
                            fx += i * base_size;
                            ituCoverFlowSetXY(coverFlow, i, fx - coverFlow->overlapsize, __LINE__);
                        }
                        else
                        {
                            ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                            if (child != NULL)
                            {
                                fx += i * child->rect.width;
                                ituCoverFlowSetXY(coverFlow, i, fx, __LINE__);
                            }
                        }
                    }
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ENABLE_ALL) == 0)
            {
                int i = 0;
                for (; i < count; ++i)
                {
                    ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                    if (child != NULL)
                    {
                        if (i == coverFlow->focusIndex)
                        {
                            ituWidgetEnable(child);
                        }
                        else
                        {
                            ituWidgetDisable(child);
                        }
                    }
                }
            }
            widget->flags &= ~ITU_DRAGGING;
            coverFlow->touchCount = 0;

            // fix for stop anywhere not display after load
            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                ITUWidget * child_1 = CoverFlowGetVisibleChild(coverFlow, 0);
                ITUWidget * child_2 = CoverFlowGetVisibleChild(coverFlow, count - 1);
                if ((child_1 != NULL) && (child_2 != NULL))
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        if ((child_1->rect.y > 0) || ((child_2->rect.y + child_2->rect.height) < widget->rect.height))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.y > 0)
                            {
                                move_step = 0 - child_1->rect.y;
                            }
                            else
                            {
                                move_step = widget->rect.height - (child_2->rect.y + child_2->rect.height);
                            }

                            for (i = 0; i < count; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.y;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                    else
                    {
                        if ((child_1->rect.x > 0) || ((child_2->rect.x + child_2->rect.width) < widget->rect.width))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.x > 0)
                            {
                                move_step = 0 - child_1->rect.x;
                            }
                            else
                            {
                                move_step = widget->rect.width - (child_2->rect.x + child_2->rect.width);
                            }

                            for (i = 0; i < count; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.x;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                }
            }

            if (COVERFLOW_LOCAL_DEBUG)
            {
                int x;

                for (x = 0; x < count; x++)
                {
                    const ITUWidget * cc = CoverFlowGetVisibleChild(coverFlow, x);

                    if (x != 1)
                    {
                        continue;
                    }

                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        (void)printf("[CoverFlowLayout] [%d] [%d]\n", x, cc->rect.y);
                    }
                    else
                    {
                        (void)printf("[CoverFlowLayout] [%d] [%d]\n", x, cc->rect.x);
                    }
                }
            }
        }
    }
}

void ituCoverFlowFixFC (ITUCoverFlow * coverFlow)
{
    int count = CoverFlowGetVisibleChildCount(coverFlow);

    if ((count > 0) && (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)))
    {
        // (void)printf("FC checking\n");
        if (coverFlow->focusIndex >= count)
        {
            coverFlow->focusIndex = count - 1;
        }
        else if (coverFlow->focusIndex < 0)
        {
            coverFlow->focusIndex = 0;
        }
        else
        {
            /* do nothing */
        }
    }
}

bool ituCoverFlowUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool           result    = false;
    ITUCoverFlow * coverFlow = (ITUCoverFlow *)widget;
    int            widget_size, base_size, childCount;
    ITUWidget* childFirst;
    ITUWidget* childLast;

    assert(NULL != coverFlow);
    childFirst = CoverFlowGetVisibleChild(coverFlow, 0);
    assert(NULL != childFirst);
    childCount = CoverFlowGetVisibleChildCount(coverFlow);
    assert(childCount > 0);
    childLast = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
    assert(NULL != childLast);

    if (coverFlow)
    {
        ITUWidget * childbase = CoverFlowGetVisibleChild(coverFlow, 0);
        ITUWidget * cc        = NULL;
        int         i, min_w, min_h, orgW, orgH;

        if (!childbase)
        {
            return result;
        }

        min_w = orgW = childbase->rect.width;
        min_h = orgH = childbase->rect.height;

        for (i = 1; i < childCount; i++)
        {
            cc = CoverFlowGetVisibleChild(coverFlow, i);
            if (cc != NULL)
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if (cc->rect.height < min_h)
                    {
                        min_h = cc->rect.height;
                        childbase = cc;
                    }
                }
                else
                {
                    if (cc->rect.width < min_w)
                    {
                        min_w = cc->rect.width;
                        childbase = cc;
                    }
                }
            }
        }

        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            base_size = ((coverFlow->orgItemSize > 0) ? (coverFlow->orgItemSize) : (orgH)) - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
            widget_size = widget->rect.height;
        }
        else
        {
            base_size   = ((coverFlow->orgItemSize > 0) ? (coverFlow->orgItemSize) : (orgW)) - (((coverFlow->overlapsize > 0)) ? (coverFlow->overlapsize) : (0));
            widget_size = widget->rect.width;
        }

        if (coverFlow->bounceRatio <= 0)
        {
            coverFlow->bounceRatio = base_size;
        }
    }

    if ((widget->flags & ITU_TOUCHABLE) && ituWidgetIsEnabled(widget) &&
        (ev == ITU_EVENT_MOUSEDOWN || ev == ITU_EVENT_MOUSEUP))
    {
        int x                          = arg2 - widget->rect.x;
        int y                          = arg3 - widget->rect.y;

        coverFlow->boundary_touch_memo = 0;

#if 0
        if (ev == ITU_EVENT_MOUSEUP)
        {
            coverFlow->touchPos = 0;
        }
#endif

        // workaround for some user insist on using coverflow + stopanywhere
        // use arg1 < 0 to make some bypass trick
        if (ituWidgetIsInside(widget, x, y) && (arg1 >= 0))
        {
            result |= ituFlowWindowUpdate(widget, ev, arg1, arg2, arg3);
        }
    }
    else
    {
        result |= ituFlowWindowUpdate(widget, ev, arg1, arg2, arg3);
    }

	if ((widget->flags & ITU_TOUCHABLE) || (arg1 < 0)) //(arg1 < 0 for API next/prev)
    {
        bool fast_slide = false;
		
		if (arg1 < 0)
		{
			arg1 = 0;
		}

        // try slide check for current target
        if ((ituScene->dragged != widget) && (ituScene->dragged != NULL) && (arg1 != 0))
        {
            if (ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDERIGHT)
            {
                if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL))
                {
                    (void)printf("[Coverflow] bypass slide[H].\n");
                    result = ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);
                    return result;
                }
            }
            else if (ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDEDOWN)
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    (void)printf("[Coverflow] bypass slide[V].\n");
                    result = ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);
                    return result;
                }
            }
            else
            {
                /* do nothing */
            }
        }

        if (ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDERIGHT || ev == ITU_EVENT_TOUCHSLIDEUP ||
            ev == ITU_EVENT_TOUCHSLIDEDOWN)
        {
            // to fix when slide at Non-Cycle mode boundary area
            if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                int         pos1, pos2;
                bool        bForceMouseUp = false;

                if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
                {
                    if (ev == ITU_EVENT_TOUCHSLIDELEFT)
                    {
                        int next = ((coverFlow->inc < 0) ? (1) : (0));
#if 0
                        (void)printf("[LEFT] [inc %d] [Fi %d] [frame %d]\n", coverFlow->inc, coverFlow->focusIndex,
                            coverFlow->frame);
#endif
                        if ((coverFlow->focusIndex + next) >= (childCount - 1))
                        {
                            return true;
                        }

                        if (coverFlow->bounceRatio && coverFlow->boundaryAlign)
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                if ((childLast->rect.y + base_size) <=
                                    (widget_size - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                {
                                    CoverFlowCleanQueue(coverFlow);
                                    return true;
                                }
                            }
                            else
                            {
                                if ((childLast->rect.x + base_size) <=
                                    (widget_size - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                {
                                    CoverFlowCleanQueue(coverFlow);
                                    return true;
                                }
                            }
                        }
                    }
                    else if (ev == ITU_EVENT_TOUCHSLIDERIGHT)
                    {
                        int next = ((coverFlow->inc > 0) ? (-1) : (0));
#if 0
                        (void)printf("[RIGHT] [inc %d] [Fi %d] [frame %d]\n", coverFlow->inc, coverFlow->focusIndex,
                            coverFlow->frame);
#endif
                        if ((coverFlow->focusIndex + next) <= 0)
                        {
                            return true;
                        }
                    }
                    else if (ev == ITU_EVENT_TOUCHSLIDEUP)
                    {
                        int next = ((coverFlow->inc < 0) ? (1) : (0));
#if 0
                        (void)printf("[UP] [inc %d] [Fi %d] [frame %d]\n", coverFlow->inc, coverFlow->focusIndex,
                            coverFlow->frame);
#endif
                        if ((coverFlow->focusIndex + next) >= (childCount - 1))
                        {
                            return true;
                        }
                    }
                    else if (ev == ITU_EVENT_TOUCHSLIDEDOWN)
                    {
                        int next = ((coverFlow->inc > 0) ? (-1) : (0));
#if 0
                        (void)printf("[DOWN] [inc %d] [Fi %d] [frame %d]\n", coverFlow->inc, coverFlow->focusIndex,
                            coverFlow->frame);
#endif
                        if ((coverFlow->focusIndex + next) <= 0)
                        {
                            return true;
                        }
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                if ((coverFlow->boundaryAlign) || (childCount <= 2))
                {
                    pos1 = 0;
                    pos2 = widget_size;
                }
                else
                {
                    pos1 = save_div(widget_size, -base_size); //(widget_size - base_size) / 2;
                    pos2 = save_div(widget_size, base_size);  //(widget_size + base_size) / 2;
                                                              ////for exact 2 div
                                                              // pos1++;
                    // pos2++;
                }

                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if (childFirst->rect.y > pos1)
                    {
                        bForceMouseUp = true;
                    }
                    else if ((childLast->rect.y + childLast->rect.height) < pos2)
                    {
                        bForceMouseUp = true;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    if (childFirst->rect.x > pos1)
                    {
                        bForceMouseUp = true;
                    }
                    else if ((childLast->rect.x + childLast->rect.width) < pos2)
                    {
                        bForceMouseUp = true;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                if (bForceMouseUp)
                {
                    ituUnPressWidget(widget);
                    ituWidgetUpdate(coverFlow, ITU_EVENT_MOUSEUP, arg1, arg2, arg3);
                    return true;
                }
            }

            // this should be check for goose again
            if ((itpGetTickCount() - coverFlow->clock) < COVERFLOW_FAST_SLIDE_TIMECHECK)
            {
                fast_slide = true;
            }

            coverFlow->slide_diff = arg1;

            if (ituWidgetIsEnabled(widget) && !result)
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if (!widget->rect.width || !widget->rect.height || ituWidgetIsInside(widget, x, y))
                {
                    result |= ituExecActions(widget, coverFlow->actions, ev, arg1);
                }
            }

            ///////try to fix no slide and no mouseup at the same time
            if ((coverFlow->slideMaxCount == 0) && (coverFlow->prevnext_trigger == 0))
            {
                // coverFlow->coverFlowFlags |= ITU_COVERFLOW_SLIDING;
                ituWidgetUpdate(coverFlow, ITU_EVENT_MOUSEUP, arg1, arg2, arg3);
            }
        }

        if (((ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDERIGHT) &&
             ((coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) == 0) && (coverFlow->slideMaxCount > 0)) ||
            coverFlow->prevnext_trigger)
        {
            coverFlow->touchCount = 0;

            if (ituWidgetIsEnabled(widget))
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if (ituWidgetIsInside(widget, x, y))
                {
                    if (childCount > 0)
                    {
                        bool boundary_touch       = false;
                        bool boundary_touch_left  = false;
                        bool boundary_touch_right = false;
                        ////try to fix the mouse up shadow(last frame) diff when sliding start(frame 0)
                        int  offset, absoffset, interval;
                        offset   = x - coverFlow->touchPos;
                        interval = CoverFlowSaveDiv(offset, base_size);
                        offset -= (interval * base_size);
                        absoffset = offset > 0 ? offset : -offset;

                        if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
                        {
                            if (absoffset > base_size / 2)
                            {
                                coverFlow->frame = (int)((float)coverFlow->totalframe *
                                                         CoverFlowSaveDivF((float)absoffset, (float)base_size));
                            }
                            else if (absoffset)
                            {
                                coverFlow->frame = coverFlow->totalframe -
                                                   (int)((float)coverFlow->totalframe *
                                                         CoverFlowSaveDivF((float)absoffset, (float)base_size));
                            }
                            else
                            {
                                coverFlow->frame = 0;
                            }
                        }

#if 0
                        if (widget->flags & ITU_DRAGGABLE)
                        {
                            coverFlow->frame = coverFlow->totalframe - ((abs(x - coverFlow->touchPos) *
                        }
                        coverFlow->totalframe)/base_size) + 1;
#endif

                        fast_slide = false;

                        if ((!(widget->flags & ITU_DRAGGABLE)) || fast_slide)
                        {
                            // if (!(widget->flags & ITU_DRAGGABLE))
                            if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
                            {
                                coverFlow->frame = 0;
                            }
                            (void)printf("[CoverFlow][Fast Slide!!]\n\n");
                        }
                        else
                        {
                            (void)printf("[CoverFlow][Normal Slide!!]\n\n");
                        }

                        ituUnPressWidget(widget);

                        // check boundary touch for H non-cycle
                        if (coverFlow->boundaryAlign)
                        {
                            int max_neighbor_item = (CoverFlowSaveDiv(widget_size, base_size) - 1) / 2;

                            coverFlow->slideCount = 0;

                            if (max_neighbor_item == 0)
                            {
                                max_neighbor_item++;
                            }

                            if (coverFlow->focusIndex >= max_neighbor_item)
                            {
                                if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                    ((childCount - coverFlow->focusIndex - 1) < max_neighbor_item))
                                {
                                    boundary_touch       = true;
                                    boundary_touch_right = true;
                                    coverFlow->coverFlowFlags |= ITU_BOUNCE_2;
                                }
                                else
                                {
                                    if ((childLast->rect.x + childLast->rect.width) <= widget_size)
                                    {
                                        boundary_touch       = true;
                                        boundary_touch_right = true;
                                        coverFlow->coverFlowFlags |= ITU_BOUNCE_2;
                                    }
                                }
                            }
                            else
                            {
                                boundary_touch      = true;
                                boundary_touch_left = true;
                                coverFlow->coverFlowFlags |= ITU_BOUNCE_1;
                            }
                        }

                        if (ev == ITU_EVENT_TOUCHSLIDELEFT)
                        {
                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)))
                            {
                                if (coverFlow->inc > 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    coverFlow->frame = coverFlow->totalframe - coverFlow->frame;
                                    coverFlow->inc *= -1;
                                    if (coverFlow->focusIndex > 0)
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    return true;
                                }
                                else if (coverFlow->inc < 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    if (coverFlow->focusIndex < (childCount - 2))
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    else
                                    {
                                        coverFlow->frame = coverFlow->totalframe;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    ituCoverFlowFixFC(coverFlow);
                                    ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                   coverFlow->focusIndex);
                                    return true;
                                }
                                else
                                {
                                    if (coverFlow->focusIndex <= (childCount - 2))
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                }
                            }

                            // reverse
                            if ((coverFlow->slideMaxCount > 0) || (coverFlow->prevnext_trigger))
                            {
                                coverFlow->prevnext_trigger = 0;
                                coverFlow->coverFlowFlags |= ITU_COVERFLOW_SLIDING;
                                coverFlow->touchCount = 0;
                            }

                            if (widget->flags & ITU_DRAGGING)
                            {
                                widget->flags &= ~ITU_DRAGGING;
                                ituScene->dragged = NULL;
                                coverFlow->inc    = 0;
                            }

                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) ||
                                (coverFlow->focusIndex < (childCount - 1)) || boundary_touch)
                            {
                                if (widget->flags & ITU_DRAGGING)
                                {
                                    widget->flags &= ~ITU_DRAGGING;
                                    ituScene->dragged = NULL;
                                    coverFlow->inc    = 0;
                                }

                                if (coverFlow->inc == 0)
                                {
                                    coverFlow->inc = 0 - base_size;
                                }

                                if (boundary_touch)
                                {
                                    if (((childLast->rect.x + childLast->rect.width) <= widget_size) ||
                                        (childLast->rect.width > widget_size))
                                    {
                                        coverFlow->inc   = -1;
                                        coverFlow->frame = coverFlow->totalframe - 1;

                                        if (boundary_touch_right && !(widget->flags & ITU_DRAGGING) &&
                                            coverFlow->bounceRatio > 0)
                                        {
                                            coverFlow->inc = 0 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex + 1, __LINE__);
                                            }

                                            coverFlow->focusIndex++;

                                            widget->flags |= ITU_BOUNCING;
                                            coverFlow->frame = 0;
                                        }
                                    }
                                }
                            }
                            else if (coverFlow->focusIndex >= childCount - 1)
                            { // maybe useless now
                                // try to fix the ScaleCoverFlow side effect for non-cycle mode
                                if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
                                    (coverFlow->boundaryAlign == 0))
                                {
                                    coverFlow->inc = 0;
                                }
                                else if ((childCount) > 0 && !(widget->flags & ITU_DRAGGING) && coverFlow->bounceRatio > 0)
                                {
                                    if (coverFlow->inc == 0)
                                    {
                                        coverFlow->inc = 0 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    }

                                    widget->flags |= ITU_BOUNCING;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                        else // if (ev == ITU_EVENT_TOUCHSLIDERIGHT)
                        {
                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)))
                            {
                                if (coverFlow->inc < 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    coverFlow->frame = coverFlow->totalframe - coverFlow->frame;
                                    coverFlow->inc *= -1;
                                    if (coverFlow->focusIndex < (childCount - 1))
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    return true;
                                }
                                else if (coverFlow->inc > 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    if (coverFlow->focusIndex > 1)
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    else
                                    {
                                        coverFlow->frame = coverFlow->totalframe;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    ituCoverFlowFixFC(coverFlow);
                                    ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                   coverFlow->focusIndex);
                                    return true;
                                }
                                else
                                {
                                    if (coverFlow->focusIndex >= 1)
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                }
                            }

                            // reverse
                            if ((coverFlow->slideMaxCount > 0) || (coverFlow->prevnext_trigger))
                            {
                                coverFlow->prevnext_trigger = 0;
                                coverFlow->coverFlowFlags |= ITU_COVERFLOW_SLIDING;
                                coverFlow->touchCount = 0;
                            }

                            if (widget->flags & ITU_DRAGGING)
                            {
                                widget->flags &= ~ITU_DRAGGING;
                                ituScene->dragged = NULL;
                                coverFlow->inc    = 0;
                            }

                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) || (coverFlow->focusIndex > 0) ||
                                boundary_touch)
                            {
                                if (widget->flags & ITU_DRAGGING)
                                {
                                    widget->flags &= ~ITU_DRAGGING;
                                    ituScene->dragged = NULL;
                                    coverFlow->inc    = 0;
                                }

        #if 0
                                if (boundary_touch)
                                {
                                    coverFlow->focusIndex -= ((coverFlow->focusIndex > 1) ? (2) : (0));
                                }
        #endif
                                if (coverFlow->inc == 0)
                                {
                                    coverFlow->inc = base_size;
                                }

                                if (boundary_touch)
                                {
                                    if ((childLast->rect.x + childLast->rect.width) <= widget_size)
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                        }

                                        coverFlow->focusIndex = CoverFlowGetVisibleChildCount(coverFlow) -
                                                                CoverFlowSaveDiv(widget_size, base_size);

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                        }
                                    }

                                    if ((childFirst->rect.x) >= 0)
                                    {
                                        coverFlow->inc   = 1;
                                        coverFlow->frame = coverFlow->totalframe - 1;
                                    }

                                    if (boundary_touch_left && !(widget->flags & ITU_DRAGGING) &&
                                        coverFlow->bounceRatio > 0)
                                    {
                                        coverFlow->inc = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                        widget->flags |= ITU_BOUNCING;
                                        coverFlow->frame = 0;
                                    }
                                }
                            }
                            else if (coverFlow->focusIndex <= 0)
                            { // maybe useless now
                                // try to fix the ScaleCoverFlow side effect for non-cycle mode
                                if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
                                    (coverFlow->boundaryAlign == 0))
                                {
                                    if (widget->flags & ITU_DRAGGING)
                                    {
                                        widget->flags &= ~ITU_DRAGGING;
                                    }
                                    coverFlow->inc = 0;
                                }
                                else if (childCount > 0 && !(widget->flags & ITU_DRAGGING) && coverFlow->bounceRatio > 0)
                                {
                                    if (coverFlow->inc == 0)
                                    {
                                        coverFlow->inc = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    }

                                    widget->flags |= ITU_BOUNCING;
                                    // coverFlow->frame = 1;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                        result = true;
                    }
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                coverFlow->frame = 0;
            }
        }
        else if (((ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDEDOWN) &&
                  (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL) && (coverFlow->slideMaxCount > 0)) ||
                 coverFlow->prevnext_trigger)
        {
            coverFlow->touchCount = 0;

            if (ituWidgetIsEnabled(widget)) // && (widget->flags & ITU_DRAGGABLE))
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if (ituWidgetIsInside(widget, x, y))
                {
                    if (childCount > 0)
                    {
                        bool boundary_touch        = false;
                        bool boundary_touch_top    = false;
                        bool boundary_touch_bottom = false;
                        ////try to fix the mouse up shadow(last frame) diff when sliding start(frame 0)
                        int  offset, absoffset, interval;
                        offset   = y - coverFlow->touchPos;
                        interval = CoverFlowSaveDiv(offset, base_size);
                        offset -= (interval * base_size);
                        absoffset = offset > 0 ? offset : -offset;

                        if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
                        {
                            if (absoffset > base_size / 2)
                            {
                                coverFlow->frame = (int)((float)coverFlow->totalframe *
                                                         CoverFlowSaveDivF((float)absoffset, (float)base_size));
                            }
                            else if (absoffset)
                            {
                                coverFlow->frame = coverFlow->totalframe -
                                                   (int)((float)coverFlow->totalframe *
                                                         CoverFlowSaveDivF((float)absoffset, (float)base_size));
                            }
                            else
                            {
                                coverFlow->frame = 0;
                            }
                        }

                        if ((!(widget->flags & ITU_DRAGGABLE)) || fast_slide)
                        {
                            // if (!(widget->flags & ITU_DRAGGABLE))
                            if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
                            {
                                coverFlow->frame = 0;
                            }
                            ituLog("[CoverFlow][Fast Slide!!]\n\n");
                        }
                        else
                        {
                            ituLog("[CoverFlow][Normal Slide!!]\n\n");
                        }

                        ituUnPressWidget(widget);

                        if (coverFlow->boundaryAlign)
                        {
                            int max_neighbor_item = (CoverFlowSaveDiv(widget->rect.height, base_size) - 1) / 2;

                            coverFlow->slideCount = 0;

                            if (max_neighbor_item == 0)
                            {
                                max_neighbor_item++;
                            }

                            if (coverFlow->focusIndex >= max_neighbor_item)
                            {
                                if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                    ((childCount - coverFlow->focusIndex - 1) < max_neighbor_item))
                                {
                                    boundary_touch        = true;
                                    boundary_touch_bottom = true;
                                    coverFlow->coverFlowFlags |= ITU_BOUNCE_2;
                                }
                                else
                                {
                                    if ((childLast->rect.y + childLast->rect.height) <= widget_size)
                                    {
                                        boundary_touch        = true;
                                        boundary_touch_bottom = true;
                                        coverFlow->coverFlowFlags |= ITU_BOUNCE_2;
                                    }
                                }
                            }
                            else
                            {
                                boundary_touch     = true;
                                boundary_touch_top = true;
                                coverFlow->coverFlowFlags |= ITU_BOUNCE_1;
                            }
                        }

                        if (ev == ITU_EVENT_TOUCHSLIDEUP)
                        {
                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)))
                            {
                                if (coverFlow->inc > 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    coverFlow->frame = coverFlow->totalframe - coverFlow->frame;
                                    coverFlow->inc *= -1;
                                    if (coverFlow->focusIndex > 0)
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    return true;
                                }
                                else if (coverFlow->inc < 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    if (coverFlow->focusIndex < (childCount - 2))
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    else
                                    {
                                        coverFlow->frame = coverFlow->totalframe;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    ituCoverFlowFixFC(coverFlow);
                                    ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                   coverFlow->focusIndex);
                                    return true;
                                }
                                else
                                {
                                    if (coverFlow->focusIndex <= (childCount - 2))
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                }
                            }

                            // reverse
                            if ((coverFlow->slideMaxCount > 0) || (coverFlow->prevnext_trigger))
                            {
                                coverFlow->prevnext_trigger = 0;
                                coverFlow->coverFlowFlags |= ITU_COVERFLOW_SLIDING;
                                coverFlow->touchCount = 0;
                            }

                            if (widget->flags & ITU_DRAGGING)
                            {
                                widget->flags &= ~ITU_DRAGGING;
                                ituScene->dragged = NULL;
                                coverFlow->inc    = 0;
                            }

                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) ||
                                (coverFlow->focusIndex < childCount - 1) || boundary_touch)
                            {
                                if (widget->flags & ITU_DRAGGING)
                                {
                                    widget->flags &= ~ITU_DRAGGING;
                                    ituScene->dragged = NULL;
                                    coverFlow->inc    = 0;
                                }

                                if (coverFlow->inc == 0)
                                {
                                    coverFlow->inc = 0 - base_size;
                                }

                                if (boundary_touch)
                                {
                                    if ((childLast->rect.y + childLast->rect.height) <= widget_size)
                                    {
                                        coverFlow->inc   = -1;
                                        coverFlow->frame = coverFlow->totalframe - 1;

                                        if ((boundary_touch) && (coverFlow->focusIndex > 0))
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                            }

                                            coverFlow->focusIndex = CoverFlowGetVisibleChildCount(coverFlow) -
                                                                    CoverFlowSaveDiv(widget_size, base_size);

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                            }
                                        }

                                        if (boundary_touch_bottom && !(widget->flags & ITU_DRAGGING) &&
                                            coverFlow->bounceRatio > 0)
                                        {
                                            coverFlow->inc = 0 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex + 1, __LINE__);
                                            }

                                            coverFlow->focusIndex++;

                                            widget->flags |= ITU_BOUNCING;
                                            coverFlow->frame = 0;
                                        }
                                    }
                                }
                            }
                            else if (coverFlow->focusIndex >= childCount - 1)
                            { // maybe useless now
                                // try to fix the ScaleCoverFlow side effect for non-cycle mode
                                if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
                                    (coverFlow->boundaryAlign == 0))
                                {
                                    coverFlow->inc = 0;
                                }
                                else if (childCount > 0 && !(widget->flags & ITU_DRAGGING) && coverFlow->bounceRatio > 0)
                                {
                                    if (coverFlow->inc == 0)
                                    {
                                        coverFlow->inc = 0 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    }

                                    widget->flags |= ITU_BOUNCING;
                                    // coverFlow->frame = 1;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                        else // if (ev == ITU_EVENT_TOUCHSLIDEDOWN)
                        {
                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)))
                            {
                                if (coverFlow->inc < 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    coverFlow->frame = coverFlow->totalframe - coverFlow->frame;
                                    coverFlow->inc *= -1;
                                    if (coverFlow->focusIndex < (childCount - 1))
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex++;
                                    }
                                    return true;
                                }
                                else if (coverFlow->inc > 0)
                                {
                                    if (coverFlow->totalframe != coverFlow->org_totalframe)
                                    {
                                        coverFlow->frame /= 2;
                                        coverFlow->totalframe = coverFlow->org_totalframe;
                                        (void)printf("[mark totalframe %d]\n", coverFlow->totalframe);
                                    }
                                    if (coverFlow->focusIndex > 1)
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    else
                                    {
                                        coverFlow->frame = coverFlow->totalframe;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                    ituCoverFlowFixFC(coverFlow);
                                    ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                   coverFlow->focusIndex);
                                    return true;
                                }
                                else
                                {
                                    if (coverFlow->focusIndex >= 1)
                                    {
                                        coverFlow->frame = 0;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex--;
                                    }
                                }
                            }

                            // reverse
                            if ((coverFlow->slideMaxCount > 0) || (coverFlow->prevnext_trigger))
                            {
                                coverFlow->prevnext_trigger = 0;
                                coverFlow->coverFlowFlags |= ITU_COVERFLOW_SLIDING;
                                coverFlow->touchCount = 0;
                            }

                            if (widget->flags & ITU_DRAGGING)
                            {
                                widget->flags &= ~ITU_DRAGGING;
                                ituScene->dragged = NULL;
                                coverFlow->inc    = 0;
                            }

                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) || (coverFlow->focusIndex > 0) ||
                                boundary_touch)
                            {
                                if (widget->flags & ITU_DRAGGING)
                                {
                                    widget->flags &= ~ITU_DRAGGING;
                                    ituScene->dragged = NULL;
                                    coverFlow->inc    = 0;
                                }

                                // if (boundary_touch)
                                //  coverFlow->focusIndex -= ((coverFlow->focusIndex > 1) ? (1) : (0));
                                if (boundary_touch)
                                {
                                    if ((COVERFLOW_DEBUG_FOCUSINDEX) && (coverFlow->focusIndex > 1))
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                               coverFlow->focusIndex - 2, __LINE__);
                                    }

                                    coverFlow->focusIndex -= ((coverFlow->focusIndex > 1) ? (2) : (0));
                                }

                                if (coverFlow->inc == 0)
                                {
                                    coverFlow->inc = base_size;
                                }

                                if (boundary_touch)
                                {
                                    if ((childFirst->rect.y) >= 0)
                                    {
                                        coverFlow->inc   = +1;
                                        coverFlow->frame = coverFlow->totalframe - 1;
                                    }

                                    if (boundary_touch_top && !(widget->flags & ITU_DRAGGING) &&
                                        coverFlow->bounceRatio > 0)
                                    {
                                        coverFlow->inc = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex + 1, __LINE__);
                                        }

                                        coverFlow->focusIndex += 1;
                                        widget->flags |= ITU_BOUNCING;
                                        coverFlow->frame = 0;
                                    }
                                }
                                // coverFlow->frame = 1;
                            }
                            else if (coverFlow->focusIndex <= 0)
                            { // maybe useless now
                                // try to fix the ScaleCoverFlow side effect for non-cycle mode
                                if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
                                    (coverFlow->boundaryAlign == 0))
                                {
                                    coverFlow->inc = 0;
                                }
                                else if (childCount > 0 && !(widget->flags & ITU_DRAGGING) && coverFlow->bounceRatio > 0)
                                {
                                    if (coverFlow->inc == 0)
                                    {
                                        coverFlow->inc = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    }

                                    widget->flags |= ITU_BOUNCING;
        #if 0
                                    coverFlow->frame = 1;
        #endif
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                        result = true;
                    }
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                coverFlow->frame = 0;
            }
        }
        else if (ev == ITU_EVENT_MOUSEMOVE)
        {
            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGING))
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if ((ituWidgetIsInside(widget, x, y)) && (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)))
                {
                    int center_align_pos1;
                    int center_align_pos2;

                    if (coverFlow->boundaryAlign)
                    {
                        center_align_pos1 = 0;
                        center_align_pos2 = widget_size;
                    }
                    else
                    {
                        center_align_pos1 = save_div(widget_size, -base_size); //((widget_size - base_size) / 2);
                        center_align_pos2 = center_align_pos1 + base_size;
                    }

                    // (void)printf("[coverflow][dragging][%d, %d]\n", x, y);

                    if (childCount > 0)
                    {
                        int dist = 0;
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                        {
                            dist = y - coverFlow->touchPos;
                        }
                        else
                        {
                            dist = x - coverFlow->touchPos;
                        }

                        // (void)printf("[MOVE]dist %d touchPos %d\n", dist, coverFlow->touchPos);

                        if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
                            (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)))
                        {
                            if (coverFlow->bounceRatio > 0)
                            { // the case when drag touch the boundary and move back away
                                int  min_item_diffvalue = 0;
                                int  max_item_diffvalue = 0;
                                bool b_touch_min        = false;
                                bool b_touch_max        = false;
                                bool print_check        = false;

                                if (1) //(coverFlow->boundaryAlign)
                                {
                                    int min_item_boundary = coverFlow->boundary_memo1 + dist;
                                    int max_item_boundary = coverFlow->boundary_memo2 + dist;

                                    min_item_diffvalue    = min_item_boundary -
                                                         CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) -
                                                         center_align_pos1;
                                    max_item_diffvalue = (max_item_boundary - center_align_pos2) +
                                                         CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                    if (min_item_boundary >=
                                        (CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) + center_align_pos1))
                                    {
                                        b_touch_min = true;
                                    }
                                    else if (max_item_boundary <=
                                             (center_align_pos2 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                    {
                                        b_touch_max = true;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }

                                // (void)printf("memo %d toupos %d\n", coverFlow->boundary_touch_memo, coverFlow->touchPos);

                                if (coverFlow->boundary_touch_memo == 0)
                                {
                                    if (b_touch_min)
                                    {
                                        coverFlow->boundary_touch_memo = min_item_diffvalue;
                                        if (print_check)
                                        {
                                            (void)printf("active1...%d\n", coverFlow->boundary_touch_memo);
                                        }
                                    }
                                    else if (b_touch_max)
                                    {
                                        coverFlow->boundary_touch_memo = max_item_diffvalue;
                                        if (print_check)
                                        {
                                            (void)printf("active2...%d\n", coverFlow->boundary_touch_memo);
                                        }
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                                else if ((min_item_diffvalue > coverFlow->boundary_touch_memo) &&
                                         (coverFlow->boundary_touch_memo > 0))
                                {
                                    coverFlow->boundary_touch_memo = min_item_diffvalue;
                                    if (print_check)
                                    {
                                        (void)printf("active3...%d\n", coverFlow->boundary_touch_memo);
                                    }
                                }
                                else if ((max_item_diffvalue < coverFlow->boundary_touch_memo) &&
                                         (coverFlow->boundary_touch_memo < 0))
                                {
                                    coverFlow->boundary_touch_memo = max_item_diffvalue;
                                    if (print_check)
                                    {
                                        (void)printf("active4...%d\n", coverFlow->boundary_touch_memo);
                                    }
                                }
                                else
                                {
                                    if ((max_item_diffvalue > coverFlow->boundary_touch_memo) &&
                                        (coverFlow->boundary_touch_memo < 0))
                                    {
                                        if (min_item_diffvalue <= 0)
                                        {
                                            dist += coverFlow->boundary_touch_memo * -1;
                                            if (print_check)
                                            {
                                                (void)printf("dist5....%d\n", dist);
                                            }
                                        }
                                        else
                                        {
                                            coverFlow->touchPos += coverFlow->boundary_touch_memo;
                                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                            {
                                                dist = y - coverFlow->touchPos;
                                            }
                                            else
                                            {
                                                dist = x - coverFlow->touchPos;
                                            }
                                            coverFlow->boundary_touch_memo = 0;
                                            if (print_check)
                                            {
                                                (void)printf("dist6....%d\n", dist);
                                            }
                                        }
                                    }
                                    else if ((min_item_diffvalue < coverFlow->boundary_touch_memo) &&
                                             (coverFlow->boundary_touch_memo > 0))
                                    {
                                        if (min_item_diffvalue >= 0)
                                        {
                                            dist -= coverFlow->boundary_touch_memo;
                                            if (print_check)
                                            {
                                                (void)printf("dist7....%d\n", dist);
                                            }
                                        }
                                        else
                                        {

                                            coverFlow->touchPos += coverFlow->boundary_touch_memo;
                                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                            {
                                                dist = y - coverFlow->touchPos;
                                            }
                                            else
                                            {
                                                dist = x - coverFlow->touchPos;
                                            }
                                            coverFlow->boundary_touch_memo = 0;
                                            if (print_check)
                                            {
                                                (void)printf("dist8....%d\n", dist);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }

                                
                            }
                        }

                        // if (dist < 0)
                        //  dist = -dist;
                        // (void)printf("Fc %d BB %d\n", coverFlow->focusIndex, coverFlow->boundary_touch_memo);

                        if ((abs(dist) >= ITU_DRAG_DISTANCE) || coverFlow->boundary_touch_memo)
                        {
                            if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_DISABLE_MOVE_UNPRESS))
                            {
                                ituUnPressWidget(widget);
                            }
                            ituWidgetUpdate(widget, ITU_EVENT_DRAGGING, 0, 0, 0);
                        }

                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                            !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                        {
                            int dist_spec;

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                dist_spec = y - coverFlow->init_drag_xy;
                            }
                            else
                            {
                                dist_spec = x - coverFlow->init_drag_xy;
                            }

                            if (abs(dist_spec) >= ITU_DRAG_DISTANCE)
                            {
                                if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_DISABLE_MOVE_UNPRESS))
                                {
                                    ituUnPressWidget(widget);
                                }
                                // ituWidgetUpdate(widget, ITU_EVENT_DRAGGING, 0, 0, 0);
                            }
                            // (void)printf("dist_spec is %d, y %d, pos %d\n", dist_spec, y, Arr[0]);
                        }

                        ////////////////////////////
                        // ITU_EVENT_MOUSEMOVE
                        // Vertical Mode
                        ////////////////////////////
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                        {
                            // offset = y - coverFlow->touchPos;
                            int offset = dist;
                            // (void)printf("0: offset=%d\n", offset);
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                            {
                                if (abs(offset) < CoverFlowSaveDiv(base_size, coverFlow->min_change_dist_factor))
                                {
                                    if (coverFlow->dragFlag == 0)
                                    {
                                        coverFlow->dragFlag = 1;
                                    }
                                }
                                else
                                {
                                    coverFlow->dragFlag = -1;
                                }

                                if (offset >= base_size)
                                {
                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                    }

                                    if (coverFlow->focusIndex > 0)
                                    {
                                        coverFlow->focusIndex--;
                                    }
                                    else
                                    {
                                        coverFlow->focusIndex = childCount - 1;
                                    }

                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                    }

                                    offset -= base_size;
                                    coverFlow->touchPos += base_size;
                                    coverFlow->dragFlag = -1;
                                }

                                if (offset <= (base_size * -1))
                                {
                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                    }

                                    if (coverFlow->focusIndex < (childCount - 1))
                                    {
                                        coverFlow->focusIndex++;
                                    }
                                    else
                                    {
                                        coverFlow->focusIndex = 0;
                                    }

                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                    }

                                    offset += base_size;
                                    coverFlow->touchPos -= base_size;
                                    coverFlow->dragFlag = -1;
                                }
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                            {
                                int i, index, fy;
                                index = coverFlow->focusIndex;
                                fy = widget_size / 2 - base_size / 2;
                                for (i = 0; i < childCount; i++)
                                {
                                    ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, index);
                                    if (child != NULL)
                                    {
                                        ituCoverFlowSetXY(coverFlow, index, fy + offset, __LINE__);
										fy += base_size;
                                    }

                                    if (index >= childCount - 1)
                                    {
                                        index = 0;
                                    }
                                    else
                                    {
                                        index++;
                                    }
                                }
                                fy = widget_size / 2 - base_size / 2; //reset for next loop
                                index = coverFlow->focusIndex - 1;
                                if (index < 0)
                                {
                                    index = childCount - 1;
                                }
                                for (i = 0; i < childCount; ++i)
                                {
                                    if ((fy + offset) >= 0)
                                    {
                                        ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, index);
                                        if (child != NULL)
                                        {
											fy -= base_size;
                                            ituCoverFlowSetXY(coverFlow, index, fy + offset, __LINE__);
                                        }

                                        if (index == 0)
                                        {
                                            index = childCount - 1;
                                        }
                                        else
                                        {
                                            index--;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // limit the move under non-cycle/Vertical boundaryAlign mode
                                int  i, fy = 0;
                                bool b_touch = false;

                                if ((coverFlow->boundaryAlign) && (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
                                {
                                    if ((childFirst->rect.y + offset) >
                                        CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                    {
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                        b_touch = true;
                                    }
                                    else if ((childLast->rect.y + base_size + offset) <
                                             (widget->rect.height -
                                              CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                    {
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                        b_touch = true;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                                

                                if (childCount == 1) // for special one item case
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset >= CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                        {
                                            offset = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                        }
                                        else if (offset <= -1 * CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                        {
                                            offset = -1 * CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                }
                                else if ((coverFlow->boundary_memo1 + offset) >=
                                         (CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) + center_align_pos1))
                                {
                                    offset = (CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) + center_align_pos1) -
                                             coverFlow->boundary_memo1;
                                }
                                else if ((coverFlow->boundary_memo2 + offset) <=
                                         (center_align_pos2 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                {
                                    offset =
                                        ((center_align_pos2 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) -
                                         coverFlow->boundary_memo2);
                                }
        #if 0
                                else if (coverFlow->focusIndex <= 0)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset >= base_size / coverFlow->bounceRatio)
                                            offset = base_size / coverFlow->bounceRatio;
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
                                else if (coverFlow->focusIndex >= childCount - 1)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset <= -base_size / coverFlow->bounceRatio)
                                            offset = -base_size / coverFlow->bounceRatio;
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
        #endif
                                else
                                {
                                    /* do nothing */
                                }

                                if (coverFlow->bounceRatio == 0)
                                {
                                    offset = 0;
                                }

                                for (i = 0; i < childCount; ++i)
                                { //[MOVE][Vertical][non-cycle][layout]
                                    ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                                    // int fy = widget->rect.height / 2 - child->rect.height / 2;

                                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                        !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                    {
                                        if (i == 0)
                                        {
                                            int         local_touchpos = 0;
                                            fy                         = childFirst->rect.y + offset;

                                            if (b_touch)
                                            {
                                                if ((childFirst->rect.y + offset) >
                                                    CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                                {
                                                    fy = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                                    local_touchpos = 0;
                                                }
                                                else if ((childLast->rect.y + base_size + offset) <
                                                         (widget->rect.height -
                                                          CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                                {
                                                    fy = (widget->rect.height -
                                                          CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) -
                                                        childCount * base_size;
                                                    local_touchpos = widget_size;
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }
                                            }

                                            if (!b_touch)
                                            {
                                                coverFlow->touchPos = y;
                                            }
                                            else
                                            {
                                                coverFlow->touchPos = local_touchpos; // coverFlow->boundary_touch_memo;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        fy = widget->rect.height / 2 - base_size / 2;
                                    }

                                    if (coverFlow->boundaryAlign && b_touch)
                                    {
                                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                            !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                        {
                                            // widget->flags |= ITU_UNDRAGGING;
                                            // ituScene->dragged = NULL;
                                            coverFlow->boundary_touch_memo = y;

                                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE1) && (offset > 0))
                                            {
                                                break;
                                            }
                                            else if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE2) &&
                                                     (offset < 0))
                                            {
                                                break;
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                    if (!((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)))
                                    {
                                        if (coverFlow->boundaryAlign)
                                        {
                                            int max_neighbor_item =
                                                (CoverFlowSaveDiv(widget->rect.height, base_size) - 1) / 2;
                                            int max_height_item = CoverFlowSaveDiv(widget->rect.height, base_size);
                                            fy                  = 0;

                                            if (max_neighbor_item == 0)
                                            {
                                                max_neighbor_item++;
                                            }

                                            if (coverFlow->focusIndex > 0) //>= max_neighbor_item)
                                            {
                                                if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                                    ((childCount - coverFlow->focusIndex - 1) < max_height_item))
                                                {
                                                    fy = widget->rect.height - (childCount * base_size);
                                                }
                                                else
                                                {
                                                    fy -= base_size * coverFlow->focusIndex;
                                                }
                                            }
                                            else
                                            {
                                                fy = 0;
                                            }
                                        }
                                        else
                                        {
                                            fy -= base_size * coverFlow->focusIndex;
                                        }
                                    }

                                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                        !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                    {
                                        if (child != NULL)
                                        {
                                            ituCoverFlowSetXY(coverFlow, i, fy + (i * child->rect.height), __LINE__);
                                        }
                                    }
                                    else
                                    {
                                        if (coverFlow->overlapsize > 0)
                                        {
                                            fy += i * base_size;
                                            fy += offset;
                                            ituCoverFlowSetXY(coverFlow, i, fy - coverFlow->overlapsize, __LINE__);
                                        }
                                        else
                                        {
                                            if (child != NULL)
                                            {
                                                fy += i * child->rect.height;
                                                fy += offset;
                                                ituCoverFlowSetXY(coverFlow, i, fy, __LINE__);
                                            }
                                        }
                                    }

                                    if (i == 0)
                                    {
                                        if (coverFlow->overlapsize > 0)
                                        {
                                            coverFlow->movelog = fy - coverFlow->overlapsize;
                                        }
                                        else
                                        {
                                            coverFlow->movelog = fy;
                                        }
                                    }
                                }
                            }
                        }
                        ////////////////////////////
                        // ITU_EVENT_MOUSEMOVE
                        // Horizontal Mode
                        ////////////////////////////
                        else
                        {
                            // offset = x - coverFlow->touchPos;
                            int offset = dist;

                            // (void)printf("0: offset=%d\n", offset);
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                            {
                                if (abs(offset) < CoverFlowSaveDiv(base_size, coverFlow->min_change_dist_factor))
                                {
                                    if (coverFlow->dragFlag == 0)
                                    {
                                        coverFlow->dragFlag = 1;
                                    }
                                }
                                else
                                {
                                    coverFlow->dragFlag = -1;
                                }

                                if (offset >= base_size)
                                {
                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                    }

                                    if (coverFlow->focusIndex > 0)
                                    {
                                        coverFlow->focusIndex--;
                                    }
                                    else
                                    {
                                        coverFlow->focusIndex = childCount - 1;
                                    }

                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                    }

                                    offset -= base_size;
                                    coverFlow->touchPos += base_size;
                                    coverFlow->dragFlag = -1;
                                }

                                if (offset <= (base_size * -1))
                                {
                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                    }

                                    if (coverFlow->focusIndex < (childCount - 1))
                                    {
                                        coverFlow->focusIndex++;
                                    }
                                    else
                                    {
                                        coverFlow->focusIndex = 0;
                                    }

                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                    {
                                        (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                    }

                                    offset += base_size;
                                    coverFlow->touchPos -= base_size;
                                    coverFlow->dragFlag = -1;
                                }
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                            {
                                int i, index, fx;
                                index = coverFlow->focusIndex;
                                fx = widget_size / 2 - base_size / 2;
                                for (i = 0; i < childCount; i++)
                                {
                                    ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, index);
                                    if (child != NULL)
                                    {
                                        ituCoverFlowSetXY(coverFlow, index, fx + offset, __LINE__);
                                    fx += base_size;
                                    }

                                    if (index >= childCount - 1)
                                    {
                                        index = 0;
                                    }
                                    else
                                    {
                                        index++;
                                    }
                                }
                                fx = widget_size / 2 - base_size / 2; //reset for next loop
                                index = coverFlow->focusIndex - 1;
                                if (index < 0)
                                {
                                    index = childCount - 1;
                                }
                                for (i = 0; i < childCount; ++i)
                                {
                                    if ((fx + offset) >= 0)
                                    {
                                        ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, index);
                                        if (child != NULL)
                                        {
                                        	fx -= base_size;
                                            ituCoverFlowSetXY(coverFlow, index, fx + offset, __LINE__);
                                        }

                                        if (index == 0)
                                        {
                                            index = childCount - 1;
                                        }
                                        else
                                        {
                                            index--;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // limit the move under non-cycle/Horizontal boundaryAlign mode
                                int  i, fx = 0;
                                int  boundary_over = 0;
                                bool b_touch       = false;

                                if ((coverFlow->boundaryAlign) && (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
                                {
                                    ITUWidget * child_1 = CoverFlowGetVisibleChild(coverFlow, 0);
                                    ITUWidget * child_2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                    if ((child_1 != NULL) && (child_2 != NULL))
                                    {
                                        if ((child_1->rect.x + offset) >
                                            CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                        {
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                            b_touch = true;
                                        }
                                        else if ((child_2->rect.x + base_size + offset) <
                                            (widget->rect.width - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                        {
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                            b_touch = true;
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                }
                                

                                if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
                                {
                                    if (childCount == 1) // for special one item case
                                    {
                                        if (coverFlow->bounceRatio > 0)
                                        {
                                            if (offset >= CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                            {
                                                offset = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                            }
                                            else if (offset <= -1 * CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                            {
                                                offset = -1 * CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }
                                        }
                                    }
                                    else if ((coverFlow->boundary_memo1 + offset) >=
                                             (CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) + center_align_pos1))
                                    {
                                        offset =
                                            (CoverFlowSaveDiv(base_size, coverFlow->bounceRatio) + center_align_pos1) -
                                            coverFlow->boundary_memo1;
                                    }
                                    else if ((coverFlow->boundary_memo2 + offset) <=
                                             (center_align_pos2 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                    {
                                        offset =
                                            ((center_align_pos2 - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) -
                                             coverFlow->boundary_memo2);
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }

                                if (coverFlow->bounceRatio == 0)
                                {
                                    offset = 0;
                                }

                                for (i = 0; i < childCount; ++i)
                                {
                                    ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);

                                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                        !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                    {
                                        if (i == 0)
                                        {
                                            ITUWidget * child_1        = CoverFlowGetVisibleChild(coverFlow, 0);
                                            ITUWidget * child_2        = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                            int         local_touchpos = 0;
                                            fx                         = child_1->rect.x + offset;

                                            if (b_touch)
                                            {
                                                if ((child_1 != NULL) && (child_2 != NULL))
                                                {
                                                    if ((child_1->rect.x + offset) >
                                                        CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                                    {
                                                        fx = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                                        local_touchpos = 0;
                                                    }
                                                    else if ((child_2->rect.x + base_size + offset) <
                                                        (widget->rect.width -
                                                            CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)))
                                                    {
                                                        fx = (widget->rect.width -
                                                            CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) -
                                                            childCount * base_size;
                                                        local_touchpos = widget_size;
                                                    }
                                                    else
                                                    {
                                                        /* do nothing */
                                                    }
                                                }
                                            }

                                            if (!b_touch)
                                            {
                                                coverFlow->touchPos = x;
                                            }
                                            else
                                            {
                                                coverFlow->touchPos = local_touchpos; 
                                                // coverFlow->boundary_touch_memo;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        fx = widget->rect.width / 2 - base_size / 2;
                                    }

                                    if (coverFlow->boundaryAlign && b_touch)
                                    {
                                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                            !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                        {
                                            coverFlow->boundary_touch_memo = x;

                                            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE1) && (offset > 0))
                                            {
                                                break;
                                            }
                                            else if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE2) &&
                                                     (offset < 0))
                                            {
                                                break;
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                    if (!((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)))
                                    {
                                        if (coverFlow->boundaryAlign)
                                        { //[MOVE][Horizontal][non-cycle][layout]
                                            int max_neighbor_item =
                                                (CoverFlowSaveDiv(widget->rect.width, base_size) - 1) / 2;
                                            int max_width_item = CoverFlowSaveDiv(widget->rect.width, base_size);
                                            fx                 = 0;

                                            if (max_neighbor_item == 0)
                                            {
                                                max_neighbor_item++;
                                            }

                                            if (coverFlow->focusIndex > 0) //>= max_neighbor_item)
                                            {
                                                if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                                    ((childCount - coverFlow->focusIndex - 1) < max_width_item))
                                                {
                                                    fx = widget->rect.width - (childCount * base_size);
                                                }
                                                else
                                                {
                                                    fx -= base_size * coverFlow->focusIndex;
                                                }
                                            }
                                            else
                                            {
                                                fx = 0;
                                            }
                                        }
                                        else
                                        {
                                            fx -= base_size * coverFlow->focusIndex;
                                        }
                                    }

                                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                                        !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                                    {
                                        if (child != NULL)
                                        {
                                            ituCoverFlowSetXY(coverFlow, i, fx + (i * child->rect.width), __LINE__);
                                        }
                                    }
                                    else
                                    {
                                        // for powei
                                        if (coverFlow->overlapsize > 0)
                                        {
                                            fx += i * base_size;
                                            fx += offset;
                                            ituCoverFlowSetXY(coverFlow, i, fx - coverFlow->overlapsize, __LINE__);
                                        }
                                        else
                                        {
                                            fx += i * base_size;
                                            fx += offset;

                                            // boundary check again
                                            if ((i == 0) && (coverFlow->boundaryAlign))
                                            {
                                                int base_bounce =
                                                    ((coverFlow->bounceRatio <= 0)
                                                         ? (1)
                                                         : CoverFlowSaveDiv(base_size, coverFlow->bounceRatio));
                                                int start_check_pos = fx;
                                                int end_check_pos   = fx + (childCount * base_size);

                                                if (start_check_pos > base_bounce)
                                                {
                                                    boundary_over = base_bounce - start_check_pos;
                                                    (void)printf("[Boundary over][bypass][line %d]\n", __LINE__);
                                                }
                                                else if (end_check_pos < (widget_size - base_bounce))
                                                {
                                                    boundary_over = (widget_size - base_bounce) - end_check_pos;
                                                    (void)printf("[Boundary over][bypass][line %d]\n", __LINE__);
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }
                                            }

                                            if (boundary_over)
                                            {
                                                fx += boundary_over;
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, fx, __LINE__);
                                        }

                                    }
                                    if (i == 0)
                                    {
                                        if (coverFlow->overlapsize > 0)
                                        {
                                            coverFlow->movelog = fx - coverFlow->overlapsize;
                                        }
                                        else
                                        {
                                            coverFlow->movelog = fx;
                                        }
                                    }
                                }
                            }
                        }
                        result = true;
                        ituDirtyWidget(widget, result);
                    }
                }
                else
                {
                    ituUnPressWidget(widget);
                    // ituWidgetUpdate(coverFlow, ITU_EVENT_MOUSEUP, arg1, arg2, arg3);
                    if (!ituWidgetIsInside(widget, x, y))
                    {
                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
                        {
                            ituWidgetUpdate(coverFlow, ITU_EVENT_MOUSEUP, arg1, arg2, arg3);
                        }
                    }
                    return true;
                }
            }
        }
        else if (ev == ITU_EVENT_MOUSEDOWN)
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                if (coverFlow->focusIndex < 0)
                {
                    coverFlow->focusIndex = 0;
                }
                else if (coverFlow->focusIndex > (childCount - 1))
                {
                    coverFlow->focusIndex = childCount - 1;
                }
                else
                {
                    /* do nothing */
                }

                coverFlow->bounceFlag = 0;
            }

            coverFlow->clock = itpGetTickCount();
            coverFlow->dragFlag = 0;

            if (ituWidgetIsInside(widget, x, y))
            {
                if (ituScene->dragged != NULL)
                {
                    ITCTree *         tree = (ITCTree *)ituScene->dragged;
                    if (tree != NULL)
                    {
                        const ITUWidget* ww = (ITUWidget*)tree->parent;
                        if (ww != NULL)
                        {
                            if (ww->type != ITU_TRACKBAR)
                            {
                                (void)printf("other widget dragging, name %s\n", widget->name);
                                ituUnPressWidget(widget);
                                return true;
                            }
                            else
                            {
                                return true;
                            }
                        }
                    }
                }

                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                {
                    ITUWidget * c1 = CoverFlowGetVisibleChild(coverFlow, 0);

                    // side effect when use prev or next to slide but set slidemaxcount is 0
                    if (coverFlow->slideMaxCount == 0)
                    {
                        return true;
                    }

                    coverFlow->frame = coverFlow->totalframe;
                    // (void)printf("sliding bypass action.\n");

                    if (c1 != NULL)
                    {
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                coverFlow->mousedown_position = c1->rect.y;
                                coverFlow->touchPos = y;
                            }
                            else
                            {
                                coverFlow->mousedown_position = c1->rect.x;
                                coverFlow->touchPos = x;
                            }

                        }
                        else
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                coverFlow->mousedown_position = c1->rect.y;
                                coverFlow->boundary_touch_memo = 0;
                                coverFlow->touchPos = y;
                            }
                            else
                            {
                                coverFlow->mousedown_position = c1->rect.x;
                                coverFlow->boundary_touch_memo = 0;
                                coverFlow->touchPos = x;
                            }
                        }
                    }

                    if (childCount > 1)
                    {
                        if ((coverFlow->inc < 0) && (coverFlow->focusIndex < (childCount - 1)))
                        {
                            coverFlow->frame = coverFlow->totalframe;
                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                        }
                        else if ((coverFlow->inc > 0) && (coverFlow->focusIndex > 0))
                        {
                            coverFlow->frame = coverFlow->totalframe;
                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }

                    CoverFlowCleanQueue(coverFlow);

                    if (widget->flags & ITU_DRAGGABLE)
                    {
                        widget->flags |= ITU_DRAGGING;
                        ituScene->dragged = widget;
                    }
                    return true;
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    coverFlow->init_drag_xy = y;
                }
                else
                {
                    coverFlow->init_drag_xy = x;
                }

                coverFlow->frame = coverFlow->totalframe;
            }

            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGABLE))
            {
                ITUWidget * c1 = CoverFlowGetVisibleChild(coverFlow, 0);
                ITUWidget * c2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                if ((c1 != NULL) && (c2 != NULL))
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        coverFlow->mousedown_position = c1->rect.y;
                        coverFlow->boundary_memo1 = c1->rect.y;
                        coverFlow->boundary_memo2 = c2->rect.y + base_size;
                    }
                    else
                    {
                        coverFlow->mousedown_position = c1->rect.x;
                        coverFlow->boundary_memo1 = c1->rect.x;
                        coverFlow->boundary_memo2 = c2->rect.x + base_size;
                    }
                }
            }

            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGABLE) && coverFlow->bounceRatio > 0)
            {
                if (ituWidgetIsInside(widget, x, y))
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        coverFlow->touchPos = y;
                    }
                    else
                    {
                        coverFlow->touchPos = x;
                    }

                    if (widget->flags & ITU_HAS_LONG_PRESS)
                    {
                        coverFlow->touchCount = 1;
                    }
                    else
                    {
                        widget->flags |= ITU_DRAGGING;
                        ituScene->dragged = widget;
                    }
                }
            }
        }
        else if (ev == ITU_EVENT_MOUSEUP)
        {
            if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)) && coverFlow->boundaryAlign &&
                (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
            { // fix the error position when drag too much outside or too fast then mouse up
                ITUWidget * child_1 = CoverFlowGetVisibleChild(coverFlow, 0);
                ITUWidget * child_2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                if ((child_1 != NULL) && (child_2 != NULL))
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        if ((child_1->rect.y > 0) || ((child_2->rect.y + child_2->rect.height) < widget->rect.height))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.y > 0)
                            {
                                move_step = 0 - child_1->rect.y;
                            }
                            else
                            {
                                move_step = widget->rect.height - (child_2->rect.y + child_2->rect.height);
                            }

                            for (i = 0; i < childCount; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.y;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                    else
                    {
                        if ((child_1->rect.x > 0) || ((child_2->rect.x + child_2->rect.width) < widget->rect.width))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.x > 0)
                            {
                                move_step = 0 - child_1->rect.x;
                            }
                            else
                            {
                                move_step = widget->rect.width - (child_2->rect.x + child_2->rect.width);
                            }

                            for (i = 0; i < childCount; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.x;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                }
            }

            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGABLE) && ((widget->flags & ITU_DRAGGING)) &&
                (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)))
            {
                int x  = arg2 - widget->rect.x;
                int y  = arg3 - widget->rect.y;

                result = false;

                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                    {
                        if (!result && childCount > 0)
                        {
                            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, 0);

                            if ((coverFlow->inc == 0) && (child != NULL))
                            {
                                int offset, absoffset, interval;

                                offset   = y - coverFlow->touchPos;
                                interval = CoverFlowSaveDiv(offset, child->rect.height);
                                offset -= (interval * child->rect.height);
                                absoffset = offset > 0 ? offset : -offset;

                                if (absoffset > child->rect.height / 2)
                                {
                                    coverFlow->frame =
                                        (int)((float)coverFlow->totalframe *
                                              CoverFlowSaveDivF((float)absoffset, (float)child->rect.height));

                                    if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                               coverFlow->focusIndex - interval, __LINE__);
                                    }

                                    coverFlow->focusIndex -= interval;

                                    if (offset >= 0)
                                    {
                                        if (coverFlow->focusIndex < 0)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex + childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex += childCount;
                                        }
                                        ituCoverFlowFixFC(coverFlow);
                                        ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                       coverFlow->focusIndex);
                                        ituLog("1: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        if (coverFlow->focusIndex >= childCount)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex - childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex -= childCount;
                                        }
                                        ituCoverFlowFixFC(coverFlow);
                                        ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                       coverFlow->focusIndex);
                                        ituLog("2: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }

                                    // try fix bug
                                    coverFlow->inc = offset;
                                }
                                else if (absoffset)
                                {
                                    coverFlow->frame = (int)((float)coverFlow->totalframe *
                                                             CoverFlowSaveDivF((float)absoffset, (float)base_size));

                                    if (offset >= 0)
                                    {
                                        if (coverFlow->focusIndex < 0)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex + childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex += childCount;
                                        }

                                        ituLog("3: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        if (coverFlow->focusIndex >= childCount)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       coverFlow->focusIndex - childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex -= childCount;
                                        }

                                        ituLog("4: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }

                                    if (coverFlow->dragFlag == 1) // small dragging
                                    {
                                        coverFlow->inc = offset * -1;
                                    }
                                    else
                                    {
                                        coverFlow->inc = offset;
                                    }
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            widget->flags |= ITU_UNDRAGGING;
                            ituScene->dragged = NULL;
                        }
                    }
                    else
                    {
                        if ((childCount > 0) && (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)))
                        {   //[MOUSEUP][Vertical][non-cycle][layout]
                            bool boundary_touch = false;
                            if (coverFlow->boundaryAlign)
                            {
                                int max_neighbor_item = (CoverFlowSaveDiv(widget->rect.height, base_size) - 1) / 2;
                                int max_height_item   = CoverFlowSaveDiv(widget->rect.height, base_size);

                                if (max_neighbor_item == 0)
                                {
                                    max_neighbor_item++;
                                }

                                if (coverFlow->focusIndex >= max_neighbor_item)
                                {
                                    if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                        ((childCount - coverFlow->focusIndex - 1) < max_neighbor_item))
                                    {
                                        boundary_touch = true;
                                    }
                                    else
                                    {
                                        if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                            ((childCount - coverFlow->focusIndex - 1) < max_height_item))
                                        {
                                            boundary_touch = true;
                                        }
                                    }
                                }
                                else
                                {
                                    boundary_touch = true;
                                }

                                if (!boundary_touch) // re-check again for some special case that with bad
                                                     // max_neighbor_item
                                { // this case should debug for some extreme item count
                                    ITUWidget * child1 = CoverFlowGetVisibleChild(coverFlow, 0);
                                    ITUWidget * child2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                    int         min_item_pos = 0, max_item_pos = 0, min_boundary_value = 0, max_boundary_value = 0;
                                    if ((child1 != NULL) && (child2 != NULL))
                                    {
                                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                        {
                                            min_item_pos = child1->rect.y;
                                            max_item_pos = child2->rect.y + base_size;
                                        }
                                        else
                                        {
                                            min_item_pos = child1->rect.x;
                                            max_item_pos = child2->rect.x + base_size;
                                        }
                                    }
                                    min_boundary_value = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    max_boundary_value =
                                        widget_size - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                    if (min_item_pos >= min_boundary_value)
                                    {
                                        boundary_touch = true;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex, 0,
                                                   __LINE__);
                                        }

                                        coverFlow->focusIndex = 0;
                                    }
                                    else if (max_item_pos <= max_boundary_value)
                                    {
                                        boundary_touch = true;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                childCount - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex = childCount - 1;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                            }

                            if (coverFlow->inc == 0)
                            {
                                int offset, absoffset, interval;

                                offset   = CoverFlowGetDraggingDist(coverFlow);
                                interval = CoverFlowSaveDiv(offset, base_size);
                                offset -= (interval * base_size);

                                //////////check bounce
                                if (coverFlow->focusIndex <= 0)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset >= CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                        {
                                            offset = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                        }

                                        if (offset == 0)
                                        {
                                            offset = CoverFlowGetDraggingDist(coverFlow);
                                        }
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
                                else if (coverFlow->focusIndex >= childCount - 1)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset <= CoverFlowSaveDiv(-base_size, coverFlow->bounceRatio))
                                        {
                                            offset = CoverFlowSaveDiv(-base_size, coverFlow->bounceRatio);
                                        }

                                        if (offset == 0)
                                        {
                                            offset = CoverFlowGetDraggingDist(coverFlow);
                                        }
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
                                else
                                {
                                    /* do nothing */
                                }
                                absoffset = offset > 0 ? offset : -offset;

                                if ((absoffset > (base_size / 2)) ||
                                    (absoffset > CoverFlowSaveDiv(widget_size, coverFlow->min_change_dist_factor)))
                                { // small shift alignment
                                    if (offset >= 0)
                                    {
                                        coverFlow->frame = 0;

                                        if (coverFlow->focusIndex > interval)
                                        {
                                            // fix the alignment wrong when mouseup at maximum focusindex then stop
                                            // wrong layout when mouseup done.
                                            int mod = 1;
                                            if ((coverFlow->boundaryAlign) && (coverFlow->focusIndex == (childCount - 1)))
                                            {
                                                int max_touch_start_size_index =
                                                    (childCount - CoverFlowSaveDiv(widget_size, base_size));
                                                mod = coverFlow->focusIndex - max_touch_start_size_index + 1;
                                            }

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                            }

                                            coverFlow->focusIndex -= interval + mod;

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                            }

                                            if (coverFlow->focusIndex < 0)
                                            {
                                                coverFlow->focusIndex = 0;
                                            }

                                            if (boundary_touch)
                                            {
                                                while ((CoverFlowCheckBoundaryTouch(widget) == ITU_BOUNCE_2) &&
                                                       (coverFlow->focusIndex > 0))
                                                {
                                                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                                                    {
                                                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n",
                                                               coverFlow->focusIndex, coverFlow->focusIndex - 1,
                                                               __LINE__);
                                                    }

                                                    coverFlow->focusIndex--;
                                                }
                                            }
                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                            ituLog("5: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                        else
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       -1, __LINE__);
                                            }

                                            coverFlow->focusIndex = -1;
                                            ituLog("6: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                    }
                                    else
                                    {
                                        coverFlow->frame = 0;

                                        if (coverFlow->focusIndex < childCount + interval - 1)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                            }

                                            coverFlow->focusIndex -= interval - 1;

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                            }

                                            if (coverFlow->focusIndex > (childCount - 1))
                                            {
                                                coverFlow->focusIndex = childCount - 1;
                                            }

                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                            ituLog("7: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                        else
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex = childCount;
                                            ituLog("8: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                    }
                                    coverFlow->inc = offset * -1;
                                }
                                else if (absoffset)
                                {
                                    coverFlow->frame = (int)((float)coverFlow->totalframe *
                                                             CoverFlowSaveDivF((float)absoffset, (float)base_size));

                                    if (offset >= 0)
                                    { // big shift alignment
                                        int lastf = coverFlow->focusIndex;

                                        // fix the alignment wrong when mouseup at maximum focusindex then stop wrong
                                        // layout when mouseup done.
                                        int mod   = 0;
                                        if ((coverFlow->boundaryAlign) && (coverFlow->focusIndex == (childCount - 1)))
                                        {
                                            int max_touch_start_size_index =
                                                (childCount - CoverFlowSaveDiv(widget_size, base_size));
                                            mod = coverFlow->focusIndex - max_touch_start_size_index;
                                        }

                                        if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - interval - mod, __LINE__);
                                        }

                                        coverFlow->focusIndex -= interval + mod;

                                        // small shift alignment
                                        if (boundary_touch)
                                        {
                                            while ((CoverFlowCheckBoundaryTouch(widget) == ITU_BOUNCE_2) &&
                                                   (coverFlow->focusIndex > 0))
                                            {
                                                if (COVERFLOW_DEBUG_FOCUSINDEX)
                                                {
                                                    (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n",
                                                           coverFlow->focusIndex, coverFlow->focusIndex - 1, __LINE__);
                                                }

                                                coverFlow->focusIndex--;
                                            }
                                        }

                                        if (coverFlow->focusIndex < 0)
                                        {
                                            coverFlow->focusIndex = 0;
                                        }

                                        if (lastf != coverFlow->focusIndex)
                                        {
                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                        }
                                        ituLog("9: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        int lastf = coverFlow->focusIndex;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - interval, __LINE__);
                                        }

                                        coverFlow->focusIndex -= interval;

                                        if (coverFlow->focusIndex >= childCount + 1)
                                        {
                                            coverFlow->focusIndex = childCount;
                                        }

                                        if (lastf != coverFlow->focusIndex)
                                        {
                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                        }
                                        ituLog("10: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    coverFlow->inc = offset;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                        }

                        if (widget)
                        {
                            ITUWidget * c1         = CoverFlowGetVisibleChild(coverFlow, 0);
                            ITUWidget * c2         = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                            int         pos_center = (widget_size / 2) - (base_size / 2);

                            if ((coverFlow->inc) && (c1 != NULL) && (c2 != NULL))
                            {
                                if (((coverFlow->focusIndex <= 0) && (c1->rect.y == pos_center)) ||
                                    ((coverFlow->focusIndex >= (childCount - 1)) && (c2->rect.y == pos_center)))
                                {
                                    ituScene->dragged = NULL;
                                    coverFlow->frame = 0;
                                    coverFlow->inc = 0;
                                    (void)printf("[MouseUP][coverflow check fail][%d]\n", __LINE__);
                                }
                            }
                        }
                        else
                        {
                            widget->flags |= ITU_UNDRAGGING;
                            ituScene->dragged = NULL;
                        }
                    }
                }
                else
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                    {
                        if (childCount > 0)
                        {
                            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, 0);

                            if ((coverFlow->inc == 0) && (child != NULL))
                            {
                                int offset, absoffset, interval;

                                offset = x - coverFlow->touchPos;
                                interval = CoverFlowSaveDiv(offset, child->rect.width);
                                offset -= (interval * child->rect.width);
                                absoffset = offset > 0 ? offset : -offset;

                                if (absoffset > child->rect.width / 2)
                                {
                                    coverFlow->frame =
                                        (int)((float)coverFlow->totalframe *
                                            CoverFlowSaveDivF((float)absoffset, (float)child->rect.width));

                                    if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                    {
                                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                            coverFlow->focusIndex - interval, __LINE__);
                                    }

                                    coverFlow->focusIndex -= interval;

                                    if (offset >= 0)
                                    {
                                        if (coverFlow->focusIndex < 0)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    coverFlow->focusIndex + childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex += childCount;
                                        }
                                        ituCoverFlowFixFC(coverFlow);
                                        ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                            coverFlow->focusIndex);
                                        ituLog("1: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                            coverFlow->frame, offset, coverFlow->inc, interval,
                                            coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        if (coverFlow->focusIndex >= childCount)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    coverFlow->focusIndex - childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex -= childCount;
                                        }
                                        ituCoverFlowFixFC(coverFlow);
                                        ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                            coverFlow->focusIndex);
                                        ituLog("2: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                            coverFlow->frame, offset, coverFlow->inc, interval,
                                            coverFlow->focusIndex);
                                    }

                                    // try fix bug
                                    coverFlow->inc = offset;
                                }
                                else if (absoffset)
                                {
                                    coverFlow->frame = (int)((float)coverFlow->totalframe *
                                        CoverFlowSaveDivF((float)absoffset, (float)base_size));

                                    if (offset >= 0)
                                    {
                                        if (coverFlow->focusIndex < 0)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    coverFlow->focusIndex + childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex += childCount;
                                        }

                                        ituLog("3: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                            coverFlow->frame, offset, coverFlow->inc, interval,
                                            coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        if (coverFlow->focusIndex >= childCount)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    coverFlow->focusIndex - childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex -= childCount;
                                        }

                                        ituLog("4: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                            coverFlow->frame, offset, coverFlow->inc, interval,
                                            coverFlow->focusIndex);
                                    }

                                    if (coverFlow->dragFlag == 1) // small dragging
                                    {
                                        coverFlow->inc = offset * -1;
                                    }
                                    else
                                    {
                                        coverFlow->inc = offset;
                                    }
                                }
                                else
                                {
                                    /* do nothing */
                                }
                                widget->flags |= ITU_UNDRAGGING;
                                ituScene->dragged = NULL;
                            }
                        }
                    }
                    else
                    {
                        if ((childCount > 0) && (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)))
                        {
                            //[MOUSEUP][Horizontal][non-cycle][layout]
                            if (coverFlow->boundaryAlign)
                            {
                                int  max_neighbor_item = (CoverFlowSaveDiv(widget->rect.width, base_size) - 1) / 2;
                                int  max_width_item    = CoverFlowSaveDiv(widget->rect.width, base_size);
                                bool boundary_touch    = false;

                                if (max_neighbor_item == 0)
                                {
                                    max_neighbor_item++;
                                }

                                if (coverFlow->focusIndex >= max_neighbor_item)
                                {
                                    if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                        ((childCount - coverFlow->focusIndex - 1) < max_neighbor_item))
                                    {
                                        boundary_touch = true;
                                    }
                                    else
                                    {
                                        if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                            ((childCount - coverFlow->focusIndex - 1) < max_width_item))
                                        {
                                            boundary_touch = true;
                                        }
                                    }
                                }
                                else
                                {
                                    boundary_touch = true;
                                }

                                if (!boundary_touch) // re-check again for some special case that with bad
                                                     // max_neighbor_item
                                { // this case should debug for some extreme item count
                                    ITUWidget * child1 = CoverFlowGetVisibleChild(coverFlow, 0);
                                    ITUWidget * child2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                    int         min_item_pos, max_item_pos, min_boundary_value, max_boundary_value;
                                    if ((child1 != NULL) && (child2 != NULL))
                                    {
                                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                        {
                                            min_item_pos = child1->rect.y;
                                            max_item_pos = child2->rect.y + base_size;
                                        }
                                        else
                                        {
                                            min_item_pos = child1->rect.x;
                                            max_item_pos = child2->rect.x + base_size;
                                        }
                                    }
                                    min_boundary_value = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                    max_boundary_value =
                                        widget_size - CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);

                                    if (min_item_pos >= min_boundary_value)
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex, 0,
                                                   __LINE__);
                                        }

                                        coverFlow->focusIndex = 0;
                                    }
                                    else if (max_item_pos <= max_boundary_value)
                                    {
                                        if (COVERFLOW_DEBUG_FOCUSINDEX)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                childCount - 1, __LINE__);
                                        }

                                        coverFlow->focusIndex = childCount - 1;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                            }

                            if (coverFlow->inc == 0)
                            {
                                int offset, absoffset, interval;

                                offset   = CoverFlowGetDraggingDist(coverFlow);
                                interval = CoverFlowSaveDiv(offset, base_size);
                                offset -= (interval * base_size);

                                ///////////// check bounce
                                if (coverFlow->focusIndex <= 0)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset >= CoverFlowSaveDiv(base_size, coverFlow->bounceRatio))
                                        {
                                            offset = CoverFlowSaveDiv(base_size, coverFlow->bounceRatio);
                                        }

                                        if (offset == 0)
                                        {
                                            offset = CoverFlowGetDraggingDist(coverFlow);
                                        }
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
                                else if (coverFlow->focusIndex >= childCount - 1)
                                {
                                    if (coverFlow->bounceRatio > 0)
                                    {
                                        if (offset <= CoverFlowSaveDiv(-base_size, coverFlow->bounceRatio))
                                        {
                                            offset = CoverFlowSaveDiv(-base_size, coverFlow->bounceRatio);
                                        }

                                        if (offset == 0)
                                        {
                                            offset = CoverFlowGetDraggingDist(coverFlow);
                                        }
                                    }
                                    else
                                    {
                                        offset = 0;
                                    }
                                }
                                else
                                {
                                    /* do nothing */
                                }
                                absoffset = offset > 0 ? offset : -offset;

                                if ((absoffset > (base_size / 2)) ||
                                    (absoffset > CoverFlowSaveDiv(widget_size, coverFlow->min_change_dist_factor)))
                                {
                                    if (offset >= 0)
                                    {
                                        coverFlow->frame = 0;

                                        if (coverFlow->focusIndex > interval)
                                        {
                                            // fix the alignment wrong when mouseup at maximum focusindex then stop
                                            // wrong layout when mouseup done.
                                            int mod = 1;
                                            int max_touch_start_size_index =
                                                (childCount - CoverFlowSaveDiv(widget_size, base_size));

                                            if ((coverFlow->boundaryAlign) &&
                                                (coverFlow->focusIndex > max_touch_start_size_index))
                                            {
                                                mod = coverFlow->focusIndex - max_touch_start_size_index + 1;
                                            }

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                            }

                                            coverFlow->focusIndex -= interval + mod;

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                            }

                                            if (coverFlow->focusIndex < 0)
                                            {
                                                coverFlow->focusIndex = 0;
                                            }

                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                            ituLog("5: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                        else
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                       -1, __LINE__);
                                            }

                                            coverFlow->focusIndex = -1;
                                            ituLog("6: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                    }
                                    else
                                    {
                                        coverFlow->frame = 0;

                                        if (coverFlow->focusIndex < childCount + interval - 1)
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                                            }

                                            coverFlow->focusIndex -= interval - 1;

                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                                            }

                                            if (coverFlow->focusIndex > (childCount - 1))
                                            {
                                                coverFlow->focusIndex = childCount - 1;
                                            }

                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                            ituLog("7: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                        else
                                        {
                                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                                            {
                                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                    childCount, __LINE__);
                                            }

                                            coverFlow->focusIndex = childCount;
                                            ituLog("8: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                                   coverFlow->frame, offset, coverFlow->inc, interval,
                                                   coverFlow->focusIndex);
                                        }
                                    }
                                    // try fix bug
                                    coverFlow->inc = offset * -1;
                                    
                                }
                                else if (absoffset)
                                {
                                    coverFlow->frame = (int)((float)coverFlow->totalframe *
                                                             CoverFlowSaveDivF((float)absoffset, (float)base_size));

                                    if (offset >= 0)
                                    {
                                        int lastf = coverFlow->focusIndex;
                                        int max_touch_start_size_index =
                                            (childCount - CoverFlowSaveDiv(widget_size, base_size));
                                        // fix the alignment wrong when mouseup at maximum focusindex then stop wrong
                                        // layout when mouseup done.
                                        int mod = 0;
                                        if ((coverFlow->boundaryAlign) &&
                                            (coverFlow->focusIndex >= max_touch_start_size_index))
                                        {
                                            mod = coverFlow->focusIndex - max_touch_start_size_index;
                                        }

                                        if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - interval - mod, __LINE__);
                                        }

                                        coverFlow->focusIndex -= interval + mod;

                                        if (coverFlow->focusIndex < 0)
                                        {
                                            coverFlow->focusIndex = 0;
                                        }

                                        if (lastf != coverFlow->focusIndex)
                                        {
                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                        }
                                        ituLog("9: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    else
                                    {
                                        // coverFlow->inc = base_size;
                                        int lastf = coverFlow->focusIndex;

                                        if (COVERFLOW_DEBUG_FOCUSINDEX && interval)
                                        {
                                            (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                                   coverFlow->focusIndex - interval, __LINE__);
                                        }

                                        coverFlow->focusIndex -= interval;

                                        if (coverFlow->focusIndex >= childCount + 1)
                                        {
                                            coverFlow->focusIndex = childCount;
                                        }

                                        if (lastf != coverFlow->focusIndex)
                                        {
                                            ituCoverFlowFixFC(coverFlow);
                                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED,
                                                           coverFlow->focusIndex);
                                        }
                                        ituLog("10: frame=%d offset=%d inc=%d interval=%d focusIndex=%d\n",
                                               coverFlow->frame, offset, coverFlow->inc, interval,
                                               coverFlow->focusIndex);
                                    }
                                    coverFlow->inc = offset;
                                }
                                else
                                {
                                    /* do nothing */
                                }

                                if (widget)
                                {
                                    ITUWidget * c1         = CoverFlowGetVisibleChild(coverFlow, 0);
                                    ITUWidget * c2         = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                    int         pos_center = (widget_size / 2) - (base_size / 2);

                                    if ((coverFlow->inc) && (c1 != NULL) && (c2 != NULL))
                                    {
                                        if (((coverFlow->focusIndex <= 0) && (c1->rect.x == pos_center)) ||
                                            ((coverFlow->focusIndex >= (childCount - 1)) && (c2->rect.x == pos_center)))
                                        {
                                            ituScene->dragged = NULL;
                                            coverFlow->frame = 0;
                                            coverFlow->inc = 0;
                                            (void)printf("[MouseUP][coverflow check fail][%d]\n", __LINE__);
                                        }
                                        else
                                        {
                                            widget->flags |= ITU_UNDRAGGING;
                                            ituScene->dragged = NULL;
                                        }
                                    }
                                }
                                else
                                {
                                    widget->flags |= ITU_UNDRAGGING;
                                    ituScene->dragged = NULL;
                                }
                                // widget->flags |= ITU_UNDRAGGING;
                                // ituScene->dragged = NULL;
                            }
                        }
                    }
                }
                result = true;
            }
            widget->flags &= ~ITU_DRAGGING;
            coverFlow->touchCount = 0;
            coverFlow->touchPos   = 0;
        }
        else
        {
            /* do nothing */
        }
    }

    if (ev == ITU_EVENT_TIMER)
    {
        if (coverFlow->touchCount > 0)
        {
            int x, y, dist;

            assert(widget->flags & ITU_HAS_LONG_PRESS);

            ituWidgetGetGlobalPosition(widget, &x, &y);

            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
            {
                dist = ituScene->lastMouseY - (y + coverFlow->touchPos);
            }
            else
            {
                dist = ituScene->lastMouseX - (x + coverFlow->touchPos);
            }

            if (dist < 0)
            {
                dist = -dist;
            }

            if (dist >= ITU_DRAG_DISTANCE)
            {
                widget->flags |= ITU_DRAGGING;
                ituScene->dragged     = widget;
                coverFlow->touchCount = 0;
            }
        }
        else if ((widget->flags & ITU_DRAGGING) && (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                 (coverFlow->frame <= coverFlow->totalframe))
        { // for goose check
            coverFlow->frame++;

            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP)
            {
                // do nothing
            }
            else
            {
                if (coverFlow->inc > 0)
                {
                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                    {
                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                               coverFlow->focusIndex - 1, __LINE__);
                    }

                    coverFlow->focusIndex--;
                }
                else
                {
                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                    {
                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                               coverFlow->focusIndex + 1, __LINE__);
                    }

                    coverFlow->focusIndex++;
                }

                ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);

                if (coverFlow->inc > 0)
                {
                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                    {
                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                               coverFlow->focusIndex + 1, __LINE__);
                    }

                    coverFlow->focusIndex++;
                }
                else
                {
                    if (COVERFLOW_DEBUG_FOCUSINDEX)
                    {
                        (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                               coverFlow->focusIndex - 1, __LINE__);
                    }

                    coverFlow->focusIndex--;
                }

                ituCoverFlowFixFC(coverFlow);
                return true;
            }
        }
        else
        {
            /* do nothing */
        }

        // for goose check
        if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) &&
            (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)) && (widget->flags & ITU_UNDRAGGING))
        {
            ITUWidget * c1  = CoverFlowGetVisibleChild(coverFlow, 0);
            ITUWidget * c2  = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
            int         pos = (widget_size / 2) + (base_size / 2);

            if ((c1 != NULL) && (c2 != NULL))
            {
                if (coverFlow->inc < 0)
                {
                    if ((coverFlow->focusIndex >= (childCount - 1)) && ((c2->rect.x + c2->rect.width) == pos) &&
                        (coverFlow->frame == 0))
                    {
                        coverFlow->inc = 0;
                        widget->flags &= ~ITU_DRAGGING;
                        widget->flags &= ~ITU_UNDRAGGING;
                    }
                }
                else if (coverFlow->inc > 0)
                {
                    if ((coverFlow->focusIndex <= 0) && ((c1->rect.x + c1->rect.width) == pos) && (coverFlow->frame == 0))
                    {
                        coverFlow->inc = 0;
                        widget->flags &= ~ITU_DRAGGING;
                        widget->flags &= ~ITU_UNDRAGGING;
                    }
                }
                else
                {
                    /* do nothing */
                }
            }
        }

        if (coverFlow->inc)
        {
            result = true;

            // if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
            //  (void)printf("[Coverflow][Sliding][frame %d/%d]\n", coverFlow->frame, coverFlow->totalframe);
            // else
            //  (void)printf("[Coverflow][Undragging][frame %d/%d]\n", coverFlow->frame, coverFlow->totalframe);

            // try to fix dual bounce
            if ((widget->flags & ITU_BOUNCING) &&
                ((coverFlow->coverFlowFlags & ITU_BOUNCE_1) && (coverFlow->coverFlowFlags & ITU_BOUNCE_2)))
            {
                ITUWidget * child_1 = CoverFlowGetVisibleChild(coverFlow, 0);
                if (child_1 != NULL)
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        if (child_1->rect.y < (0 - base_size))
                        {
                            coverFlow->coverFlowFlags &= ~ITU_BOUNCE_1;
                        }
                        else
                        {
                            coverFlow->coverFlowFlags &= ~ITU_BOUNCE_2;
                        }
                    }
                    else
                    {
                        if (child_1->rect.x < (0 - base_size))
                        {
                            coverFlow->coverFlowFlags &= ~ITU_BOUNCE_1;
                        }
                        else
                        {
                            coverFlow->coverFlowFlags &= ~ITU_BOUNCE_2;
                        }
                    }
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                {
                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE1) ||
                        (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE2))
                    {
                        coverFlow->frame = coverFlow->totalframe;
                    }
                    else
                    {
                        int         move_step = 0;
                        ITUWidget * child_1   = CoverFlowGetVisibleChild(coverFlow, 0);
                        ITUWidget * child_2   = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                        float       step = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
                        // step = step * (float)M_PI / 2;
                        // step = sinf(step);
                        // move_step = (int)(coverFlow->inc * step);
                        move_step        = (int)((CoverFlowSaveDivF((float)coverFlow->inc, (float)COVERFLOW_FACTOR)) *
                                          (float)coverFlow->slide_diff / 40.0);

                        if (step <= COVERFLOW_PROCESS_STAGE1)
                        {
                            move_step /= 3;
                        }
                        else if (step <= COVERFLOW_PROCESS_STAGE2)
                        {
                            move_step /= 6;
                        }
                        else
                        {
                            move_step /= 12;
                        }

                        if ((child_1 != NULL) && (child_2 != NULL))
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                int i = 0;
                                if ((coverFlow->bounceRatio > 0) && !(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE))
                                {
                                    int tor = CoverFlowSaveDiv(child_1->rect.height, coverFlow->bounceRatio);

                                    if ((child_1->rect.y + move_step) > tor)
                                    {
                                        move_step = tor - child_1->rect.y;
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE;
                                    }
                                    else if ((child_2->rect.y + child_2->rect.height + move_step) <
                                        (widget->rect.height - tor))
                                    {
                                        move_step = widget->rect.height - tor - (child_2->rect.y + child_2->rect.height);
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                                else
                                {
                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE)
                                    {
                                        int tor_step =
                                            (CoverFlowSaveDiv(child_1->rect.height, coverFlow->bounceRatio) / 10) + 1;
                                        move_step = (coverFlow->inc > 0) ? (-1 * tor_step) : (1 * tor_step);

                                        if (((child_1->rect.y + move_step) <= 0) && (child_1->rect.y > 0))
                                        {
                                            move_step = 0 - child_1->rect.y;
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                        }
                                        else if (((child_2->rect.y + child_2->rect.height + move_step) >=
                                            widget->rect.height) &&
                                            (child_2->rect.y < widget->rect.height))
                                        {
                                            move_step = widget->rect.height - (child_2->rect.y + child_2->rect.height);
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                    else
                                    {
                                        if ((child_1->rect.y + move_step) > 0)
                                        {
                                            move_step = 0 - child_1->rect.y;
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                        }
                                        else if ((child_2->rect.y + child_2->rect.height + move_step) < widget->rect.height)
                                        {
                                            move_step = widget->rect.height - (child_2->rect.y + child_2->rect.height);
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                }

                                for (i = 0; i < childCount; i++)
                                {
                                    ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);

                                    if (child != NULL)
                                    {
                                        int         fy = child->rect.y;

                                        if (coverFlow->frame > 0)
                                        {
                                            fy += move_step;
                                        }

                                        ituCoverFlowSetXY(coverFlow, i, fy, __LINE__);
                                        // (void)printf("[fy] %d [step] %.3f\n", fy, step);
                                    }
                                }

                                // (void)printf("[Frame %d]move_step %d\n", coverFlow->frame, move_step);
                            }
                            else
                            { // For Horizontal
                                int i = 0;
                                if ((coverFlow->bounceRatio > 0) && !(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE))
                                {
                                    int tor = CoverFlowSaveDiv(child_1->rect.width, coverFlow->bounceRatio);

                                    if ((child_1->rect.x + move_step) > tor)
                                    {
                                        move_step = tor - child_1->rect.x;
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE;
                                    }
                                    else if ((child_2->rect.x + child_2->rect.width + move_step) <
                                        (widget->rect.width - tor))
                                    {
                                        move_step = widget->rect.width - tor - (child_2->rect.x + child_2->rect.width);
                                        coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                                else
                                {
                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE)
                                    {
                                        int tor_step =
                                            (CoverFlowSaveDiv(child_1->rect.width, coverFlow->bounceRatio) / 10) + 1;
                                        move_step = (coverFlow->inc > 0) ? (-1 * tor_step) : (1 * tor_step);

                                        if (((child_1->rect.x + move_step) < 0) && (child_1->rect.x > 0))
                                        {
                                            move_step = 0 - child_1->rect.x;
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                        }
                                        else if (((child_2->rect.x + child_2->rect.width + move_step) >
                                            widget->rect.width) &&
                                            (child_2->rect.x < widget->rect.width))
                                        {
                                            move_step = widget->rect.width - (child_2->rect.x + child_2->rect.width);
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                    else
                                    {
                                        if ((child_1->rect.x + move_step) > 0)
                                        {
                                            move_step = 0 - child_1->rect.x;
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE1;
                                        }
                                        else if ((child_2->rect.x + child_2->rect.width + move_step) < widget->rect.width)
                                        {
                                            move_step = widget->rect.width - (child_2->rect.x + child_2->rect.width);
                                            coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYBOUNCE2;
                                        }
                                        else
                                        {
                                            /* do nothing */
                                        }
                                    }
                                }

                                for (i = 0; i < childCount; i++)
                                {
                                    ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                    if (child != NULL)
                                    {
                                        int         fx = child->rect.x;

                                        if (coverFlow->frame > 0)
                                        {
                                            fx += move_step;
                                        }

                                        ituCoverFlowSetXY(coverFlow, i, fx, __LINE__);
                                        // (void)printf("[fx] %d [step] %.3f\n", fx, step);
                                    }
                                }

                                // (void)printf("[Frame %d]move_step %d\n", coverFlow->frame, move_step);
                            }
                        }
                    }
                }
                else
                {
                    coverFlow->frame = coverFlow->totalframe;
                }
            }
            else if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
            { // <<< ITU_EVENT_TIMER >>>
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int   index, count2;
                    float step      = 0.0;
                    int   local_inc = coverFlow->inc;
                    int   local_fi  = coverFlow->focusIndex;

                    while (local_inc > (base_size / 2))
                    {
                        local_inc -= base_size;
                    }
                    while (local_inc < (-1 * base_size / 2))
                    {
                        local_inc += base_size;
                    }

                    // bbbbb
                    //  cubic ease out: y = (x - 1)^3 + 1
                    // step = step - 1;
                    // step = step * step * step + 1;

                    // workaround for wrong left-side display with hide child
                    count2 = childCount / 2 + 1 - ((coverFlow->inc > 0) ? (1) : (0));
                    index  = local_fi;

                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                    {
                        int fy    = widget->rect.height / 2 - base_size / 2;
                        int i     = 0;
                        local_inc = coverFlow->inc;
                        step      = 1.0 - CoverFlowAniStepCal(coverFlow);
                        // from focus to right side all
                        for (i = 0; i < count2; ++i)
                        {
                            int         ci       = ((index > (childCount - 1)) ? (0) : (index));
                            int         fix      = (int)(local_inc * step);
                            int         local_fy = fy + i * base_size;
                            ITUWidget * child    = CoverFlowGetVisibleChild(coverFlow, ci);

                            // try to fix the next child is wrong position under cycle mode
                            if (child)
                            {
                                int         check_item = (((ci + 1) > (childCount - 1)) ? (0) : (ci + 1));
                                ITUWidget * child_next = CoverFlowGetVisibleChild(coverFlow, check_item);

                                if (child_next)
                                {
                                    if ((child_next->rect.y < child->rect.y) && ((i < (count2 - 1))))
                                    {
                                        ituCoverFlowSetXY(coverFlow, check_item, child->rect.y + base_size, __LINE__);
                                    }
                                }
                            }
                            if ((i == 0) && (coverFlow->frame == 0))
                            {
                                if (((local_fy + fix) < child->rect.y) && (local_inc > 0))
                                {
                                    int calframe = 0;
                                    int calfix   = fix;
                                    while ((local_fy + calfix) < child->rect.y)
                                    {
                                        calframe++;
                                        if ((coverFlow->frame + calframe) >= coverFlow->totalframe)
                                        {
                                            break;
                                        }
                                        calfix =
                                            (int)(local_inc * (CoverFlowSaveDivF((float)(coverFlow->frame + calframe),
                                                                                 (float)coverFlow->totalframe)));
                                        (void)printf("calframe to %d [locay_fy %d calfix %d child_y %d]\n", calframe,
                                               local_fy, calfix, child->rect.y);
                                    }
                                    coverFlow->frame += calframe;
                                    fix = calfix;
                                }
                                else if (((local_fy + fix) > child->rect.y) && (local_inc < 0))
                                {
                                    int calframe = 0;
                                    int calfix   = fix;
                                    while ((local_fy + calfix) > child->rect.y)
                                    {
                                        calframe++;
                                        if ((coverFlow->frame + calframe) >= coverFlow->totalframe)
                                        {
                                            break;
                                        }
                                        calfix =
                                            (int)(local_inc * (CoverFlowSaveDivF((float)(coverFlow->frame + calframe),
                                                                                 (float)coverFlow->totalframe)));
                                        (void)printf("calframe to %d [locay_fy %d calfix %d child_y %d]\n", calframe,
                                               local_fy, calfix, child->rect.y);
                                    }
                                    coverFlow->frame += calframe;
                                    fix = calfix;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }

                            local_fy += fix;

                            ituCoverFlowSetXY(coverFlow, ci, local_fy, __LINE__);
                            index = ci + 1;
                        }

                        count2 = childCount - count2;
                        index  = local_fi - 1;
                        fy     = fy - base_size;
                        // from left side all to focus -1
                        for (i = 0; i < count2; ++i)
                        {
                            int ci       = ((index < 0) ? (childCount - 1) : (index));
                            int fix      = (int)(local_inc * step);
                            int local_fy = fy;
                            local_fy -= i * base_size;
                            local_fy += fix;

                            ituCoverFlowSetXY(coverFlow, ci, local_fy, __LINE__);
                            index = ci - 1;

                            // fix for user use bad design (item count less than display size + 2)
                            if ((coverFlow->inc > 0) && (count2 <= 1))
                            {
                                int               prev2 = ((index < 0) ? (childCount - 1) : (index));
                                const ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, prev2);
                                if (child->rect.y > widget->rect.height)
                                {
                                    count2++;
                                }
                            }
                        }
                    }
                    else
                    {
                        int local_frame = coverFlow->frame;
                        int i           = 0;
                        step            = 1.0 - CoverFlowAniStepCal(coverFlow);

                        if (coverFlow->dragFlag < 1)
                        {
                            local_inc = ((coverFlow->inc > 0) ? (base_size) : (-base_size));
                            step      = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
                        }
                        else
                        {
                            local_inc = coverFlow->inc * -1;
                        }

                        // (void)printf("frame %d fix %d\n", coverFlow->frame, (int)(local_inc * step));

                        for (i = 0; i < count2; ++i)
                        {
                            int         ci           = ((index > (childCount - 1)) ? (0) : (index));
                            int         fix          = (int)(local_inc * step);
                            ITUWidget * child        = CoverFlowGetVisibleChild(coverFlow, ci);
                            bool        layout_check = true;
                            int         fy           = widget->rect.height / 2 - base_size / 2;

                            // childCount = 2, when right way(inc > 0) the right child will be at left side
                            if ((coverFlow->inc > 0) && (childCount == 2))
                            {
                                fy -= i * base_size;
                            }
                            else
                            {
                                fy += i * base_size;
                            }
                            // debugging layout_check (something wrong)
                            // if (((fx\y > 0) && (child->rect.y > 0)) || ((fy < 0) && (child->rect.y < 0)))
                            //  layout_check = true;

                            if (((fy + fix) > child->rect.y) && (local_inc < 0) && layout_check &&
                                (coverFlow->dragFlag < 1))
                            {
                                while (((fy + fix) > child->rect.y) && (local_frame < coverFlow->totalframe))
                                {
                                    local_frame++;
                                    step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                                    fix  = (int)(local_inc * step);
                                }
                                step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                            }
                            else if (((fy + fix) < child->rect.y) && (local_inc > 0) && layout_check &&
                                     (coverFlow->dragFlag < 1))
                            {
                                while (((fy + fix) < child->rect.y) && (local_frame < coverFlow->totalframe))
                                {
                                    local_frame++;
                                    step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                                    fix  = (int)(local_inc * step);
                                }
                                step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                            }
                            else
                            {
                                /* do nothing */
                            }

                            fy += fix;
                            ituCoverFlowSetXY(coverFlow, ci, fy, __LINE__);
                            index = ci + 1;
                        }

                        count2 = childCount - count2;
                        index  = local_fi - 1;
                        for (i = 0; i < count2; ++i)
                        {
                            int ci  = ((index < 0) ? (childCount - 1) : (index));
                            int fix = (int)(local_inc * step);
                            int fy  = widget->rect.height / 2 - base_size / 2;
                            fy -= base_size;
                            fy -= i * base_size;
                            fy += fix;
                            ituCoverFlowSetXY(coverFlow, ci, fy, __LINE__);
                            index = ci - 1;

                            // fix for user use bad design (item count less than display size + 2)
                            if ((coverFlow->inc > 0) && (count2 <= 1))
                            {
                                int               prev2 = ((index < 0) ? (childCount - 1) : (index));
                                const ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, prev2);
                                if (child->rect.y > widget->rect.height)
                                {
                                    count2++;
                                }
                            }
                        }
                    }

                    // for goose do more one frame at last frame (slow down)
                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                        (coverFlow->frame == (coverFlow->totalframe - 1)) &&
                        (childCount <= COVERFLOW_SMOOTH_LASTFRAME_COUNT))
                    {
                        if (coverFlow->org_totalframe == coverFlow->totalframe)
                        {
                            coverFlow->totalframe *= 2;
                            coverFlow->frame = coverFlow->totalframe - 2;
                        }
                    }

                    result = true;
                }
                else if (widget->flags & ITU_BOUNCING)
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Vertical / Non-Cycle >>>
                    int   i    = 0;
                    float step = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
                    step       = step - 1;
                    step       = step * step * step + 1;

                    // (void)printf("step=%f\n", step);

                    for (i = 0; i < childCount; ++i)
                    {
                        // int fy = widget->rect.height / 2 - base_size / 2;
                        // fy -= base_size * coverFlow->focusIndex;
                        // fy += i * base_size;

                        int fy;
                        if (coverFlow->coverFlowFlags & ITU_BOUNCE_1)
                        {
                            fy = 0;
                        }
                        else if (coverFlow->coverFlowFlags & ITU_BOUNCE_2)
                        {
                            fy = widget_size - childCount * base_size;
                        }
                        else
                        {
                            if (coverFlow->focusIndex == 0)
                            {
                                fy = 0;
                            }
                            else
                            {
                                fy = widget_size - (coverFlow->focusIndex + 1) * base_size;
                            }
                        }

                        fy += i * base_size;

                        fy += (int)(coverFlow->inc * step - coverFlow->overlapsize);
                        ituCoverFlowSetXY(coverFlow, i, fy, __LINE__);
                        // if (i == 0)
                        //  (void)printf("fy %d\n", fy);
                    }
                }
                else
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Vertical / Non-Cycle >>>
                    bool wrong_pos_check = true;
                    // working next
                    // if (coverFlow->boundaryAlign == 0)
                    //  wrong_pos_check = false;

                    while (wrong_pos_check)
                    {
                        int         local_focusindex   = coverFlow->focusIndex;
                        int         i                  = 0;
                        int         cal_init_pos       = 0;
                        float       step               = 0.0;

                        wrong_pos_check = false;
                        step            = CoverFlowAniStepCal(coverFlow);

                        /*if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                        {
                        step = (float)(coverFlow->totalframe - coverFlow->frame) / (float)coverFlow->totalframe;
                        }
                        else
                        {
                        step = (float)coverFlow->frame / (float)coverFlow->totalframe;
                        }
                        step = step - 1;
                        step = step * step * step + 1;*/

                        for (i = 0; i < childCount; i++)
                        {
                            int         fix = 0;
                            int         child_rect_pos;
                            int         childlast_rect_pos;
                            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                            if (child == NULL)
                            {
                                continue;
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                child_rect_pos     = child->rect.y;
                                childlast_rect_pos = childLast->rect.y;
                            }
                            else
                            {
                                child_rect_pos     = child->rect.x;
                                childlast_rect_pos = childLast->rect.x;
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                            {
                                fix = (base_size - (int)((float)(base_size)*step));
                                fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                            }
                            else // Undragging mode
                            {
                                if (abs(coverFlow->inc) >= (base_size / 2))
                                {
                                    fix = (base_size - (int)((float)(base_size)*step));

                                    if (coverFlow->inc < 0)
                                    {
                                        if (fix > 0)
                                        {
                                            fix *= -1;
                                        }
                                    }
                                }
                                else
                                {
                                    fix = (base_size - (int)((float)(base_size)*step));
                                    fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                                }
                            }

                            if (coverFlow->boundaryAlign)
                            {
                                int temp_local_cal = 0;
                                if (i == 0)
                                {
                                    bool align_start_side, check_position = true;

                                    if ((widget_size - (childlast_rect_pos + base_size)) >= 0)
                                    {
                                        align_start_side = false;
                                    }
                                    else
                                    {
                                        align_start_side = true;
                                    }

                                    if (align_start_side)
                                    {
                                        while (check_position)
                                        {
                                            if (abs((0 - (base_size * local_focusindex)) - child_rect_pos) <= base_size)
                                            {
                                                check_position = false;
                                            }
                                            else
                                            {
                                                local_focusindex += ((coverFlow->inc > 0) ? (-1) : (1));

                                                if (local_focusindex < 0)
                                                {
                                                    local_focusindex = 0;
                                                    check_position   = false;
                                                }
                                                else if (local_focusindex > (childCount - 1))
                                                {
                                                    local_focusindex = childCount - 1;
                                                    check_position   = false;
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }
                                            }
                                        }

                                        cal_init_pos = 0 - (base_size * local_focusindex);

                                        // (void)printf("align [start] side mode [frame %d] [cal init pos %d]\n",
                                        // coverFlow->frame, cal_init_pos);
                                    }
                                    else
                                    {
                                        int focusIndexMax =
                                            (childCount - 1) - ((CoverFlowSaveDiv(widget_size, base_size) / 2) + 1);
                                        int align_fix_dist = 0;

                                        while (check_position)
                                        {
                                            if (abs((widget_size - (childCount * base_size)) + align_fix_dist -
                                                    child_rect_pos) <= base_size)
                                            {
                                                check_position = false;
                                            }
                                            else
                                            {
                                                local_focusindex += ((coverFlow->inc > 0) ? (-1) : (1));

                                                if (local_focusindex < 0)
                                                {
                                                    align_fix_dist   = 0;
                                                    local_focusindex = 0;
                                                    check_position   = false;
                                                }
                                                else if (local_focusindex > (childCount - 1))
                                                {
                                                    align_fix_dist   = 0;
                                                    local_focusindex = childCount - 1;
                                                    check_position   = false;
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }

                                                if (local_focusindex < focusIndexMax)
                                                {
                                                    align_fix_dist = (focusIndexMax - local_focusindex) * base_size;
                                                }
                                            }
                                        }
                                        cal_init_pos = widget_size - (childCount * base_size) + align_fix_dist;
                                        // (void)printf("align [end] side mode [frame %d]\n", coverFlow->frame);
                                    }
                                }
                                else
                                {
                                    cal_init_pos += base_size;
                                }

                                temp_local_cal = (widget_size - (childlast_rect_pos + base_size + fix));

                                if ((coverFlow->bounceFlag == ITU_BOUNCE_2) || (widget_size == base_size))
                                {
                                    temp_local_cal = 0;
                                }

                                if ((temp_local_cal > CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) &&
                                    ((childlast_rect_pos + base_size) <= widget_size))
                                {
                                    i = childCount;

                                    if (coverFlow->frame < coverFlow->totalframe)
                                    {
                                        temp_local_cal = ((childlast_rect_pos + base_size) - widget_size);
                                        if (temp_local_cal > (base_size / 5))
                                        {
                                            for (i = 0; i < childCount; i++)
                                            {
                                                child = CoverFlowGetVisibleChild(coverFlow, i);
                                                if (child != NULL)
                                                {
                                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                    {
                                                        child_rect_pos = child->rect.y;
                                                    }
                                                    else
                                                    {
                                                        child_rect_pos = child->rect.x;
                                                    }

                                                    ituCoverFlowSetXY(coverFlow, i, child_rect_pos - coverFlow->overlapsize - (temp_local_cal / 5), __LINE__);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            for (i = 0; i < childCount; i++)
                                            {
                                                child = CoverFlowGetVisibleChild(coverFlow, i);
                                                if (child != NULL)
                                                {
                                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                    {
                                                        child_rect_pos = child->rect.y;
                                                    }
                                                    else
                                                    {
                                                        child_rect_pos = child->rect.x;
                                                    }

                                                    ituCoverFlowSetXY(coverFlow, i, child_rect_pos - coverFlow->overlapsize - temp_local_cal, __LINE__);
                                                }
                                            }
                                        }
                                    }

                                    continue;
                                }

                                if (coverFlow->frame <= coverFlow->totalframe)
                                {
                                    bool local_debug_print = false;

                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) > child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) < child_rect_pos)))
                                        {
                                            /// check the first and last child position is outside or not
                                            int base_bounce =
                                                ((coverFlow->bounceRatio <= 0) ? (base_size)
                                                                               : (coverFlow->bounceRatio));
                                            bool check_outside = false;

                                            if (i == 0)
                                            {
                                                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                {
                                                    if (child->rect.y > CoverFlowSaveDiv(base_size, base_bounce))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if (child->rect.x > CoverFlowSaveDiv(base_size, base_bounce))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                            }
                                            else if (i == (childCount - 1))
                                            {
                                                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                {
                                                    if ((child->rect.y + base_size) <
                                                        (widget_size - CoverFlowSaveDiv(base_size, base_bounce)))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if ((child->rect.x + base_size) <
                                                        (widget_size - CoverFlowSaveDiv(base_size, base_bounce)))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }

                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            if (!check_outside)
                                            {
                                                ituCoverFlowSetXY(coverFlow, i,
                                                                  cal_init_pos - coverFlow->overlapsize + fix,
                                                                  __LINE__);
                                            }
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                    else // Undragging mode
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) < child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) > child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else // default center align mode
                            {
                                cal_init_pos = save_div(widget_size, -base_size) -
                                               ((coverFlow->focusIndex - i) *
                                                base_size); //((widget_size - base_size) / 2) - ((coverFlow->focusIndex
                                                            //- i) * base_size);

                                if (coverFlow->frame <= coverFlow->totalframe)
                                {
                                    bool local_debug_print = false;

                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) > child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) < child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                    else // Undragging mode
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) < child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) > child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }

                                            ////////// try to save more frame when wrong position 2018/4/12
                                            if (i == 0)
                                            {
                                                int  savecount = 0;
                                                bool recheck   = true;
                                                while (recheck)
                                                {
                                                    coverFlow->frame++;
                                                    step = CoverFlowAniStepCal(coverFlow);

                                                    if (coverFlow->frame < coverFlow->totalframe)
                                                    {
                                                        if (abs(coverFlow->inc) >= (base_size / 2))
                                                        {
                                                            fix = (base_size - (int)((float)(base_size)*step));

                                                            if (coverFlow->inc < 0)
                                                            {
                                                                if (fix > 0)
                                                                {
                                                                    fix *= -1;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            fix = (base_size - (int)((float)(base_size)*step));
                                                            fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                                                        }

                                                        if (((coverFlow->inc > 0) &&
                                                             ((cal_init_pos + fix) < child_rect_pos)) ||
                                                            ((coverFlow->inc < 0) &&
                                                             ((cal_init_pos + fix) > child_rect_pos)))
                                                        {
                                                            coverFlow->frame--;
                                                            i       = childCount;
                                                            recheck = false;
                                                        }
                                                        else
                                                        {
                                                            savecount++;
                                                            // (void)printf("[Coverflow][Undragging][Position sync save frame
                                                            // %d]\n", savecount);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        recheck = false;
                                                    }
                                                }
                                            }
                                            /////////////////////////////////
                                        }
                                    }
                                }
                            }

                            if (coverFlow->frame <= coverFlow->totalframe)
                            {
                                // for goose do more one frame at last frame (slow down)
                                if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                    (coverFlow->frame == (coverFlow->totalframe - 1)) &&
                                    (childCount <= COVERFLOW_SMOOTH_LASTFRAME_COUNT))
                                {
                                    if (coverFlow->org_totalframe == coverFlow->totalframe)
                                    {
                                        coverFlow->totalframe *= 2;
                                        coverFlow->frame = coverFlow->totalframe - 2;
                                    }
                                }
                            }
                        }

                        if (coverFlow->focusIndex < 0)
                        {
                            coverFlow->focusIndex = 0;
                        }
                        else if (coverFlow->focusIndex > (childCount - 1))
                        {
                            coverFlow->focusIndex = childCount - 1;
                        }
                        else
                        {
                            /* do nothing */
                        }

                        
                    }
                }
            }
            else
            {
                // <<< ITU_EVENT_TIMER >>>
                // <<< Horizontal >>>
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int   index, count2;
                    float step      = 0.0;
                    int   local_inc = coverFlow->inc;
                    int   local_fi  = coverFlow->focusIndex;

                    while (local_inc > (base_size / 2))
                    {
                        local_inc -= base_size;
                    }
                    while (local_inc < (-1 * base_size / 2))
                    {
                        local_inc += base_size;
                    }

                    // bbbbb
                    //  cubic ease out: y = (x - 1)^3 + 1
                    // step = step - 1;
                    // step = step * step * step + 1;

                    // workaround for wrong left-side display with hide child
                    count2 = childCount / 2 + 1 - ((coverFlow->inc > 0) ? (1) : (0));

                    index  = local_fi;

                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                    {
                        int fx    = widget->rect.width / 2 - base_size / 2;
                        int i     = 0;
                        local_inc = coverFlow->inc;
                        step      = 1.0 - CoverFlowAniStepCal(coverFlow);

                        // from focus to right side all
                        for (i = 0; i < count2; ++i)
                        {
                            int         ci       = ((index > (childCount - 1)) ? (0) : (index));
                            int         fix      = (int)(local_inc * step);
                            int         local_fx = fx + i * base_size;
                            ITUWidget * child    = CoverFlowGetVisibleChild(coverFlow, ci);

                            // try to fix the next child is wrong position under cycle mode
                            if (child)
                            {
                                int         check_item = (((ci + 1) > (childCount - 1)) ? (0) : (ci + 1));
                                ITUWidget * child_next = CoverFlowGetVisibleChild(coverFlow, check_item);

                                if (child_next)
                                {
                                    if ((child_next->rect.x < child->rect.x) && (i < (count2 - 1)))
                                    {
                                        ituCoverFlowSetXY(coverFlow, check_item, child->rect.x + base_size, __LINE__);
                                    }
                                }
                            }

                            if ((i == 0) && (coverFlow->frame == 0))
                            {
                                if (((local_fx + fix) < child->rect.x) && (local_inc > 0))
                                {
                                    int calframe = 0;
                                    int calfix   = fix;
                                    while ((local_fx + calfix) < child->rect.x)
                                    {
                                        calframe++;
                                        if ((coverFlow->frame + calframe) >= coverFlow->totalframe)
                                        {
                                            break;
                                        }
                                        calfix =
                                            (int)(local_inc * (CoverFlowSaveDivF((float)(coverFlow->frame + calframe),
                                                                                 (float)coverFlow->totalframe)));
                                        (void)printf("calframe to %d [locay_fx %d calfix %d child_x %d]\n", calframe,
                                               local_fx, calfix, child->rect.x);
                                    }
                                    coverFlow->frame += calframe;
                                    fix = calfix;
                                }
                                else if (((local_fx + fix) > child->rect.x) && (local_inc < 0))
                                {
                                    int calframe = 0;
                                    int calfix   = fix;
                                    while ((local_fx + calfix) > child->rect.x)
                                    {
                                        calframe++;
                                        if ((coverFlow->frame + calframe) >= coverFlow->totalframe)
                                        {
                                            break;
                                        }
                                        calfix =
                                            (int)(local_inc * (CoverFlowSaveDivF((float)(coverFlow->frame + calframe),
                                                                                 (float)coverFlow->totalframe)));
                                        (void)printf("calframe to %d [locay_fx %d calfix %d child_x %d]\n", calframe,
                                               local_fx, calfix, child->rect.x);
                                    }
                                    coverFlow->frame += calframe;
                                    fix = calfix;
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }

                            local_fx += fix;

                            ituCoverFlowSetXY(coverFlow, ci, local_fx, __LINE__);
                            index = ci + 1;
                        }

                        count2 = childCount - count2;
                        index  = local_fi - 1;
                        fx     = fx - base_size;
                        // from left side all to focus -1
                        for (i = 0; i < count2; ++i)
                        {
                            int ci       = ((index < 0) ? (childCount - 1) : (index));
                            int fix      = (int)(local_inc * step);
                            int local_fx = fx;
                            local_fx -= i * base_size;
                            local_fx += fix;

                            ituCoverFlowSetXY(coverFlow, ci, local_fx, __LINE__);
                            index = ci - 1;

                            // fix for user use bad design (item count less than display size + 2)
                            if ((coverFlow->inc > 0) && (count2 <= 1))
                            {
                                int               prev2 = ((index < 0) ? (childCount - 1) : (index));
                                const ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, prev2);
                                if (child->rect.x > widget->rect.width)
                                {
                                    count2++;
                                }
                            }
                        }
                    }
                    else
                    {
                        int local_frame = coverFlow->frame;
                        int i           = 0;
						//step = 1.0 - CoverFlowAniStepCal(coverFlow); ongoing
						step = CoverFlowAniStepCal(coverFlow);
						step = step - 1;
						step = step * step * step + 1;

                        if (coverFlow->dragFlag < 1)
                        {
                            local_inc = ((coverFlow->inc > 0) ? (base_size) : (-base_size));
                            step      = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
                        }
                        else
                        {
                            local_inc = coverFlow->inc * -1;
                        }

                        // (void)printf("frame %d fix %d\n", coverFlow->frame, (int)(local_inc * step));

                        for (i = 0; i < count2; ++i)
                        {
                            int         ci           = ((index > (childCount - 1)) ? (0) : (index));
                            int         fix          = (int)(local_inc * step);
                            ITUWidget * child        = CoverFlowGetVisibleChild(coverFlow, ci);
                            bool        layout_check = true;
                            int         fx           = widget->rect.width / 2 - base_size / 2;

                            // childCount = 2, when right way(inc > 0) the right child will be at left side
                            if ((coverFlow->inc > 0) && (childCount == 2))
                            {
                                fx -= i * base_size;
                            }
                            else
                            {
                                fx += i * base_size;
                            }
                            // debugging layout_check (something wrong)
                            // if (((fx > 0) && (child->rect.x > 0)) || ((fx < 0) && (child->rect.x < 0)))
                            //  layout_check = true;

                            if (((fx + fix) > child->rect.x) && (local_inc < 0) && layout_check &&
                                (coverFlow->dragFlag < 1))
                            {
                                while (((fx + fix) > child->rect.x) && (local_frame < coverFlow->totalframe))
                                {
                                    local_frame++;
                                    step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                                    fix  = (int)(local_inc * step);
                                }
                                step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                            }
                            else if (((fx + fix) < child->rect.x) && (local_inc > 0) && layout_check &&
                                     (coverFlow->dragFlag < 1))
                            {
								while (0)//checking case (((fx + fix) < child->rect.x) && (local_frame < coverFlow->totalframe))
                                {
                                    local_frame++;
                                    step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                                    fix  = (int)(local_inc * step);
                                }
                                step = CoverFlowSaveDivF((float)local_frame, (float)coverFlow->totalframe);
                            }
                            else
                            {
                                /* do nothing */
                            }

                            fx += fix;
                            ituCoverFlowSetXY(coverFlow, ci, fx, __LINE__);
							//(void)printf("[%d] [%d] [%d]\n", ci, fx, coverFlow->frame);
                            index = ci + 1;
                        }

                        count2 = childCount - count2;
                        index  = local_fi - 1;
						//step = step - 1;
						//step = step * step * step + 1;
                        for (i = 0; i < count2; ++i)
                        {
                            int ci  = ((index < 0) ? (childCount - 1) : (index));
                            int fix = (int)(local_inc * step);
                            int fx  = widget->rect.width / 2 - base_size / 2;
                            fx -= base_size;
                            fx -= i * base_size;
                            fx += fix;
                            ituCoverFlowSetXY(coverFlow, ci, fx, __LINE__);
                            index = ci - 1;
                            // fix for user use bad design (item count less than display size + 2)
                            if ((coverFlow->inc > 0) && (count2 <= 1))
                            {
                                int               prev2 = ((index < 0) ? (childCount - 1) : (index));
                                const ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, prev2);
                                if (child->rect.x > widget->rect.width)
                                {
                                    count2++;
                                }
                            }
                        }
                    }

                    // for goose do more one frame at last frame (slow down)
                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                        (coverFlow->frame == (coverFlow->totalframe - 1)) &&
                        (childCount <= COVERFLOW_SMOOTH_LASTFRAME_COUNT))
                    {
                        if (coverFlow->org_totalframe == coverFlow->totalframe)
                        {
                            coverFlow->totalframe *= 2;
                            coverFlow->frame = coverFlow->totalframe - 2;
                        }
                    }

                    result = true;
                }
                else if (widget->flags & ITU_BOUNCING)
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Horizontal / Non-Cycle >>>
                    int   i    = 0;
                    float step = CoverFlowSaveDivF((float)coverFlow->frame, (float)coverFlow->totalframe);
                    step       = step - 1;
                    step       = step * step * step + 1;

                    // (void)printf("frame=%d step=%f\n", coverFlow->frame, step);
                    for (i = 0; i < childCount; ++i)
                    {
                        // int fx = widget->rect.width / 2 - base_size / 2;
                        // fx -= base_size * coverFlow->focusIndex;
                        int fx;
                        if (coverFlow->coverFlowFlags & ITU_BOUNCE_1)
                        {
                            fx = 0;
                        }
                        else if (coverFlow->coverFlowFlags & ITU_BOUNCE_2)
                        {
                            fx = widget_size - (childCount * base_size);
                        }
                        else
                        {
                            if (coverFlow->focusIndex == 0)
                            {
                                fx = 0;
                            }
                            else
                            {
                                fx = widget_size - (coverFlow->focusIndex + 1) * base_size;
                            }
                        }

                        fx += i * base_size;
                        fx += (int)(coverFlow->inc * step);

                        ituCoverFlowSetXY(coverFlow, i, fx - coverFlow->overlapsize, __LINE__);
                    }
                }
                else
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Horizontal / Non-Cycle >>>
                    bool wrong_pos_check = true;

                    while (wrong_pos_check)
                    {
                        int         local_focusindex   = coverFlow->focusIndex;
                        int         i                  = 0;
                        int         cal_init_pos       = 0;
                        float       step               = 0.0;
                        ITUWidget * childlast          = CoverFlowGetVisibleChild(coverFlow, childCount - 1);

                        wrong_pos_check = false;
                        step            = CoverFlowAniStepCal(coverFlow);

                        for (i = 0; i < childCount; i++)
                        {
                            int         fix = 0;
                            int         child_rect_pos;
                            int         childlast_rect_pos;
                            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                            if (child == NULL)
                            {
                                continue;
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                child_rect_pos     = child->rect.y;
                                childlast_rect_pos = childlast->rect.y;
                            }
                            else
                            {
                                child_rect_pos     = child->rect.x;
                                childlast_rect_pos = childlast->rect.x;
                            }

                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                            {
                                fix = (base_size - (int)((float)(base_size)*step));
                                fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                            }
                            else // Undragging mode
                            {
                                if (abs(coverFlow->inc) >= (base_size / 2))
                                {
                                    fix = (base_size - (int)((float)(base_size)*step));

                                    if (coverFlow->inc < 0)
                                    {
                                        if (fix > 0)
                                        {
                                            fix *= -1;
                                        }
                                    }
                                }
                                else
                                {
                                    fix = (base_size - (int)((float)(base_size)*step));
                                    fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                                }
                            }

                            if (coverFlow->boundaryAlign)
                            {
                                int temp_local_cal = 0;
                                if (i == 0)
                                {
                                    bool align_start_side, check_position = true;

                                    if ((widget_size - (childlast_rect_pos + base_size)) >= 0)
                                    {
                                        align_start_side = false;
                                    }
                                    else
                                    {
                                        align_start_side = true;
                                    }

                                    if (align_start_side)
                                    {
                                        while (check_position)
                                        {
                                            if (abs((0 - (base_size * local_focusindex)) - child_rect_pos) <= base_size)
                                            {
                                                check_position = false;
                                            }
                                            else
                                            {
                                                local_focusindex += ((coverFlow->inc > 0) ? (-1) : (1));

                                                if (local_focusindex < 0)
                                                {
                                                    local_focusindex = 0;
                                                    check_position   = false;
                                                }
                                                else if (local_focusindex > (childCount - 1))
                                                {
                                                    local_focusindex = childCount - 1;
                                                    check_position   = false;
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }
                                            }
                                        }

                                        cal_init_pos = 0 - (base_size * local_focusindex);

                                        // (void)printf("align [start] side mode [frame %d] [cal init pos %d]\n",
                                        // coverFlow->frame, cal_init_pos);
                                    }
                                    else
                                    {
                                        int focusIndexMax =
                                            (childCount - 1) - ((CoverFlowSaveDiv(widget_size, base_size) / 2) + 1);
                                        int align_fix_dist = 0;

                                        while (check_position)
                                        {
                                            if (abs((widget_size - (childCount * base_size)) + align_fix_dist -
                                                    child_rect_pos) <= base_size)
                                            {
                                                check_position = false;
                                            }
                                            else
                                            {
                                                local_focusindex += ((coverFlow->inc > 0) ? (-1) : (1));

                                                if (local_focusindex < 0)
                                                {
                                                    align_fix_dist   = 0;
                                                    local_focusindex = 0;
                                                    check_position   = false;
                                                }
                                                else if (local_focusindex > (childCount - 1))
                                                {
                                                    align_fix_dist   = 0;
                                                    local_focusindex = childCount - 1;
                                                    check_position   = false;
                                                }
                                                else
                                                {
                                                    /* do nothing */
                                                }

                                                if (local_focusindex < focusIndexMax)
                                                {
                                                    align_fix_dist = (focusIndexMax - local_focusindex) * base_size;
                                                }
                                            }
                                        }
                                        cal_init_pos = widget_size - (childCount * base_size) + align_fix_dist;
                                        // (void)printf("align [end] side mode [frame %d]\n", coverFlow->frame);
                                    }
                                }
                                else
                                {
                                    cal_init_pos += base_size;
                                }

                                temp_local_cal = (widget_size - (childlast_rect_pos + base_size + fix));

                                if ((coverFlow->bounceFlag == ITU_BOUNCE_2) || (widget_size == base_size))
                                {
                                    temp_local_cal = 0;
                                }

                                if ((temp_local_cal > CoverFlowSaveDiv(base_size, coverFlow->bounceRatio)) &&
                                    ((childlast_rect_pos + base_size) <= widget_size))
                                {
                                    bool local_debug_print = false;
                                    i                      = childCount;

                                    if (coverFlow->frame < coverFlow->totalframe)
                                    {
                                        temp_local_cal = ((childlast_rect_pos + base_size) - widget_size);
                                        if (temp_local_cal > (base_size / 5))
                                        {
                                            for (i = 0; i < childCount; i++)
                                            {
                                                child = CoverFlowGetVisibleChild(coverFlow, i);
                                                if (child != NULL)
                                                {
                                                    ituCoverFlowSetXY(coverFlow, i,
                                                        child->rect.x - coverFlow->overlapsize -
                                                        (temp_local_cal / 5),
                                                        __LINE__);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            for (i = 0; i < childCount; i++)
                                            {
                                                child = CoverFlowGetVisibleChild(coverFlow, i);
                                                if (child != NULL)
                                                {
                                                    ituCoverFlowSetXY(
                                                        coverFlow, i,
                                                        child->rect.x - coverFlow->overlapsize - temp_local_cal, __LINE__);
                                                }
                                            }
                                        }
                                    }

                                    if (local_debug_print)
                                    {
                                        (void)printf("[Temp local cal][Continue][%d]\n", coverFlow->frame);
                                    }

                                    continue;
                                }

                                if (coverFlow->frame <= coverFlow->totalframe)
                                {
                                    bool local_debug_print = false;

                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) > child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) < child_rect_pos)))
                                        {
                                            /// check the first and last child position is outside or not
                                            int base_bounce =
                                                ((coverFlow->bounceRatio <= 0) ? (base_size)
                                                                               : (coverFlow->bounceRatio));
                                            bool check_outside = false;

                                            if (i == 0)
                                            {
                                                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                {
                                                    if (child->rect.y > CoverFlowSaveDiv(base_size, base_bounce))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if (child->rect.x > CoverFlowSaveDiv(base_size, base_bounce))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                            }
                                            else if (i == (childCount - 1))
                                            {
                                                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                                {
                                                    if ((child->rect.y + base_size) <
                                                        (widget_size - CoverFlowSaveDiv(base_size, base_bounce)))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if ((child->rect.x + base_size) <
                                                        (widget_size - CoverFlowSaveDiv(base_size, base_bounce)))
                                                    {
                                                        check_outside = true;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }

                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            if (!check_outside)
                                            {
                                                ituCoverFlowSetXY(coverFlow, i,
                                                                  cal_init_pos - coverFlow->overlapsize + fix,
                                                                  __LINE__);
                                            }
                                            else if (i == 0)
                                            {
                                                i = childCount;
                                                continue;
                                            }
                                            else
                                            {
                                                /* do nothing */
                                            }
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                    else // Undragging mode
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) < child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) > child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[+][D][%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[-][D][%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("==+==[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("==-==[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else // default center align mode
                            {
                                cal_init_pos = save_div(widget_size, -base_size) -
                                               ((coverFlow->focusIndex - i) *
                                                base_size); //((widget_size - base_size) / 2) - ((coverFlow->focusIndex
                                                            //- i) * base_size);

                                if (coverFlow->frame <= coverFlow->totalframe)
                                {
                                    bool local_debug_print = false;

                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) > child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) < child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }
                                        }
                                    }
                                    else // Undragging mode
                                    {
                                        if (((coverFlow->inc > 0) && ((cal_init_pos + fix) < child_rect_pos)) ||
                                            ((coverFlow->inc < 0) && ((cal_init_pos + fix) > child_rect_pos)))
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                (void)printf("[D][%d] [frame %d]\n", cal_init_pos + fix, coverFlow->frame);
                                            }

                                            ituCoverFlowSetXY(coverFlow, i, cal_init_pos - coverFlow->overlapsize + fix,
                                                              __LINE__);
                                        }
                                        else
                                        {
                                            if ((i == 0) && local_debug_print)
                                            {
                                                if (coverFlow->inc > 0)
                                                {
                                                    (void)printf("[%d + %d] < [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                                else
                                                {
                                                    (void)printf("[%d + %d] > [%d] [frame %d]\n", cal_init_pos, fix,
                                                           child_rect_pos, coverFlow->frame);
                                                }
                                            }

                                            ////////// try to save more frame when wrong position 2018/4/12
                                            if (i == 0)
                                            {
                                                int  savecount = 0;
                                                bool recheck   = true;
                                                while (recheck)
                                                {
                                                    coverFlow->frame++;
                                                    step = CoverFlowAniStepCal(coverFlow);

                                                    if (coverFlow->frame < coverFlow->totalframe)
                                                    {
                                                        if (abs(coverFlow->inc) >= (base_size / 2))
                                                        {
                                                            fix = (base_size - (int)((float)(base_size)*step));

                                                            if (coverFlow->inc < 0)
                                                            {
                                                                if (fix > 0)
                                                                {
                                                                    fix *= -1;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            fix = (base_size - (int)((float)(base_size)*step));
                                                            fix *= ((coverFlow->inc > 0) ? (1) : (-1));
                                                        }

                                                        if (((coverFlow->inc > 0) &&
                                                             ((cal_init_pos + fix) < child_rect_pos)) ||
                                                            ((coverFlow->inc < 0) &&
                                                             ((cal_init_pos + fix) > child_rect_pos)))
                                                        {
                                                            coverFlow->frame--;
                                                            i       = childCount;
                                                            recheck = false;
                                                        }
                                                        else
                                                        {
                                                            savecount++;
                                                            // (void)printf("[Coverflow][Undragging][Position sync save frame
                                                            // %d]\n", savecount);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        recheck = false;
                                                    }
                                                }
                                            }
                                            /////////////////////////////////
                                        }
                                    }
                                }
                            }

                            if (coverFlow->frame <= coverFlow->totalframe)
                            {
                                // for goose do more one frame at last frame (slow down)
                                if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) &&
                                    (coverFlow->frame == (coverFlow->totalframe - 1)) &&
                                    (childCount <= COVERFLOW_SMOOTH_LASTFRAME_COUNT))
                                {
                                    if (coverFlow->org_totalframe == coverFlow->totalframe)
                                    {
                                        coverFlow->totalframe *= 2;
                                        coverFlow->frame = coverFlow->totalframe - 2;
                                    }
                                }
                            }
                        }

                        if (coverFlow->focusIndex < 0)
                        {
                            coverFlow->focusIndex = 0;
                        }
                        else if (coverFlow->focusIndex > (childCount - 1))
                        {
                            coverFlow->focusIndex = childCount - 1;
                        }
                        else
                        {
                            /* do nothing */
                        }

                        
                    }
                }
            }

            coverFlow->frame++;
            // (void)printf("coverflow frame %d  fd %d\n", coverFlow->frame, coverFlow->focusIndex);

            // try motion
            if ((childCount > COVERFLOW_SMOOTH_LASTFRAME_COUNT) && (coverFlow->slideMaxCount > 2) &&
                (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
            {
                int cycle_step = (coverFlow->slideMaxCount - coverFlow->slideCount);

                // (void)printf("slidecount %d\n", coverFlow->slideCount);

                if ((coverFlow->totalframe == coverFlow->org_totalframe) && (cycle_step <= 2))
                {
                    coverFlow->totalframe *= 2;
                }
                else if ((coverFlow->totalframe != coverFlow->org_totalframe) && (cycle_step > 2))
                {
                    coverFlow->totalframe = coverFlow->org_totalframe;
                }
                else
                {
                    /* do nothing */
                }

                coverFlow->frame += (cycle_step / 2);
            #if 0
                (void)printf("[%d]coverFlow->frame %d\n", coverFlow->slideCount, coverFlow->frame);
                (void)printf("%d\t", coverFlow->frame);

                (void)printf("%d\t", cycle_step / 3);
            #endif
            }

            // for goose reset frame at last frame (slow down)
            // this case onlt use for count small (<= COVERFLOW_SMOOTH_LASTFRAME_COUNT)
            if ((coverFlow->frame > coverFlow->totalframe) && (childCount <= COVERFLOW_SMOOTH_LASTFRAME_COUNT))
            {
                if (coverFlow->org_totalframe != coverFlow->totalframe)
                {
                    coverFlow->totalframe /= 2;
                    coverFlow->frame = coverFlow->totalframe + 1;
                }
            }

            // <<< ITU_EVENT_TIMER >>>
            // <<< Frame END >>>
            if (coverFlow->frame > coverFlow->totalframe)
            {
                if (widget->flags & ITU_BOUNCING)
                {
                    widget->flags &= ~ITU_BOUNCING;
                    coverFlow->coverFlowFlags &= ~ITU_BOUNCE_1;
                    coverFlow->coverFlowFlags &= ~ITU_BOUNCE_2;
                }

                if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                    !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
                {
                    bool        alignment_done = true;
                    ITUWidget * child_1        = CoverFlowGetVisibleChild(coverFlow, 0);
                    ITUWidget * child_2        = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                    if ((child_1 != NULL) && (child_2 != NULL))
                    {
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                        {
                            if (child_1->rect.y > 0)
                            {
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE1;
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE2;
                                alignment_done = false;
                                coverFlow->frame--;
                            }
                            else if ((child_2->rect.y + child_2->rect.height) < (widget->rect.height))
                            {
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE1;
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE2;
                                alignment_done = false;
                                coverFlow->frame--;
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                        else
                        {
                            if ((child_1->rect.x) > 0)
                            {
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE1;
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE2;
                                alignment_done = false;
                                coverFlow->frame--;
                            }
                            else if ((child_2->rect.x + child_2->rect.width) < widget->rect.width)
                            {
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE1;
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE2;
                                alignment_done = false;
                                coverFlow->frame--;
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                    }

                    // to avoid bounce turn back not finish when frame end.
                    if (alignment_done)
                    {
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE)
                        {
                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE;
                        }

                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE1)
                        {
                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE1;

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex, 0, __LINE__);
                            }

                            coverFlow->focusIndex = 0;

                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
                            coverFlow->inc   = 0;
                            coverFlow->frame = 0;
                        }
                        else if (coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYBOUNCE2)
                        {
                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_ANYBOUNCE2;

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex, childCount - 1,
                                       __LINE__);
                            }

                            coverFlow->focusIndex = childCount - 1;

                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
                            coverFlow->inc   = 0;
                            coverFlow->frame = 0;
                        }
                        else
                        {
                            /* do nothing */
                        }

                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
                        {
                            // coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
                            // coverFlow->frame = 0;
                            // coverFlow->inc = 0;

                            if (coverFlow->slideCount < coverFlow->slideMaxCount)
                            {
                                coverFlow->frame = coverFlow->totalframe + 1;
                            }
                            else
                            {
                                coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
                                coverFlow->inc   = 0;
                                coverFlow->frame = 0;
                            }
                        }
                    }
                }
                // here two case should be debug long time
                else if (coverFlow->inc > 0)
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Frame END >>>
                    // <<< INC > 0 >>>
                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) ||
                        !(widget->flags & ITU_DRAGGABLE)) // || (!coverFlow->boundaryAlign))
                    {
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                        {
                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                            }

                            if (coverFlow->focusIndex <= 0)
                            {
                                coverFlow->focusIndex = childCount - 1;
                            }
                            else
                            {
                                coverFlow->focusIndex--;
                            }

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                            }

                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                            // ituCoverFlowOnCoverChanged(coverFlow, widget);
                        }
                        else
                        {
                            if (coverFlow->focusIndex > 0)
                            {
                                // fix the alignment wrong when sliding start at maximum focusindex then stop wrong
                                // layout when sliding done.
                                int mod                        = 1;
                                int max_touch_start_size_index = (childCount - CoverFlowSaveDiv(widget_size, base_size));
                                if ((coverFlow->boundaryAlign) &&
                                    (coverFlow->focusIndex >= max_touch_start_size_index) &&
                                    (widget_size > (base_size * 2)))
                                {
                                    mod = coverFlow->focusIndex - max_touch_start_size_index + 1;
                                }

                                if (COVERFLOW_DEBUG_FOCUSINDEX)
                                {
                                    (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                           coverFlow->focusIndex - mod, __LINE__);
                                }

                                coverFlow->focusIndex -= mod;
                                ituCoverFlowFixFC(coverFlow);
                                ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                                // ituCoverFlowOnCoverChanged(coverFlow, widget);
                            }
                        }
                    }
                    else if (widget->flags & ITU_UNDRAGGING)
                    {
                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) && (coverFlow->dragFlag < 1))
                        // if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                        {
                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                            }

                            if (coverFlow->focusIndex <= 0)
                            {
                                coverFlow->focusIndex = childCount - 1;
                            }
                            else
                            {
                                coverFlow->focusIndex--;
                            }

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                            }

                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                        }
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< Frame END >>>
                    // <<< INC < 0 >>>
                    if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING) ||
                        !(widget->flags & ITU_DRAGGABLE)) // || (!coverFlow->boundaryAlign))
                    {
                        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                        {
                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                            }

                            if (coverFlow->focusIndex >= childCount - 1)
                            {
                                coverFlow->focusIndex = 0;
                            }
                            else
                            {
                                coverFlow->focusIndex++;
                            }

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                            }

                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                            // ituCoverFlowOnCoverChanged(coverFlow, widget);
                        }
                        else
                        {
                            if (coverFlow->focusIndex < (childCount - 1))
                            {
                                if (COVERFLOW_DEBUG_FOCUSINDEX)
                                {
                                    (void)printf("[Debug][CoverFlow][FC %d to %d][%d]\n", coverFlow->focusIndex,
                                           coverFlow->focusIndex + 1, __LINE__);
                                }

                                coverFlow->focusIndex++;
                                ituCoverFlowFixFC(coverFlow);
                                ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                                // ituCoverFlowOnCoverChanged(coverFlow, widget);
                            }
                        }
                    }
                    else if (widget->flags & ITU_UNDRAGGING)
                    {
                        if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE) && (coverFlow->dragFlag < 1))
                        // if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                        {
                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
                            }

                            if (coverFlow->focusIndex >= childCount - 1)
                            {
                                coverFlow->focusIndex = 0;
                            }
                            else
                            {
                                coverFlow->focusIndex++;
                            }

                            if (COVERFLOW_DEBUG_FOCUSINDEX)
                            {
                                (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
                            }

                            ituCoverFlowFixFC(coverFlow);
                            ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                        }
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING)) // && coverFlow->boundaryAlign)
                {
                    coverFlow->slideCount++;

                    if (((coverFlow->slideCount + 1) >= coverFlow->slideMaxCount) || (coverFlow->focusIndex < 0) ||
                        (coverFlow->focusIndex > (childCount - 1)))
                    {
                        if (coverFlow->frame <= coverFlow->totalframe)
                        {
                            coverFlow->slideCount--;
                        }
                        else
                        {
                            coverFlow->slideCount = 0;
                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;

                            coverFlow->frame = 0;
                            coverFlow->inc   = 0;

                            if (!(widget->flags & ITU_DRAGGING)) // for sliding then mousedown --> do not perform
                                                                 // this(keep dragging for goose)
                            {
                                CoverFlowFlushQueue(widget, coverFlow, childCount, widget_size, base_size);
                            }
                        }
                    }
                    else // should do animation again when slidecount < slidemaxcount
                    {
                        if (widget->flags & ITU_DRAGGING)
                        {
                            coverFlow->inc = 0;
                            coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;
                        }

                        if (coverFlow->inc)
                        {
                            coverFlow->frame = 1; // use 1 to make the animation frame more smooth
                            return true;
                        }
                    }
                }
                else if (!(coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP))
                {
                    // <<< ITU_EVENT_TIMER >>>
                    // <<< QUEUE process >>>
                    // long long *customdata = (long long *)ituWidgetGetCustomData(coverFlow);
                    int  i           = 0;
                    bool no_queue    = true;
                    coverFlow->frame = 0;
                    // do not change here 20170920
                    // ituExecActions(widget, coverFlow->actions, ITU_EVENT_CHANGED, coverFlow->focusIndex);
                    ituCoverFlowOnCoverChanged(coverFlow, widget);

                    for (i = 0; i < COVERFLOW_MAX_PROCARR_SIZE; i++)
                    {
                        if (coverFlow->procArr[i] != 0)
                        {
                            coverFlow->procArr[i] = 0;

                            if ((i + 1) < COVERFLOW_MAX_PROCARR_SIZE)
                            {
                                if (coverFlow->procArr[i + 1] != 0)
                                {
                                    no_queue = false;
                                }
                            }

                            break;
                        }
                    }

                    if (no_queue)
                    {
                        coverFlow->inc = 0;
                        widget->flags &= ~ITU_UNDRAGGING;
                        // mark now, debug side effect for wheel update
                        ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
                        // CoverFlowLayout(widget);
                    }
                    else
                    {
                        bool boundary_touch = false;

                        if (coverFlow->boundaryAlign)
                        {
                            int max_neighbor_item = (CoverFlowSaveDiv(widget_size, base_size) - 1) / 2;

                            coverFlow->slideCount = 0;

                            if (max_neighbor_item == 0)
                            {
                                max_neighbor_item++;
                            }

                            if (coverFlow->focusIndex >= max_neighbor_item)
                            {
                                if ((childCount >= (max_neighbor_item * 2 + 1)) &&
                                    ((childCount - coverFlow->focusIndex - 1) < max_neighbor_item))
                                {
                                    boundary_touch = true;
                                }
                                else
                                {
                                    ITUWidget * cf = CoverFlowGetVisibleChild(coverFlow, childCount - 1);
                                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                                    {
                                        if ((cf->rect.y + cf->rect.height) <= widget_size)
                                        {
                                            boundary_touch = true;
                                        }
                                    }
                                    else
                                    {
                                        if ((cf->rect.x + cf->rect.width) <= widget_size)
                                        {
                                            boundary_touch = true;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                boundary_touch = true;
                            }
                        }

                        if (!boundary_touch)
                        {
                            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                            {
                                if (coverFlow->procArr[i + 1] < 0)
                                {
                                    ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEDOWN, 0, widget->rect.x,
                                                    widget->rect.y);
                                }
                                else if (coverFlow->procArr[i + 1] > 0)
                                {
                                    ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEUP, 0, widget->rect.x, widget->rect.y);
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                if (coverFlow->procArr[i + 1] < 0)
                                {
                                    ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDERIGHT, 0, widget->rect.x,
                                                    widget->rect.y);
                                }
                                else if (coverFlow->procArr[i + 1] > 0)
                                {
                                    ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDELEFT, 0, widget->rect.x,
                                                    widget->rect.y);
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                        }
                    }

                    ituScene->dragged = NULL;
                }
                else
                {
                    /* do nothing */
                }
            }
        }
    }

    if (widget->flags & ITU_TOUCHABLE)
    {
        // <<< Default Event Clear >>>
        if (ev == ITU_EVENT_MOUSEDOWN || ev == ITU_EVENT_MOUSEUP || ev == ITU_EVENT_MOUSEDOUBLECLICK ||
            ev == ITU_EVENT_MOUSELONGPRESS || ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDEUP ||
            ev == ITU_EVENT_TOUCHSLIDERIGHT || ev == ITU_EVENT_TOUCHSLIDEDOWN)
        {
            if (ituWidgetIsEnabled(widget))
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if (ituWidgetIsInside(widget, x, y))
                {
                    result |= widget->dirty;
                    return widget->visible ? result : false;
                }
            }
        }
    }

    if (ev == ITU_EVENT_LAYOUT)
    {
        // to avoid integer div zero
        if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, 0);

            if (child != NULL)
            {
                if (coverFlow->overlapsize > (child->rect.height * COVERFLOW_OVERLAP_MAX_PERCENTAGE / 100))
                {
                    coverFlow->overlapsize = child->rect.height * COVERFLOW_OVERLAP_MAX_PERCENTAGE / 100;
                }

                // not support vertical yet
                // coverFlow->overlapsize = 0;

                if (coverFlow->totalframe > child->rect.height)
                {
                    coverFlow->totalframe = child->rect.height;
                }
            }
        }
        else
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, 0);

            if (coverFlow->overlapsize > (child->rect.width * COVERFLOW_OVERLAP_MAX_PERCENTAGE / 100))
            {
                coverFlow->overlapsize = child->rect.width * COVERFLOW_OVERLAP_MAX_PERCENTAGE / 100;
            }

            if (coverFlow->totalframe > child->rect.width)
            {
                coverFlow->totalframe = child->rect.width;
            }
        }

        if (childCount > 0)
        {
            if (coverFlow->focusIndex > (childCount - 1))
            {
                coverFlow->focusIndex = childCount - 1;
            }
            else if (coverFlow->focusIndex < 0)
            {
                coverFlow->focusIndex = 0;
            }
            else
            {
                /* do nothing */
            }

            if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
            {
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int i, index, count2;

                    count2 = childCount / 2 + 1;
                    index  = coverFlow->focusIndex;

                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fy = widget->rect.height / 2 - base_size / 2;
                            fy += i * child->rect.height;
                            ituCoverFlowSetXY(coverFlow, index, fy, __LINE__);
                        }

                        if (index >= childCount - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }

                    count2 = childCount - count2;
                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fy = widget->rect.height / 2 - base_size / 2;
                            fy -= count2 * child->rect.height;
                            fy += i * child->rect.height;
                            ituCoverFlowSetXY(coverFlow, index, fy, __LINE__);
                        }

                        if (index >= childCount - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }
                }
                else //[LAYOUT][Vertical][non-cycle]
                {
                    int i = 0;
                    for (; i < childCount; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                        int         fy    = widget->rect.height / 2 - base_size / 2;

                        
                        if (coverFlow->boundaryAlign)
                        {
                            if ((base_size * childCount) <= widget_size)
                            {
                                fy = 0;
                            }
                            else if (base_size >= widget_size)
                            {
                                fy = 0 - (coverFlow->focusIndex * base_size);
                            }
                            else
                            {
                                // try fix wrong boundary position when big focus index under non-cycle mode 2019/10/21
                                int max_touch_start_size_index = (childCount - 1 - CoverFlowSaveDiv(widget_size, base_size));

                                if (coverFlow->focusIndex <= max_touch_start_size_index)
                                {
                                    fy = 0 - (coverFlow->focusIndex * base_size);
                                }
                                else
                                {
                                    // fy = 0 - (max_touch_start_size_index * base_size);
                                    fy = widget_size - childCount * base_size; // try fix wrong boundary position when big
                                                                          // focus index under non-cycle mode 2019/10/21
                                }
                            }
                        }
                        else
                        {
                            fy -= base_size * coverFlow->focusIndex;
                        }

                        if (coverFlow->overlapsize > 0)
                        {
                            fy += i * base_size;
                            ituCoverFlowSetXY(coverFlow, i, fy - coverFlow->overlapsize, __LINE__);
                        }
                        else
                        {
                            if (child != NULL)
                            {
                                fy += i * child->rect.height;
                                ituCoverFlowSetXY(coverFlow, i, fy, __LINE__);
                            }
                        }
                    }
                }
            }
            else
            {
                //[LAYOUT][Horizontal][cycle]
                if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
                {
                    int i, index, count2;

                    count2 = childCount / 2 + 1;
                    index  = coverFlow->focusIndex;

                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fx = widget->rect.width / 2 - child->rect.width / 2;
                            fx += i * child->rect.width;
                            ituCoverFlowSetXY(coverFlow, index, fx, __LINE__);
                        }

                        if (index >= childCount - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }

                    count2 = childCount - count2;
                    for (i = 0; i < count2; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, index);
                        if (child != NULL)
                        {
                            int         fx = widget->rect.width / 2 - child->rect.width / 2;
                            fx -= count2 * child->rect.width;
                            fx += i * child->rect.width;
                            ituCoverFlowSetXY(coverFlow, index, fx, __LINE__);
                        }

                        if (index >= childCount - 1)
                        {
                            index = 0;
                        }
                        else
                        {
                            index++;
                        }
                    }
                }
                else //[LAYOUT][Horizontal][non-cycle]
                {
                    int i = 0;
                    for (; i < childCount; ++i)
                    {
                        ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);
                        int         fx    = widget->rect.width / 2 - base_size / 2;

                        
                        if (coverFlow->boundaryAlign)
                        {
                            if ((base_size * childCount) <= widget_size)
                            {
                                fx = 0;
                            }
                            else if (base_size >= widget_size)
                            {
                                fx = 0 - (coverFlow->focusIndex * base_size);
                            }
                            else
                            {
                                // try fix wrong boundary position when big focus index under non-cycle mode 2019/10/21
                                int max_touch_start_size_index = (childCount - 1 - CoverFlowSaveDiv(widget_size, base_size));

                                if (coverFlow->focusIndex <= max_touch_start_size_index)
                                {
                                    fx = 0 - (coverFlow->focusIndex * base_size);
                                }
                                else
                                {
                                    // fx = 0 - (max_touch_start_size_index * base_size);
                                    fx = widget_size - childCount * base_size; // try fix wrong boundary position when big
                                                                          // focus index under non-cycle mode 2019/10/21
                                }
                            }
                        }
                        else
                        {
                            fx -= base_size * coverFlow->focusIndex;
                        }

                        if (coverFlow->overlapsize > 0)
                        {
                            fx += i * base_size;
                            ituCoverFlowSetXY(coverFlow, i, fx - coverFlow->overlapsize, __LINE__);
                        }
                        else
                        {
                            if (child != NULL)
                            {
                                fx += i * child->rect.width;
                                ituCoverFlowSetXY(coverFlow, i, fx, __LINE__);
                            }
                        }
                    }
                }
            }

            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ENABLE_ALL) == 0)
            {
                int i = 0;
                for (; i < childCount; ++i)
                {
                    ITUWidget * child = CoverFlowGetVisibleChild(coverFlow, i);

                    if (child != NULL)
                    {
                        if (i == coverFlow->focusIndex)
                        {
                            ituWidgetEnable(child);
                        }
                        else
                        {
                            ituWidgetDisable(child);
                        }
                    }
                }
            }

            if ((widget->flags & ITU_DRAGGING) && (coverFlow->coverFlowFlags & ITU_COVERFLOW_SLIDING))
            {
                // for goose prevent mousedown can not drag when under sliding
            }
            else
            {
                widget->flags &= ~ITU_DRAGGING;
                coverFlow->touchCount = 0;
            }

            // fix for stop anywhere not display after load
            if ((coverFlow->coverFlowFlags & ITU_COVERFLOW_ANYSTOP) &&
                !(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE))
            {
                ITUWidget * child_1 = CoverFlowGetVisibleChild(coverFlow, 0);
                ITUWidget * child_2 = CoverFlowGetVisibleChild(coverFlow, childCount - 1);

                if ((child_1 != NULL) && (child_2 != NULL))
                {
                    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                    {
                        if ((child_1->rect.y > 0) || ((child_2->rect.y + child_2->rect.height) < widget->rect.height))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.y > 0)
                            {
                                move_step = 0 - child_1->rect.y;
                            }
                            else
                            {
                                move_step = widget->rect.height - (child_2->rect.y + child_2->rect.height);
                            }

                            for (i = 0; i < childCount; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.y;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                    else
                    {
                        if ((child_1->rect.x > 0) || ((child_2->rect.x + child_2->rect.width) < widget->rect.width))
                        {
                            int i, move_step = 0;
                            if (child_1->rect.x > 0)
                            {
                                move_step = 0 - child_1->rect.x;
                            }
                            else
                            {
                                move_step = widget->rect.width - (child_2->rect.x + child_2->rect.width);
                            }

                            for (i = 0; i < childCount; i++)
                            {
                                ITUWidget* child = CoverFlowGetVisibleChild(coverFlow, i);
                                if (child != NULL)
                                {
                                    int         fd = child->rect.x;
                                    fd += move_step;
                                    ituCoverFlowSetXY(coverFlow, i, fd, __LINE__);
                                }
                            }

                            coverFlow->frame = 0;
                        }
                    }
                }
            }
        }
    }

    result |= widget->dirty;

    // fix bug when widget not visible but will still have dirty
    if ((widget->visible == 0) && (widget->dirty))
    {
        ituDirtyWidget(widget, false);
    }

    return widget->visible ? result : false;
}

void ituCoverFlowDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int            destx, desty;
    uint8_t        desta;
    ITURectangle   prevClip;
    ITURectangle * rect = (ITURectangle *)&widget->rect;

    assert(widget);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->color.alpha / 255;
    desta = desta * widget->alpha / 255;

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    if (desta == 255)
    {
        ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
    }
    else if (desta > 0)
    {
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
        if (surf)
        {
            ituColorFill(surf, 0, 0, rect->width, rect->height, &widget->color);
            ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
            ituDestroySurface(surf);
        }
    }
    else
    {
        /* do nothing */
    }

    if ((desta == 0) || (desta == 255))
    {
        ITUCoverFlow * coverflow = (ITUCoverFlow *)widget;

        if (coverflow->totalframe == 0)
        {
            coverflow->totalframe = coverflow->org_totalframe;
        }

        if (coverflow->inc)
        {
            int way  = ((coverflow->inc > 0) ? (-1) : (1));
            int step = CoverFlowSaveDiv(widget->rect.width, coverflow->totalframe);

            if (!(widget->flags & ITU_DRAGGING))
            {
                if (coverflow->eye_motion >= 2)
                {
                    ituFlowWindowDraw(widget, dest, x + (step * way), y, 60);
                    // (void)printf("[coverflow]eye_motion[2]\n");
                }

                if (coverflow->eye_motion >= 1)
                {
                    ituFlowWindowDraw(widget, dest, x + ((step / 2) * way), y, 120);
                    // (void)printf("[coverflow]eye_motion[1]\n");
                }
            }

            ituFlowWindowDraw(widget, dest, x, y, 255);
        }
        else
        {
            ituFlowWindowDraw(widget, dest, x, y, alpha);
        }
    }
    else
    {
        ituFlowWindowDraw(widget, dest, x, y, alpha);
    }

    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    ituDirtyWidget(widget, false);
}

void ituCoverFlowOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    ITUCoverFlow * coverFlow = (ITUCoverFlow *)widget;
    assert(NULL != coverFlow);

    switch (action)
    {
        case ITU_ACTION_PREV:
            ituCoverFlowPrev((ITUCoverFlow *)widget);
            break;

        case ITU_ACTION_NEXT:
            ituCoverFlowNext((ITUCoverFlow *)widget);
            break;

        case ITU_ACTION_GOTO:
            if (param[0] != '\0')
            {
                ituCoverFlowGoto((ITUCoverFlow *)widget, atoi(param));
            }
            break;

        case ITU_ACTION_DODELAY0:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY0, atoi(param));
            break;

        case ITU_ACTION_DODELAY1:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY1, atoi(param));
            break;

        case ITU_ACTION_DODELAY2:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY2, atoi(param));
            break;

        case ITU_ACTION_DODELAY3:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY3, atoi(param));
            break;

        case ITU_ACTION_DODELAY4:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY4, atoi(param));
            break;

        case ITU_ACTION_DODELAY5:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY5, atoi(param));
            break;

        case ITU_ACTION_DODELAY6:
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY6, atoi(param));
            break;

        case ITU_ACTION_DODELAY7:
            if ((!(coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) && (coverFlow->boundaryAlign))
            {
                coverFlow->coverFlowFlags |= ITU_COVERFLOW_ANYSTOP;
            }
            ituExecActions(widget, coverFlow->actions, ITU_EVENT_DELAY7, atoi(param));
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

static void CoverFlowOnCoverChanged (ITUCoverFlow * coverFlow, ITUWidget * widget)
{
    // DO NOTHING
}

void ituCoverFlowInit (ITUCoverFlow * coverFlow, ITULayout layout)
{
    assert(NULL != coverFlow);
    ITU_ASSERT_THREAD();

    (void)memset(coverFlow, 0, sizeof(ITUCoverFlow));

    if (layout == ITU_LAYOUT_VERTICAL)
    {
        coverFlow->coverFlowFlags &= ITU_COVERFLOW_VERTICAL;
    }

    ituFlowWindowInit(&coverFlow->fwin, layout);

    ituWidgetSetType(coverFlow, ITU_COVERFLOW);
    ituWidgetSetName(coverFlow, coverFlowName);
    ituWidgetSetClone(coverFlow, ituCoverFlowClone);
    ituWidgetSetUpdate(coverFlow, ituCoverFlowUpdate);
    ituWidgetSetDraw(coverFlow, ituCoverFlowDraw);
    ituWidgetSetOnAction(coverFlow, ituCoverFlowOnAction);
    ituCoverFlowSetCoverChanged(coverFlow, CoverFlowOnCoverChanged);
}

void ituCoverFlowLoad (ITUCoverFlow * coverFlow, uint32_t base)
{
    assert(NULL != coverFlow);

    ituFlowWindowLoad(&coverFlow->fwin, base);
    ituWidgetSetClone(coverFlow, ituCoverFlowClone);
    ituWidgetSetUpdate(coverFlow, ituCoverFlowUpdate);
    ituWidgetSetDraw(coverFlow, ituCoverFlowDraw);
    ituWidgetSetOnAction(coverFlow, ituCoverFlowOnAction);
    ituCoverFlowSetCoverChanged(coverFlow, CoverFlowOnCoverChanged);

    if (coverFlow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
    {
        coverFlow->boundaryAlign = false;
    }
#if 0
    else
    {
        coverFlow->slideMaxCount = 1;
    }
#endif

    if (coverFlow->slideMaxCount)
    {
        if ((coverFlow->slideMaxCount < 3) && (coverFlow->slideMaxCount > 0))
        {
            coverFlow->slideMaxCount = 1;
        }
    }
}

void ituCoverFlowGoto (ITUCoverFlow * coverFlow, int index)
{
    assert(NULL != coverFlow);
    ITU_ASSERT_THREAD();

    if (coverFlow->focusIndex == index)
    {
        return;
    }

    if (COVERFLOW_DEBUG_FOCUSINDEX)
    {
        (void)printf("[Debug][CoverFlow][FC %d ", coverFlow->focusIndex);
    }

    coverFlow->focusIndex = index;

    if (COVERFLOW_DEBUG_FOCUSINDEX)
    {
        (void)printf("to %d][%d]\n", coverFlow->focusIndex, __LINE__);
    }

    ituWidgetUpdate(coverFlow, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituCoverFlowPrev (ITUCoverFlow * coverflow)
{
    ITUWidget *  widget   = (ITUWidget*)coverflow;
    int          i        = 0;
	int          argDef   = 0;
    ITU_ASSERT_THREAD();

    if (widget == NULL)
    {
        return;
    }

	if ((widget->flags & ITU_TOUCHABLE) == 0)
		argDef = -1;

    if ((!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) && (coverflow->boundaryAlign == 0))
    {
        if (coverflow->focusIndex <= 0)
        {
            return;
        }
    }

    for (i = COVERFLOW_MAX_PROCARR_SIZE - 1; i >= 0; i--)
    {
        if (coverflow->procArr[i] != 0)
        {
            break;
        }
    }
    if (i == (COVERFLOW_MAX_PROCARR_SIZE - 1))
    {
        return;
    }
    else if (i < 0)
    {
        coverflow->touchPos = 0;

        if (coverflow->slideMaxCount == 0)
        {
            coverflow->prevnext_trigger = 1;
        }

        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
			ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEDOWN, argDef, widget->rect.x, widget->rect.y);
        }
        else
        {
			ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDERIGHT, argDef, widget->rect.x, widget->rect.y);
        }
    }
    else
    {
        coverflow->procArr[i + 1] = -1;
    }
}

void ituCoverFlowNext (ITUCoverFlow * coverflow)
{
    ITUWidget *  widget   = (ITUWidget*)coverflow;
    int          i        = 0;
    int          count    = 0;
	int          argDef   = 0;
    ITU_ASSERT_THREAD();

    if (widget == NULL)
    {
        return;
    }

    count = CoverFlowGetVisibleChildCount(coverflow);

	if ((widget->flags & ITU_TOUCHABLE) == 0)
		argDef = -1;

    if ((!(coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)) && (coverflow->boundaryAlign == 0))
    {
        if (coverflow->focusIndex >= (count - 1))
        {
            return;
        }
    }

    for (i = COVERFLOW_MAX_PROCARR_SIZE - 1; i >= 0; i--)
    {
        if (coverflow->procArr[i] != 0)
        {
            break;
        }
    }
    if (i == (COVERFLOW_MAX_PROCARR_SIZE - 1))
    {
        return;
    }
    else if (i < 0)
    {
        coverflow->touchPos = 0;

        if (coverflow->slideMaxCount == 0)
        {
            coverflow->prevnext_trigger = 1;
        }

        if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
        {
			ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEUP, argDef, widget->rect.x, widget->rect.y);
        }
        else
        {
			ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDELEFT, argDef, widget->rect.x, widget->rect.y);
        }
    }
    else
    {
        coverflow->procArr[i + 1] = 1;
    }
}

int CoverFlowGetFirstDisplayIndex (ITUCoverFlow * coverflow)
{
    assert(coverflow);
    ITU_ASSERT_THREAD();

    if (coverflow->coverFlowFlags & ITU_COVERFLOW_CYCLE)
    {
        return -1;
    }
    else
    {
        int i, count = CoverFlowGetVisibleChildCount(coverflow);

        for (i = 0; i < count; i++)
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverflow, i);

            if (child != NULL)
            {
                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    if ((child->rect.y + child->rect.height) > 0)
                    {
                        return i;
                    }
                }
                else
                {
                    if ((child->rect.x + child->rect.width) > 0)
                    {
                        return i;
                    }
                }
            }
            else
            {
                return -1;
            }
        }

        return -1;
    }
}

int CoverFlowGetDraggingDist (ITUCoverFlow * coverflow)
{
    ITUWidget * widget = (ITUWidget *)coverflow;
    ITU_ASSERT_THREAD();

    if (!widget)
    {
        return 0;
    }
    else
    {
        if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGING))
        {
            ITUWidget * child = CoverFlowGetVisibleChild(coverflow, 0);
            int         pos;

            if (child != NULL)
            {
                if (coverflow->coverFlowFlags & ITU_COVERFLOW_VERTICAL)
                {
                    pos = child->rect.y;
                }
                else
                {
                    pos = child->rect.x;
                }
                
                return (pos - coverflow->mousedown_position);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
}

bool CoverFlowCheckIdle (ITUCoverFlow * coverflow)
{
    const ITUWidget * widget = (ITUWidget *)coverflow;

    if (coverflow->inc)
    {
        return false;
    }

    if (coverflow->frame)
    {
        return false;
    }

    if (coverflow->totalframe != coverflow->org_totalframe)
    {
        return false;
    }

    if (widget->flags & ITU_DRAGGING)
    {
        return false;
    }

    if (coverflow->coverFlowFlags & ITU_COVERFLOW_SLIDING)
    {
        return false;
    }

    return true;
}

void ituCoverFlowDisableMoveUnPress (ITUCoverFlow * coverFlow, bool disable)
{
    assert(NULL != coverFlow);
    ITU_ASSERT_THREAD();

    if (disable)
    {
        coverFlow->coverFlowFlags |= ITU_COVERFLOW_DISABLE_MOVE_UNPRESS;
    }
    else
    {
        coverFlow->coverFlowFlags &= ~ITU_COVERFLOW_DISABLE_MOVE_UNPRESS;
    }
}

void ituCoverFlowReset (ITUCoverFlow * coverflow)
{
    assert(coverflow);
    ITU_ASSERT_THREAD();

    if (coverflow)
    {
        ITUWidget * widget    = (ITUWidget *)coverflow;
        coverflow->inc        = 0;
        coverflow->frame      = 0;
        coverflow->totalframe = coverflow->org_totalframe;
        widget->flags &= ~ITU_UNDRAGGING;
        widget->flags &= ~ITU_DRAGGING;
        coverflow->coverFlowFlags &= ~ITU_COVERFLOW_SLIDING;

        CoverFlowCleanQueue(coverflow);
        coverflow->touchCount = 0;
        if (ituScene)
        {
            if (ituScene->dragged == widget)
            {
                ituScene->dragged = NULL;
            }
        }
    }
}
