#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char animationName[] = "ITUAnimation";

#define ITUANIMATION_DEBUG 0
#if ITUANIMATION_DEBUG
    #define animationLog(fmt, ...) (void)printf(fmt, ##__VA_ARGS__)
#else
    #define animationLog(fmt, ...)
#endif

static void AnimationOnStop(ITUAnimation* animation)
{
    // DO NOTHING
}

bool ituAnimationClone(ITUWidget* widget, ITUWidget** cloned)
{
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        void *       newObj    = malloc(sizeof(ITUAnimation));
        const void * srcObj    = (const void *)widget;
        ITUWidget *  newWidget = (ITUWidget *)newObj;
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newObj, srcObj, sizeof(ITUAnimation));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    return ituWidgetCloneImpl(widget, cloned);
}

void ituAnimationSetFontSize(ITUWidget* obj, int width, int height, int line)
{
    ITUText * tc = (ITUText *)obj;
    ituTextSetFontWidth(tc, width);
    ituTextSetFontHeight(tc, height);
    animationLog("%s --> w: %d, h: %d, l: %d\n", obj->name, width, height, line);
}

bool ituAnimationUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    ITUAnimation* animation = (ITUAnimation*) widget;
    bool result = false;
    assert(NULL != animation);

    if (ev == ITU_EVENT_TIMER)
    {
        if (animation->playing)
        {
            if (--animation->delayCount <= 0)
            {
                if (animation->frame >= (animation->totalframe - 1))
                {
                    //stop at last frame and not to go next keyframe when marked ITU_ANIM_LASTFRAME_STOP flag
                    if (animation->animationFlags & ITU_ANIM_LASTFRAME_STOP)
                    {
                        animation->playing = false;
                        animation->playCount = 0;
                        ituAnimationOnStop(animation);
                        ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
                        result = widget->dirty = true;
                        return result;
                    }

                    if ((animation->playCount > 0) && (--animation->playCount <= 0))
                    {
                        ituAnimationStop(animation);
                        animation->frame = 0;

                        if ((animation->animationFlags & ITU_ANIM_MOVE) ||
                            (animation->animationFlags & ITU_ANIM_EASE_IN) ||
                            (animation->animationFlags & ITU_ANIM_EASE_OUT) ||
                            (animation->animationFlags & ITU_ANIM_SCALE))
                        {
                            (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
                        }

                        if (animation->animationFlags & ITU_ANIM_COLOR)
                        {
                            (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
                            animation->child->alpha = animation->orgAlpha;
                        }

                        if (animation->animationFlags & ITU_ANIM_ROTATE)
                        {
                            animation->child->angle = animation->orgAngle;
                        }

                        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
                        {
                            animation->child->transformX = animation->orgTransformX;
                            animation->child->transformY = animation->orgTransformY;
                        }

                        if (animation->child->type == ITU_TEXT)
                        {
                            int value = (int)ituWidgetGetCustomData(animation->child);
                            int orgFontWidth = value >> 16;
                            int orgFontHeight = (value << 16) >> 16;
                            ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
                        }

                        if (animation->animationFlags & ITU_ANIM_REVERSE)
                        {
                            ituAnimationGoto(animation, animation->keyframe - 1);
                        }
                        else
                        {
                            ituAnimationGoto(animation, animation->keyframe + 1);
                        }

                        animation->playCount = 0;
                        ituAnimationOnStop(animation);
                        ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
                    }
                    else
                    {
                        if (animation->animationFlags & ITU_ANIM_REVERSE)
                        {
                            if ((animation->animationFlags & ITU_ANIM_CYCLE) && !animation->repeat)
                            {
                                const ITUWidget* target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe - 1);
                                if (NULL == target)
                                {
                                    ituAnimationStop(animation);
                                    animation->frame = 0;

                                    if ((animation->animationFlags & ITU_ANIM_MOVE) ||
                                        (animation->animationFlags & ITU_ANIM_EASE_IN) ||
                                        (animation->animationFlags & ITU_ANIM_EASE_OUT) ||
                                        (animation->animationFlags & ITU_ANIM_SCALE))
                                    {
                                        (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
                                    }

                                    if (animation->animationFlags & ITU_ANIM_COLOR)
                                    {
                                        (void)memcpy(&animation->child->color, &animation->orgColor, sizeof(ITUColor));
                                        animation->child->alpha = animation->orgAlpha;
                                    }

                                    if (animation->animationFlags & ITU_ANIM_ROTATE)
                                    {
                                        animation->child->angle = animation->orgAngle;
                                    }

                                    if (animation->animationFlags & ITU_ANIM_TRANSFORM)
                                    {
                                        animation->child->transformX = animation->orgTransformX;
                                        animation->child->transformY = animation->orgTransformY;
                                    }

                                    if (animation->child->type == ITU_TEXT)
                                    {
                                        int value = (int)ituWidgetGetCustomData(animation->child);
                                        int orgFontWidth = value >> 16;
                                        int orgFontHeight = (value << 16) >> 16;
                                        ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
                                    }

                                    ituAnimationGoto(animation, 0);
                                    animation->playCount = 0;
                                    ituAnimationOnStop(animation);
                                    ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
                                }
                                else
                                {
                                    ituAnimationReversePlay(animation, animation->keyframe - 1);
                                }
                            }
                            else
                            {
                                ituAnimationReversePlay(animation, animation->keyframe - 1);
                            }
                        }
                        else
                        {
                            if ((animation->animationFlags & ITU_ANIM_CYCLE) && !animation->repeat)
                            {
                                const ITUWidget* target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe + 1);
                                if (NULL == target)
                                {
                                    ituAnimationStop(animation);
                                    animation->frame = 0;

                                    if ((animation->animationFlags & ITU_ANIM_MOVE) ||
                                        (animation->animationFlags & ITU_ANIM_EASE_IN) ||
                                        (animation->animationFlags & ITU_ANIM_EASE_OUT) ||
                                        (animation->animationFlags & ITU_ANIM_SCALE))
                                    {
                                        (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
                                    }

                                    if (animation->animationFlags & ITU_ANIM_COLOR)
                                    {
                                        (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
                                        animation->child->alpha = animation->orgAlpha;
                                    }

                                    if (animation->animationFlags & ITU_ANIM_ROTATE)
                                    {
                                        animation->child->angle = animation->orgAngle;
                                    }

                                    if (animation->animationFlags & ITU_ANIM_TRANSFORM)
                                    {
                                        animation->child->transformX = animation->orgTransformX;
                                        animation->child->transformY = animation->orgTransformY;
                                    }

                                    if (animation->child->type == ITU_TEXT)
                                    {
                                        int value = (int)ituWidgetGetCustomData(animation->child);
                                        int orgFontWidth = value >> 16;
                                        int orgFontHeight = (value << 16) >> 16;
                                        ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
                                    }

                                    ituAnimationGoto(animation, 0);
                                    animation->playCount = 0;
                                    ituAnimationOnStop(animation);
                                    ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
                                }
                                else
                                {
                                    ituAnimationPlay(animation, animation->keyframe + 1);
                                }
                            }
                            else
                            {
                                ituAnimationPlay(animation, animation->keyframe + 1);
                            }
                        }
                    }
                }
                else
                {
                    ITUWidget* target = NULL;

                    if (animation->animationFlags & ITU_ANIM_REVERSE)
                    {
                        if (animation->keyframe > 0)
                        {
                            target = (ITUWidget *)itcTreeGetChildAt(animation, animation->keyframe - 1);
                        }

                        if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
                        {
                            int count = itcTreeGetChildCount(animation);
                            target = (ITUWidget*) itcTreeGetChildAt(animation, count - 1);
                        }
                    }
                    else
                    {
                        target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe + 1);
                        if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
                        {
                            target = (ITUWidget*) itcTreeGetChildAt(animation, 0);
                        }
                    }

                    if (NULL != target)
                    {
                        int frame = ++animation->frame;

                        if (animation->animationFlags & ITU_ANIM_MOVE)
                        {
                            animation->child->rect.x = animation->keyRect.x + (target->rect.x - animation->keyRect.x) * frame / animation->totalframe;
                            animation->child->rect.y = animation->keyRect.y + (target->rect.y - animation->keyRect.y) * frame / animation->totalframe;
                        }
                        else if (animation->animationFlags & ITU_ANIM_EASE_IN)
                        {
                            float step = (float)frame / animation->totalframe;
                            step = step * step * step;
                            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
                            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
                        }
                        else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
                        {
                            float step = (float)frame / animation->totalframe;
                            step = step - 1;
                            step = step * step * step + 1;
                            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
                            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
                        }
                        else
                        {
                            /* do nothing */
                        }

                        if (animation->animationFlags & ITU_ANIM_SCALE)
                        {
                            if (!(animation->animationFlags & ITU_ANIM_MOVE) && !(animation->animationFlags & ITU_ANIM_EASE_IN) && !(animation->animationFlags & ITU_ANIM_EASE_OUT) && (animation->animationFlags & ITU_ANIM_SCALE_CENTER))
                            {
                                animation->child->rect.x = animation->keyRect.x - (target->rect.width - animation->keyRect.width) / 2 * frame / animation->totalframe;
                                animation->child->rect.y = animation->keyRect.y - (target->rect.height - animation->keyRect.height) / 2 * frame / animation->totalframe;
                                animation->child->rect.width = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                                animation->child->rect.height = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
                            }
                            else if (animation->animationFlags & ITU_ANIM_EASE_IN)
                            {
                                float step = (float)frame / animation->totalframe;
                                step = step * step * step;
                                animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                                animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
                            }
                            else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
                            {
                                float step = (float)frame / animation->totalframe;
                                step = step - 1;
                                step = step * step * step + 1;
                                animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                                animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
                            }
                            else
                            {
                                int targetW = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                                int targetH = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
                                animation->child->rect.width = targetW;
                                animation->child->rect.height = targetH;
                                #if 0
                                (void)printf("n=%s c=%d/%d t=%d/%d k=%d/%d f=%d/%d\n", target->name, animation->child->rect.width, animation->child->rect.height, target->rect.width, target->rect.height, animation->keyRect.width, animation->keyRect.height, frame, animation->totalframe);
                                #endif
                            }

                            if (animation->child->type == ITU_TEXT)
                            {
                                int targetW = animation->child->rect.width;
                                int targetH = animation->child->rect.height;
                                int value = (int)ituWidgetGetCustomData(animation->child);
                                int orgFontWidth = value >> 16;
                                int orgFontHeight = (value << 16) >> 16;
                                ituAnimationSetFontSize(animation->child,
                                    orgFontWidth * targetW / animation->orgRect.width,
                                    orgFontHeight * targetH / animation->orgRect.height,
                                    __LINE__);
                            }
                        }
                        if (animation->animationFlags & ITU_ANIM_COLOR)
                        {
                            animation->child->color.alpha = animation->keyColor.alpha + (target->color.alpha - animation->keyColor.alpha) * frame / animation->totalframe;
                            animation->child->color.red = animation->keyColor.red + (target->color.red - animation->keyColor.red) * frame / animation->totalframe;
                            animation->child->color.green = animation->keyColor.green + (target->color.green - animation->keyColor.green) * frame / animation->totalframe;
                            animation->child->color.blue = animation->keyColor.blue + (target->color.blue - animation->keyColor.blue) * frame / animation->totalframe;
                            animation->child->alpha = animation->keyAlpha + (target->alpha - animation->keyAlpha) * frame / animation->totalframe;
                        }
                        if (animation->animationFlags & ITU_ANIM_ROTATE)
                        {
                            animation->child->angle = animation->keyAngle + (target->angle - animation->keyAngle) * frame / animation->totalframe;
                        }
                        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
                        {
                            animation->child->transformX = animation->keyTransformX + (target->transformX - animation->keyTransformX) * frame / animation->totalframe;
                            animation->child->transformY = animation->keyTransformY + (target->transformY - animation->keyTransformY) * frame / animation->totalframe;
                        }
                    }
                    else
                    {
                        animation->playing  = false;
                        animation->frame    = 0;
                        animation->playCount = 0;
                        ituAnimationOnStop(animation);
                        ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
                    }
                }
                animation->delayCount = animation->delay;
                widget->dirty = true;
                result = true;
            }
        }
    }
    else if (ev == ITU_EVENT_LAYOUT)
    {
        if (NULL == animation->child)
        {
            animation->child = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe);
            if (NULL != animation->child)
            {
                (void)memcpy(&animation->orgRect, &animation->child->rect, sizeof (ITURectangle));
                (void)memcpy(&animation->keyRect, &animation->child->rect, sizeof (ITURectangle));
                (void)memcpy(&animation->orgColor, &animation->child->color, sizeof (ITUColor));
                animation->orgAlpha = animation->child->alpha;
                (void)memcpy(&animation->keyColor, &animation->child->color, sizeof (ITUColor));
                animation->keyAlpha = animation->child->alpha;
                animation->orgAngle = animation->child->angle;
                animation->keyAngle = animation->child->angle;
                animation->orgTransformX = animation->child->transformX;
                animation->orgTransformY = animation->child->transformY;
                animation->keyTransformX = animation->child->transformX;
                animation->keyTransformY = animation->child->transformY;
                animation->frame = 0;
                result = widget->dirty = true;

                if (animation->child->type == ITU_TEXT)
                {
                    const ITUText* tc = (ITUText*)animation->child;
                    animation->orgFontWidth = tc->fontWidth;
                    animation->orgFontHeight = tc->fontHeight;
                }
            }
        }

        if (1)
        {
            int i = 0;
            int count = itcTreeGetChildCount(animation);
            for (i = 0; i < count; i++)
            {
                ITUWidget* widgetItem = (ITUWidget*)itcTreeGetChildAt(animation, i);
                if (widgetItem)
                {
                    int exc = (int)ituWidgetGetCustomData(widgetItem);
                    if ((widgetItem->type == ITU_TEXT) && (exc == 0))
                    {
                        const ITUText* text = (ITUText*)widgetItem;
                        int value = (text->fontWidth << 16) | text->fontHeight;
                        ituWidgetSetCustomData(widgetItem, value);
                        animationLog("[Animation]backup font size for %s\n", widgetItem->name);
                    }
                }
            }
        }
    }
    else if ((ev == ITU_EVENT_MOUSEDOWN) || (ev == ITU_EVENT_MOUSEUP) || (ev == ITU_EVENT_MOUSEDOUBLECLICK) ||
             (ev == ITU_EVENT_MOUSEMOVE) || (ev == ITU_EVENT_MOUSELONGPRESS) || (ev == ITU_EVENT_TOUCHSLIDELEFT) ||
             (ev == ITU_EVENT_TOUCHSLIDEUP) || (ev == ITU_EVENT_TOUCHSLIDERIGHT) || (ev == ITU_EVENT_TOUCHSLIDEDOWN) ||
             (ev == ITU_EVENT_TOUCHPINCH))
    {
        if (NULL != animation->child)
        {
            arg2 -= widget->rect.x;
            arg3 -= widget->rect.y;
            result |= ituWidgetUpdate(animation->child, ev, arg1, arg2, arg3);
            return widget->visible ? result : false;
        }
    }
    else
    {
        /* do nothing */
    }
    result |= ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);
    return widget->visible ? result : false;
}

void ituAnimationDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    ITUAnimation* animation = (ITUAnimation*) widget;

    if (NULL != animation->child)
    {
        ITURectangle prevClip;

        ituWidgetSetClipping(widget, dest, x, y, &prevClip);

        x += widget->rect.x;
        y += widget->rect.y;
        alpha = alpha * widget->alpha / 255;

        if ((animation->animationFlags & ITU_ANIM_MOTION_BLUR) && (animation->frame > 0))
        {
            int frame = animation->frame - 1;
            ITUWidget* target = NULL;

            if (animation->animationFlags & ITU_ANIM_REVERSE)
            {
                if (animation->keyframe > 0)
                {
                    target = (ITUWidget *)itcTreeGetChildAt(animation, animation->keyframe - 1);
                }

                if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
                {
                    int count = itcTreeGetChildCount(animation);
                    target = (ITUWidget*)itcTreeGetChildAt(animation, count - 1);
                }
            }
            else
            {
                target = (ITUWidget*)itcTreeGetChildAt(animation, animation->keyframe + 1);
                if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
                {
                    target = (ITUWidget*)itcTreeGetChildAt(animation, 0);
                }
            }

            if (NULL != target)
            {
                ITURectangle currRect;

                (void)memcpy(&currRect, &animation->child->rect, sizeof (ITURectangle));

                if (animation->animationFlags & ITU_ANIM_MOVE)
                {
                    animation->child->rect.x = animation->keyRect.x + (target->rect.x - animation->keyRect.x) * frame / animation->totalframe;
                    animation->child->rect.y = animation->keyRect.y + (target->rect.y - animation->keyRect.y) * frame / animation->totalframe;
                }
                else if (animation->animationFlags & ITU_ANIM_EASE_IN)
                {
                    float step = (float)frame / animation->totalframe;
                    step = step * step * step;
                    animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
                    animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
                }
                else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
                {
                    float step = (float)frame / animation->totalframe;
                    step = step - 1;
                    step = step * step * step + 1;
                    animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
                    animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
                }
                else
                {
                    /* do nothing */
                }

                if (animation->animationFlags & ITU_ANIM_SCALE)
                {
                    if (!(animation->animationFlags & ITU_ANIM_MOVE) && !(animation->animationFlags & ITU_ANIM_EASE_IN) && !(animation->animationFlags & ITU_ANIM_EASE_OUT) && (animation->animationFlags & ITU_ANIM_SCALE_CENTER))
                    {
                        animation->child->rect.x = animation->keyRect.x - (target->rect.width - animation->keyRect.width) / 2 * frame / animation->totalframe;
                        animation->child->rect.y = animation->keyRect.y - (target->rect.height - animation->keyRect.height) / 2 * frame / animation->totalframe;
                        animation->child->rect.width = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                        animation->child->rect.height = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
                    }
                    else if (animation->animationFlags & ITU_ANIM_EASE_IN)
                    {
                        float step = (float)frame / animation->totalframe;
                        step = step * step * step;
                        animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                        animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
                    }
                    else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
                    {
                        float step = (float)frame / animation->totalframe;
                        step = step - 1;
                        step = step * step * step + 1;
                        animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                        animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
                    }
                    else
                    {
                        animation->child->rect.width = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                        animation->child->rect.height = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
#if 0
                        (void)printf("n=%s c=%d/%d t=%d/%d k=%d/%d f=%d/%d\n", target->name, animation->child->rect.width, animation->child->rect.height, target->rect.width, target->rect.height, animation->keyRect.width, animation->keyRect.height, frame, animation->totalframe);
#endif
                    }
                }
                ituWidgetDraw(animation->child, dest, x, y, alpha / 5);

                (void)memcpy(&animation->child->rect, &currRect, sizeof (ITURectangle));
            }
        }

        ituWidgetDraw(animation->child, dest, x, y, alpha);

        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    }
    ituDirtyWidget(animation, false);
}

void ituAnimationOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
    ITUAnimation* animation = (ITUAnimation*) widget;
    assert(widget);

    switch (action)
    {
    case ITU_ACTION_PLAY:
        if (param[0] != '\0')
        {
            char buf[ITU_ACTION_PARAM_SIZE], *ptr, *saveptr;
            int keyframe;

            strlcpy(buf, param, sizeof(buf));
            keyframe = atoi(strtok_r(buf, " ", &saveptr));
            ptr = strtok_r(NULL, " ", &saveptr);
            if (ptr)
            {
                animation->repeat = false;
                animation->playCount = atoi(ptr);
            }
            ituAnimationPlay(animation, keyframe);
        }
        else
        {
            if (NULL != animation->child)
            {
                animation->animationFlags &= ~ITU_ANIM_REVERSE;
                animation->playing = true;
            }
            else
            {
                ituAnimationPlay(animation, animation->keyframe);
            }
        }
        break;

    case ITU_ACTION_BACK:
        if (param[0] != '\0')
        {
            char buf[ITU_ACTION_PARAM_SIZE], *ptr, *saveptr;
            int keyframe;

            strlcpy(buf, param, sizeof(buf));
            keyframe = atoi(strtok_r(buf, " ", &saveptr));
            ptr = strtok_r(NULL, " ", &saveptr);
            if (ptr)
            {
                animation->repeat = false;
                animation->playCount = atoi(ptr);
            }
            ituAnimationReversePlay(animation, keyframe);
        }
        else
        {
            ituAnimationReversePlay(animation, animation->keyframe);
        }
        break;

    case ITU_ACTION_STOP:
        ituAnimationStop(animation);
        ituAnimationOnStop(animation);
        ituExecActions(widget, animation->actions, ITU_EVENT_STOPPED, 0);
        break;

    case ITU_ACTION_GOTO:
        if (param[0] != '\0')
        {
            ituAnimationGoto(animation, atoi(param));
        }
        break;

    case ITU_ACTION_RELOAD:
        ituAnimationReset(animation);
        break;

    default:
        ituWidgetOnActionImpl(widget, action, param);
        break;
    }
}

