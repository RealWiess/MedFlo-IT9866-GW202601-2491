#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char checkboxName[] = "ITUCheckBox";

void ituCheckBoxExit(ITUWidget* widget)
{
    ITUCheckBox* checkbox = (ITUCheckBox*) widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (checkbox->focusCheckedSurf)
    {
        ituSurfaceRelease(checkbox->focusCheckedSurf);
        checkbox->focusCheckedSurf = NULL;
    }

    if (checkbox->checkedSurf)
    {
        ituSurfaceRelease(checkbox->checkedSurf);
        checkbox->checkedSurf = NULL;
    }
    ituButtonExit(widget);
}

bool ituCheckBoxClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUCheckBox* checkbox = (ITUCheckBox*)widget;
    ITUCheckBox* newCheckBox;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget* newWidget = malloc(sizeof(ITUCheckBox));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUCheckBox));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned = newWidget;
    }

    newCheckBox = (ITUCheckBox*)*cloned;

    if (!(widget->flags & ITU_EXTERNAL)) //fixed when use external can't get correct surf address when clone
    {
        ITUSurface* surf = checkbox->staticCheckedSurf;

        if (surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                newCheckBox->checkedSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                newCheckBox->checkedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                            (const uint8_t *)surf->addr, surf->flags);
            }
        }

        surf = checkbox->staticFocusCheckedSurf;

        if (surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                newCheckBox->focusCheckedSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                newCheckBox->focusCheckedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                                 (const uint8_t *)surf->addr, surf->flags);
            }
        }
    }

    return ituButtonClone(widget, cloned);
}

