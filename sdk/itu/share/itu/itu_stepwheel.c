#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char stepwheelName[] = "ITUStepWheel";

#define ITUSTEPWHEEL_DEBUG 0
#if ITUSTEPWHEEL_DEBUG
    #define stepwheelLog(fmt, ...) (void)printf("[%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
    #define stepwheelLog(fmt, ...)
#endif

#define ITUSTEPWHEEL_TOUCH_GOTO_TOR 3

static void StepWheelOnValueChanged(ITUStepWheel* wheel, char* value)
{
    // DO NOTHING
}

static void StepWheelChanged(ITUWidget* widget)
{
    if (widget != NULL)
    {
        if (widget->type == ITU_STEPWHEEL)
        {
            ITUStepWheel* spw = (ITUStepWheel*)widget;
            ituExecActions(widget, spw->actions, ITU_EVENT_CHANGED, spw->focusIndex);
        }
    }
}

static uint8_t StepWheelColorFix(float color)
{
    int value = (int)color;
    if (value > 255)
    {
        value -= 256;
    }
    else
    {
        if (value < 0)
        {
            value = 0;
        }
    }

    return (uint8_t)value;
}

static void StepWheelGradingFontColor(ITUStepWheel* wheel, ITUWidget* child)
{
    if ((wheel != NULL) && (child != NULL))
    {
        const ITUText* text = (ITUText*)child;

        if ((wheel->focusFontHeight == wheel->fontHeight) ||
            (wheel->focusFontHeight == wheel->stepFontHeight1) ||
            (wheel->focusFontHeight == wheel->stepFontHeight2) ||
            (wheel->focusFontHeight == wheel->stepFontHeight3))
        {
            ITUWidget* fc = ituStepWheelGetFocusItem(wheel);
            if (fc == child)
            {
                ituWidgetSetColor(child, wheel->focusColor.alpha, wheel->focusColor.red, wheel->focusColor.green, wheel->focusColor.blue);
            }
            else
            {
                ituWidgetSetColor(child, wheel->normalColor.alpha, wheel->normalColor.red, wheel->normalColor.green, wheel->normalColor.blue);
            }
        }
        else if (text->fontHeight == wheel->fontHeight)
        {
            ituWidgetSetColor(child, wheel->normalColor.alpha, wheel->normalColor.red, wheel->normalColor.green, wheel->normalColor.blue);
        }
        else if (text->fontHeight == wheel->focusFontHeight)
        {
            ituWidgetSetColor(child, wheel->focusColor.alpha, wheel->focusColor.red, wheel->focusColor.green, wheel->focusColor.blue);
        }
        else
        {
            uint8_t alpha, red, green, blue;
            float gr, gg, gb, ga;
            float devF = (float)(wheel->focusFontHeight - wheel->fontHeight);
            float   factor = 0;
            if (devF != 0)
            {
                factor = (float)(wheel->focusFontHeight - text->fontHeight) / devF;
            }
            gr = (float)(wheel->focusColor.red - wheel->normalColor.red);
            gg = (float)(wheel->focusColor.green - wheel->normalColor.green);
            gb = (float)(wheel->focusColor.blue - wheel->normalColor.blue);
            ga = (float)(wheel->focusColor.alpha - wheel->normalColor.alpha);
            gr = (float)(wheel->focusColor.red - (gr * factor));
            gg = (float)(wheel->focusColor.green - (gg * factor));
            gb = (float)(wheel->focusColor.blue - (gb * factor));
            ga = (float)(wheel->focusColor.alpha - (ga * factor));
            red = StepWheelColorFix(gr);
            green = StepWheelColorFix(gg);
            blue = StepWheelColorFix(gb);
            alpha = StepWheelColorFix(ga);

            ituWidgetSetColor(child, alpha, red, green, blue);
        }
    }
}

static int StepWheelGetItemCount(ITUWidget* widget)
{
    if (widget == NULL)
    {
        return 0;
    }
    else if (widget->type != ITU_STEPWHEEL)
    {
        return 0;
    }
    else
    {
        return itcTreeGetChildCount(widget);
    }
}

static ITUWidget* StepWheelGetItem(ITUWidget* widget, int id)
{
    if (widget == NULL)
    {
        return NULL;
    }
    else if (widget->type != ITU_STEPWHEEL)
    {
        return NULL;
    }
    else
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        if ((spw->count == 0) || (id < 0) || (id >= spw->count))
        {
            return NULL;
        }
        else
        {
            if (spw->childArr[id] != NULL)
            {
                return spw->childArr[id];
            }
            return (ITUWidget*)itcTreeGetChildAt(widget, id);
        }
    }
}

static int StepWheelTransCycleIndex(ITUWidget* widget, int index)
{
    if (widget == NULL)
    {
        return 0;
    }
    else
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        if (index >= spw->count)
        {
            return index - spw->count;
        }
        else if (index < 0)
        {
            return (spw->count + index);
        }
        else
        {
            return index;
        }
    }
}

