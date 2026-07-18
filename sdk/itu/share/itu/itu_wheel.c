#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#define WHEEL_OUTSIDE_DRAGGABLE       0 // special case for some customer
#define WHEEL_CYCLE_FIT_OUTSIDEBUFFER 1

#define WHEEL_DEBUG_MSG               0
static const char wheelName[] = "ITUWheel";

ITUWidget *       ituWheelGetChild (ITUWidget * widget, int index)
{
    int         count = itcTreeGetChildCount((ITCTree *)widget);
    ITUWidget * child = NULL;
    if (count > 0)
    {
        int mIndex = (index < 0) ? (count + index) : ((index >= count) ? (index - count) : (index));
        child      = (ITUWidget *)itcTreeGetChildAt((ITCTree *)widget, mIndex);
    }
    return child;
}

void ituWheelChildSetY (const ITUWheel * wheel, ITUWidget * child, int index, int y, int line)
{
    if (wheel && child)
    {
        ituWidgetSetY(child, y);
        // if (index == wheel->focusIndex)
        //  (void)printf("[DBG]fc set to %d [Line %d]\n", y, line);
    }
}

void ituWheelCycleArrange (ITUWidget * widget)
{
    ITUWheel * wheel = (ITUWheel *)widget;

    if (wheel)
    {
        int i                  = 0;
        int count              = itcTreeGetChildCount((ITCTree *)widget);
        // ITUWidget* child = ituWheelGetChild(widget, 0);
        // int focusPosFix = (wheel->focusFontHeight - wheel->fontHeight) / ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV;
        // firstIndex is left 1/2 arr_count shift from focusIndex
        int firstItemLeftShift = ((wheel->cycle_arr_count - 1) / 2);
        int pIndex             = wheel->focusIndex - firstItemLeftShift;
        pIndex                 = (pIndex < 0) ? (count + pIndex) : (pIndex);

        for (i = 0; i < wheel->cycle_arr_count; i++)
        {
            wheel->cycle_arr[i] = pIndex;
            pIndex++;
            if (pIndex >= count)
            {
                pIndex = 0;
            }
        }

        wheel->minci = wheel->cycle_arr[0];
        wheel->maxci = wheel->cycle_arr[wheel->cycle_arr_count - 1];
    }
    return;
}

void ituWheelFocusChange (ITUWidget * widget, int newfocus, int line)
{
    ITUWheel * wheel  = (ITUWheel *)widget;
    int        mFocus = newfocus;

    assert(wheel);

    if (wheel->focusIndex == mFocus)
    {
        return;
    }
    else
    {
        int count = itcTreeGetChildCount((ITCTree *)widget);
        if (wheel->cycle_tor)
        {
            if (mFocus < 0)
            {
                mFocus = count + mFocus;
            }
            if (mFocus >= count)
            {
                mFocus = 0;
            }
        }
        else
        {
            if (mFocus < 0)
            {
                mFocus = 0;
            }
            if (mFocus >= count)
            {
                mFocus = count - 1;
            }
        }
        wheel->focus_c    = mFocus;
        wheel->focusIndex = mFocus;
        if (wheel->cycle_tor)
        {
            ituWheelCycleArrange(widget);
        }

        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
    }
}

static int get_max_focusindex (ITUWidget * widget)
{
    ITUWheel * wheel = (ITUWheel *)widget;
    int        i;
    int        realcount = 0;
    int        count;

    assert(wheel);

    count = itcTreeGetChildCount(wheel);

    for (i = 0; i < count; i++)
    {
        char *      str   = NULL;

        ITUWidget * child = (ITUWidget *)itcTreeGetChildAt(wheel, i);
        ITUText *   text  = (ITUText *)child;

        if (child == NULL)
        {
            continue;
        }

        if (text->fontIndex >= ITU_FREETYPE_MAX_FONTS)
        {
            text->fontIndex = 0;
        }

        str = ituTextGetString(text);

        if (str && (str[0] != '\0'))
        {
            realcount++;
        }
    }

    realcount--;

    return realcount;
}

void refix_wheel_layout (ITUWheel * wheel)
{
    if (wheel)
    {
        if (wheel->cycle_tor > 0)
        {
            ITUWidget * widget = (ITUWidget *)wheel;
            ITUWidget * child  = (ITUWidget *)itcTreeGetChildAt(wheel, 0);

            if (child)
            {
                int fitc = (widget->rect.height / child->rect.height);
                int i, fitmod, si = wheel->focus_c;

                if ((widget->rect.height % child->rect.height) > (child->rect.height / 2))
                {
                    fitc++;
                }

                // fitmod = ((fitc % 3) > 0) ? (fitc / 2) : (0);
                fitmod = ((fitc % 2) > 0) ? ((fitc - 1) / 2) : (fitc / 2);

                // for (i = 0; i < (wheel->itemCount / 2 + wheel->cycle_tor); i++)
                for (i = 0; i < (fitmod + WHEEL_CYCLE_FIT_OUTSIDEBUFFER); i++)
                {
                    si--;
                    if (si < wheel->minci)
                    {
                        si = wheel->maxci;
                    }
                }

                // for (i = 0; i < (wheel->itemCount + 2 * wheel->cycle_tor); i++)
                for (i = 0; i < (fitc + (2 * WHEEL_CYCLE_FIT_OUTSIDEBUFFER)); i++)
                {
                    wheel->cycle_arr[i] = si;

                    if ((si + 1) > wheel->maxci)
                    {
                        si = wheel->minci;
                    }
                    else
                    {
                        si++;
                    }
                }
            }
        }
    }
}

void set_wheel_font_size (const ITUWheel * wheel, ITUText * text, int size)
{
    if (wheel->fontsquare == 1)
    {
        ituTextSetFontSize(text, size);

        /*if ((shiftx == 0) && (size % 2))
        {
        shiftx = 1;
        ituWidgetSetX(text, text->widget.rect.x + shiftx);
        ituWidgetSetCustomData(text, shiftx);
        (void)printf(">>>>>\n");
        }
        else if ((shiftx != 0) && ((size % 2) == 0))
        {
        shiftx = 0;
        ituWidgetSetX(text, text->widget.rect.x - 1);
        ituWidgetSetCustomData(text, shiftx);
        (void)printf("<<<<<\n");
        }*/
    }
    else
    {
        ituTextSetFontHeight(text, size);
    }
}

void WheelRunTimeSizeColorCal (ITUWheel * wheel)
{
    int         count  = itcTreeGetChildCount(wheel);
    ITUWidget * child  = itcTreeGetChildAt(wheel, 0);
    ITUWidget * widget = (ITUWidget *)wheel;
    int         i, size_dev = wheel->focusFontHeight - wheel->fontHeight;
    int         centralPos     = widget->rect.height / 2;
    int         base_size      = child->rect.height;
    int         base_half_size = base_size / 2;
    int         min_dist       = base_size;
    for (i = 0; i < count; i++)
    {
        int dist;
        child = itcTreeGetChildAt(wheel, i);
        dist  = abs((child->rect.y + base_half_size) - centralPos);
        // ituWidgetSetColor(child, wheel->normalColor.alpha, wheel->normalColor.red, wheel->normalColor.green,
        // wheel->normalColor.blue);
        if (dist >= base_size)
        {
            if (wheel->fontsquare == 1)
            {
                ituTextSetFontSize((ITUText *)child, wheel->fontHeight);
            }
            else
            {
                ituTextSetFontHeight((ITUText *)child, wheel->fontHeight);
            }
            // (void)printf("set child %d size %d\n", i, wheel->fontHeight);
            ituWidgetSetColor(child, wheel->normalColor.alpha, wheel->normalColor.red, wheel->normalColor.green,
                              wheel->normalColor.blue);
        }
        else
        {
            float factor     = (float)(base_size - dist) / base_size;
            float stepValue  = size_dev * factor;
            float stepColorA = (wheel->focusColor.alpha - wheel->normalColor.alpha) * factor;
            float stepColorR = (wheel->focusColor.red - wheel->normalColor.red) * factor;
            float stepColorG = (wheel->focusColor.green - wheel->normalColor.green) * factor;
            float stepColorB = (wheel->focusColor.blue - wheel->normalColor.blue) * factor;
            if (wheel->fontsquare == 1)
            {
                ituTextSetFontSize((ITUText *)child, wheel->fontHeight + (int)stepValue);
            }
            else
            {
                ituTextSetFontHeight((ITUText *)child, wheel->fontHeight + (int)stepValue);
            }
            // (void)printf("set child %d size %d\n", i, wheel->fontHeight + (int)stepValue);
            ituWidgetSetColor(child, wheel->normalColor.alpha + (int)stepColorA,
                              wheel->normalColor.red + (int)stepColorR, wheel->normalColor.green + (int)stepColorG,
                              wheel->normalColor.blue + (int)stepColorB);
        }

        if (dist < min_dist)
        {
            min_dist = dist;
        }
    }
}

