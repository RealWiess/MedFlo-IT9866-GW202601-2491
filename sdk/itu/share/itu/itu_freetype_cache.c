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
#include "itu_cfg.h"
#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    #include "../harfbuzz/hb.h"
    #include "../harfbuzz/hb-ot.h"
#endif
#include "ite/ith.h"
#include "ite/itu.h"
#include "itu_private.h"

#ifdef CFG_ITU_ARABICCONVERTER_SUPPORT
wchar_t * convertArabic (wchar_t * normal);
wchar_t * convertHebrew (wchar_t * normal);
#endif

#define MAX_SBIT_CACHE         128
#define MAX_CAL_BOLD_SIZE      256

#define ITUFREETYPECACHE_DEBUG 0
#if ITUFREETYPECACHE_DEBUG
    #define freetypeCacheLog(fmt, ...)  (void)printf("[%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
    #define freetypeCacheLog(fmt, ...)
#endif

/* this simple record is used to model a given `installed' face */
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

int convertHarfBuzz (TFontFT *tft, ITUSurface *surf, int x, int y, const char *text, int maxLen);
int getSizeHarfBuzz (TFontFT *tft, int *width, int *height, const char *text, int maxLen);

static TFontFT ft;

#ifdef ITU_ENABLE_DEFER_DESTROY
extern int  ituAddDeferDestroyData (void * pData, void * destroyFunc);

static void _FreeFTCNode (void * pData)
{
    FTC_Node_Unref(pData, ft.cache_manager);
}
#endif

#define btw(a, b, c) (((a) >= (b)) && ((a) <= (c)))

//eng 0041-005A, 0061-007A
//chinese 2E80 - 2FDF, 3100 - 312F, 3400 - 4DBF, 4E00 - 9FFF, F900 - FAFF
//jpn 3040 - 30FF, 31F0 - 31FF
//kor 1100 - 11FF, 3130 - 318F, AC00 - D7AF
static int FT_uni_pass_table[][2] = { { 0x0021, 0x007E }, 
    { 0x00A1, 0x017F }, { 0x2010, 0x22C5 }, { 0x0041, 0x005A },
    { 0x0061, 0x007A }, { 0x2E80, 0x2FDF }, { 0x3100, 0x312F }, { 0x3400, 0x4DBF },
    { 0x4E00, 0x9FFF }, { 0xF900, 0xFAFF }, { 0x3040, 0x30FF }, { 0x31F0, 0x31FF },
    { 0x1100, 0x11FF }, { 0x3130, 0x318F }, { 0xAC00, 0xD7AF },
};

static FT_Error Bitmap_Convert_GRAY4 (FT_Library library, const FT_Bitmap * source, FT_Bitmap * target)
{
    FT_Error  error = FT_Err_Ok;
    FT_Memory memory;

    if (!library)
    {
        return FT_Err_Invalid_Library_Handle;
    }

    memory = library->memory;

    switch (source->pixel_mode)
    {
        case FT_PIXEL_MODE_GRAY:
        case FT_PIXEL_MODE_GRAY2:
            {
                FT_Long old_size;

                old_size = target->rows * target->pitch;
                if (old_size < 0)
                {
                    old_size = -old_size;
                }

                target->pixel_mode = FT_PIXEL_MODE_GRAY4;
                target->rows       = source->rows;
                target->width      = source->width;

                target->pitch      = ITH_ALIGN_UP(source->width, 2) / 2;

                if (target->rows * target->pitch > (unsigned int)old_size &&
                    FT_QREALLOC(target->buffer, old_size, target->rows * target->pitch))
                {
                    return error;
                }
            }
            break;

        default:
            error = FT_Err_Invalid_Argument;
    }

    switch (source->pixel_mode)
    {
        case FT_PIXEL_MODE_GRAY:
            {
                FT_Byte * s = source->buffer;
                FT_Byte * t = target->buffer;
                FT_Int    i;

                target->num_grays = 16;

                if ((s != NULL) && (t != NULL))
                {
                    for (i = source->rows; i > 0; i--)
                    {
                        FT_Byte* ss = s;
                        FT_Byte* tt = t;
                        FT_Int    j;

                        /* get the full bytes */
                        for (j = source->width >> 1; j > 0; j--)
                        {
                            tt[0] = (FT_Byte)((ss[0] & 0xF0) | ss[1] >> 4);

                            ss += 2;
                            tt += 1;
                        }

                        j = source->width & 1;
                        if (j > 0)
                        {
                            tt[0] = (FT_Byte)(ss[0] & 0xF0);
                        }

                        s += source->pitch;
                        t += target->pitch;
                    }
                }
            }
            break;

        case FT_PIXEL_MODE_GRAY2:
            {
                FT_Byte * s = source->buffer;
                FT_Byte * t = target->buffer;
                FT_Int    i;

                target->num_grays = 16;

                if ((s != NULL) && (t != NULL))
                {
                    for (i = source->rows; i > 0; i--)
                    {
                        FT_Byte* ss = s;
                        FT_Byte* tt = t;
                        FT_Int    j;

                        /* get the full bytes */
                        for (j = source->width >> 2; j > 0; j--)
                        {
                            FT_Int val = ss[0];

                            tt[0] = (FT_Byte)(((val & 0xC0) >> 2) | ((val & 0x30) >> 4));
                            tt[1] = (FT_Byte)(((val & 0x0C) << 2) | (val & 0x03));

                            ss += 1;
                            tt += 2;
                        }

                        j = source->width & 3;
                        if (j > 0)
                        {
                            FT_Int val = ss[0];

                            switch (j)
                            {
                            case 3:
                                tt[0] = (FT_Byte)(((val & 0xC0) >> 2) | ((val & 0x30) >> 4));
                                tt[1] = (FT_Byte)((val & 0x0C) << 2);
                                break;

                            case 2:
                                tt[0] = (FT_Byte)(((val & 0xC0) >> 2) | ((val & 0x30) >> 4));
                                break;

                            case 1:
                                tt[0] = (FT_Byte)((val & 0xC0) >> 2);
                                break;
                            }
                        }

                        s += source->pitch;
                        t += target->pitch;
                    }
                }
            }
            break;

        default:;
    }

    return error;
}

/* The face requester is a function provided by the client application   */
/* to the cache manager, whose role is to translate an `abstract' face   */
/* ID into a real FT_Face object.                                        */
/*                                                                       */
/* In this program, the face IDs are simply pointers to TFont objects.   */
static FT_Error face_requester (FTC_FaceID face_id, FT_Library lib, FT_Pointer request_data, FT_Face * aface)
{
    FT_Error error;
    TFont *  font = (TFont *)face_id;

    if (request_data != NULL)
    {
        ITU_LOG_ERR("FT requester address: %d\n", (int)(&request_data));
    }

    if (font->array)
    {
        error = FT_New_Memory_Face(lib, font->array, font->array_size, font->face_index, aface);
    }
    else
    {
        error = FT_New_Face(lib, font->filepathname, font->face_index, aface);
    }
    if (error)
    {
        ITU_LOG_ERR("FT_New_Face fail: %d\n", error);
        font->face = NULL;
    }
    else
    {
        if ((*aface)->charmaps)
        {
            (*aface)->charmap = (*aface)->charmaps[font->cmap_index];
        }

        font->face = *aface;
    }

    return error;
}

bool ituFtFaceCheck(TFont* font)
{
    FT_Error     error = FT_Err_Ok;
    int result = true;
    if (font->face == NULL)
    {
        FT_Size size;
        FT_Face face;
        FTC_SBit sbit;
        FT_UInt gindex = 65;
        error = FTC_Manager_LookupFace(ft.cache_manager, font->scaler.face_id, &face);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
            result = false;
        }
        error = FTC_SBitCache_LookupScaler(ft.sbits_cache, &font->scaler, font->load_flags, gindex, &sbit, NULL);
        if (error != FT_Err_Ok)
        {
            ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
            result = false;
        }
        error = FTC_Manager_LookupSize(ft.cache_manager, &font->scaler, &size);
        if (error != FT_Err_Ok)
        {
            result = false;
        }
    }
    else
    {
        if (font->face->size == NULL)
        {
            FT_Size  size;
            error = FTC_Manager_LookupSize(ft.cache_manager, &font->scaler, &size);
            if (error != FT_Err_Ok)
            {
                result = false;
            }
        }
    }
    return result;
}

bool ituFtChkHarfbuzzNN(int code)
{
    int i = 0;
    int size = sizeof(FT_uni_pass_table) / (sizeof(int) * 2);

    for (; i < size; i++)
    {
        if ((code >= FT_uni_pass_table[i][0]) && (code <= FT_uni_pass_table[i][1]))
        {
            return true; //language use default freetype
        }
        if (FT_uni_pass_table[i][0] == 0)
        {
            return false;
        }
    }
    return false;
}