//check slide event and set inc to +1 or -1
static bool StepWheelEventIsSlide(ITUWidget* widget, ITUEvent ev)
{
    if (widget != NULL)
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        if (spw->slidable)
        {
            if (spw->verticalMode)
            {
                if (ev == ITU_EVENT_TOUCHSLIDEUP)
                {
                    spw->inc = 1;
                    return true;
                }
                if (ev == ITU_EVENT_TOUCHSLIDEDOWN)
                {
                    spw->inc = -1;
                    return true;
                }
            }
            else
            {
                if (ev == ITU_EVENT_TOUCHSLIDELEFT)
                {
                    spw->inc = 1;
                    return true;
                }
                if (ev == ITU_EVENT_TOUCHSLIDERIGHT)
                {
                    spw->inc = -1;
                    return true;
                }
            }
        }
    }

    return false;
}

static int StepWheelCTDItem(ITUWidget* widget)
{
    if (widget != NULL)
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        int i = 0, fitItemID = -1;
        int centerPos = ((spw->verticalMode ? (widget->rect.height) : (widget->rect.width)) / 2);
        int inc = (spw->verticalMode ? (widget->rect.height) : (widget->rect.width));
        ITCTree* tree = (ITCTree*)widget;
        ITCTree* treeitem = tree->child;
        ITUWidget* item = (ITUWidget*)treeitem;
        if (spw->childArr[0] == NULL)
        {
            if (spw->count > 0)
            {
                ITUStepWheel* spw = (ITUStepWheel*)widget;
                fitItemID = spw->focusIndex;
            }
            for (; i < spw->count; i++)
            {
                if (item != NULL)
                {
                    int itemDistToCenter = (spw->verticalMode ? (item->rect.y + (item->rect.height / 2)) : (item->rect.x + (item->rect.width / 2)));
                    int dev = abs(itemDistToCenter - centerPos);
                    if (dev < inc)
                    {
                        inc = dev;
                        fitItemID = i;
                    }
                    treeitem = treeitem->sibling;
                    item = (ITUWidget*)treeitem;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            int tickCount = 0;
            i = StepWheelTransCycleIndex(widget, spw->focusIndex - 2);
            fitItemID = spw->focusIndex;
            //check range is focusIndex +/- 2
            for (; tickCount < 5; tickCount++)
            {
                item = spw->childArr[i];
                if (item != NULL)
                {
                    int itemDistToCenter = (spw->verticalMode ? (item->rect.y + (item->rect.height / 2)) : (item->rect.x + (item->rect.width / 2)));
                    int dev = abs(itemDistToCenter - centerPos);
                    if (dev < inc)
                    {
                        inc = dev;
                        fitItemID = i;
                    }
                }
                else
                {
                    break;
                }
                i = StepWheelTransCycleIndex(widget, i + 1);
            }
        }
        return fitItemID;
    }
    return -1;
    
}

static bool StepWheelBoundCheck(ITUWidget* widget, int index, int* x, int* y)
{
    if (widget != NULL)
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        ITUWidget* childZero = StepWheelGetItem(widget, 0);
        if ((!spw->cycleMode) && (childZero != NULL))
        {
            int xx = *x;
            int yy = *y;
            if (spw->verticalMode)
            {
                int GC1 = (widget->rect.height - childZero->rect.height) / 2;
                int GC2 = GC1 + childZero->rect.height;
                int childLastY, childZeroY = yy - (index * childZero->rect.height);
                if (childZeroY > (GC1 + spw->nonCycleBoundTor))
                {
                    *y = yy - (childZeroY - (GC1 + spw->nonCycleBoundTor));
                    return true;
                }
                childLastY = yy + ((spw->count - index) * childZero->rect.height);
                if (childLastY < (GC2 - spw->nonCycleBoundTor))
                {
                    *y = yy + ((GC2 - spw->nonCycleBoundTor) - childLastY);
                    return true;
                }
            }
            else
            {
                int GC1 = (widget->rect.width - childZero->rect.width) / 2;
                int GC2 = GC1 + childZero->rect.width;
                int childLastX, childZeroX = xx - (index * childZero->rect.width);
                if (childZeroX > (GC1 + spw->nonCycleBoundTor))
                {
                    *x = xx - (childZeroX - (GC1 + spw->nonCycleBoundTor));
                    return true;
                }
                childLastX = xx + ((spw->count - index) * childZero->rect.width);
                if (childLastX < (GC2 - spw->nonCycleBoundTor))
                {
                    *x = xx + ((GC2 - spw->nonCycleBoundTor) - childLastX);
                    return true;
                }
            }
        }
    }

    return false;
}