void WheelRuntimePosMove (ITUWheel * wheel, int move)
{
    int count = itcTreeGetChildCount(wheel);

    if (wheel->cycle_tor > 0)
    {
        int         i         = 0;
        int         prevOutId = ((wheel->cycle_arr[0] - 1) < 0) ? (count - 1) : (wheel->cycle_arr[0] - 1);
        int         nextOutId = ((wheel->cycle_arr[wheel->cycle_arr_count - 1] + 1) >= count)
                                    ? (0)
                                    : (wheel->cycle_arr[wheel->cycle_arr_count - 1] + 1);
        ITUWidget * child     = NULL;

        for (i = 0; i < wheel->cycle_arr_count; i++)
        {
            child = (ITUWidget *)itcTreeGetChildAt(wheel, wheel->cycle_arr[i]);
            child->rect.y += move;
            if ((i == 0) && (move > 0))
            {
                ITUWidget * childPrepare = (ITUWidget *)itcTreeGetChildAt(wheel, prevOutId);
                if (childPrepare)
                {
                    childPrepare->rect.y = child->rect.y - childPrepare->rect.height;
                }
            }
            if ((i == wheel->cycle_arr_count - 1) && (move < 0))
            {
                ITUWidget * childPrepare = (ITUWidget *)itcTreeGetChildAt(wheel, nextOutId);
                if (childPrepare)
                {
                    childPrepare->rect.y = child->rect.y + childPrepare->rect.height;
                }
            }
        }
    }
    else
    {
        int         i            = 0;
        int         dist         = move;
        int         outside_dist = 0;
        ITUWidget * child        = NULL;
        ITUWidget * childPrepare = NULL;

        if (ITU_WHEEL_OUTSIDE_BOUNCE_DIST_FACTOR > 0)
        {
            child = (ITUWidget *)itcTreeGetChildAt(wheel, 0);
            if (ITU_WHEEL_OUTSIDE_BOUNCE_DIST_FACTOR == 1)
            {
                outside_dist = child->rect.height;
            }
            else
            {
                outside_dist = child->rect.height / 2;
            }
        }
        if (dist > 0)
        {
            childPrepare = (ITUWidget *)itcTreeGetChildAt(wheel, 0);
            if ((childPrepare->rect.y + dist) >= (wheel->inside + outside_dist))
            {
                dist = wheel->inside + outside_dist - childPrepare->rect.y;
            }
        }
        else
        {
            childPrepare = (ITUWidget *)itcTreeGetChildAt(wheel, count - 1);
            if ((childPrepare->rect.y + dist) <= (wheel->inside - outside_dist))
            {
                dist = wheel->inside - outside_dist - childPrepare->rect.y;
            }
        }
        for (i = 0; i < count; i++)
        {
            child = (ITUWidget *)itcTreeGetChildAt(wheel, i);
            child->rect.y += dist;
        }
    }
}

