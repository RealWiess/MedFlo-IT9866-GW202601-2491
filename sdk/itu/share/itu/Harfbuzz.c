#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_MANAGER_H
#include FT_BITMAP_H
#include <freetype/internal/ftobjs.h>
#include FT_OUTLINE_H
#include <assert.h>
#include <malloc.h>
#include <wchar.h>

#include "ite/itu.h"
#include "itu_cfg.h"
#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    #include "../harfbuzz/hb.h"
    #include "../harfbuzz/hb-ft.h"
    #include "../harfbuzz/hb-ot.h"
#endif
#include "itu_private.h"

//===== Bless Chang =====
// interface combined Harfbuzz and ituFreeType
// work for (Text)(Harfbuzz) support

#define ITUHARFBUZZ_DEBUG 0
#if ITUHARFBUZZ_DEBUG
    #define harfbuzzLog(fmt, ...)  (void)printf("[%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
    #define harfbuzzLog(fmt, ...)
#endif

typedef struct
{
    char *         filepathname;
    char *         filename;
    uint8_t *      array;
    int            array_size;
    int            face_index;
    int            cmap_index;
    int            num_indices;
    FTC_ScalerRec  scaler;
    FT_ULong       load_flags;
    FT_Render_Mode render_mode;
    ITUGlyphFormat format;
    FT_Face        face;
} TFont;

typedef struct
{
    FT_Library         library;                       /* the FreeType library            */
    FTC_Manager        cache_manager;                 /* the cache manager               */
    FTC_ImageCache     image_cache;                   /* the glyph image cache           */
    FTC_SBitCache      sbits_cache;                   /* the glyph small bitmaps cache   */
    FTC_CMapCache      cmap_cache;                    /* the charmap cache..             */
    TFont              fonts[ITU_FREETYPE_MAX_FONTS]; /* installed fonts */
    TFont *            current_font;                  /* selected font */
    TFont *            default_font;                  /* default font */
    TFont* pri_font;
    FT_Bitmap          bitmap;                        /* used as bitmap conversion buffer */
    unsigned int       style;
    int                bold_size;
    int                normal_size;
    int                priority[ITU_FREETYPE_MAX_FONTS];    /* the font priority */
    int                current_index;
    unsigned int       style_extra[ITU_FREETYPE_MAX_FONTS]; /* the extra style for each font */
    ITUFtHarfbuzzStyle hbStyle[ITU_FREETYPE_MAX_FONTS];     /* the harfbuzz special style config */
#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    THarfbuzzGID* gid;
    hb_font_t* hbFont;
    hb_buffer_t* hbBuffer;
#endif
} TFontFT;

#ifdef ITU_ENABLE_DEFER_DESTROY
extern int  ituAddDeferDestroyData (void * pData, void * destroyFunc);

//static void _FreeFTCNode (void * pData)
//{
//    FTC_Node_Unref(pData, hft.cache_manager);
//}
#endif

