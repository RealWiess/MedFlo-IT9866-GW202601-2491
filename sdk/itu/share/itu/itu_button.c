#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itp.h"
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char btnName[] = "ITUButton";

//Special flag used to active the press alpha(image) delay mode
#define PRESS_ALPHA_DELAY_ARG 255

//The delay frame when active the press alpha(image) delay mode
#define PRESS_ALPHA_DELAY_FRAME 3

void ituButtonExit(ITUWidget* widget)
{
    ITUButton* btn = (ITUButton*) widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    // clear surf when surface link from iconlistbox target
    if (btn->dImgILB)
    {
        btn->bg.icon.surf = NULL;
    }

    if (btn->pressSurf)
    {
        ituSurfaceRelease(btn->pressSurf);
        btn->pressSurf = NULL;
    }

    if (btn->focusSurf)
    {
        ituSurfaceRelease(btn->focusSurf);
        btn->focusSurf = NULL;
    }
    ituIconExit(widget);
}

bool ituButtonClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUButton* btn = (ITUButton*)widget;
    ITUButton* newBtn;
    int layoutmode = ITU_LAYOUT_CENTER;
    bool result;
    char* textSrc = ituTextGetString(&btn->text);
    char textOrg[50];
    (void)memset(textOrg, 0, sizeof(char)* 50);
    if (textSrc)
    {
        (void)memcpy(textOrg, textSrc, sizeof(char) * 50);
    }

    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUButton));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUButton));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;

        *cloned                                                                  = newWidget;
    }

    newBtn = (ITUButton*)*cloned;

    //fix cloned button with wrong text alignment
    if (newBtn)
    {
        switch (newBtn->text.layout)
        {
        case ITU_LAYOUT_TOP_LEFT:
            layoutmode = ITU_LAYOUT_LEFT;
            break;

        case ITU_LAYOUT_TOP_RIGHT:
            layoutmode = ITU_LAYOUT_RIGHT;
            break;

        case ITU_LAYOUT_TOP_CENTER:
            layoutmode = ITU_LAYOUT_UP;
            break;

        case ITU_LAYOUT_MIDDLE_LEFT:
            layoutmode = ITU_LAYOUT_LEFT;
            break;

        case ITU_LAYOUT_MIDDLE_RIGHT:
            layoutmode = ITU_LAYOUT_RIGHT;
            break;

        case ITU_LAYOUT_MIDDLE_CENTER:
            layoutmode = ITU_LAYOUT_CENTER;
            break;

        case ITU_LAYOUT_BOTTOM_LEFT:
            layoutmode = ITU_LAYOUT_LEFT;
            break;

        case ITU_LAYOUT_BOTTOM_RIGHT:
            layoutmode = ITU_LAYOUT_RIGHT;
            break;

        case ITU_LAYOUT_BOTTOM_CENTER:
            layoutmode = ITU_LAYOUT_DOWN;
            break;

        default:
            break;
        }


        // change internal tree structure of new button
        newBtn->text.widget.tree.parent = (ITCTree*)(&newBtn->bwin);
        newBtn->bwin.widget.tree.parent = (ITCTree*)newBtn;

        newBtn->bwin.widgets[layoutmode] = &newBtn->text.widget;
        newBtn->bwin.widget.tree.child = &newBtn->text.widget.tree;
        newBtn->bg.icon.widget.tree.child = &newBtn->bwin.widget.tree;
    }

    if (!(widget->flags & ITU_EXTERNAL)) //fixed when use external can't get correct surf address when clone
    {
        ITUSurface* surf = newBtn->bg.icon.staticSurf;

        if (NULL != surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                newBtn->bg.icon.surf = ituSurfaceDecompress(surf);
            }
            else
            {
                newBtn->bg.icon.surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                        (const uint8_t *)surf->addr, surf->flags);
            }
        }

        surf = newBtn->staticFocusSurf;

        if (NULL != surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                newBtn->focusSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                newBtn->focusSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                     (const uint8_t *)surf->addr, surf->flags);
            }
        }

        surf = newBtn->staticPressSurf;

        if (NULL != surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                newBtn->pressSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                newBtn->pressSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                     (const uint8_t *)surf->addr, surf->flags);
            }
        }
    }

    //fix cloned button will draw last text into the new press surf (just work around)
    ituTextSetString(&btn->text, "");
    newBtn->text.string = NULL;
    result = ituBackgroundClone(widget, cloned);
    ituTextSetString(&btn->text, textOrg);
    ituTextSetString(&newBtn->text, textOrg);

    return result;
}

bool ituButtonTopCheckLoop(ITUWidget* tree, ITUWidget* ref, int x, int y)
{
    ITCTree * itcTree = (ITCTree *)tree;
    ITCTree * child   = itcTree;
    for (; child; child = child->sibling)
    {
        ITUWidget * widget = (ITUWidget *)child;
        if (ref->flags & ITU_LONG_PRESSING)
        {
            if (widget != ref)
            {
                int checkType[] = {ITU_BUTTON,           ITU_PROGRESSBAR,
                                   ITU_KEYBOARD,         ITU_TRACKBAR,
                                   ITU_DIGITALCLOCK,     ITU_ANALOGCLOCK,
                                   ITU_CALENDAR,         ITU_CIRCLEPROGRESSBAR,
                                   ITU_SCROLLBAR,        ITU_METER,
                                   ITU_COLORPICKER,      ITU_BACKGROUNDBUTTON,
                                   ITU_RIPPLEBACKGROUND, ITU_CURVE,
                                   ITU_WHEELBACKGROUND,  ITU_WAVEBACKGROUND,
                                   ITU_OSCILLOSCOPE,     ITU_TABLEBAR,
                                   ITU_TABLEGRID,        ITU_CHECKBOX,
                                   ITU_RADIOBOX,         ITU_POPUPRADIOBOX,
                                   ITU_POPUPBUTTON};
                int i = 0, rer = 0, checkMax = sizeof(checkType) / sizeof(int);

                for (; i < checkMax; i++)
                {
                    if ((int)widget->type == checkType[i])
                    {
                        rer++;
                        break;
                    }
                }
                if (rer != 0)
                {
                    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                    // ituWidgetGetGlobalPosition(widget, &x1, &y1);
                    x1 = widget->rect.x;
                    y1 = widget->rect.y;
                    x2 = x1 + widget->rect.width;
                    y2 = y1 + widget->rect.height;
                    if (ituWidgetIsEnabled(widget) && (x > x1) && (x < x2) &&
                        (y > y1) && (y < y2))
                    {
                        ref->flags &= ~ITU_LONG_PRESSING;
                        return false;
                    }
                }
            }
        }
        else
        {
            if (widget == ref)
            {
                ref->flags |= ITU_LONG_PRESSING;
                continue;
            }
			if ((child->child) && (widget->type != ITU_ANIMATION))
            {
                ITUWidget * newChild = (ITUWidget *)child->child;
                if (!ituButtonTopCheckLoop(newChild, ref, x, y))
                {
                    return false;
                }
            }
        }
    }
    ref->flags &= ~ITU_LONG_PRESSING;
    return true;
}