void WheelLayoutCal (ITUWidget * widget)
{
    ITUWheel *  wheel       = (ITUWheel *)widget;
    ITUWidget * child_check = NULL;
    int         baseSize    = 0;
    int         focusPosFix = 0;
    int         count       = 0;

    if (wheel)
    {
        count       = itcTreeGetChildCount((ITCTree *)widget);
        child_check = ituWheelGetChild(widget, 0);
        if (child_check)
        {
            baseSize = child_check->rect.height;
            if (ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV != 0)
            {
                focusPosFix = (wheel->focusFontHeight - wheel->fontHeight) / ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV;
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        return;
    }

    if (wheel->cycle_tor)
    {
        int fitc = (widget->rect.height / baseSize);
        int i, firstItemLeftShift, fy;

        if ((widget->rect.height % child_check->rect.height) > (child_check->rect.height / 2))
        {
            fitc++;
        }

        // widget->rect.height = (fitc * child_check->rect.height) + (fitc + 1);
        wheel->cycle_arr_count = fitc + (WHEEL_CYCLE_FIT_OUTSIDEBUFFER * 2);
        ituWheelCycleArrange(widget);

        // firstIndex is left 1/2 arr_count shift from focusIndex
        firstItemLeftShift = ((wheel->cycle_arr_count - 1) / 2);
        // first arr item position
        fy                 = ((widget->rect.height - baseSize) / 2) - (firstItemLeftShift * baseSize);

        // initial place to outside and check font index
        for (i = 0; i < count; i++)
        {
            ITUWidget * child = ituWheelGetChild(widget, i);
            ITUText *   text  = (ITUText *)child;

            if (child == NULL)
            {
                continue;
            }

            child->rect.y = -baseSize;

            if (text->fontIndex >= ITU_FREETYPE_MAX_FONTS)
            {
                text->fontIndex = 0;
            }
        }

        // set the cycle arr item position
        for (i = 0; i < wheel->cycle_arr_count; i++)
        {
            ITUWidget * child = ituWheelGetChild(widget, wheel->cycle_arr[i]);

            if (child)
            {
                child->rect.y = (wheel->cycle_arr[i] == wheel->focusIndex) ? (fy - focusPosFix) : (fy);
            }

            fy += baseSize;
        }
    }
    else
    {
        // first item position
        int i, fy = ((widget->rect.height - baseSize) / 2) - (wheel->focusIndex * baseSize);
        for (i = 0; i < count; i++)
        {
            ITUWidget * child = ituWheelGetChild(widget, i);
            ITUText *   text  = (ITUText *)child;
            if (child)
            {
                child->rect.y = (i == wheel->focusIndex) ? (fy - focusPosFix) : (fy);
                fy += baseSize;
            }
            if (text->fontIndex >= ITU_FREETYPE_MAX_FONTS)
            {
                text->fontIndex = 0;
            }
        }
    }
    WheelRunTimeSizeColorCal(wheel);
    return;
}

bool ituWheelUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool        result     = false;
    ITUWheel *  wheel      = (ITUWheel *)widget;

    ITUWidget * childFirst = ituWheelGetChild(widget, 0);
    int         itemCount  = itcTreeGetChildCount((ITCTree *)widget);
    int         itemSize   = ((childFirst != NULL) ? (childFirst->rect.height) : (widget->rect.height));
    int         widgetSize = widget->rect.height;
    int         good_center;
    int         fitc = (widget->rect.height / itemSize);

    if (childFirst == NULL)
    {
        return false;
    }

    if ((widget->rect.height % itemSize) > (itemSize / 2))
    {
        fitc++;
    }

    // fitmod = ((fitc % 3) > 0) ? (fitc / 2) : (0);
    good_center = (fitc / 2) + (fitc % 2);
    // widget->rect.height = (fitc * itemSize) + (fitc + 1);

    assert(wheel);

    if ((ev == ITU_EVENT_LAYOUT) || (ev == ITU_EVENT_LANGUAGE))
    {
        result |= ituFlowWindowUpdate(widget, ev, arg1, arg2, arg3);
    }

    if (ev == ITU_EVENT_LAYOUT)
    {
        // set custom data to original totalframe
        // ituWidgetSetCustomData(widget, wheel->totalframe);

        if (wheel->scal < 1) // for scal
        {
            wheel->scal = 1;
        }

        wheel->moving_step = 0; // useless now

        wheel->itemCount   = itemCount;

        if (wheel->inside > 0) // for inside check
        {
            wheel->inside = 0;
        }

        if ((wheel->slide_step <= 0) || (wheel->slide_step > 10)) // for wheel slide step speed
        {
            wheel->slide_step = 2;
        }

        if (wheel->idle > 0) // for state check
        {
            wheel->idle = 0;
        }

        WheelLayoutCal(widget);
    }

    // if (wheel->cycle_tor > 0)
    // if ((ev == ITU_EVENT_LAYOUT) || (ev == ITU_EVENT_LANGUAGE))
    //  result |= ituFlowWindowUpdate(widget, ev, arg1, arg2, arg3);

    if (widget->flags & ITU_TOUCHABLE)
    {
        if ((ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDEDOWN) && (wheel->slide == 1))
        {
            wheel->touchCount = 0;
            wheel->focus_dev  = 0;

            if (ituWidgetIsEnabled(widget) && !wheel->inc && !result)
            {
                int x = arg2 - widget->rect.x;
                int y = arg3 - widget->rect.y;

                if (((ituWidgetIsInside(widget, x, y)) || (wheel->inside > 0) || WHEEL_OUTSIDE_DRAGGABLE) &&
                    (widget->flags & ITU_DRAGGING))
                {
                    wheel->inside = 0;

                    // wheel->moving_step = 1;

                    if (ev == ITU_EVENT_TOUCHSLIDEUP)
                    {
                        int fmax = get_max_focusindex(widget);

                        if ((wheel->focusIndex <= fmax) || (wheel->cycle_tor > 0))
                        {
                            if (wheel->sliding)
                            {
                                wheel->frame = wheel->totalframe + 1;
                                // wheel->inc = 0;
                                // wheel->sliding = 0;
                                ituWidgetUpdate(wheel, ITU_EVENT_TIMER, 0, 0, 0);
                            }

                            if (wheel->inc == 0)
                            {
                                wheel->inc     = 0 - itemSize;
                                wheel->frame   = 0;
                                wheel->sliding = 1;

                                if ((arg1 >= ITU_WHEEL_MOTION_THRESHOLD) && (widget->flags & ITU_DRAGGABLE))
                                {
                                    wheel->slide_step = arg1;
                                    if (arg1 > (ITU_WHEEL_MOTION_THRESHOLD * 3 / 2))
                                    {
                                        wheel->totalframe = ITU_WHEEL_SLIDE_INIT_FRAME;

                                        wheel->slide_itemcount =
                                            (arg1 / ITU_WHEEL_MOTION_THRESHOLD) * ITU_WHEEL_STEP_SPEED_BASE;
                                        // (void)printf("[SLIDE 1st count %d]\n", wheel->slide_itemcount);
                                    }
                                    else
                                    {
                                        wheel->slide_itemcount = 0;
                                    }
                                }
                                else
                                {
                                    if (widget->flags & ITU_DRAGGABLE)
                                    {
                                        wheel->slide_step      = ITU_WHEEL_MOTION_THRESHOLD;
                                        wheel->slide_itemcount = 0;
                                    }
                                    else
                                    {
                                        wheel->sliding = 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (ev == ITU_EVENT_TOUCHSLIDEDOWN)
                    {
                        if ((wheel->focusIndex >= 0) || (wheel->cycle_tor > 0))
                        {
                            (void)printf("fc %d\n", wheel->focusIndex);
                            if (wheel->sliding)
                            {
                                wheel->frame = wheel->totalframe + 1;
                                // wheel->inc = 0;
                                // wheel->sliding = 0;
                                ituWidgetUpdate(wheel, ITU_EVENT_TIMER, 0, 0, 0);
                            }

                            if (wheel->inc == 0)
                            {
                                wheel->inc     = itemSize;
                                wheel->frame   = 0;
                                wheel->sliding = 1;

                                if ((arg1 >= ITU_WHEEL_MOTION_THRESHOLD) && (widget->flags & ITU_DRAGGABLE))
                                {
                                    wheel->slide_step = arg1;
                                    if (arg1 > (ITU_WHEEL_MOTION_THRESHOLD * 3 / 2))
                                    {
                                        wheel->totalframe = ITU_WHEEL_SLIDE_INIT_FRAME;

                                        wheel->slide_itemcount =
                                            (arg1 / ITU_WHEEL_MOTION_THRESHOLD) * ITU_WHEEL_STEP_SPEED_BASE;
                                        // (void)printf("[SLIDE 1st count %d]\n", wheel->slide_itemcount);
                                    }
                                    else
                                    {
                                        wheel->slide_itemcount = 0;
                                    }
                                }
                                else
                                {
                                    if (widget->flags & ITU_DRAGGABLE)
                                    {
                                        wheel->slide_step      = ITU_WHEEL_MOTION_THRESHOLD;
                                        wheel->slide_itemcount = 0;
                                    }
                                    else
                                    {
                                        wheel->sliding = 0;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        /* do nothing */
                    }
                    widget->flags &= ~ITU_DRAGGING;
                    // widget->flags &= ~ITU_UNDRAGGING;
                    result = true;
                }
                else
                {
                    wheel->frame = 0;
                    wheel->inc   = 0;
                    // result = true;
                }
            }
        }
        else if (ev == ITU_EVENT_MOUSEMOVE)
        {
            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGING))
            {
                int x                = arg2 - widget->rect.x;
                int y                = arg3 - widget->rect.y;
                int dragdiff         = y - wheel->drag_last_pos;
                wheel->drag_last_pos = y;
                wheel->idle          = 1;

                // for long drag check
                // if (abs(y - wheel->touchY) >= ITU_DRAG_DISTANCE)
                //  widget->flags |= ITU_LONG_DRAG;

                //[MouseMove][NON-Cycle]
                if (wheel->cycle_tor <= 0)
                {
                    if ((ituWidgetIsInside(widget, x, y)) || (WHEEL_OUTSIDE_DRAGGABLE))
                    {
                        int  fmax, offset;
                        bool dragging_bounce_check = false;

                        fmax                       = get_max_focusindex(widget);
                        offset                     = y - wheel->touchY;

                        if (wheel->scal > 0)
                        {
                            offset /= wheel->scal;
                        }

                        wheel->focus_dev = offset - ((wheel->fix_count * itemSize / wheel->drag_change_factor) *
                                                     ((offset < 0) ? (-1) : (1)));

                        if (ITU_WHEEL_OUTSIDE_BOUNCE_DIST_FACTOR > 0)
                        {
                            int ex_offset = offset;
                            if (ex_offset > 0)
                            {
                                ex_offset -= wheel->fix_count * itemSize;
                            }
                            else
                            {
                                ex_offset += wheel->fix_count * itemSize;
                            }
                            if (((ex_offset > 0) && (wheel->focusIndex == 0)) ||
                                ((ex_offset < 0) && (wheel->focusIndex == fmax)))
                            {
                                int checkDist = itemSize;
                                if (ITU_WHEEL_OUTSIDE_BOUNCE_DIST_FACTOR == 2)
                                {
                                    checkDist /= 2;
                                }
                                if (abs(ex_offset) <= checkDist)
                                {
                                    dragging_bounce_check = true;
                                }
                            }
                            // else
                            //  (void)printf("%d %d %d\n", wheel->focusIndex, wheel->fix_count, ex_offset);
                        }

                        if ((offset >= (itemSize / wheel->drag_change_factor)) ||
                            (offset <= (0 - (itemSize / wheel->drag_change_factor))) || dragging_bounce_check)
                        {
                            if (offset > 0)
                            {
                                int shift = offset / (itemSize / wheel->drag_change_factor);

                                if ((wheel->fix_count < shift) && (wheel->focusIndex > 0))
                                {
                                    wheel->fix_count++;
                                    wheel->focusIndex--;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][A][wheel->fix_count %d][shift %d]===\n", wheel->fix_count,
                                    // shift);
                                }
                                else if ((wheel->fix_count > shift) && (wheel->focusIndex < fmax))
                                {
                                    wheel->fix_count--;
                                    wheel->focusIndex++;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][B][wheel->fix_count %d][shift %d]===\n", wheel->fix_count,
                                    // shift);
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                            else
                            {
                                int shift = (-1) * offset / (itemSize / wheel->drag_change_factor);

                                if ((wheel->fix_count < shift) && (wheel->focusIndex < fmax))
                                {
                                    wheel->fix_count++;
                                    wheel->focusIndex++;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][C][wheel->fix_count %d][shift %d]===\n", wheel->fix_count,
                                    // shift);
                                }
                                else if ((wheel->fix_count > shift) && (wheel->focusIndex > 0))
                                {
                                    wheel->fix_count--;
                                    wheel->focusIndex--;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][D][wheel->fix_count %d][shift %d]===\n", wheel->fix_count,
                                    // shift);
                                }
                                else
                                {
                                    /* do nothing */
                                }
                            }
                        }
                        else
                        {
                            if (offset > 0)
                            {
                                if ((wheel->fix_count) && (wheel->focusIndex < fmax))
                                {
                                    wheel->fix_count--;
                                    wheel->focusIndex++;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][E][wheel->fix_count %d]===\n", wheel->fix_count);
                                }
                            }
                            else
                            {
                                if ((wheel->fix_count) && (wheel->focusIndex > 0))
                                {
                                    wheel->fix_count--;
                                    wheel->focusIndex--;

                                    ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                                    // (void)printf("===[wheel][F][wheel->fix_count %d]===\n", wheel->fix_count);
                                }
                            }
                        }

                        if (((offset > 0) && (wheel->focusIndex > 0)) || ((offset < 0) && (wheel->focusIndex < fmax)) ||
                            dragging_bounce_check)
                        {
                            int i = 0;
                            for (; i < itemCount; ++i)
                            {
                                int         fy;
                                ITUWidget * child = ituWheelGetChild(widget, i);
                                fy                = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                                fy += i * child->rect.height;
                                fy += offset;
                                fy += (wheel->fix_count * child->rect.height / wheel->drag_change_factor) *
                                      ((offset > 0) ? (-1) : (1));

                                ituWidgetSetY(child, fy);
                            }
                            WheelRunTimeSizeColorCal(wheel);
                        }

                        result = true;
                    }
                }
                else //[MouseMove][Cycle]
                {
                    if (ituWidgetIsInside(widget, x, y))
                    {
                        ITUWidget * childFocus = ituWheelGetChild(widget, wheel->focusIndex);
                        int         newFocus   = wheel->focusIndex;
                        int         offset     = y - wheel->touchY;

                        if (wheel->scal > 0)
                        {
                            offset /= wheel->scal;
                        }

                        // WheelRuntimePosMove(wheel, dragdiff);

                        if (abs(offset) >= (itemSize / wheel->drag_change_factor))
                        {
                            if (offset > 0)
                            {
                                wheel->shift_one--;
                                newFocus--;
                            }
                            else
                            {
                                wheel->shift_one++;
                                newFocus++;
                            }
                            ituWheelFocusChange(widget, newFocus, __LINE__);
                            WheelLayoutCal(widget);
                            wheel->touchY = y;
                        }
                        else if (abs(offset) > 0)
                        {
                            WheelRuntimePosMove(wheel, dragdiff);
                        }
                        else
                        {
                            /* do nothing */
                        }

                        wheel->focus_dev = childFocus->rect.y - ((widget->rect.height - itemSize) / 2);
                        result           = true;
                        WheelRunTimeSizeColorCal(wheel);
                    }
                }
            }
        }
        else if (ev == ITU_EVENT_MOUSEDOWN)
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;
            // (void)printf("[x,y,touchy] [%d,%d,%d]\n",x ,y , wheel->touchY);

            widget->flags &= ~ITU_LONG_DRAG;

            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGABLE) && !result)
            {
                if (ituWidgetIsInside(widget, x, y))
                {
                    wheel->fix_count = 0;
                    wheel->touchY    = y;
                    widget->flags |= ITU_DRAGGING;
                    ituScene->dragged    = widget;
                    result               = true;
                    wheel->drag_last_pos = y;
                    wheel->shift_one     = 0;
                }
            }

            if (ituWidgetIsEnabled(widget) && ituWidgetIsInside(widget, x, y))
            {
                // force to reset frame and total frame to default
                wheel->totalframe = wheel->org_totalframe;

                // wheel->frame = wheel->totalframe;
                // try fix continue update timer draw
                wheel->frame      = 0;

                wheel->inc        = 0;
                // if (wheel->sliding == 1)
                //{
                //  wheel->idle = 1;
                // }
                // wheel->idle = 1;
            }
            else
            {
                // Force stop wheel sliding
                if (wheel->sliding && wheel->frame)
                {
                    wheel->frame   = 0;
                    wheel->inc     = 0;
                    wheel->sliding = 0;
                    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
                    result = true;
                    return result;
                }
            }
        }
        else if (ev == ITU_EVENT_MOUSEUP)
        {
            if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_DRAGGABLE) && (widget->flags & ITU_DRAGGING))
            {
                int  fmax            = get_max_focusindex(widget);
                int  x               = arg2 - widget->rect.x;
                int  y               = arg3 - widget->rect.y;
                bool soft_click      = false;
                wheel->sliding       = 0;
                wheel->drag_last_pos = y;

                // for long drag check
                if ((abs(y - wheel->touchY) >= ITU_DRAG_DISTANCE) && (ituWidgetIsInside(widget, x, y)))
                {
                    widget->flags |= ITU_LONG_DRAG;
                }
                else if (wheel->shift_one != 0)
                {
                    widget->flags |= ITU_LONG_DRAG;
                }
                else
                {
                    /* do nothing */
                }

                //[MouseUP][NON-Cycle]
                if ((itemCount > 0) && (wheel->cycle_tor <= 0)) //
                {
                    int offset, absoffset, interval;
                    offset   = y - wheel->touchY;

                    /*if (!ituWidgetIsInside(widget, x, y))
                    {
                    if (offset > 0)
                    offset = widget->rect.height - wheel->touchY;
                    else
                    offset = 0 - wheel->touchY;
                    }*/

                    interval = offset / itemSize;
                    offset -= (interval * itemSize);
                    absoffset        = offset > 0 ? offset : -offset;
                    // wheel->focus_dev = offset - ((wheel->fix_count * child->rect.height / wheel->drag_change_factor)
                    // * ((offset < 0) ? (-1) : (1)));
                    wheel->focus_dev = 0;

                    // fix for non-cycle drag to limit (2018/5/24)
                    if (((offset > 0) && (wheel->focusIndex == 0)) || ((offset < 0) && (wheel->focusIndex == fmax)))
                    {
                        // (void)printf("non-cycle limit outside.\n");
                        offset = absoffset = 0;
                    }

                    if ((wheel->inc == 0) && (wheel->fix_count == 0))
                    {
                        // fix wrong interval when drag outside
                        if (!ituWidgetIsInside(widget, x, y))
                        {
                            interval = 0;
                        }

                        if (absoffset > itemSize / wheel->mouseup_change_factor)
                        {
                            int div_value = itemSize / wheel->totalframe;
                            div_value     = (div_value == 0) ? (1) : (div_value);
                            wheel->frame  = absoffset / div_value + 1;

                            if (offset > 0)
                            {
                                wheel->inc = itemSize;
                                wheel->focusIndex -= interval;
                                // (void)printf("===[wheel][1][frame %d][offset %d][inc %d][interval %d][focusIndex %d]===\n",
                                // wheel->frame, offset, wheel->inc, interval, wheel->focusIndex);
                            }
                            else if (offset < 0)
                            {
                                wheel->inc = -itemSize;
                                wheel->focusIndex -= interval;
                                // (void)printf("===[wheel][2][frame %d][offset %d][inc %d][interval %d][focusIndex %d]===\n",
                                // wheel->frame, offset, wheel->inc, interval, wheel->focusIndex);
                            }
                            else
                            {
                                /* do nothing */
                            }

                            if (!ituWidgetIsInside(widget, x, y))
                            {
                                wheel->inc = 0;
                            }
                        }
                        else
                        {
                            int div_value         = itemSize / wheel->totalframe;
                            div_value             = (div_value == 0) ? (1) : (div_value);
                            wheel->frame          = wheel->totalframe - absoffset / div_value;
                            wheel->org_totalframe = wheel->totalframe;

                            if (offset > 0)
                            {
                                wheel->inc = -itemSize;
                                wheel->focusIndex -= interval; // +1;

                                wheel->frame = wheel->totalframe;

                                if (interval == 0)
                                {
                                    wheel->inc = 0;
                                }
                                // (void)printf("===[wheel][3][frame %d][offset %d][inc %d][interval %d][focusIndex %d]===\n",
                                // wheel->frame, offset, wheel->inc, interval, wheel->focusIndex);
                            }
                            else if (offset < 0)
                            {
                                wheel->inc = itemSize;
                                wheel->focusIndex -= interval; // -1;

                                wheel->frame = wheel->totalframe;

                                if (interval == 0)
                                {
                                    wheel->inc = 0;
                                }
                                // (void)printf("===[wheel][4][frame %d][offset %d][inc %d][interval %d][focusIndex %d]===\n",
                                // wheel->frame, offset, wheel->inc, interval, wheel->focusIndex);
                            }
                            else
                            {
                                /* do nothing */
                            }

                            if (!(widget->flags & ITU_LONG_DRAG))
                            {
                                soft_click = true;
                            }
                        }

                        if (wheel->inc > 0)
                        {
                            if (wheel->focusIndex <= 0)
                            {
                                wheel->focusIndex = 0;
                                wheel->frame      = wheel->totalframe;
                                wheel->inc        = 0;
                                // (void)printf("===[wheel][5]\n");
                            }
                            // else
                            //{
                            //  if (wheel->focusIndex >= fmax)
                            //  {
                            //      wheel->focusIndex = fmax;
                            //      wheel->frame = wheel->totalframe;
                            //      wheel->inc = 0;
                            //      //(void)printf("===[wheel][6]\n");
                            //  }
                            // }
                        }
                        else // if (wheel->inc < 0)
                        {
                            // if (wheel->focusIndex <= 0)
                            //{
                            //  wheel->focusIndex = 0;
                            //  wheel->frame = wheel->totalframe;
                            //  wheel->inc = 0;
                            //  //(void)printf("===[wheel][7]\n");
                            // }
                            // else
                            //{
                            if (wheel->focusIndex >= fmax)
                            {
                                wheel->focusIndex = fmax;
                                wheel->frame      = wheel->totalframe;
                                wheel->inc        = 0;
                                // (void)printf("===[wheel][8]\n");
                            }
                            //}

                            /*
                            if (wheel->idle == 0)
                            {
                            if ((absoffset <= 1) && (ituWidgetIsVisible(widget)))
                            {
                            //ituExecActions(widget, wheel->actions, ITU_EVENT_CUSTOM, 0);
                            wheel->idle = 10;
                            }
                            }
                            else
                            {
                            wheel->idle = 0;
                            }*/
                        }
                        widget->flags |= ITU_UNDRAGGING;
                    }
                    else if (wheel->fix_count)
                    {
                        wheel->frame     = wheel->totalframe;
                        wheel->inc       = 0;
                        wheel->fix_count = 0;
                        widget->flags |= ITU_UNDRAGGING;
                    }
                    else
                    {
                        widget->flags |= ITU_UNDRAGGING;
                        wheel->frame = wheel->totalframe;
                        widget->flags &= ~ITU_DRAGGING;
                    }

                    if (soft_click)
                    {
                        int goal = wheel->focusIndex;
                        int c_top;
                        int c_bot;

                        c_top = (widget->rect.height - itemSize) / 2;
                        c_bot = c_top + itemSize;

                        if (wheel->touchY < (c_top - 2))
                        {
                            int id, click_shift = ((c_top - wheel->touchY) / itemSize) + 1;

                            for (id = 0; id < click_shift; id++)
                            {
                                if (goal > 0)
                                {
                                    goal--;
                                }
                                else
                                {
                                    if (wheel->cycle_tor)
                                    {
                                        goal = get_max_focusindex(widget);
                                    }
                                }
                            }

                            wheel->inc   = 0;
                            wheel->frame = 0;
                            widget->flags &= ~ITU_UNDRAGGING;
                            wheel->focus_dev = 0;
                            ituWheelGoto(wheel, goal);
                        }
                        else if (wheel->touchY > (c_bot + 2))
                        {
                            int id, click_shift = ((wheel->touchY - c_bot) / itemSize) + 1;

                            for (id = 0; id < click_shift; id++)
                            {
                                if (goal < get_max_focusindex(widget))
                                {
                                    goal++;
                                }
                                else
                                {
                                    if (wheel->cycle_tor)
                                    {
                                        goal = 0;
                                    }
                                }
                            }

                            wheel->inc   = 0;
                            wheel->frame = 0;
                            widget->flags &= ~ITU_UNDRAGGING;
                            wheel->focus_dev = 0;
                            ituWheelGoto(wheel, goal);
                        }
                        else
                        {
                            /* do nothing */
                        }

                        // (void)printf("[soft click][goal %d]\n", goal);
                    }
                    else
                    {
                        wheel->idle = 0;
                        if ((wheel->focusIndex >= 0) && (wheel->focusIndex <= fmax))
                        {
                            ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                        }
                    }
                }
                else if ((itemCount > 0) && wheel->cycle_tor) //[MouseUP][Cycle]
                {
                    widget->flags |= ITU_UNDRAGGING;
                    widget->flags &= ~ITU_DRAGGING;

                    if (wheel->inc == 0)
                    {
                        int offset, absoffset;

                        offset       = y - wheel->touchY;
                        absoffset    = offset > 0 ? offset : -offset;

                        soft_click   = false;

                        wheel->frame = 0;

                        if (absoffset > itemSize / wheel->mouseup_change_factor)
                        {
                            int         newFocus   = wheel->focusIndex;
                            ITUWidget * childFocus = NULL;

                            if (offset > 0)
                            {
                                newFocus--;
                            }
                            else
                            {
                                newFocus++;
                            }

                            ituWheelFocusChange(widget, newFocus, __LINE__);

                            childFocus       = ituWheelGetChild(widget, wheel->focusIndex);

                            wheel->focus_dev = childFocus->rect.y - ((widget->rect.height - itemSize) / 2);

                            WheelRunTimeSizeColorCal(wheel);
                        }
                        else
                        {
                            if (!(widget->flags & ITU_LONG_DRAG))
                            {
                                soft_click = true;
                            }
                        }

                        // default mouseup trigger goto
                        if (soft_click)
                        {
                            int goal = wheel->focusIndex;
                            int c_top;
                            int c_bot;

                            c_top = (widget->rect.height - itemSize) / 2;
                            c_bot = c_top + itemSize;

                            if (wheel->touchY < (c_top - 2))
                            {
                                int id, click_shift = ((c_top - wheel->touchY) / itemSize) + 1;

                                for (id = 0; id < click_shift; id++)
                                {
                                    if (goal > 0)
                                    {
                                        goal--;
                                    }
                                    else
                                    {
                                        if (wheel->cycle_tor)
                                        {
                                            goal = get_max_focusindex(widget);
                                        }
                                    }
                                }
                                wheel->inc   = 0;
                                wheel->frame = 0;
                                widget->flags &= ~ITU_UNDRAGGING;
                                wheel->focus_dev = 0;
                                ituWheelGoto(wheel, goal);
                            }
                            else if (wheel->touchY > (c_bot + 2))
                            {
                                int id, click_shift = ((wheel->touchY - c_bot) / itemSize) + 1;

                                for (id = 0; id < click_shift; id++)
                                {
                                    if (goal < get_max_focusindex(widget))
                                    {
                                        goal++;
                                    }
                                    else
                                    {
                                        if (wheel->cycle_tor)
                                        {
                                            goal = 0;
                                        }
                                    }
                                }
                                wheel->inc   = 0;
                                wheel->frame = 0;
                                widget->flags &= ~ITU_UNDRAGGING;
                                wheel->focus_dev = 0;
                                ituWheelGoto(wheel, goal);
                            }
                            else
                            {
                                wheel->inc   = 0;
                                wheel->frame = 0;
                                widget->flags &= ~ITU_UNDRAGGING;
                                wheel->focus_dev = 0;
                                WheelLayoutCal(widget);
                            }
                            // (void)printf("[soft click]\n");
                        }
                    }
                    else
                    {
                        wheel->frame = wheel->totalframe;
                    }
                }
                else
                {
                    /* do nothing */
                }
                result = true;
            }
            else
            {
                // Force stop wheel sliding
                if (wheel->sliding && wheel->frame)
                {
                    wheel->frame   = 0;
                    wheel->inc     = 0;
                    wheel->sliding = 0;
                    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
                    result = true;
                }
            }

            widget->flags &= ~ITU_DRAGGING;
            wheel->touchCount = 0;
            widget->flags &= ~ITU_LONG_DRAG;
        }
        else
        {
            /* do nothing */
        }
    }

    if (ev == ITU_EVENT_TIMER)
    {
        if ((wheel->sliding) || (wheel->frame == wheel->totalframe))
        {
            result = true;
        }

        // fix for some case make slide_step too small and wheel will not stop
        if (wheel->sliding && ((wheel->slide_step < ITU_WHEEL_MOTION_THRESHOLD) || (wheel->slide == 0)))
        {
            wheel->sliding = 0;
            return result;
        }

        // try to fix idle check error
        if ((wheel->idle == 1) && (wheel->inc == 0) && (wheel->frame == 0) && (wheel->focus_dev == 0))
        {
            if (!(widget->flags & ITU_DRAGGING))
            {
                wheel->idle       = 0;
                wheel->totalframe = wheel->org_totalframe;
            }
        }

        // use inc and temp1 to do goto animation
        if ((wheel->inc != 0) && (wheel->temp1 == 1) && (wheel->sliding == 0) && (!(widget->flags & ITU_UNDRAGGING)))
        {
            wheel->frame++;

            if (wheel->frame > wheel->totalframe)
            {
                wheel->frame = 0;
                wheel->inc   = 0;
                ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
            }
            else
            {
                int i   = 0;
                int dev = wheel->inc / wheel->totalframe;

                if (wheel->cycle_tor)
                {
                    int         id         = 0;
                    ITUWidget * child      = NULL;
                    ITUWidget * childFocus = ituWheelGetChild(widget, wheel->focusIndex);
                    int * cycleOut = (int *)malloc(sizeof(int) * itemCount); // used to memo child modified or not
                    int   posVar   = 0;
                    (void)memset(cycleOut, 1, sizeof(int) * itemCount);
                    ituWidgetSetY(childFocus, childFocus->rect.y + dev);
                    cycleOut[wheel->focusIndex] = 0;
                    posVar                      = ituWidgetGetY(childFocus); // init posVar to focus child position
                    id                          = wheel->focusIndex;         // init id to focusIndex
                    for (i = 1; i < itemCount; i++)
                    {
                        id++; // next index
                        if (id >= itemCount)
                        {
                            id = 0;
                        }
                        posVar += itemSize;
                        if (posVar >= widgetSize)
                        {
                            break;
                        }
                        else
                        {
                            child        = ituWheelGetChild(widget, id);
                            cycleOut[id] = 0;
                            ituWidgetSetY(child, posVar);
                        }
                    }
                    posVar = ituWidgetGetY(childFocus); // init posVar to focus child position
                    id     = wheel->focusIndex;         // init id to focusIndex
                    for (i = 1; i < itemCount; i++)
                    {
                        id--; // prev index
                        if (id < 0)
                        {
                            id = itemCount - 1;
                        }
                        posVar -= itemSize;
                        if (posVar <= (0 - itemSize))
                        {
                            break;
                        }
                        else
                        {
                            child        = ituWheelGetChild(widget, id);
                            cycleOut[id] = 0;
                            ituWidgetSetY(child, posVar);
                        }
                    }
                    // move all outside child to outside
                    for (i = 0; i < itemCount; i++)
                    {
                        if (cycleOut[i])
                        {
                            child = ituWheelGetChild(widget, i);
                            ituWidgetSetY(child, widgetSize);
                        }
                    }
                    free(cycleOut);
                }
                else
                {
                    int         posVar     = 0;
                    ITUWidget * childFocus = ituWheelGetChild(widget, wheel->focusIndex);
                    ituWidgetSetY(childFocus, childFocus->rect.y + dev);
                    posVar = ituWidgetGetY(childFocus) - (wheel->focusIndex * itemSize);
                    for (i = 0; i < itemCount; ++i)
                    {
                        ITUWidget * child = ituWheelGetChild(widget, i);
                        ituWidgetSetY(child, posVar);
                        posVar += itemSize;
                    }
                }
            }
            return true;
        }

        //[Timer][NON-Cycle]
        if (wheel->cycle_tor <= 0)
        {
            int fmax = get_max_focusindex(widget);

            if (0) //(wheel->focus_dev)
            {
                if (widget->flags & ITU_UNDRAGGING)
                {
                    (void)printf("UNDragging!\n");
                }

                if (wheel->sliding)
                {
                    (void)printf("sliding!\n");
                }

                if (widget->flags & ITU_DRAGGING)
                {
                    (void)printf("Dragging!\n");
                }

                (void)printf("inc %d frame %d\n", wheel->inc, wheel->frame);
            }

            if ((wheel->focus_dev) && ((!(widget->flags & ITU_UNDRAGGING)) && (wheel->sliding == 0)) &&
                (!(widget->flags & ITU_DRAGGING)))
            {
                int i  = 0;

                result = true;

                for (i = 0; i < itemCount; ++i)
                {
                    int         fy;
                    ITUWidget * child = ituWheelGetChild(widget, i);
                    fy                = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                    fy += i * itemSize;
                    fy -= wheel->focus_dev * itemSize * wheel->frame / wheel->totalframe;

                    if (((wheel->focus_dev < 0) && (wheel->focusIndex == 0)) ||
                        ((wheel->focus_dev > 0) && (wheel->focusIndex == fmax)))
                    {
                        wheel->frame = wheel->totalframe;
                        break;
                    }

                    ituWidgetSetY(child, fy);
                }

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    bool fi_changed = true;

                    if ((wheel->focus_dev < 0) && (wheel->focusIndex > 0))
                    {
                        wheel->focusIndex--;
                    }
                    else if ((wheel->focus_dev > 0) && (wheel->focusIndex < fmax))
                    {
                        wheel->focusIndex++;
                    }
                    else
                    {
                        fi_changed = false;
                    }

                    wheel->frame     = 0;
                    wheel->focus_dev = 0;

                    if (fi_changed)
                    {
                        ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
                        ituExecActions((ITUWidget *)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                }
                else
                {
                    WheelRunTimeSizeColorCal(wheel);
                }
            }
            else if ((wheel->focus_dev) && ((widget->flags & ITU_UNDRAGGING) && (wheel->sliding == 0)))
            {
                int i = 0;

                for (i = 0; i < itemCount; ++i)
                {
                    int         fy;
                    ITUWidget * child = ituWheelGetChild(widget, i);
                    fy                = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                    fy += i * itemSize;
                    fy += wheel->focus_dev * wheel->frame / wheel->totalframe;

                    ituWidgetSetY(child, fy);
                }
                // (void)printf("[TIMER][frame %d]\n", wheel->frame);
                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    bool fi_changed = true;

                    if ((wheel->inc > 0) && (wheel->focusIndex > 0))
                    {
                        wheel->focusIndex--;
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex < fmax))
                    {
                        wheel->focusIndex++;
                    }
                    else
                    {
                        fi_changed = false;
                    }

                    wheel->frame = 0;

                    if (wheel->sliding == 0)
                    {
                        wheel->inc  = 0;
                        wheel->idle = 0;
                    }
                    else
                    {
                        wheel->sliding = 0;
                    }

                    widget->flags &= ~ITU_UNDRAGGING;

                    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

                    wheel->focus_dev = 0;

                    if (fi_changed)
                    {
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                }
                WheelRunTimeSizeColorCal(wheel);
            }
            else if ((widget->flags & ITU_UNDRAGGING) && (wheel->sliding == 0))
            {
                int i = 0;

                // fix for empty item under non-cycle mode due to wrong focusindex
                if (wheel->focusIndex < 0)
                {
                    wheel->focusIndex = 0;
                }
                else if (wheel->focusIndex > fmax)
                {
                    wheel->focusIndex = fmax;
                }
                else
                {
                    /* do nothing */
                }

                for (i = 0; i < itemCount; ++i)
                {
                    int         fy;
                    ITUWidget * child = ituWheelGetChild(widget, i);
                    fy                = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                    fy += i * itemSize;
                    fy += wheel->inc * wheel->frame / wheel->totalframe;

                    ituWidgetSetY(child, fy);
                }

                // if (wheel->sliding == 0)
                //  wheel->frame = wheel->totalframe;

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    bool fi_changed = true;

                    if ((wheel->inc > 0) && (wheel->focusIndex > 0))
                    {
                        wheel->focusIndex--;
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex < fmax))
                    {
                        wheel->focusIndex++;
                    }
                    else
                    {
                        fi_changed = false;
                    }

                    wheel->frame = 0;

                    if (wheel->sliding == 0)
                    {
                        wheel->inc  = 0;
                        wheel->idle = 0;
                    }
                    else
                    {
                        wheel->sliding = 0;
                    }

                    widget->flags &= ~ITU_UNDRAGGING;
                    wheel->focus_dev = 0;

                    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

                    // fix for focusindex outside the non - cycle sometime
                    if (wheel->focusIndex < 0)
                    {
                        wheel->focusIndex = 0;
                    }
                    else if (wheel->focusIndex > fmax)
                    {
                        wheel->focusIndex = fmax;
                    }
                    else
                    {
                        /* do nothing */
                    }

                    if (fi_changed)
                    {
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    // value = ituTextGetString(text);
                    // ituWheelOnValueChanged(wheel, value);
                }
                WheelRunTimeSizeColorCal(wheel);

            } // non-cycle mode sliding
            else if ((wheel->inc) && (wheel->sliding) && (wheel->focusIndex != 0) && (wheel->focusIndex != fmax))
            {
                int i, fy;

                for (i = 0; i < itemCount; ++i)
                {
                    int         fac;
                    ITUWidget * child = ituWheelGetChild(widget, i);

                    if (i == 0)
                    {
                        fy = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                    }

                    ituWidgetSetY(child, fy + wheel->inc * wheel->frame / wheel->totalframe);
                    fac = fy + (wheel->inc * wheel->frame / wheel->totalframe);
                    ituWidgetSetY(child, fac);

                    // child->rect.height = itemSize;
                    fy += itemSize;
                }

                // wheel->moving_step += 1;

                // if (wheel->moving_step > xx)
                //{
                if (widget->flags & ITU_DRAGGABLE)
                {
                    if ((wheel->inc > 0) && (wheel->focusIndex == 1))
                    {
                        wheel->frame = wheel->totalframe;
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex == (fmax - 1)))
                    {
                        wheel->frame = wheel->totalframe;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                // wheel->moving_step = 1;

                // if (wheel->shift_one) // && (widget->flags & ITU_DRAGGABLE))
                //{
                //  wheel->frame = wheel->totalframe;
                //  wheel->shift_one = 0;
                //  wheel->sliding = 0;
                // }
                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    if ((wheel->inc > 0) && (wheel->focusIndex > 0))
                    {
                        if ((!((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0))) ||
                            (wheel->focusIndex == 1))
                        {
                            wheel->idle = 0;
                        }
                        wheel->focusIndex--;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex < fmax))
                    {
                        if ((!((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0))) ||
                            (wheel->focusIndex == (fmax - 1)))
                        {
                            wheel->idle = 0;
                        }
                        wheel->focusIndex++;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else
                    {
                        /* do nothing */
                    }

                    // fix bug the sliding can not go to end when touch boundary early
                    if (wheel->sliding)
                    {
                        if (((wheel->inc > 0) && (wheel->focusIndex <= 0)) ||
                            ((wheel->inc < 0) && (wheel->focusIndex >= fmax)))
                        {
                            wheel->totalframe = wheel->org_totalframe;
                            wheel->sliding    = 0;
                        }
                    }

                    wheel->frame = 0;

                    ////////////////////////////////
                    // Force feedback algorithm
                    if ((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0))
                    {
                        int    y1;
                        float  progress_range;
                        double pow_b;
                        double fix_mod = ((double)wheel->slide_step / ITU_WHEEL_MOTION_THRESHOLD);

                        if (fix_mod > ITU_WHEEL_MAX_INIT_POWER)
                        {
                            fix_mod = ITU_WHEEL_MAX_INIT_POWER;
                        }

                        pow_b = 5.0 * ITU_WHEEL_MAX_INIT_POWER / fix_mod;

                        y1 = (int)(pow(pow_b, (double)wheel->totalframe / (ITU_WHEEL_MOTION_FACTOR / (5.0 - fix_mod))));

                        if (y1 > ITU_WHEEL_MOTION_FACTOR)
                        {
                            y1 = ITU_WHEEL_MOTION_FACTOR;
                        }

                        (void)printf("[ydiff %d] [pow_b %.3f] [totalframe %d]\n", wheel->slide_step, pow_b,
                               wheel->totalframe);

                        progress_range = (float)wheel->totalframe / ITU_WHEEL_MOTION_FACTOR;

                        if (wheel->slide_itemcount)
                        {
                            wheel->slide_itemcount--;
                        }
                        else
                        {
                            if (progress_range <= ITU_WHEEL_PROCESS_STAGE1)
                            {
                                wheel->totalframe += y1;
                            }
                            else if (progress_range <= ITU_WHEEL_PROCESS_STAGE2)
                            {
                                wheel->totalframe += y1 * 3;
                            }
                            else
                            {
                                wheel->totalframe += y1 * (int)fix_mod;
                            }
                        }

                        if (wheel->totalframe > 40)
                        {
                            wheel->totalframe = ITU_WHEEL_MOTION_FACTOR;
                        }
                    }
                    else
                    {
                        wheel->inc = 0;
                        widget->flags &= ~ITU_UNDRAGGING;

                        // wheel->moving_step = 0;

                        wheel->totalframe = wheel->org_totalframe;
                        wheel->focus_dev  = 0;

                        if (wheel->sliding == 1)
                        {
                            wheel->sliding = 0;
                        }

                        ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

                        // check here (fix the wrong position when slide stop by itself)
                        widget->dirty = true;
                        result |= widget->dirty;
                    }
                    WheelRunTimeSizeColorCal(wheel);
                }
            }
            else if (wheel->inc) // non-cycle mode bump boundary
            {
                int i, fy;

                for (i = 0; i < itemCount; ++i)
                {
                    ITUWidget * child = ituWheelGetChild(widget, i);

                    if (i == 0)
                    {
                        fy = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                    }

                    ituWidgetSetY(child, fy + wheel->inc * wheel->frame / wheel->totalframe);

                    // child->rect.height = itemSize;

                    fy += itemSize;
                }
                if (widget->flags & ITU_DRAGGABLE)
                {
                    if ((wheel->inc > 0) && (wheel->focusIndex == 1))
                    {
                        wheel->frame = wheel->totalframe;
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex == (fmax - 1)))
                    {
                        wheel->frame = wheel->totalframe;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                /*if ((wheel->shift_one) && (widget->flags & ITU_DRAGGABLE))
                {
                wheel->frame = wheel->totalframe;
                wheel->shift_one = 0;
                }*/

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    if ((wheel->inc > 0) && (wheel->focusIndex > 0))
                    {
                        wheel->idle = 0;
                        wheel->focusIndex--;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex < fmax))
                    {
                        wheel->idle = 0;
                        wheel->focusIndex++;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else
                    {
                        /* do nothing */
                    }

                    wheel->frame = 0;
                    wheel->inc   = 0;
                    widget->flags &= ~ITU_UNDRAGGING;

                    if (wheel->sliding == 1)
                    {
                        wheel->sliding = 0;
                    }

                    for (i = 0; i < itemCount; ++i)
                    {
                        ITUWidget * child = ituWheelGetChild(widget, i);

                        if (i == 0)
                        {
                            fy = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                        }

                        ituWidgetSetY(child, fy);

                        // child->rect.height = itemSize;

                        fy += itemSize;
                    }
                }
                else if (widget->flags & ITU_DRAGGABLE)
                {
                    if ((wheel->inc > 0) && (wheel->focusIndex > 0))
                    {
                        wheel->focusIndex--;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else if ((wheel->inc < 0) && (wheel->focusIndex < fmax))
                    {
                        wheel->focusIndex++;
                        ituExecActions(widget, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
                    }
                    else
                    {
                        /* do nothing */
                    }

                    for (i = 0; i < itemCount; ++i)
                    {
                        ITUWidget * child = ituWheelGetChild(widget, i);

                        if (i == 0)
                        {
                            fy = 0 - ((wheel->focusIndex - good_center + 1) * itemSize);
                        }

                        ituWidgetSetY(child, fy);

                        // child->rect.height = itemSize;

                        fy += itemSize;
                    }
                }
                else
                {
                    /* do nothing */
                }
                WheelRunTimeSizeColorCal(wheel);
            }
            else
            {
                if ((wheel->focusIndex == 0) || (wheel->focusIndex == fmax))
                {
                    wheel->frame = 0;
                    wheel->inc   = 0;
                    widget->flags &= ~ITU_UNDRAGGING;
                }
            }
        }
        else //[Timer][Cycle]
        {
            //[Timer][Cycle][MouseUP][Animation]
            if ((widget->flags & ITU_UNDRAGGING) && (wheel->sliding == 0) && (!(widget->flags & ITU_DRAGGING)))
            {
                int         i     = 0;
                ITUWidget * child = ituWheelGetChild(widget, wheel->cycle_arr[0]);
                if (wheel->frame == 0)
                {
                    wheel->tempy = child->rect.y;
                }

                for (; i < wheel->cycle_arr_count; i++)
                {
                    int fy;

                    child = ituWheelGetChild(widget, wheel->cycle_arr[i]);

                    if (i == 0)
                    {
                        if (wheel->frame == 0)
                        {
                            wheel->tempy = child->rect.y;
                        }
                    }

                    fy = wheel->tempy - (wheel->focus_dev * 1 * wheel->frame / wheel->totalframe) + (i * itemSize);
                    if (wheel->cycle_arr[i] == wheel->focusIndex)
                    {
                        fy -= (wheel->focusFontHeight - wheel->fontHeight) / ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV;
                    }

                    ituWidgetSetY(child, fy);
                }

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    wheel->frame     = 0;
                    wheel->focus_dev = 0;
                    widget->flags &= ~ITU_UNDRAGGING;
                    WheelLayoutCal(widget);
                }
                else
                {
                    WheelRunTimeSizeColorCal(wheel);
                }

                return true;
            }
            else if (wheel->inc) //[Cycle][Sliding]
            {
                int i, fy;

                // refix_wheel_layout(wheel);
                ituWheelCycleArrange(widget);

                for (i = 0; i < wheel->cycle_arr_count; ++i)
                {
                    ITUWidget * child;

                    if (i == 0)
                    {
                        fy = 0 - itemSize * WHEEL_CYCLE_FIT_OUTSIDEBUFFER;
                    }

                    child = ituWheelGetChild(widget, wheel->cycle_arr[i]);

                    if (wheel->cycle_arr[i] == wheel->focus_c)
                    {
                        ituWidgetSetY(child, fy + (wheel->inc * wheel->frame / wheel->totalframe) -
                                                 ((wheel->focusFontHeight - wheel->fontHeight) /
                                                  ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV));
                    }
                    else
                    {
                        ituWidgetSetY(child, fy + wheel->inc * wheel->frame / wheel->totalframe);
                    }
                    fy += itemSize;
                }

                /*if ((wheel->shift_one) && (widget->flags & ITU_DRAGGABLE))
                {
                wheel->frame = wheel->totalframe;
                wheel->shift_one = 0;
                }*/

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    if (wheel->inc > 0)
                    {
                        // newfocus = wheel->focus_c;

                        ituWheelFocusChange(widget, wheel->focusIndex - 1, __LINE__);
                        if (!((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0)))
                        {
                            wheel->idle = 0;
                        }
                    }
                    else if (wheel->inc < 0)
                    {
                        // newfocus = wheel->focus_c;

                        ituWheelFocusChange(widget, wheel->focusIndex + 1, __LINE__);
                        if (!((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0)))
                        {
                            wheel->idle = 0;
                        }
                    }
                    else
                    {
                        /* do nothing */
                    }

                    wheel->frame = 0;

                    ///////////////////////////////////
                    // Force feedback algorithm cycle-mode
                    if ((wheel->totalframe < ITU_WHEEL_MOTION_FACTOR) && (wheel->sliding > 0))
                    {
                        int    y1;
                        float  progress_range;
                        double pow_b;
                        double fix_mod = ((double)wheel->slide_step / ITU_WHEEL_MOTION_THRESHOLD);

                        if (fix_mod > ITU_WHEEL_MAX_INIT_POWER)
                        {
                            fix_mod = ITU_WHEEL_MAX_INIT_POWER;
                        }

                        pow_b = 5.0 * ITU_WHEEL_MAX_INIT_POWER / fix_mod;

                        y1 = (int)(pow(pow_b, (double)wheel->totalframe / (ITU_WHEEL_MOTION_FACTOR / (5.0 - fix_mod))));

                        if (y1 > ITU_WHEEL_MOTION_FACTOR)
                        {
                            y1 = ITU_WHEEL_MOTION_FACTOR;
                        }

                        (void)printf("[ydiff %d] [pow_b %.3f] [totalframe %d]\n", wheel->slide_step, pow_b,
                               wheel->totalframe);

                        progress_range = (float)wheel->totalframe / ITU_WHEEL_MOTION_FACTOR;

                        if (wheel->slide_itemcount)
                        {
                            wheel->slide_itemcount--;
                        }
                        else
                        {
                            if (progress_range <= ITU_WHEEL_PROCESS_STAGE1)
                            {
                                wheel->totalframe += y1;
                            }
                            else if (progress_range <= ITU_WHEEL_PROCESS_STAGE2)
                            {
                                wheel->totalframe += y1 * 3;
                            }
                            else
                            {
                                wheel->totalframe += y1 * (int)fix_mod;
                            }
                        }

                        if (wheel->totalframe > 40)
                        {
                            wheel->totalframe = ITU_WHEEL_MOTION_FACTOR;
                        }
                    }
                    else
                    {
                        wheel->inc = 0;
                        widget->flags &= ~ITU_UNDRAGGING;
                        wheel->totalframe = wheel->org_totalframe;

                        if (wheel->sliding == 1)
                        {
                            wheel->sliding = 0;
                        }

                        WheelLayoutCal(widget);
                    }
                }
                else
                {
                    WheelRunTimeSizeColorCal(wheel);
                }
            }
            else if ((wheel->focus_dev != 0) && (!(widget->flags & ITU_DRAGGING)))
            {
                int         i        = 0;
                int         init_dev = wheel->focus_dev * itemSize;
                ITUWidget * child    = ituWheelGetChild(widget, wheel->cycle_arr[0]);
                if (wheel->frame == 0)
                {
                    wheel->tempy = child->rect.y;
                }

                for (; i < wheel->cycle_arr_count; i++)
                {
                    int fy;

                    child = ituWheelGetChild(widget, wheel->cycle_arr[i]);

                    if (i == 0)
                    {
                        if (wheel->frame == 0)
                        {
                            wheel->tempy = child->rect.y;
                        }
                    }

                    fy = wheel->tempy - (init_dev * 1 * wheel->frame / wheel->totalframe) + (i * itemSize);
                    if (wheel->cycle_arr[i] == wheel->focusIndex)
                    {
                        fy -= (wheel->focusFontHeight - wheel->fontHeight) / ITU_WHEEL_FOCUSFONT_POS_Y_FIX_DIV;
                    }

                    ituWidgetSetY(child, fy);
                }

                wheel->frame++;

                if (wheel->frame > wheel->totalframe)
                {
                    if (wheel->focus_dev < 0)
                    {
                        ituWheelFocusChange(widget, wheel->focusIndex - 1, __LINE__);
                    }
                    else
                    {
                        ituWheelFocusChange(widget, wheel->focusIndex + 1, __LINE__);
                    }

                    wheel->frame     = 0;
                    wheel->focus_dev = 0;
                    widget->flags &= ~ITU_UNDRAGGING;
                    wheel->touchCount = 0;
                    wheel->inc        = 0;
                    // WheelLayoutCal(widget);
                    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
                }
                else
                {
                    WheelRunTimeSizeColorCal(wheel);
                }

                return true;
            }
            else
            {
                /* do nothing */
            }
        }
    }

    result |= widget->dirty;
    return widget->visible ? result : false;
}

void ituWheelDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
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
    ituFlowWindowDraw(widget, dest, x, y, alpha);
    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);

    // draw test line for debug position alignment
    if (0)
    {
        ITUWidget * child  = ituWheelGetFocusItem((ITUWheel *)widget);
        ITUColor    colorR = {255, 255, 0, 0};
        ITUColor    colorB = {255, 0, 0, 255};
        ituDrawLine(dest, destx, desty + child->rect.y + child->rect.height, destx + widget->rect.width,
                    desty + child->rect.y + child->rect.height, &colorB, 3);
        ituDrawLine(dest, destx, desty + child->rect.y, destx + widget->rect.width, desty + child->rect.y, &colorB, 3);
        ituDrawLine(dest, destx, desty + (child->rect.height * 2), destx + widget->rect.width,
                    desty + (child->rect.height * 2), &colorR, 3);
    }
}

static void WheelOnValueChanged (ITUWheel * wheel, char * value)
{
    // DO NOTHING
}

void ituWheelInit (ITUWheel * wheel)
{
    assert(wheel);
    ITU_ASSERT_THREAD();

    (void)memset(wheel, 0, sizeof(ITUWheel));

    ituFlowWindowInit(&wheel->fwin, ITU_LAYOUT_UP);

    ituWidgetSetType(wheel, ITU_WHEEL);
    ituWidgetSetName(wheel, wheelName);
    ituWidgetSetUpdate(wheel, ituWheelUpdate);
    ituWidgetSetDraw(wheel, ituWheelDraw);
    ituWidgetSetOnAction(wheel, ituWheelOnAction);
    ituWheelSetValueChanged(wheel, WheelOnValueChanged);
}