void ituAnimationInit(ITUAnimation* animation)
{
    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    (void)memset(animation, 0, sizeof (ITUAnimation));

    ituWidgetInit(&animation->widget);

    ituWidgetSetType(animation, ITU_ANIMATION);
    ituWidgetSetName(animation, animationName);
    ituWidgetSetClone(animation, ituAnimationClone);
    ituWidgetSetUpdate(animation, ituAnimationUpdate);
    ituWidgetSetDraw(animation, ituAnimationDraw);
    ituWidgetSetOnAction(animation, ituAnimationOnAction);
    ituAnimationSetOnStop(animation, AnimationOnStop);

    ituAnimationSetDelay(animation, 10);
    animation->child = (ITUWidget*) itcTreeGetChildAt(animation, 0);
}

void ituAnimationLoad(ITUAnimation* animation, uint32_t base)
{
    assert(NULL != animation);

    ituWidgetLoad((ITUWidget*)animation, base);
    ituWidgetSetClone(animation, ituAnimationClone);
    ituWidgetSetUpdate(animation, ituAnimationUpdate);
    ituWidgetSetDraw(animation, ituAnimationDraw);
    ituWidgetSetOnAction(animation, ituAnimationOnAction);
    ituAnimationSetOnStop(animation, AnimationOnStop);

    if (animation->playing)
    {
        animation->delayCount = animation->delay;
    }
}

