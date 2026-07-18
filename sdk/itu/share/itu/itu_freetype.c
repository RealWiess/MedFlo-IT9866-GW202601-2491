#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/internal/ftobjs.h>
#include FT_OUTLINE_H
#include FT_BITMAP_H

#include <assert.h>
#include <malloc.h>
#include <wchar.h>
#include "itu_cfg.h"
#include "ite/ith.h"
#include "ite/itu.h"
#include "itu_private.h"

#ifdef CFG_ITU_ARABICCONVERTER_SUPPORT
wchar_t * convertArabic (wchar_t * normal);
#endif

struct
{
    FT_Library     library;                       /* the FreeType library            */
    FT_Face        fonts[ITU_FREETYPE_MAX_FONTS]; /* installed fonts */
    FT_Face        current_font;                  /* selected font */
    FT_Face        default_font;                  /* default font */
    ITUGlyphFormat glyph_formats[ITU_FREETYPE_MAX_FONTS];
    ITUGlyphFormat current_glyph_format;
    ITUGlyphFormat default_glyph_format;
    unsigned int   style;
    int            bold_size;
    int            priority[ITU_FREETYPE_MAX_FONTS];    /* the font priority */
    int            current_index;
    unsigned int   style_extra[ITU_FREETYPE_MAX_FONTS]; /* the extra style for each font */
} ft;

#ifdef ITU_ENABLE_DEFER_DESTROY
extern void ituM2dDestroyDefferBuffer(void);
#endif

void ituFtExit(void)
{
    int i;
#ifdef ITU_ENABLE_DEFER_DESTROY
    ituM2dDestroyDefferBuffer();
#endif
    for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
    {
        if (ft.fonts[i])
        {
            FT_Done_Face(ft.fonts[i]);
        }
    }

    FT_Done_FreeType(ft.library);

    (void)memset(&ft, 0, sizeof(ft));
}

int ituFtLoadFont (int index, char * filename, ITUGlyphFormat format)
{
    FT_Error error;

    assert(index >= 0);
    assert(index < ITU_FREETYPE_MAX_FONTS);
    assert(filename);

    if (index >= ITU_FREETYPE_MAX_FONTS)
    {
        ITU_LOG_ERR("out of font index: %d >= %d\n", index, ITU_FREETYPE_MAX_FONTS);
        return __LINE__;
    }

    error = FT_New_Face(ft.library, filename, 0, &ft.fonts[index]);
    if (error)
    {
        ITU_LOG_ERR("couldn't open this file: %s\n", filename);
        return error;
    }

    ft.glyph_formats[index] = format;

    if (!ft.current_font)
    {
        ft.current_font         = ft.fonts[index];
        ft.current_glyph_format = format;
    }

    if (!ft.default_font)
    {
        ft.default_font         = ft.fonts[index];
        ft.default_glyph_format = format;
    }

    return FT_Err_Ok;
}

int ituFtLoadFontArray (int index, uint8_t * array, int size, ITUGlyphFormat format)
{
    FT_Error error;

    assert(index >= 0);
    assert(index < ITU_FREETYPE_MAX_FONTS);
    assert(array);
    assert(size > 0);

    if (index >= ITU_FREETYPE_MAX_FONTS)
    {
        ITU_LOG_ERR("out of font index: %d >= %d\n", index, ITU_FREETYPE_MAX_FONTS);
        return __LINE__;
    }

    error = FT_New_Memory_Face(ft.library, array, size, 0, &ft.fonts[index]);
    if (error)
    {
        ITU_LOG_ERR("couldn't open font array: %d\n", error);
        return error;
    }

    ft.glyph_formats[index] = format;

    if (!ft.current_font)
    {
        ft.current_font         = ft.fonts[index];
        ft.current_glyph_format = format;
    }

    if (!ft.default_font)
    {
        ft.default_font         = ft.fonts[index];
        ft.default_glyph_format = format;
    }

    return FT_Err_Ok;
}

