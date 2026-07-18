#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#pragma warning(disable : 4996)
static const char textName[] = "ITUText";

void              ituTextExit (ITUWidget * widget)
{
    ITUText * text = (ITUText *)widget;
    assert(widget);
    ITU_ASSERT_THREAD();

    if (text->string != NULL)
    {
        free(text->string);
        text->string = NULL;
    }
    if (text->tableString != NULL)
    {
        free(text->tableString);
        text->tableString = NULL;
    }
    ituWidgetExitImpl(widget);
}

bool ituTextClone (ITUWidget * widget, ITUWidget ** cloned)
{
    ITUText * text = (ITUText *)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUText));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUText));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    if (text->string != NULL)
    {
        ITUText * newText = (ITUText *)*cloned;
        newText->string   = strdup(text->string);
    }
    if (text->tableString != NULL)
    {
        ITUText * newText    = (ITUText *)*cloned;
        newText->tableString = strdup(text->tableString);
    }

    return ituWidgetCloneImpl(widget, cloned);
}

void ituTextRefreshSize (ITUText * text, unsigned int style)
{
    if (text)
    {
        ituFtSetCurrentFont(text->fontIndex);

        if (text->fontHeight > 0)
        {
            ituFtSetFontSize(text->fontWidth, text->fontHeight);
        }

        if (text->textFlags & ITU_TEXT_BOLD)
        {
            style |= ITU_FT_STYLE_BOLD;
            ituFtSetFontStyleValue(ITU_FT_STYLE_BOLD, text->boldSize);
        }

        ituFtSetFontStyle(style);
    }
}

bool ituTextUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool      result = false;
    ITUText * text   = (ITUText *)widget;
    assert(text);

    result = ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);

    if (ev == ITU_EVENT_LANGUAGE)
    {
        ituTextSetLanguage(text, arg1);
        result = widget->dirty = true;
    }
    else if (ev == ITU_EVENT_TIMER)
    {
        if (widget->pObj == ((ITCTree *)widget))
        {
            result       = true;
            widget->pObj = NULL;
        }
    }

    return widget->visible ? result : false;
}

void ituTextDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    ITUText *      text  = (ITUText *)widget;
    ITURectangle * rect  = (ITURectangle *)&widget->rect;
    ITCTree *      tree  = (ITCTree *)widget;
    int            destx = 0, desty = 0, boldSize = 0;
    int            widgetWidth  = widget->rect.width;
    int            widgetHeight = widget->rect.height;
    uint8_t        desta = 0, destbga = 0;
    bool           rsBypassCheck = true;
    ITURectangle   prevClip      = {0, 0, 0, 0};
    char *         string        = NULL;
    char *         newString     = NULL;
    unsigned int   style         = ITU_FT_STYLE_DEFAULT;

    ITUWidget *    realWidget   = widget;
    ITUWidget *    realWidgetBW = widget;
    assert(text);
    assert(dest);

    // check here for resize requirement
    if (tree->parent != NULL)
    {
        realWidgetBW = (ITUWidget *)tree->parent;
        if (text->textFlags & ITU_TEXT_BRIEF_LONGWORD)
        {
            rsBypassCheck = false;
        }
        else if (realWidgetBW->type == ITU_WHEEL)
        {
            rsBypassCheck = false;
        }
        else if (text->fontPriority[0] == -2) //backdoor for bypass resize
        {
            rsBypassCheck = false;
        }
        else
        {
            if (realWidgetBW->type == ITU_BORDERWINDOW)
            {
                rect = (ITURectangle*)&realWidgetBW->rect;
                widgetWidth = rect->width;
                widgetHeight = rect->height;
                destx = x;
                desty = y;
            }
            else
            {
                destx = rect->x + x;
                desty = rect->y + y;
            }
            /*if (realWidgetBW->type == ITU_BORDERWINDOW)
            {
                if (tree->parent->parent != NULL)
                {
                    realWidget   = (ITUWidget *)tree->parent->parent;
                    rect         = (ITURectangle *)&realWidgetBW->rect;
                    widgetWidth  = rect->width;
                    widgetHeight = rect->height;
                }
            }*/
        }
    }

    if ((widgetWidth == 0) || (widgetHeight == 0))
    {
        widget->dirty = false;
        return;
    }

    ituFtSetFontPriority(text->fontPriority, 0);

    if ((text->fontHeight >= ITU_FREETYPE_MAX_SBIT_CACHE) && (text->fontWidth >= ITU_FREETYPE_MAX_SBIT_CACHE))
    {
        ituFtResetCache();
        ituFtSetCurrentFont(text->fontIndex);
    }

    // if prefer enable, force replace default lang here
    if (ituScene)
    {
        if (ituScene->prefer_lang > 0)
        {
            text->lang = ituScene->prefer_lang;
        }
    }

    if (text->string != NULL)
    {
        string = text->string;

        if (text->textFlags & ITU_TEXT_ARABIC)
        {
            style |= ITU_FT_STYLE_ARABIC;
        }
        else if (text->textFlags & ITU_TEXT_HEBREW)
        {
            style |= ITU_FT_STYLE_HEBREW;
        }
    }
    else if (text->stringSet != NULL)
    {
        if (ituTextProcLangTableToString(text))
        {
            string = text->tableString;
        }
        else
        {
            string = text->stringSet->strings[text->lang];
        }

        if ((text->textFlags & ITU_TEXT_ARABIC) && ((1u << text->lang) & text->arabicIndices))
        {
            style |= ITU_FT_STYLE_ARABIC;
        }
        else if ((text->textFlags & ITU_TEXT_HEBREW) && ((1u << text->lang) & text->hebrewIndices))
        {
            style |= ITU_FT_STYLE_HEBREW;
        }
    }

    if (string == NULL)
    {
        widget->dirty = false;
        return;
    }

    if (text->textFlags & ITU_TEXT_BOLD)
    {
        boldSize = text->boldSize;
    }

    if (realWidgetBW->type == ITU_WHEEL)
    {
        destx = rect->x + x;
        desty = rect->y + y;
    }
    
    desta   = alpha * widget->color.alpha / 255;
    desta   = desta * widget->alpha / 255;
    destbga = alpha * text->bgColor.alpha / 255;
    destbga = destbga * widget->alpha / 255;

    if ((widget->flags & ITU_CLIP_DISABLED) == 0)
    {
        ituWidgetSetClipping(widget, dest, x, y, &prevClip);
    }

    if (destbga == 255)
    {
        ituColorFill(dest, destx, desty, rect->width, rect->height, &text->bgColor);
    }
    else if (destbga > 0)
    {
#ifndef CFG_M2D_ENABLE
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
        if (surf)
        {
            ituColorFill(surf, 0, 0, rect->width, rect->height, &text->bgColor);
            ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, destbga);
            ituDestroySurface(surf);
        }
#else
        ituColorFillBlend(dest, destx, desty, rect->width, rect->height, &text->bgColor, true, true, destbga);
