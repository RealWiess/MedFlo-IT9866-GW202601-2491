#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#pragma warning(disable : 4996)
static const char textboxName[] = "ITUTextBox";

bool              ituTextBoxClone (ITUWidget * widget, ITUWidget ** cloned)
{
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUTextBox));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUTextBox));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }
    return ituTextClone(widget, cloned);
}

void ituTextBoxRefreshSize (ITUTextBox * textbox, unsigned int style)
{
    ITUText * text = &textbox->text;
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

char * ituTextBoxGetSpaceWrap (ITUWidget * widget, char * buf, int * sepCount)
{
    char * targetChkStr = " ";

    int    strLength    = strlen(buf);
    int    widthCheck   = 0;
    int    heightCheck  = 0;
    char * strTmp       = NULL;
    char * bufDump      = strdup(buf);
    char * saveptrDump;

    char * tokenWord    = NULL;
    char * strLast      = NULL;

    bool   bWordOutSide = false;
    int    boldSize     = 0;
    if (widget == NULL)
    {
        return NULL;
    }
    else
    {
        ITUText* txt = (ITUText*)widget;
        if (txt->textFlags & ITU_TEXT_BOLD)
        {
            boldSize = txt->boldSize;
        }
    }

    if (strLength <= 0)
    {
        *sepCount = -1;
        free(bufDump);
        return NULL;
    }
    else
    {
        ituFtGetTextDimension(buf, &widthCheck, &heightCheck, widget, boldSize);
        if (widthCheck <= widget->rect.width)
        {
            *sepCount = -1;
            free(bufDump);
            return strdup(buf);
        }
    }
    strTmp = malloc(sizeof(char) * strLength);
    (void)memset(strTmp, '\0', sizeof(char) * strLength);
    tokenWord = strtok_r(bufDump, targetChkStr, &saveptrDump);

    if (tokenWord)
    {
        ituFtGetTextDimension(tokenWord, &widthCheck, &heightCheck, widget, boldSize);
        if (widthCheck > widget->rect.width)
        {
            ((ITUTextBox *)widget)->textboxFlags |= ITU_TEXTBOX_EN_SPACEWRAP_LONGF;
            *sepCount = -1;
            strLast = strdup(tokenWord);
            free(strTmp);
            free(bufDump);
            return strLast;
        }
    }

    do
    {
        if (!tokenWord)
        {
            break;
        }

        widthCheck = 0;
        if (strlen(tokenWord) > 0)
        {
            if (strlen(strTmp) == 0) // strtok first time without targetChkStr(" ")
            {
                free(strLast);
                strLast = strdup(strTmp);
                strlcat(strTmp, tokenWord, strLength);
            }
            else
            {
                free(strLast);
                strLast = strdup(strTmp);
                strlcat(strTmp, targetChkStr, strLength);
                strlcat(strTmp, tokenWord, strLength);
            }

            ituFtGetTextDimension(strTmp, &widthCheck, &heightCheck, widget, boldSize);
            if (widthCheck > widget->rect.width)
            {
                *sepCount -= 1;
                bWordOutSide = true;
                break;
            }

            tokenWord = strtok_r(NULL, targetChkStr, &saveptrDump);
            if (tokenWord)
            {
                if (strlen(saveptrDump) == 0) // string end
                {
                    // end to end under debug

                    // modify here to check the last word too long again
                    int lastW = 0, lastH = 0;
                    ituFtGetTextDimension(tokenWord, &lastW, &lastH, widget, boldSize);
                    if ((widthCheck + lastW) > widget->rect.width)
                    {
                        ((ITUTextBox *)widget)->textboxFlags |= ITU_TEXTBOX_EN_SPACEWRAP_LONGF;
                        free(strLast);
                        strLast = strdup(strTmp);
                        free(strTmp);
                        free(bufDump);
                        return strLast;
                    }

                    *sepCount = -1;
                    free(strTmp);
                    free(bufDump);
                    return strLast;
                }
                else
                {
                    *sepCount += 1;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    } while (widthCheck <= widget->rect.width);

    if (bWordOutSide)
    {
        free(strTmp);
        free(bufDump);
        return strLast;
    }
    else
    {
        free(strTmp);
        free(strLast);
        free(bufDump);
        *sepCount = -1;
        return NULL;
    }
}

void ituTextBoxSetString (ITUTextBox * textbox, char * string)
{
    ITU_ASSERT_THREAD();

    ituTextSetString(textbox, NULL);
    textbox->cursorIndex = 0;
    if (string)
    {
        ituTextBoxInput(textbox, string);
    }
    else
    {
        ituCopyColor(&textbox->text.widget.color, &textbox->defColor);
    }
}

void ituTextBoxInput (ITUTextBox * textbox, char * input)
{
    ITUText * text = &textbox->text;
    char *    ptr, *ptr0;
    int       i, len;
    // maxLen up to 1024 when use ituTextBoxSetString under multiline mode
    int       maxLen = (textbox->textboxFlags & ITU_TEXTBOX_MULTILINE) ? (0x400) : (textbox->maxLen);
    char      c, *suffix = NULL;
    assert(textbox);
    ITU_ASSERT_THREAD();

    if (maxLen <= 0 || !input)
    {
        return;
    }

    if (*input == '\b') // delete
    {
        ituTextBoxBack(textbox);
        return;
    }

    len = strlen(input);

    if (len > 0)
    {
        if (text->string == NULL)
        {
            text->string = malloc(maxLen + 1);
            if (text->string == NULL)
            {
                return;
            }

            text->string[0]      = '\0';
            textbox->cursorIndex = 0;

            ituCopyColor(&text->widget.color, &textbox->fgColor);
        }

        if (text->string[textbox->cursorIndex] != '\0')
        {
            suffix = strdup(&text->string[textbox->cursorIndex]);
        }
    }

    ptr  = input;
    ptr0 = ptr;

    while (*ptr != '\0')
    {
        wchar_t buf;

        len = strlen(ptr);
        len = mbtowc(&buf, ptr, len);

        if (textbox->cursorIndex + len > maxLen)
        {
            break;
        }

        for (i = 0; i < len; i++)
        {
            c = ptr[i];

            if (c == '\r' || c == '\t' || c == '\177')
            {
                continue;
            }

            if (textbox->textboxFlags & ITU_TEXTBOX_UPPER)
            {
                text->string[textbox->cursorIndex] = (char)toupper(c);
            }
            else if (textbox->textboxFlags & ITU_TEXTBOX_LOWER)
            {
                text->string[textbox->cursorIndex] = (char)tolower(c);
            }
            else
            {
                text->string[textbox->cursorIndex] = (char)c;
            }
            textbox->cursorIndex++;
        }

        ptr0 = ptr;
        ptr += len;
    }

    if (suffix)
    {
        strncpy(&text->string[textbox->cursorIndex], suffix, maxLen - textbox->cursorIndex);
        free(suffix);
    }
    else if (text->string)
    {
        text->string[textbox->cursorIndex] = '\0';
    }
    else
    {
        /* do nothing */
    }

    ituWidgetSetDirty(textbox, true);
}

void ituTextBoxBack (ITUTextBox * textbox)
{
    ITUText * text = &textbox->text;
    char *    ptr, *ptr0;
    int       len = 0;
    assert(textbox);
    ITU_ASSERT_THREAD();

    if (!text->string || textbox->cursorIndex == 0)
    {
        return;
    }

    ptr  = text->string;
    ptr0 = ptr;

    while (*ptr != '\0')
    {
        wchar_t buf;

        len = strlen(ptr);
        len = mbtowc(&buf, ptr, len);

        if (ptr == &text->string[textbox->cursorIndex])
        {
            break;
        }

        ptr0 = ptr;
        ptr += len;
    }

    strcpy(ptr0, ptr);
    textbox->cursorIndex -= len;

    if (text->string[0] == '\0')
    {
        free(text->string);
        text->string = NULL;
        ituCopyColor(&text->widget.color, &textbox->defColor);
    }
}

char * ituTextBoxGetString (ITUTextBox * textbox)
{
    ITUText * text = &textbox->text;
    assert(text);
    ITU_ASSERT_THREAD();

    if (text->string)
    {
        return text->string;
    }
    else
    {
        return NULL;
    }
}

bool ituTextBoxUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool         result;
    ITUTextBox * textbox = (ITUTextBox *)widget;
    ITUText* txt = (ITUText*)widget;
    int boldSize = 0;
    assert(textbox);

    result = ituTextUpdate(widget, ev, arg1, arg2, arg3);

    if (txt->textFlags & ITU_TEXT_BOLD)
    {
        boldSize = txt->boldSize;
    }

    if (ev == ITU_EVENT_MOUSEDOWN)
    {
        if (ituWidgetIsEnabled(widget))
        {
            int x = arg2 - widget->rect.x;
            int y = arg3 - widget->rect.y;

            if (ituWidgetIsInside(widget, x, y))
            {
                ituFocusWidget(widget);

                if (textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
                {
                    ITUText * text   = &textbox->text;
                    char *    orgstr = text->string;
                    char *    buf    = NULL;

                    if (text->string)
                    {
                        int                  len  = strlen(text->string);
                        int                  xx   = 0;

                        const ITURectangle * rect = (ITURectangle *)&widget->rect;
                        int                  destx, desty, cursorIndex;
                        char *               pch, *saveptr;

                        buf = malloc(len + 2);
                        if (!buf)
                        {
                            return false;
                        }

                        destx = 0;
                        desty = 0;

                        strlcpy(buf, orgstr, len + 2);
                        pch         = strtok_r(buf, "\n", &saveptr);
                        cursorIndex = 0;

                        if (text->fontHeight > 0)
                        {
                            ituFtSetCurrentFont(text->fontIndex);
                            ituFtSetFontSize(text->fontWidth, text->fontHeight);
                        }

                        if (text->textFlags & ITU_TEXT_BOLD)
                        {
                            ituFtSetFontStyle(ITU_FT_STYLE_BOLD);
                            ituFtSetFontStyleValue(ITU_FT_STYLE_BOLD, text->boldSize);
                        }
                        else
                        {
                            ituFtSetFontStyle(ITU_FT_STYLE_DEFAULT);
                        }

                        while (pch != NULL)
                        {
                            int    w    = 0;
                            char * pch2 = pch;

                            xx          = 0;
                            while (*pch2 != '\0')
                            {
                                int size = ituFtGetCharWidth(pch2, &w, boldSize);

                                if (size == 0)
                                {
                                    break;
                                }

                                if (destx + xx < x && desty < y)
                                {
                                    textbox->cursorIndex = cursorIndex;
                                }

                                if (textbox->textboxFlags & ITU_TEXTBOX_WORDWRAP)
                                {
                                    if (xx + w >= rect->width)
                                    {
                                        xx = 0;
                                        desty += textbox->lineHeight;
                                    }
                                }
                                xx += w;
                                pch2 += size;
                                cursorIndex += size;
                            }

                            if (textbox->textboxFlags & ITU_TEXTBOX_MULTILINE)
                            {
                                pch = strtok_r(NULL, "\n", &saveptr);
                                if (pch)
                                {
                                    desty += textbox->lineHeight;
                                    cursorIndex++;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (destx + xx <= x && desty <= y)
                        {
                            textbox->cursorIndex = cursorIndex;
                        }
                    }
                    free(buf);
                    buf = NULL;
                }
                ituExecActions((ITUWidget *)textbox, textbox->actions, ev, 0);
            }
        }
    }
    else if (ev >= ITU_EVENT_CUSTOM || ev == ITU_EVENT_TIMER)
    {
        if (ituWidgetIsEnabled(widget))
        {
            ituExecActions(widget, textbox->actions, ev, arg1);
            result |= widget->dirty;
        }
    }
    else if (ituWidgetIsActive(widget) && ituWidgetIsEnabled(widget))
    {
        switch (ev)
        {
            case ITU_EVENT_KEYDOWN:
                if (isascii(arg1))
                {
                    char buf[2];

                    ituFocusWidget(widget);

                    buf[0] = (char)arg1;
                    buf[1] = '\0';
                    ituTextBoxInput(textbox, buf);
                    ituExecActions((ITUWidget *)textbox, textbox->actions, ev, arg1);
                }
                break;
        }
        result |= widget->dirty;
    }
    else
    {
        /* do nothing */
    }

    return widget->visible ? result : false;
}

void ituTextBoxDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    ITUTextBox *       textbox    = (ITUTextBox *)widget;
    ITUText *          text       = NULL;
    char *             orgstr     = NULL;
    char *             buf        = NULL;
    int                borderSize = 0;
    int                boldSize   = 0;
    ITUFtHarfbuzzStyle hbStyle    = {0, 0};
    unsigned int       style      = ITU_FT_STYLE_DEFAULT;
    assert(textbox);
    assert(dest);
    text       = &textbox->text;
    orgstr     = text->string;
    borderSize = text->boldSize;
#ifdef CFG_ITU_FT_CACHE_ENABLE
    hbStyle    = ituFtGetHarfbuzzStyle(text->fontIndex);
#endif
    ituFtSetFontPriority(text->fontPriority, 0);

    if (text)
    {
        if (text->string)
        {
            if (text->textFlags & ITU_TEXT_ARABIC)
            {
                style |= ITU_FT_STYLE_ARABIC;
            }
            else if (text->textFlags & ITU_TEXT_HEBREW)
            {
                style |= ITU_FT_STYLE_HEBREW;
            }
            else
            {
                /* do nothing */
            }
        }
        else if (text->stringSet)
        {
            if ((text->textFlags & ITU_TEXT_ARABIC) && ((1u << text->lang) & text->arabicIndices))
            {
                style |= ITU_FT_STYLE_ARABIC;
            }
            else if ((text->textFlags & ITU_TEXT_HEBREW) && ((1u << text->lang) & text->hebrewIndices))
            {
                style |= ITU_FT_STYLE_HEBREW;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            widget->dirty = false;
            return;
        }
    }

    if (text->textFlags & ITU_TEXT_BOLD)
    {
        boldSize = text->boldSize;
    }

    ituTextBoxRefreshSize(textbox, style);

    if ((text->string) && (!(textbox->textboxFlags & ITU_TEXTBOX_MULTILINE)))
    {
        int            len  = strlen(text->string);
        ITURectangle * rect = (ITURectangle *)&widget->rect;
        ITURectangle   prevClip;
        int            destx, desty;
        uint8_t        desta, destbga;

        destx   = rect->x + x;
        desty   = rect->y + y;
        desta   = alpha * widget->color.alpha / 255;
        desta   = desta * widget->alpha / 255;
        destbga = alpha * text->bgColor.alpha / 255;
        destbga = destbga * widget->alpha / 255;

        buf     = malloc(len + 2);
        if (!buf)
        {
            return;
        }

        if (textbox->textboxFlags & ITU_TEXTBOX_PASSWORD)
        {
            (void)memset(buf, '*', len);
            buf[len]     = '\0';
            text->string = buf;
        }
        else
        {
            strlcpy(buf, orgstr, len + 2);
        }

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
            ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
            if (surf)
            {
                ituColorFill(surf, 0, 0, rect->width, rect->height, &text->bgColor);
                ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, destbga);
                ituDestroySurface(surf);
            }
        }
        else
        {
            /* do nothing */
        }

        ituFtSetCurrentFont(text->fontIndex);

        if (desta == 255)
        {
            int w = 0, h = 0, index = 0, ww = 0;

            ituSetColor(&dest->fgColor, widget->color.alpha, widget->color.red, widget->color.green,
                        widget->color.blue);

            ituFtGetTextDimension(buf, &ww, NULL, widget, boldSize);
            if (ww < rect->width)
            {
                index = 0;
            }
            else
            {
                int    count = 0;
                char * pch   = &buf[len - 1];

                ww           = 0;

                while (pch != buf)
                {
                    int wLocal = 0;
                    int size   = ituFtGetCharWidth(pch, &wLocal, boldSize);
                    if (size == 0)
                    {
                        pch--;
                        continue;
                    }

                    if (ww + wLocal > rect->width)
                    {
                        break;
                    }

                    ww += wLocal + text->letterSpacing;
                    count += size;
                    pch--;
                }
                index = len - count;
            }

            if (text->layout == ITU_LAYOUT_TOP_CENTER)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width / 2 - w / 2;
            }
            else if (text->layout == ITU_LAYOUT_TOP_RIGHT)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width - w;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_LEFT)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                desty += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width / 2 - w / 2;
                desty += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width - w;
                desty += rect->height / 2 - h / 2;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_LEFT)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                desty += rect->height - h;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_CENTER)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width / 2 - w / 2;
                desty += rect->height - h;
            }
            else if (text->layout == ITU_LAYOUT_BOTTOM_RIGHT)
            {
                ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                destx += rect->width - w;
                desty += rect->height - h;
            }
            else
            {
                /* do nothing */
            }

            if (text->letterSpacing != 0)
            {
                char * pch = &buf[index];
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
            }
            else
            {
                ituFtDrawText(dest, destx + borderSize, desty, &buf[index]);
            }
        }
        else if (desta > 0)
        {
            ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
            if (surf)
            {
                int w = 0, h = 0, dx = 0, dy = 0;
                int index = 0, ww = 0;

                ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);
                ituSetColor(&surf->fgColor, widget->color.alpha, widget->color.red, widget->color.green,
                            widget->color.blue);

                ituFtGetTextDimension(buf, &ww, NULL, widget, boldSize);
                if (ww < rect->width)
                {
                    index = 0;
                }
                else
                {
                    int    count = 0;
                    char * pch   = &buf[len - 1];

                    ww           = 0;

                    while (pch != buf)
                    {
                        int wLocal = 0;
                        int size   = ituFtGetCharWidth(pch, &wLocal, boldSize);
                        if (size == 0)
                        {
                            pch--;
                            continue;
                        }

                        if (ww + wLocal > rect->width)
                        {
                            break;
                        }

                        ww += wLocal + text->letterSpacing;
                        count += size;
                        pch--;
                    }
                    index = len - count;
                }

                if (text->layout == ITU_LAYOUT_TOP_CENTER)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width / 2 - w / 2;
                }
                else if (text->layout == ITU_LAYOUT_TOP_RIGHT)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width - w;
                }
                else if (text->layout == ITU_LAYOUT_MIDDLE_LEFT)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dy += rect->height / 2 - h / 2;
                }
                else if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width / 2 - w / 2;
                    dy += rect->height / 2 - h / 2;
                }
                else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width - w;
                    dy += rect->height / 2 - h / 2;
                }
                else if (text->layout == ITU_LAYOUT_BOTTOM_LEFT)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dy += rect->height - h;
                }
                else if (text->layout == ITU_LAYOUT_BOTTOM_CENTER)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width / 2 - w / 2;
                    dy += rect->height - h;
                }
                else if (text->layout == ITU_LAYOUT_BOTTOM_RIGHT)
                {
                    ituFtGetTextDimension(&buf[index], &w, &h, widget, boldSize);
                    dx += rect->width - w;
                    dy += rect->height - h;
                }
                else
                {
                    /* do nothing */
                }

                if (text->letterSpacing != 0)
                {
                    char * pch    = &buf[index];
                    int    wLocal = 0, xx = 0;
                    while (*pch != '\0')
                    {
                        int size = ituFtGetCharWidth(pch, &wLocal, boldSize);
                        if (size == 0)
                        {
                            break;
                        }

                        ituFtDrawChar(dest, destx + xx, desty, pch);

                        xx += wLocal + text->letterSpacing;
                        pch += size;
                    }
                }
                else
                {
                    ituFtDrawText(surf, dx + borderSize, dy, &buf[index]);
                }
                if (destbga == 0) // destbga = 0 = BackAlpha zero
                {
                    ituBitBlt(dest, destx, desty, rect->width, rect->height, surf, 0, 0);
                }
                else
                {
                    ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                }
                ituDestroySurface(surf);
            }
        }
        else
        {
            /* do nothing */
        }

        if ((widget->flags & ITU_CLIP_DISABLED) == 0)
        {
            ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
        }

        if (textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
        {
            int    w = 0, xx = 0, cursorIndex = 0;
            char * pch2 = text->string;

            while (pch2 != NULL)
            {
                int size;
                if (textbox->cursorIndex == cursorIndex)
                {
                    //int destxL = widget->rect.x + x;
                    //int destyL = widget->rect.y + y;

                    if (xx < widget->rect.width)
                    {
                        ituColorFill(dest, destx + xx, desty, 1, text->fontHeight, &widget->color);
                    }

                    break;
                }

                size = ituFtGetCharWidth(pch2, &w, boldSize);

                if (size == 0)
                {
                    break;
                }

                xx += w + text->letterSpacing;
                pch2 += size;
                cursorIndex += size;
            }
        }
        text->string = orgstr;
        free(buf);
        buf = NULL;
    }
    else if (textbox->textboxFlags & ITU_TEXTBOX_MULTILINE)
    {
        ITURectangle * rect = (ITURectangle *)&widget->rect;
        int            len, destx, desty, cursorIndex;
        uint8_t        desta, destbga;
        ITURectangle   prevClip;
        char *         pch       = NULL;
        char *         saveptr   = NULL;
        bool           pch_alloc = false;
        int            wLocal = 0, hLocal = 0;

        // orgstr = text->string
        // someone use ituTextBoxSetString --> orgstr = text->string
        // or use the stringSet stored with xml/itu
        if (orgstr == NULL)
        {
            if (text->stringSet != NULL)
            {
                if (ituTextProcLangTableToString(text))
                {
                    orgstr = text->tableString;
                }
                else
                {
                    orgstr = text->stringSet->strings[text->lang];
                }
            }
            else
            {
                return;
            }

            if (orgstr == NULL)
            {
                return;
            }
        }

        len = strlen(orgstr);

        buf = malloc(len + 2);
        if (!buf)
        {
            return;
        }

        strlcpy(buf, orgstr, len + 2);
        ituFtGetTextDimension(buf, &wLocal, &hLocal, widget, boldSize);

        destx   = rect->x + x;
        desty   = rect->y + y;
        desta   = alpha * widget->color.alpha / 255;
        desta   = desta * widget->alpha / 255;
        destbga = alpha * text->bgColor.alpha / 255;
        destbga = destbga * widget->alpha / 255;

        ituWidgetSetClipping(widget, dest, x, y, &prevClip);

        if (!(textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP))
        {
            if (destbga == 255)
            {
                // try to fix error blt when textbox widget size too large
                int minx = (destx < 0) ? (0) : (destx);
                int miny = (desty < 0) ? (0) : (desty);
                int mdw  = ((minx + rect->width) > dest->width) ? (dest->width - minx) : (rect->width);
                int mdh  = ((miny + rect->height) > dest->height) ? (dest->height - miny) : (rect->height);
                ituColorFill(dest, minx, miny, mdw, mdh, &text->bgColor);
                // (void)printf("[%d]textbox colorfill on dest(%d/%d) from (%d/%d) to (%d/%d)\n", __LINE__, dest->width,
                // dest->height, minx, miny, minx + mdw, miny + mdh);
            }
            else if (destbga > 0)
            {
                // try to fix error blt when textbox widget size too large
                int          minx = (destx < 0) ? (0) : (destx);
                int          miny = (desty < 0) ? (0) : (desty);
                int          mdw  = ((minx + rect->width) > dest->width) ? (dest->width - minx) : (rect->width);
                int          mdh  = ((miny + rect->height) > dest->height) ? (dest->height - miny) : (rect->height);
                ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituColorFill(surf, 0, 0, rect->width, rect->height, &text->bgColor);
                    // (void)printf("[%d]textbox alphablend on dest(%d/%d) from (%d/%d) to (%d/%d)\n", __LINE__, dest->width,
                    // dest->height, minx, miny, minx + mdw, miny + mdh);
                    ituAlphaBlend(dest, minx, miny, mdw, mdh, surf, 0, 0, destbga);
                    ituDestroySurface(surf);
                }
            }
        }
        else
        {
            /* do nothing */
        }

        //strlcpy(buf, orgstr, len + 2);
        // pch = strtok_r(buf, "\r\n", &saveptr);
        cursorIndex = 0;

        // take space wrap first time
        if (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP)
        {
            int sepCount = 0;
            pch          = ituTextBoxGetSpaceWrap(widget, buf, &sepCount);
            if (pch)
            {
                pch_alloc = true;
            }
            // sepCount: how many times the line string need to be separated with space.
            if (sepCount > 0)
            {
                strtok_r(buf, " ", &saveptr);
                while (sepCount > 0)
                {
                    strtok_r(NULL, " ", &saveptr);
                    sepCount--;
                }

                // free(pch);
                // pch = NULL;
            }
            else
            {
                if (pch_alloc)
                {
                    free(pch);
                    pch_alloc = false;
                }
                pch = NULL;
                pch = strtok_r(buf, "\r\n", &saveptr);
            }
        }
        else
        {
            if (pch_alloc)
            {
                free(pch);
                pch_alloc = false;
            }
            pch = NULL;
            pch = strtok_r(buf, "\r\n", &saveptr);
        }

        if ((desta > 0) && (desta <= 255))
        {
            int            xx          = 0;
            int            widthCheck  = 0;
            int            heightCheck = 0;
            ITUPixelFormat format =
                (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP) ? (ITU_ARGB8888) : (dest->format);
            ITUSurface * surf = ((desta < 255) || (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP))
                                    ? (ituCreateSurface(rect->width, rect->height, 0, format, NULL, 0))
                                    : (NULL);
            int          yy   = (surf) ? (0) : (desty);

            if (surf)
            {
                ituBitBlt(surf, 0, 0, surf->width, surf->height, dest, destx, desty);
            }

            if (surf)
            {
                ituSetColor(&surf->fgColor, widget->color.alpha, widget->color.red, widget->color.green,
                            widget->color.blue);
            }
            else
            {
                ituSetColor(&dest->fgColor, widget->color.alpha, widget->color.red, widget->color.green,
                            widget->color.blue);
            }

            while (pch != NULL)
            {
                if (textbox->textboxFlags & ITU_TEXTBOX_WORDWRAP || textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
                {
                    int    size = 0, w = 0;
                    char * pch2 = pch;

                    xx          = 0;

                    if (text->layout == ITU_LAYOUT_MIDDLE_CENTER || text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                    {
                        char * pch3 = pch2;

                        while (*pch3 != '\0')
                        {
                            int size2 = ituFtGetCharWidth(pch3, &w, boldSize);
                            if (size2 == 0)
                            {
                                break;
                            }

                            if (textbox->textboxFlags & ITU_TEXTBOX_WORDWRAP)
                            {
                                if (xx + w >= rect->width)
                                {
                                    break;
                                }
                            }
                            xx += w + text->letterSpacing;
                            pch3 += size2;
                        }

                        if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
                        {
                            xx = rect->width / 2 - xx / 2;
                        }
                        else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                        {
                            xx = rect->width - xx - 1;
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }

                    while (*pch2 != '\0')
                    {
                        if (*pch2 == '\r')
                        {
                            pch2++;
                            continue;
                        }

                        // if (textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
                        //{
                        //     if (textbox->cursorIndex == cursorIndex)
                        //         ituColorFill(dest, destx + xx, desty, 1, text->fontHeight, &widget->color);
                        // }

                        size = ituFtGetCharWidth(pch2, &w, boldSize);

                        if (size == 0)
                        {
                            break;
                        }

                        if (textbox->textboxFlags & ITU_TEXTBOX_WORDWRAP)
                        {
                            if (xx + w >= rect->width)
                            {
                                // english wordwrap when single word length > widget width
                                if (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP_LONGF)
                                {
                                    int sepCount = 0;
                                    if (buf && pch2)
                                    {
                                        strcpy(buf, pch2);
                                    }

                                    if (pch_alloc)
                                    {
                                        free(pch);
                                        pch_alloc = false;
                                    }
                                    pch = NULL;
                                    pch = ituTextBoxGetSpaceWrap(widget, buf, &sepCount);
                                    if (pch)
                                    {
                                        pch_alloc = true;
                                    }
                                    // sepCount: how many times the line string need to be separated with space.
                                    if (sepCount > 0)
                                    {
                                        strtok_r(buf, " ", &saveptr);
                                        while (sepCount > 0)
                                        {
                                            strtok_r(NULL, " ", &saveptr);
                                            sepCount--;
                                        }

                                        // free(pch);
                                        // pch = NULL;
                                    }
                                    else
                                    {
                                        if (pch_alloc)
                                        {
                                            free(pch);
                                            pch_alloc = false;
                                        }
                                        pch = NULL;
                                        pch = strtok_r(buf, "\r\n", &saveptr);
                                    }

                                    // check pch string length here then compute the align position
                                    ituFtGetTextDimension(pch, &widthCheck, &heightCheck, widget, boldSize);
                                    if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
                                    {
                                        widthCheck = rect->width / 2 - widthCheck / 2;
                                    }
                                    else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                                    {
                                        widthCheck = rect->width - widthCheck - 1;
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }

                                    textbox->textboxFlags &= ~ITU_TEXTBOX_EN_SPACEWRAP_LONGF;
                                    yy += textbox->lineHeight;
                                    if (surf)
                                    {
                                        ituFtDrawText(surf, borderSize + widthCheck, yy, pch);
                                    }
                                    else
                                    {
                                        ituFtDrawText(dest, destx + borderSize + widthCheck, yy, pch);
                                    }
                                    break;
                                }

                                xx = 0;
                                if (text->layout == ITU_LAYOUT_MIDDLE_CENTER || text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                                {
                                    char * pch3 = pch2;

                                    while (*pch3 != '\0')
                                    {
                                        int w2    = 0;
                                        int size2 = ituFtGetCharWidth(pch3, &w2, boldSize);
                                        if (size2 == 0)
                                        {
                                            break;
                                        }

                                        if (textbox->textboxFlags & ITU_TEXTBOX_WORDWRAP)
                                        {
                                            if (xx + w2 >= rect->width)
                                            {
                                                break;
                                            }
                                        }
                                        xx += w2 + text->letterSpacing;
                                        pch3 += size2;
                                    }

                                    if (text->layout == ITU_LAYOUT_MIDDLE_CENTER)
                                    {
                                        xx = rect->width / 2 - xx / 2;
                                    }
                                    else if (text->layout == ITU_LAYOUT_MIDDLE_RIGHT)
                                    {
                                        if (hbStyle.dir == 1)
                                        {
                                            xx = 0;
                                        }
                                        else
                                        {
                                            xx = rect->width - xx - 1;
                                        }
                                    }
                                    else
                                    {
                                        /* do nothing */
                                    }
                                }
                                yy += textbox->lineHeight;
                            }
                        }
                        if (surf)
                        {
                            ituFtDrawChar(surf, borderSize + xx, yy, pch2);
                        }
                        else
                        {
                            if (hbStyle.dir == 1)
                            {
                                int rDestx = destx + rect->width - borderSize;
                                rDestx -= xx + w;
                                ituFtDrawChar(dest, rDestx, yy, pch2);
                            }
                            else
                            {
                                ituFtDrawChar(dest, destx + borderSize + xx, yy, pch2);
                            }
                        }
                        xx += w + text->letterSpacing;
                        pch2 += size;
                        cursorIndex += size;
                    }
                }
                else
                {
                    if (text->letterSpacing != 0)
                    {
                        int wLocal = 0, xLocal = 0;
                        while (*pch != '\0')
                        {
                            int size = ituFtGetCharWidth(pch, &wLocal, boldSize);
                            if (size == 0)
                            {
                                break;
                            }

                            ituFtDrawChar(dest, destx + xLocal, desty + yy, pch);

                            xLocal += wLocal + text->letterSpacing;
                            pch += size;
                        }
                    }
                    else
                    {
                        if (surf)
                        {
                            ituFtDrawText(surf, borderSize, yy, pch);
                        }
                        else
                        {
                            ituFtDrawText(dest, destx + borderSize, yy, pch);
                        }
                    }
                }

                // the last check line string here and get pch of next line to next while loop
                if (saveptr && (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP))
                {
                    int widthChecks  = 0;
                    int heightChecks = 0;
                    // check pch string length here then compute the align position
                    ituFtGetTextDimension(saveptr, &widthChecks, &heightChecks, widget, boldSize);
                    if (widthChecks > widget->rect.width)
                    {
                        int    sepCount = 0;
                        char * strTmp   = strdup(saveptr);

                        // reset buf to store the last strtok saveptr
                        free(buf);

                        buf = malloc(sizeof(char) * strlen(orgstr));
                        if (buf && strTmp)
                        {
                            strcpy(buf, strTmp);
                            if (pch_alloc)
                            {
                                free(pch);
                                pch_alloc = false;
                            }
                            pch = NULL;
                            pch = ituTextBoxGetSpaceWrap(widget, buf, &sepCount);
                            if (pch)
                            {
                                pch_alloc = true;
                            }
                            strtok_r(buf, " ", &saveptr);
                        }
                        free(strTmp);

                        // sepCount: how many times the line string need to be separated with space.
                        if (sepCount > 0)
                        {
                            while (sepCount > 0)
                            {
                                strtok_r(NULL, " ", &saveptr);
                                sepCount--;
                            }
                        }

                        // if (pch_alloc)//bug mark for trac2745
                        //{
                        //  free(pch);
                        //  pch_alloc = false;
                        // }
                        // pch = NULL;
                    }
                    else
                    {
                        if (pch_alloc)
                        {
                            free(pch);
                            pch_alloc = false;
                        }
                        pch = NULL;
                        pch = strtok_r(NULL, "\r\n", &saveptr);
                    }
                }
                else
                {
                    if (pch_alloc)
                    {
                        free(pch);
                        pch_alloc = false;
                    }
                    pch = NULL;
                    pch = strtok_r(NULL, "\r\n", &saveptr);
                }

                if (pch)
                {
                    yy += textbox->lineHeight;
                    cursorIndex++;
                }
                else if ((textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP) && (heightCheck > 0))
                {
                    int newRectHeight = (surf) ? (yy + heightCheck) : (yy + heightCheck - desty);
                    ituWidgetSetHeight(widget, newRectHeight);
                }
                else
                {
                    /* do nothing */
                }
            }

            // used when desta not 255
            if (surf)
            {
                int vdx  = (destx < 0) ? (0) : (destx);
                int vdy  = (desty < 0) ? (0) : (desty);
                int modx = vdx - destx;
                int mody = vdy - desty;
                int rex  = ((destx + rect->width) > dest->width) ? (dest->width) : (destx + rect->width);
                int rey  = ((desty + rect->height) > dest->height) ? (dest->height) : (desty + rect->height);
                int rew  = rex - vdx;
                int reh  = rey - vdy;
                // (void)printf("[%d]alphablend surf(%d/%d) (%d,%d)/(%d,%d) to dest\n", __LINE__, surf->width, surf->height,
                // 0, 0, rect->width, rect->height); ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf,
                // 0, 0, desta);

                // (void)printf("[%d]textbox alphablend surf(%d/%d) (%d,%d)/(%d,%d) to dest(%d/%d) desta %d\n", __LINE__,
                // surf->width, surf->height, modx, mody, rew, reh, vdx, vdy, desta);

                if (textbox->textboxFlags & ITU_TEXTBOX_EN_SPACEWRAP)
                {
                    if (destbga == 255)
                    {
                        // try to fix error blt when textbox widget size too large
                        int minx = (destx < 0) ? (0) : (destx);
                        int miny = (desty < 0) ? (0) : (desty);
                        int mdw  = ((minx + rect->width) > dest->width) ? (dest->width - minx) : (rect->width);
                        int mdh  = ((miny + rect->height) > dest->height) ? (dest->height - miny) : (rect->height);
                        ituColorFill(dest, minx, miny, mdw, mdh, &text->bgColor);
                        // (void)printf("[%d]textbox colorfill on dest(%d/%d) from (%d/%d) to (%d/%d)\n", __LINE__,
                        // dest->width, dest->height, minx, miny, minx + mdw, miny + mdh);
                    }
                    else if (destbga > 0)
                    {
                        // try to fix error blt when textbox widget size too large
                        int minx = (destx < 0) ? (0) : (destx);
                        int miny = (desty < 0) ? (0) : (desty);
                        int mdw  = ((minx + rect->width) > dest->width) ? (dest->width - minx) : (rect->width);
                        int mdh  = ((miny + rect->height) > dest->height) ? (dest->height - miny) : (rect->height);
                        ITUSurface * surfTmp = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                        if (surfTmp)
                        {
                            ituColorFill(surfTmp, 0, 0, rect->width, rect->height, &text->bgColor);
                            // (void)printf("[%d]textbox alphablend on dest(%d/%d) from (%d/%d) to (%d/%d)\n", __LINE__,
                            // dest->width, dest->height, minx, miny, minx + mdw, miny + mdh);
                            ituAlphaBlend(dest, minx, miny, mdw, mdh, surfTmp, 0, 0, destbga);
                            ituDestroySurface(surfTmp);
                        }
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

                if ((desta == 255) || (destbga == 0)) // destbga = 0 = BackAlpha zero
                {
                    ituBitBlt(dest, vdx, vdy, rew, reh, surf, modx, mody);
                }
                else
                {
                    ituAlphaBlend(dest, vdx, vdy, rew, reh, surf, modx, mody, desta);
                }
            }

            if (textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
            {
                if (!surf)
                {
                    yy -= desty;
                }

                if (textbox->cursorIndex == cursorIndex)
                {
                    ituColorFill(dest, destx + xx, desty + yy, 1, text->fontHeight, &widget->color);
                }
            }

            if (surf)
            {
                ituDestroySurface(surf);
            }
        }

        ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
        // ituWidgetDrawImpl(widget, dest, x, y, alpha);
        free(buf);
        buf = NULL;
        if (pch_alloc)
        {
            free(pch);
            pch_alloc = false;
        }
        pch = NULL;
    }
    else if (textbox->textboxFlags & ITU_TEXTBOX_CURSOR)
    {
        const ITURectangle * rect = (ITURectangle *)&widget->rect;
        int                  destx, desty;

        destx = rect->x + x;
        desty = rect->y + y;

        if (textbox->cursorIndex == 0)
        {
            ituColorFill(dest, destx, desty, 1, text->fontHeight, &widget->color);
        }
    }
    else
    {
        ituTextDraw(widget, dest, x + borderSize, y, alpha);
    }

    if (ituWidgetIsActive(widget))
    {
        int            destx, desty;
        uint8_t        desta;
        ITURectangle * rect = (ITURectangle *)&widget->rect;

        destx               = rect->x + x;
        desty               = rect->y + y;
        desta               = alpha * textbox->focusColor.alpha / 255;

        if (desta == 255)
        {
            ituColorFill(dest, destx, desty, rect->width, borderSize, &textbox->focusColor);
            ituColorFill(dest, destx, desty + rect->height - borderSize, rect->width, borderSize, &textbox->focusColor);
            ituColorFill(dest, destx, desty + borderSize, borderSize, rect->height - borderSize * 2,
                         &textbox->focusColor);
            ituColorFill(dest, destx + rect->width - borderSize, desty + borderSize, borderSize,
                         rect->height - borderSize * 2, &textbox->focusColor);
        }
        else if (desta > 0)
        {
            ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, ITU_ARGB4444, NULL, 0);
            if (surf)
            {
                ITUColor black = {0, 0, 0, 0};
                ituColorFill(surf, 0, 0, rect->width, rect->height, &black);
                ituColorFill(surf, 0, 0, rect->width, borderSize, &widget->color);
                ituColorFill(surf, 0, rect->height - borderSize, rect->width, borderSize, &widget->color);
                ituColorFill(surf, 0, borderSize, borderSize, rect->height - borderSize * 2, &widget->color);
                ituColorFill(surf, rect->width - borderSize, borderSize, borderSize, rect->height - borderSize * 2,
                             &widget->color);
                ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                ituDestroySurface(surf);
            }
        }
        else
        {
            /* do nothing */
        }
    }
}

void ituTextBoxOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    ITUTextBox * textbox = (ITUTextBox *)widget;
    assert(widget);

    switch (action)
    {
        case ITU_ACTION_INPUT:
            ituTextBoxInput(textbox, param);
            break;

        case ITU_ACTION_BACK:
            ituTextBoxInput(textbox, "\b");
            break;

        case ITU_ACTION_CLEAR:
            ituTextBoxSetString(textbox, NULL);
            break;

        case ITU_ACTION_DODELAY0:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY0, atoi(param));
            break;

        case ITU_ACTION_DODELAY1:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY1, atoi(param));
            break;

        case ITU_ACTION_DODELAY2:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY2, atoi(param));
            break;

        case ITU_ACTION_DODELAY3:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY3, atoi(param));
            break;

        case ITU_ACTION_DODELAY4:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY4, atoi(param));
            break;

        case ITU_ACTION_DODELAY5:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY5, atoi(param));
            break;

        case ITU_ACTION_DODELAY6:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY6, atoi(param));
            break;

        case ITU_ACTION_DODELAY7:
            ituExecActions(widget, textbox->actions, ITU_EVENT_DELAY7, atoi(param));
            break;

        default:
            ituTextOnAction(widget, action, param);
            break;
    }
}

