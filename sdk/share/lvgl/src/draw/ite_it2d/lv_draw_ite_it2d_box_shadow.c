/**
 * @file lv_draw_ite_it2d_box_shadow.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_area_private.h"
#include "../sw/lv_draw_sw_mask_private.h"
#include "../lv_draw_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_mask.h"
#include "lv_draw_ite_it2d_rect.h"
#include "lv_draw_ite_it2d_stack_blur.h"
#if LV_USE_DRAW_ITE_IT2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t magic;
    int16_t radius;
    int16_t size;
    int16_t blur;
} lv_draw_box_shadow_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_draw_box_shadow_key_t box_shadow_key_create(int32_t radius, int32_t size, int32_t blur);

static ite_it2d_surface_t * box_shadow_surf_obtain(int32_t radius, int32_t size, int32_t blur);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_box_shadow(lv_draw_task_t * t, const lv_draw_box_shadow_dsc_t * dsc,
                                 const lv_area_t * coords)
{
    /*Calculate the rectangle which is blurred to get the shadow in `shadow_area`*/
    lv_area_t core_area;
    core_area.x1 = coords->x1  + dsc->ofs_x - dsc->spread;
    core_area.x2 = coords->x2  + dsc->ofs_x + dsc->spread;
    core_area.y1 = coords->y1  + dsc->ofs_y - dsc->spread;
    core_area.y2 = coords->y2  + dsc->ofs_y + dsc->spread;

    /*Calculate the bounding box of the shadow*/
    lv_area_t shadow_area;
    shadow_area.x1 = core_area.x1 - dsc->width / 2 - 1;
    shadow_area.x2 = core_area.x2 + dsc->width / 2 + 1;
    shadow_area.y1 = core_area.y1 - dsc->width / 2 - 1;
    shadow_area.y2 = core_area.y2 + dsc->width / 2 + 1;

    lv_opa_t opa = dsc->opa;
    if(opa > LV_OPA_MAX) opa = LV_OPA_COVER;

    /*Get clipped draw area which is the real draw area.
     *It is always the same or inside `shadow_area`*/
    lv_layer_t * layer = t->target_layer;
    lv_area_t draw_area, clip_area = t->clip_area;
    if(!lv_area_intersect(&draw_area, &shadow_area, &clip_area)) return;
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    int32_t radius = dsc->radius;
    /* No matter how big the shadow is, what we need is just a corner */
    const int w = lv_area_get_width(&core_area), h = lv_area_get_height(&core_area);
    int32_t short_side = LV_MIN(w, h);
    int32_t frag_size = LV_MIN(LV_MAX(dsc->width / 2, radius), short_side >> 1);

    /* This is how big the corner is after blurring */
    int32_t blur_growth = (int32_t)(dsc->width / 2 + 1);

    int32_t blur_frag_size = (int32_t)(frag_size + blur_growth);

    ite_it2d_surface_t * surf = box_shadow_surf_obtain(radius, frag_size, dsc->width);
    if(surf) {
        ite_it2d_context_t ctx;
        ite_it2d_ctx_init(&ctx);

        ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(t->target_layer);
        ite_it2d_set_dst_surface(&ctx, dstsurf);

        if(opa < LV_OPA_MAX) {
            ite_it2d_set_const_alpha(&ctx, opa);
            ite_it2d_enable_const_alpha(&ctx, true);
        }
        ite_it2d_set_fg_color(&ctx, ite_it2d_color_make(dsc->color.red, dsc->color.green, dsc->color.blue, 255));
        ite_it2d_enable_fg_color(&ctx, true);
        ite_it2d_enable_blending(&ctx, true);

        lv_ite_it2d_draw_corners(&ctx, surf, blur_frag_size, &draw_area, &clip_area, false);
        lv_ite_it2d_draw_borders(&ctx, surf, blur_frag_size, &draw_area, &clip_area, false);
        lv_ite_it2d_draw_center(&ctx, surf, blur_frag_size, &draw_area, &clip_area, false);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_draw_box_shadow_key_t box_shadow_key_create(int32_t radius, int32_t size, int32_t blur)
{
    lv_draw_box_shadow_key_t key;
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_BOX_SHADOW;
    key.radius = radius;
    key.size = size;
    key.blur = blur;
    return key;
}

static ite_it2d_surface_t * box_shadow_surf_obtain(int32_t radius, int32_t size, int32_t blur)
{
    lv_draw_box_shadow_key_t key = box_shadow_key_create(radius, size, blur);

    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(!surf) {
        int32_t blur_growth = blur / 2 + 1;
        int32_t blur_frag_size = size + blur_growth;
        lv_area_t mask_area = {blur_growth, blur_growth}, mask_area_blurred = {0, 0};
        lv_area_set_width(&mask_area, size * 2);
        lv_area_set_height(&mask_area, size * 2);
        lv_area_set_width(&mask_area_blurred, blur_frag_size * 2);
        lv_area_set_height(&mask_area_blurred, blur_frag_size * 2);

        lv_draw_sw_mask_radius_param_t mask_rout_param;
        lv_draw_sw_mask_radius_init(&mask_rout_param, &mask_area, radius, false);
        void * masks[2] = {0};
        masks[0] = &mask_rout_param;
        lv_opa_t * mask_buf = lv_draw_ite_it2d_mask_dump_opa(&mask_area_blurred, masks);
        if(!mask_buf) {
            lv_draw_sw_mask_free_param(&mask_rout_param);
            return NULL;
        }

        lv_stack_blur_grayscale(mask_buf, lv_area_get_width(&mask_area_blurred), lv_area_get_height(&mask_area_blurred),
                                blur / 2 + blur % 2);

        surf = lv_zalloc(sizeof(ite_it2d_surface_t));
        LV_ASSERT_MALLOC(surf);
        surf->width = blur_frag_size;
        surf->height = blur_frag_size;
        surf->pitch = lv_area_get_width(&mask_area_blurred);
        surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;
        surf->buf = mask_buf;
        surf->id = ite_it2d_get_current_id();
        lv_sys_cache_data_flush_range(mask_buf, surf->pitch * surf->height);

        lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        lv_draw_sw_mask_free_param(&mask_rout_param);
    }
    return surf;
}

#endif /*LV_DRAW_USE_ITE_IT2D*/