void ituWheelLoad (ITUWheel * wheel, uint32_t base)
{
    assert(wheel);

    ituFlowWindowLoad(&wheel->fwin, base);

    ituWidgetSetUpdate(wheel, ituWheelUpdate);
    ituWidgetSetDraw(wheel, ituWheelDraw);
    ituWidgetSetOnAction(wheel, ituWheelOnAction);
    ituWheelSetValueChanged(wheel, WheelOnValueChanged);
}

void ituWheelOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    unsigned int oldFlags;
    ITUWheel *   wheel = (ITUWheel *)widget;
    assert(widget);

    switch (action)
    {
        case ITU_ACTION_PREV:
            if ((wheel->cycle_tor <= 0) && (!(widget->flags & ITU_DRAGGABLE)))
            {
                oldFlags = widget->flags;
                widget->flags |= ITU_TOUCHABLE;
                widget->flags |= ITU_DRAGGING; //fix for non-cycle bypass
                // wheel->shift_one = 0;
                ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEUP, 0, widget->rect.x, widget->rect.y);
                if ((oldFlags & ITU_TOUCHABLE) == 0)
                {
                    widget->flags &= ~ITU_TOUCHABLE;
                }
            }
            else
            {
                ituWheelPrev(wheel);
            }

            break;

        case ITU_ACTION_NEXT:
            if ((wheel->cycle_tor <= 0) && (!(widget->flags & ITU_DRAGGABLE)))
            {
                oldFlags = widget->flags;
                widget->flags |= ITU_TOUCHABLE;
                widget->flags |= ITU_DRAGGING; //fix for non-cycle bypass
                // wheel->shift_one = 0;
                ituWidgetUpdate(widget, ITU_EVENT_TOUCHSLIDEDOWN, 0, widget->rect.x, widget->rect.y);
                if ((oldFlags & ITU_TOUCHABLE) == 0)
                {
                    widget->flags &= ~ITU_TOUCHABLE;
                }
            }
            else
            {
                ituWheelNext(wheel);
            }

            break;

        case ITU_ACTION_GOTO:
            if (param[0] != '\0')
            {
                ituWheelGoto((ITUWheel *)widget, atoi(param));
            }
            break;

        case ITU_ACTION_DODELAY0:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY0, atoi(param));
            break;

        case ITU_ACTION_DODELAY1:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY1, atoi(param));
            break;

        case ITU_ACTION_DODELAY2:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY2, atoi(param));
            break;

        case ITU_ACTION_DODELAY3:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY3, atoi(param));
            break;

        case ITU_ACTION_DODELAY4:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY4, atoi(param));
            break;

        case ITU_ACTION_DODELAY5:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY5, atoi(param));
            break;

        case ITU_ACTION_DODELAY6:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY6, atoi(param));
            break;

        case ITU_ACTION_DODELAY7:
            ituExecActions(widget, ((ITUWheel *)widget)->actions, ITU_EVENT_DELAY7, atoi(param));
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

