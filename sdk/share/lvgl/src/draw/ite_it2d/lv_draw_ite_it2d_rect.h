/**
 * @file lv_draw_ite_it2d_rect.h
 *
 */
#ifndef LV_DRAW_ITE_IT2D_RECT_H
#define LV_DRAW_ITE_IT2D_RECT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#if LV_USE_DRAW_ITE_IT2D

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

ite_it2d_surface_t * lv_draw_ite_it2d_bg_frag_obtain(int32_t radius);

void lv_ite_it2d_draw_corners(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                              const lv_area_t * coords, const lv_area_t * clip, bool full);

void lv_ite_it2d_draw_borders(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                              const lv_area_t * coords, const lv_area_t * clipped, bool full);

void lv_ite_it2d_draw_center(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                             const lv_area_t * coords, const lv_area_t * clipped, bool full);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_RECT_H*/