static void ButtonLoadExternalData(ITUButton* btn, ITULayer* layer)
{
    ITUWidget*  widget  = (ITUWidget*)btn;
    ITUSurface* surf    =             NULL;
    bool        drawReq = ituSceneClearD2D(ituScene);

    assert(widget);

    if (!(widget->flags & ITU_EXTERNAL))
    {
        return;
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (!btn->staticPressSurf && btn->pressed)
    {
        return;
    }
    else if (btn->pressed)
    {
#else
    if (btn->staticPressSurf && !btn->pressSurf)
    {
#endif

        if (layer == NULL)
        {
            layer = ituGetLayer(widget);
        }

        if (layer != NULL)
        {
            surf = ituLayerLoadExternalSurface(layer, (uint32_t)btn->staticPressSurf);
        }

        if (surf != NULL)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                ituSceneCheckD2D(ituScene, drawReq);
                btn->pressSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                btn->pressSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                  (const uint8_t *)surf->addr, surf->flags);
            }
        }
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        if (!(widget->flags & ITU_EXTERNAL_IN_PLACE))
        {
            widget->flags |= ITU_EXTERNAL_IN_PLACE;
        }
        return;
#endif
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if ((!btn->staticFocusSurf && widget->active) || (!widget->active))
    {
        return;
    }
#else
    if (!btn->staticFocusSurf || btn->focusSurf)
    {
        return;
    }
#endif

    if (layer == NULL)
    {
        layer = ituGetLayer(widget);
    }

    if (layer != NULL)
    {
        surf = ituLayerLoadExternalSurface(layer, (uint32_t)btn->staticFocusSurf);
    }

    if (surf != NULL)
    {
        if (surf->flags & ITU_COMPRESSED)
        {
            ituSceneCheckD2D(ituScene, drawReq);
            btn->focusSurf = ituSurfaceDecompress(surf);
        }
        else
        {
            btn->focusSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                              (const uint8_t *)surf->addr, surf->flags);
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (!(widget->flags & ITU_EXTERNAL_IN_PLACE))
    {
        widget->flags |= ITU_EXTERNAL_IN_PLACE;
    }
#endif
}

bool ituButtonUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool result = false;
    ITUButton* btn = (ITUButton*) widget;
    assert(btn);

    if (ev == ITU_EVENT_LAYOUT)
    {
        btn->bwin.widget.rect.width = widget->rect.width;
        btn->bwin.widget.rect.height = widget->rect.height;

        ituButtonSetStyleIndex(btn, btn->ImgTargetIndex);
    }
    
    if (ev == ITU_EVENT_RELEASE)
    {
        if (btn->pressed)
        {
            ituButtonSetPressed(btn, false);
        }
    }

    result |= ituIconUpdate(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_MOUSEDOWN)
    {
        if (ituWidgetIsEnabled(widget) && !result)
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;
            bool ev_topcheck = true;
            //[ITU_MOUSE_EV_TOPCHECK] check tree to find any button been touch in front of itself (tree level under itself)
            if (widget->flags & ITU_MOUSE_EV_TOPCHECK)
            {
                //mark for loop check
                if (ituButtonTopCheckLoop((ITUWidget*)ituGetLayer(widget), widget, arg2, arg3))
                    ev_topcheck = false;
            }

            if (ituWidgetIsInside(widget, x, y) && ev_topcheck)
            {
                if (btn->mouseUpDelay > 0)
                {
                    btn->mouseUpDelayCount = itpGetTickCount();
                }

                if (btn->mouseLongPressDelay > 0)
                {
                    btn->mouseLongPressDelayCount = itpGetTickCount();
                }

                if (arg1 == PRESS_ALPHA_DELAY_ARG)
                {
                    if (btn->press_alpha_delay_count == 0)
                    {
                        btn->press_alpha_delay_count = PRESS_ALPHA_DELAY_FRAME;
                    }

                    widget->dirty = true;
                    btn->pressed = true;
                }
                else
                {
                    ituButtonSetPressed(btn, true);
                }

                result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, 0);

                if (widget->type == ITU_BUTTON)
                {
                    result |= ituWidgetOnPress(widget, ev, arg1, x, y);
                }

                result |= widget->dirty;
                return result;
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEUP)
    {
        if (ituWidgetIsEnabled(widget))
        {
            ITULayer* thisLayer = ituGetLayer(widget);
            btn->press_alpha_delay_count = 0;

            if (NULL != thisLayer)
            {
                ITUWidget* parentLayer = (ITUWidget*)thisLayer;

                //clear ITU_CHILD_LONG_DRAGGING when using dynamic menu
                if (parentLayer->flags & ITU_CHILD_LONG_DRAGGING)
                {
                    parentLayer->flags &= ~ITU_CHILD_LONG_DRAGGING;
                    //(void)printf("layer %s is removed ITU_CHILD_LONG_DRAGGING\n", parentLayer->name);
                }

                if (!thisLayer->widget.visible)
                {
                    btn->pressed = 0;
                    return result;
                }
            }

            //tempory modified for mouseup check fsg2 (use mouseupDelay set to < 0)
            if (btn->fsg2 == 0)
            {
                if (thisLayer && btn->pressed)
                {
                    if (!thisLayer->widget.visible)
                    {
                        ituButtonSetPressed(btn, false);
                    }
                }
            }

            if (btn->pressed)
            {
                ituButtonSetPressed(btn, false);

                if (btn->mouseUpDelay == 0 || (btn->mouseUpDelayCount >= UINT_MAX))
                {
                    result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, 0);
                    btn->mouseUpDelayCount = 0;
                    btn->mouseLongPressDelayCount = 0;
                }

                if (widget->type == ITU_BUTTON)
                {
                    ituFocusWidget(btn);
                }

                result |= widget->dirty;
                return result;
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEDOUBLECLICK || ev == ITU_EVENT_MOUSELONGPRESS ||
        ev == ITU_EVENT_TOUCHSLIDELEFT || ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDERIGHT || ev == ITU_EVENT_TOUCHSLIDEDOWN)
    {
        if (ituWidgetIsEnabled(widget) && !result)
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;
            btn->press_alpha_delay_count = 0;

            if (!btn->mouseLongPressDelay && (!widget->rect.width || !widget->rect.height || ituWidgetIsInside(widget, x, y)))
            {
                if (ituButtonTopCheckLoop((ITUWidget*)ituGetLayer(widget), widget, arg2, arg3))
                    result |= ituExecActions(widget, btn->actions, ev, arg1);
            }
            if (btn->pressed && (ev != ITU_EVENT_MOUSELONGPRESS) && (!(btn->fsg1 & ITU_BUTTON_FLAG_SLIDE_MOUSEUP)))
            {
                ituButtonSetPressed(btn, false);
                result |= widget->dirty;
            }
            else if (ev == ITU_EVENT_MOUSELONGPRESS) //use for dynamic menu
            {
                ITUWidget* parentLayer = (ITUWidget*)ituGetLayer(widget);
                if (parentLayer)
                {
                    parentLayer->flags |= ITU_CHILD_LONG_DRAGGING;
                    //(void)printf("layer %s is set to ITU_CHILD_LONG_DRAGGING\n", parentLayer->name);
                }
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEMOVE)
    {
        if (ituWidgetIsEnabled(widget))
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            //check when move outside the button
            bool bWidgetInSideCheck = ((ituWidgetIsInside(widget, x, y)) ? (false) : (true));

            //check the flag first for special requirement
            if (bWidgetInSideCheck)
            {
                if (btn->pressed && (btn->fsg1 & ITU_BUTTON_FLAG_OUTSIDE_FORCE_MOUSEUP))
                {
                    ituButtonSetPressed(btn, false);
                    ituWidgetUpdate(btn, ITU_EVENT_MOUSEMOVEUP, 0, arg2, arg3);
                    return result;
                }
            }

            //disable outside check when using trackbar
            if (btn && bWidgetInSideCheck && ituButtonIsPressed(btn))
            {
                const ITCTree* btnTree = (ITCTree*)btn;
                ITCTree* parent = btnTree->parent;

                if (parent)
                {
                    const ITUWidget * pWidget = (ITUWidget *)parent;
                    if (pWidget->type == ITU_TRACKBAR)
                    {
                        bWidgetInSideCheck = false;
                    }
                }
            }

            //disable outside check when using dynamic menu (ITU_CHILD_LONG_DRAGGING)
            if (bWidgetInSideCheck)
            {
                const ITUWidget* parentLayer = (ITUWidget*)ituGetLayer(widget);
                if (parentLayer)
                {
                    if (parentLayer->flags & ITU_CHILD_LONG_DRAGGING)
                    {
                        bWidgetInSideCheck = false;
                        //(void)printf("remove button outside check when layer under ITU_CHILD_LONG_DRAGGING\n");
                    }
                }
            }

            if (((btn->press_alpha_delay_count < 255) && (arg1 == PRESS_ALPHA_DELAY_ARG)) || bWidgetInSideCheck)
            {
                if (ituButtonIsPressed(btn))
                {
                    //(void)printf("unpress %s\n", widget->name);
                    ituButtonSetPressed(btn, false);
                }
                btn->press_alpha_delay_count = 0;
            }
        }
    }
    else if (ev >= ITU_EVENT_CUSTOM || ev == ITU_EVENT_TIMER)
    {
        if (ituWidgetIsEnabled(widget))
        {
            if (ev == ITU_EVENT_TIMER)
            {
                if ((btn->press_alpha_delay_count > 0) && (btn->press_alpha_delay_count < 255))
                {
                    if (btn->press_alpha_delay_count == 1)
                    {
                        ituWidgetSetColor(widget, btn->pressColor.alpha, btn->pressColor.red, btn->pressColor.green, btn->pressColor.blue);
                        //btn->pressed = true;
                        widget->dirty = true;
                    }

                    btn->press_alpha_delay_count--;
                }

                if (btn->pressed)
                {
                    if (btn->mouseUpDelayCount != 0 && btn->mouseUpDelayCount != UINT_MAX)
                    {
                        if (itpGetTickDuration(btn->mouseUpDelayCount) >= btn->mouseUpDelay)
                        {
                            btn->mouseUpDelayCount = UINT_MAX;
                        }
                    }

                    if (btn->mouseLongPressDelayCount != 0)
                    {
                        if ((int)itpGetTickDuration(btn->mouseLongPressDelayCount) >= btn->mouseLongPressDelay)
                        {
                            result |= ituExecActions(widget, btn->actions, ITU_EVENT_MOUSELONGPRESS, 0);
                            btn->mouseLongPressDelayCount = 0;
                            btn->mouseUpDelayCount = 0;
                        }
                    }
                }
            }
            result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, arg1);
        }
    }
    else if (ev == ITU_EVENT_LOAD)
    {
        ituButtonLoadStaticData(btn);
        result = true;
    }
    else if (ev == ITU_EVENT_LOAD_EXTERNAL)
    {
        ButtonLoadExternalData(btn, (ITULayer*)arg1);
        result = true;
    }
    else if (ev == ITU_EVENT_RELEASE)
    {
        //clear surf when surface link from iconlistbox target
        if (btn->dImgILB)
            btn->bg.icon.surf = NULL;

        ituButtonReleaseSurface(btn);
        result = true;
    }
    else if (ituWidgetIsActive(widget) && ituWidgetIsEnabled(widget) && !result)
    {
        switch (ev)
        {
        case ITU_EVENT_KEYDOWN:
            if (arg1 == ituScene->enterKey)
            {
                ituButtonSetPressed(btn, true);
                ituFocusWidget(btn);
                result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, arg1);
            }
            break;

        case ITU_EVENT_KEYUP:
            if (arg1 == ituScene->enterKey)
            {
                ituButtonSetPressed(btn, false);
                result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, arg1);
            }
            break;
        }
        result |= widget->dirty;
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        ituButtonSetPressed(btn, btn->pressed);
        result = widget->dirty = true;
    }
    else if (ev == ITU_EVENT_MOUSEMOVEUP)
    {
        result |= ituExecActions((ITUWidget*)btn, btn->actions, ev, 0);
    }
    else
    {
        /* do nothing */
    }
    return result;
}