static void CheckBoxLoadExternalData(ITUCheckBox* checkbox, ITULayer* layer)
{
    ITUWidget* widget = (ITUWidget*)checkbox;
    ITUSurface* surf;
    bool drawReq = ituSceneClearD2D(ituScene);

    assert(widget);

    if (!(widget->flags & ITU_EXTERNAL))
    {
        return;
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (widget->active && checkbox->checked && checkbox->staticFocusCheckedSurf)
    {
#else
    if (checkbox->staticFocusCheckedSurf && !checkbox->focusCheckedSurf)
    {
#endif
        if (!layer)
        {
            layer = ituGetLayer(widget);
        }

        surf = ituLayerLoadExternalSurface(layer, (uint32_t)checkbox->staticFocusCheckedSurf);
        if (surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                ituSceneCheckD2D(ituScene, drawReq);
                checkbox->focusCheckedSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                checkbox->focusCheckedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
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
    if (!checkbox->staticCheckedSurf || !checkbox->checked)
    {
        return;
    }
#else
    if (!checkbox->staticCheckedSurf || checkbox->checkedSurf)
    {
        return;
    }
#endif

    if (!layer)
    {
        layer = ituGetLayer(widget);
    }

    surf = ituLayerLoadExternalSurface(layer, (uint32_t)checkbox->staticCheckedSurf);
    if (surf)
    {
        if (surf->flags & ITU_COMPRESSED)
        {
            ituSceneCheckD2D(ituScene, drawReq);
            checkbox->checkedSurf = ituSurfaceDecompress(surf);
        }
        else
        {
            checkbox->checkedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
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

void ituCheckBoxLoadExternalDataAPI (ITUCheckBox * checkbox)
{
    CheckBoxLoadExternalData(checkbox, NULL);
}

bool ituCheckBoxUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool          result       = false;
    ITUButton *   btn          = (ITUButton *)widget;
    ITUCheckBox * checkbox     = (ITUCheckBox *)widget;
    ITUEvent      response_key = (widget->flags & ITU_RESPONSE_TO_UP_KEY) ? (ITU_EVENT_MOUSEUP) : (ITU_EVENT_MOUSEDOWN);
    assert(checkbox);

    if (ev == response_key)
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
                    ituFocusWidget(checkbox);
                    ituCheckBoxSetChecked(checkbox, !checkbox->checked);
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
                        ituFocusWidget(btn);
                        ituCheckBoxSetChecked(checkbox, !checkbox->checked);
                        ituWidgetOnPress(widget, ev, arg1, x, y);
                        result |= ituExecActions((ITUWidget *)btn, btn->actions, ev, 0);
                        result |= widget->dirty;
                    }
                }
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        result |= ituButtonUpdate(widget, ev, arg1, arg2, arg3);
    }

    if (ev == ITU_EVENT_LAYOUT)
    {
        ituCheckBoxSetStyleIndex(checkbox, checkbox->ImgTargetIndex);
        ituCheckBoxSetChecked(checkbox, checkbox->checked);
        result = widget->dirty = true;
    }
    else if (ev == ITU_EVENT_LOAD)
    {
        ituCheckBoxLoadStaticData(checkbox);
        result = true;
    }
    else if (ev == ITU_EVENT_LOAD_EXTERNAL)
    {
        CheckBoxLoadExternalData(checkbox, (ITULayer*)arg1);
        result = true;
    }
    else if (ev == ITU_EVENT_RELEASE)
    {
        ituCheckBoxReleaseSurface(checkbox);
        result = true;
    }
    else if (ituWidgetIsActive(widget) && ituWidgetIsEnabled(widget) && !result)
    {
        response_key = (widget->flags & ITU_RESPONSE_TO_UP_KEY) ? (ITU_EVENT_KEYUP) : (ITU_EVENT_KEYDOWN);

        if (ev == response_key)
        {
            if (arg1 == ituScene->enterKey)
            {
                ituFocusWidget(checkbox);
                ituCheckBoxSetChecked(checkbox, !checkbox->checked);
                result |= widget->dirty;
            }
        }
    }
    else
    {
        /* do nothing */
    }
    return widget->visible ? result : false;
}

void ituCheckBoxColorFill(ITUSurface* dest, int destx, int desty, int width, int height, ITUColor* color, uint8_t desta)
{
    if ((desta > 0) && (color->alpha > 0))
    {
        if (desta == 255)
        {
            ituColorFill(dest, destx, desty, width, height, color);
        }
        else
        {
            ITUSurface* surf = ituCreateSurface(width, height, 0, dest->format, NULL, 0);
            if (surf)
            {
                ituColorFill(surf, 0, 0, width, height, color);
                ituAlphaBlend(dest, destx, desty, width, height, surf, 0, 0, desta);
                ituDestroySurface(surf);
            }
        }
    }
}

void ituCheckBoxDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    uint8_t desta;
    ITURectangle prevClip;
    ITUCheckBox* checkbox = (ITUCheckBox*)widget;
    ITURectangle* rect = (ITURectangle*)&widget->rect;
    assert(checkbox);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->alpha / 255;

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] CheckBoxDraw but SurfacePool not ready yet!\n");
        return;
    }

    if (widget->visible && (widget->alpha > 0) && (alpha > 0))
    {
        if (checkbox->checked)
        {
            ituSceneSetD2D(ituScene, widget);
            if (widget->flags & ITU_EXTERNAL)
            {
                CheckBoxLoadExternalData(checkbox, NULL);
            }
            else
            {
                ituCheckBoxLoadStaticData(checkbox);
            }
        }
    }
#endif

    if ((checkbox->checked && checkbox->checkedSurf) || (widget->active && checkbox->checked && checkbox->focusCheckedSurf))
    {
        ITUSurface* srcSurf = NULL;

        ituWidgetSetClipping(widget, dest, x, y, &prevClip);

        if (checkbox->checked)
        {
            if (widget->active && checkbox->focusCheckedSurf)
            {
                srcSurf = checkbox->focusCheckedSurf;
            }
            else
            {
                srcSurf = checkbox->checkedSurf;
            }
        }

        if (srcSurf != NULL)
        {
            if ((srcSurf->width < widget->rect.width || srcSurf->height < widget->rect.height) &&
                (srcSurf->format == ITU_ARGB1555 || srcSurf->format == ITU_ARGB4444 || srcSurf->format == ITU_ARGB8888))
            {
                ituCheckBoxColorFill(dest, destx, desty, rect->width, rect->height, &widget->color, desta);
            }
        }
        else
        {
            ituCheckBoxColorFill(dest, destx, desty, rect->width, rect->height, &widget->color, desta);
        }

        if (srcSurf != NULL)
        {
            desta = alpha * widget->alpha / 255;
            if (desta > 0)
            {
                if (desta == 255)
                {
                    if (widget->angle == 0)
                    {
                        if (widget->flags & ITU_STRETCH)
                        {
                            ituStretchBlt(dest, destx, desty, rect->width, rect->height, srcSurf, 0, 0, srcSurf->width,
                                          srcSurf->height);
                        }
                        else
                        {
                            ituBitBlt(dest, destx, desty, srcSurf->width, srcSurf->height, srcSurf, 0, 0);
                        }
                    }
                    else
                    {
#ifndef CFG_M2D_ENABLE
                        ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, srcSurf, srcSurf->width / 2, srcSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
                        ituRotate(dest, destx, desty, srcSurf, srcSurf->width / 2, srcSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#endif
                    }
                }
                else
                {
                    if (widget->flags & ITU_STRETCH)
                    {
                        ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                        if (surf)
                        {
                            ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);
                            ituStretchBlt(surf, 0, 0, rect->width, rect->height, srcSurf, 0, 0, srcSurf->width, srcSurf->height);
                            ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                            ituDestroySurface(surf);
                        }
                    }
                    else
                    {
                        ituAlphaBlend(dest, destx, desty, srcSurf->width, srcSurf->height, srcSurf, 0, 0, desta);
                    }
                }

            }
        }
        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
        ituWidgetDrawImpl(widget, dest, x, y, alpha);
    }
    else
    {
        ituButtonDraw(widget, dest, x, y, desta);
    }
}

void ituCheckBoxOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUCheckBox* checkbox = (ITUCheckBox*) widget;
    assert(checkbox);

    switch (action)
    {
    case ITU_ACTION_CHECK:
        ituCheckBoxSetChecked(checkbox, true);
        break;

    case ITU_ACTION_UNCHECK:
        ituCheckBoxSetChecked(checkbox, false);
        break;

    default:
        ituButtonOnAction(widget, action, param);
        break;
    }
}

void ituCheckBoxInit(ITUCheckBox* checkbox)
{
    assert(checkbox);
    ITU_ASSERT_THREAD();

    (void)memset(checkbox, 0, sizeof (ITUCheckBox));

    ituButtonInit(&checkbox->btn);

    ituWidgetSetType(checkbox, ITU_CHECKBOX);
    ituWidgetSetName(checkbox, checkboxName);
    ituWidgetSetExit(checkbox, ituCheckBoxExit);
    ituWidgetSetClone(checkbox, ituCheckBoxClone);
    ituWidgetSetUpdate(checkbox, ituCheckBoxUpdate);
    ituWidgetSetDraw(checkbox, ituCheckBoxDraw);
    ituWidgetSetOnAction(checkbox, ituCheckBoxOnAction);
}

void ituCheckBoxLoad(ITUCheckBox* checkbox, uint32_t base)
{
    const ITUWidget* widget = (ITUWidget*)checkbox;
    assert(checkbox);

    ituButtonLoad(&checkbox->btn, base);
    ituWidgetSetExit(checkbox, ituCheckBoxExit);
    ituWidgetSetClone(checkbox, ituCheckBoxClone);
    ituWidgetSetUpdate(checkbox, ituCheckBoxUpdate);
    ituWidgetSetDraw(checkbox, ituCheckBoxDraw);
    ituWidgetSetOnAction(checkbox, ituCheckBoxOnAction);

    if (!(widget->flags & ITU_EXTERNAL))
    {
        if (checkbox->staticCheckedSurf)
        {
            ITUSurface* surf = (ITUSurface*)(base + (uint32_t)checkbox->staticCheckedSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                checkbox->checkedSurf = NULL;
            }
            else
            {
                checkbox->checkedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                         (const uint8_t *)surf->addr, surf->flags);
            }

            checkbox->staticCheckedSurf = surf;
        }

        if (checkbox->staticFocusCheckedSurf)
        {
            ITUSurface* surf = (ITUSurface*)(base + (uint32_t)checkbox->staticFocusCheckedSurf);
            if (surf->flags & ITU_COMPRESSED)
            {
                checkbox->focusCheckedSurf = NULL;
            }
            else
            {
                checkbox->focusCheckedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                              (const uint8_t *)surf->addr, surf->flags);
            }

            checkbox->staticFocusCheckedSurf = surf;
        }
    }
}