void ituAnimationSetDelay(ITUAnimation* animation, int delay)
{
    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    animation->delay           = delay;
    animation->widget.dirty    = true;
}

void ituAnimationPlay(ITUAnimation* animation, int keyframe)
{
    ITUWidget* target;

    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    animation->animationFlags &= ~ITU_ANIM_REVERSE;

    if (keyframe >= 0)
    {
        if (keyframe == 0)
        {
            ituAnimationReset(animation);
        }
        ituAnimationGoto(animation, keyframe);
    }
    else
    {
        ituAnimationGoto(animation, animation->keyframe);
    }

    target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe + 1);
    if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
    {
        target = (ITUWidget*) itcTreeGetChildAt(animation, 0);
    }

    animation->delayCount = animation->delay;

    if (NULL != target)
    {
        animation->playing = true;
    }
    else
    {
        if (animation->repeat && animation->totalframe > 1)
        {
            ituAnimationPlay(animation, 0);
        }
        else
        {
            if ((animation->animationFlags & ITU_ANIM_MOVE) || (animation->animationFlags & ITU_ANIM_EASE_IN) ||
                (animation->animationFlags & ITU_ANIM_EASE_OUT) || (animation->animationFlags & ITU_ANIM_SCALE))
            {
                (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
            }

            if (animation->animationFlags & ITU_ANIM_COLOR)
            {
                (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
                animation->child->alpha = animation->orgAlpha;
            }

            if (animation->animationFlags & ITU_ANIM_ROTATE)
            {
                animation->child->angle = animation->orgAngle;
            }

            if (animation->animationFlags & ITU_ANIM_TRANSFORM)
            {
                animation->child->transformX = animation->orgTransformX;
                animation->child->transformY = animation->orgTransformY;
            }

            ituAnimationOnStop(animation);
            ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
        }
    }
    animation->widget.dirty = true;
}

void ituAnimationStop(ITUAnimation* animation)
{
    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    if (!animation->playing)
    {
        return;
    }

    animation->playing         = false;
    animation->widget.dirty    = true;
}

void ituAnimationGoto(ITUAnimation* animation, int keyframe)
{
    ITUWidget* target;

    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    animation->frame = 0;

    target = (ITUWidget*) itcTreeGetChildAt(animation, keyframe);
    if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
    {
        if (animation->animationFlags & ITU_ANIM_REVERSE)
        {
            int count = itcTreeGetChildCount(animation);
            target = (ITUWidget*) itcTreeGetChildAt(animation, count - 1);
        }
        else
        {
            target = (ITUWidget*) itcTreeGetChildAt(animation, 0);
            keyframe = 0;
        }
        //keyframe = 0;
    }
    if (NULL != target)
    {
        if ((animation->animationFlags & ITU_ANIM_MOVE) || (animation->animationFlags & ITU_ANIM_EASE_IN) || (animation->animationFlags & ITU_ANIM_EASE_OUT))
        {
            animation->keyRect.x = target->rect.x;
            animation->keyRect.y = target->rect.y;
        }
        else
        {
            animation->keyRect.x = animation->child->rect.x;
            animation->keyRect.y = animation->child->rect.y;
        }
        if (animation->animationFlags & ITU_ANIM_SCALE)
        {
            animation->keyRect.width = target->rect.width;
            animation->keyRect.height = target->rect.height;
        }
        else
        {
            animation->keyRect.width = animation->child->rect.width;
            animation->keyRect.height = animation->child->rect.height;
        }
        if (animation->animationFlags & ITU_ANIM_COLOR)
        {
            (void)memcpy(&animation->keyColor, &target->color, sizeof (ITUColor));
            animation->keyAlpha = target->alpha;
        }
        else
        {
            (void)memcpy(&animation->keyColor, &animation->child->color, sizeof (ITUColor));
            animation->keyAlpha = animation->child->alpha;
        }
        if (animation->animationFlags & ITU_ANIM_ROTATE)
        {
            animation->keyAngle = target->angle;
        }
        else
        {
            animation->keyAngle = animation->child->angle;
        }
        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
        {
            animation->keyTransformX = target->transformX;
            animation->keyTransformY = target->transformY;
        }
        else
        {
            animation->keyTransformX = animation->child->transformX;
            animation->keyTransformY = animation->child->transformY;
        }

        if (1)//(animation->playing)
        {
            ituAnimationStop(animation);
            animation->frame = 0;
            (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof (ITURectangle));
            (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
            animation->child->alpha = animation->orgAlpha;
            animation->child->angle = animation->orgAngle;
            animation->child->transformX = animation->orgTransformX;
            animation->child->transformY = animation->orgTransformY;
            if (animation->child->type == ITU_TEXT)
            {
                int value = (int)ituWidgetGetCustomData(animation->child);
                int orgFontWidth = value >> 16;
                int orgFontHeight = (value << 16) >> 16;
                ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
            }
        }

        (void)memcpy(&animation->orgRect, &target->rect, sizeof (ITURectangle));
        (void)memcpy(&animation->orgColor, &target->color, sizeof (ITUColor));
        animation->orgAlpha = target->alpha;
        animation->orgAngle = target->angle;
        animation->orgTransformX = target->transformX;
        animation->orgTransformY = target->transformY;

        if ((animation->animationFlags & ITU_ANIM_MOVE) == 0 && (animation->animationFlags & ITU_ANIM_EASE_IN) == 0 && (animation->animationFlags & ITU_ANIM_EASE_OUT) == 0)
        {
            target->rect.x = animation->keyRect.x;
            target->rect.y = animation->keyRect.y;
        }
        if ((animation->animationFlags & ITU_ANIM_SCALE) == 0)
        {
            target->rect.width = animation->keyRect.width;
            target->rect.height = animation->keyRect.height;
        }
        if ((animation->animationFlags & ITU_ANIM_COLOR) == 0)
        {
            (void)memcpy(&target->color, &animation->keyColor, sizeof (ITUColor));
            target->alpha = animation->keyAlpha;
        }
        if ((animation->animationFlags & ITU_ANIM_ROTATE) == 0)
        {
            target->angle = animation->keyAngle;
        }
        if ((animation->animationFlags & ITU_ANIM_TRANSFORM) == 0)
        {
            target->transformX = animation->keyTransformX;
            target->transformY = animation->keyTransformY;
        }
        animation->keyframe = keyframe;
        animation->child = target;
    }
}

void ituAnimationGotoFrame(ITUAnimation* animation, int frame)
{
    ITUWidget* target = NULL;
    ITU_ASSERT_THREAD();

    if (frame < 0 || frame > animation->totalframe || animation->frame == frame)
    {
        return;
    }

    if (animation->animationFlags & ITU_ANIM_REVERSE)
    {
        target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe - 1);
        if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
        {
            int count = itcTreeGetChildCount(animation);
            target = (ITUWidget*) itcTreeGetChildAt(animation, count - 1);
        }
    }
    else
    {
        target = (ITUWidget*) itcTreeGetChildAt(animation, animation->keyframe + 1);
        if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
        {
            target = (ITUWidget*) itcTreeGetChildAt(animation, 0);
        }
    }

    if (NULL != target)
    {
#if 0
        if (animation->animationFlags & ITU_ANIM_MOVE)
        {
            animation->child->rect.x =
                animation->keyRect.x + (target->rect.x - animation->keyRect.x) * frame / animation->totalframe;
            animation->child->rect.y =
                animation->keyRect.y + (target->rect.y - animation->keyRect.y) * frame / animation->totalframe;
        }
        else if (animation->animationFlags & ITU_ANIM_EASE_IN)
        {
            float step               = (float)frame / animation->totalframe;
            step                     = step * step * step;
            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
        }
        else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
        {
            float step               = (float)frame / animation->totalframe;
            step                     = step - 1;
            step                     = step * step * step + 1;
            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
        }
        else
        {
            /* do nothing */
        }

        if (animation->animationFlags & ITU_ANIM_SCALE)
        {
            if (!(animation->animationFlags & ITU_ANIM_MOVE) && !(animation->animationFlags & ITU_ANIM_EASE_IN) &&
                !(animation->animationFlags & ITU_ANIM_EASE_OUT) && (animation->animationFlags & ITU_ANIM_SCALE_CENTER))
            {
                animation->child->rect.x = animation->keyRect.x - (target->rect.width - animation->keyRect.width) / 2 *
                                                                      frame / animation->totalframe;
                animation->child->rect.y = animation->keyRect.y - (target->rect.height - animation->keyRect.height) /
                                                                      2 * frame / animation->totalframe;
                animation->child->rect.width =
                    animation->keyRect.width +
                    (target->rect.width - animation->keyRect.width) * frame / animation->totalframe / 2;
                animation->child->rect.height =
                    animation->keyRect.height +
                    (target->rect.height - animation->keyRect.height) * frame / animation->totalframe / 2;
            }
            else if (animation->animationFlags & ITU_ANIM_EASE_IN)
            {
                float step = (float)frame / animation->totalframe;
                step       = step * step * step;
                animation->child->rect.width =
                    animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                animation->child->rect.height =
                    animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
            }
            else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
            {
                float step = (float)frame / animation->totalframe;
                step       = step - 1;
                step       = step * step * step + 1;
                animation->child->rect.width =
                    animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                animation->child->rect.height =
                    animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
            }
            else
            {
                animation->child->rect.width =
                    animation->keyRect.width +
                    (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                animation->child->rect.height =
                    animation->keyRect.height +
                    (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
            }

            if (animation->child->type == ITU_TEXT)
            {
                int targetW       = animation->child->rect.width;
                int targetH       = animation->child->rect.height;
                int value         = (int)ituWidgetGetCustomData(animation->child);
                int orgFontWidth  = value >> 16;
                int orgFontHeight = (value << 16) >> 16;
                ituAnimationSetFontSize(animation->child, orgFontWidth * targetW / animation->orgRect.width,
                                        orgFontHeight * targetH / animation->orgRect.height, __LINE__);
            }
    #if 0
            (void)printf("n=%s c=%d/%d t=%d/%d k=%d/%d f=%d/%d\n", target->name, animation->child->rect.width,
            animation->child->rect.height, target->rect.width, target->rect.height, animation->keyRect.width,
            animation->keyRect.height, frame, animation->totalframe);
    #endif
        }
        if (animation->animationFlags & ITU_ANIM_COLOR)
        {
            animation->child->color.alpha =
                animation->keyColor.alpha +
                (target->color.alpha - animation->keyColor.alpha) * frame / animation->totalframe;
            animation->child->color.red =
                animation->keyColor.red + (target->color.red - animation->keyColor.red) * frame / animation->totalframe;
            animation->child->color.green =
                animation->keyColor.green +
                (target->color.green - animation->keyColor.green) * frame / animation->totalframe;
            animation->child->color.blue = animation->keyColor.blue + (target->color.blue - animation->keyColor.blue) *
                                                                          frame / animation->totalframe;
            animation->child->alpha =
                animation->keyAlpha + (target->alpha - animation->keyAlpha) * frame / animation->totalframe;
        }
        if (animation->animationFlags & ITU_ANIM_ROTATE)
        {
            animation->child->angle =
                animation->keyAngle + (target->angle - animation->keyAngle) * frame / animation->totalframe;
        }
        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
        {
            animation->child->transformX = animation->keyTransformX + (target->transformX - animation->keyTransformX) *
                                                                          frame / animation->totalframe;
            animation->child->transformY = animation->keyTransformY + (target->transformY - animation->keyTransformY) *
                                                                          frame / animation->totalframe;
        }
#endif

        if (animation->animationFlags & ITU_ANIM_MOVE)
        {
            animation->child->rect.x = animation->keyRect.x + (target->rect.x - animation->keyRect.x) * frame / animation->totalframe;
            animation->child->rect.y = animation->keyRect.y + (target->rect.y - animation->keyRect.y) * frame / animation->totalframe;
        }
        else if (animation->animationFlags & ITU_ANIM_EASE_IN)
        {
            float step = (float)frame / animation->totalframe;
            step = step * step * step;
            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
        }
        else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
        {
            float step = (float)frame / animation->totalframe;
            step = step - 1;
            step = step * step * step + 1;
            animation->child->rect.x = animation->keyRect.x + (int)((target->rect.x - animation->keyRect.x) * step);
            animation->child->rect.y = animation->keyRect.y + (int)((target->rect.y - animation->keyRect.y) * step);
        }
        else
        {
            /* do nothing */
        }

        if (animation->animationFlags & ITU_ANIM_SCALE)
        {
            if (!(animation->animationFlags & ITU_ANIM_MOVE) && !(animation->animationFlags & ITU_ANIM_EASE_IN) && !(animation->animationFlags & ITU_ANIM_EASE_OUT) && (animation->animationFlags & ITU_ANIM_SCALE_CENTER))
            {
                animation->child->rect.x = animation->keyRect.x - (target->rect.width - animation->keyRect.width) / 2 * frame / animation->totalframe;
                animation->child->rect.y = animation->keyRect.y - (target->rect.height - animation->keyRect.height) / 2 * frame / animation->totalframe;
                animation->child->rect.width = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                animation->child->rect.height = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
            }
            else if (animation->animationFlags & ITU_ANIM_EASE_IN)
            {
                float step = (float)frame / animation->totalframe;
                step = step * step * step;
                animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
            }
            else if (animation->animationFlags & ITU_ANIM_EASE_OUT)
            {
                float step = (float)frame / animation->totalframe;
                step = step - 1;
                step = step * step * step + 1;
                animation->child->rect.width = animation->keyRect.width + (int)((target->rect.width - animation->keyRect.width) * step);
                animation->child->rect.height = animation->keyRect.height + (int)((target->rect.height - animation->keyRect.height) * step);
            }
            else
            {
                int targetW = animation->keyRect.width + (target->rect.width - animation->keyRect.width) * frame / animation->totalframe;
                int targetH = animation->keyRect.height + (target->rect.height - animation->keyRect.height) * frame / animation->totalframe;
                animation->child->rect.width = targetW;
                animation->child->rect.height = targetH;
#if 0
                (void)printf("n=%s c=%d/%d t=%d/%d k=%d/%d f=%d/%d\n", target->name, animation->child->rect.width, animation->child->rect.height, target->rect.width, target->rect.height, animation->keyRect.width, animation->keyRect.height, frame, animation->totalframe);
#endif
            }

            if (animation->child->type == ITU_TEXT)
            {
                int targetW = animation->child->rect.width;
                int targetH = animation->child->rect.height;
                int value = (int)ituWidgetGetCustomData(animation->child);
                int orgFontWidth = value >> 16;
                int orgFontHeight = (value << 16) >> 16;
                ituAnimationSetFontSize(animation->child,
                    orgFontWidth * targetW / animation->orgRect.width,
                    orgFontHeight * targetH / animation->orgRect.height,
                    __LINE__);
            }
        }
        if (animation->animationFlags & ITU_ANIM_COLOR)
        {
            animation->child->color.alpha = animation->keyColor.alpha + (target->color.alpha - animation->keyColor.alpha) * frame / animation->totalframe;
            animation->child->color.red = animation->keyColor.red + (target->color.red - animation->keyColor.red) * frame / animation->totalframe;
            animation->child->color.green = animation->keyColor.green + (target->color.green - animation->keyColor.green) * frame / animation->totalframe;
            animation->child->color.blue = animation->keyColor.blue + (target->color.blue - animation->keyColor.blue) * frame / animation->totalframe;
            animation->child->alpha = animation->keyAlpha + (target->alpha - animation->keyAlpha) * frame / animation->totalframe;
        }
        if (animation->animationFlags & ITU_ANIM_ROTATE)
        {
            animation->child->angle = animation->keyAngle + (target->angle - animation->keyAngle) * frame / animation->totalframe;
        }
        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
        {
            animation->child->transformX = animation->keyTransformX + (target->transformX - animation->keyTransformX) * frame / animation->totalframe;
            animation->child->transformY = animation->keyTransformY + (target->transformY - animation->keyTransformY) * frame / animation->totalframe;
        }

        animation->frame = frame;
        animation->widget.dirty = true;
    }
}

void ituAnimationReset(ITUAnimation* animation)
{
    ITU_ASSERT_THREAD();

    ituAnimationStop(animation);
    animation->frame = 0;
    if (NULL != animation->child)
    {
        if ((animation->animationFlags & ITU_ANIM_MOVE) || (animation->animationFlags & ITU_ANIM_EASE_IN) ||
            (animation->animationFlags & ITU_ANIM_EASE_OUT) || (animation->animationFlags & ITU_ANIM_SCALE))
        {
            (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
        }
        if (animation->animationFlags & ITU_ANIM_COLOR)
        {
            (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
            animation->child->alpha = animation->orgAlpha;
        }
        if (animation->animationFlags & ITU_ANIM_ROTATE)
        {
            animation->child->angle = animation->orgAngle;
        }
        if (animation->animationFlags & ITU_ANIM_TRANSFORM)
        {
            animation->child->transformX = animation->orgTransformX;
            animation->child->transformY = animation->orgTransformY;
        }

        if (animation->child->type == ITU_TEXT)
        {
            int value = (int)ituWidgetGetCustomData(animation->child);
            int orgFontWidth = value >> 16;
            int orgFontHeight = (value << 16) >> 16;
            ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
        }

        animation->child = NULL;
    }
    animation->keyframe = 0;
    animation->animationFlags &= ~ITU_ANIM_REVERSE;
    ituWidgetUpdate(animation, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituAnimationReversePlay(ITUAnimation* animation, int keyframe)
{
    ITUWidget* target = NULL;
    int count;

    assert(NULL != animation);
    ITU_ASSERT_THREAD();

    count = itcTreeGetChildCount(animation);

    if (count < 2)
    {
        return;
    }

    if (keyframe >= 0)
    {
        ituAnimationGoto(animation, keyframe);
    }
    else
    {
        ituAnimationGoto(animation, count - 1);
    }

    if (animation->keyframe - 1 >= 0)
    {
        target = (ITUWidget *)itcTreeGetChildAt(animation, animation->keyframe - 1);
    }

    if (!target && (animation->animationFlags & ITU_ANIM_CYCLE))
    {
        target = (ITUWidget*) itcTreeGetChildAt(animation, 0);
    }

    animation->delayCount = animation->delay;
    animation->animationFlags |= ITU_ANIM_REVERSE;

    if (NULL != target)
    {
        animation->playing = true;
    }
    else
    {
        if (animation->repeat && animation->totalframe > 1)
        {
            ituAnimationReversePlay(animation, count - 1);
        }
        else
        {
            if ((animation->animationFlags & ITU_ANIM_MOVE) || (animation->animationFlags & ITU_ANIM_EASE_IN) ||
                (animation->animationFlags & ITU_ANIM_EASE_OUT) || (animation->animationFlags & ITU_ANIM_SCALE))
            {
                (void)memcpy(&animation->child->rect, &animation->orgRect, sizeof(ITURectangle));
            }
            if (animation->animationFlags & ITU_ANIM_COLOR)
            {
                (void)memcpy(&animation->child->color, &animation->orgColor, sizeof (ITUColor));
                animation->child->alpha = animation->orgAlpha;
            }
            if (animation->animationFlags & ITU_ANIM_ROTATE)
            {
                animation->child->angle = animation->orgAngle;
            }

            if (animation->animationFlags & ITU_ANIM_TRANSFORM)
            {
                animation->child->transformX = animation->orgTransformX;
                animation->child->transformY = animation->orgTransformY;
            }

            if (animation->child->type == ITU_TEXT)
            {
                int value = (int)ituWidgetGetCustomData(animation->child);
                int orgFontWidth = value >> 16;
                int orgFontHeight = (value << 16) >> 16;
                ituAnimationSetFontSize(animation->child, orgFontWidth, orgFontHeight, __LINE__);
            }

            ituAnimationOnStop(animation);
            ituExecActions((ITUWidget*)animation, animation->actions, ITU_EVENT_STOPPED, 0);
        }
    }
    animation->widget.dirty = true;
}

void ituAnimationSetLastFrameStop(ITUAnimation* animation, bool enable)
{
    assert(NULL != animation);
    ITU_ASSERT_THREAD();
    if (NULL != animation)
    {
        if (enable)
        {
            animation->animationFlags |= ITU_ANIM_LASTFRAME_STOP;
        }
        else
        {
            animation->animationFlags &= ~ITU_ANIM_LASTFRAME_STOP;
        }
    }
}