void ituFtSetCurrentFont (int index)
{
    assert(index >= 0);
    assert(index < ITU_FREETYPE_MAX_FONTS);

    if (index < 0 || index >= ITU_FREETYPE_MAX_FONTS)
    {
        ITU_LOG_ERR("out of font index: %d >= %d\n", index, ITU_FREETYPE_MAX_FONTS);
        return;
    }

    ft.current_font         = ft.fonts[index];
    ft.current_glyph_format = ft.glyph_formats[index];
}

void ituFtSetFontSize (int width, int height)
{
    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        return;
    }

    FT_Set_Pixel_Sizes(ft.current_font, width, height);
}

int ituFtDrawText (ITUSurface * surf, int x, int y, const char * text)
{
    FT_Error error      = FT_Err_Ok;
    FT_ULong load_flags = FT_LOAD_DEFAULT;
    int      i = 0, len = 0, x_advance = 0;
    wchar_t  buf[512]    = {0};
    FT_UInt  gindex      = 0;
    FT_UInt  prev_gindex = 0;

    assert(text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }

    len       = mbstowcs(buf, text, 512);
    x_advance = 0;

    switch (ft.current_glyph_format)
    {
        case ITU_GLYPH_1BPP:
            load_flags = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            load_flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", ft.current_glyph_format);
            goto end;
    }

#ifdef CFG_ITU_ARABICCONVERTER_SUPPORT
    if (ft.style & ITU_FT_STYLE_ARABIC)
    {
        wchar_t * ptr = convertArabic(buf);
        if (ptr)
        {
            wcscpy(buf, ptr);
            free(ptr);
        }
    }
#endif

    for (i = 0; i < len; i++)
    {
        int            charcode = 0;
        FT_GlyphSlot   glyf     = NULL;
        FT_Glyph       glyph    = NULL;
        FT_Face        face     = ft.current_font;
        ITUGlyphFormat format   = ITU_GLYPH_8BPP;
        int            yy       = 0;

        charcode                = buf[i];

        error                   = FT_Load_Char(face, charcode, load_flags);
        if (error && face != ft.default_font)
        {
            face  = ft.default_font;
            error = FT_Load_Char(face, charcode, load_flags);
        }
        if (error)
        {
            continue;
        }

        if ((ft.style & ITU_FT_STYLE_BOLD) && ft.bold_size > 0)
        {
            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                FT_Outline_Embolden(&face->glyph->outline, ft.bold_size * 64);
            }
            else if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
            {
                FT_Bitmap_Embolden(ft.library, &face->glyph->bitmap, ft.bold_size * 64, ft.bold_size * 64);
            }
        }

        glyf  = face->glyph;
        error = FT_Get_Glyph(glyf, &glyph);
        if (error)
        {
            continue;
        }

        gindex = FT_Get_Char_Index(face, charcode);

        if (FT_HAS_KERNING(face) && prev_gindex)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, prev_gindex, gindex, FT_KERNING_UNSCALED, &delta);

            x_advance += delta.x >> 6;
        }

        switch (glyf->bitmap.pixel_mode)
        {
            case FT_PIXEL_MODE_MONO:
                format = ITU_GLYPH_1BPP;
                break;

            case FT_PIXEL_MODE_GRAY4:
                format = ITU_GLYPH_4BPP;
                break;

            case FT_PIXEL_MODE_GRAY:
                format = ITU_GLYPH_8BPP;
                break;
        }
        yy = y + face->size->metrics.y_ppem - glyf->bitmap_top;
        if (yy < 0)
        {
            ITURectangle prevClip = {0, 0, 0, 0};
            ituSetClipping(surf, x + x_advance + glyf->bitmap_left, 0, glyf->bitmap.width, glyf->bitmap.rows,
                           &prevClip);
            ituDrawGlyph(surf, x + x_advance + glyf->bitmap_left, yy, format, glyf->bitmap.buffer, glyf->bitmap.width,
                         glyf->bitmap.rows);
            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
        }
        else
        {
            ituDrawGlyph(surf, x + x_advance + glyf->bitmap_left, yy, format, glyf->bitmap.buffer, glyf->bitmap.width,
                         glyf->bitmap.rows);
        }

        x_advance += glyf->advance.x >> 6;
        FT_Done_Glyph(glyph);

        prev_gindex = gindex;
    }

end:
    return error;
}

void ituFtResetCache (void)
{
    // DO NOTHING
}