void ituCheckBoxSetChecked(ITUCheckBox* checkbox, bool checked)
{
    ITUWidget* widget = (ITUWidget*) checkbox;
    ITUText* text = &checkbox->btn.text;
    assert(checkbox);
    ITU_ASSERT_THREAD();

    if (checked && checkbox->checkedFontColor.alpha > 0)
    {
        ituWidgetSetColor(text, checkbox->checkedFontColor.alpha, checkbox->checkedFontColor.red,
                          checkbox->checkedFontColor.green, checkbox->checkedFontColor.blue);
    }
    else
    {
        ituWidgetSetColor(text, checkbox->orgFontColor.alpha, checkbox->orgFontColor.red, checkbox->orgFontColor.green,
                          checkbox->orgFontColor.blue);
    }

    if (checked && checkbox->checkedColor.alpha > 0)
    {
        ituWidgetSetColor(widget, checkbox->checkedColor.alpha, checkbox->checkedColor.red,
                          checkbox->checkedColor.green, checkbox->checkedColor.blue);
    }
    else
    {
        ituWidgetSetColor(widget, checkbox->btn.bgColor.alpha, checkbox->btn.bgColor.red, checkbox->btn.bgColor.green,
                          checkbox->btn.bgColor.blue);
    }

    if (widget->active && checkbox->btn.focusColor.alpha > 0)
    {
        ituWidgetSetColor(widget, checkbox->btn.focusColor.alpha, checkbox->btn.focusColor.red,
                          checkbox->btn.focusColor.green, checkbox->btn.focusColor.blue);
    }

    checkbox->checked = checked;
    widget->dirty = true;
}

bool ituCheckBoxIsChecked(ITUCheckBox* checkbox)
{
    assert(checkbox);
    ITU_ASSERT_THREAD();
    return checkbox->checked;
}

