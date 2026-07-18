/**
 * @file lv_draw_ite_it2d.h
 *
 */

#ifndef LV_DRAW_ITE_IT2D_H
#define LV_DRAW_ITE_IT2D_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_draw.h"

#if LV_USE_DRAW_ITE_IT2D

#include "../../misc/lv_area.h"
#include "../../draw/lv_draw_label.h"
#include "../../draw/lv_draw_rect.h"
#include "../../draw/lv_draw_arc.h"
#include "../../draw/lv_draw_image.h"
#include "../../draw/lv_draw_triangle.h"
#include "../../draw/lv_draw_line.h"
#include "lv_draw_ite_it2d_surf_cache.h"
#include "lv_draw_ite_it2d_utils.h"
#include "ite/ith.h"

/*********************
 *      DEFINES
 *********************/

#define LV_DRAW_ITE_IT2D_READY              LV_IMAGE_FLAGS_USER1

#define LV_DRAW_ITE_IT2D_CACHE_TEXT_STATIC  1
#define LV_DRAW_ITE_IT2D_SW                 0

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_draw_ite_it2d_init(void);

void lv_draw_ite_it2d_buf_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                               const lv_draw_buf_t * src, const lv_area_t * src_area);

void lv_draw_ite_it2d_image(lv_draw_task_t * t,
                            const lv_draw_image_dsc_t * draw_dsc,
                            const lv_area_t * coords);

void lv_draw_ite_it2d_fill(lv_draw_task_t * t, const lv_draw_fill_dsc_t * dsc, const lv_area_t * coords);

void lv_draw_ite_it2d_border(lv_draw_task_t * t, const lv_draw_border_dsc_t * dsc, const lv_area_t * coords);

void lv_draw_ite_it2d_box_shadow(lv_draw_task_t * t, const lv_draw_box_shadow_dsc_t * dsc,
                                 const lv_area_t * coords);

void lv_draw_ite_it2d_label(lv_draw_task_t * t, const lv_draw_label_dsc_t * dsc, const lv_area_t * coords);

void lv_draw_ite_it2d_arc(lv_draw_task_t * t, const lv_draw_arc_dsc_t * dsc, const lv_area_t * coords);

void lv_draw_ite_it2d_line(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc);

void lv_draw_ite_it2d_layer(lv_draw_task_t * t, const lv_draw_image_dsc_t * draw_dsc, const lv_area_t * coords);

void lv_draw_ite_it2d_triangle(lv_draw_task_t * t, const lv_draw_triangle_dsc_t * dsc);

void lv_draw_ite_it2d_mask_rect(lv_draw_task_t * t, const lv_draw_mask_rect_dsc_t * dsc,
                                const lv_area_t * coords);

static inline void lv_sys_cache_data_invd_range(void * addr, size_t size)
{
    ithInvalidateDCacheRange(addr, size);
}

static inline void lv_sys_cache_data_flush_range(void * addr, size_t size)
{
    ithFlushDCacheRange(addr, size);
}

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_H*/