int convertHarfBuzz (TFontFT *tft, ITUSurface * surf, int x, int y, const char * text, int maxLen)
{
#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int hid = -1;
    bool bAddSurfHash = false;
#endif

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    FT_Error error = FT_Err_Ok;
    int      i = 0, x_advance = 0;
    FT_UInt  prev_gindex = 0;
    wchar_t  buf[512]    = L"";
    TFont* workFont;
    TFontFT hft;
    int scalSize = tft->normal_size;
    unsigned int len = 0;
    unsigned int orglen = 0;
    uint32_t gbuf[512] = { 0 };
    bool foundSaved = false;
    bool shaped = false;
    uint32_t* gFound = NULL;

    memcpy(&hft, tft, sizeof(TFontFT));
    workFont = (hft.pri_font == NULL) ? (hft.current_font) : (hft.pri_font);

    if (maxLen <= 0)
    {
        orglen = mbstowcs(buf, text, 512);
    }
    else
    {
        orglen = mbtowc(buf, text, 512);
    }

    if (hft.gid != NULL)
    {
        if (hft.gid->count[hft.current_index] > 0)
        {
            int m = 0;
            for (; m < hft.gid->count[hft.current_index]; m++)
            {
                wchar_t* cacheCid = hft.gid->cidArr[hft.current_index][m];
                if (cacheCid != NULL)
                {
                    if (cacheCid[0] == (wchar_t)orglen)
                    {
                        int n = 1;
                        for (; n < 512; n++)
                        {
                            if (cacheCid[n] != buf[n - 1])
                            {
                                n--;
                                break;
                            }
                        }
                        if (n == (orglen * 2))
                        {
                            foundSaved = true;
                            break;
                        }
                    }
                }
            }
            if (foundSaved)
            {
                gFound = hft.gid->gidArr[hft.current_index][m];
            }
        }
    }

    if (workFont != NULL)
    {
        FT_Size size;
        FT_Face face;
        FTC_SBit sbit;
        FT_UInt gindex = FTC_CMapCache_Lookup(hft.cmap_cache, workFont->scaler.face_id, workFont->cmap_index, (int)buf[0]);

        if (gindex == 0)
        {
            return 0;
        }

        workFont->scaler.width = workFont->scaler.height = scalSize;

        error = FTC_Manager_LookupFace(hft.cache_manager, workFont->scaler.face_id, &face);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
            return 0;
        }
        workFont->face = face;

        error = FTC_SBitCache_LookupScaler(hft.sbits_cache, &workFont->scaler, workFont->load_flags, gindex, &sbit, NULL);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
            return 0;
        }
        error = FTC_Manager_LookupSize(hft.cache_manager, &workFont->scaler, &size);
        if (error != FT_Err_Ok)
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    if ((workFont->face->num_glyphs <= 0) || (workFont->face->stream == NULL))
    {
        return 0;
    }

    hft.hbFont = hb_ft_font_create(workFont->face, 0);
    hft.hbBuffer = hb_buffer_create();

    // for direction
    if (hft.hbStyle[hft.current_index].dir == 1)
    {
        hb_buffer_set_direction(hft.hbBuffer, HB_DIRECTION_RTL);
    }
    else
    {
        hb_buffer_set_direction(hft.hbBuffer, HB_DIRECTION_LTR);
    }

    // for country
    if (hft.hbStyle[hft.current_index].lang == 1)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_ARABIC);
    }
    else if (hft.hbStyle[hft.current_index].lang == 2)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_HEBREW);
    }
    else if (hft.hbStyle[hft.current_index].lang == 3)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_KHMER);
    }
    else if (hft.hbStyle[hft.current_index].lang == 4)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_THAI);
    }
    else
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_COMMON);
    }

    hb_buffer_add_utf8(hft.hbBuffer, text, -1, 0, (maxLen <= 0) ? (-1) : (maxLen));

    hb_buffer_guess_segment_properties(hft.hbBuffer);

    if (!foundSaved)
    {
        hb_shape(hft.hbFont, hft.hbBuffer, 0, 0);
        shaped = true;
    }

    if (hft.style_extra[hft.current_index] & ITU_FT_STYLE_HARFBUZZ)
    {
        hb_glyph_info_t *     infos       = hb_buffer_get_glyph_infos(hft.hbBuffer, 0);
        int                   calBoldSize = hft.bold_size;
        int                   sizeWForSpace = 0;
        len = hb_buffer_get_length(hft.hbBuffer);

        if (foundSaved && (gFound != NULL))
        {
            len = gFound[0];
        }

        if ((hft.style & ITU_FT_STYLE_BOLD) && hft.bold_size > 0)
        {
            calBoldSize = (calBoldSize >= 0xFF) ? (0xFF) : (calBoldSize);
        }
        else
        {
            calBoldSize = 0;
        }

        for (i = 0; i < len; i++)
        {
            int                   charcode = buf[i];
            FT_GlyphSlot          glyf;
            FT_Glyph              glyph;
            FT_Face               face = workFont->face;
            ITUGlyphFormat        format = ITU_GLYPH_8BPP;
            FT_BitmapGlyph        bitmapGlyph;
            FT_UInt               gindex;
            TFont *               tfont = workFont;

            hb_glyph_info_t *     info  = &infos[i];

            gindex = info->codepoint;
            if (foundSaved && (gFound != NULL))
            {
                gindex = gFound[i + 1];
            }
            gbuf[i] = gindex;

    #ifdef CFG_ITU_FONT_SURFACE_CACHE
            if (ituDrawGlyph == ituDrawGlyphEmpty)
            {
                hid = -1; // when ft cache preload font, don't use surface cache
            }
            else if (ituScene)
            {
                int id = 0;
                for (; id < ITU_FREETYPE_MAX_FONTS; id++)
                {
                    // if (hft.current_font == &hft.fonts[id])
                    if (hft.current_index == id)
                    {
                        bAddSurfHash =
                            ituSceneFontArrHashAdd(ituScene, id, ((gindex << 16) + calBoldSize), &(buf[i]),
                                                   scalSize, &hid, &surf->fgColor);
                        break;
                    }
                }
            }

            if ((hid >= 0) && (!bAddSurfHash))
            {
                x_advance += ituScene->fontSurf[hid].lb;
                harfbuzzLog("[C][(x,y)-%d,%d][x_adv %d][wrb %d][%s][hid %d][%d/%d][%lc]\n",
                    x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y, 
                    x_advance - ituScene->fontSurf[hid].lb, ituScene->fontSurf[hid].wrb,
                    (calBoldSize > 0) ? ("B") : (""), hid, i + 1, len, buf[i]);
                ituDrawGlyph(surf, x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y,
                             ituScene->fontSurf[hid].format, ituScene->fontSurf[hid].surf, ituScene->fontSurf[hid].fp,
                             ituScene->fontSurf[hid].fr);
                x_advance -= ituScene->fontSurf[hid].lb;
                x_advance += ituScene->fontSurf[hid].wrb;
                continue;
            }
    #endif

            if (!shaped)
            {
                hb_shape(hft.hbFont, hft.hbBuffer, 0, 0);
                infos = hb_buffer_get_glyph_infos(hft.hbBuffer, 0);
                len = hb_buffer_get_length(hft.hbBuffer);
                info = &infos[i];
                gindex = info->codepoint;
                gbuf[i] = gindex;
                shaped = true;
            }

            glyf = face->glyph;
            error = FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);
            if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
            {
                //for space --> use default glyph directly
                if ((sizeWForSpace > 0) && (charcode == 32))
                {
                    x_advance += sizeWForSpace;

                    harfbuzzLog("draw same space char [x_adv %d][wrb %d][%s][%d/%d][%lc]\n",
                        x_advance, sizeWForSpace,
                        (calBoldSize > 0) ? ("B") : (""), i + 1, len, buf[i]);

                    continue;
                }
                else if ((face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) && (charcode == 32))
                {
                    FT_Get_Glyph(face->glyph, &glyph);
                    error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_MONO, NULL, 0);
                    if (error)
                    {
                        harfbuzzLog("FT_Glyph_To_Bitmap fail: %d\n", error);
                        FT_Done_Glyph(glyph);
                        continue;
                    }
                }
                else
                {
                    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                    error = FT_Get_Glyph(face->glyph, &glyph);
                    if (error)
                    {
                        harfbuzzLog("FT_Ger_Glyph fail: %d\n", error);
                        continue;
                    }
                }
            }
            else
            {
                FT_Bitmap_Embolden(hft.library, &face->glyph->bitmap, calBoldSize, calBoldSize);
                error = FT_Get_Glyph(face->glyph, &glyph);
                if (error)
                {
                    harfbuzzLog("FT_Ger_Glyph fail: %d\n", error);
                    continue;
                }
            }
            
            // FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
            bitmapGlyph = (FT_BitmapGlyph)glyph;

            ithFlushDCacheRange(bitmapGlyph->bitmap.buffer, bitmapGlyph->bitmap.rows * bitmapGlyph->bitmap.pitch);
            ithFlushMemBuffer();

            switch (bitmapGlyph->bitmap.pixel_mode)
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
                default:
                    format = ITU_GLYPH_8BPP;
                    break;
            }
            if (bitmapGlyph->bitmap.buffer != NULL)
            {
                int    xx, yy;

                int realHeight = ((glyf->face->bbox.yMax - glyf->face->bbox.yMin) >> 6);
                //int upfix = (glyf->face->underline_position >> 6);
                int metricHeight = (glyf->face->height >> 6);
                //int topFixY = realHeight - bitmapGlyph->top + upfix;
                int topFixY = face->size->metrics.y_ppem - bitmapGlyph->top;

                x_advance += bitmapGlyph->left;
                xx = x + x_advance;
                //yy = y + face->size->metrics.y_ppem - bitmapGlyph->top;
                yy = y + topFixY;

    #ifdef CFG_ITU_FONT_SURFACE_CACHE
                if (hid >= 0)
                {
                    ituScene->fontSurf[hid].lb     = bitmapGlyph->left;
                    ituScene->fontSurf[hid].x      = 0;
                    ituScene->fontSurf[hid].wrb    = glyf->advance.x >> 6;
                    ituScene->fontSurf[hid].y      = yy - y;
                    ituScene->fontSurf[hid].fr     = bitmapGlyph->bitmap.rows;
                    ituScene->fontSurf[hid].fp     = bitmapGlyph->bitmap.width;
                    ituScene->fontSurf[hid].h      = bitmapGlyph->bitmap.rows;
                    ituScene->fontSurf[hid].format = format;
                    ituScene->fontSurf[hid].surf =
                        (unsigned char *)malloc(ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp);
                    (void)memcpy(ituScene->fontSurf[hid].surf, bitmapGlyph->bitmap.buffer,
                           (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
        #if CFG_CPU_WB
                    ithFlushDCacheRange((void *)ituScene->fontSurf[hid].surf,
                                        (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                    ithFlushMemBuffer();
        #endif
                }
    #endif
                ituDrawGlyph(surf, xx, yy, format, bitmapGlyph->bitmap.buffer, bitmapGlyph->bitmap.width,
                             bitmapGlyph->bitmap.rows);
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                harfbuzzLog("[N][(x,y)-%d,%d][x_adv %d][wrb %d][%s][hid %d][%d/%d][%lc]\n",
                    xx, yy,
                    x_advance - bitmapGlyph->left, ituScene->fontSurf[hid].wrb,
                    (calBoldSize > 0) ? ("B") : (""), hid, i + 1, len, buf[i]);
#else
                harfbuzzLog("[(x,y)-%d,%d][x_adv %d][wrb %d][%s][%d/%d][%lc]\n",
                    xx, yy,
                    x_advance - bitmapGlyph->left, (glyf->advance.x >> 6),
                    (calBoldSize > 0) ? ("B") : (""), i + 1, len, buf[i]);
#endif
                x_advance -= bitmapGlyph->left;
                x_advance += (glyf->advance.x >> 6);
                if (charcode == 32)
                {
                    sizeWForSpace = glyf->advance.x >> 6;
                }
            }
            else
            {
                // ITU_LOG_ERR("FreeCache drawtext get no valid glyph map from bold outline!\n" );
                harfbuzzLog("[draw]FT_Bitmap_Buffer error.\n");
            }

    #ifdef ITU_ENABLE_DEFER_DESTROY
            ituAddDeferDestroyData(glyph, FT_Done_Glyph);
    #else
            FT_Done_Glyph(glyph);
    #endif
            prev_gindex = gindex;
        }
        if ((!foundSaved) && (hft.gid != NULL))
        {
            hft.gid->count[hft.current_index]++;
            if (hft.gid->count[hft.current_index] >= 1)
            {
                wchar_t* cacheCid;
                uint32_t* cacheGid;
                if (hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] != NULL)
                {
                    free(hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1]);
                }
                if (hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] != NULL)
                {
                    free(hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1]);
                }
                hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] = (wchar_t*)malloc(sizeof(wchar_t) * (orglen * 2 + 1));
                hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] = (uint32_t*)malloc(sizeof(uint32_t) * (len + 1));
                cacheCid = hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1];
                cacheGid = hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1];
                if (cacheCid != NULL)
                {
                    memset(cacheCid, 0, (sizeof(wchar_t) * (orglen * 2 + 1)));
                    cacheCid[0] = (wchar_t)orglen;
                    wcsncpy(&cacheCid[1], &buf[0], orglen);
#if CFG_CPU_WB
                    ithFlushDCacheRange((void*)cacheCid, (sizeof(wchar_t)* (orglen * 2 + 1)));
                    ithFlushMemBuffer();
#endif
                }
                if (cacheGid != NULL)
                {
                    memset(cacheGid, 0, (sizeof(uint32_t) * (len + 1)));
                    cacheGid[0] = len;
                    memcpy(&cacheGid[1], &gbuf[0], sizeof(uint32_t) * len);
#if CFG_CPU_WB
                    ithFlushDCacheRange((void*)cacheGid, (sizeof(uint32_t)* (len + 1)));
                    ithFlushMemBuffer();
#endif
                }
            }
        }
    }

    hb_buffer_destroy(hft.hbBuffer);
    hb_font_destroy(hft.hbFont);
    hft.hbFont = NULL;
    memcpy(tft, &hft, sizeof(TFontFT));