void ituWheelPrev (ITUWheel * wheel)
{
    assert(wheel);
    ITU_ASSERT_THREAD();

    // force to reset frame and total frame to default
    wheel->totalframe = wheel->org_totalframe;

    if (wheel->cycle_tor)
    {
        /*if (wheel->focusIndex > 0)
        wheel->focusIndex--;
        else
        wheel->focusIndex = fmax;

        ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
        ituExecActions((ITUWidget*)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);*/
        wheel->frame     = 0;
        wheel->focus_dev = -1;
    }
    else
    {
        // if (wheel->focusIndex > 0)
        //  wheel->focusIndex--;
        wheel->frame     = 0;
        wheel->focus_dev = -1;
    }

    /*
    if (wheel->focusIndex > 0)
    {
    ITUText* text;
    char* value;
    wheel->focusIndex--;
    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

    text = (ITUText*)itcTreeGetChildAt(wheel, wheel->itemCount / 2 + wheel->focusIndex);
    value = ituTextGetString(text);

    ituExecActions((ITUWidget*)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
    //ituWheelOnValueChanged(wheel, value);
    }
    */
}

void ituWheelNext (ITUWheel * wheel)
{
    assert(wheel);
    ITU_ASSERT_THREAD();

    // force to reset frame and total frame to default
    wheel->totalframe = wheel->org_totalframe;

    if (wheel->cycle_tor)
    {
        /*if (wheel->focusIndex < fmax)
        wheel->focusIndex++;
        else
        wheel->focusIndex = 0;

        ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
        ituExecActions((ITUWidget*)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);*/
        wheel->frame     = 0;
        wheel->focus_dev = 1;
    }
    else
    {
        // if (wheel->focusIndex < fmax)
        //  wheel->focusIndex++;
        wheel->frame     = 0;
        wheel->focus_dev = 1;
    }

    /*
    int count = itcTreeGetChildCount(wheel) - wheel->itemCount;

    if (wheel->focusIndex < count)
    {
    ITUText* text;
    char* value;
    wheel->focusIndex++;
    ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

    text = (ITUText*)itcTreeGetChildAt(wheel, wheel->itemCount / 2 + wheel->focusIndex);
    value = ituTextGetString(text);

    ituExecActions((ITUWidget*)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
    //ituWheelOnValueChanged(wheel, value);
    }
    */
}