void ituCheckBoxLoadStaticData(ITUCheckBox* checkbox)
{
    ITUWidget* widget = (ITUWidget*)checkbox;
    ITUSurface* surf;

    ITULayer* layer = ituGetLayer(widget);
    bool drawReq = ituSceneClearD2D(ituScene);

    if (widget->flags & ITU_EXTERNAL)
    {
        return;
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (layer)
    {
        if (layer->widget.flags & ITU_EXTERNAL)
        {
            return;
        }
    }

    if (widget->active && checkbox->checked && checkbox->staticFocusCheckedSurf)
    {
#else
    if (checkbox->staticFocusCheckedSurf || !checkbox->focusCheckedSurf)
    {
#endif

        surf = checkbox->staticFocusCheckedSurf;
        if (surf)
        {
            if (surf->flags & ITU_COMPRESSED)
            {
                ituSceneCheckD2D(ituScene, drawReq);
                checkbox->focusCheckedSurf = ituSurfaceDecompress(surf);
            }
            else
            {
                checkbox->focusCheckedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                              (const uint8_t *)surf->addr, surf->flags);
            }
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
            return;
#endif
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (!checkbox->staticCheckedSurf || !checkbox->checked)
    {
        return;
    }
#else
    if (!checkbox->staticCheckedSurf || checkbox->checkedSurf)
    {
        return;
    }
#endif

    surf = checkbox->staticCheckedSurf;
    if (surf)
    {
        if (surf->flags & ITU_COMPRESSED)
        {
            ituSceneCheckD2D(ituScene, drawReq);
            checkbox->checkedSurf = ituSurfaceDecompress(surf);
        }
        else
        {
            checkbox->checkedSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format,
                                                     (const uint8_t *)surf->addr, surf->flags);
        }
    }
}

void ituCheckBoxReleaseSurface(ITUCheckBox* checkbox)
{
    ITUWidget* widget = (ITUWidget*)checkbox;
    ITULayer* layer = NULL;
    ITU_ASSERT_THREAD();

    if (checkbox->focusCheckedSurf)
    {
        ituSurfaceRelease(checkbox->focusCheckedSurf);
        checkbox->focusCheckedSurf = NULL;

        if (widget->flags & ITU_EXTERNAL)
        {
            layer = ituGetLayer(widget);

            if (checkbox->staticCheckedSurf)
            {
                ituLayerReleaseExternalSurface(layer);
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                if (widget->flags & ITU_EXTERNAL_IN_PLACE)
                {
                    widget->flags &= ~ITU_EXTERNAL_IN_PLACE;
                }
#endif
            }
        }
    }

    if (checkbox->checkedSurf)
    {
        ituSurfaceRelease(checkbox->checkedSurf);
        checkbox->checkedSurf = NULL;

        if (widget->flags & ITU_EXTERNAL)
        {
            if (!layer)
            {
                layer = ituGetLayer(widget);
            }

            if (checkbox->staticFocusCheckedSurf)
            {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                if (widget->flags & ITU_EXTERNAL_IN_PLACE)
                {
                    widget->flags &= ~ITU_EXTERNAL_IN_PLACE;
                    ituLayerReleaseExternalSurface(layer);
                }
#else
                ituLayerReleaseExternalSurface(layer);
#endif
            }
        }
    }
}

void ituCheckBoxSetStyleIndex(ITUCheckBox* chk, int index)
{
    assert(chk);
    ITU_ASSERT_THREAD();
    if (chk && (index >= 0) && (index < 16))
    {
        ITUButton* btn = &chk->btn;
        chk->ImgTargetIndex = index;
        //for default image target style
        if (!btn->bg.icon.surf && !chk->dImgILB && (chk->dImgILBName[0] != '\0'))
        {
            chk->dImgILB = ituSceneFindWidget(ituScene, chk->dImgILBName);
        }
        if (chk->dImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)chk->dImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                btn->bg.icon.staticSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->bg.icon.surf = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        //for focus image target style
        if (!btn->focusSurf && !chk->fImgILB && (chk->fImgILBName[0] != '\0'))
        {
            chk->fImgILB = ituSceneFindWidget(ituScene, chk->fImgILBName);
        }
        if (chk->fImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)chk->fImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[chk->ImgTargetIndex])
            {
                btn->staticFocusSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->focusSurf = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        //for press image target style
        if (!btn->pressSurf && !chk->pImgILB && (chk->pImgILBName[0] != '\0'))
        {
            chk->pImgILB = ituSceneFindWidget(ituScene, chk->pImgILBName);
        }
        if (chk->pImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)chk->pImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->surfArray[chk->ImgTargetIndex])
            {
                btn->staticPressSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                btn->pressSurf = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        //for checked image target style
        if (!chk->checkedSurf && !chk->cImgILB && (chk->cImgILBName[0] != '\0'))
        {
            chk->cImgILB = ituSceneFindWidget(ituScene, chk->cImgILBName);
        }
        if (chk->cImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)chk->cImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                chk->staticCheckedSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                chk->checkedSurf = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
        //for focuschecked image target style
        if (!chk->focusCheckedSurf && !chk->fcImgILB && (chk->fcImgILBName[0] != '\0'))
        {
            chk->fcImgILB = ituSceneFindWidget(ituScene, chk->fcImgILBName);
        }
        if (chk->fcImgILB)
        {
            ITUIconListBox* ilb = (ITUIconListBox*)chk->fcImgILB;
            ituIconListBoxLoadStaticData(ilb);
            if (ilb->staticSurfArray[chk->ImgTargetIndex])
            {
                chk->staticFocusCheckedSurf = ilb->staticSurfArray[chk->ImgTargetIndex];
                chk->focusCheckedSurf = ilb->surfArray[chk->ImgTargetIndex];
            }
        }
    }
}