FT_UInt ituFtCheckGlyphIndex (TFont ** tfont, int charcode, const char * text, int pos)
{
    FT_UInt gindex = 0;
    int     i = 0;
    bool    usePriority  = false;
    bool    gindexMissed = false;

    for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
    {
        if (i == 0)
        {
            if (((ft.priority[0] == 0) || (&ft.fonts[ft.priority[0]] == ft.current_font)) && (ft.priority[1] <= 0))
            {
                break;
            }
        }
        if (ft.priority[i] >= 0)
        {
            usePriority = true;
            break;
        }
        else
        {
            break;
        }
    }

    ft.default_font = ft.current_font;
    ft.pri_font = NULL;

    //use current font to find out the gindex at first
    if (ft.current_font->num_indices > 0)
    {
        gindex = FTC_CMapCache_Lookup(ft.cmap_cache, ft.current_font->scaler.face_id, ft.current_font->cmap_index, charcode);
    }

    if (usePriority && (gindex == 0))
    {
        TFont * tfontVar = NULL;
        i                = 0;
        gindexMissed = true;
        while ((gindex == 0) && (ft.priority[i] < ITU_FREETYPE_MAX_FONTS))
        {
            if (ft.priority[i] < 0)
            {
                i++;
                break;
            }
            tfontVar = &ft.fonts[ft.priority[i]];
            if (tfontVar == NULL)
            {
                i++;
                continue;
            }
            else
            {
                tfontVar->scaler.width  = ft.current_font->scaler.width;
                tfontVar->scaler.height = ft.current_font->scaler.height;
                tfontVar->scaler.pixel  = 1;
                *tfont                  = &ft.fonts[ft.priority[i]];
                gindex = FTC_CMapCache_Lookup(ft.cmap_cache, tfontVar->scaler.face_id, tfontVar->cmap_index, charcode);
                if (gindex > 0)
                {
                    freetypeCacheLog("[FreeType]pos %d of %s using priority font index %s now.\n", pos, text, tfontVar->filename);
                    ft.pri_font = tfontVar;
                    gindexMissed = false;

                    //do face check when find out gindex with other font(not current)
                    ituFtFaceCheck(ft.pri_font);
                }
            }
            i++;
        }
    }
    else
    {
        // make sure to use fontindex 0 as default font when current(or all) font has a lack of char
        if (gindex == 0)
        {
            TFont * tfontZero = &ft.fonts[0];
            if (tfontZero->num_indices > 0)
            {
                tfontZero->scaler.width  = ft.current_font->scaler.width;
                tfontZero->scaler.height = ft.current_font->scaler.height;
                tfontZero->scaler.pixel  = 1;
                *tfont                   = tfontZero;
                gindex = FTC_CMapCache_Lookup(ft.cmap_cache, tfontZero->scaler.face_id, tfontZero->cmap_index, charcode);
                if (gindex > 0)
                {
                    ft.pri_font = tfontZero;

                    //do face check when find out gindex with other font(not current)
                    ituFtFaceCheck(ft.pri_font);
                }
            }
        }
        else
        {
            //do face check when find out gindex with current font
            ituFtFaceCheck(ft.current_font);
        }
    }

    if (gindexMissed)
    {
        freetypeCacheLog("[FontCheck pos_%d]gindex %d [%s][%s]\n", pos, gindex, text, ft.default_font->filename);
    }

    return gindex;
}

void ituFtExit (void)
{
    int i;

    for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
    {
        if (ft.fonts[i].filepathname)
        {
            free((void *)ft.fonts[i].filepathname);
        }
    }

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if (ft.gid != NULL)
    {
        for (i = 0; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            if (ft.gid->count[i] > 0)
            {
                int j;
                for (j = 0; j < ft.gid->count[i]; j++)
                {
                    if (ft.gid->cidArr[i][j] != NULL)
                    {
                        free(ft.gid->cidArr[i][j]);
                        ft.gid->cidArr[i][j] = NULL;
                    }

                    if (ft.gid->gidArr[i][j] != NULL)
                    {
                        free(ft.gid->gidArr[i][j]);
                        ft.gid->gidArr[i][j] = NULL;
                    }
                }
            }
        }
        free(ft.gid);
    }
#endif

    FT_Bitmap_Done(ft.library, &ft.bitmap);
    FTC_Manager_Done(ft.cache_manager);
    FT_Done_FreeType(ft.library);

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if (ft.hbFont != NULL)
    {
        hb_font_destroy(ft.hbFont);
        ft.hbFont = NULL;
    }
#endif

    (void)memset(&ft, 0, sizeof(ft));
}

int ituFtLoadFont (int index, char * filename, ITUGlyphFormat format)
{
    FT_Error error;
    FT_Face  face;
    TFont *  font;

    assert(index >= 0);
    assert(index < ITU_FREETYPE_MAX_FONTS);
    assert(filename);

    if (index >= ITU_FREETYPE_MAX_FONTS)
    {
        ITU_LOG_ERR("out of font index: %d >= %d\n", index, ITU_FREETYPE_MAX_FONTS);
        return __LINE__;
    }

    font = &ft.fonts[index];

    if (font->array)
    {
        FTC_Manager_RemoveFaceID(ft.cache_manager, font->scaler.face_id);
        font->array = NULL;
    }
    else if (font->filepathname)
    {
        FTC_Manager_RemoveFaceID(ft.cache_manager, font->scaler.face_id);
        free((void *)font->filepathname);
        font->filepathname = NULL;
    }

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if (ft.hbFont != NULL)
    {
        hb_font_destroy(ft.hbFont);
        ft.hbFont = NULL;
    }
#endif

    error = FT_New_Face(ft.library, filename, 0, &face);
    if (error)
    {
        ITU_LOG_ERR("couldn't open this file: %s\n", filename);
        return error;
    }
    else
    {
#ifdef CFG_ITU_HARFBUZZ_SUPPORT
        ft.gid = (THarfbuzzGID*)malloc(sizeof(THarfbuzzGID));
        if (ft.gid != NULL)
        {
            memset(ft.gid, 0, sizeof(THarfbuzzGID));
        }
#endif
    }

    font->filepathname = (char *)malloc(strlen(filename) + 1);
    if (!font->filepathname)
    {
        ITU_LOG_ERR("out of memory: %d\n", (int)strlen(filename) + 1);
        return __LINE__;
    }

    strlcpy(font->filepathname, filename, strlen(filename) + 1);

    font->filename = strrchr(font->filepathname, '/');
    if (font->filename)
    {
        font->filename++;
    }
    else
    {
        font->filename = font->filepathname;
    }

    font->face_index  = 0;
    font->cmap_index  = face->charmap ? FT_Get_Charmap_Index(face->charmap) : 0;
    font->num_indices = 0x110000L;

    FT_Done_Face(face);
    face                 = NULL;

    font->scaler.face_id = (FTC_FaceID)font;

    switch (format)
    {
        case ITU_GLYPH_1BPP:
            font->load_flags = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            font->render_mode = FT_RENDER_MODE_MONO;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            font->load_flags  = FT_LOAD_TARGET_NORMAL;
            font->render_mode = FT_RENDER_MODE_NORMAL;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", format);
            return __LINE__;
    }
    font->format = format;

    if (!ft.current_font)
    {
        ft.current_font  = font;
        ft.current_index = index;
    }

    if (!ft.default_font)
    {
        ft.default_font = font;
    }

    return FT_Err_Ok;
}

int ituFtLoadFontArray (int index, uint8_t * array, int size, ITUGlyphFormat format)
{
    FT_Error error;
    FT_Face  face;
    TFont *  font;

    assert(index >= 0);
    assert(index < ITU_FREETYPE_MAX_FONTS);
    assert(array);
    assert(size > 0);

    if (index >= ITU_FREETYPE_MAX_FONTS)
    {
        ITU_LOG_ERR("out of font index: %d >= %d\n", index, ITU_FREETYPE_MAX_FONTS);
        return __LINE__;
    }

    font = &ft.fonts[index];

    if (font->array)
    {
        FTC_Manager_RemoveFaceID(ft.cache_manager, font->scaler.face_id);
        font->array = NULL;
    }
    else if (font->filepathname)
    {
        FTC_Manager_RemoveFaceID(ft.cache_manager, font->scaler.face_id);
        free((void *)font->filepathname);
        font->filepathname = NULL;
    }

    error = FT_New_Memory_Face(ft.library, array, size, 0, &face);
    if (error)
    {
        ITU_LOG_ERR("couldn't open font array: %d\n", error);
        return error;
    }

    font->array       = array;
    font->array_size  = size;
    font->face_index  = 0;
    font->cmap_index  = face->charmap ? FT_Get_Charmap_Index(face->charmap) : 0;
    font->num_indices = 0x110000L;

    FT_Done_Face(face);
    face                 = NULL;

    font->scaler.face_id = (FTC_FaceID)font;

    switch (format)
    {
        case ITU_GLYPH_1BPP:
            font->load_flags  = FT_LOAD_TARGET_MONO | FT_LOAD_RENDER;
            font->render_mode = FT_RENDER_MODE_MONO;
            break;

        case ITU_GLYPH_4BPP:
        case ITU_GLYPH_8BPP:
            font->load_flags  = FT_LOAD_TARGET_NORMAL;
            font->render_mode = FT_RENDER_MODE_NORMAL;
            break;

        default:
            ITU_LOG_ERR("unknown glyph format: %d\n", format);
            return __LINE__;
    }
    font->format = format;

    if (!ft.current_font)
    {
        ft.current_font  = font;
        ft.current_index = index;
    }

    if (!ft.default_font)
    {
        ft.default_font = font;
    }

    return FT_Err_Ok;
}