#endif
    }

    ituTextRefreshSize(text, style);

    if (desta == 255)
    {
        int w = 0, h = 0, modY = 0;

        ituSetColor(&dest->fgColor, desta, widget->color.red, widget->color.green, widget->color.blue);

        if ((text->textFlags & ITU_TEXT_BRIEF_LONGWORD) && (widgetWidth > 0))
        {
            int currentW = 0, currentH = 0;
            ituFtGetTextDimension(string, &currentW, &currentH, widget, boldSize);
            if (widget->tree.parent)
            {
                ITUWidget * pObj = (ITUWidget *)widget->tree.parent;
                if ((strlen(pObj->name) == 0) && (pObj->rect.width < widgetWidth))
                {
                    widgetWidth = pObj->rect.width;
                }
            }
            if (currentW > widgetWidth)
            {
                int     k = 0;
                wchar_t wideStr[512] = L"";
                char    strLastSave[4096] = "";
                int     wideLen = 0, normalLen = strlen(string);
                int     wordSize[512] = { 0 };
                int     dotSizeW = 0, dotSizeH = 0;
                ituFtGetTextDimension("...", &dotSizeW, &dotSizeH, widget, boldSize);
                while (k < normalLen)
                {
                    wordSize[wideLen] = mbtowc((wideStr + wideLen), (string + k), 512);
                    k += wordSize[wideLen];
                    wideLen++;
                    if (k > 0)
                    {
                        char    strTmp[4096] = "";
                        strncpy(strTmp, string, k);
                        strncat(strTmp, "....", 3);
                        ituFtGetTextDimension(strTmp, &currentW, &currentH, widget, boldSize);
                        if (currentW > widgetWidth)
                        {
                            newString = (char*)calloc(1, sizeof(char) * 4096);
                            if (newString)
                            {
                                strncpy(newString, strLastSave, 4096);
                            }
                            break;
                        }
                        else
                        {
                            strncpy(strLastSave, strTmp, 4096);
                        }
                    }
                }
            }
        }

        if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
        {
            modY = ituFtGetTextDimension(newString, &w, &h, widget, boldSize);
        }
        else
        {
            modY = ituFtGetTextDimension(string, &w, &h, widget, boldSize);
        }

        h = h + modY;

        // resize requirement
        if (((w > widgetWidth) || (h > widgetHeight)) && rsBypassCheck)
        {
            if (w > widgetWidth)
            {
                realWidget->rect.width = realWidget->org_rect.width = w;
            }
            if (h > widgetHeight)
            {
                realWidget->rect.height = realWidget->org_rect.height = h;
            }

            if (realWidget != widget)
            {
                if (w > widgetWidth)
                {
                    realWidgetBW->rect.width = realWidgetBW->org_rect.width = w;
                }
                if (h > widgetHeight)
                {
                    realWidgetBW->rect.height = realWidgetBW->org_rect.height = h;
                }

                // reset bgOrg for button
                if (realWidget->type == ITU_BUTTON)
                {
                    ITUButton *     button = (ITUButton *)realWidget;
                    ITUBackground * bg     = &button->bg;
                    bg->orgWidth = bg->orgHeight = 0;
                }
            }
            widget->pObj = (ITCTree *)widget;

            if (rsBypassCheck)
            {
                if (realWidgetBW->type == ITU_BORDERWINDOW)
                {
                    rect = (ITURectangle*)&realWidgetBW->rect;
                    destx = x;
                    desty = y;
                }
                else
                {
                    destx = rect->x + x;
                    desty = rect->y + y;
                }
                widgetWidth = rect->width;
                widgetHeight = rect->height;
            }
        }

        if (text->layout == ITU_LAYOUT_TOP_CENTER)
        {
            destx += widgetWidth / 2 - w / 2;
        }
        else if (text->layout == ITU_LAYOUT_TOP_RIGHT)
        {
            destx += widgetWidth - w;
        }
        else if (text->layout == ITU_LAYOUT_MIDDLE_LEFT)
        {
            desty += widgetHeight / 2 - h / 2;
        }
        else if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
        {
            destx += widgetWidth / 2 - w / 2;
            desty += widgetHeight / 2 - h / 2;
        }
        else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
        {
            destx += widgetWidth - w;
            desty += widgetHeight / 2 - h / 2;
        }
        else if (text->layout == ITU_LAYOUT_BOTTOM_LEFT)
        {
            desty += widgetHeight - h;
        }
        else if (text->layout == ITU_LAYOUT_BOTTOM_CENTER)
        {
            destx += widgetWidth / 2 - w / 2;
            desty += widgetHeight - h;
        }
        else if (text->layout == ITU_LAYOUT_BOTTOM_RIGHT)
        {
            destx += widgetWidth - w;
            desty += widgetHeight - h;
        }

        if (text->letterSpacing != 0)
        {
            char * pch = (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD)) ? (newString) : (string);
            int    xx  = 0;
            while (*pch != '\0')
            {
                int size = ituFtGetCharWidth(pch, &w, boldSize);
                if (size == 0)
                {
                    break;
                }

                ituFtDrawChar(dest, destx + xx, desty, pch);

                xx += w + text->letterSpacing;
                pch += size;
            }
            // if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
            //	free(newString);
        }
        else
        {
            if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
            {
                ituFtDrawText(dest, destx, desty, newString);
                // free(newString);
            }
            else
            {
                ituFtDrawText(dest, destx, desty, string);
            }
        }
    }
    else if (desta > 0)
    {
        int          w = 0, h = 0;
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
        if (surf)
        {
            int modY = 0, dx = 0, dy = 0;

            ituColorFill(surf, 0, 0, surf->width, surf->height, &text->bgColor);
            ituBitBlt(surf, 0, 0, surf->width, surf->height, dest, destx, desty);
            ituSetColor(&surf->fgColor, 255, widget->color.red, widget->color.green, widget->color.blue);

            if ((text->textFlags & ITU_TEXT_BRIEF_LONGWORD) && (widgetWidth > 0))
            {
                int currentW = 0, currentH = 0;
                ituFtGetTextDimension(string, &currentW, &currentH, widget, boldSize);
                if (widget->tree.parent)
                {
                    ITUWidget * pObj = (ITUWidget *)widget->tree.parent;
                    if ((strlen(pObj->name) == 0) && (pObj->rect.width < widgetWidth))
                    {
                        widgetWidth = pObj->rect.width;
                    }
                }
                if (currentW > widgetWidth)
                {
                    int     k = 0;
                    wchar_t wideStr[512] = L"";
                    char    strLastSave[4096] = "";
                    int     wideLen = 0, normalLen = strlen(string);
                    int     wordSize[512] = { 0 };
                    int     dotSizeW = 0, dotSizeH = 0;
                    ituFtGetTextDimension("...", &dotSizeW, &dotSizeH, widget, boldSize);
                    while (k < normalLen)
                    {
                        wordSize[wideLen] = mbtowc((wideStr + wideLen), (string + k), 512);
                        k += wordSize[wideLen];
                        wideLen++;
                        if (k > 0)
                        {
                            char    strTmp[4096] = "";
                            strncpy(strTmp, string, k);
                            strncat(strTmp, "....", 3);
                            ituFtGetTextDimension(strTmp, &currentW, &currentH, widget, boldSize);
                            if (currentW > widgetWidth)
                            {
                                newString = (char*)calloc(1, sizeof(char) * 4096);
                                if (newString)
                                {
                                    strncpy(newString, strLastSave, 4096);
                                }
                                break;
                            }
                            else
                            {
                                strncpy(strLastSave, strTmp, 4096);
                            }
                        }
                    }
                }
            }

            if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
            {
                modY = ituFtGetTextDimension(newString, &w, &h, widget, boldSize);
            }
            else
            {
                modY = ituFtGetTextDimension(string, &w, &h, widget, boldSize);
            }

            h = h + modY;

            // resize requirement
            if ((w > widgetWidth) || (h > widgetHeight))
            {
                if (w > widgetWidth)
                {
                    realWidget->rect.width = realWidget->org_rect.width = w;
                }
                if (h > widgetHeight)
                {
                    realWidget->rect.height = realWidget->org_rect.height = h;
                }

                if (realWidget != widget)
                {
                    if (w > widgetWidth)
                    {
                        realWidgetBW->rect.width = realWidgetBW->org_rect.width = w;
                    }
                    if (h > widgetHeight)
                    {
                        realWidgetBW->rect.height = realWidgetBW->org_rect.height = h;
                    }

                    // reset bgOrg for button
                    if (realWidget->type == ITU_BUTTON)
                    {
                        ITUButton *     button = (ITUButton *)realWidget;
                        ITUBackground * bg     = &button->bg;
                        bg->orgWidth = bg->orgHeight = 0;
                    }
                }
                widget->pObj = (ITCTree *)widget;
            }

            if (text->layout == ITU_LAYOUT_TOP_CENTER)
            {
                dx += rect->width / 2 - w / 2;
            }
            else if (text->layout == ITU_LAYOUT_TOP_RIGHT)
            {
                dx += rect->width - w;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_LEFT)
            {
                dy += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
            {
                dx += rect->width / 2 - w / 2;
                dy += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
            {
                dx += rect->width - w;
                dy += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_LEFT)
            {
                dy += rect->height - h;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_CENTER)
            {
                dx += rect->width / 2 - w / 2;
                dy += rect->height - h;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_RIGHT)
            {
                dx += rect->width - w;
                dy += rect->height - h;
            }

            // dx = (dx >= 0) ? (dx) : (0);
            // dy = (dy >= 0) ? (dy) : (0);

            if (text->letterSpacing != 0)
            {
                char * pch = (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD)) ? (newString) : (string);
                int    xx  = 0;
                while (*pch != '\0')
                {
                    int size = ituFtGetCharWidth(pch, &w, boldSize);
                    if (size == 0)
                    {
                        break;
                    }

                    ituFtDrawChar(surf, dx + xx, dy, pch);

                    xx += w + text->letterSpacing;
                    pch += size;
                }
                // if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
                //	free(newString);
            }
            else
            {
                if (newString && (text->textFlags & ITU_TEXT_BRIEF_LONGWORD))
                {
                    ituFtDrawText(surf, dx, dy, newString);
                    // free(newString);
                }
                else
                {
                    ituFtDrawText(surf, dx, dy, string);
                }
            }
            ituAlphaBlend(dest, destx, desty, surf->width, surf->height, surf, 0, 0, desta);
            ituDestroySurface(surf);
        }
    }

    if (newString)
    {
        free(newString);
    }

    if ((widget->flags & ITU_CLIP_DISABLED) == 0)
    {
        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
    }

    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}

void ituTextOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    ITUText * text = (ITUText *)widget;
    assert(widget);

    switch (action)
    {
        case ITU_ACTION_INPUT:
            ituTextSetString(text, param);
            break;

        case ITU_ACTION_LANGUAGE:
            ituTextUpdate(widget, ITU_EVENT_LANGUAGE, atoi(param), 0, 0);
            break;

        default:
            ituWidgetOnActionImpl(widget, action, param);
            break;
    }
}

void ituTextInit (ITUText * text)
{
    assert(text);
    ITU_ASSERT_THREAD();

    (void)memset(text, 0, sizeof(ITUText));

    ituWidgetInit(&text->widget);

    ituWidgetSetType(text, ITU_TEXT);
    ituWidgetSetName(text, textName);
    ituWidgetSetExit(text, ituTextExit);
    ituWidgetSetClone(text, ituTextClone);
    ituWidgetSetUpdate(text, ituTextUpdate);
    ituWidgetSetDraw(text, ituTextDraw);
    ituWidgetSetOnAction(text, ituTextOnAction);

    ituSetColor(&text->widget.color, 255, 255, 255, 255);
}

void ituTextLoad (ITUText * text, uint32_t base)
{
    assert(text);

    ituWidgetLoad((ITUWidget *)text, base);

    ituWidgetSetExit(text, ituTextExit);
    ituWidgetSetClone(text, ituTextClone);
    ituWidgetSetUpdate(text, ituTextUpdate);
    ituWidgetSetDraw(text, ituTextDraw);
    ituWidgetSetOnAction(text, ituTextOnAction);

    if (text->stringSet)
    {
        text->stringSet = (ITUStringSet *)(base + (uint32_t)text->stringSet);
    }
}