void ituButtonChildPaint(ITUWidget* widget, ITUSurface* dest, int destx, int desty, ITUSurface* targetSurf, int alpha)
{
    if (widget != NULL)
    {
        do
        {
            ITCTree* node = widget->tree.child;
            for (; node; node = node->child)
            {
                ITUWidget* child = (ITUWidget*)node;
                if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
                    ituWidgetDraw(child, targetSurf, 0, 0, alpha);
                child->dirty = false;
            }
        } while (false);
    }
}

void ituButtonCalPos(ITUWidget* widget, int* imgdestx, int* imgdesty)
{
    ITUButton* btn = (ITUButton*)widget;
    ITUSurface* imgSurf;
    const ITURectangle* rect = (ITURectangle*)&widget->rect;
    int mw, mh;
    if (btn->pressed && btn->pressSurf)
    {
        imgSurf = btn->pressSurf;
    }
    else if (btn->pressed && btn->focusSurf)
    {
        imgSurf = btn->focusSurf;
    }
    else
    {
        imgSurf = btn->bg.icon.surf;
    }

    if (imgSurf == NULL)
    {
        return;
    }
    else
    {
        mw = (imgSurf->width >= rect->width) ? (rect->width) : (imgSurf->width);
        mh = (imgSurf->height >= rect->height) ? (rect->height) : (imgSurf->height);
    }

    if ((btn->imagepos == ITU_LAYOUT_TOP_LEFT) || (btn->imagepos < 2))
    {
        return;
    }

    if (widget->flags & ITU_STRETCH)
    {
        *imgdestx = 0;
        *imgdesty = 0;
    }
    else if (btn->imagepos == ITU_LAYOUT_TOP_CENTER)
    {
        *imgdestx += (rect->width - mw) / 2;
    }
    else if (btn->imagepos == ITU_LAYOUT_TOP_RIGHT)
    {
        *imgdestx += (rect->width - mw);
    }
    else if (btn->imagepos == ITU_LAYOUT_MIDDLE_LEFT)
    {
        *imgdesty += (rect->height - mh) / 2;
    }
    else if (btn->imagepos == ITU_LAYOUT_MIDDLE_RIGHT)
    {
        *imgdestx += (rect->width - mw);
        *imgdesty += (rect->height - mh) / 2;
    }
    else if (btn->imagepos == ITU_LAYOUT_BOTTOM_LEFT)
    {
        *imgdesty += (rect->height - mh);
    }
    else if (btn->imagepos == ITU_LAYOUT_BOTTOM_CENTER)
    {
        *imgdestx += (rect->width - mw) / 2;
        *imgdesty += (rect->height - mh);
    }
    else if (btn->imagepos == ITU_LAYOUT_BOTTOM_RIGHT)
    {
        *imgdestx += (rect->width - mw);
        *imgdesty += (rect->height - mh);
    }
    else // default middle_center
    {
        *imgdestx += (rect->width - mw) / 2;
        *imgdesty += (rect->height - mh) / 2;
    }
}

void ituButtonDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    uint8_t desta;
    ITURectangle prevClip;
    ITUButton* btn = (ITUButton*)widget;
    ITUIcon* icon = &btn->bg.icon;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    assert(btn);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255;

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] ButtonDraw but SurfacePool not ready yet!\n");
        return;
    }

    if (widget->visible && (widget->alpha > 0) && (alpha > 0))
    {
        if ((btn->pressed && btn->staticPressSurf) || (widget->active && btn->staticFocusSurf))
        {
            ituSceneSetD2D(ituScene, widget);
            if (widget->flags & ITU_EXTERNAL)
            {
                ButtonLoadExternalData(btn, NULL);
            }
            else
            {
                ituButtonLoadStaticData(btn);
            }
        }
        else if ((btn->imagepos != ITU_LAYOUT_TOP_LEFT) && (btn->imagepos > 1))
        {
            ituSceneSetD2D(ituScene, widget);
            if (icon->staticSurf)
            {
                if (widget->flags & ITU_EXTERNAL)
                {
                    ituIconLoadExternalDataAPI(icon);
                }
                else
                {
                    ituIconLoadStaticData(icon);
                }
            }
        }
    }
#endif

    //check mask link here
    if (icon)
    {
        if (!(icon->iconflags & ITUICON_CORNER_MASK_ACTIVE))
        {
            ituCornerRender(widget, NULL, NULL, NULL, NULL, NULL);
        }
    }

    if ((btn->pressed && (btn->pressSurf != NULL)) || (widget->active && (btn->focusSurf != NULL)))
    {
        ITUSurface* srcSurf = NULL;
        ITUSurface* bgSurf = NULL;

        if (widget->angle == 0)
        {
            ituWidgetSetClipping(widget, dest, x, y, &prevClip);
        }
        else
        {
            bgSurf = ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
            if (bgSurf != NULL)
            {
                ituBitBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, dest, destx, desty);
            }
        }

        if (btn->pressed && btn->pressSurf)
        {
            if ((btn->press_alpha_delay_count > 0) && (btn->press_alpha_delay_count < 255))
            {
                return;
            }

            srcSurf = btn->pressSurf;
        }
        else // widget->active == true
        {
            srcSurf = btn->focusSurf;
        }

        if (srcSurf != NULL)
        {
            if ((srcSurf->width < widget->rect.width || srcSurf->height < widget->rect.height) ||
                (srcSurf->format == ITU_ARGB1555 || srcSurf->format == ITU_ARGB4444 || srcSurf->format == ITU_ARGB8888))
            {
                if (desta > 0)
                {
                    ITUSurface* destSurf = dest;
                    int dx = destx, dy = desty;
                    if (bgSurf != NULL)
                    {
                        dx = dy = 0;
                        destSurf = bgSurf;
                    }
                    
                    if (desta == 255)
                    {
                        if (widget->color.alpha > 0)
                        {
                            ituColorFill(destSurf, dx, dy, rect->width, rect->height, &widget->color);
                        }
                    }
                    else
                    {
                        if (widget->color.alpha > 0)
                        {
	                        ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
	                        if (NULL != surf)
	                        {
                                ituColorFill(surf, 0, 0, rect->width, rect->height, &widget->color);
	                            ituAlphaBlend(destSurf, dx, dy, rect->width, rect->height, surf, 0, 0, desta);
	                            ituDestroySurface(surf);
	                        }
                    	}
                	}
            	}
        	}
        }

        if (srcSurf != NULL)
        {
            desta = alpha * widget->alpha / 255;
            if (desta > 0)
            {
                ITUSurface* destSurf = dest;
                int dx = destx, dy = desty;
                if (bgSurf != NULL)
                {
                    dx = dy = 0;
                    destSurf = bgSurf;
                }
                ituButtonCalPos(widget, &dx, &dy);

                if (desta == 255)
                {
                    if (widget->flags & ITU_STRETCH)
                    {
                        ituStretchBlt(destSurf, dx, dy, rect->width, rect->height, srcSurf, 0, 0, srcSurf->width,
                            srcSurf->height);
                    }
                    else
                    {
                        ituBitBlt(destSurf, dx, dy,
                            ((dx + srcSurf->width) <= destSurf->width) ? (srcSurf->width) : (destSurf->width - dx),
                            ((dy + srcSurf->height) <= destSurf->height) ? (srcSurf->height) : (destSurf->height - dy),
                            srcSurf, 0, 0);
                    }
                }
                else
                {
                    if (widget->flags & ITU_STRETCH)
                    {
                        ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                        if (NULL != surf)
                        {
                            ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, dx, dy);
                            ituStretchBlt(surf, 0, 0, rect->width, rect->height, srcSurf, 0, 0, srcSurf->width, srcSurf->height);
                            ituAlphaBlend(destSurf, dx, dy, rect->width, rect->height, surf, 0, 0, desta);
                            ituDestroySurface(surf);
                        }
                    }
                    else
                    {
                        ituAlphaBlend(destSurf, dx, dy,
                            ((dx + srcSurf->width) <= destSurf->width) ? (srcSurf->width) : (destSurf->width - dx),
                            ((dy + srcSurf->height) <= destSurf->height) ? (srcSurf->height) : (destSurf->height - dy),
                            srcSurf, 0, 0, desta);
                    }
                }

                if (bgSurf != NULL)
                {
                    ituButtonChildPaint(widget, dest, destx, desty, bgSurf, alpha);

                    if (widget->angle == 0)
                    {
                        ituBitBlt(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0);
                    }
                    else
                    {
#ifndef CFG_M2D_ENABLE
                        ituRotate(dest, destx + bgSurf->width / 2, desty + bgSurf->height / 2,
                            bgSurf, bgSurf->width / 2, bgSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
                        ituTransform(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0,
                            bgSurf->width, bgSurf->height, bgSurf->width / 2, bgSurf->height / 2, 1.0f,
                            1.0f, (float)widget->angle, 0, true, true, desta);
#endif
                    }
                    ituDestroySurface(bgSurf);
                }
                else
                {
                    ITCTree* node = widget->tree.child;
                    for (; node; node = node->child)
                    {
                        ITUWidget* child = (ITUWidget*)node;
                        if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
                        {
                            ituWidgetDraw(child, dest, destx, desty, desta);
                        }
                        child->dirty = false;
                    }
                }
            }

            if (widget->angle == 0)
            {
                ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
            }
        }
    }
    else
    {
        if ((icon->surf != NULL) && (btn->imagepos != ITU_LAYOUT_TOP_LEFT) && (btn->imagepos > 1))
        {
            int dx = 0, dy = 0;
            ITUSurface* bgSurf = ituCreateSurface(widget->rect.width, widget->rect.height, 0, ITU_ARGB8888, NULL, 0);
            if (bgSurf != NULL)
            {
                ituBitBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, dest, destx, desty);
                //for debug usage
                //ituBitBlt(dest, 0, 0, bgSurf->width, bgSurf->height, bgSurf, 0, 0);
            }
            else
            {
                return;
            }

            destx = rect->x + x;
            desty = rect->y + y;

#ifndef CFG_M2D_ENABLE
            if (widget->color.alpha > 0)
            {
                ITUSurface* mcSurf = ituCreateSurface(bgSurf->width, bgSurf->height, 0, bgSurf->format, NULL, 0);
                if (mcSurf != NULL)
                {
                    ituColorFill(mcSurf, 0, 0, mcSurf->width, mcSurf->height, &widget->color);
                    ituAlphaBlend(bgSurf, 0, 0, mcSurf->width, mcSurf->height, mcSurf, 0, 0, desta);
                    ituDestroySurface(mcSurf);
                }
            }
#else
            ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &widget->color);