void ituFtSetHarfbuzzMode (int index, ITUFtHarfbuzzStyle hbstyle, bool enable)
{
    if ((index >= 0) && (index < ITU_FREETYPE_MAX_FONTS))
    {
        unsigned int style = ituFtGetFontExtraStyle(index);

        if (enable)
        {
            style |= ITU_FT_STYLE_HARFBUZZ;
        }
        else
        {
            style &= ~ITU_FT_STYLE_HARFBUZZ;
        }

        ituFtSetFontExtraStyle(index, style);

        ft.hbStyle[index] = hbstyle;
    }
}

ITUFtHarfbuzzStyle ituFtGetHarfbuzzStyle (int index)
{
    if ((index > 0) && (index < ITU_FREETYPE_MAX_FONTS))
    {
        return ft.hbStyle[index];
    }
    return ft.hbStyle[0];
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

    ft.current_font  = &ft.fonts[index];
    ft.current_index = index;
}

void ituFtSetFontSize (int width, int height)
{
    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        return;
    }
    else
    {
        int id = 0;
        ft.normal_size = height;
        for (; id < ITU_FREETYPE_MAX_FONTS; id++)
        {
            if (&ft.fonts[id] != NULL)
            {
                ft.fonts[id].scaler.width = (FT_UInt)ft.normal_size;
                ft.fonts[id].scaler.height = (FT_UInt)ft.normal_size;
            }
        }
        ft.current_font->scaler.width = (FT_UInt)width;
        ft.current_font->scaler.height = (FT_UInt)height;
        ft.current_font->scaler.pixel = 1;
    }
}