int ituFtGetTextDimension (const char * text, int * width, int * height, void* widget, int boldsize)
{
    FT_Error     error = FT_Err_Ok;
    FT_ULong     load_flags;
    int          i, len, x_advance;
    wchar_t      buf[512];
    unsigned int max_height;
    FT_UInt      gindex      = 0;
    FT_UInt      prev_gindex = 0;
    assert(text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        return -1;
    }

    len       = mbstowcs(buf, text, 512);
    x_advance = 0;

    switch (ft.current_glyph_format)
    {
        case ITU_GLYPH_1BPP:
            load_flags = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            load_flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", ft.current_glyph_format);
            return -1;
    }

#ifdef CFG_ITU_ARABICCONVERTER_SUPPORT
    if (ft.style & ITU_FT_STYLE_ARABIC)
    {
        wchar_t * ptr = convertArabic(buf);
        if (ptr)
        {
            wcscpy(buf, ptr);
            free(ptr);
        }
    }
#endif

    max_height = 0;
    for (i = 0; i < len; i++)
    {
        int          charcode;
        FT_GlyphSlot glyf;
        FT_Glyph     glyph;
        unsigned int h;
        FT_Face      face = ft.current_font;

        charcode          = buf[i];
        error             = FT_Load_Char(face, charcode, load_flags);
        if (error && face != ft.default_font)
        {
            face  = ft.default_font;
            error = FT_Load_Char(face, charcode, load_flags);
        }
        if (error)
        {
            continue;
        }

        glyf  = face->glyph;
        error = FT_Get_Glyph(glyf, &glyph);
        if (error)
        {
            continue;
        }

        gindex = FT_Get_Char_Index(face, charcode);

        if (FT_HAS_KERNING(face) && prev_gindex)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, prev_gindex, gindex, FT_KERNING_UNSCALED, &delta);

            x_advance += delta.x >> 6;
        }

        h = (face->size->metrics.ascender - face->size->metrics.descender) >> 6;
        if (max_height < h)
        {
            max_height = h;
        }

        x_advance += glyf->advance.x >> 6;
        FT_Done_Glyph(glyph);

        prev_gindex = gindex;
    }

    if (width)
    {
        *width = x_advance;
    }

    if (height)
    {
        *height = max_height;
    }

    return 0;
}

int ituFtGetCharWidth (const char * text, int * width, int boldsize)
{
    FT_Error error      = FT_Err_Ok;
    FT_ULong load_flags = FT_LOAD_DEFAULT;
    int      len        = 0;
    wchar_t  buf        = 0;
    assert(text);
    assert(width);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }

    len = strlen(text);
    len = mbtowc(&buf, text, len);

    switch (ft.current_glyph_format)
    {
        case ITU_GLYPH_1BPP:
            load_flags = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            load_flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", ft.current_glyph_format);
            goto end;
    }

    if (len > 0)
    {
        int          charcode = 0;
        FT_GlyphSlot glyf     = NULL;
        FT_Glyph     glyph    = NULL;
        FT_Face      face     = ft.current_font;

        charcode              = buf;
        error                 = FT_Load_Char(face, charcode, load_flags);
        if (error && face != ft.default_font)
        {
            face  = ft.default_font;
            error = FT_Load_Char(face, charcode, load_flags);
        }
        if (error)
        {
            ITU_LOG_ERR("load char fail: %d\n", error);
            goto end;
        }

        glyf  = face->glyph;
        error = FT_Get_Glyph(glyf, &glyph);
        if (error)
        {
            ITU_LOG_ERR("get glyph fail: %d\n", error);
            goto end;
        }

        *width = glyf->advance.x >> 6;
        FT_Done_Glyph(glyph);
    }

end:
    return len;
}

