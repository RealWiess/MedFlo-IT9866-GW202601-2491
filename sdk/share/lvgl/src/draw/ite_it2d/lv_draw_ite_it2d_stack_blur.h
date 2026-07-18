/**
 * @file lv_draw_ite_it2d_stack_blur.h
 *
 */
#ifndef LV_DRAW_ITE_IT2D_STACK_BLUR_H
#define LV_DRAW_ITE_IT2D_STACK_BLUR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_ITE_IT2D

#include "../../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_stack_blur_grayscale(lv_opa_t * buf, uint16_t w, uint16_t h, uint16_t r);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ITE_IT2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ITE_IT2D_STACK_BLUR_H*/