void ituTextSetFontWidth (ITUText * text, int height)
{
    assert(text);
    ITU_ASSERT_THREAD();
    text->fontWidth    = height;
    text->widget.dirty = true;
}

void ituTextSetFontHeight (ITUText * text, int height)
{
    assert(text);
    ITU_ASSERT_THREAD();
    text->fontHeight   = height;
    text->widget.dirty = true;
}

void ituTextSetFontSize (ITUText * text, int size)
{
    assert(text);
    ITU_ASSERT_THREAD();
    text->fontWidth    = size;
    text->fontHeight   = size;
    text->widget.dirty = true;
}

void ituTextSetLanguage (ITUText * text, int lang)
{
    assert(text);
    ITU_ASSERT_THREAD();

    if (text->stringSet == NULL)
    {
        return;
    }

    if (lang >= text->stringSet->count)
    {
        lang = 0;
    }

    if (text->lang != lang)
    {
        text->lang = lang;
        ituTextProcLangTableToString(text);
    }

    if (text->string == NULL)
    {
        text->widget.dirty = true;
    }
}

void ituTextSetStringImpl (ITUText * text, char * string)
{
    assert(text);
    ITU_ASSERT_THREAD();

    if (text->string != NULL)
    {
        free(text->string);
        text->string = NULL;
    }

    if (string != NULL)
    {
        // when got different char dec value or length
        // check your locale setting --> utf/en only
        text->string = strdup(string);
    }

    ituTextRefreshSize(text, ITU_FT_STYLE_DEFAULT);

    text->widget.dirty = true;
}