void ituTextBoxInit (ITUTextBox * textbox)
{
    assert(textbox);
    ITU_ASSERT_THREAD();

    (void)memset(textbox, 0, sizeof(ITUTextBox));

    ituTextInit(&textbox->text);

    ituWidgetSetType(textbox, ITU_TEXTBOX);
    ituWidgetSetName(textbox, textboxName);
    ituWidgetSetClone(textbox, ituTextBoxClone);
    ituWidgetSetUpdate(textbox, ituTextBoxUpdate);
    ituWidgetSetDraw(textbox, ituTextBoxDraw);
    ituWidgetSetOnAction(textbox, ituTextBoxOnAction);

    ituSetColor(&textbox->text.widget.color, 255, 255, 255, 255);
}

void ituTextBoxLoad (ITUTextBox * textbox, uint32_t base)
{
    assert(textbox);

    ituTextLoad(&textbox->text, base);

    ituWidgetSetClone(textbox, ituTextBoxClone);
    ituWidgetSetUpdate(textbox, ituTextBoxUpdate);
    ituWidgetSetDraw(textbox, ituTextBoxDraw);
    ituWidgetSetOnAction(textbox, ituTextBoxOnAction);
}

void ituTextBoxSetEngWordWrap (ITUTextBox * textbox, bool OnOff)
{
    if (textbox)
    {
        if (OnOff)
        {
            textbox->textboxFlags |= ITU_TEXTBOX_EN_SPACEWRAP;
        }
        else
        {
            textbox->textboxFlags &= ~ITU_TEXTBOX_EN_SPACEWRAP;
        }
    }
}

