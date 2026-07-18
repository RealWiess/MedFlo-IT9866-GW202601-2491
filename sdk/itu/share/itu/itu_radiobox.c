#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char radioboxName[] = "ITURadioBox";

bool ituRadioBoxClone (ITUWidget * widget, ITUWidget ** cloned)
{
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITURadioBox));
        if (newWidget == NULL)
        {
            return false;
        }

        /*
         * Note: Although the parameter 'widget' is declared as type ITUWidget*,
         * this function is only intended to be called with a pointer to an
         * ITURadioBox object. Since ITURadioBox is a struct that extends
         * ITUWidget (i.e., ITUWidget is the first member), it is safe to cast
         * or copy the memory using sizeof(ITURadioBox).
         *
         * Some static analysis tools may issue a warning here, suggesting that
         * copying sizeof(ITURadioBox) from a base type pointer (ITUWidget*) is
         * suspicious. However, in our design, we guarantee that 'widget' always
         * points to a valid ITURadioBox instance, so this warning is considered
         * a false positive and can be ignored.
         */
        /* coverity[SIZEOF_MISMATCH] */
        (void)memcpy(newWidget, widget, sizeof(ITURadioBox));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    return ituCheckBoxClone(widget, cloned);
}

bool ituRadioBoxUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool          result       = false;
    ITUButton *   btn          = (ITUButton *)widget;
    ITURadioBox * radiobox     = (ITURadioBox *)widget;
    ITUEvent      response_key = (widget->flags & ITU_RESPONSE_TO_UP_KEY) ? (ITU_EVENT_MOUSEUP) : (ITU_EVENT_MOUSEDOWN);
    assert(radiobox);

    if (ev == ITU_EVENT_LAYOUT)
    {
        ituRadioBoxSetStyleIndex(radiobox, radiobox->checkbox.ImgTargetIndex);
        result = widget->dirty = true;
    }
    else if (ev == response_key)
    {
        result |= ituIconUpdate(widget, ev, arg1, arg2, arg3);

        if (ituWidgetIsEnabled(widget) && !result)
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            if (ev == ITU_EVENT_MOUSEDOWN)
            {
                if (ituWidgetIsInside(widget, x, y))
                {
                    ituButtonSetPressed(btn, true);
                    ituFocusWidget(radiobox);
                    ituRadioBoxSetChecked(radiobox, true);
                    result |= ituWidgetOnPress(widget, ev, arg1, x, y);
                    result |= ituExecActions((ITUWidget *)btn, btn->actions, ev, 0);
                    result |= widget->dirty;
                }
            }
            else if (ev == ITU_EVENT_MOUSEUP)
            {
                if (btn->pressed)
                {
                    ituButtonSetPressed(btn, false);

                    if (ituWidgetIsInside(widget, x, y))
                    {
                        ituFocusWidget(radiobox);
                        ituRadioBoxSetChecked(radiobox, true);
                        result |= ituWidgetOnPress(widget, ev, arg1, x, y);
                        result |= ituExecActions((ITUWidget *)btn, btn->actions, ev, 0);
                        result |= widget->dirty;
                    }
                }
            }
        }
        return widget->visible ? result : false;
    }

    result |= ituCheckBoxUpdate(widget, ev, arg1, arg2, arg3);

    if (ituWidgetIsActive(widget) && ituWidgetIsEnabled(widget) && !result)
    {
        result |= ituButtonUpdate(widget, ev, arg1, arg2, arg3);

        response_key = (widget->flags & ITU_RESPONSE_TO_UP_KEY) ? (ITU_EVENT_KEYUP) : (ITU_EVENT_KEYDOWN);

        if (ev == response_key)
        {
            if (arg1 == ituScene->enterKey)
            {
                ituFocusWidget(radiobox);
                ituRadioBoxSetChecked(radiobox, true);
                result |= widget->dirty;
            }
        }
        return widget->visible ? result : false;
    }
    return widget->visible ? result : false;
}

void ituRadioBoxOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    ITURadioBox * radiobox = (ITURadioBox *)widget;
    assert(radiobox);

    switch (action)
    {
        case ITU_ACTION_CHECK:
            ituRadioBoxSetChecked(radiobox, true);
            break;

        case ITU_ACTION_UNCHECK:
            ituRadioBoxSetChecked(radiobox, false);
            break;

        default:
            ituButtonOnAction(widget, action, param);
            break;
    }
}

