/**
 * @file lv_draw_ite_it2d_border.c
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
#if LV_USE_DRAW_ITE_IT2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t magic;
    int16_t rout, rin;
    int16_t x1, y1, x2, y2;
} lv_draw_border_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void draw_border_complex(lv_draw_task_t * t, const lv_area_t * outer_area, const lv_area_t * inner_area,
                                int32_t rout, int32_t rin, lv_color_t color, lv_opa_t opa);

static void draw_border_simple(lv_draw_task_t * t, const lv_area_t * outer_area, const lv_area_t * inner_area,
                               lv_color_t color, lv_opa_t opa);

static lv_draw_border_key_t border_key_create(int32_t rout, int32_t rin, const lv_area_t * outer_area,
                                              const lv_area_t * inner_area);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_border(lv_draw_task_t * t, const lv_draw_border_dsc_t * dsc, const lv_area_t * coords)
{
    if(dsc->opa <= LV_OPA_MIN) return;
    if(dsc->width == 0) return;
    if(dsc->side == LV_BORDER_SIDE_NONE) return;

    int32_t coords_w = lv_area_get_width(coords);
    int32_t coords_h = lv_area_get_height(coords);
    int32_t rout = dsc->radius;
    int32_t short_side = LV_MIN(coords_w, coords_h);
    if(rout > short_side >> 1) rout = short_side >> 1;

    /*Get the inner area*/
    lv_area_t area_inner;
    lv_area_copy(&area_inner, coords);
    area_inner.x1 += ((dsc->side & LV_BORDER_SIDE_LEFT) ? dsc->width : - (dsc->width + rout));
    area_inner.x2 -= ((dsc->side & LV_BORDER_SIDE_RIGHT) ? dsc->width : - (dsc->width + rout));
    area_inner.y1 += ((dsc->side & LV_BORDER_SIDE_TOP) ? dsc->width : - (dsc->width + rout));
    area_inner.y2 -= ((dsc->side & LV_BORDER_SIDE_BOTTOM) ? dsc->width : - (dsc->width + rout));

    int32_t rin = rout - dsc->width;
    if(rin < 0) rin = 0;

    if(rout == 0 && rin == 0) {
        draw_border_simple(t, coords, &area_inner, dsc->color, dsc->opa);
    }
    else {
        draw_border_complex(t, coords, &area_inner, rout, rin, dsc->color, dsc->opa);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void draw_border_simple(lv_draw_task_t * t, const lv_area_t * outer_area, const lv_area_t * inner_area,
                               lv_color_t color, lv_opa_t opa)
{
    lv_layer_t * layer = t->target_layer;

    lv_area_t clip_area;
    if(!lv_area_intersect(&clip_area, outer_area, &t->clip_area)) return;
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t out_area = *outer_area;
    lv_area_move(&out_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t in_area = *inner_area;
    lv_area_move(&in_area, -layer->buf_area.x1, -layer->buf_area.y1);

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(layer);
    ite_it2d_set_dst_surface(&ctx, dstsurf);

    if(opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }
    ite_it2d_set_fg_color(&ctx, ite_it2d_color_make(color.red, color.green, color.blue, 255));

    ite_it2d_area_t it2d_clip_area;
    it2d_clip_area.x1 = clip_area.x1;
    it2d_clip_area.y1 = clip_area.y1;
    it2d_clip_area.x2 = clip_area.x2;
    it2d_clip_area.y2 = clip_area.y2;

    ite_it2d_set_clip_area(&ctx, &it2d_clip_area);
    ite_it2d_enable_clipping(&ctx, true);

    bool top_side = outer_area->y1 <= inner_area->y1;
    bool bottom_side = outer_area->y2 >= inner_area->y2;
    bool left_side = outer_area->x1 <= inner_area->x1;
    bool right_side = outer_area->x2 >= inner_area->x2;

    lv_area_t a;
    ite_it2d_rect_t rect;

    /*Top*/
    a.x1 = out_area.x1;
    a.x2 = out_area.x2;
    a.y1 = out_area.y1;
    a.y2 = in_area.y1 - 1;
    if(top_side) {
        lv_area_to_ite_it2d_rect(&a, &rect);
        ite_it2d_set_dst_rect(&ctx, &rect);
        ite_it2d_fill(&ctx);
    }

    /*Bottom*/
    a.y1 = in_area.y2 + 1;
    a.y2 = out_area.y2;
    if(bottom_side) {
        lv_area_to_ite_it2d_rect(&a, &rect);
        ite_it2d_set_dst_rect(&ctx, &rect);
        ite_it2d_fill(&ctx);
    }

    /*Left*/
    a.x1 = out_area.x1;
    a.x2 = in_area.x1 - 1;
    a.y1 = (top_side) ? in_area.y1 : out_area.y1;
    a.y2 = (bottom_side) ? in_area.y2 : out_area.y2;
    if(left_side) {
        lv_area_to_ite_it2d_rect(&a, &rect);
        ite_it2d_set_dst_rect(&ctx, &rect);
        ite_it2d_fill(&ctx);
    }

    /*Right*/
    a.x1 = in_area.x2 + 1;
    a.x2 = out_area.x2;
    if(right_side) {
        lv_area_to_ite_it2d_rect(&a, &rect);
        ite_it2d_set_dst_rect(&ctx, &rect);
        ite_it2d_fill(&ctx);
    }
}

static void draw_border_complex(lv_draw_task_t * t, const lv_area_t * outer_area, const lv_area_t * inner_area,
                                int32_t rout, int32_t rin, lv_color_t color, lv_opa_t opa)
{
    lv_layer_t * layer = t->target_layer;

    lv_area_t clip_area;
    if(!lv_area_intersect(&clip_area, outer_area, &t->clip_area)) return;
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t out_area = *outer_area;
    lv_area_move(&out_area, -layer->buf_area.x1, -layer->buf_area.y1);

    opa = opa >= LV_OPA_COVER ? LV_OPA_COVER : opa;

    lv_draw_border_key_t key = border_key_create(rout, rin, outer_area, inner_area);
    int32_t w = lv_area_get_width(outer_area), h = lv_area_get_height(outer_area);
    int32_t short_side = LV_MIN(w, h);
    int32_t radius = LV_MIN(rout, short_side >> 1);
    int32_t max_side = LV_MAX4(key.x1, key.y1, -key.x2, -key.y2);
    int32_t frag_size = LV_MAX(radius, max_side);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(surf == NULL) {
        /* Create a mask surf with size of (frag_size * 2 + 3) */
        const lv_area_t frag_area = {0, 0, frag_size * 2 + 2, frag_size * 2 + 2};

        /*Create mask for the outer area*/
        void * masks[3] = {0};
        lv_draw_sw_mask_radius_param_t mask_rout_param;
        lv_draw_sw_mask_radius_init(&mask_rout_param, &frag_area, rout, false);
        masks[0] = &mask_rout_param;

        /*Create mask for the inner mask*/
        const lv_area_t frag_inner_area = {frag_area.x1 + key.x1, frag_area.y1 + key.y1,
                                           frag_area.x2 + key.x2, frag_area.y2 + key.y2
                                          };
        lv_draw_sw_mask_radius_param_t mask_rin_param;
        lv_draw_sw_mask_radius_init(&mask_rin_param, &frag_inner_area, rin, true);
        masks[1] = &mask_rin_param;

        surf = lv_draw_ite_it2d_mask_dump_surf(&frag_area, masks);
        if(surf) {
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }

        lv_draw_sw_mask_free_param(&mask_rout_param);
        lv_draw_sw_mask_free_param(&mask_rin_param);
    }

    if(surf) {
        ite_it2d_context_t ctx;
        ite_it2d_ctx_init(&ctx);

        ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(layer);
        ite_it2d_set_dst_surface(&ctx, dstsurf);

        if(opa < LV_OPA_MAX) {
            ite_it2d_set_const_alpha(&ctx, opa);
            ite_it2d_enable_const_alpha(&ctx, true);
        }
        ite_it2d_set_fg_color(&ctx, ite_it2d_color_make(color.red, color.green, color.blue, 255));
        ite_it2d_enable_fg_color(&ctx, true);
        ite_it2d_enable_blending(&ctx, true);

        lv_ite_it2d_draw_corners(&ctx, surf, frag_size, &out_area, &clip_area, true);
        lv_ite_it2d_draw_borders(&ctx, surf, frag_size, &out_area, &clip_area, true);
    }
}

static lv_draw_border_key_t border_key_create(int32_t rout, int32_t rin, const lv_area_t * outer_area,
                                              const lv_area_t * inner_area)
{
    lv_draw_border_key_t key;
    /* VERY IMPORTANT! Padding between members is uninitialized, so we have to wipe them manually */
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_BORDER;
    key.rout = rout;
    key.rin = rin;
    key.x1 = inner_area->x1 - outer_area->x1;
    key.x2 = inner_area->x2 - outer_area->x2;
    key.y1 = inner_area->y1 - outer_area->y1;
    key.y2 = inner_area->y2 - outer_area->y2;
    return key;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