#if CFG_CPU_WB
    ithFlushDCacheRange((void*)tft, (sizeof(TFontFT)));
    ithFlushMemBuffer();
#endif

#endif

    return 0;
}

int getSizeHarfBuzz (TFontFT * tft, int * width, int * height, const char * text, int maxLen)
{
#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int  hid          = -1;
    bool bAddSurfHash = false;
#endif

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    FT_Error error = FT_Err_Ok;
    int      i = 0, x_advance = 0, max_height = 0, max_bottom_shift = 0;
    FT_UInt  prev_gindex = 0;
    wchar_t  buf[512]    = L"";
    TFont* workFont;
    TFontFT hft;
    int scalSize = tft->normal_size;
    unsigned int len = 0;
    unsigned int orglen = 0;
    uint32_t gbuf[512] = { 0 };
    bool foundSaved = false;
    bool shaped = false;
    uint32_t* gFound = NULL;

    memcpy(&hft, tft, sizeof(TFontFT));
    workFont = (hft.pri_font == NULL) ? (hft.current_font) : (hft.pri_font);

    if (maxLen == 0)
    {
        orglen = mbstowcs(buf, text, 512);
    }
    else
    {
        orglen = mbtowc(buf, text, 512);
    }

    if (hft.gid != NULL)
    {
        if (hft.gid->count[hft.current_index] > 0)
        {
            int m = 0;
            for (; m < hft.gid->count[hft.current_index]; m++)
            {
                wchar_t* cacheCid = hft.gid->cidArr[hft.current_index][m];
                if (cacheCid != NULL)
                {
                    if (cacheCid[0] == (wchar_t)orglen)
                    {
                        int n = 1;
                        for (; n < 512; n++)
                        {
                            if (cacheCid[n] != buf[n - 1])
                            {
                                n--;
                                break;
                            }
                        }
                        if (n == (orglen * 2))
                        {
                            foundSaved = true;
                            break;
                        }
                    }
                }
            }
            if (foundSaved)
            {
                gFound = hft.gid->gidArr[hft.current_index][m];
            }
        }
    }

    if (workFont != NULL)
    {
        FT_Size size;
        FT_Face face;
        FTC_SBit sbit;
        FT_UInt gindex = FTC_CMapCache_Lookup(hft.cmap_cache, workFont->scaler.face_id, workFont->cmap_index, (int)buf[0]);

        if (gindex == 0)
        {
            return 0;
        }

        workFont->scaler.width = workFont->scaler.height = scalSize;

        error = FTC_Manager_LookupFace(hft.cache_manager, workFont->scaler.face_id, &face);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
            return 0;
        }
        workFont->face = face;
        error = FTC_SBitCache_LookupScaler(hft.sbits_cache, &workFont->scaler, workFont->load_flags, gindex, &sbit, NULL);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
            return 0;
        }
        error = FTC_Manager_LookupSize(hft.cache_manager, &workFont->scaler, &size);
        if (error != FT_Err_Ok)
        {
            return 0;
        }
    }
    else
    {
        harfbuzzLog("No work font!\n");
        return 0;
    }

    if ((workFont->face->num_glyphs <= 0) || (workFont->face->stream == NULL))
    {
        harfbuzzLog("working font with face error!\n");
        return 0;
    }

    hft.hbFont = hb_ft_font_create(workFont->face, 0);
    hft.hbBuffer = hb_buffer_create();

    // for direction
    if (hft.hbStyle[hft.current_index].dir == 1)
    {
        hb_buffer_set_direction(hft.hbBuffer, HB_DIRECTION_RTL);
    }
    else
    {
        hb_buffer_set_direction(hft.hbBuffer, HB_DIRECTION_LTR);
    }

    // for country
    if (hft.hbStyle[hft.current_index].lang == 1)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_ARABIC);
    }
    else if (hft.hbStyle[hft.current_index].lang == 2)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_HEBREW);
    }
    else if (hft.hbStyle[hft.current_index].lang == 3)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_KHMER);
    }
    else if (hft.hbStyle[hft.current_index].lang == 4)
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_THAI);
    }
    else
    {
        hb_buffer_set_script(hft.hbBuffer, HB_SCRIPT_COMMON);
    }

    hb_buffer_add_utf8(hft.hbBuffer, text, -1, 0, (maxLen == 0) ? (-1) : (maxLen));
    hb_buffer_guess_segment_properties(hft.hbBuffer);

    if (!foundSaved)
    {
        hb_shape(hft.hbFont, hft.hbBuffer, 0, 0);
        shaped = true;
    }

    if (hft.style_extra[hft.current_index] & ITU_FT_STYLE_HARFBUZZ)
    {
        hb_glyph_info_t *     infos       = hb_buffer_get_glyph_infos(hft.hbBuffer, 0);
        int                   calBoldSize = hft.bold_size;
        int                   sizeWForSpace = 0;
        len = hb_buffer_get_length(hft.hbBuffer);

        if (foundSaved && (gFound != NULL))
        {
            if (gFound[0] > 0)
                len = gFound[0];
        }
        

        if ((hft.style & ITU_FT_STYLE_BOLD) && hft.bold_size > 0)
        {
            calBoldSize = (calBoldSize >= 0xFF) ? (0xFF) : (calBoldSize);
        }
        else
        {
            calBoldSize = 0;
        }

        for (i = 0; i < len; i++)
        {
            int                   charcode = buf[i];
            FT_GlyphSlot          glyf;
            FT_Glyph              glyph;
            FT_Face               face = workFont->face;
            FT_BitmapGlyph        bitmapGlyph;
            FT_UInt               gindex = 0;
            TFont *               tfont  = workFont;
            ITUGlyphFormat        format = ITU_GLYPH_8BPP;

            hb_glyph_info_t *     info   = &infos[i];

            gindex                       = info->codepoint;
            if (foundSaved && (gFound != NULL))
            {
                gindex = gFound[i + 1];
            }
            gbuf[i] = gindex;

    #ifdef CFG_ITU_FONT_SURFACE_CACHE
            if (ituDrawGlyph == ituDrawGlyphEmpty)
            {
                hid = -1; // when ft cache preload font, don't use surface cache
            }
            else if (ituScene)
            {
                int id = 0;
                for (; id < ITU_FREETYPE_MAX_FONTS; id++)
                {
                    if (hft.current_index == id)
                    {
                        bAddSurfHash =
                            ituSceneFontArrHashSizeCheck(ituScene, id, ((gindex << 16) + calBoldSize),
                                                         &buf[i], scalSize, &hid);

                        if (!bAddSurfHash)
                        {
                            ituSceneFontArrHashSizePreSet(ituScene, id, ((gindex << 16) + calBoldSize), &buf[i], scalSize, &hid);
                        }

                        break;
                    }
                }
            }

            if ((hid >= 0) && bAddSurfHash)
            {
                x_advance += ituScene->fontSurf[hid].wrb;

                if (max_height < ituScene->fontSurf[hid].h)
                {
                    max_height = ituScene->fontSurf[hid].h;
                }

                if (max_bottom_shift < ituScene->fontSurf[hid].bottomShift)
                {
                    max_bottom_shift = ituScene->fontSurf[hid].bottomShift;
                }

                harfbuzzLog("[C][wrb %d][x_adv %d]-[%d/%d][%lc]\n", ituScene->fontSurf[hid].wrb, x_advance, i + 1, len, buf[i]);

                continue;
            }
    #endif

            if (!shaped)
            {
                hb_shape(hft.hbFont, hft.hbBuffer, 0, 0);
                infos = hb_buffer_get_glyph_infos(hft.hbBuffer, 0);
                len = hb_buffer_get_length(hft.hbBuffer);
                info = &infos[i];
                gindex = info->codepoint;
                gbuf[i] = gindex;
                shaped = true;
            }

            glyf  = face->glyph;
            error = FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);
            if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
            {
                //for space --> use default glyph directly
                if ((sizeWForSpace > 0) && (charcode == 32))
                {
                    x_advance += sizeWForSpace;
                    harfbuzzLog("same space char size [(w,h)-%d,%d][x_adv %d][%s][%d/%d][%lc]\n",
                        sizeWForSpace, max_height, x_advance,
                        (calBoldSize > 0) ? ("B") : (""), i + 1, len, buf[i]);
                    continue;
                }
                else if ((face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) && (charcode == 32))
                {
                    FT_Get_Glyph(face->glyph, &glyph);
                    error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_MONO, NULL, 0);
                    if (error)
                    {
                        harfbuzzLog("FT_Glyph_To_Bitmap fail: %d\n", error);
                        FT_Done_Glyph(glyph);
                        continue;
                    }
                }
                else
                {
                    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                    error = FT_Get_Glyph(face->glyph, &glyph);
                    if (error)
                    {
                        harfbuzzLog("FT_Ger_Glyph fail: %d\n", error);
                        continue;
                    }
                }
            }
            else
            {
                FT_Bitmap_Embolden(hft.library, &face->glyph->bitmap, calBoldSize, calBoldSize);
                error = FT_Get_Glyph(face->glyph, &glyph);
                if (error)
                {
                    harfbuzzLog("FT_Ger_Glyph fail: %d\n", error);
                    continue;
                }
            }

            bitmapGlyph = (FT_BitmapGlyph)glyph;
            ithFlushDCacheRange(bitmapGlyph->bitmap.buffer, bitmapGlyph->bitmap.rows * bitmapGlyph->bitmap.pitch);
            ithFlushMemBuffer();

            switch (bitmapGlyph->bitmap.pixel_mode)
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
                default:
                    format = ITU_GLYPH_8BPP;
                    break;
            }
            if (bitmapGlyph->bitmap.buffer != NULL)
            {
                //face->size->metrics.y_ppem //tfont->face->size->metrics.height >> 6;
                //face->size->metrics.y_ppem - bitmapGlyph->top
                int realHeight = ((glyf->face->bbox.yMax - glyf->face->bbox.yMin) >> 6);
                //int upfix = (glyf->face->underline_position >> 6);
                int metricHeight = (glyf->face->height >> 6);
                //int topFixY = realHeight - bitmapGlyph->top + upfix;
                int topFixY = face->size->metrics.y_ppem - bitmapGlyph->top;

                if (metricHeight > face->size->metrics.y_ppem)
                    metricHeight = (glyf->metrics.vertAdvance >> 6);

                if (max_bottom_shift < ((glyf->metrics.height >> 6) - bitmapGlyph->top))
                {
                    max_bottom_shift = ((glyf->metrics.height >> 6) - bitmapGlyph->top);
                }

                if (max_height < metricHeight)
                {
                    max_height = metricHeight;
                }

    #ifdef CFG_ITU_FONT_SURFACE_CACHE
                if ((hid >= 0) && (hid < ITU_FONT_SURFACE_CACHE_STRING_COUNT))
                {
                    ituScene->fontSurf[hid].lb = bitmapGlyph->left;
                    ituScene->fontSurf[hid].x = 0;
                    ituScene->fontSurf[hid].wrb = glyf->advance.x >> 6;
                    ituScene->fontSurf[hid].y = topFixY;
                    ituScene->fontSurf[hid].fr = bitmapGlyph->bitmap.rows;
                    ituScene->fontSurf[hid].fp = bitmapGlyph->bitmap.width;
                    ituScene->fontSurf[hid].h = metricHeight;
                    ituScene->fontSurf[hid].format = format;
                    if (ituScene->fontSurf[hid].surf == NULL)
                    {
                        ituScene->fontSurf[hid].surf =
                            (unsigned char*)malloc(ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp);
                        (void)memcpy(ituScene->fontSurf[hid].surf, bitmapGlyph->bitmap.buffer,
                            (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                        harfbuzzLog("[N][(w,h)-%d,%d][x_adv %d][%s][hid %d][%d/%d][%lc]\n",
                            ituScene->fontSurf[hid].wrb, metricHeight, x_advance + ituScene->fontSurf[hid].wrb,
                            (calBoldSize > 0) ? ("B") : (""), hid, i + 1, len, buf[i]);
#if CFG_CPU_WB
                        ithFlushDCacheRange((void*)ituScene->fontSurf[hid].surf,
                            (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                        ithFlushMemBuffer();
#endif
                    }
                }
    #else
                harfbuzzLog("[(w,h)-%d,%d][x_adv %d][%s][%d/%d][%lc]\n",
                    (glyf->advance.x >> 6), metricHeight, x_advance + (glyf->advance.x >> 6),
                    (calBoldSize > 0) ? ("B") : (""), i + 1, len, buf[i]);
    #endif

                x_advance += glyf->advance.x >> 6;
                if (charcode == 32)
                {
                    sizeWForSpace = glyf->advance.x >> 6;
                }
            }
            else
            {
                harfbuzzLog("[size]FT_Bitmap_Buffer error.\n");
            }

    #ifdef ITU_ENABLE_DEFER_DESTROY
            ituAddDeferDestroyData(glyph, FT_Done_Glyph);
    #else
            FT_Done_Glyph(glyph);
    #endif
            prev_gindex = gindex;
        }
        if ((!foundSaved) && (hft.gid != NULL))
        {
            hft.gid->count[hft.current_index]++;
            if (hft.gid->count[hft.current_index] >= 1)
            {
                wchar_t* cacheCid;
                uint32_t* cacheGid;
                if (hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] != NULL)
                {
                    free(hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1]);
                }
                if (hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] != NULL)
                {
                    free(hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1]);
                }
                hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] = (wchar_t*)malloc(sizeof(wchar_t) * (orglen * 2 + 1));
                hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1] = (uint32_t*)malloc(sizeof(uint32_t) * (len + 1));
                cacheCid = hft.gid->cidArr[hft.current_index][hft.gid->count[hft.current_index] - 1];
                cacheGid = hft.gid->gidArr[hft.current_index][hft.gid->count[hft.current_index] - 1];
                if (cacheCid != NULL)
                {
                    memset(cacheCid, 0, (sizeof(wchar_t) * (orglen * 2 + 1)));
                    cacheCid[0] = (wchar_t)orglen;
                    wcsncpy(&cacheCid[1], &buf[0], orglen);
#if CFG_CPU_WB
                    ithFlushDCacheRange((void*)cacheCid, (sizeof(wchar_t)* (orglen * 2 + 1)));
                    ithFlushMemBuffer();
#endif
                }
                if (cacheGid != NULL)
                {
                    memset(cacheGid, 0, (sizeof(uint32_t) * (len + 1)));
                    cacheGid[0] = len;
                    memcpy(&cacheGid[1], &gbuf[0], sizeof(uint32_t)* len);
#if CFG_CPU_WB
                    ithFlushDCacheRange((void*)cacheGid, (sizeof(uint32_t)* (len + 1)));
                    ithFlushMemBuffer();
#endif
                }
            }
        }
    }

    hb_buffer_destroy(hft.hbBuffer);
    hb_font_destroy(hft.hbFont);
    hft.hbFont = NULL;
    memcpy(tft, &hft, sizeof(TFontFT));

#if CFG_CPU_WB
    ithFlushDCacheRange((void*)tft, (sizeof(TFontFT)));
    ithFlushMemBuffer();
#endif

    if (width)
    {
        *width = x_advance;
    }

    if (height)
    {
        *height = max_height;
    }

    return max_bottom_shift;
#else
    return 0;
#endif
}