void ituRadioBoxInit (ITURadioBox * radiobox)
{
    assert(radiobox);
    ITU_ASSERT_THREAD();

    (void)memset(radiobox, 0, sizeof(ITURadioBox));

    ituCheckBoxInit(&radiobox->checkbox);

    ituWidgetSetType(radiobox, ITU_RADIOBOX);
    ituWidgetSetName(radiobox, radioboxName);
    ituWidgetSetClone(radiobox, ituRadioBoxClone);
    ituWidgetSetUpdate(radiobox, ituRadioBoxUpdate);
    ituWidgetSetOnAction(radiobox, ituRadioBoxOnAction);
}

void ituRadioBoxLoad (ITURadioBox * radiobox, uint32_t base)
{
    assert(radiobox);

    ituCheckBoxLoad(&radiobox->checkbox, base);

    ituWidgetSetClone(radiobox, ituRadioBoxClone);
    ituWidgetSetUpdate(radiobox, ituRadioBoxUpdate);
    ituWidgetSetOnAction(radiobox, ituRadioBoxOnAction);
}

void ituRadioBoxSetChecked (ITURadioBox * radiobox, bool checked)
{
    const ITCTree * parent = ((ITCTree *)radiobox)->parent;
    assert(radiobox);
    ITU_ASSERT_THREAD();

    if (parent)
    {
        ITCTree * node;

        for (node = parent->child; node; node = node->sibling)
        {
            ITUWidget * child = (ITUWidget *)node;

            if (child == (ITUWidget *)radiobox)
            {
                continue;
            }

            if (child->type == ITU_RADIOBOX || child->type == ITU_POPUPRADIOBOX)
            {
                ITURadioBox * rb = (ITURadioBox *)child;
                if (ituRadioBoxIsChecked(rb))
                {
                    ituCheckBoxSetChecked(&rb->checkbox, false);
                }
            }
        }
    }
    ituCheckBoxSetChecked(&radiobox->checkbox, checked);
}

bool ituRadioBoxIsChecked (ITURadioBox * radiobox)
{
    assert(radiobox);
    ITU_ASSERT_THREAD();
    return radiobox->checkbox.checked;
}

void ituRadioBoxSetStyleIndex (ITURadioBox * radiobox, int index)
{
    assert(radiobox);
    ITU_ASSERT_THREAD();
    if (radiobox && (index >= 0) && (index < 16))
    {
        ITUCheckBox * chk   = (ITUCheckBox *)radiobox;
        ITUButton *   btn   = &chk->btn;
        chk->ImgTargetIndex = index;
        // for default image target style
        if (!btn->bg.icon.surf && !chk->dImgILB && (chk->dImgILBName[0] != '\0'))
        {
            chk->dImgILB = ituSceneFindWidget(ituScene, chk->dImgILBName);
        }
        if (chk->dImgILB)
        {
            ITUIconListBox * ilb = (ITUIconListBox *)chk->dImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                btn->bg.icon.staticSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->bg.icon.surf       = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        // for focus image target style
        if (!btn->focusSurf && !chk->fImgILB && (chk->fImgILBName[0] != '\0'))
        {
            chk->fImgILB = ituSceneFindWidget(ituScene, chk->fImgILBName);
        }
        if (chk->fImgILB)
        {
            ITUIconListBox * ilb = (ITUIconListBox *)chk->fImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[chk->ImgTargetIndex])
            {
                btn->staticFocusSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->focusSurf       = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        // for press image target style
        if (!btn->pressSurf && !chk->pImgILB && (chk->pImgILBName[0] != '\0'))
        {
            chk->pImgILB = ituSceneFindWidget(ituScene, chk->pImgILBName);
        }
        if (chk->pImgILB)
        {
            ITUIconListBox * ilb = (ITUIconListBox *)chk->pImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[chk->ImgTargetIndex])
            {
                btn->staticPressSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->pressSurf       = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        // for checked image target style
        if (!chk->checkedSurf && !chk->cImgILB && (chk->cImgILBName[0] != '\0'))
        {
            chk->cImgILB = ituSceneFindWidget(ituScene, chk->cImgILBName);
        }
        if (chk->cImgILB)
        {
            ITUIconListBox * ilb = (ITUIconListBox *)chk->cImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                chk->staticCheckedSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                chk->checkedSurf       = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        // for focuschecked image target style
        if (!chk->focusCheckedSurf && !chk->fcImgILB && (chk->fcImgILBName[0] != '\0'))
        {
            chk->fcImgILB = ituSceneFindWidget(ituScene, chk->fcImgILBName);
        }
        if (chk->fcImgILB)
        {
            ITUIconListBox * ilb = (ITUIconListBox *)chk->fcImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                chk->staticFocusCheckedSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                chk->focusCheckedSurf       = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
    }
}