int ituFtDrawText (ITUSurface * surf, int x, int y, const char * text)
{
    FT_Error error = FT_Err_Ok;
    int      i, len, x_advance;
    wchar_t  buf[512];
    FT_UInt  prev_gindex = 0;

#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int      hid          = -1;
    int      widthCheck   = 0;
    int      heightCheck  = 0;
    ITUColor emptyColor   = {0, 255, 255, 255};
    bool     bAddSurfHash = false;
#endif

    assert(text);

    freetypeCacheLog("[ituFtDrawText][%d,%d][%s]\n", x, y, text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }
    else
    {
        //sync all font array
        int id = 0;
        for (; id < ITU_FREETYPE_MAX_FONTS; id++)
        {
            TFont* fontNode = &(ft.fonts[id]);
            if (fontNode != NULL)
            {
                ft.fonts[id].scaler.width = ft.normal_size;
                ft.fonts[id].scaler.height = ft.normal_size;
            }
        }
    }

    len       = mbstowcs(buf, text, 512);
    x_advance = 0;

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
    else if (ft.style & ITU_FT_STYLE_HEBREW)
    {
        wchar_t * ptr = convertHebrew(buf);
        if (ptr)
        {
            wcscpy(buf, ptr);
            free(ptr);
        }
    }
#endif

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if (ft.style_extra[ft.current_index] & ITU_FT_STYLE_HARFBUZZ)
    {
        int wc = 0;
        for (i = 0; i < len; i++)
        {
            int charcode = buf[i];
            int gindex = ituFtCheckGlyphIndex(&ft.current_font, charcode, text, i);
            if (gindex == 0)
            {
                //for each charcode of buf[len]
                //when return gindex 0 --> can't find out gindex in current font and priority font
                break;
            }
            if (ituFtChkHarfbuzzNN(charcode))
                wc++;
        }
        if (wc < len)
            return convertHarfBuzz(&ft, surf, x, y, text, 0);
    }
#endif

    // for BOLD Font Case
    if ((ft.style & ITU_FT_STYLE_BOLD) && ft.bold_size > 0)
    {
        int calBoldSize = ft.bold_size;
        calBoldSize     = (calBoldSize >= MAX_CAL_BOLD_SIZE) ? (MAX_CAL_BOLD_SIZE) : (calBoldSize);
        for (i = 0; i < len; i++)
        {
            int            charcode;
            FT_GlyphSlot   glyf;
            FT_Glyph       glyph;
            FT_Face        face   = ft.current_font->face;
            ITUGlyphFormat format = ITU_GLYPH_8BPP;
            FT_BitmapGlyph bitmapGlyph;
            FT_UInt        gindex;
            TFont *        tfont = ft.current_font;
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
                    if (ft.current_font == &ft.fonts[id])
                    {
                        bAddSurfHash = ituSceneFontArrHashAdd(ituScene, id, ft.bold_size, &buf[i], ft.current_font->scaler.height,
                                                              &hid, &surf->fgColor);
                        break;
                    }
                }
            }

            if ((hid >= 0) && (!bAddSurfHash))
            {
                ithFlushDCacheRange(ituScene->fontSurf[hid].surf, (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                ithFlushMemBuffer();

                x_advance += ituScene->fontSurf[hid].lb;
                freetypeCacheLog("[FontSurfCache][Reused-%d,%d][B] hid %d - Str[%lc]\n",
                                  x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y, hid,
                                  buf[i]);
                ituDrawGlyph(surf, x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y,
                             ituScene->fontSurf[hid].format, ituScene->fontSurf[hid].surf, ituScene->fontSurf[hid].fp,
                             ituScene->fontSurf[hid].fr);
                x_advance -= ituScene->fontSurf[hid].lb;
                x_advance += ituScene->fontSurf[hid].wrb;
                continue;
            }
#endif

            charcode = buf[i];
            gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, i);
            if (ft.pri_font != NULL)
            {
                tfont = ft.pri_font;
                face = tfont->face;
            }

            error    = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id, &face);
            if (error)
            {
                ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
                continue;
            }

            error = FT_Load_Char(face, charcode, ft.current_font->load_flags);
            if (error && face != ft.default_font->face)
            {
                face  = ft.default_font->face;
                error = FT_Load_Char(face, charcode, ft.default_font->load_flags);
            }
            if (error)
            {
                continue;
            }

            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                if (face->glyph->outline.flags > 16)
                {
                    face->glyph->outline.flags = 16;
                }

                // FT_Outline_Embolden(&face->glyph->outline, ft.bold_size * (((face->glyph->outline.flags >= 8) &&
                // (face->glyph->outline.flags < 64)) ? (face->glyph->outline.flags * 16) : (64)));
                // FT_Outline_Embolden(&face->glyph->outline, 1 * 64);
                if (ft.bold_size >= 1)
                {
                    FT_Outline_Embolden(&face->glyph->outline, calBoldSize);
                }
                else
                {
                    FT_Outline_Embolden(&face->glyph->outline, 0);
                }
            }
            else if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
            {
                FT_Bitmap_Embolden(ft.library, &face->glyph->bitmap, calBoldSize, calBoldSize);
            }

            glyf  = face->glyph;
            error = FT_Get_Glyph(glyf, &glyph);
            if (error)
            {
                continue;
            }

            FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
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
            }
            if (bitmapGlyph->bitmap.buffer)
            {
                int    xx, yy;
                FT_Pos xCal = 0;
                //for application usage
                //we use scaled default mode always
                //here marked for debug only
                /*
                if (FT_HAS_KERNING(face) && prev_gindex)
                {
                    FT_Vector delta;
                    FT_Get_Kerning(tfont->face, prev_gindex, gindex, FT_KERNING_UNSCALED, &delta);
                    xCal = delta.x >> 6;
                    x_advance += xCal;
                }
                */
                xx = x + x_advance + bitmapGlyph->left;
                yy = y + face->size->metrics.y_ppem - bitmapGlyph->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                if (hid >= 0)
                {
                    ituScene->fontSurf[hid].lb     = xCal + bitmapGlyph->left;
                    ituScene->fontSurf[hid].x      = 0;
                    ituScene->fontSurf[hid].wrb    = glyf->advance.x >> 6;
                    ituScene->fontSurf[hid].y      = yy - y;
                    ituScene->fontSurf[hid].fr     = bitmapGlyph->bitmap.rows;
                    ituScene->fontSurf[hid].fp     = bitmapGlyph->bitmap.width;
                    ituScene->fontSurf[hid].format = format;
                    ituScene->fontSurf[hid].surf =
                        (unsigned char *)malloc(ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp);
                    (void)memcpy(ituScene->fontSurf[hid].surf, bitmapGlyph->bitmap.buffer,
                           (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][B] hid %d - Str[%lc]\n", xx, yy, hid, buf[i]);
                }
#endif
                ituDrawGlyph(surf, xx, yy, format, bitmapGlyph->bitmap.buffer, bitmapGlyph->bitmap.width,
                             bitmapGlyph->bitmap.rows);
            }
            else
            {
                // ITU_LOG_ERR("FreeCache drawtext get no valid glyph map from bold outline!\n" );
            }
            x_advance += glyf->advance.x >> 6;
#ifdef ITU_ENABLE_DEFER_DESTROY
            ituAddDeferDestroyData(glyph, FT_Done_Glyph);
#else
            FT_Done_Glyph(glyph);
#endif
            prev_gindex = gindex;
        }
    }
    else // for Normal Font Case
    {
        for (i = 0; i < len; i++)
        {
            FT_UInt  gindex;
            int      charcode;
            TFont *  tfont = ft.current_font;
            FTC_SBit sbit;
            FT_Face  face;
            FTC_Node node;
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
                    if (ft.current_font == &ft.fonts[id])
                    {
                        bAddSurfHash = ituSceneFontArrHashAdd(ituScene, id, 0, &buf[i], ft.current_font->scaler.height,
                                                              &hid, &surf->fgColor);
                        break;
                    }
                }
            }

            if ((hid >= 0) && (!bAddSurfHash))
            {
                x_advance += ituScene->fontSurf[hid].lb;
                freetypeCacheLog("[FontSurfCache][Reused-%d,%d][N] hid %d - Str[%lc]\n",
                                  x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y, hid,
                                  buf[i]);

                ithFlushDCacheRange(ituScene->fontSurf[hid].surf, (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                ithFlushMemBuffer();

                if (ituScene->fontSurf[hid].y + y < 0)
                {
                    ITURectangle prevClip;
                    ituSetClipping(surf, ituScene->fontSurf[hid].x, 0, ituScene->fontSurf[hid].w,
                                   ituScene->fontSurf[hid].h, &prevClip);
                    ituDrawGlyph(surf, x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y,
                                 ituScene->fontSurf[hid].format, ituScene->fontSurf[hid].surf,
                                 ituScene->fontSurf[hid].fp, ituScene->fontSurf[hid].fr);
                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                }
                else
                {
                    ituDrawGlyph(surf, x_advance + ituScene->fontSurf[hid].x + x, ituScene->fontSurf[hid].y + y,
                                 ituScene->fontSurf[hid].format, ituScene->fontSurf[hid].surf,
                                 ituScene->fontSurf[hid].fp, ituScene->fontSurf[hid].fr);
                }

                x_advance += ituScene->fontSurf[hid].wrb;
                continue;
            }
#endif
            charcode = buf[i];

            gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, i);
            if (ft.pri_font != NULL)
            {
                tfont = ft.pri_font;
            }

            error    = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id, &face);
            if (error)
            {
                ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
                continue;
            }

            /* use the SBits cache to store small glyph bitmaps; this is a lot */
            /* more memory-efficient                                           */
            error = FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler, tfont->load_flags, gindex, &sbit, &node);

            // check in ftc_snode_compare
            //! sbit->buffer and sbit->width == 255 when freetype font check result is unavailable bitmap
            if ((tfont->scaler.width < MAX_SBIT_CACHE && tfont->scaler.height < MAX_SBIT_CACHE) && (sbit->buffer))
            {
                if (error)
                {
                    ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                    continue;
                }
                else
                {
                    FT_Size size;
                    FT_Pos  xCal = 0;
                    FTC_Manager_LookupSize(ft.cache_manager, &tfont->scaler, &size);

                    //for application usage
                    //we use scaled default mode always
                    //here marked for debug only
                    /*
                    if (FT_HAS_KERNING(face) && prev_gindex)
                    {
                        FT_Vector delta;
                        FT_Get_Kerning(tfont->face, prev_gindex, gindex, FT_KERNING_UNSCALED, &delta);
                        xCal = delta.x >> 6;
                        if (xCal != 0)
                        {
                            x_advance += xCal;
                        }
                    }
                    */

                    if ((tfont->format == ITU_GLYPH_4BPP) &&
                        (sbit->format == FT_PIXEL_MODE_GRAY || sbit->format == FT_PIXEL_MODE_GRAY2))
                    {
                        FT_Bitmap source;
                        int       xx, yy;

                        source.rows       = sbit->height;
                        source.width      = sbit->width;
                        source.pitch      = sbit->pitch;
                        source.buffer     = sbit->buffer;
                        source.pixel_mode = sbit->format;
                        Bitmap_Convert_GRAY4(ft.library, &source, &ft.bitmap);

                        ithFlushDCacheRange(ft.bitmap.buffer, ft.bitmap.rows * ft.bitmap.width);
                        ithFlushMemBuffer();
                        xx = x + x_advance + sbit->left;
                        yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                        if (hid >= 0)
                        {
                            ituScene->fontSurf[hid].lb     = xCal;
                            ituScene->fontSurf[hid].x      = sbit->left;
                            ituScene->fontSurf[hid].wrb    = sbit->xadvance;
                            ituScene->fontSurf[hid].y      = yy - y;
                            ituScene->fontSurf[hid].fr     = ft.bitmap.rows;
                            ituScene->fontSurf[hid].fp     = ft.bitmap.width;
                            ituScene->fontSurf[hid].format = ITU_GLYPH_4BPP;
                            ituScene->fontSurf[hid].surf   = (unsigned char *)malloc(ft.bitmap.rows * ft.bitmap.width);
                            (void)memcpy(ituScene->fontSurf[hid].surf, ft.bitmap.buffer, (ft.bitmap.rows * ft.bitmap.width));
                            freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy, hid,
                                              buf[i]);
                        }
#endif
                        if (yy < 0)
                        {
                            ITURectangle prevClip;
                            ituSetClipping(surf, xx, 0, ft.bitmap.width, ft.bitmap.rows, &prevClip);
                            ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                         ft.bitmap.rows);
                            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                        }
                        else
                        {
                            ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                         ft.bitmap.rows);
                        }
                    }
                    else
                    {
                        ITUGlyphFormat format = ITU_GLYPH_8BPP;
                        int            xx, yy;

                        switch (sbit->format)
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

                        ithFlushDCacheRange(sbit->buffer, sbit->width * sbit->height);
                        ithFlushMemBuffer();

                        xx = x + x_advance + sbit->left;
                        yy = y + tfont->face->size->metrics.y_ppem - sbit->top;

#ifdef CFG_ITU_FONT_SURFACE_CACHE
                        if (hid >= 0)
                        {
                            ituScene->fontSurf[hid].lb     = xCal;
                            ituScene->fontSurf[hid].x      = sbit->left;
                            ituScene->fontSurf[hid].wrb    = sbit->xadvance;
                            ituScene->fontSurf[hid].y      = yy - y;
                            ituScene->fontSurf[hid].fr     = sbit->height;
                            ituScene->fontSurf[hid].fp     = sbit->width;
                            ituScene->fontSurf[hid].format = format;
                            ituScene->fontSurf[hid].surf =
                                (unsigned char *)malloc(ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp);
                            (void)memcpy(ituScene->fontSurf[hid].surf, sbit->buffer,
                                   (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                            freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy, hid,
                                              buf[i]);
                        }
#endif
                        if (yy < 0)
                        {
                            ITURectangle prevClip;
                            ituSetClipping(surf, xx, 0, sbit->width, sbit->height, &prevClip);
                            ituDrawGlyph(surf, xx, yy, format, sbit->buffer, sbit->width, sbit->height);
                            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                        }
                        else
                        {
                            ituDrawGlyph(surf, xx, yy, format, sbit->buffer, sbit->width, sbit->height);
                        }
#ifdef ITU_ENABLE_DEFER_DESTROY
                        ituAddDeferDestroyData(node, _FreeFTCNode);
#else
                        FTC_Node_Unref(node, ft.cache_manager);
#endif
                    }
                }

                x_advance += sbit->xadvance;
            }
            else
            {
                /* otherwise, use an image cache to store glyph outlines, and render */
                /* them on demand. we can thus support very large sizes easily..     */
                FT_Glyph glyph = NULL;
                if (error)
                {
                    ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                    continue;
                }

                error = FTC_ImageCache_LookupScaler(ft.image_cache, &tfont->scaler, tfont->load_flags, gindex, &glyph,
                                                    NULL);
                if (error)
                {
                    ITU_LOG_ERR("FTC_ImageCache_LookupScaler fail: %d\n", error);
                    continue;
                }

                if (glyph)
                {
                    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
                    {
                        error = FT_Glyph_To_Bitmap(&glyph, tfont->render_mode, NULL, 0);
                        if (error)
                        {
                            ITU_LOG_ERR("FT_Glyph_To_Bitmap fail: %d\n", error);
                            FT_Done_Glyph(glyph);
                            continue;
                        }
                    }

                    if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
                    {
                        FT_BitmapGlyph bitmap = (FT_BitmapGlyph)glyph;
                        FT_Bitmap *    source = &bitmap->bitmap;

                        if (source->width > 0)
                        {
                            FT_Pos xCal = 0;
                            //for application usage
                            //we use scaled default mode always
                            //here marked for debug only
                            /*
                            if (FT_HAS_KERNING(face) && prev_gindex)
                            {
                                FT_Vector delta;
                                FT_Get_Kerning(tfont->face, prev_gindex, gindex, FT_KERNING_UNSCALED, &delta);
                                xCal = delta.x >> 6;
                                x_advance += xCal;
                            }
                            */

                            if ((tfont->format == ITU_GLYPH_4BPP) &&
                                (sbit->format == FT_PIXEL_MODE_GRAY || sbit->format == FT_PIXEL_MODE_GRAY2))
                            {
                                int xx, yy;

                                Bitmap_Convert_GRAY4(ft.library, source, &ft.bitmap);

                                ithFlushDCacheRange(ft.bitmap.buffer, ft.bitmap.rows * ft.bitmap.pitch);
                                ithFlushMemBuffer();

                                xx = x + x_advance + sbit->left;
                                yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                                if (hid >= 0)
                                {
                                    ituScene->fontSurf[hid].lb     = xCal;
                                    ituScene->fontSurf[hid].x      = sbit->left;
                                    ituScene->fontSurf[hid].wrb    = (glyph->advance.x + 0x8000) >> 16;
                                    ituScene->fontSurf[hid].y      = yy - y;
                                    ituScene->fontSurf[hid].fr     = ft.bitmap.rows;
                                    ituScene->fontSurf[hid].fp     = ft.bitmap.pitch;
                                    ituScene->fontSurf[hid].format = ITU_GLYPH_4BPP;
                                    ituScene->fontSurf[hid].surf =
                                        (unsigned char *)malloc(ft.bitmap.rows * ft.bitmap.pitch);
                                    (void)memcpy(ituScene->fontSurf[hid].surf, ft.bitmap.buffer,
                                           (ft.bitmap.rows * ft.bitmap.pitch));
                                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy,
                                                      hid, buf[i]);
                                }
#endif
                                if (yy < 0)
                                {
                                    ITURectangle prevClip;
                                    ituSetClipping(surf, xx, 0, ft.bitmap.width, ft.bitmap.rows, &prevClip);
                                    ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                                 ft.bitmap.rows);
                                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width,
                                                          prevClip.height);
                                }
                                else
                                {
                                    ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                                 ft.bitmap.rows);
                                }
                            }
                            else
                            {
                                ITUGlyphFormat format;
                                int            xx, yy;

                                ithFlushDCacheRange(source->buffer, source->rows * source->pitch);
                                ithFlushMemBuffer();

                                switch (sbit->format)
                                {
                                    case FT_PIXEL_MODE_MONO:
                                        format = ITU_GLYPH_1BPP;
                                        break;

                                    case FT_PIXEL_MODE_GRAY4:
                                        format = ITU_GLYPH_4BPP;
                                        break;

                                    case FT_PIXEL_MODE_GRAY:
                                    default:
                                        format = ITU_GLYPH_8BPP;
                                        break;
                                }
                                xx = x + x_advance + bitmap->left;
                                yy = y + tfont->face->size->metrics.y_ppem - bitmap->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                                if (hid >= 0)
                                {
                                    ituScene->fontSurf[hid].lb     = xCal;
                                    ituScene->fontSurf[hid].x      = bitmap->left;
                                    ituScene->fontSurf[hid].wrb    = (glyph->advance.x + 0x8000) >> 16;
                                    ituScene->fontSurf[hid].y      = yy - y;
                                    ituScene->fontSurf[hid].fr     = source->rows;
                                    ituScene->fontSurf[hid].fp     = source->pitch;
                                    ituScene->fontSurf[hid].format = format;
                                    ituScene->fontSurf[hid].surf =
                                        (unsigned char *)malloc(source->rows * source->pitch);
                                    (void)memcpy(ituScene->fontSurf[hid].surf, source->buffer,
                                           (source->rows * source->pitch));
                                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy,
                                                      hid, buf[i]);
                                }