static int StepWheelCenterShiftDist(ITUWidget* widget)
{
    if (widget != NULL)
    {
        if (widget->type == ITU_STEPWHEEL)
        {
            ITUStepWheel* spw = (ITUStepWheel*)widget;
            int CenterID = StepWheelCTDItem(widget);
            ITUWidget* item = StepWheelGetItem(widget, CenterID);
            if (item != NULL)
            {
                int GCX = (widget->rect.width - item->rect.width) / 2;
                int GCY = (widget->rect.height - item->rect.height) / 2;
                return (spw->verticalMode ? (item->rect.y - GCY) : (item->rect.x - GCX));
            }
        }
    }

    return 0;
}

static void StepWheelShiftAllItem(ITUWidget* widget, int shift)
{
    if (widget != NULL)
    {
        if (widget->type == ITU_STEPWHEEL)
        {
            ITUStepWheel* spw = (ITUStepWheel*)widget;
            int i = 0;
            for (; i < spw->count; i++)
            {
                ITUWidget* item = StepWheelGetItem(widget, 0);
                if (item != NULL)
                {
                    int posX = (spw->verticalMode) ? (item->rect.x) : (item->rect.y +shift);
                    int posY = (spw->verticalMode) ? (item->rect.x + shift) : (item->rect.y);
                    ituWidgetSetPosition(item, posX, posY);
                }
            }
        }
    }
}

static void StepWheelItemCal(ITUWidget* widget, ITUEvent ev, int move)
{
    if (widget != NULL)
    {
        if (widget->type == ITU_STEPWHEEL)
        {
            ITUStepWheel* spw = (ITUStepWheel*)widget;
            ITUWidget* item = StepWheelGetItem(widget, 0);

            if ((spw->count > 0) && (item != NULL))
            {
                int CenterID = ((ev == ITU_EVENT_LAYOUT) ? (spw->focusIndex) : (StepWheelCTDItem(widget)));
                ITUWidget* CenterChild = StepWheelGetItem(widget, CenterID);
                int CenterPosX = CenterChild->rect.x;
                int CenterPosY = CenterChild->rect.y;
                int itemSize = (spw->verticalMode) ? (item->rect.height) : (item->rect.width);

                if (!spw->cycleMode)
                {
                    int i = 0;
                    int posX = (spw->verticalMode) ? (CenterPosX) : (CenterPosX - (CenterID * itemSize));
                    int posY = (spw->verticalMode) ? (CenterPosY - (CenterID * itemSize)) : (CenterPosY);
                    ITCTree* tree = (ITCTree*)widget;
                    ITCTree* item = tree->child;
                    ITUWidget* child = (ITUWidget*)item;
                    while (child != NULL)
                    {
                        int moveModX = ((spw->verticalMode) ? (0) : (move));
                        int moveModY = ((spw->verticalMode) ? (move) : (0));
                        int setX = posX + moveModX;
                        int setY = posY + moveModY;
                        StepWheelBoundCheck(widget, i, &setX, &setY);
                        ituWidgetSetPosition(child, setX, setY);
                        posX += (spw->verticalMode) ? (0) : (itemSize);
                        posY += (spw->verticalMode) ? (itemSize) : (0);
                        i++;
                        item = item->sibling;
                        child = (ITUWidget*)item;
                    }
                }
                else
                {
                    int i = spw->focusIndex; //start from focusIndex to find out the boundary item
                    int posX = CenterPosX;
                    int posY = CenterPosY;
                    int pos = (spw->verticalMode ? (posY) : (posX));
                    int tickLoop = (spw->count / 2) + 2; //minimum 2 ~ item count/2
                    ITUWidget* child = NULL;
                    ITUWidget* childStart = NULL;
                    ITUWidget* childZero = StepWheelGetItem(widget, 0);
                    ITCTree* treeItem = NULL;
                    do
                    {
                        pos -= itemSize;
                        i = StepWheelTransCycleIndex(widget, i - 1);
                        tickLoop--;
                    } while ((pos >= 0) && (tickLoop > 0));
                    if (spw->verticalMode)
                    {
                        posY = pos;
                    }
                    else
                    {
                        posX = pos;
                    }
                    child = StepWheelGetItem(widget, i);
                    treeItem = (ITCTree*)child;
                    childStart = child;
                    tickLoop = 0; //reset to zero
                    while ((child != NULL) && (tickLoop < spw->count))
                    {
                        int moveModX = ((spw->verticalMode) ? (0) : (move));
                        int moveModY = ((spw->verticalMode) ? (move) : (0));
                        ituWidgetSetPosition(child, posX + moveModX, posY + moveModY);
                        posX += (spw->verticalMode) ? (0) : (itemSize);
                        posY += (spw->verticalMode) ? (itemSize) : (0);
                        i = StepWheelTransCycleIndex(widget, i + 1);
                        child = StepWheelGetItem(widget, i);

                        if (child == childStart)
                        {
                            break;
                        }

                        tickLoop++;
                    }
                }
                if (move != 0)
                {
                    CenterID = StepWheelCTDItem(widget);
                    if (CenterID != spw->focusIndex)
                    {
                        spw->focusIndex = CenterID;
                        StepWheelChanged(widget);
                    }
                }
            }
        }
    }
}