#endif
            //Bless added to re-calculate position for current layout alignment
            ituButtonCalPos(widget, &dx, &dy);
#ifndef CFG_M2D_ENABLE
            if (desta < 255)
            {
                //ituAlphaBlend(bgSurf, dx, dy, icon->surf->width, icon->surf->height, icon->surf, 0, 0, desta);

                if (widget->flags & ITU_STRETCH)
                {
                    ITUSurface* surf = ituCreateSurface(bgSurf->width, bgSurf->height, 0, bgSurf->format, NULL, 0);
                    if (NULL != surf)
                    {
                        ituBitBlt(surf, 0, 0, surf->width, surf->height, dest, destx, desty);
                        ituStretchBlt(surf, 0, 0, surf->width, surf->height, icon->surf, 0, 0, icon->surf->width, icon->surf->height);
                        ituAlphaBlend(bgSurf, 0, 0, bgSurf->width, bgSurf->height, surf, 0, 0, desta);
                        ituDestroySurface(surf);
                    }
                }
                else
                {
                    ituAlphaBlend(bgSurf, dx, dy,
                        ((dx + icon->surf->width) <= bgSurf->width) ? (icon->surf->width) : (bgSurf->width - dx),
                        ((dy + icon->surf->height) <= bgSurf->height) ? (icon->surf->height) : (bgSurf->height - dy),
                        icon->surf, 0, 0, desta);
                }
            }
            else
            {
                //ituBitBlt(bgSurf, dx, dy, icon->surf->width, icon->surf->height, icon->surf, 0, 0);

                if (widget->flags & ITU_STRETCH)
                {
                    ituStretchBlt(bgSurf, dx, dy, bgSurf->width, bgSurf->height, 
                        icon->surf, 0, 0, icon->surf->width, icon->surf->height);
                }
                else
                {
                    ituBitBlt(bgSurf, dx, dy,
                        ((dx + icon->surf->width) <= bgSurf->width) ? (icon->surf->width) : (bgSurf->width - dx),
                        ((dy + icon->surf->height) <= bgSurf->height) ? (icon->surf->height) : (bgSurf->height - dy),
                        icon->surf, 0, 0);
                }
            }