#endif
                                if (yy < 0)
                                {
                                    ITURectangle prevClip;
                                    ituSetClipping(surf, xx, 0, source->width, source->rows, &prevClip);
                                    ituDrawGlyph(surf, xx, yy, format, source->buffer, source->width, source->rows);
                                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width,
                                                          prevClip.height);
                                }
                                else
                                {
                                    ituDrawGlyph(surf, xx, yy, format, source->buffer, source->width, source->rows);
                                }
                            }
                        }
                    }
                    else
                    {
                        ITU_LOG_ERR("invalid glyph format returned: %d\n", glyph->format);
                        FT_Done_Glyph(glyph);
                        goto end;
                    }
                    x_advance += (glyph->advance.x + 0x8000) >> 16;

#ifdef ITU_ENABLE_DEFER_DESTROY
                    ituAddDeferDestroyData(glyph, FT_Done_Glyph);
#else
                    FT_Done_Glyph(glyph);
#endif
                }
            }
            prev_gindex = gindex;
        }
    }
end:
    return error;
}

void ituFtResetCache (void)
{
    FTC_Manager_Reset(ft.cache_manager);
}

int ituFtGetTextDimension (const char * text, int * width, int * height, void* widget, int boldsize)
{
    int          lsp = 0; //used for letter space working
    int          i, len, x_advance;
    int          max_bottom_shift = 0;
    wchar_t      buf[512]         = L"";
    unsigned int max_height       = 0;
    FT_UInt      prev_gindex      = 0;
    FT_Error     error            = FT_Err_Ok;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int  hid    = 0;
    int  id     = 0;
    bool cached = false;
    assert(ituScene);
    for (; id < ITU_FREETYPE_MAX_FONTS; id++)
    {
        if (ft.current_font == &ft.fonts[id])
        {
            break;
        }
    }
    if (id == ITU_FREETYPE_MAX_FONTS)
    {
        id = 0;
    }
#endif
    assert(text);

    if (!ft.current_font)
    {
        freetypeCacheLog("current font not exist!\n");
        return error;
    }

    len        = mbstowcs(buf, text, strlen(text));
    x_advance  = 0;
    max_height = 0;

    if (widget != NULL)
    {
        ITUWidget* objWidget = (ITUWidget*)widget;
        if (objWidget->type == ITU_TEXT)
        {
            ITUText* txt = (ITUText*)objWidget;
            if (len > 1)
            {
                lsp = txt->letterSpacing * (len - 1);
            }
        }
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
    else if (ft.style & ITU_FT_STYLE_HEBREW)
    {
        wchar_t * ptr = convertHebrew(buf);
        if (ptr)
        {
            wcscpy(buf, ptr);
            free(ptr);
        }
    }
#endif

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if (ft.style_extra[ft.current_index] & ITU_FT_STYLE_HARFBUZZ)
    {
        int wc = 0, cx = 0, cy = 0;
        bool checkFace = false;
        TFont* tfont = ft.current_font;

        for (i = 0; i < len; i++)
        {
            int charcode = buf[i];
            int gindex = ituFtCheckGlyphIndex(&tfont, charcode, text, i);
            if (gindex == 0)
            {
                //for each charcode of buf[len]
                //when return gindex 0 --> can't find out gindex in current font and priority font
                break;
            }
            if (ituFtChkHarfbuzzNN(charcode))
                wc++;
        }
        
        if (wc < len)
        {
            max_bottom_shift = getSizeHarfBuzz(&ft, &cx, &cy, text, 0);

            if (abs(lsp * 2) > cx)
            {
                lsp = (lsp > 0) ? (cx / 2) : (-cx / 2);
                if (((len - 1) > 0) && (widget != NULL))
                {
                    ITUText* txt = (ITUText*)widget;
                    txt->letterSpacing = (lsp / (len - 1));
                }
            }

            if (width)
            {
                *width = cx + (lsp);
            }
            if (height)
            {
                *height = cy;
            }
            freetypeCacheLog("[HarfBuzz][w,h  %d,%d][%s]\n", cx, cy, text);
            return max_bottom_shift;
        }
    }
#endif

    for (i = 0; i < len; i++)
    {
        FT_UInt      gindex;
        int          charcode;
        TFont *      tfont = ft.current_font;
        FTC_SBit     sbit;
        FT_Face      face;
        unsigned int h;

#ifdef CFG_ITU_FONT_SURFACE_CACHE
        bool cacheAndFound = false;
        cached             = ituSceneFontArrHashSizeCheck(
            ituScene, id, boldsize, &buf[i],
            ft.current_font->scaler.height, &hid);
        if (cached)
        {
            if ((ituScene->fontSurf[hid].h != 0) && (ituScene->fontSurf[hid].wrb != 0))
            {
                cacheAndFound = true;
                x_advance += ituScene->fontSurf[hid].wrb;

                if (max_height < ituScene->fontSurf[hid].h)
                {
                    max_height = ituScene->fontSurf[hid].h;
                }

                if (tfont->scaler.width < MAX_SBIT_CACHE && tfont->scaler.height < MAX_SBIT_CACHE)
                {
                    if (max_bottom_shift < ituScene->fontSurf[hid].bottomShift)
                    {
                        max_bottom_shift = ituScene->fontSurf[hid].bottomShift;
                    }
                }
            }
        }

        if (cacheAndFound)
        {
            freetypeCacheLog("[C][wrb %d][h %d][hid %d][%lc]\n", ituScene->fontSurf[hid].wrb, ituScene->fontSurf[hid].h, hid, buf[i]);
            continue;
        }
#endif

        charcode = buf[i];
        gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, i);
        if (ft.pri_font != NULL)
        {
            tfont = ft.pri_font;
        }

        error = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id,
                                       &face);
        if (error)
        {
            freetypeCacheLog("FTC_Manager_LookupFace fail: %d\n", error);
            continue;
        }

        error = FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler,
                                       tfont->load_flags, gindex, &sbit, NULL);
        if (error)
        {
            freetypeCacheLog("FTC_SBitCache_LookupScaler fail: %d\n", error);
            continue;
        }

        /* use the SBits cache to store small glyph bitmaps; this is a lot */
        /* more memory-efficient                                           */

        if (tfont->scaler.width < MAX_SBIT_CACHE && tfont->scaler.height < MAX_SBIT_CACHE)
        {
            FT_Size size;

            FTC_Manager_LookupSize(ft.cache_manager, &tfont->scaler, &size);

            h = (face->size->metrics.ascender - face->size->metrics.descender) >> 6;
            x_advance += sbit->xadvance;

            if (max_height < h)
            {
                max_height = h;
            }
            if (max_bottom_shift < (sbit->height - sbit->top))
            {
                max_bottom_shift = (sbit->height - sbit->top);
            }

#ifdef CFG_ITU_FONT_SURFACE_CACHE

            if (!cached)
            {
                ituScene->fontSurf[hid].wrb         = sbit->xadvance;
                ituScene->fontSurf[hid].h           = h;
                ituScene->fontSurf[hid].bottomShift = sbit->height - sbit->top;
                freetypeCacheLog("[N][wrb %d][h %d][hid %d][%lc]\n", ituScene->fontSurf[hid].wrb, ituScene->fontSurf[hid].h, hid, buf[i]);
            }
#endif
        }
        else
        {
            /* otherwise, use an image cache to store glyph outlines, and render
             */
            /* them on demand. we can thus support very large sizes easily.. */
            FT_Glyph glyph;

            error = FTC_ImageCache_LookupScaler(ft.image_cache, &tfont->scaler,
                                                tfont->load_flags, gindex,
                                                &glyph, NULL);
            if (error != FT_Err_Ok)
            {
                freetypeCacheLog("FTC_ImageCache_LookupScaler fail: %d\n", error);
                continue;
            }

            if (glyph != NULL)
            {
                if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
                {
                    error = FT_Glyph_To_Bitmap(&glyph, tfont->render_mode, NULL, 0);
                    if (error != FT_Err_Ok)
                    {
                        freetypeCacheLog("FT_Glyph_To_Bitmap fail: %d\n", error);
                        continue;
                    }
                }

                h = tfont->face->size->metrics.height >> 6;
                if (max_height < h)
                {
                    max_height = h;
                }

                x_advance += (glyph->advance.x + 0x8000) >> 16;

#ifdef CFG_ITU_FONT_SURFACE_CACHE
                if (!cached)
                {
                    ituScene->fontSurf[hid].wrb         = (glyph->advance.x + 0x8000) >> 16;
                    ituScene->fontSurf[hid].h           = h;
                    ituScene->fontSurf[hid].bottomShift = 0;
                }
#endif
                FT_Done_Glyph(glyph);
            }
        }
        prev_gindex = gindex;
    }

    if (abs(lsp * 2) > x_advance)
    {
        lsp = (lsp > 0) ? (x_advance / 2) : (-x_advance / 2);
        if (((len - 1) > 0) && (widget != NULL))
        {
            ITUText* txt = (ITUText*)widget;
            txt->letterSpacing = (lsp / (len - 1));
        }
    }

    if (width)
    {
        *width = x_advance + lsp;
    }

    if (height)
    {
        *height = max_height;
    }
    freetypeCacheLog("[ituFtGetTextDimension][%d,%d][%s]\n", x_advance, max_height, text);
    return max_bottom_shift;
}

