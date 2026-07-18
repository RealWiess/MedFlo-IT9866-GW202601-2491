/**
 * @file lv_draw_ite_it2d_utils.h
 *
 */
#ifndef LV_DRAW_ITE_IT2D_UTILS_H
#define LV_DRAW_ITE_IT2D_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#if LV_USE_DRAW_ITE_IT2D

#include "../../misc/lv_color.h"
#include "../../misc/lv_area.h"

#include LV_ITE_IT2D_INCLUDE_PATH

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

ite_it2d_surface_t * lv_layer_get_ite_it2d_surface(lv_layer_t * layer);

void lv_area_to_ite_it2d_rect(const lv_area_t * in, ite_it2d_rect_t * out);

void lv_color_to_ite_it2d_color(const lv_color_t * in, ite_it2d_color_t * out);

void lv_ite_it2d_to_a8421(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height, int32_t stride,
                          uint8_t bpp);

void lv_ite_it2d_to_xargb(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height, uint8_t bpp);

void lv_ite_it2d_x8888_to_a8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height);

void lv_ite_it2d_rgb565a8_to_argb8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height);

void lv_ite_it2d_l8_to_a8888(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height);

void lv_ite_it2d_a8888_mix_a8(uint8_t * dst, const uint8_t * src, int32_t width, int32_t height);

bool lv_ite_it2d_is_src_cf_supported(lv_color_format_t cf);

bool lv_ite_it2d_is_hw_cf_supported(lv_color_format_t cf);

ite_it2d_pixel_format_t lv_color_format_to_ite_it2d_pixel_format(lv_color_format_t cf);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_UTILS_H*/