ITUWidget * ituWheelGetFocusItem (ITUWheel * wheel)
{
    assert(wheel);
    ITU_ASSERT_THREAD();

    if (wheel->focusIndex >= 0)
    {
        ITUWidget * item = (ITUWidget *)itcTreeGetChildAt(wheel, wheel->focusIndex);
        return item;
    }
    return NULL;

    /*
    if (wheel->focusIndex >= 0)
    {
    ITUWidget* item = (ITUWidget*)itcTreeGetChildAt(wheel, wheel->itemCount / 2 + wheel->focusIndex);
    return item;
    }
    return NULL;
    */
}

void ituWheelGoto (ITUWheel * wheel, int index)
{
    ITUWidget * widget = (ITUWidget *)wheel;

    ITU_ASSERT_THREAD();

    if (wheel)
    {
        int  count       = itcTreeGetChildCount(wheel);
        bool debug_print = true;

        if (widget->flags & ITU_LONG_DRAG)
        {
            if (debug_print)
            {
                (void)printf("[WHEEL]Long dragging trigger, bypass goto!\n");
            }

            WheelLayoutCal(widget);
            return;
        }

        if ((index < 0) || (index >= count))
        {
            return;
        }

        if (wheel->focusIndex == index)
        {
            return;
        }

        if ((wheel->sliding == 0) && (wheel->temp1 == 1))
        {
            ITUWidget * wTarget = (ITUWidget *)itcTreeGetChildAt(wheel, index);
            wheel->frame        = 0;

            if (wheel->cycle_tor)
            {
                wTarget = (ITUWidget *)itcTreeGetChildAt(wheel, index);

                ituWidgetSetY(wTarget, ((widget->rect.height - wTarget->rect.height) / 2) - wTarget->rect.height);

                wheel->inc = wTarget->rect.height;
            }
            else
            {
                if (index == (count - 1))
                {
                    ituWidgetSetY(wTarget, ((widget->rect.height - wTarget->rect.height) / 2) + wTarget->rect.height);
                    wheel->inc = 0 - wTarget->rect.height;
                }
                else
                {
                    ituWidgetSetY(wTarget, ((widget->rect.height - wTarget->rect.height) / 2) - wTarget->rect.height);
                    wheel->inc = wTarget->rect.height;
                }
            }
            widget->flags &= ~ITU_UNDRAGGING;
            wheel->focusIndex = index;

            return;
        }

        // if ((wheel->cycle_tor == 1) && (wheel->maxci == 0))
        //  ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

        if (debug_print)
        {
            (void)printf("[WHEEL][GOTO %d >> %d]sliding:%d, frame:%d, totalframe:%d, inc:%d\n", wheel->focusIndex, index,
                   wheel->sliding, wheel->frame, wheel->totalframe, wheel->inc);
        }

        if (wheel->sliding)
        {
            wheel->sliding    = 0;
            wheel->frame      = 0;
            wheel->inc        = 0;
            wheel->focusIndex = index;
            wheel->totalframe = wheel->org_totalframe;
            ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);

            ituWheelGoto(wheel, index);
        }
        else
        {
            if (wheel->cycle_tor)
            {
                ituWheelFocusChange(widget, index, __LINE__);
                // widget->flags |= ITU_UNDRAGGING;
                // widget->flags &= ~ITU_DRAGGING;
                wheel->touchCount = 0;
                wheel->inc        = 0;
                // wheel->frame = wheel->totalframe;
                ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
            }
            else
            {
                if (index >= count)
                {
                    return;
                }

                wheel->focusIndex = index;
                ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
            }
        }

        ituExecActions((ITUWidget *)wheel, wheel->actions, ITU_EVENT_CHANGED, wheel->focusIndex);
    }
}

