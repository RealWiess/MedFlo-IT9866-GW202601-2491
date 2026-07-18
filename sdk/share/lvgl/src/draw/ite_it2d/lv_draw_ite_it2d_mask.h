/**
 * @file lv_draw_ite_it2d_mask.h
 *
 */

#ifndef LV_DRAW_ITE_IT2D_MASK_H
#define LV_DRAW_ITE_IT2D_MASK_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#if LV_USE_DRAW_ITE_IT2D

#include LV_ITE_IT2D_INCLUDE_PATH

#include "../../misc/lv_area.h"
#include "lv_draw_ite_it2d_surf_cache.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum lv_draw_mask_id_t {
    LV_DRAW_MASK_ID_RECOLOR,
    LV_DRAW_MASK_ID_FRAG,
    LV_DRAW_MASK_ID_GRADIENT,
} lv_draw_mask_id_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_opa_t * lv_draw_ite_it2d_mask_dump_opa(const lv_area_t * coords, void * masks[]);

ite_it2d_surface_t * lv_draw_ite_it2d_mask_dump_surf(const lv_area_t * coords, void * masks[]);

ite_it2d_surface_t * lv_draw_ite_it2d_mask_surf_obtain(lv_draw_mask_id_t type, int32_t w, int32_t h,
                                                       ite_it2d_pixel_format_t format);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_MASK_H*/