int ituFtGetCharWidth (const char * text, int * width, int boldsize)
{
    FT_Error error = FT_Err_Ok;
    int      len   = 0;
    wchar_t  buf[512] = L"";
#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int  hid    = 0;
    bool cached = false;
    assert(ituScene);
#endif
    assert(text);
    assert(width);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }

    len = strlen(text);
    len = mbtowc(buf, text, len);

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
    if ((len > 0) && (ft.style_extra[ft.current_index] & ITU_FT_STYLE_HARFBUZZ))
    {
        int cx = 0, cy = 0, max_bottom_shift = 0;
        int      charcode = buf[0];

        if (!ituFtChkHarfbuzzNN(charcode))
        {
            FT_UInt gindex = ituFtCheckGlyphIndex(&ft.current_font, charcode, text, 0);

            max_bottom_shift = getSizeHarfBuzz(&ft, &cx, &cy, text, len);

            if (width)
            {
                *width = cx;
            }

            return len;
        }
    }
#endif

    if (len > 0)
    {
        FT_UInt  gindex;
        int      charcode;
        TFont *  tfont = ft.current_font;
        FTC_SBit sbit;
        FT_Face  face;

#ifdef CFG_ITU_FONT_SURFACE_CACHE
        int id = 0;
        for (; id < ITU_FREETYPE_MAX_FONTS; id++)
        {
            if (ft.current_font == &ft.fonts[id])
            {
                cached = ituSceneFontArrHashSizeCheck(ituScene, id, boldsize,
                                                      &buf[0], ft.current_font->scaler.height, &hid);
                if (cached)
                {
                    if ((ituScene->fontSurf[hid].surf) && ((ituScene->fontSurf[hid].w * ituScene->fontSurf[hid].h) > 0))
                    {
                        *width = ituScene->fontSurf[hid].wrb;
                        goto end;
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
            }
        }
#endif

        charcode = buf[0];
        gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, 0);
        if (ft.pri_font != NULL)
        {
            tfont = ft.pri_font;
        }

        error    = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id, &face);
        if (error)
        {
            ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
            goto end;
        }

        /* use the SBits cache to store small glyph bitmaps; this is a lot */
        /* more memory-efficient                                           */

        if (tfont->scaler.width < MAX_SBIT_CACHE && tfont->scaler.height < MAX_SBIT_CACHE)
        {
            error = FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler, tfont->load_flags, gindex, &sbit, NULL);
            if (error)
            {
                ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                goto end;
            }

            *width = sbit->xadvance;
        }
        else
        {
            /* otherwise, use an image cache to store glyph outlines, and render */
            /* them on demand. we can thus support very large sizes easily..     */
            FT_Glyph glyph;

            error = FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler, tfont->load_flags, gindex, &sbit, NULL);
            if (error)
            {
                ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                goto end;
            }

            error =
                FTC_ImageCache_LookupScaler(ft.image_cache, &tfont->scaler, tfont->load_flags, gindex, &glyph, NULL);
            if (error)
            {
                ITU_LOG_ERR("FTC_ImageCache_LookupScaler fail: %d\n", error);
                goto end;
            }

            if (glyph)
            {
                if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
                {
                    error = FT_Glyph_To_Bitmap(&glyph, tfont->render_mode, NULL, 0);
                    if (error)
                    {
                        ITU_LOG_ERR("FT_Glyph_To_Bitmap fail: %d\n", error);
                        goto end;
                    }
                }

                *width = (glyph->advance.x + 0x8000) >> 16;

                FT_Done_Glyph(glyph);
            }
        }
    }

end:
    return len;
}