static void StepWheelRunTimeSizeColorCal(ITUWidget* widget)
{
    if (widget != NULL)
    {
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        ITUWidget* item = StepWheelGetItem(widget, 0);

        if ((spw->count > 0) && (item != NULL))
        {
            int centralPos = (spw->verticalMode ? (widget->rect.height - item->rect.height) : (widget->rect.width - item->rect.width)) / 2;
            int base_size = (spw->verticalMode ? (item->rect.height) : (item->rect.width));
            int i = 0;
            ITCTree* tree = (ITCTree*)widget;
            ITCTree* item = tree->child;
            ITUWidget* child = (ITUWidget*)item;
            for (; i < spw->count; i++)
            {
                int dist = abs((spw->verticalMode ? (child->rect.y) : (child->rect.x)) - centralPos);

                int dist_mp = dist / base_size;
                if (dist_mp < 3)
                {
                    //int   mod_dist = base_size - ((dist % base_size) + (((dist_mp > 0) ? (1) : (0)) * base_size));
                    int   mod_dist = (dist % base_size);
                    float factor = ((float)mod_dist / base_size);
                    float stepValue = 0.0;
                    int   base_fontsize = 0;
                    int   size_dev;
                    
                    if (dist_mp < 1)
                    {
                        base_fontsize = spw->focusFontHeight;
                        size_dev = base_fontsize - spw->stepFontHeight1;

                    }
                    else if (dist_mp < 2)
                    {
                        base_fontsize = spw->stepFontHeight1;
                        size_dev = base_fontsize - spw->stepFontHeight2;
                    }
                    else
                    {
                        base_fontsize = spw->stepFontHeight2;
                        size_dev = base_fontsize - spw->fontHeight;
                    }

                    stepValue = (size_dev * factor);

                    if (spw->fontsquareMode)
                    {
                        // cover for font scaler not width-height-equal
                        int kValue = (int)roundf(stepValue);
                        kValue = kValue - (kValue % 3);
                        ituTextSetFontSize((ITUText*)child, base_fontsize - kValue);
                    }
                    else
                    {
                        ituTextSetFontHeight((ITUText*)child, base_fontsize - (int)stepValue);
                    }
                }
                else
                {
                    ituTextSetFontHeight((ITUText*)child, spw->fontHeight);
                }

                StepWheelGradingFontColor(spw, child);
                item = item->sibling;
                child = (ITUWidget*)item;
            }
        }
    }
}

void ituStepWheelExit(ITUWidget* widget)
{
    ITUStepWheel* spw = (ITUStepWheel*)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    ituWidgetExitImpl(widget);
}

bool ituStepWheelClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUStepWheel* spw = (ITUStepWheel*)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        void* newObj = malloc(sizeof(ITUStepWheel));
        const void* srcObj = (const void*)widget;
        ITUWidget* newWidget = (ITUWidget*)newObj;
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newObj, srcObj, sizeof(ITUStepWheel));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned = newWidget;
    }

    return ituWidgetCloneImpl(widget, cloned);
}