char * ituTextGetStringImpl (ITUText * text)
{
    assert(text);
    ITU_ASSERT_THREAD();

    if (text->string != NULL)
    {
        return text->string;
    }
    else if (text->stringSet != NULL)
    {
        if (text->tableString != NULL)
        {
            return text->tableString;
        }
        else
        {
            return text->stringSet->strings[text->lang];
        }
    }
    else
    {
        return NULL;
    }
}

void ituTextSetBackColor (ITUText * text, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    assert(text);
    ITU_ASSERT_THREAD();

    text->bgColor.alpha = alpha;
    text->bgColor.red   = red;
    text->bgColor.green = green;
    text->bgColor.blue  = blue;
    text->widget.dirty  = true;
}

void ituTextSetBriefLongWord (ITUText * text, bool OnOff)
{
    if (text)
    {
        if (OnOff)
        {
            text->textFlags |= ITU_TEXT_BRIEF_LONGWORD;
        }
        else
        {
            text->textFlags &= ~ITU_TEXT_BRIEF_LONGWORD;
        }
    }
}

bool ituTextProcLangTableToString (ITUText * text)
{
    if (text != NULL)
    {
        char * strIndex = text->stringSet->strings[text->lang];

        if (text->useLangTableFile == 0)
        {
            return false;
        }

        if (text->tableString == NULL)
        {
            text->tableString = (char *)malloc(sizeof(char) * ITU_LANGUAGE_TABLE_WORDLEN);
            if (text->tableString == NULL)
            {
                return false;
            }
            else
            {
                (void)memset(text->tableString, '\0', sizeof(char) * ITU_LANGUAGE_TABLE_WORDLEN);
            }
        }

        if (strlen(strIndex) > 0)
        {
            int idx = atoi(strIndex);
            if ((idx >= 0) && (idx < ITU_LANGUAGE_STRING_ISIZE))
            {
                const wchar_t * wcString = ITULanguage_String_Table[text->lang][idx];
                int             i = 0, j = 0, wlen = wcslen(wcString);
                for (; i < wlen; i++)
                {
                    int xxx = wcString[i];

                    if (xxx <= 0xFF)
                    {
                        text->tableString[j] = wcString[i];
                        j++;
                    }
                    else
                    {
                        wctomb(&text->tableString[j], wcString[i]);
                        j += ITU_TEXT_LATS_BIT;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void ituTextSetWideString (ITUText * txt, const wchar_t * wcString)
{
    const wchar_t * wcsIndirectString = &wcString[0];
    int             i = 0, j = 0, wlen = wcslen(wcsIndirectString);
    char            mbString[512] = {'\0'};
    // debug for local test below
    // size_t          countConverted;
    // mbstate_t       mbstate;
    // (void)memset((void*)&mbstate, 0, sizeof(mbstate));
    // countConverted = wcsrtombs(mbString, &wcsIndirectString, 512, &mbstate);

    for (; i < wlen; i++)
    {
        int xxx = wcString[i];

        if (xxx <= 0xFF)
        {
            mbString[j] = wcString[i];
            j++;
        }
        // else if ((xxx >= 0x3040) && (xxx <= 0x309F))
        //{
        //     int k = 0;
        //     for (k = i; k < wlen; k++)
        //     {
        //         (void)printf("[%d] - %x\n", k, wcString[k]);
        //     }
        //     wctomb(&mbString[j], wcString[i]);
        //     //mbString[j] = wcString[i];
        //     j += 3;
        // }
        else if ((xxx >= 0x0600) && (xxx <= 0x06FF)) //for arabic
        {
            wctomb(&mbString[j], wcString[i]);
            while (mbString[j] != 0)
            {
                j++;
            }
        }
        else
        {
            wctomb(&mbString[j], wcString[i]);
            j += ITU_TEXT_LATS_BIT;
        }
    }

    ituTextSetString(txt, &mbString[0]);
}
