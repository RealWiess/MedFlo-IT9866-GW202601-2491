/**
 * @file lv_itg.h
 *
 */

#ifndef LV_ITG_H
#define LV_ITG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#if LV_USE_ITG

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register the ITG decoder functions in LVGL
 */
void lv_itg_init(void);
void lv_itg_deinit(void);

lv_obj_t * lv_itg_anim_create(lv_obj_t * parent);
void lv_itg_anim_set_src(lv_obj_t * obj, const void * src);
void lv_itg_anim_restart(lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ITG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_ITG_H*/