#else
			if (widget->flags & ITU_STRETCH)
			{
				ituStretchBlt(bgSurf, dx, dy, bgSurf->width, bgSurf->height,
					icon->surf, 0, 0, icon->surf->width, icon->surf->height);
			}
			else
			{
				ituBitBlt(bgSurf, dx, dy,
					((dx + icon->surf->width) <= bgSurf->width) ? (icon->surf->width) : (bgSurf->width - dx),
					((dy + icon->surf->height) <= bgSurf->height) ? (icon->surf->height) : (bgSurf->height - dy),
					icon->surf, 0, 0);
			}
#endif

            ituButtonChildPaint(widget, dest, destx, desty, bgSurf, alpha);

            if (widget->angle == 0)
            {
                ituAlphaBlend(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0, desta);
            }
            else
            {
#ifndef CFG_M2D_ENABLE
                ituRotate(dest, destx + bgSurf->width / 2, desty + bgSurf->height / 2,
                    bgSurf, bgSurf->width / 2, bgSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
                ituTransform(dest, destx, desty, bgSurf->width, bgSurf->height, bgSurf, 0, 0,
                    bgSurf->width, bgSurf->height, bgSurf->width / 2, bgSurf->height / 2, 1.0f,
                    1.0f, (float)widget->angle, 0, true, true, desta);
#endif
            }
            //ituWidgetDrawImpl(widget, dest, x, y, alpha);
            ituDestroySurface(bgSurf);
        }
        else
        {
            ituBackgroundDraw(widget, dest, x, y, desta);
        }
    }
}

void ituButtonOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUButton* button = (ITUButton*) widget;
    assert(button);

    switch (action)
    {
    case ITU_ACTION_DODELAY0:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY0, atoi(param));
        break;

    case ITU_ACTION_DODELAY1:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY1, atoi(param));
        break;

    case ITU_ACTION_DODELAY2:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY2, atoi(param));
        break;

    case ITU_ACTION_DODELAY3:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY3, atoi(param));
        break;

    case ITU_ACTION_DODELAY4:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY4, atoi(param));
        break;

    case ITU_ACTION_DODELAY5:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY5, atoi(param));
        break;

    case ITU_ACTION_DODELAY6:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY6, atoi(param));
        break;

    case ITU_ACTION_DODELAY7:
        ituExecActions(widget, button->actions, ITU_EVENT_DELAY7, atoi(param));
        break;

    default:
        ituWidgetOnActionImpl(widget, action, param);
        break;
    }
}

void ituButtonInit(ITUButton* btn)
{
    assert(btn);
    ITU_ASSERT_THREAD();

    (void)memset(btn, 0, sizeof (ITUButton));

    ituBackgroundInit(&btn->bg);
    ituBorderWindowInit(&btn->bwin);
    ituTextInit(&btn->text);

    ituWidgetSetType(btn, ITU_BUTTON);
    ituWidgetSetName(btn, btnName);
    ituWidgetSetExit(btn, ituButtonExit);
    ituWidgetSetClone(btn, ituButtonClone);
    ituWidgetSetUpdate(btn, ituButtonUpdate);
    ituWidgetSetDraw(btn, ituButtonDraw);
    ituWidgetSetOnAction(btn, ituButtonOnAction);

    ituBorderWindowAdd(&btn->bwin, &btn->text.widget, ITU_LAYOUT_CENTER);
}