int ituFtDrawChar (ITUSurface * surf, int x, int y, const char * text)
{
    FT_Error error = FT_Err_Ok;
    int      len;
    wchar_t  buf[512] = L"";

#ifdef CFG_ITU_FONT_SURFACE_CACHE
    int      hid          = -1;
    int      widthCheck   = 0;
    int      heightCheck  = 0;
    ITUColor emptyColor   = {0, 255, 255, 255};
    bool     bAddSurfHash = false;
#endif

    assert(text);

    if (!ft.current_font)
    {
        ITU_LOG_ERR("current font not exist\n");
        goto end;
    }
    else
    {
        //sync all font array
        int id = 0;
        for (; id < ITU_FREETYPE_MAX_FONTS; id++)
        {
            TFont* fontNode = &(ft.fonts[id]);
            if (fontNode != NULL)
            {
                ft.fonts[id].scaler.width = ft.normal_size;
                ft.fonts[id].scaler.height = ft.normal_size;
            }
        }
    }

    // buf[0] = '\0';
    len = strlen(text);
    len = mbtowc(buf, text, len);
    freetypeCacheLog("[ituFtDrawChar][%d,%d][%s]\n", x, y, text);

    if (len > 0)
    {

#ifdef CFG_ITU_HARFBUZZ_SUPPORT
        if (ft.style_extra[ft.current_index] & ITU_FT_STYLE_HARFBUZZ)
        {
            int charcode = buf[0];
            if (!ituFtChkHarfbuzzNN(charcode))
            {
                ituFtCheckGlyphIndex(&ft.current_font, charcode, text, 0);
                return convertHarfBuzz(&ft, surf, x, y, text, len);
            }
        }
#endif

        if ((ft.style & ITU_FT_STYLE_BOLD) && ft.bold_size > 0)
        {
            int            charcode;
            FT_GlyphSlot   glyf;
            FT_Glyph       glyph;
            FT_Face        face   = ft.current_font->face;
            ITUGlyphFormat format = ITU_GLYPH_8BPP;
            FT_BitmapGlyph bitmapGlyph;
            FT_UInt        gindex;
            TFont *        tfont       = ft.current_font;
            int            calBoldSize = ft.bold_size;
            calBoldSize                = (calBoldSize >= MAX_CAL_BOLD_SIZE) ? (MAX_CAL_BOLD_SIZE) : (calBoldSize);
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
                    if (ft.current_font == &ft.fonts[id])
                    {
                        bAddSurfHash = ituSceneFontArrHashAdd(ituScene, id, ft.bold_size, &buf[0], ft.current_font->scaler.height,
                                                              &hid, &surf->fgColor);
                        break;
                    }
                }
            }

            if ((hid >= 0) && (!bAddSurfHash))
			{
                ithFlushDCacheRange(ituScene->fontSurf[hid].surf, (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                ithFlushMemBuffer();

                freetypeCacheLog("[FontSurfCache][Reused-%d,%d][B] hid %d - Str[%lc]\n",
                                  ituScene->fontSurf[hid].lb + ituScene->fontSurf[hid].x + x,
                                  ituScene->fontSurf[hid].y + y, hid, buf[0]);
                ituDrawGlyph(surf, ituScene->fontSurf[hid].lb + ituScene->fontSurf[hid].x + x,
                             ituScene->fontSurf[hid].y + y, ituScene->fontSurf[hid].format,
                             ituScene->fontSurf[hid].surf, ituScene->fontSurf[hid].fp, ituScene->fontSurf[hid].fr);
                error = FT_Err_Ok;
                goto end;
            }

#endif

            charcode = buf[0];
            gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, 0);
            if (ft.pri_font != NULL)
            {
                tfont = ft.pri_font;
            }

            error    = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id, &face);
            if (error)
            {
                ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d for gindex %d\n", error, gindex);
            }

            error = FT_Load_Char(face, charcode, ft.current_font->load_flags);
            if (error && face != ft.default_font->face)
            {
                face  = ft.default_font->face;
                error = FT_Load_Char(face, charcode, ft.default_font->load_flags);
            }
            if (error)
            {
                ITU_LOG_ERR("FT_Load_Char fail: %d for gindex %d\n", error, gindex);
                goto end;
            }

            if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                if (ft.bold_size >= 1)
                {
                    FT_Outline_Embolden(&face->glyph->outline, calBoldSize);
                }
                else
                {
                    FT_Outline_Embolden(&face->glyph->outline, 0);
                }
            }
            else if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
            {
                FT_Bitmap_Embolden(ft.library, &face->glyph->bitmap, calBoldSize, calBoldSize);
            }

            glyf  = face->glyph;
            error = FT_Get_Glyph(glyf, &glyph);
            if (error)
            {
                ITU_LOG_ERR("FT_Get_Glyph fail: %d\n", error);
                goto end;
            }

            FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
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
            }

            if (bitmapGlyph->bitmap.buffer)
            {
                int xx, yy;
                xx = x;
                yy = y + face->size->metrics.y_ppem - bitmapGlyph->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                if (hid >= 0)
                {
                    ituScene->fontSurf[hid].lb     = 0;
                    ituScene->fontSurf[hid].x      = 0;
                    ituScene->fontSurf[hid].y      = yy - y;
                    ituScene->fontSurf[hid].fr     = bitmapGlyph->bitmap.rows;
                    ituScene->fontSurf[hid].fp     = bitmapGlyph->bitmap.width;
                    ituScene->fontSurf[hid].format = format;
                    ituScene->fontSurf[hid].surf =
                        (unsigned char *)malloc(ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr);
                    (void)memcpy(ituScene->fontSurf[hid].surf, bitmapGlyph->bitmap.buffer,
                           (ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr));
                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][B] hid %d - Str[%lc]\n", xx, yy, hid, buf[0]);
                }
#endif
                ituDrawGlyph(surf, xx, yy, format, bitmapGlyph->bitmap.buffer, bitmapGlyph->bitmap.width,
                             bitmapGlyph->bitmap.rows);
            }
            else
            {
                ITU_LOG_ERR("FreeCache drawchar get no valid glyph map from bold outline!\n");
            }
#ifdef ITU_ENABLE_DEFER_DESTROY
            ituAddDeferDestroyData(glyph, FT_Done_Glyph);
#else
            FT_Done_Glyph(glyph);
#endif
        }
        else
        {
            FT_UInt  gindex;
            int      charcode;
            TFont *  tfont = ft.current_font;
            FTC_SBit sbit;
            FT_Face  face;
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
                    if (ft.current_font == &ft.fonts[id])
                    {
                        bAddSurfHash = ituSceneFontArrHashAdd(ituScene, id, 0, &buf[0], ft.current_font->scaler.height,
                                                              &hid, &surf->fgColor);
                        break;
                    }
                }
            }

            if ((hid >= 0) && (!bAddSurfHash))
            {
                freetypeCacheLog("[FontSurfCache][Reused-%d,%d][N] hid %d - Str[%lc]\n",
                                  ituScene->fontSurf[hid].lb + ituScene->fontSurf[hid].x + x,
                                  ituScene->fontSurf[hid].y + y, hid, buf[0]);

                ithFlushDCacheRange(ituScene->fontSurf[hid].surf, (ituScene->fontSurf[hid].fr * ituScene->fontSurf[hid].fp));
                ithFlushMemBuffer();

                if (ituScene->fontSurf[hid].y + y < 0)
                {
                    ITURectangle prevClip;
                    ituSetClipping(surf, ituScene->fontSurf[hid].x, 0, ituScene->fontSurf[hid].w,
                                   ituScene->fontSurf[hid].h, &prevClip);
                    ituDrawGlyph(surf, ituScene->fontSurf[hid].lb + ituScene->fontSurf[hid].x + x,
                                 ituScene->fontSurf[hid].y + y, ituScene->fontSurf[hid].format,
                                 ituScene->fontSurf[hid].surf, ituScene->fontSurf[hid].fp, ituScene->fontSurf[hid].fr);
                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                }
                else
                {
                    ituDrawGlyph(surf, ituScene->fontSurf[hid].lb + ituScene->fontSurf[hid].x + x,
                                 ituScene->fontSurf[hid].y + y, ituScene->fontSurf[hid].format,
                                 ituScene->fontSurf[hid].surf, ituScene->fontSurf[hid].fp, ituScene->fontSurf[hid].fr);
                }
                error = FT_Err_Ok;
                goto end;
            }
#endif

            charcode = buf[0];
            gindex   = ituFtCheckGlyphIndex(&tfont, charcode, text, 0);
            if (ft.pri_font != NULL)
            {
                tfont = ft.pri_font;
            }

            error    = FTC_Manager_LookupFace(ft.cache_manager, tfont->scaler.face_id, &face);
            if (error)
            {
                ITU_LOG_ERR("FTC_Manager_LookupFace fail: %d\n", error);
                goto end;
            }

            /* use the SBits cache to store small glyph bitmaps; this is a lot */
            /* more memory-efficient                                           */

            if (tfont->scaler.width < MAX_SBIT_CACHE && tfont->scaler.height < MAX_SBIT_CACHE)
            {
                error =
                    FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler, tfont->load_flags, gindex, &sbit, NULL);
                if (error)
                {
                    ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                    goto end;
                }

                if (sbit->buffer)
                {
                    FT_Size size;
                    FTC_Manager_LookupSize(ft.cache_manager, &tfont->scaler, &size);

                    if ((tfont->format == ITU_GLYPH_4BPP) &&
                        (sbit->format == FT_PIXEL_MODE_GRAY || sbit->format == FT_PIXEL_MODE_GRAY2))
                    {
                        FT_Bitmap source;
                        int       xx, yy;

                        source.rows       = sbit->height;
                        source.width      = sbit->width;
                        source.pitch      = sbit->pitch;
                        source.buffer     = sbit->buffer;
                        source.pixel_mode = sbit->format;
                        Bitmap_Convert_GRAY4(ft.library, &source, &ft.bitmap);

                        ithFlushDCacheRange(ft.bitmap.buffer, ft.bitmap.rows * ft.bitmap.pitch);
                        ithFlushMemBuffer();

                        xx = x + sbit->left;
                        yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                        if (hid >= 0)
                        {
                            ituScene->fontSurf[hid].lb     = 0;
                            ituScene->fontSurf[hid].x      = sbit->left;
                            ituScene->fontSurf[hid].y      = yy - y;
                            ituScene->fontSurf[hid].fr     = ft.bitmap.rows;
                            ituScene->fontSurf[hid].fp     = ft.bitmap.width;
                            ituScene->fontSurf[hid].format = ITU_GLYPH_4BPP;
                            ituScene->fontSurf[hid].surf =
                                (unsigned char *)malloc(ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr);
                            (void)memcpy(ituScene->fontSurf[hid].surf, ft.bitmap.buffer,
                                   (ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr));
                            freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy, hid,
                                              buf[0]);
                        }
#endif
                        if (yy < 0)
                        {
                            ITURectangle prevClip;
                            ituSetClipping(surf, xx, 0, ft.bitmap.width, ft.bitmap.rows, &prevClip);
                            ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                         ft.bitmap.rows);
                            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                        }
                        else
                        {
                            ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                         ft.bitmap.rows);
                        }
                    }
                    else
                    {
                        ITUGlyphFormat format = ITU_GLYPH_8BPP;
                        int            xx, yy;

                        switch (sbit->format)
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

                        ithFlushDCacheRange(sbit->buffer, sbit->pitch * sbit->height);
                        ithFlushMemBuffer();

                        xx = x + sbit->left;
                        yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                        if (hid >= 0)
                        {
                            ituScene->fontSurf[hid].lb     = 0;
                            ituScene->fontSurf[hid].x      = sbit->left;
                            ituScene->fontSurf[hid].y      = yy - y;
                            ituScene->fontSurf[hid].fr     = sbit->height;
                            ituScene->fontSurf[hid].fp     = sbit->width;
                            ituScene->fontSurf[hid].format = format;
                            ituScene->fontSurf[hid].surf =
                                (unsigned char *)malloc(ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr);
                            (void)memcpy(ituScene->fontSurf[hid].surf, sbit->buffer,
                                   (ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr));
                            freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy, hid,
                                              buf[0]);
                        }
