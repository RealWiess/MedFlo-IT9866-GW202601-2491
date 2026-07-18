/**
 * @file lv_draw_ite_it2d_triangle.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../sw/lv_draw_sw_mask_private.h"
#include "../lv_draw_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_mask.h"
#if LV_USE_DRAW_ITE_IT2D

#include "../../misc/lv_area_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t magic;
    int16_t x1, y1, x2, y2, x3, y3;
} lv_draw_triangle_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_draw_triangle_key_t triangle_key_create(lv_point_t p[3]);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_triangle(lv_draw_task_t * t, const lv_draw_triangle_dsc_t * dsc)
{
    lv_area_t tri_area;
    tri_area.x1 = (int32_t)LV_MIN3(dsc->p[0].x, dsc->p[1].x, dsc->p[2].x);
    tri_area.y1 = (int32_t)LV_MIN3(dsc->p[0].y, dsc->p[1].y, dsc->p[2].y);
    tri_area.x2 = (int32_t)LV_MAX3(dsc->p[0].x, dsc->p[1].x, dsc->p[2].x);
    tri_area.y2 = (int32_t)LV_MAX3(dsc->p[0].y, dsc->p[1].y, dsc->p[2].y);

    bool is_common;
    lv_layer_t * layer = t->target_layer;
    lv_area_t clip_area;
    is_common = lv_area_intersect(&clip_area, &tri_area, &t->clip_area);
    if(!is_common) return;

    lv_area_t draw_area = tri_area;
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_point_t p[3];
    /*If there is a vertical side use it as p[0] and p[1]*/
    if(dsc->p[0].x == dsc->p[1].x) {
        p[0] = lv_point_from_precise(&dsc->p[0]);
        p[1] = lv_point_from_precise(&dsc->p[1]);
        p[2] = lv_point_from_precise(&dsc->p[2]);
    }
    else if(dsc->p[0].x == dsc->p[2].x) {
        p[0] = lv_point_from_precise(&dsc->p[0]);
        p[1] = lv_point_from_precise(&dsc->p[2]);
        p[2] = lv_point_from_precise(&dsc->p[1]);
    }
    else if(dsc->p[1].x == dsc->p[2].x) {
        p[0] = lv_point_from_precise(&dsc->p[1]);
        p[1] = lv_point_from_precise(&dsc->p[2]);
        p[2] = lv_point_from_precise(&dsc->p[0]);
    }
    else {
        p[0] = lv_point_from_precise(&dsc->p[0]);
        p[1] = lv_point_from_precise(&dsc->p[1]);
        p[2] = lv_point_from_precise(&dsc->p[2]);

        /*Set the smallest y as p[0]*/
        if(p[0].y > p[1].y) lv_point_swap(&p[0], &p[1]);
        if(p[0].y > p[2].y) lv_point_swap(&p[0], &p[2]);

        /*Set the greatest y as p[1]*/
        if(p[1].y < p[2].y) lv_point_swap(&p[1], &p[2]);
    }

    /*Be sure p[0] is on the top*/
    if(p[0].y > p[1].y) lv_point_swap(&p[0], &p[1]);

    lv_draw_triangle_key_t key = triangle_key_create(p);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(surf == NULL) {
        /*If right == true p[2] is on the right side of the p[0] p[1] line*/
        bool right = ((p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[1].y - p[0].y) * (p[2].x - p[0].x)) < 0;

        void * masks[4] = {0};
        lv_draw_sw_mask_line_param_t mask_left;
        lv_draw_sw_mask_line_param_t mask_right;
        lv_draw_sw_mask_line_param_t mask_bottom;

        lv_draw_sw_mask_line_points_init(&mask_left, p[0].x, p[0].y,
                                         p[1].x, p[1].y,
                                         right ? LV_DRAW_SW_MASK_LINE_SIDE_RIGHT : LV_DRAW_SW_MASK_LINE_SIDE_LEFT);

        lv_draw_sw_mask_line_points_init(&mask_right, p[0].x, p[0].y,
                                         p[2].x, p[2].y,
                                         right ? LV_DRAW_SW_MASK_LINE_SIDE_LEFT : LV_DRAW_SW_MASK_LINE_SIDE_RIGHT);

        if(p[1].y == p[2].y) {
            lv_draw_sw_mask_line_points_init(&mask_bottom, p[1].x, p[1].y,
                                             p[2].x, p[2].y, LV_DRAW_SW_MASK_LINE_SIDE_TOP);
        }
        else {
            lv_draw_sw_mask_line_points_init(&mask_bottom, p[1].x, p[1].y,
                                             p[2].x, p[2].y,
                                             right ? LV_DRAW_SW_MASK_LINE_SIDE_LEFT  : LV_DRAW_SW_MASK_LINE_SIDE_RIGHT);
        }

        masks[0] = &mask_left;
        masks[1] = &mask_right;
        masks[2] = &mask_bottom;

        surf = lv_draw_ite_it2d_mask_dump_surf(&tri_area, masks);
        if(surf) {
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }

        lv_draw_sw_mask_free_param(&mask_bottom);
        lv_draw_sw_mask_free_param(&mask_left);
        lv_draw_sw_mask_free_param(&mask_right);
    }

    if(surf == NULL) return;

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(layer);
    ite_it2d_set_dst_surface(&ctx, dstsurf);

    ite_it2d_rect_t rect;
    rect.x = draw_area.x1;
    rect.y = draw_area.y1;
    rect.w = lv_area_get_width(&draw_area);
    rect.h = lv_area_get_height(&draw_area);
    ite_it2d_set_dst_rect(&ctx, &rect);

    ite_it2d_area_t it2d_clip_area;
    it2d_clip_area.x1 = clip_area.x1;
    it2d_clip_area.y1 = clip_area.y1;
    it2d_clip_area.x2 = clip_area.x2;
    it2d_clip_area.y2 = clip_area.y2;
    ite_it2d_set_clip_area(&ctx, &it2d_clip_area);
    ite_it2d_enable_clipping(&ctx, true);

    ite_it2d_color_t color;
    lv_color_to_ite_it2d_color(&dsc->color, &color);
    ite_it2d_set_fg_color(&ctx, color);
    ite_it2d_enable_fg_color(&ctx, true);

    if(dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    ite_it2d_enable_blending(&ctx, true);

    ite_it2d_set_src_surface(&ctx, surf);

    ite_it2d_copy(&ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_draw_triangle_key_t triangle_key_create(lv_point_t p[3])
{
    lv_draw_triangle_key_t key;
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_TRIANGLE;
    key.x1 = p[0].x;
    key.y1 = p[0].y;
    key.x2 = p[1].x;
    key.y2 = p[1].y;
    key.x3 = p[2].x;
    key.y3 = p[2].y;
    return key;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
