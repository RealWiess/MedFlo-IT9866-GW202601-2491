#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#include "ite/itp.h"

static const char widgetName[] = "ITUWidget";

static void       InitializeFadeEffect (ITUWidget * widget, bool isShowing)
{
    widget->effect = malloc(sizeof(ITUFadeEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituFadeEffectInit((ITUFadeEffect *)widget->effect, isShowing);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

static void InitializeScrollEffect (ITUWidget * widget, ITUScrollType type,
                                    bool isShowing)
{
    widget->effect = malloc(sizeof(ITUScrollEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituScrollEffectInit((ITUScrollEffect *)widget->effect, type);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

static void InitializeScrollFadeEffect (ITUWidget * widget, ITUScrollType type,
                                        bool isShowing)
{
    widget->effect = malloc(sizeof(ITUScrollFadeEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituScrollFadeEffectInit((ITUScrollFadeEffect *)widget->effect, type,
            isShowing);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

static void InitializeScaleEffect (ITUWidget * widget, bool isShowing)
{
    widget->effect = malloc(sizeof(ITUScaleEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituScaleEffectInit((ITUScaleEffect *)widget->effect, isShowing);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

static void InitializeScaleFadeEffect (ITUWidget * widget, bool isShowing)
{
    widget->effect = malloc(sizeof(ITUScaleFadeEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituScaleFadeEffectInit((ITUScaleFadeEffect *)widget->effect, isShowing);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

static void InitializeWipeEffect (ITUWidget * widget, ITUWipeType type,
                                  bool isShowing)
{
    widget->effect = malloc(sizeof(ITUWipeEffect));
    if (widget->effect != NULL)
    {
        int step = widget->effects[ITU_STATE_NORMAL];

        ituWipeEffectInit((ITUWipeEffect *)widget->effect, type);

        if (step > 0)
        {
            ituEffectSetTotalStep(widget->effect, step);
        }
        else
        {
            ituEffectSetTotalStep(widget->effect, widget->effectSteps);
        }

        ituEffectStart(widget->effect, widget);
        ituEffectUpdate(widget->effect, widget);
        widget->state = isShowing ? ITU_STATE_SHOWING : ITU_STATE_HIDING;
    }
}

void ituWidgetExitImpl (ITUWidget * widget)
{
    ITCTree *node, *next;
    assert(widget != NULL);
    assert(ituScene != NULL);

    if ((ituScene->focused != NULL) && (ituScene->focused == widget))
    {
        ituScene->focused = NULL;
    }

    node = widget->tree.child;
    while (node != NULL)
    {
        next = node->sibling;
        ituWidgetExit(node);
        node = next;
    }

    if (widget->flags & ITU_DYNAMIC)
    {
        free(widget);
    }
}

bool ituWidgetCloneImpl (ITUWidget * widget, ITUWidget ** cloned)
{
    ITCTree *node, *next;
    assert(widget != NULL);
    assert(cloned != NULL);

    if (*cloned != NULL)
    {
        (*cloned)->flags |= ITU_DYNAMIC;
        (*cloned)->dirty = true;
        return true;
    }
    else
    {
        ITUWidget * newWidget = malloc(sizeof(ITUWidget));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUWidget));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    if (*cloned != NULL)
    {
        (*cloned)->flags |= ITU_DYNAMIC;

        node = widget->tree.child;
        while (node != NULL)
        {
            bool        result;
            ITUWidget* child = NULL;
            next = node->sibling;

            result = ituWidgetClone(node, &child);
            if (!result)
            {
                return false;
            }

            ituWidgetAdd(*cloned, child);
            node = next;
        }
        (*cloned)->dirty = true;
    }
    return true;
}

void ituWidgetAddImpl (ITUWidget * widget, ITUWidget * child)
{
    assert(widget != NULL);
    assert(child != NULL);

    widget->dirty = true;

    itcTreePushBack(&widget->tree, child);
}

static void WidgetHide (int arg)
{
    ITUWidget * widget = (ITUWidget *)arg;
    assert(widget != NULL);

    ituWidgetSetVisible(widget, false);
}

bool ituWidgetUpdateImpl (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool      result     = false;
    ITCTree * node       = NULL;
    int       childCount = 0;

#ifdef ITUWIDGET_TREE_USE_ALLOCA
    ITUWidget ** children = NULL;
#else
    ITUWidget * children[ITU_WIDGET_CHILD_MAX];
#endif

    assert(widget != NULL);

    /*if (ev != ITU_EVENT_TIMER && ev != ITU_EVENT_MOUSEMOVE)
    {
        (void)printf("%s{%d,%d,%d,%d} t:%d v:%d, ac:%d d:%d a:%d\n",
            widget->name,
            widget->rect.x,
            widget->rect.y,
            widget->rect.width,
            widget->rect.height,
            widget->type,
            widget->visible,
            widget->active,
            widget->dirty,
            widget->color.alpha);
    }*/

#ifdef ITUWIDGET_TREE_USE_ALLOCA
    for (node = widget->tree.child; node != NULL; node = node->sibling)
    {
        childCount++;
    }

    if (childCount > 0)
    {
        children = (ITUWidget **)alloca(sizeof(ITUWidget *) * childCount);

        if (children == NULL)
        {
            return false;
        }

        childCount = 0;
    }
#endif

    for (node = widget->tree.child; node != NULL; node = node->sibling)
    {
        children[childCount] = (ITUWidget *)node;
        childCount++;

#ifndef ITUWIDGET_TREE_USE_ALLOCA
        if (childCount >= ITU_WIDGET_CHILD_MAX)
        {
            break;
        } 
#endif
    }

    if (ev == ITU_EVENT_KEYDOWN || ev == ITU_EVENT_KEYUP)
    {
        if (ituWidgetIsEnabled(widget))
        {
            while (--childCount >= 0)
            {
                ITUWidget * child = children[childCount];

                if (child->visible)
                {
                    result |= ituWidgetUpdate(child, ev, arg1, arg2, arg3);

                    if (result)
                    {
                        break;
                    }
                }
            }

            if (ituWidgetIsActive(widget) && !result)
            {
                result = ituWidgetOnPress(widget, ev, arg1, arg2, arg3);
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEDOWN)
    {
        if (ituWidgetIsEnabled(widget))
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            if (ituWidgetIsInside(widget, x, y))
            {
                while (--childCount >= 0)
                {
                    ITUWidget * child = children[childCount];

                    if (child->visible)
                    {
                        result |= ituWidgetUpdate(child, ev, arg1, x, y);
                        if (result)
                        {
                            break;
                        }
                    }
                }

                if (!result)
                {
                    result = ituWidgetOnPress(widget, ev, arg1, x, y);
                }
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEUP)
    {
        int x = arg2 - widget->rect.x;
        int y = arg3 - widget->rect.y;

        while (--childCount >= 0)
        {
            ITUWidget * child = children[childCount];

            result |= ituWidgetUpdate(child, ev, arg1, x, y);
        }
    }
    else if (ev == ITU_EVENT_MOUSEMOVE)
    {
        if (ituWidgetIsEnabled(widget))
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            while (--childCount >= 0)
            {
                ITUWidget * child = children[childCount];

                if (child->visible)
                {
                    result |= ituWidgetUpdate(child, ev, arg1, x, y);
                }
            }
        }
    }
    else if (ev == ITU_EVENT_MOUSEDOUBLECLICK || ev == ITU_EVENT_MOUSELONGPRESS || ev == ITU_EVENT_TOUCHSLIDELEFT ||
             ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDERIGHT || ev == ITU_EVENT_TOUCHSLIDEDOWN ||
             ev == ITU_EVENT_TOUCHPINCH)
    {
        if (ituWidgetIsEnabled(widget))
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            while (--childCount >= 0)
            {
                ITUWidget * child = children[childCount];

                if (child->visible)
                {
                    result |= ituWidgetUpdate(child, ev, arg1, x, y);
                    if (result)
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if (ev == ITU_EVENT_TIMER && ituWidgetIsEffecting(widget))
        {
            if (ituEffectIsPlaying(widget->effect))
            {
                ituEffectUpdate(widget->effect, widget);
            }
            else
            {
                //when use API to trigger layer hide directly
                //layer will hide after someone's effect done (and check layer state is ITU_STATE_HIDING)
                ITUWidget* layerWidget = (ITUWidget*)ituGetLayer(widget);
                if (layerWidget != NULL)
                {
                    if ((!ituWidgetIsEffecting(layerWidget)) && (layerWidget->state == ITU_STATE_HIDING))
                    {
                        layerWidget->state = ITU_STATE_NORMAL;
                        ituWidgetSetVisible(layerWidget, false);
                    }
                }

                if (widget->state == ITU_STATE_SHOWING)
                {
                    if (widget->hideDelay != -1)
                    {
                        ituSceneExecuteCommand(ituScene, widget->hideDelay, WidgetHide, (int)widget);
                    }
                }
                else if (widget->state == ITU_STATE_HIDING)
                {
                    widget->visible = false;
                    ituWidgetUpdate(widget, ITU_EVENT_RELEASE, 0, 0, 0);
                }
                else
                {
                    /* do nothing */
                }

                ituEffectStop(widget->effect, widget);
                ituEffectExit(widget->effect);
                free(widget->effect);
                widget->effect = NULL;
                widget->state  = ITU_STATE_NORMAL;
                ituWidgetSetEffect(widget, ITU_STATE_NORMAL, 0);
            }
            widget->dirty = true;
        }

        for (node = widget->tree.child; node != NULL; node = node->sibling)
        {
            ITUWidget * child = (ITUWidget *)node;

            if (child->visible || (ev == ITU_EVENT_LOAD && child->visible) || ev == ITU_EVENT_RELEASE ||
                ev == ITU_EVENT_LANGUAGE || ev == ITU_EVENT_LOAD_IMAGE || ev == ITU_EVENT_LAYOUT ||
                ev == ITU_EVENT_TIMER)
            {
#ifdef CFG_ITU_DBG
                uint32_t size = itpGetFreeHeapSize();
#endif
                widget->dirty |= ituWidgetUpdate(child, ev, arg1, arg2, arg3);
#ifdef CFG_ITU_DBG
                if (ev == ITU_EVENT_LOAD)
                {
                    (void)printf("[LoadWidget][%s][%d]\n", child->name, size - itpGetFreeHeapSize());
                }
#endif
            }
        }

        result = widget->dirty;
    }

    return widget->visible ? result : false;
}

void ituWidgetDrawImpl (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    ITCTree *    node;
    ITURectangle prevClip;

    assert(widget != NULL);
    assert(dest != NULL);

    /*(void)printf("%s{%d,%d,%d,%d} t:%d v:%d, ac:%d d:%d a:%d\n",
        widget->name,
        widget->rect.x,
        widget->rect.y,
        widget->rect.width,
        widget->rect.height,
        widget->type,
        widget->visible,
        widget->active,
        widget->dirty,
        widget->color.alpha );*/

    if (!widget->visible)
    {
        return;
    }

    ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    x += widget->rect.x;
    y += widget->rect.y;
    alpha = alpha * widget->alpha / 255;

    for (node = widget->tree.child; node != NULL; node = node->sibling)
    {
        ITUWidget * child = (ITUWidget *)node;
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        if (child->visible && (alpha > 0))
#else
        if (child->visible)
#endif
        {
            if (ituWidgetIsOverlapClipping(child, dest, x, y))
            {
                ituWidgetDraw(node, dest, x, y, alpha);
            }
            else
            {
                ituDirtyWidget(child, false);
            }
        }
        child->dirty = false;
    }

    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
}

static void WidgetShow (int arg)
{
    ITUWidget * widget = (ITUWidget *)arg;
    assert(widget != NULL);

    ituWidgetSetVisible(widget, true);
}

void ituWidgetOnActionImpl (ITUWidget * widget, ITUActionType action, char * param)
{
    assert(widget != NULL);

    switch (action)
    {
        case ITU_ACTION_SHOW:
            if (param[0] != '\0')
            {
                ITUEffectType effect, oldEffect = widget->effects[ITU_STATE_SHOWING];
                char          buf[ITU_ACTION_PARAM_SIZE], *saveptr;
                int           oldStep = widget->effects[ITU_STATE_NORMAL];

                strlcpy(buf, param, sizeof(buf));
                effect = atoi(strtok_r(buf, " ", &saveptr));

                // to check the show param is valid effect type
                if (effect < ITU_EFFECT_MAX_COUNT)
                {
                    char * ptr = strtok_r(NULL, " ", &saveptr);
                    if (ptr != NULL)
                    {
                        ituWidgetSetEffect(widget, ITU_STATE_NORMAL,
                                           atoi(ptr)); // use widget->effects[ITU_STATE_NORMAL] to store step parameter
                    }

                    ituWidgetSetEffect(widget, ITU_STATE_SHOWING, effect);

                    ituWidgetSetVisible(widget, true);

                    widget->effects[ITU_STATE_SHOWING] = oldEffect;
                    widget->effects[ITU_STATE_NORMAL]  = oldStep;
                }
                else
                {
                    if (widget->showDelay == 0)
                    {
                        ituWidgetSetVisible(widget, true);
                    }
                    else
                    {
                        ituSceneExecuteCommand(ituScene, widget->showDelay, WidgetShow, (int)widget);
                    }
                }
            }
            else
            {
                if (widget->showDelay == 0)
                {
                    ituWidgetSetVisible(widget, true);
                }
                else
                {
                    ituSceneExecuteCommand(ituScene, widget->showDelay, WidgetShow, (int)widget);
                }
            }
            break;

        case ITU_ACTION_HIDE:
            if (param[0] != '\0')
            {
                ITUEffectType effect, oldEffect = widget->effects[ITU_STATE_HIDING];
                char          buf[ITU_ACTION_PARAM_SIZE], *ptr, *saveptr;
                int           oldStep = widget->effects[ITU_STATE_NORMAL];

                strlcpy(buf, param, sizeof(buf));
                effect = atoi(strtok_r(buf, " ", &saveptr));
                ptr    = strtok_r(NULL, " ", &saveptr);
                if (ptr != NULL)
                {
                    ituWidgetSetEffect(widget, ITU_STATE_NORMAL,
                                       atoi(ptr)); // use widget->effects[ITU_STATE_NORMAL] to store step parameter
                }

                ituWidgetSetEffect(widget, ITU_STATE_HIDING, effect);

                ituWidgetSetVisible(widget, false);

                widget->effects[ITU_STATE_HIDING] = oldEffect;
                widget->effects[ITU_STATE_NORMAL] = oldStep;
            }
            else
            {
                ituWidgetSetVisible(widget, false);
            }
            break;

        case ITU_ACTION_FOCUS:
            ituFocusWidget(widget);
            break;

        case ITU_ACTION_ENABLE:
            ituWidgetEnable(widget);
            break;

        case ITU_ACTION_DISABLE:
            ituWidgetDisable(widget);
            break;

        default:
            (void)printf("================================\n");
            (void)printf("[widget %s assert]\n", widget->name);
            (void)printf("================================\n");
            assert(0);
            break;
    }
}

bool ituWidgetOnPressImpl (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    return false;
}

void ituWidgetInit (ITUWidget * widget)
{
    assert(widget != NULL);

    (void)memset(widget, 0, sizeof(ITUWidget));
    ITU_ASSERT_THREAD();

    widget->type = ITU_WIDGET;
    strlcpy(widget->name, widgetName, sizeof(widget->name));

    widget->Exit        = ituWidgetExitImpl;
    widget->Clone       = ituWidgetCloneImpl;
    widget->Update      = ituWidgetUpdateImpl;
    widget->Draw        = ituWidgetDrawImpl;
    widget->OnAction    = ituWidgetOnActionImpl;
    widget->OnPress     = ituWidgetOnPressImpl;

    widget->visible     = true;
    widget->dirty       = true;
    widget->color.alpha = 255U;

    ituWidgetEnable(widget);
}

void ituWidgetLoad (ITUWidget * widget, uint32_t base)
{
    assert(widget != NULL);

    /*(void)printf("%s{%d,%d,%d,%d} t:%d v:%d, ac:%d d:%d a:%d\n",
        widget->name,
        widget->rect.x,
        widget->rect.y,
        widget->rect.width,
        widget->rect.height,
        widget->type,
        widget->visible,
        widget->active,
        widget->dirty,
        widget->color.alpha);*/

    if (widget->tree.parent != NULL)
    {
        widget->tree.parent = (ITCTree *)((uint32_t)widget->tree.parent + base);
    }

    if (widget->tree.sibling != NULL)
    {
        widget->tree.sibling = (ITCTree *)((uint32_t)widget->tree.sibling + base);
    }

    if (widget->tree.child != NULL)
    {
        widget->tree.child = (ITCTree *)((uint32_t)widget->tree.child + base);
    }

    widget->effect   = NULL;
    widget->Exit     = ituWidgetExitImpl;
    widget->Clone    = ituWidgetCloneImpl;
    widget->Update   = ituWidgetUpdateImpl;
    widget->Draw     = ituWidgetDrawImpl;
    widget->OnAction = ituWidgetOnActionImpl;
    widget->OnPress  = ituWidgetOnPressImpl;
}

void ituWidgetSetNameImpl (ITUWidget * widget, const char * name)
{
    assert(widget != NULL);
    assert(name != NULL);
    ITU_ASSERT_THREAD();

    strlcpy(widget->name, name, sizeof(widget->name));
}

void ituWidgetSetVisibleImpl (ITUWidget * widget, bool visible)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    if (!visible && (widget->flags & ITU_ALWAYS_VISIBLE))
    {
        return;
    }
    else if (widget->effect != NULL)
    {
        if (widget->state == ITU_STATE_HIDING)
        {
            widget->visible = false;
            ituWidgetUpdate(widget, ITU_EVENT_RELEASE, 0, 0, 0);
        }
        ituEffectStop(widget->effect, widget);
        ituEffectExit(widget->effect);
        free(widget->effect);
        widget->effect = NULL;
        widget->state  = ITU_STATE_NORMAL;
        ituWidgetSetEffect(widget, ITU_STATE_NORMAL, 0);
    }
    else if (widget->visible == visible)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if (visible)
    {
        switch (widget->effects[ITU_STATE_SHOWING])
        {
            case ITU_EFFECT_FADE:
                InitializeFadeEffect(widget, true);
                break;

            case ITU_EFFECT_SCROLL_LEFT:
                InitializeScrollEffect(widget, ITU_SCROLL_IN_LEFT, true);
                break;

            case ITU_EFFECT_SCROLL_UP:
                InitializeScrollEffect(widget, ITU_SCROLL_IN_UP, true);
                break;

            case ITU_EFFECT_SCROLL_RIGHT:
                InitializeScrollEffect(widget, ITU_SCROLL_IN_RIGHT, true);
                break;

            case ITU_EFFECT_SCROLL_DOWN:
                InitializeScrollEffect(widget, ITU_SCROLL_IN_DOWN, true);
                break;

            case ITU_EFFECT_SCROLL_LEFT_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_IN_LEFT, true);
                break;

            case ITU_EFFECT_SCROLL_UP_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_IN_UP, true);
                break;

            case ITU_EFFECT_SCROLL_RIGHT_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_IN_RIGHT, true);
                break;

            case ITU_EFFECT_SCROLL_DOWN_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_IN_DOWN, true);
                break;

            case ITU_EFFECT_SCALE:
                InitializeScaleEffect(widget, true);
                break;

            case ITU_EFFECT_SCALE_FADE:
                InitializeScaleFadeEffect(widget, true);
                break;

            case ITU_EFFECT_WIPE_LEFT:
                InitializeWipeEffect(widget, ITU_WIPE_IN_LEFT, true);
                break;

            case ITU_EFFECT_WIPE_UP:
                InitializeWipeEffect(widget, ITU_WIPE_IN_UP, true);
                break;

            case ITU_EFFECT_WIPE_RIGHT:
                InitializeWipeEffect(widget, ITU_WIPE_IN_RIGHT, true);
                break;

            case ITU_EFFECT_WIPE_DOWN:
                InitializeWipeEffect(widget, ITU_WIPE_IN_DOWN, true);
                break;

            default:
                break;
        }
        widget->visible = true;
        ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
        ituWidgetUpdate(widget, ITU_EVENT_LOAD, 0, 0, 0);
        ituWidgetUpdate(widget, ITU_EVENT_LOAD_EXTERNAL, 0, 0, 0);

        if (widget->state != ITU_STATE_SHOWING && widget->hideDelay != -1)
        {
            ituSceneExecuteCommand(ituScene, widget->hideDelay, WidgetHide, (int)widget);
        }
    }
    else
    {
        switch (widget->effects[ITU_STATE_HIDING])
        {
            case ITU_EFFECT_FADE:
                InitializeFadeEffect(widget, false);
                break;

            case ITU_EFFECT_SCROLL_LEFT:
                InitializeScrollEffect(widget, ITU_SCROLL_OUT_LEFT, false);
                break;

            case ITU_EFFECT_SCROLL_UP:
                InitializeScrollEffect(widget, ITU_SCROLL_OUT_UP, false);
                break;

            case ITU_EFFECT_SCROLL_RIGHT:
                InitializeScrollEffect(widget, ITU_SCROLL_OUT_RIGHT, false);
                break;

            case ITU_EFFECT_SCROLL_DOWN:
                InitializeScrollEffect(widget, ITU_SCROLL_OUT_DOWN, false);
                break;

            case ITU_EFFECT_SCROLL_LEFT_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_OUT_LEFT, false);
                break;

            case ITU_EFFECT_SCROLL_UP_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_OUT_UP, false);
                break;

            case ITU_EFFECT_SCROLL_RIGHT_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_OUT_RIGHT, false);
                break;

            case ITU_EFFECT_SCROLL_DOWN_FADE:
                InitializeScrollFadeEffect(widget, ITU_SCROLL_OUT_DOWN, false);
                break;

            case ITU_EFFECT_SCALE:
                InitializeScaleEffect(widget, false);
                break;

            case ITU_EFFECT_SCALE_FADE:
                InitializeScaleFadeEffect(widget, false);
                break;

            case ITU_EFFECT_WIPE_LEFT:
                InitializeWipeEffect(widget, ITU_WIPE_OUT_LEFT, false);
                break;

            case ITU_EFFECT_WIPE_UP:
                InitializeWipeEffect(widget, ITU_WIPE_OUT_UP, false);
                break;

            case ITU_EFFECT_WIPE_RIGHT:
                InitializeWipeEffect(widget, ITU_WIPE_OUT_RIGHT, false);
                break;

            case ITU_EFFECT_WIPE_DOWN:
                InitializeWipeEffect(widget, ITU_WIPE_OUT_DOWN, false);
                break;

            default:
                widget->visible = false;
                ituWidgetUpdate(widget, ITU_EVENT_RELEASE, 0, 0, 0);
                if (widget->tree.parent)
                {
                    ituWidgetSetDirty(widget->tree.parent, true);
                }
                break;
        }
    }
    widget->dirty = true;
}

bool ituWidgetIsVisibleImpl (ITUWidget * widget)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    if (!widget->visible)
    {
        return false;
    }
    else
    {
        ITUWidget * parent = (ITUWidget *)widget->tree.parent;
        while (parent)
        {
            if (!parent->visible)
            {
                return false;
            }

            parent = (ITUWidget *)parent->tree.parent;
        }
    }
    return true;
}

void ituWidgetSetActiveImpl (ITUWidget * widget, bool active)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->active = active;
    widget->dirty  = true;
}

void ituWidgetSetAlphaImpl (ITUWidget * widget, uint8_t alpha)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    if (widget->alpha == alpha)
    {
        return;
    }

    widget->alpha = alpha;
    widget->dirty = true;
}

void ituWidgetSetColorImpl (ITUWidget * widget, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->color.alpha = alpha;
    widget->color.red   = red;
    widget->color.green = green;
    widget->color.blue  = blue;
    widget->dirty       = true;
}

void ituWidgetSetXImpl (ITUWidget * widget, int x)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.x = x;
    widget->dirty  = true;
}

void ituWidgetSetYImpl (ITUWidget * widget, int y)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.y = y;
    widget->dirty  = true;
}

void ituWidgetSetPositionImpl (ITUWidget * widget, int x, int y)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.x = x;
    widget->rect.y = y;
    widget->dirty  = true;
}

void ituWidgetSetWidthImpl (ITUWidget * widget, int width)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.width = width;
    widget->dirty      = true;

    ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituWidgetSetHeightImpl (ITUWidget * widget, int height)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.height = height;
    widget->dirty       = true;

    ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituWidgetSetDimensionImpl (ITUWidget * widget, int width, int height)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->rect.width  = width;
    widget->rect.height = height;
    widget->dirty       = true;

    ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
}

void ituWidgetSetBoundImpl (ITUWidget * widget, int x, int y, int width, int height)
{
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    widget->bound.x      = x;
    widget->bound.y      = y;
    widget->bound.width  = width;
    widget->bound.height = height;
    widget->dirty        = true;
}

bool ituWidgetIsInsideImpl (ITUWidget * widget, int x, int y)
{
    int bw, bh;

    ITU_ASSERT_THREAD();

    if (!widget->visible)
    {
        return false;
    }

    bw = widget->bound.width ? widget->bound.width : widget->rect.width;
    bh = widget->bound.height ? widget->bound.height : widget->rect.height;

    if (x >= 0 && y >= 0 && x <= bw && y <= bh)
    {
        return true;
    }
    return false;
}

void ituWidgetSetClipping (ITUWidget * widget, ITUSurface * dest, int x, int y, ITURectangle * prevClip)
{
    assert(widget != NULL);
    assert(dest != NULL);
    assert(prevClip != NULL);
    ITU_ASSERT_THREAD();

    (void)memcpy(prevClip, &dest->clipping, sizeof(ITURectangle));

    if (dest->flags & ITU_CLIPPING)
    {
        int cx, cy, cw, ch;
        int left, top, right, bottom, r1, r2;

        if (widget->bound.width > 0 || widget->bound.height > 0)
        {
            x += widget->bound.x;
            y += widget->bound.y;

            left   = x > prevClip->x ? x : prevClip->x;

            r1     = x + widget->bound.width;
            r2     = prevClip->x + prevClip->width;
            right  = r1 < r2 ? r1 : r2;

            r1     = y + widget->bound.height;
            r2     = prevClip->y + prevClip->height;
            bottom = r1 < r2 ? r1 : r2;

            top    = y > prevClip->y ? y : prevClip->y;
        }
        else
        {
            x += widget->rect.x;
            y += widget->rect.y;

            left   = x > prevClip->x ? x : prevClip->x;

            r1     = x + widget->rect.width;
            r2     = prevClip->x + prevClip->width;
            right  = r1 < r2 ? r1 : r2;

            r1     = y + widget->rect.height;
            r2     = prevClip->y + prevClip->height;
            bottom = r1 < r2 ? r1 : r2;

            top    = y > prevClip->y ? y : prevClip->y;
        }

        cx = left;
        cy = top;
        cw = right - left;
        ch = bottom - top;

        if (cw < 0)
        {
            cw = 0;
        }

        if (ch < 0)
        {
            ch = 0;
        }

        ituSurfaceSetClipping(dest, cx, cy, cw, ch);
    }
    else
    {
        if (widget->bound.width > 0 || widget->bound.height > 0)
        {
            x += widget->bound.x;
            y += widget->bound.y;
            ituSurfaceSetClipping(dest, x, y, widget->bound.width, widget->bound.height);
        }
        else
        {
            x += widget->rect.x;
            y += widget->rect.y;
            ituSurfaceSetClipping(dest, x, y, widget->rect.width, widget->rect.height);
        }
    }
}

static bool ValueInRange (int value, int min, int max)
{
    return (value >= min) && (value <= max);
}

static bool RectOverlap (const ITURectangle * A, const ITURectangle * B)
{
    bool xOverlap = ValueInRange(A->x, B->x, B->x + B->width) || ValueInRange(B->x, A->x, A->x + A->width);

    bool yOverlap = ValueInRange(A->y, B->y, B->y + B->height) || ValueInRange(B->y, A->y, A->y + A->height);

    return xOverlap && yOverlap;
}

bool ituWidgetIsOverlapClipping (ITUWidget * widget, ITUSurface * dest, int x, int y)
{
    ITURectangle A = { 0,0,0,0 };
    ITURectangle B = { 0,0,0,0 };

    if (widget->bound.width > 0 && widget->bound.height)
    {
        A.x      = x + widget->bound.x;
        A.y      = y + widget->bound.y;
        A.width  = widget->bound.width;
        A.height = widget->bound.height;
    }
    else if (widget->rect.width > 0 && widget->rect.height > 0)
    {
        A.x      = x + widget->rect.x;
        A.y      = y + widget->rect.y;
        A.width  = widget->rect.width;
        A.height = widget->rect.height;
    }
    else
    {
        return false;
    }

    if (dest->flags & ITU_CLIPPING)
    {
        B.x      = dest->clipping.x;
        B.y      = dest->clipping.y;
        B.width  = dest->clipping.width;
        B.height = dest->clipping.height;
    }
    else
    {
        B.x      = 0;
        B.y      = 0;
        B.width  = dest->width;
        B.height = dest->height;
    }

    return RectOverlap(&A, &B);
}

void ituWidgetGetGlobalPositionImpl (ITUWidget * widget, int * x, int * y)
{
    int cx = 0;
    int cy = 0;

    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    while (widget)
    {
        cx += widget->rect.x;
        cy += widget->rect.y;

        widget = (ITUWidget *)widget->tree.parent;
    }

    if (x != NULL)
    {
        *x = cx;
    }

    if (y != NULL)
    {
        *y = cy;
    }
}

void ituWidgetShowImpl (ITUWidget * widget, ITUEffectType effect, int step)
{
    ITUWidget*    target = widget;
    ITUEffectType oldEffect;
    int           oldStep;
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    if (widget->type == ITU_LAYER) //use first background child when use layer as target
    {
        ITCTree* tree = (ITCTree*)widget;
        ITCTree* child = NULL;
        for (child = tree->child; child; child = child->sibling)
        {
            ITUWidget* widgetObj = (ITUWidget*)child;
            if (widgetObj->type == ITU_BACKGROUND)
            {
                target = widgetObj;
                break;
            }
        }
        if (target == widget)
        {
            return; //found none valid background target under layer first tree level
        }
    }

    oldEffect = target->effects[ITU_STATE_SHOWING];
    oldStep = target->effects[ITU_STATE_NORMAL];
    ituWidgetSetEffect(target, ITU_STATE_NORMAL, step); // use target->effects[ITU_STATE_NORMAL] to store step parameter
    ituWidgetSetEffect(target, ITU_STATE_SHOWING, effect);
    if (widget->type == ITU_LAYER)
    {
        ituWidgetSetVisible(widget, true); //show this layer first
    }
    ituWidgetSetVisible(target, true);
    target->effects[ITU_STATE_SHOWING] = oldEffect;
    target->effects[ITU_STATE_NORMAL] = oldStep;
}

void ituWidgetHideImpl (ITUWidget * widget, ITUEffectType effect, int step)
{
    ITUWidget*    target = widget;
    ITUEffectType oldEffect;
    int           oldStep;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (widget->type == ITU_LAYER) //use first background child when use layer as target
    {
        ITCTree* tree = (ITCTree*)widget;
        ITCTree* child = NULL;
        for (child = tree->child; child; child = child->sibling)
        {
            ITUWidget* widgetObj = (ITUWidget*)child;
            if (widgetObj->type == ITU_BACKGROUND)
            {
                target = widgetObj;
                break;
            }
        }
        if (target == widget)
        {
            return; //found none valid background target under layer first tree level
        }
    }

    oldEffect = target->effects[ITU_STATE_HIDING];
    oldStep = target->effects[ITU_STATE_NORMAL];
    ituWidgetSetEffect(target, ITU_STATE_NORMAL, step); // use target->effects[ITU_STATE_NORMAL] to store step parameter
    ituWidgetSetEffect(target, ITU_STATE_HIDING, effect);
    ituWidgetSetVisible(target, false);
    if (widget->type == ITU_LAYER)
    {
        widget->state = ITU_STATE_HIDING; //mark layer into hiding.
    }
    target->effects[ITU_STATE_HIDING] = oldEffect;
    target->effects[ITU_STATE_NORMAL] = oldStep;
}

void ituWidgetToTopImpl (ITUWidget * widget)
{
    ITCTree * parent;
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    parent = widget->tree.parent;

    if (parent != NULL)
    {
        itcTreeRemove(widget);
        itcTreePushFront(parent, widget);
    }
}

void ituWidgetToBottomImpl (ITUWidget * widget)
{
    ITCTree * parent;
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    parent = widget->tree.parent;

    if (parent != NULL)
    {
        itcTreeRemove(widget);
        itcTreePushBack(parent, widget);
    }
}

void ituWidgetMoveUpImpl (ITUWidget * widget)
{
    ITCTree * parent;
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    parent = widget->tree.parent;

    if (parent != NULL)
    {
        ITCTree * child = parent->child;
        if (child == (ITCTree *)widget)
        {
            return;
        }

        while (child != NULL)
        {
            ITCTree * sibling = child->sibling;
            if (sibling == (ITCTree *)widget)
            {
                itcTreeSwap(child, sibling);
                return;
            }
            child = sibling;
        }
    }
}

void ituWidgetMoveDownImpl (ITUWidget * widget)
{
    ITCTree * parent;
    ITCTree * sibling;
    assert(widget != NULL);
    ITU_ASSERT_THREAD();

    parent  = widget->tree.parent;
    sibling = widget->tree.sibling;
    if (parent && sibling)
    {
        itcTreeSwap(widget, sibling);
    }
}
