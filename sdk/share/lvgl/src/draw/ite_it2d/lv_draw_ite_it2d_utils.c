/**
 * @file lv_draw_ite_it2d_utils.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_ite_it2d_utils.h"
#if LV_USE_DRAW_ITE_IT2D
#include "../lv_draw.h"
#include "../../misc/lv_area_private.h"
#include "../../core/lv_refr_private.h"
#include "lvgl_display.h"
#include "lv_draw_ite_it2d.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

ite_it2d_surface_t * lv_layer_get_ite_it2d_surface(lv_layer_t * layer)
{
    ite_it2d_surface_t * surf = NULL;

    if(layer->user_data) {
        surf = (ite_it2d_surface_t *) layer->user_data;
        surf->buf = layer->draw_buf->data;
        return surf;
    }
    else {
        void * buf = lv_draw_layer_alloc_buf(layer);
        if(buf) {
            surf = lv_zalloc(sizeof(ite_it2d_surface_t));
            LV_ASSERT_MALLOC(surf);
            surf->width = lv_area_get_width(&layer->buf_area);
            surf->height = lv_area_get_height(&layer->buf_area);
            surf->flags.format = lv_color_format_to_ite_it2d_pixel_format(layer->color_format);
            surf->flags.static_buf = 1;
            surf->buf = buf;
            ite_it2d_create_surface(surf);

            layer->user_data = surf;
        }
    }
    return surf;
}

void lv_area_to_ite_it2d_rect(const lv_area_t * in, ite_it2d_rect_t * out)
{
    out->x = in->x1;
    out->y = in->y1;
    out->w = in->x2 - in->x1 + 1;
    out->h = in->y2 - in->y1 + 1;
}

void lv_color_to_ite_it2d_color(const lv_color_t * in, ite_it2d_color_t * out)
{
    out->ch.alpha = 255;
    out->ch.blue = in->blue;
    out->ch.green = in->green;
    out->ch.red = in->red;
}

void lv_ite_it2d_to_a8421(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height, int32_t stride,
                          uint8_t bpp)
{
    uint8_t div = 8 / bpp;
    if(width % div == 0) {
        lv_memcpy(dst, src, stride * height);
    }
    else {
        uint32_t src_len = width * height;
        uint32_t src_cur = 0;
        int8_t dst_curbit = 8 - bpp;
        uint16_t dst_cur = 0;
        uint8_t opa_mask = 0;
        switch(bpp) {
            case 1:
                opa_mask = 0x1;
                break;
            case 2:
                opa_mask = 0x3;
                break;
            case 4:
                opa_mask = 0xF;
                break;
        }
        lv_memzero(dst, stride * height);
        while(src_cur < src_len) {
            int8_t src_curbit = 8 - bpp;
            uint8_t src_byte = src[src_cur * bpp / 8];
            while(src_curbit >= 0 && src_cur < src_len) {
                uint8_t src_bits = opa_mask & (src_byte >> src_curbit);
                dst[dst_cur * bpp / 8] |= src_bits << dst_curbit;
                src_curbit -= bpp;
                src_cur++;
                if(dst_curbit > 0) {
                    dst_curbit -= bpp;
                }
                else {
                    dst_curbit = 8 - bpp;
                }
                dst_cur++;
                if(dst_cur >= width) {
                    dst_cur = 0;
                    dst += stride;
                    dst_curbit = 8 - bpp;
                }
            }
        }
    }
}

void lv_ite_it2d_to_xargb(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height, uint8_t bpp)
{
    uint32_t src_len = width * height;
    uint32_t src_cur = 0;
    uint8_t opa_mask = 0;
    uint8_t font_rgb[3];
    int8_t subpx_cnt = 0;
    switch(bpp) {
        case 1:
            opa_mask = 0x1;
            break;
        case 2:
            opa_mask = 0x3;
            break;
        case 4:
            opa_mask = 0xF;
            break;
        case 8:
            opa_mask = 0xFF;
            break;
        default:
            LV_ASSERT(0);
            return;
    }
    while(src_cur < src_len) {
        int8_t src_curbit = 8 - bpp;
        uint8_t src_byte = src[src_cur * bpp / 8];
        while(src_curbit >= 0 && src_cur < src_len) {
            uint8_t src_bits = opa_mask & (src_byte >> src_curbit);
            src_curbit -= bpp;
            src_cur++;
            font_rgb[subpx_cnt] = src_bits * 255 / opa_mask;
            subpx_cnt++;
            if(subpx_cnt == 3) {
                subpx_cnt = 0;
                *dst++ = 0x0;
                *dst++ = font_rgb[0];
                *dst++ = font_rgb[1];
                *dst++ = font_rgb[2];
            }
        }
    }
}

void lv_ite_it2d_x8888_to_a8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height)
{
    uint32_t src_len = width * height * 4;
    uint32_t src_cur = 0;
    while(src_cur < src_len) {
        *dst++ = src[src_cur + 0];
        *dst++ = src[src_cur + 1];
        *dst++ = src[src_cur + 2];
        *dst++ = 0xFF;
        src_cur += 4;
    }
}

void lv_ite_it2d_rgb565a8_to_argb8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height)
{
    uint32_t src_len = width * height * 2;
    uint32_t src_cur = 0;
    uint16_t color;
    uint8_t * alpha = (uint8_t *)(src + width * height * 2);
    while(src_cur < src_len) {
        color = src[src_cur++];
        color |= src[src_cur++] << 8;
        *dst++ = (color & 0x001f) << 3;
        *dst++ = ((color & 0x07e0) >> 5) << 2;
        *dst++ = ((color & 0xf800) >> 11) << 3;
        *dst++ = *alpha++;
    }
}

void lv_ite_it2d_l8_to_a8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height)
{
    uint32_t src_len = width * height;
    uint32_t src_cur = 0;
    while(src_cur < src_len) {
        uint8_t src_byte = src[src_cur];
        *dst++ = src_byte;
        *dst++ = src_byte;
        *dst++ = src_byte;
        *dst++ = 0xFF;
        src_cur++;
    }
}

void lv_ite_it2d_a8888_mix_a8(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height)
{
    uint32_t src_len = width * height;
    uint32_t src_cur = 0;
    uint8_t alpha;
    while(src_cur < src_len) {
        alpha = src[src_cur];
        dst += 3;
        *dst = LV_OPA_MIX2(*dst, alpha);
        dst++;
        src_cur++;
    }
}

bool lv_ite_it2d_is_src_cf_supported(lv_color_format_t cf)
{
    switch(cf) {
#ifdef CONFIG_ITE_ITJPEG
        case LV_COLOR_FORMAT_RAW:
#endif
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_RGB565A8:
        case LV_COLOR_FORMAT_L8:
        case LV_COLOR_FORMAT_A8:
        case LV_COLOR_FORMAT_A4:
        case LV_COLOR_FORMAT_A2:
        case LV_COLOR_FORMAT_A1:
        case LV_COLOR_FORMAT_ARGB1555:
        case LV_COLOR_FORMAT_ARGB4444:
            return true;
        default:
            return false;
    }
}

bool lv_ite_it2d_is_hw_cf_supported(lv_color_format_t cf)
{
    switch(cf) {
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB1555:
        case LV_COLOR_FORMAT_ARGB4444:
            return true;
        default:
            return false;
    }
}

ite_it2d_pixel_format_t lv_color_format_to_ite_it2d_pixel_format(lv_color_format_t cf)
{
    switch(cf) {
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
            return ITE_IT2D_PIXEL_FORMAT_ARGB_8888;
        case LV_COLOR_FORMAT_RGB565:
            return ITE_IT2D_PIXEL_FORMAT_RGB_565;
        case LV_COLOR_FORMAT_L8:
        case LV_COLOR_FORMAT_A8:
            return ITE_IT2D_PIXEL_FORMAT_A_8;
        case LV_COLOR_FORMAT_A4:
            return ITE_IT2D_PIXEL_FORMAT_A_4;
        case LV_COLOR_FORMAT_A2:
            return ITE_IT2D_PIXEL_FORMAT_A_2;
        case LV_COLOR_FORMAT_A1:
            return ITE_IT2D_PIXEL_FORMAT_A_1;
        case LV_COLOR_FORMAT_ARGB1555:
            return ITE_IT2D_PIXEL_FORMAT_ARGB_1555;
        case LV_COLOR_FORMAT_ARGB4444:
            return ITE_IT2D_PIXEL_FORMAT_ARGB_4444;
        default:
            LV_LOG_ERROR("unsupported color format: 0x%X", cf);
            break;
    }

    LV_ASSERT(false);
    return 0;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
