/**
 * @file lv_draw_ite_it2d_mask.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../sw/lv_draw_sw_mask_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_mask.h"
#if LV_USE_DRAW_ITE_IT2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t magic;
    uint8_t type;
} lv_draw_mask_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_draw_mask_key_t mask_key_create(lv_draw_mask_id_t type);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_opa_t * lv_draw_ite_it2d_mask_dump_opa(const lv_area_t * coords, void * masks[])
{
    LV_ASSERT(coords->x2 >= coords->x1);
    LV_ASSERT(coords->y2 >= coords->y1);
    int32_t w = lv_area_get_width(coords), h = lv_area_get_height(coords);
    lv_opa_t * mask_buf = lv_draw_ite_it2d_alloc_buffer(w * h);
    if(!mask_buf) {
        return NULL;
    }
    for(int32_t y = 0; y < h; y++) {
        lv_opa_t * line_buf = &mask_buf[y * w];
        lv_memset(line_buf, 0xff, w);
        int32_t abs_x = (int32_t) coords->x1, abs_y = (int32_t)(y + coords->y1), len = (int32_t) w;
        lv_draw_sw_mask_res_t res = lv_draw_sw_mask_apply(masks, line_buf, abs_x, abs_y, len);
        if(res == LV_DRAW_SW_MASK_RES_TRANSP) {
            lv_memzero(line_buf, w);
        }
    }
    return mask_buf;
}

ite_it2d_surface_t * lv_draw_ite_it2d_mask_dump_surf(const lv_area_t * coords, void * masks[])
{
    int32_t w = lv_area_get_width(coords), h = lv_area_get_height(coords);
    lv_opa_t * mask_buf = lv_draw_ite_it2d_mask_dump_opa(coords, masks);
    if(!mask_buf) {
        return NULL;
    }
    ite_it2d_surface_t * surf = lv_zalloc(sizeof(ite_it2d_surface_t));
    LV_ASSERT_MALLOC(surf);
    surf->width = w;
    surf->height = h;
    surf->pitch = w;
    surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;
    surf->buf = mask_buf;
    surf->id = ite_it2d_get_current_id();
    lv_sys_cache_data_flush_range(mask_buf, w * h);
    return surf;
}

ite_it2d_surface_t * lv_draw_ite_it2d_mask_surf_obtain(lv_draw_mask_id_t type, int32_t w, int32_t h,
                                                       ite_it2d_pixel_format_t format)
{
    lv_draw_mask_key_t mask_key = mask_key_create(type);
    uint32_t bpp = ite_it2d_bits_per_pixel(format) / 8;
    uint32_t size = w * h * bpp;
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&mask_key, sizeof(lv_draw_mask_key_t));
    if(surf) {
        if(size <= (uint32_t)surf->user_data) {
            surf->width = w;
            surf->height = h;
            surf->pitch = w * bpp;
            surf->flags.format = format;
        }
        else {
            surf = NULL;
        }
    }
    if(!surf) {
        surf = lv_zalloc(sizeof(ite_it2d_surface_t));
        LV_ASSERT_MALLOC(surf);

        surf->width = w;
        surf->height = h;
        surf->flags.format = format;
        surf->user_data = (void *)(size);

        if(!lv_draw_ite_it2d_create_surface(surf)) {
            lv_free(surf);
            return NULL;
        }
        lv_draw_ite_it2d_surf_cache_put(&mask_key, sizeof(lv_draw_mask_key_t), surf);
    }
    return surf;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_draw_mask_key_t mask_key_create(lv_draw_mask_id_t type)
{
    lv_draw_mask_key_t key;
    /* VERY IMPORTANT! Padding between members is uninitialized, so we have to wipe them manually */
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_MASK;
    key.type = type;
    return key;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
