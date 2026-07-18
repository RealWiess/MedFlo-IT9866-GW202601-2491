#include <sys/ioctl.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include "itu_cfg.h"
#include "ite/itp.h"
#include "ite/itu.h"

#ifdef CFG_UCL_LIBRARY
    #include "ucl/ucl.h"
#endif

#define ITUSCENE_DEBUG 0
#if ITUSCENE_DEBUG
#define sceneLog(fmt, ...) (void)printf(fmt, ##__VA_ARGS__)
//#define sceneLog(fmt, ...) (void)printf("[%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define sceneLogW(fmt, ...) (void)wprintf(L"[%S][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define sceneLog(fmt, ...)
#define sceneLogW(fmt, ...)
#endif

void ituSceneInit (ITUScene * scene, ITUWidget * root)
{
    assert(scene);

    (void)memset(scene, 0, sizeof(ITUScene));
    scene->root         = root;

    ituScene            = scene;
    scene->threadID     = pthread_self();
    scene->dbuffIDX     = 0;
    scene->prefer_lang  = -1;
    ituScene->ErrorFunc = ErrorFuncEmpty;
}

void ituSceneExit (ITUScene * scene)
{
    ITCTree * node;
    assert(scene);
    ITU_ASSERT_THREAD();

    if (ITU_USE_SPRITE_COMMON_BUF)
    {
        if (scene->surfpl[0])
        {
            if (scene->vPoolBuf[0])
            {
                ITU_LOG_DBG("[ituSceneExit][free scene->vPoolBuf-%d]\n", 0);
                itpVmemFree(scene->vPoolBuf[0]);
                scene->vPoolBuf[0] = 0;
            }
            scene->surfpl[0]->flags &= ~ITU_SPRITE_BUF;
            ituDestroySurface(scene->surfpl[0]);
            scene->surfpl[0] = NULL;
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (scene)
    {
        int i = 0;
        for (i = 0; i < ITU_DOUBLE_DBUFF; i++)
        {
            if (scene->surfpl[i])
            {
                if (scene->surfpl[i]->flags & ITU_SURFPOOL)
                {
                    if (scene->vPoolBuf[i])
                    {
                        ITU_LOG_DBG("[ituSceneExit][free scene->vPoolBuf-%d]\n", i);
                        itpVmemFree(scene->vPoolBuf[i]);
                        scene->vPoolBuf[i] = 0;
                    }
                    if (scene->vAlphaBuf[i])
                    {
                        ITU_LOG_DBG("[ituSceneExit][free scene->vAlphaBuf-%d]\n", i);
                        itpVmemFree(scene->vAlphaBuf[i]);
                        scene->vAlphaBuf[i] = 0;
                    }
                    scene->surfpl[i]->flags &= ~ITU_SURFPOOL;
                    ituDestroySurface(scene->surfpl[i]);
                    scene->surfpl[i] = NULL;
                }
            }
        }
    }
#endif

#ifdef CFG_ITU_FONT_SURFACE_CACHE
    if (scene)
    {
        int i     = 0;
        int count = 0;
        for (; i < ITU_FONT_SURFACE_CACHE_STRING_COUNT; i++)
        {
            if (scene->fontSurf[i].surf || scene->fontSurf[i].wStr)
            {
                scene->fontArrHash[i].hash       = 0;
                scene->fontArrHash[i].colorValue = 0;
                scene->fontArrHash[i].specValue  = 0;
                scene->fontArrHash[i].strValue   = 0;

                if (scene->fontSurf[i].surf)
                {
                    free(scene->fontSurf[i].surf);
                    scene->fontSurf[i].surf = NULL;
                }
                if (scene->fontSurf[i].wStr)
                {
                    free(scene->fontSurf[i].wStr);
                    scene->fontSurf[i].wStr = NULL;
                }

                count++;
            }
        }
        if (count)
        {
            (void)printf("[ituSceneExit][free scene->fontSurf x %d]\n", count);
        }
    }
#endif

    if (scene)
    {
        for (node = (ITCTree *)scene->root; node; node = node->sibling)
        {
            ituWidgetExit(node);
        }

        if (scene->bufferAllocated)
        {
            (void)printf("[ituSceneExit][free scene->buffer]\n");
            free(scene->buffer);
        }
    }
}

bool ituSceneUpdate (ITUScene * scene, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool      result     = false;
    ITCTree * node       = NULL;
    int       childCount = 0;

#ifdef ITUWIDGET_TREE_USE_ALLOCA
    ITUWidget ** children = NULL;
#else
    ITUWidget * children[ITU_WIDGET_CHILD_MAX];
#endif

    assert(scene);
    assert(scene->root);
    ITU_ASSERT_THREAD();

    if (ev == ITU_EVENT_MOUSELONGPRESS && scene->dragged)
    {
        return false;
    }

    if (ev == ITU_EVENT_MOUSEDOWN || ev == ITU_EVENT_MOUSEUP || ev == ITU_EVENT_MOUSEDOUBLECLICK ||
        ev == ITU_EVENT_MOUSEMOVE || ev == ITU_EVENT_MOUSELONGPRESS || ev == ITU_EVENT_TOUCHSLIDELEFT ||
        ev == ITU_EVENT_TOUCHSLIDEUP || ev == ITU_EVENT_TOUCHSLIDERIGHT || ev == ITU_EVENT_TOUCHSLIDEDOWN ||
        ev == ITU_EVENT_TOUCHPINCH)
    {
        if (scene->rotation == ITU_ROT_0)
        {
            scene->lastMouseX = arg2;
            scene->lastMouseY = arg3;
        }
        else
        {
            int x, y;

            if (scene->rotation == ITU_ROT_90)
            {
                x = arg3;
                y = scene->screenWidth - arg2;

                switch (ev)
                {
                    case ITU_EVENT_TOUCHSLIDELEFT:
                        ev = ITU_EVENT_TOUCHSLIDEDOWN;
                        break;

                    case ITU_EVENT_TOUCHSLIDEUP:
                        ev = ITU_EVENT_TOUCHSLIDELEFT;
                        break;

                    case ITU_EVENT_TOUCHSLIDERIGHT:
                        ev = ITU_EVENT_TOUCHSLIDEUP;
                        break;

                    case ITU_EVENT_TOUCHSLIDEDOWN:
                        ev = ITU_EVENT_TOUCHSLIDERIGHT;
                        break;
                }
            }
            else if (scene->rotation == ITU_ROT_180)
            {
                x = scene->screenWidth - arg2;
                y = scene->screenHeight - arg3;

                switch (ev)
                {
                    case ITU_EVENT_TOUCHSLIDELEFT:
                        ev = ITU_EVENT_TOUCHSLIDERIGHT;
                        break;

                    case ITU_EVENT_TOUCHSLIDEUP:
                        ev = ITU_EVENT_TOUCHSLIDEDOWN;
                        break;

                    case ITU_EVENT_TOUCHSLIDERIGHT:
                        ev = ITU_EVENT_TOUCHSLIDELEFT;
                        break;

                    case ITU_EVENT_TOUCHSLIDEDOWN:
                        ev = ITU_EVENT_TOUCHSLIDEUP;
                        break;
                }
            }
            else // if (scene->rotation == ITU_ROT_270)
            {
                x = scene->screenHeight - arg3;
                y = arg2;

                switch (ev)
                {
                    case ITU_EVENT_TOUCHSLIDELEFT:
                        ev = ITU_EVENT_TOUCHSLIDEUP;
                        break;

                    case ITU_EVENT_TOUCHSLIDEUP:
                        ev = ITU_EVENT_TOUCHSLIDERIGHT;
                        break;

                    case ITU_EVENT_TOUCHSLIDERIGHT:
                        ev = ITU_EVENT_TOUCHSLIDEDOWN;
                        break;

                    case ITU_EVENT_TOUCHSLIDEDOWN:
                        ev = ITU_EVENT_TOUCHSLIDELEFT;
                        break;
                }
            }
            scene->lastMouseX = arg2 = x;
            scene->lastMouseY = arg3 = y;
        }
        scene->lastMouseEvent = ev;

        if (ev == ITU_EVENT_TOUCHPINCH)
        {
            scene->lastMouseDist = arg1;
        }
    }

#ifdef ITUWIDGET_TREE_USE_ALLOCA
    for (node = &scene->root->tree; node; node = node->sibling)
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

    for (node = &scene->root->tree; node; node = node->sibling)
    {
        children[childCount++] = (ITUWidget *)node;
    }

    switch (ev)
    {
        case ITU_EVENT_LANGUAGE:
        case ITU_EVENT_LOAD_IMAGE:
            while (--childCount >= 0)
            {
                ITUWidget * widget = children[childCount];
                result |= ituWidgetUpdate(widget, ev, arg1, arg2, arg3);
            }
            break;

        case ITU_EVENT_PRESS:
        case ITU_EVENT_KEYDOWN:
        case ITU_EVENT_MOUSEDOWN:
        case ITU_EVENT_MOUSEDOUBLECLICK:
        case ITU_EVENT_MOUSEMOVE:
        case ITU_EVENT_TOUCHSLIDELEFT:
        case ITU_EVENT_TOUCHSLIDEUP:
        case ITU_EVENT_TOUCHSLIDERIGHT:
        case ITU_EVENT_TOUCHSLIDEDOWN:
        case ITU_EVENT_MOUSELONGPRESS:
            while (--childCount >= 0)
            {
                ITUWidget * widget = children[childCount];
                if (widget->visible)
                {
                    result |= ituWidgetUpdate(widget, ev, arg1, arg2, arg3);
                    if (result)
                    {
                        break;
                    }
                }
            }
            break;

        case ITU_EVENT_KEYUP:
        case ITU_EVENT_MOUSEUP:
            while (--childCount >= 0)
            {
                ITUWidget * widget = children[childCount];
                result |= ituWidgetUpdate(widget, ev, arg1, arg2, arg3);
                if (result)
                {
                    break;
                }
            }
            break;

        default:
            for (node = &scene->root->tree; node; node = node->sibling)
            {
                ITUWidget * widget = (ITUWidget *)node;
                if (widget->visible)
                {
                    result |= ituWidgetUpdate(widget, ev, arg1, arg2, arg3);
                }
            }
            break;
    }

    if (scene->focused && (ev == ITU_EVENT_KEYUP || ev == ITU_EVENT_MOUSEUP || ev == ITU_EVENT_TOUCHSLIDEUP))
    {
        ITUWidget * focused = scene->focused;
        if (focused->type == ITU_BUTTON || focused->type == ITU_CHECKBOX || focused->type == ITU_RADIOBOX)
        {
            if (ituButtonIsPressed(focused))
            {
                ituWidgetUpdate(focused, ev, arg1, arg2, arg3);
            }
        }
    }

    if (ev < ITU_EVENT_CUSTOM)
    {
        // execute delay queue
        if (ev == ITU_EVENT_TIMER)
        {
            int i = 0;
            ituExecDelayQueue(scene->delayQueue);
            // execute delayed commands
            for (; i < ITU_COMMAND_SIZE; i++)
            {
                ITUCommand * cmd = &scene->commands[i];
                if (cmd->delay > 0)
                {
                    if (--cmd->delay == 0)
                    {
                        cmd->func(cmd->arg);
                        result = true;
                    }
                }
            }
        }

        // flush action queue
        if (ev != ITU_EVENT_LANGUAGE) //prevant enter event loop infinitely when use Action/Function callback
        {
            result |= ituFlushActionQueue(scene->actionQueue, &scene->actionQueueLen);
        }
    }

    if (ev == ITU_EVENT_MOUSEUP)
    {
        scene->dragged = NULL;
    }

    return result;
}

void ituSceneDraw (ITUScene * scene, ITUSurface * dest)
{
    ITCTree * node;
    assert(scene);
    assert(scene->root);
    assert(dest);
    ITU_ASSERT_THREAD();

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (scene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        (void)printf("[SurfacePool] SceneDraw but SurfacePool not ready yet!\n");
        return;
    }
#endif

    for (node = &scene->root->tree; node; node = node->sibling)
    {
        ITUWidget * widget = (ITUWidget *)node;
        if (widget->visible)
        {
            ituWidgetDraw(widget, dest, 0, 0, 255);
        }

        widget->dirty = false;
    }
}

void ituScenePreDraw (ITUScene * scene, ITUSurface * dest)
{
#ifdef CFG_ITU_FT_CACHE_SIZE
    void (*DrawGlyphPrev)(ITUSurface * surf, int x, int y, ITUGlyphFormat format, const uint8_t * bitmap, int w,
                          int h) = ituDrawGlyph;
    ITCTree * node;
    assert(scene);
    assert(scene->root);
    assert(dest);
    ITU_ASSERT_THREAD();

    ituDrawGlyph = ituDrawGlyphEmpty;

    for (node = &scene->root->tree; node; node = node->sibling)
    {
        ITUWidget * widget = (ITUWidget *)node;

        if (widget->flags & ITU_PREDRAW)
        {
            ituPreloadFontCache(widget, dest);
            ITU_LOG_INFO("predraw %s\n", widget->name);
            // sched_yield();
        }
    }
    ituDrawGlyph = DrawGlyphPrev;

#endif // CFG_ITU_FT_CACHE_SIZE
}

void ituSceneSetRotation (ITUScene * scene, ITURotation rot, int screenWidth, int screenHeight)
{
    assert(scene);
    ITU_ASSERT_THREAD();

#ifdef CFG_LCD_MULTIPLE
    if (rot == ITU_ROT_90 || rot == ITU_ROT_270)
    {
        screenWidth  = ithLcdGetHeight();
        screenHeight = ithLcdGetWidth();
    }
    else
    {
        screenWidth  = ithLcdGetWidth();
        screenHeight = ithLcdGetHeight();
    }
#endif
    scene->rotation     = rot;
    scene->screenWidth  = screenWidth;
    scene->screenHeight = screenHeight;

    ituSetRotation(rot);
}

void ituSceneSetFunctionTable (ITUScene * scene, ITUActionFunction * funcTable)
{
    assert(scene);
    ITU_ASSERT_THREAD();
    scene->actionFuncTable = funcTable;
}

void ituSceneSetLoadFunctionTable (ITUScene * scene, ITULoadFunction * funcTable)
{
    assert(scene);
    ITU_ASSERT_THREAD();
    scene->loadFuncTable = funcTable;
}

ITUActionFunc ituSceneFindFunction (ITUScene * scene, const char * name)
{
    assert(scene);
    assert(name);

    if (scene->actionFuncTable)
    {
        ITUActionFunction * actionFunc = scene->actionFuncTable;
        while (actionFunc->func)
        {
            if (actionFunc->name == NULL || strcmp(actionFunc->name, name) == 0)
            {
                return actionFunc->func;
            }
            actionFunc++;
        }
    }
    return NULL;
}

void * ituSceneFindWidget (ITUScene * scene, const char * name)
{
    ITCTree * node;
    assert(scene);
    assert(name);
    ITU_ASSERT_THREAD();

    if (!scene->root)
    {
        return NULL;
    }

    for (node = &scene->root->tree; node; node = node->sibling)
    {
        ITUWidget * result = ituFindWidgetChild(node, name);
        if (result)
        {
            return result;
        }
    }
    return NULL;
}

int ituSceneGetVersion(ITUScene* scene)
{
    ITCTree* node;
    assert(scene);

    if (!scene->root)
    {
        return 0;
    }

    node = &scene->root->tree;
    if (node != NULL)
    {
        ITULayer* layer = (ITULayer*)node;
        return layer->xmlVersion;
    }

    return 0;
}

static ITUWidget * FocusFirst (ITUWidget * widget)
{
    if (widget->visible && widget->flags & ITU_TAPSTOP)
    {
        return widget;
    }
    else
    {
        ITCTree * node;

        for (node = widget->tree.child; node; node = node->sibling)
        {
            ITUWidget * foundObj = FocusFirst((ITUWidget *)node);
            if (foundObj)
            {
                return foundObj;
            }
        }
    }
    return NULL;
}

static ITUWidget * FocusPrev (ITUWidget * widget)
{
    ITUWidget * prev = NULL;
    assert(widget);

    if (widget->flags & ITU_TAPSTOP)
    {
        const ITCTree * parent = widget->tree.parent;

        if (parent)
        {
            ITCTree * node;

            for (node = parent->child; node; node = node->sibling)
            {
                ITUWidget * curr = (ITUWidget *)node;

                if (!curr->visible || curr == widget)
                {
                    continue;
                }

                if (prev)
                {
                    if ((curr->tabIndex < widget->tabIndex) && (curr->tabIndex > prev->tabIndex))
                    {
                        prev = curr;
                    }
                }
                else
                {
                    if (curr->tabIndex < widget->tabIndex)
                    {
                        prev = curr;
                    }
                }
            }

            if (prev == NULL)
            {
                for (node = parent->child; node; node = node->sibling)
                {
                    ITUWidget * curr = (ITUWidget *)node;

                    if (!curr->visible || curr == widget)
                    {
                        continue;
                    }

                    if (prev)
                    {
                        if (curr->tabIndex > prev->tabIndex)
                        {
                            prev = curr;
                        }
                    }
                    else
                    {
                        prev = curr;
                    }
                }
            }
        }
    }

    if (prev == NULL)
    {
        prev = widget;
    }

    return prev;
}

ITUWidget * ituSceneFocusPrev (ITUScene * scene)
{
    ITU_ASSERT_THREAD();

    if (!scene->focused)
    {
        ITCTree * node;

        for (node = &scene->root->tree; node; node = node->sibling)
        {
            scene->focused = FocusFirst((ITUWidget *)node);
            if (scene->focused)
            {
                break;
            }
        }
    }
    else
    {
        ituWidgetSetActive(scene->focused, false);
        scene->focused = FocusPrev(scene->focused);
    }

    if (scene->focused)
    {
        ituWidgetSetActive(scene->focused, true);
    }

    return scene->focused;
}

static ITUWidget * FocusNext (ITUWidget * widget)
{
    ITUWidget * next = NULL;
    assert(widget);

    if (widget->flags & ITU_TAPSTOP)
    {
        const ITCTree * parent = widget->tree.parent;

        if (parent)
        {
            ITCTree * node;

            for (node = parent->child; node; node = node->sibling)
            {
                ITUWidget * curr = (ITUWidget *)node;

                if (!curr->visible || curr == widget)
                {
                    continue;
                }

                if (next)
                {
                    if ((curr->tabIndex > widget->tabIndex) && (curr->tabIndex < next->tabIndex))
                    {
                        next = curr;
                    }
                }
                else
                {
                    if (curr->tabIndex > widget->tabIndex)
                    {
                        next = curr;
                    }
                }
            }

            if (next == NULL)
            {
                for (node = parent->child; node; node = node->sibling)
                {
                    ITUWidget * curr = (ITUWidget *)node;

                    if (!curr->visible || curr == widget)
                    {
                        continue;
                    }

                    if (next)
                    {
                        if (curr->tabIndex < next->tabIndex)
                        {
                            next = curr;
                        }
                    }
                    else
                    {
                        next = curr;
                    }
                }
            }
        }
    }

    if (next == NULL)
    {
        next = widget;
    }

    return next;
}

ITUWidget * ituSceneFocusNext (ITUScene * scene)
{
    ITU_ASSERT_THREAD();

    if (!scene->focused)
    {
        ITCTree * node;

        for (node = &scene->root->tree; node; node = node->sibling)
        {
            scene->focused = FocusFirst((ITUWidget *)node);
            if (scene->focused)
            {
                break;
            }
        }
    }
    else
    {
        ituWidgetSetActive(scene->focused, false);
        scene->focused = FocusNext(scene->focused);
    }

    if (scene->focused)
    {
        ituWidgetSetActive(scene->focused, true);
    }

    return scene->focused;
}

void ituSceneExecuteCommand (ITUScene * scene, int delay, ITUCommandFunc func, int arg)
{
    int i;
    assert(scene);
    assert(func);

    // try to replace exist command
    for (i = 0; i < ITU_COMMAND_SIZE; i++)
    {
        ITUCommand * cmd = &scene->commands[i];
        if (cmd->func == func && cmd->arg == arg)
        {
            cmd->delay = delay;
            return;
        }
    }

    // add new command
    for (i = 0; i < ITU_COMMAND_SIZE; i++)
    {
        ITUCommand * cmd = &scene->commands[i];
        if (cmd->delay == 0)
        {
            cmd->delay = delay;
            cmd->func  = func;
            cmd->arg   = arg;
            break;
        }
    }
}

static int ituSceneResetFCS(ITUScene* scene, int resetUnit)
{
    if ((scene != NULL) && (resetUnit > 0))
    {
        int i = 0;
        for (; i < resetUnit; i++)
        {
            if (i >= ITU_FONT_SURFACE_CACHE_STRING_COUNT)
            {
                break;
            }
            scene->fontArrHash[i].hash = 0;
            scene->fontArrHash[i].colorValue = 0;
            scene->fontArrHash[i].specValue = 0;
            scene->fontArrHash[i].strValue = 0;
            scene->fontSurf[i].x = scene->fontSurf[i].y = 0;
            scene->fontSurf[i].w = scene->fontSurf[i].h = 0;
            scene->fontSurf[i].fr = scene->fontSurf[i].fp = 0;
            scene->fontSurf[i].lb = scene->fontSurf[i].wrb = 0;
            if (scene->fontSurf[i].surf != NULL)
            {
                free(scene->fontSurf[i].surf);
                scene->fontSurf[i].surf = NULL;
            }

            if (scene->fontSurf[i].wStr != NULL)
            {
                free(scene->fontSurf[i].wStr);
                scene->fontSurf[i].wStr = NULL;
            }
        }

        for (i = resetUnit; i < ITU_FONT_SURFACE_CACHE_STRING_COUNT; i++)
        {
            scene->fontArrHash[i - resetUnit].hash = scene->fontArrHash[i].hash;
            scene->fontArrHash[i - resetUnit].colorValue = scene->fontArrHash[i].colorValue;
            scene->fontArrHash[i - resetUnit].specValue = scene->fontArrHash[i].specValue;
            scene->fontArrHash[i - resetUnit].strValue = scene->fontArrHash[i].strValue;
            scene->fontSurf[i - resetUnit].x = scene->fontSurf[i].x;
            scene->fontSurf[i - resetUnit].y = scene->fontSurf[i].y;
            scene->fontSurf[i - resetUnit].w = scene->fontSurf[i].w;
            scene->fontSurf[i - resetUnit].h = scene->fontSurf[i].h;
            scene->fontSurf[i - resetUnit].fr = scene->fontSurf[i].fr;
            scene->fontSurf[i - resetUnit].fp = scene->fontSurf[i].fp;
            scene->fontSurf[i - resetUnit].lb = scene->fontSurf[i].lb;
            scene->fontSurf[i - resetUnit].wrb = scene->fontSurf[i].wrb;
            if (scene->fontSurf[i].surf != NULL)
            {
                scene->fontSurf[i - resetUnit].surf = scene->fontSurf[i].surf;
                scene->fontSurf[i].surf = NULL;
            }

            if (scene->fontSurf[i].wStr != NULL)
            {
                scene->fontSurf[i - resetUnit].wStr = wcsdup(scene->fontSurf[i].wStr);
                free(scene->fontSurf[i].wStr);
                scene->fontSurf[i].wStr = NULL;
            }
            scene->fontArrHash[i].hash = 0;
            scene->fontArrHash[i].colorValue = 0;
            scene->fontArrHash[i].specValue = 0;
            scene->fontArrHash[i].strValue = 0;
            scene->fontSurf[i].x = scene->fontSurf[i].y = 0;
            scene->fontSurf[i].w = scene->fontSurf[i].h = 0;
            scene->fontSurf[i].fr = scene->fontSurf[i].fp = 0;
            scene->fontSurf[i].lb = scene->fontSurf[i].wrb = 0;
        }
        return i;
    }

    return 0;
}

#ifdef ituSceneFontArrHashAdd_COMMENT
//use BKDR
//seed -> 131 1313 13131 131313
//(bold > 0xFF)->check HarfBuzz
//return true -> add now or (found but without surf)
//return false -> cached or use harfbuzz
#endif
bool ituSceneFontArrHashAdd(ITUScene* scene, int fontIndex, int bold, wchar_t* str, int height, int* hId, const ITUColor* color)
{
    int          i = 0;
    int          resetUnit = (ITU_FONT_SURFACE_CACHE_STRING_COUNT > 100) ? (100) : (ITU_FONT_SURFACE_CACHE_STRING_COUNT);
    unsigned int seed = 131;
    unsigned int hash = height;
    uint32_t     color_value = 0;
    uint32_t     specValue = 0;
    uint32_t     strValue = 0;
    unsigned int charCode = str[0];
    int          boldValue = bold;
    bool         hbCheck = (bold > 0xFF) ? (true) : (false);

    hash = hash * seed + charCode + hash * (boldValue << 8) + hash * height;
    hash *= seed;
    color_value = (color->alpha << 24) + (color->red << 16) + (color->green << 8) + (color->blue);
    specValue = (fontIndex << 24) + ((boldValue > 0) ? (boldValue * hash) : (0)) + (height << 8) + charCode;
    strValue = charCode + height;

    assert(scene);

    for (; i < ITU_FONT_SURFACE_CACHE_STRING_COUNT; i++)
    {
        *hId = i;
        if (scene->fontArrHash[i].hash == 0)
        {
            scene->fontArrHash[i].hash = hash;
            scene->fontArrHash[i].colorValue = color_value;
            scene->fontArrHash[i].specValue = specValue;
            scene->fontArrHash[i].strValue = strValue;

            if (scene->fontSurf[i].wStr != NULL)
            {
                free(scene->fontSurf[i].wStr);
                scene->fontSurf[i].wStr = NULL;
            }

            if (!hbCheck)
            {
                scene->fontSurf[i].wStr = wcsdup(str);
                sceneLog("============================================\n");
                sceneLog("New char [%s] [size %d] to fsc[%d]\n", scene->fontSurf[i].wStr, height, i);
                sceneLog("============================================\n\n");
            }
            return true;
        }
        else if ((scene->fontSurf[i].surf != NULL) && (scene->fontArrHash[i].hash == hash)) //already get size from before
        {
            if ((scene->fontArrHash[i].specValue == specValue) 
                && (scene->fontArrHash[i].strValue == strValue)
                && (scene->fontArrHash[i].colorValue == color_value))
            {
                if (scene->fontSurf[i].wStr != NULL)
                {
                    free(scene->fontSurf[i].wStr);
                    scene->fontSurf[i].wStr = NULL;
                }
                return false;
            }
        }
        else if ((scene->fontArrHash[i].hash == hash) &&
            (scene->fontArrHash[i].specValue == specValue) && (scene->fontArrHash[i].strValue == strValue))
        {
            if (scene->fontSurf[i].surf == NULL)
            {
                scene->fontArrHash[i].colorValue = color_value;
                return true;
            }
            else if (scene->fontArrHash[i].colorValue == color_value)
            {
                if (hbCheck)
                {
                    return false;
                }
                else if (scene->fontSurf[i].wStr != NULL)
                {
                    if (wcsncmp(str, scene->fontSurf[i].wStr, 1) == 0)
                    {
                        return false;
                    }
                }
            }
        }
    }

    i = ituSceneResetFCS(scene, resetUnit);

    if (i >= resetUnit)
    {
        i = i - resetUnit; //reset index to idle slot
    }
    else
    {
        i = 0;
    }

    *hId = i;
    scene->fontArrHash[i].hash = hash;
    scene->fontArrHash[i].colorValue = color_value;
    scene->fontArrHash[i].specValue = specValue;
    scene->fontArrHash[i].strValue = strValue;
    scene->fontSurf[i].wStr = wcsdup(str);
    sceneLog("============================================\n");
    sceneLog("[reset nodes %d]New char [%s] to fsc [%d]\n", resetUnit, scene->fontSurf[i].wStr, i);
    sceneLog("============================================\n\n\n\n");
    return true;
}

#ifdef ituSceneFontArrHashSizeCheck_COMMENT
// use BKDR
// seed -> 131 1313 13131 131313
//(bold > 0xFF)->check HarfBuzz
//return true -> found cached
//return false -> not cached yet
#endif
bool ituSceneFontArrHashSizeCheck(ITUScene* scene, int fontIndex, int bold, wchar_t* str, int height, int* hId)
{
    int          i = 0;
    int          resetUnit = (ITU_FONT_SURFACE_CACHE_STRING_COUNT > 100) ? (100) : (ITU_FONT_SURFACE_CACHE_STRING_COUNT);
    unsigned int seed = 131;
    unsigned int hash = height;
    uint32_t     specValue = 0;
    uint32_t     strValue = 0;
    wchar_t      buf[512] = L"";
    wchar_t      wStr[2] = L"";
    int          boldValue = bold;
    unsigned int charCode = str[0];

    hash = hash * seed + charCode + hash * (boldValue << 8) + hash * height;
    hash *= seed;

    specValue = (fontIndex << 24) + ((boldValue > 0) ? (boldValue * hash) : (0)) + (height << 8) + charCode;
    strValue = charCode + height;

    assert(scene);

    for (i = 0; i < ITU_FONT_SURFACE_CACHE_STRING_COUNT; i++)
    {
        *hId = i;
        if ((scene->fontArrHash[i].hash == hash) && (scene->fontArrHash[i].specValue == specValue) &&
            (scene->fontArrHash[i].strValue == strValue) && (scene->fontSurf[i].wStr != NULL))
        {
            if (wcsncmp(str, scene->fontSurf[i].wStr, 1) == 0)
            {
                if (scene->fontSurf[i].h == 0)
                {
                    sceneLog("[%s]cached but no dimension data in fsc[%d]\n", scene->fontSurf[i].wStr, i);
                    return false;
                }
                else
                {
                    sceneLog("Get cached dimension [%s] from fsc[%d]\n", scene->fontSurf[i].wStr, i);
                    return true;
                }
            }
        }
        
        if (scene->fontArrHash[i].hash == hash)
        {
            return true;
        }
        if (scene->fontArrHash[i].hash == 0)
        {
            scene->fontArrHash[i].hash = hash; //check again
            scene->fontArrHash[i].strValue = strValue; //check again
            scene->fontArrHash[i].specValue = specValue; //check again
            return false;
        }
    }

    i = ituSceneResetFCS(scene, resetUnit);

    if (i >= resetUnit)
    {
        i = i - resetUnit; //reset index to idle slot
    }
    else
    {
        i = 0;
    }

    *hId = i;

    sceneLog("============================================\n");
    sceneLog("[reset nodes %d]no space to cache dimension\n", resetUnit);
    sceneLog("============================================\n\n\n\n");

    return false;
}

void ituSceneFontArrHashSizePreSet(ITUScene* scene, int fontIndex, int bold, wchar_t* str, int height, int* hId)
{
    int          i = 0;
    int          resetUnit = (ITU_FONT_SURFACE_CACHE_STRING_COUNT > 100) ? (100) : (ITU_FONT_SURFACE_CACHE_STRING_COUNT);
    unsigned int seed = 131;
    unsigned int hash = height;
    uint32_t     specValue = 0;
    uint32_t     strValue = 0;
    int          boldValue = bold;
    unsigned int charCode = str[0];

    hash = hash * seed + charCode + hash * (boldValue << 8) + hash * height;
    hash *= seed;

    specValue = (fontIndex << 24) + ((boldValue > 0) ? (boldValue * hash) : (0)) + (height << 8) + charCode;
    strValue = charCode + height;

    assert(scene);

    for (i = 0; i < ITU_FONT_SURFACE_CACHE_STRING_COUNT; i++)
    {
        if (scene->fontArrHash[i].hash == hash)
        {
            break;
        }

        if (scene->fontArrHash[i].hash == 0)
        {
            *hId = i;
            //set hash first
            scene->fontArrHash[i].hash = hash;
            scene->fontArrHash[i].strValue = strValue;
            scene->fontArrHash[i].specValue = specValue;
            break;
        }
    }
}

void ituSceneCheckD2D (ITUScene * scene, bool drawReq)
{
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (scene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        return;
    }

    if (drawReq)
    {
        ituScene->surfpl[ituScene->dbuffIDX]->flags |= ITU_DRAWDCPS;
        ITU_LOG_DBG("[SceneCheckD2D][buffer %d][DCPS]\n", ituScene->dbuffIDX);
    }
    else
    {
        // ITU_LOG_DBG("[SceneCheckD2D][buffer %d][no DCPS]\n", ituScene->dbuffIDX );
    }
#endif
    return;
}

void ituSceneSetD2D (ITUScene * scene, const ITUWidget * widget)
{
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (scene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        return;
    }
    else if (widget)
    {
        if (widget->visible && (widget->alpha > 0))
        {
            ituScene->surfpl[ituScene->dbuffIDX]->flags |= ITU_DRAWREQ;
            ITU_LOG_DBG("[SceneSetD2D][buffer %d][%s][DRAWREQ]\n", ituScene->dbuffIDX, widget->name);
        }
        else
        {
            // ITU_LOG_DBG("[SceneSetD2D][buffer %d][%s][invisible]\n", ituScene->dbuffIDX, widget->name );
        }
    }
#endif
    return;
}

bool ituSceneClearD2D (ITUScene * scene)
{
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (scene->surfpl[ituScene->dbuffIDX] == NULL)
    {
        return false;
    }

    if (ituScene->surfpl[ituScene->dbuffIDX]->flags & ITU_DRAWREQ)
    {
        ituScene->surfpl[ituScene->dbuffIDX]->flags &= ~ITU_DRAWREQ;
        ITU_LOG_DBG("[SceneClearD2D][buffer %d][will DCPS]\n", ituScene->dbuffIDX);
        return true;
    }
#endif
    return false;
}

void ituSceneSetPreferLang (ITUScene * scene, int lang)
{
    assert(scene);
    if ((lang > 0) && (lang < ITU_STRINGSET_SIZE))
    {
        scene->prefer_lang = lang;
    }
    else
    {
        scene->prefer_lang = -1;
    }
}

void ErrorFuncEmpty (int errorCode)
{
    // empty here
}