bool ituStepWheelUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    if (widget == NULL)
    {
        return false;
    }
    else
    {
        bool result = false;
        ITUStepWheel* spw = (ITUStepWheel*)widget;
        ITUWidget* itemZero = StepWheelGetItem(widget, 0);

        //global rectangle dimension related variables
        int itemSize = (spw->verticalMode) ? (itemZero->rect.height) : (itemZero->rect.width);
        //calculate center item location x,y
        int GCX = (widget->rect.width - itemZero->rect.width) / 2;
        int GCY = (widget->rect.height - itemZero->rect.height) / 2;

        if ((ev == ITU_EVENT_LAYOUT) || (ev == ITU_EVENT_LANGUAGE))
        {
            result |= ituFlowWindowUpdate(widget, ev, arg1, arg2, arg3);
            if (widget->type == ITU_STEPWHEEL)
            {
                ITUWidget** childArr = (ITUWidget**)ituWidgetGetCustomData(spw);
                if ((childArr == NULL) && (spw->count > 0))
                {
                    ITUWidget** mallocArr = (ITUWidget**)malloc(sizeof(ITUWidget*) * spw->count);
                    if (mallocArr != NULL)
                    {
                        ITCTree* tree = (ITCTree*)widget;
                        ITCTree* item = tree->child;
                        ITUWidget* child = (ITUWidget*)item;
                        int i = 0;
                        for (; i < spw->count;)
                        {
                            if (item == NULL)
                            {
                                break;
                            }
                            else
                            {
                                mallocArr[i] = (ITUWidget*)malloc(sizeof(ITUWidget));
                                mallocArr[i] = child;
                                item = item->sibling;
                                child = (ITUWidget*)item;
                                i++;
                            }
                        }

                        if (i > 0)
                        {
                            spw->count = i;
                            ituWidgetSetCustomData(spw, mallocArr);
                        }
                    }
                }
            }
        }

        if (ev == ITU_EVENT_LAYOUT)
        {
            ITUWidget* FocusChild = StepWheelGetItem(widget, spw->focusIndex);
            if (FocusChild != NULL)
            {
                int CenterPosX = (widget->rect.width - FocusChild->rect.width) / 2;
                int CenterPosY = (widget->rect.height - FocusChild->rect.height) / 2;
                ituWidgetSetPosition(FocusChild, CenterPosX, CenterPosY);
                StepWheelItemCal(widget, ev, 0);
                StepWheelRunTimeSizeColorCal(widget);
            }
        }

        if (ituWidgetIsEnabled(widget) && (widget->flags & ITU_TOUCHABLE))
        {
			int x = widget->rect.x < 0 ? arg2 : (arg2 - widget->rect.x);
			int y = widget->rect.y < 0 ? arg3 : (arg3 - widget->rect.y);
            int minX, minY, maxX, maxY;

            if (spw->temp[0] == 0)
            {
                //ituWidgetGetGlobalPosition(widget, &minX, &minY);
				minX = 0;
				minY = 0;
				maxX = widget->rect.x < 0 ? (widget->rect.x + widget->rect.width) : widget->rect.width; 
				maxY = widget->rect.y < 0 ? (widget->rect.y + widget->rect.height) : widget->rect.height; 
				ITUWidget* wp = (ITUWidget*)widget->tree.parent;
				if (wp != NULL)
				{
					if ((widget->rect.x + widget->rect.width) > wp->rect.width)
					{
						maxX = widget->rect.x < 0 ? wp->rect.width : (wp->rect.width - widget->rect.x);
					}

					if ((widget->rect.y + widget->rect.height) > wp->rect.height)
					{
						maxX = widget->rect.y < 0 ? wp->rect.height : (wp->rect.height - widget->rect.y);
					}
				}


            }
            else // stepwheel outside draggable 
            {
                minX = 0;
                minY = 0;
                maxX = ithLcdGetWidth();
                maxY = ithLcdGetHeight();
            }

            if (ev == ITU_EVENT_MOUSEDOWN)
            {
                if (ituWidgetIsInside(widget, x, y))
                {
                    if (widget->flags & ITU_DRAGGABLE)
                    {
                        widget->flags |= ITU_DRAGGING;
                        widget->flags &= ~ITU_UNDRAGGING;
                        ituScene->dragged = widget;
                        spw->frame = 0;
                        spw->inc = 0;
                        spw->sliding = false;
                        spw->totalframe = spw->org_totalframe;
                        spw->touchPos = (spw->verticalMode ? (y) : (x));
                    }
                }
            }
            else if (ev == ITU_EVENT_MOUSEUP)
            {
                if ((widget->flags & ITU_DRAGGING) && !(spw->sliding))
                {
                    if (widget->flags & ITU_LONG_DRAG)
                    {
                        spw->inc = StepWheelCenterShiftDist(widget);
                        widget->flags &= ~ITU_LONG_DRAG;
                        widget->flags &= ~ITU_DRAGGING;
                        widget->flags |= ITU_UNDRAGGING;
                    }
                    else
                    { //for touch item goto usage
                        int i = 0, count = ituStepWheelItemCount(spw);
                        int currIT = ituStepWheelCFDistItem(spw);
                        int range1 = 0, range2 = 0;
                        for (; i < count; i++)
                        {
                            ITUWidget* child = StepWheelGetItem(widget, i);
                            if (child != NULL)
                            {
                                range1 = (spw->verticalMode) ? (child->rect.y) : (child->rect.x);
                                range2 = range1 + ((spw->verticalMode) ? (child->rect.height) : (child->rect.width));
                                if ((spw->touchPos > range1) && (spw->touchPos < range2))
                                {
                                    widget->flags &= ~ITU_DRAGGING;
                                    if (i != currIT)
                                    {
                                        ituStepWheelGoto(spw, i);
                                        result = true;
                                        break;
                                    }
                                    else
                                    {
                                        spw->inc = StepWheelCenterShiftDist(widget);
                                        widget->flags &= ~ITU_LONG_DRAG;
                                        widget->flags &= ~ITU_DRAGGING;
                                        widget->flags |= ITU_UNDRAGGING;
                                    }
                                }
                            }
                        }
                        widget->flags &= ~ITU_DRAGGING;
                        result = true;
                    }
                }
            }
            else if (ev == ITU_EVENT_MOUSEMOVE)
            {
                if (widget->flags & ITU_DRAGGING)
                {
					int mx = x;
					int my = y;
					int move = 0;
					if (spw->temp[0] == 0)
					{ 
						if ((mx > minX) && (mx < maxX) && (my > minY) && (my < maxY))
						{
							move = (spw->verticalMode ? (my) : (mx)) - spw->touchPos;
							StepWheelItemCal(widget, ev, move);
							StepWheelRunTimeSizeColorCal(widget);
							//for touch item goto usage
							if (abs(move) >= ITUSTEPWHEEL_TOUCH_GOTO_TOR)
							{
								widget->flags |= ITU_LONG_DRAG;
							}
							spw->touchPos = (spw->verticalMode ? (my) : (mx));
							result = true;
						}
					}
					else
					{
						move = (spw->verticalMode ? (my) : (mx)) - spw->touchPos;
                    StepWheelItemCal(widget, ev, move);
                    StepWheelRunTimeSizeColorCal(widget);
                    //for touch item goto usage
                    if (abs(move) >= ITUSTEPWHEEL_TOUCH_GOTO_TOR)
                    {
                        widget->flags |= ITU_LONG_DRAG;
                    }
                    spw->touchPos = (spw->verticalMode ? (my) : (mx));
                    result = true;
					}
                    
                }
            }
            else if (StepWheelEventIsSlide(widget, ev))
            {
                if (widget->flags & ITU_DRAGGING)
                {
                    widget->flags &= ~ITU_DRAGGING;
                    widget->flags &= ~ITU_LONG_DRAG;
                    if (!spw->sliding)
                    {
                        //inc comes +1/-1 from StepWheelEventIsSlide
                        spw->inc *= arg1;
                        spw->totalframe = spw->org_totalframe * 10;
                        spw->sliding = true;
                    }
                    result = true;
                }











            }
            else if (ev == ITU_EVENT_TIMER)
            {
                if (widget->flags & ITU_UNDRAGGING)
                {
                    ITUWidget* itemF = StepWheelGetItem(widget, spw->focusIndex);
                    int fromPos = (spw->verticalMode) ? (itemF->rect.y) : (itemF->rect.x);
                    int toPos = (spw->verticalMode) ? (GCY) : (GCX);
                    bool finNow = false;
                    if ((itemF != NULL) && (spw->totalframe > 0))
                    {
                        int step = spw->inc / spw->totalframe;
                        int currentPosDiff = toPos - fromPos;
                        if (step == 0) //inc too small will be fixed here
                        {
                            step = (spw->inc > 0) ? (-1) : (1);
                        }
                        else
                        {
                            step *= -1; //step is reverse way to inc (inc > 0, step < 0) (inc < 0, step > 0)
                        }
                        spw->frame++;
                        if (step < 0)
                        {
                            if ((fromPos + step) <= toPos)
                            {
                                //impossible case should reverse way to board
                                if ((fromPos < toPos) && (step < 0))
                                {
                                    step *= -1;
                                }
                                else
                                {
                                    finNow = true;
                                }
                            }
                        }
                        else // (step < 0)
                        {
                            if ((fromPos + step) >= toPos)
                            {
                                //impossible case should reverse way to board
                                if ((fromPos > toPos) && (step > 0))
                                {
                                    step *= -1;
                                }
                                else
                                {
                                    finNow = true;
                                }
                            }
                        }

                        if (spw->frame >= spw->totalframe)
                        {
                            finNow = true;
                        }

                        if (finNow)
                        {
                            StepWheelItemCal(widget, ev, currentPosDiff);
                            spw->totalframe = spw->org_totalframe;
                            spw->frame = 0;
                            spw->inc = 0;
                            widget->flags &= ~ITU_UNDRAGGING;
                            result = true;
                        }
                        else
                        {
                            StepWheelItemCal(widget, ev, step);
                        }

                        StepWheelRunTimeSizeColorCal(widget);
                    }
                    result = true;
                }

                if (spw->sliding)
                {
                    ITUWidget* itemF = StepWheelGetItem(widget, spw->focusIndex);
                    int fromPos = (spw->verticalMode) ? (itemF->rect.y) : (itemF->rect.x);
                    int toPos = (spw->verticalMode) ? (GCY) : (GCX);

                    bool finNow = false;

                    if ((itemF != NULL) && (spw->org_totalframe > 0))
                    {
                        int step = spw->inc;
                        int currentPosDiff = toPos - fromPos;

                        
                        step -= (step * spw->frame) / spw->totalframe;

                        if ((spw->forceFactor > 1) && (spw->forceFactor <= 10))
                        {
                            step = (step * 1) / (10 * (10 / spw->forceFactor));
                        }
                        else
                        {
                            step = step / 10;
                        }

                        if ((abs(step) <= spw->forceFactor) && (spw->frame == 0)) //init force too small will be fixed here
                        {
                            step = abs(spw->forceFactor);
                        }
                        else
                        {
                            step *= -1; //step is reverse way to inc (inc > 0, step < 0) (inc < 0, step > 0)
                        }
                        spw->frame++;

                        if (spw->frame >= spw->totalframe)
                        {
                            finNow = true;
                        }

                        if (finNow)
                        {
                            spw->frame = 0;
                            spw->inc = currentPosDiff;
                            spw->sliding = false;
                            widget->flags |= ITU_UNDRAGGING;
                        }
                        else
                        {
                            StepWheelItemCal(widget, ev, step);
                        }

                        StepWheelRunTimeSizeColorCal(widget);
                    }
                    result = true;
                }
            }
            else
            {
                //wait to define
            }
        }

        return result;
    }
}

void ituStepWheelDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    uint8_t desta;
    ITURectangle prevClip;
    ITURectangle* rect;
    assert(widget);
    assert(dest);

    rect = (ITURectangle*)(&widget->rect);
    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->color.alpha / 255;
    desta = desta * widget->alpha / 255;

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);
    //ituFlowWindowDraw(widget, dest, x, y, alpha);

    if (desta == 255)
    {
        ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
    }
    else if (desta > 0)
    {
        ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
        if (surf)
        {
            ituColorFill(surf, 0, 0, rect->width, rect->height, &widget->color);
            ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
            ituDestroySurface(surf);
        }
    }
    else
    {
        stepwheelLog("widget %s alpha/color-alpha/widget-alpha[%d/%d/%d]\n", widget->name, alpha, widget->color.alpha, widget->alpha);
    }

    ituFlowWindowDraw(widget, dest, x, y, alpha);
    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
}

void ituStepWheelInit(ITUStepWheel* wheel)
{
    assert(wheel);
    ITU_ASSERT_THREAD();

    (void)memset(wheel, 0, sizeof(ITUStepWheel));

    ituFlowWindowInit(&wheel->fwin, ITU_LAYOUT_UP);

    ituWidgetSetType(wheel, ITU_STEPWHEEL);
    ituWidgetSetName(wheel, stepwheelName);
    ituWidgetSetExit(wheel, ituStepWheelExit);
    ituWidgetSetClone(wheel, ituStepWheelClone);
    ituWidgetSetUpdate(wheel, ituStepWheelUpdate);
    ituWidgetSetDraw(wheel, ituStepWheelDraw);
    ituWidgetSetOnAction(wheel, ituStepWheelOnAction);
    ituStepWheelSetValueChanged(wheel, StepWheelOnValueChanged);
}

