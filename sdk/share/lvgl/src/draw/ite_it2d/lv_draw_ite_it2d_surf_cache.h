/**
 * @file  lv_draw_ite_it2d_surf_cache.h
 *
 */

#ifndef LV_DRAW_ITE_IT2D_SURF_CACHE_H
#define LV_DRAW_ITE_IT2D_SURF_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_ITE_IT2D

#include LV_ITE_IT2D_INCLUDE_PATH
#include "../lv_image_decoder.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_ITE_IT2D_CACHE_KEY_MAGIC_IMG = 0x11,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_IMG_ROUNDED_CORNERS = 0x12,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE = 0x21,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE_ROUNDED = 0x22,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE_DASHED = 0x23,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_BG = 0x31,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_BOX_SHADOW = 0x32,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_BORDER = 0x33,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_FONT_GLYPH = 0x41,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_MASK = 0x51,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_ARC = 0x61,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_ARC_ROUNDED = 0x62,
    LV_ITE_IT2D_CACHE_KEY_MAGIC_TRIANGLE = 0x71,
} lv_ite_it2d_cache_key_magic_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void  lv_draw_ite_it2d_surf_cache_init(void);

void  lv_draw_ite_it2d_surf_cache_deinit(void);

/**
 * Find cached surf by key. The surf can be destroyed during usage.
 */
ite_it2d_surface_t  * lv_draw_ite_it2d_surf_cache_get(const void * key,
                                                      size_t key_length);

void lv_draw_ite_it2d_surf_cache_put(const void * key, size_t key_length,
                                     ite_it2d_surface_t * surf);

bool lv_draw_ite_it2d_create_surface(ite_it2d_surface_t * surf);

void lv_draw_ite_it2d_destroy_surface(ite_it2d_surface_t * surf);

void * lv_draw_ite_it2d_alloc_buffer(uint32_t size);

void lv_draw_ite_it2d_free_buffer(void * buf);

/**********************
 *      MACROS
 **********************/
#endif /*LV_USE_DRAW_ITE_IT2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_SURF_CACHE_H*/