void ituWheelScal (ITUWheel * wheel, int scal)
{
    ITU_ASSERT_THREAD();

    if (scal >= 1)
    {
        wheel->scal = scal;
    }
}

bool ituWheelCheckIdle (const ITUWheel * wheel)
{
    ITU_ASSERT_THREAD();
    return (wheel->idle <= 0) ? (true) : (false);
}

int ituWheelItemCount (ITUWheel * wheel)
{
    int count = 0;
    ITU_ASSERT_THREAD();

    count = get_max_focusindex((ITUWidget *)wheel) + 1;
    return count;
}

bool ituWheelSetItemTree (ITUWheel * wheel, char ** stringarr, int itemcount)
{
    ITU_ASSERT_THREAD();

    if ((!wheel) || (itemcount <= 0) || (itemcount > ITU_WHEEL_CYCLE_ARR_LIMIT) || (stringarr[itemcount - 1] == NULL))
    {
        return false;
    }
    else
    {
        int      i, tick;
        ITCTree *child, *tree = (ITCTree *)wheel;

        tick              = (ituWheelItemCount(wheel) - itemcount);
        wheel->focusIndex = 0;

        while (tick > 0)
        {
            for (child = tree->child; child; child = child->sibling)
            {
                if (!(child->sibling))
                {
                    itcTreeRemove(child);
                    ituWidgetExit(child);
                    tick--;
                    break;
                }
            }
        }

        while (tick < 0)
        {
            ITUWidget * text     = (ITUWidget *)itcTreeGetChildAt(wheel, 0);
            ITUWidget * cloneobj = NULL;
            bool        cloned   = ituTextClone(text, &cloneobj);

            if (cloned)
            {
                ITCTree * clonechild = (ITCTree *)cloneobj;
                itcTreePushBack(tree, clonechild);
                tick++;
            }
            else
            {
                return false;
            }
        }

        for (i = 0; i < itemcount; i++)
        {
            (void)printf("<<ituWheelSetItemTree>> child %d [%s]\n", i, stringarr[i]);
        }

        for (i = 0; i < itemcount; i++)
        {
            ITUWidget * item = (ITUWidget *)itcTreeGetChildAt(wheel, i);
            ITUText *   text = (ITUText *)item;
            ituTextSetString(text, stringarr[i]);
        }

        ituWidgetUpdate(wheel, ITU_EVENT_LAYOUT, 0, 0, 0);
        return true;
    }
}

void ituWheelSetSlidable (ITUWheel * wheel, bool slidable)
{
    ITU_ASSERT_THREAD();

    assert(wheel);

    if (slidable)
    {
        wheel->slide = 1;
    }
    else
    {
        wheel->slide = 0;
    }
}

void ituWheelSetGotoAni (ITUWheel * wheel, bool enable)
{
    if (wheel)
    {
        wheel->temp1 = (enable) ? (1) : (0);
    }
}