void ituButtonLoad(ITUButton* btn, uint32_t base)
{
    const ITUWidget* widget = (ITUWidget*)btn;
    ITUText* txt = &(btn->text);
    assert(btn);

    ituBackgroundLoad(&btn->bg, base);
    ituWidgetSetExit(btn, ituButtonExit);
    ituWidgetSetClone(btn, ituButtonClone);
    ituWidgetSetUpdate(btn, ituButtonUpdate);
    ituWidgetSetDraw(btn, ituButtonDraw);
    ituWidgetSetOnAction(btn, ituButtonOnAction);

    //workaround for no text rect size when Button Text use empty under DrawRocker.
    if (txt != NULL)
    {
        ITUWidget* txtWidget = &(txt->widget);
        if (txtWidget != NULL)
        {
            if (ituWidgetGetWidth(txtWidget) == 0)
            {
                txtWidget->rect.width = 1;
                txtWidget->rect.height = 1;
            }
        }
    }

    if (!(widget->flags & ITU_EXTERNAL))
    {
        if (btn->staticFocusSurf)
        {
            ITUSurface* surf = (ITUSurface*)(base + (uint32_t)btn->staticFocusSurf);
            if (surf->flags & ITU_COMPRESSED)
                btn->focusSurf = NULL;
            else
                btn->focusSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t*)surf->addr, surf->flags);

            btn->staticFocusSurf = surf;
        }

        if (btn->staticPressSurf)
        {
            ITUSurface* surf = (ITUSurface*)(base + (uint32_t)btn->staticPressSurf);
            if (surf->flags & ITU_COMPRESSED)
                btn->pressSurf = NULL;
            else
                btn->pressSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t*)surf->addr, surf->flags);

            btn->staticPressSurf = surf;
        }
    }

    switch (btn->text.layout)
    {
    case ITU_LAYOUT_TOP_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_TOP_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_TOP_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_UP] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_CENTER] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_DOWN] = &btn->text.widget;
        break;

    default:
        btn->bwin.widgets[btn->text.layout] = &btn->text.widget;
        break;
    }

    if (btn->mouseUpDelay > UINT32_MAX / 2)
    {
        btn->mouseUpDelay = (UINT32_MAX - btn->mouseUpDelay);
        btn->fsg2 = 1;
    }
}

void ituButtonSetPressed(ITUButton* btn, bool pressed)
{
    ITUWidget* widget = (ITUWidget*)btn;
    assert(btn);
    ITU_ASSERT_THREAD();

    if (pressed && btn->pressColor.alpha > 0)
    {
        ituWidgetSetColor(widget, btn->pressColor.alpha, btn->pressColor.red, btn->pressColor.green, btn->pressColor.blue);
    }
    else
    {
        bool withCheckedColor = false;
        if ((widget->type == ITU_CHECKBOX) && (!pressed))
        {
            ITUCheckBox* checkbox = (ITUCheckBox*)widget;
            if (checkbox->checked && (checkbox->checkedColor.alpha > 0))
            {
                withCheckedColor = true;
            }
        }

        if (withCheckedColor)
        {
            ITUCheckBox* checkbox = (ITUCheckBox*)widget;
            ituWidgetSetColor(widget, checkbox->checkedColor.alpha, checkbox->checkedColor.red, checkbox->checkedColor.green, checkbox->checkedColor.blue);
        }
        else
        {
            if (widget->active && btn->focusColor.alpha > 0)
            {
                ituWidgetSetColor(widget, btn->focusColor.alpha, btn->focusColor.red, btn->focusColor.green, btn->focusColor.blue);
            }
            else
            {
                ituWidgetSetColor(widget, btn->bgColor.alpha, btn->bgColor.red, btn->bgColor.green, btn->bgColor.blue);
            }
        }
    }
    btn->pressed = (pressed) ? (1) : (0);
    widget->dirty = true;
}