int ituFtDrawChar (ITUSurface * surf, int x, int y, const char * text)
{
    FT_Error error      = FT_Err_Ok;
    FT_ULong load_flags = FT_LOAD_DEFAULT;
    int      len        = 0;
    wchar_t  buf        = 0;
    assert(text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }

    len = strlen(text);
    len = mbtowc(&buf, text, len);

    switch (ft.current_glyph_format)
    {
        case ITU_GLYPH_1BPP:
            load_flags = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            load_flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", ft.current_glyph_format);
            goto end;
    }

    if (len > 0)
    {
        int            charcode = 0;
        FT_GlyphSlot   glyf     = NULL;
        FT_Glyph       glyph    = NULL;
        FT_Face        face     = ft.current_font;
        ITUGlyphFormat format   = ITU_GLYPH_8BPP;
        int            yy       = 0;

        charcode                = buf;
        error                   = FT_Load_Char(face, charcode, load_flags);
        if (error && face != ft.default_font)
        {
            face  = ft.default_font;
            error = FT_Load_Char(face, charcode, load_flags);
        }
        if (error)
        {
            ITU_LOG_ERR("load char fail: %d\n", error);
            goto end;
        }

        if (ft.style & ITU_FT_STYLE_BOLD)
        {
            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                FT_Outline_Embolden(&face->glyph->outline, ft.bold_size * 64);
            }
            else if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
            {
                FT_Bitmap_Embolden(ft.library, &face->glyph->bitmap, ft.bold_size * 64, ft.bold_size * 64);
            }
        }

        glyf  = face->glyph;
        error = FT_Get_Glyph(glyf, &glyph);
        if (error)
        {
            ITU_LOG_ERR("get glyph fail: %d\n", error);
            goto end;
        }

        switch (glyf->bitmap.pixel_mode)
        {
            case FT_PIXEL_MODE_MONO:
                format = ITU_GLYPH_1BPP;
                break;

            case FT_PIXEL_MODE_GRAY4:
                format = ITU_GLYPH_4BPP;
                break;

            case FT_PIXEL_MODE_GRAY:
                format = ITU_GLYPH_8BPP;
                break;
        }
        yy = y + face->size->metrics.y_ppem - glyf->bitmap_top;
        if (yy < 0)
        {
            ITURectangle prevClip = {0, 0, 0, 0};
            ituSetClipping(surf, x + glyf->bitmap_left, 0, glyf->bitmap.width, glyf->bitmap.rows, &prevClip);
            ituDrawGlyph(surf, x + glyf->bitmap_left, yy, format, glyf->bitmap.buffer, glyf->bitmap.width,
                         glyf->bitmap.rows);
            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
        }
        else
        {
            ituDrawGlyph(surf, x + glyf->bitmap_left, yy, format, glyf->bitmap.buffer, glyf->bitmap.width,
                         glyf->bitmap.rows);
        }
        FT_Done_Glyph(glyph);
    }

end:
    return error;
}

int ituFtInit (void)
{
    FT_Error error;

    (void)memset(&ft, 0, sizeof(ft));

    error = FT_Init_FreeType(&ft.library);
    if (error)
    {
        ITU_LOG_ERR("could not initialize FreeType\n");
        goto end;
    }

end:
    if (error)
    {
        ituFtExit();
    }

    return error;
}

void ituFtSetFontStyle (unsigned int style)
{
    ft.style = style;
}

void ituFtSetFontStyleValue (unsigned int style, int value)
{
    switch (style)
    {
        case ITU_FT_STYLE_BOLD:
            ft.bold_size = value;
            break;
    }
}

bool ituFtIsCharValid (const char * text)
{
    // FT_Error error = FT_Err_Ok;
    int     len;
    wchar_t buf    = 0;
    bool    result = false;
    assert(text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }

    len = strlen(text);
    len = mbtowc(&buf, text, len);

    if (len > 0)
    {
        int     charcode;
        FT_UInt gindex;
        FT_Face face = ft.current_font;

        charcode     = buf;
        gindex       = FT_Get_Char_Index(face, charcode);
        if (gindex == 0 && face != ft.default_font)
        {
            face   = ft.default_font;
            gindex = FT_Get_Char_Index(face, charcode);
        }
        if (gindex)
        {
            result = true;
            goto end;
        }
    }
end:
    return result;
}

void ituFtSetFontPriority (const int * priority, const int count)
{
    if (priority)
    {
        int i = 0;
        for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            ft.priority[i] = priority[i];
        }
    }
    else
    {
        int i = 0;
        for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            ft.priority[i] = -1;
        }
    }
}