int ituTextBoxGetRowsCount(ITUTextBox* textbox)
{
    if (textbox != NULL)
    {
        ITUText* text = &textbox->text;

        int lineCount = 1;
        bool foundCheck = true;
        char* strPoint = text->stringSet->strings[text->lang];
        while (foundCheck)
        {
            strPoint = strstr(strPoint, "\r\n");
            if (strPoint != NULL)
            {
                lineCount++;
                strPoint++;
            }
            else
            {
                foundCheck = false;
            }
        }
        textbox->rowsCount = lineCount;
        return lineCount;
    }
    else
    {
        return 0;
    }
}

int ituTextBoxGetMaxRowsCount(ITUTextBox* textbox)
{
    if (textbox != NULL)
    {
        return textbox->maxrowsCount;
    }
    else
    {
        return 0;
    }
}

void ituTextBoxSetMaxRowsCount(ITUTextBox* textbox, int MaxRowsCount)
{
    if (textbox != NULL)
    {
        ITUText* text = &textbox->text;

        int targetCount = MaxRowsCount + 1;
        int lineCount = 1;
        bool foundCheck = true;
        char* strPoint = text->stringSet->strings[text->lang];
        while (foundCheck)
        {
            strPoint = strstr(strPoint, "\r\n");
            if (strPoint != NULL)
            {
                lineCount++;

                if (lineCount == targetCount)
                {
                    size_t len = strPoint - text->stringSet->strings[text->lang] + 1;
                    char* strTmp = (char*)malloc(sizeof(char) * len);
                    memset(strTmp, '\0', len);
                    strncpy(strTmp, text->stringSet->strings[text->lang], len - 1);
                    ituTextBoxSetString(textbox, strTmp);
                    free(strTmp);
                    foundCheck = false;
                }
                else
                {
                    strPoint++;
                }
            }
            else
            {
                if ((targetCount - 1) != lineCount)
                {
                    int i = 0;
                    size_t len = strlen(text->stringSet->strings[text->lang]) + 9 * (targetCount - lineCount - 1);
                    char* strTmp = (char*)malloc(sizeof(char) * (len + 1));
                    memset(strTmp, '\0', (len + 1));
                    strncpy(strTmp, text->stringSet->strings[text->lang], len - 9 * (targetCount - lineCount - 1));
                    for (; i < (targetCount - lineCount - 1); i++)
                    {
                        strncat(strTmp, "\r\nNewLine.", 9);
                    }
                    ituTextBoxSetString(textbox, strTmp);
                    free(strTmp);
                }
                foundCheck = false;
            }
        }

        textbox->maxrowsCount = MaxRowsCount;
    }
}