void ituStepWheelLoad(ITUStepWheel* wheel, uint32_t base)
{
    assert(wheel);

    ituFlowWindowLoad(&wheel->fwin, base);
    ituWidgetSetExit(wheel, ituStepWheelExit);
    ituWidgetSetClone(wheel, ituStepWheelClone);
    ituWidgetSetUpdate(wheel, ituStepWheelUpdate);
    ituWidgetSetDraw(wheel, ituStepWheelDraw);
    ituWidgetSetOnAction(wheel, ituStepWheelOnAction);
    ituStepWheelSetValueChanged(wheel, StepWheelOnValueChanged);
}

void ituStepWheelOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUStepWheel* spw = (ITUStepWheel*)widget;
    assert(widget);

    if (widget->type != ITU_STEPWHEEL)
    {
        return;
    }

    switch (action)
    {
    case ITU_ACTION_PREV:
        {
            ituStepWheelPrev(spw);
        }
        break;

    case ITU_ACTION_NEXT:
        {
            ituStepWheelNext(spw);
        }
        break;

    case ITU_ACTION_GOTO:
        if (param[0] != '\0')
        {
            ituStepWheelGoto(spw, atoi(param));
        }
        break;

    case ITU_ACTION_DODELAY0:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY0, atoi(param));
        break;

    case ITU_ACTION_DODELAY1:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY1, atoi(param));
        break;

    case ITU_ACTION_DODELAY2:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY2, atoi(param));
        break;

    case ITU_ACTION_DODELAY3:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY3, atoi(param));
        break;

    case ITU_ACTION_DODELAY4:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY4, atoi(param));
        break;

    case ITU_ACTION_DODELAY5:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY5, atoi(param));
        break;

    case ITU_ACTION_DODELAY6:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY6, atoi(param));
        break;

    case ITU_ACTION_DODELAY7:
        ituExecActions(widget, spw->actions, ITU_EVENT_DELAY7, atoi(param));
        break;

    default:
        ituWidgetOnActionImpl(widget, action, param);
        break;
    }
}

