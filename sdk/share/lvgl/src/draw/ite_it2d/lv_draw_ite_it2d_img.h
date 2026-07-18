/**
 * @file lv_draw_ite_it2d_img.h
 *
 */

#ifndef LV_DRAW_ITE_IT2D_IMG_H
#define LV_DRAW_ITE_IT2D_IMG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#if LV_USE_DRAW_ITE_IT2D

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

ite_it2d_surface_t * lv_draw_ite_it2d_img_surf_obtain(const lv_image_decoder_dsc_t * decoder_dsc);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_IMG_H*/