#endif
                        if (yy < 0)
                        {
                            ITURectangle prevClip;
                            ituSetClipping(surf, xx, 0, sbit->width, sbit->height, &prevClip);
                            ituDrawGlyph(surf, xx, yy, format, sbit->buffer, sbit->width, sbit->height);
                            ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
                        }
                        else
                        {
                            ituDrawGlyph(surf, xx, yy, format, sbit->buffer, sbit->width, sbit->height);
                        }
                    }
                }
            }
            else
            {
                /* otherwise, use an image cache to store glyph outlines, and render */
                /* them on demand. we can thus support very large sizes easily..     */
                FT_Glyph glyph = NULL;

                error =
                    FTC_SBitCache_LookupScaler(ft.sbits_cache, &tfont->scaler, tfont->load_flags, gindex, &sbit, NULL);
                if (error)
                {
                    ITU_LOG_ERR("FTC_SBitCache_LookupScaler fail: %d\n", error);
                    goto end;
                }

                error = FTC_ImageCache_LookupScaler(ft.image_cache, &tfont->scaler, tfont->load_flags, gindex, &glyph,
                                                    NULL);
                if (error)
                {
                    ITU_LOG_ERR("FTC_ImageCache_LookupScaler fail: %d\n", error);
                    goto end;
                }

                if (glyph)
                {
                    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
                    {
                        error = FT_Glyph_To_Bitmap(&glyph, tfont->render_mode, NULL, 0);
                        if (error)
                        {
                            ITU_LOG_ERR("FT_Glyph_To_Bitmap fail: %d\n", error);
                            FT_Done_Glyph(glyph);
                            goto end;
                        }
                    }

                    if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
                    {
                        FT_BitmapGlyph bitmap = (FT_BitmapGlyph)glyph;
                        FT_Bitmap *    source = &bitmap->bitmap;

                        if (source->width > 0)
                        {
                            if ((tfont->format == ITU_GLYPH_4BPP) &&
                                (sbit->format == FT_PIXEL_MODE_GRAY || sbit->format == FT_PIXEL_MODE_GRAY2))
                            {
                                int xx, yy;

                                Bitmap_Convert_GRAY4(ft.library, source, &ft.bitmap);

                                ithFlushDCacheRange(ft.bitmap.buffer, ft.bitmap.rows * ft.bitmap.pitch);
                                ithFlushMemBuffer();

                                xx = x + sbit->left;
                                yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                                if (hid >= 0)
                                {
                                    ituScene->fontSurf[hid].lb     = 0;
                                    ituScene->fontSurf[hid].x      = sbit->left;
                                    ituScene->fontSurf[hid].y      = yy - y;
                                    ituScene->fontSurf[hid].fr     = ft.bitmap.rows;
                                    ituScene->fontSurf[hid].fp     = ft.bitmap.width;
                                    ituScene->fontSurf[hid].format = ITU_GLYPH_4BPP;
                                    ituScene->fontSurf[hid].surf = (unsigned char *)malloc(ituScene->fontSurf[hid].fp *
                                                                                           ituScene->fontSurf[hid].fr);
                                    (void)memcpy(ituScene->fontSurf[hid].surf, ft.bitmap.buffer,
                                           (ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr));
                                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy,
                                                      hid, buf[0]);
                                }
#endif
                                if (yy < 0)
                                {
                                    ITURectangle prevClip;
                                    ituSetClipping(surf, xx, 0, ft.bitmap.width, ft.bitmap.rows, &prevClip);
                                    ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                                 ft.bitmap.rows);
                                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width,
                                                          prevClip.height);
                                }
                                else
                                {
                                    ituDrawGlyph(surf, xx, yy, ITU_GLYPH_4BPP, ft.bitmap.buffer, ft.bitmap.width,
                                                 ft.bitmap.rows);
                                }
                            }
                            else
                            {
                                ITUGlyphFormat format;
                                int            xx, yy;
                                switch (sbit->format)
                                {
                                    case FT_PIXEL_MODE_MONO:
                                        format = ITU_GLYPH_1BPP;
                                        break;

                                    case FT_PIXEL_MODE_GRAY4:
                                        format = ITU_GLYPH_4BPP;
                                        break;

                                    case FT_PIXEL_MODE_GRAY:
                                    default:
                                        format = ITU_GLYPH_8BPP;
                                        break;
                                }

                                ithFlushDCacheRange(source->buffer, source->pitch * source->rows);
                                ithFlushMemBuffer();

                                xx = x + bitmap->left;
                                yy = y + tfont->face->size->metrics.y_ppem - sbit->top;
#ifdef CFG_ITU_FONT_SURFACE_CACHE
                                if (hid >= 0)
                                {
                                    ituScene->fontSurf[hid].lb     = 0;
                                    ituScene->fontSurf[hid].x      = bitmap->left;
                                    ituScene->fontSurf[hid].y      = yy - y;
                                    ituScene->fontSurf[hid].fr     = source->rows;
                                    ituScene->fontSurf[hid].fp     = source->width;
                                    ituScene->fontSurf[hid].format = format;
                                    ituScene->fontSurf[hid].surf = (unsigned char *)malloc(ituScene->fontSurf[hid].fp *
                                                                                           ituScene->fontSurf[hid].fr);
                                    (void)memcpy(ituScene->fontSurf[hid].surf, source->buffer,
                                           (ituScene->fontSurf[hid].fp * ituScene->fontSurf[hid].fr));
                                    freetypeCacheLog("[FontSurfCache][Create-%d,%d][N] hid %d - Str[%lc]\n", xx, yy,
                                                      hid, buf[0]);
                                }
#endif
                                if (yy < 0)
                                {
                                    ITURectangle prevClip;
                                    ituSetClipping(surf, xx, 0, source->width, source->rows, &prevClip);
                                    ituDrawGlyph(surf, xx, yy, format, source->buffer, source->width, source->rows);
                                    ituSurfaceSetClipping(surf, prevClip.x, prevClip.y, prevClip.width,
                                                          prevClip.height);
                                }
                                else
                                {
                                    ituDrawGlyph(surf, xx, yy, format, source->buffer, source->width, source->rows);
                                }
                            }
                        }
                    }
                    else
                    {
                        ITU_LOG_ERR("invalid glyph format returned: %d\n", glyph->format);
                        FT_Done_Glyph(glyph);
                        goto end;
                    }

#ifdef ITU_ENABLE_DEFER_DESTROY
                    ituAddDeferDestroyData(glyph, FT_Done_Glyph);
#else
                    FT_Done_Glyph(glyph);
#endif
                }
            }
        }
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

    error = FTC_Manager_New(ft.library, 0, 0, CFG_ITU_FT_CACHE_SIZE, face_requester, 0, &ft.cache_manager);
    if (error)
    {
        ITU_LOG_ERR("could not initialize cache manager\n");
        goto end;
    }

    error = FTC_SBitCache_New(ft.cache_manager, &ft.sbits_cache);
    if (error)
    {
        ITU_LOG_ERR("could not initialize small bitmaps cache\n");
        goto end;
    }

    error = FTC_ImageCache_New(ft.cache_manager, &ft.image_cache);
    if (error)
    {
        ITU_LOG_ERR("could not initialize glyph image cache\n");
        goto end;
    }

    error = FTC_CMapCache_New(ft.cache_manager, &ft.cmap_cache);
    if (error)
    {
        ITU_LOG_ERR("could not initialize charmap cache\n");
        goto end;
    }

    FT_Bitmap_New(&ft.bitmap);
end:
    if (error)
    {
        ituFtExit();
    }
    else
    {
        int i = 0;
        for (; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            ft.style_extra[i] = 0;
        }
    }

    return error;
}

void ituFtSetFontStyle (unsigned int style)
{
    ft.style = style;
}

unsigned int ituFtGetFontExtraStyle (int fontindex)
{
    if (fontindex < ITU_FREETYPE_MAX_FONTS)
    {
        return ft.style_extra[fontindex];
    }
    else
    {
        return 0;
    }
}

void ituFtSetFontExtraStyle (int fontindex, unsigned int style)
{
    if (fontindex < ITU_FREETYPE_MAX_FONTS)
    {
        ft.style_extra[fontindex] = style;
    }
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
        FT_UInt gindex;
        int     charcode;
        TFont * tfont = ft.current_font;

        charcode      = buf;
        gindex        = ituFtCheckGlyphIndex(&tfont, charcode, text, 0);
        /*if (ft.pri_font != NULL)
        {
            tfont = ft.pri_font;
        }*/
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
    if (priority != NULL)
    {
        int i = 0;
        int validItem = (count > 0) ? (count) : (ITU_FREETYPE_MAX_FONTS);
        for (; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            if (i < validItem)
            {
                ft.priority[i] = priority[i];
            }
            else
            {
                ft.priority[i] = -1;
            }
        }
    }
    else
    {
        int i = 0;
        for (; i < ITU_FREETYPE_MAX_FONTS; i++)
        {
            ft.priority[i] = -1;
        }
    }
}