void ituStepWheelPrev(ITUStepWheel* spw)
{
    ITUWidget* widget = (ITUWidget*)spw;
    ITUWidget* itemZero;
    assert(spw);
    ITU_ASSERT_THREAD();
    itemZero = StepWheelGetItem(widget, 0);
    if (itemZero != NULL)
    {
        int dev = (spw->verticalMode) ? ((itemZero->rect.height / 2) + 4) : ((itemZero->rect.width / 2) + 4);
        StepWheelItemCal(widget, ITU_EVENT_CUSTOM, dev); //+dev shift --> prev
        StepWheelRunTimeSizeColorCal(widget);
        widget->flags |= ITU_UNDRAGGING;
        spw->inc = StepWheelCenterShiftDist(widget);
    }
}

void ituStepWheelNext(ITUStepWheel* spw)
{
    ITUWidget* widget = (ITUWidget*)spw;
    ITUWidget* itemZero;
    assert(spw);
    ITU_ASSERT_THREAD();
    itemZero = StepWheelGetItem(widget, 0);
    if (itemZero != NULL)
    {
        int dev = (spw->verticalMode) ? ((itemZero->rect.height / 2) + 4) : ((itemZero->rect.width / 2) + 4);
        StepWheelItemCal(widget, ITU_EVENT_CUSTOM , -dev); //-dev shift --> next
        StepWheelRunTimeSizeColorCal(widget);
        widget->flags |= ITU_UNDRAGGING;
        spw->inc = StepWheelCenterShiftDist(widget);
    }
}

ITUWidget* ituStepWheelGetFocusItem(ITUStepWheel* spw)
{
    ITUWidget* widget = (ITUWidget*)spw;
    assert(spw);
    ITU_ASSERT_THREAD();
    return StepWheelGetItem(widget, spw->focusIndex);
}

void ituStepWheelGoto(ITUStepWheel* spw, int index)
{
    ITUWidget* widget = (ITUWidget*)spw;
    int modIndex;
    assert(spw);
    ITU_ASSERT_THREAD();
    modIndex = StepWheelTransCycleIndex(widget, index);
    if (modIndex != spw->focusIndex)
    {
        spw->focusIndex = modIndex;
        StepWheelChanged(widget);
    }
    ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituStepWheelSetForceFactor(ITUStepWheel* spw, int factor)
{
    assert(spw);
    ITU_ASSERT_THREAD();
    if ((factor >= 1) && (factor <= 5))
    {
        spw->forceFactor = factor;
    }
}

bool ituStepWheelCheckIdle(const ITUStepWheel* spw)
{
    assert(spw);
    ITU_ASSERT_THREAD();
    return spw->idleNow;
}

int ituStepWheelItemCount(ITUStepWheel* spw)
{
    ITUWidget* widget = (ITUWidget*)spw;
    assert(spw);
    ITU_ASSERT_THREAD();
    if (spw->count > 0)
    {
        return spw->count;
    }
    else
    {
        spw->count = StepWheelGetItemCount(widget);
        return spw->count;
    }
}

bool ituStepWheelSetItemTree(ITUStepWheel* spw, char** stringarr, int itemcount)
{
    assert(spw);
    ITU_ASSERT_THREAD();

    if ((itemcount <= 0) || (itemcount > ITU_WHEEL_CYCLE_ARR_LIMIT) || (stringarr[itemcount - 1] == NULL))
    {
        return false;
    }
    else
    {
        int      i, tick;
        ITCTree* tree = (ITCTree*)spw;
        ITUWidget* widget = (ITUWidget*)spw;

        tick = (ituStepWheelItemCount(spw) - itemcount);
        spw->focusIndex = 0;

        while (tick > 0)
        {
            ITCTree* child;
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
            ITUWidget* text = StepWheelGetItem(widget, 0);
            ITUWidget* cloneobj = NULL;
            bool        cloned = ituTextClone(text, &cloneobj);

            if (cloned)
            {
                ITCTree* clonechild = (ITCTree*)cloneobj;
                itcTreePushBack(tree, clonechild);
                tick++;
            }
            else
            {
                return false;
            }
        }

#if ITUSTEPWHEEL_DEBUG
        for (i = 0; i < itemcount; i++)
        {
            stepwheelLog("item %d [%s]\n", i, stringarr[i]);
        }
#endif

        for (i = 0; i < itemcount; i++)
        {
            ITUText* text = (ITUText*)StepWheelGetItem(widget, i);
            if (text != NULL)
            {
                ituTextSetString(text, stringarr[i]);
            }
        }

        ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
        return true;
    }
}

void ituStepWheelSetSlidable(ITUStepWheel* spw, bool slidable)
{
    assert(spw);
    ITU_ASSERT_THREAD();
    spw->slidable = slidable;
}

int ituStepWheelCFDistItem(ITUStepWheel* spw)  
{
    ITUWidget* widget = (ITUWidget*)spw;
    assert(spw);
    ITU_ASSERT_THREAD();
    return StepWheelCTDItem(widget);
}

void ituStepWheelSetOutsideDraggable(ITUStepWheel* spw, bool draggable)
{
    assert(spw);
    ITU_ASSERT_THREAD();
    spw->temp[0] = (draggable) ? (1) : (0);
}