void ituButtonSetStringImpl(ITUButton* btn, char* string)
{
    assert(btn);
    ITU_ASSERT_THREAD();

    ituTextSetString(&btn->text, string);
    ituWidgetUpdate(btn, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituButtonLoadExternalDataAPI(ITUButton* btn, ITULayer* layer)
{
    ButtonLoadExternalData(btn, layer);
}

void ituButtonLoadStaticData(ITUButton* btn)
{
    const ITUWidget* widget = (ITUWidget*)btn;
    ITUSurface* surf;
    bool drawReq = ituSceneClearD2D(ituScene);

    if (widget->flags & ITU_EXTERNAL)
        return;

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (!btn->staticPressSurf && btn->pressed)
        return;
    else if (btn->pressed) {
#else
    if (btn->staticPressSurf || !btn->pressSurf)
    {
#endif

        surf = btn->staticPressSurf;
        if (NULL != surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                ituSceneCheckD2D(ituScene, drawReq);
                btn->pressSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                btn->pressSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                  (const uint8_t *)surf->addr, surf->flags);
            }
        }
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        return;
#endif
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if ((!btn->staticFocusSurf && widget->active) || (!widget->active))
    {
        return;
    }
#else
    if (!btn->staticFocusSurf || btn->focusSurf)
    {
        return;
    }
#endif

    surf = btn->staticFocusSurf;
    if (surf->flags & ITU_COMPRESSED)
    {
        ituSceneCheckD2D(ituScene, drawReq);
        btn->focusSurf = ituSurfaceDecompress(surf);
    }
    else
    {
        btn->focusSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                          (const uint8_t *)surf->addr, surf->flags);
    }
}

void ituButtonReleaseSurface(ITUButton* btn)
{
    ITUWidget* widget = (ITUWidget*)btn;
    ITULayer*  layer  =             NULL;

    ITU_ASSERT_THREAD();

    if (btn->pressSurf)
    {
        ituSurfaceRelease(btn->pressSurf);
        btn->pressSurf = NULL;

        if (widget->flags & ITU_EXTERNAL)
        {
            layer = ituGetLayer(widget);

            if (btn->staticPressSurf)
            {
                if (layer != NULL)
                {
                    ituLayerReleaseExternalSurface(layer);
                }
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                if (widget->flags & ITU_EXTERNAL_IN_PLACE)
                {
                    widget->flags &= ~ITU_EXTERNAL_IN_PLACE;
                }
#endif
            }
        }
    }

    if (btn->focusSurf)
    {
        ituSurfaceRelease(btn->focusSurf);
        btn->focusSurf = NULL;

        if (widget->flags & ITU_EXTERNAL)
        {
            if (layer == NULL)
            {
                layer = ituGetLayer(widget);
            }

            if (btn->staticFocusSurf)
            {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                if (widget->flags & ITU_EXTERNAL_IN_PLACE)
                {
                    widget->flags &= ~ITU_EXTERNAL_IN_PLACE;
                    if (layer != NULL)
                    {
                        ituLayerReleaseExternalSurface(layer);
                    }
                }
#else
                if (layer != NULL)
                {
                    ituLayerReleaseExternalSurface(layer);
                }
#endif
            }
        }
    }
}

void ituButtonSetTextLayoutImpl(ITUButton* btn, ITULayout layout)
{
    int i;
    assert(btn);
    ITU_ASSERT_THREAD();

    for (i = 0; i < 5; ++i)
    {
        btn->bwin.widgets[i] = NULL;
    }

    switch (layout)
    {
    case ITU_LAYOUT_TOP_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_TOP_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_TOP_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_UP] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_MIDDLE_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_CENTER] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_LEFT:
        btn->bwin.widgets[ITU_LAYOUT_LEFT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_RIGHT:
        btn->bwin.widgets[ITU_LAYOUT_RIGHT] = &btn->text.widget;
        break;

    case ITU_LAYOUT_BOTTOM_CENTER:
        btn->bwin.widgets[ITU_LAYOUT_DOWN] = &btn->text.widget;
        break;

    default:
        btn->bwin.widgets[layout] = &btn->text.widget;
        break;
    }
    btn->text.layout = layout;
    ituWidgetUpdate(btn, ITU_EVENT_LAYOUT, 0, 0, 0);
}

//void ituButtonSetCheckStat(ITUButton* btn, bool ckstat)
//{
//  assert(btn);
//  ITU_ASSERT_THREAD();
//
//}

void ituButtonSetSlideMouseUP(ITUButton* btn, bool onoff)
{
    assert(btn);
    ITU_ASSERT_THREAD();
    if (onoff)
    {
        btn->fsg1 |= ITU_BUTTON_FLAG_SLIDE_MOUSEUP;
    }
    else
    {
        btn->fsg1 &= ~ITU_BUTTON_FLAG_SLIDE_MOUSEUP;
    }
}

void ituButtonSetStyleIndex(ITUButton* btn, int index)
{
    assert(btn);
    ITU_ASSERT_THREAD();
    if (btn && (index >= 0) && (index < 16))
    {
        btn->ImgTargetIndex = index;
        //for default image target style
        if (!btn->bg.icon.surf && !btn->dImgILB && (btn->dImgILBName[0] != '\0'))
        {
            btn->dImgILB = ituSceneFindWidget(ituScene, btn->dImgILBName);
        }
        if (btn->dImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)btn->dImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[btn->ImgTargetIndex])
            {
                btn->bg.icon.staticSurf = ilb->staticSurfArray[btn->ImgTargetIndex];
                btn->bg.icon.surf = ilb->surfArray[btn->ImgTargetIndex];
            }
        }
        //for focus image target style
        if (!btn->focusSurf && !btn->fImgILB && (btn->fImgILBName[0] != '\0'))
        {
            btn->fImgILB = ituSceneFindWidget(ituScene, btn->fImgILBName);
        }
        if (btn->fImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)btn->fImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[btn->ImgTargetIndex])
            {
                btn->staticFocusSurf = ilb->staticSurfArray[btn->ImgTargetIndex];
                btn->focusSurf = ilb->surfArray[btn->ImgTargetIndex];
            }
        }
        //for press image target style
        if (!btn->pressSurf && !btn->pImgILB && (btn->pImgILBName[0] != '\0'))
        {
            btn->pImgILB = ituSceneFindWidget(ituScene, btn->pImgILBName);
        }
        if (btn->pImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)btn->pImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[btn->ImgTargetIndex])
            {
                btn->staticPressSurf = ilb->staticSurfArray[btn->ImgTargetIndex];
                btn->pressSurf = ilb->surfArray[btn->ImgTargetIndex];
            }
        }
    }
}

void ituButtonSetTopCheck(ITUWidget* widget, bool onoff)
{
    if (widget)
    {
        if (onoff)
        {
            widget->flags |= ITU_MOUSE_EV_TOPCHECK;
        }
        else
        {
            widget->flags &= ~ITU_MOUSE_EV_TOPCHECK;
        }
    }
}

void ituButtonSetOutSideForceMouseUP(ITUButton* btn, bool onoff)
{
    assert(btn);
    ITU_ASSERT_THREAD();
    if (onoff)
    {
        btn->fsg1 |= ITU_BUTTON_FLAG_OUTSIDE_FORCE_MOUSEUP;
    }
    else
    {
        btn->fsg1 &= ~ITU_BUTTON_FLAG_OUTSIDE_FORCE_MOUSEUP;
    }
}//ITU_BUTTON_FLAG_OUTSIDE_FORCE_MOUSEUPED
