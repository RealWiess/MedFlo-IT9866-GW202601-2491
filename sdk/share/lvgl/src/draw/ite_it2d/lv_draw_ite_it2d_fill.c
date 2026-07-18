/**
 * @file lv_draw_ite_it2d_fill.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_area_private.h"
#include "../lv_draw_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_rect.h"
#include "lv_draw_ite_it2d_mask.h"
#if LV_USE_DRAW_ITE_IT2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_fill(lv_draw_task_t * t, const lv_draw_fill_dsc_t * dsc, const lv_area_t * coords)
{
    if(dsc->opa <= LV_OPA_MIN) return;

    lv_area_t clipped_coords;
    if(!lv_area_intersect(&clipped_coords, coords, &t->clip_area)) return;

    lv_layer_t * layer = t->target_layer;
    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(t->target_layer);
    ite_it2d_set_dst_surface(&ctx, dstsurf);

    if(dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    if(dsc->grad.dir == LV_GRAD_DIR_NONE) {
        ite_it2d_set_fg_color(&ctx, ite_it2d_color_make(dsc->color.red, dsc->color.green, dsc->color.blue,
                                                        255));

        int32_t bg_w = lv_area_get_width(coords), bg_h = lv_area_get_height(coords);
        int32_t short_side = LV_MIN(bg_w, bg_h);
        int32_t real_radius = LV_MIN(dsc->radius, short_side >> 1);

        if(real_radius == 0) {
            /*Most simple case: just a plain rectangle*/
            lv_area_move(&clipped_coords, -layer->buf_area.x1, -layer->buf_area.y1);

            ite_it2d_rect_t rect;
            rect.x = clipped_coords.x1;
            rect.y = clipped_coords.y1;
            rect.w = lv_area_get_width(&clipped_coords);
            rect.h = lv_area_get_height(&clipped_coords);
            ite_it2d_set_dst_rect(&ctx, &rect);

            ite_it2d_fill(&ctx);
        }
        else {
            ite_it2d_surface_t * surf = lv_draw_ite_it2d_bg_frag_obtain(real_radius);
            if(!surf) {
                return;
            }

            ite_it2d_enable_fg_color(&ctx, true);
            ite_it2d_enable_blending(&ctx, true);

            lv_area_t draw_area = *coords;
            lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

            lv_area_t clip_area = t->clip_area;
            lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

            lv_ite_it2d_draw_corners(&ctx, surf, real_radius, &draw_area, &clip_area, false);
            lv_ite_it2d_draw_borders(&ctx, surf, real_radius, &draw_area, &clip_area, false);
            lv_ite_it2d_draw_center(&ctx, surf, real_radius, &draw_area, &clip_area, false);
        }
    }
    else {
        LV_ASSERT(dsc->radius == 0);
        LV_ASSERT(dsc->grad.stops_count <= 2);

        lv_area_t draw_area = *coords;
        lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

        ite_it2d_rect_t rect;
        rect.x = draw_area.x1;
        rect.y = draw_area.y1;
        rect.w = lv_area_get_width(&draw_area);
        rect.h = lv_area_get_height(&draw_area);
        ite_it2d_set_dst_rect(&ctx, &rect);

        lv_area_move(&clipped_coords, -layer->buf_area.x1, -layer->buf_area.y1);

        ite_it2d_area_t it2d_clip_area;
        it2d_clip_area.x1 = clipped_coords.x1;
        it2d_clip_area.y1 = clipped_coords.y1;
        it2d_clip_area.x2 = clipped_coords.x2;
        it2d_clip_area.y2 = clipped_coords.y2;

        ite_it2d_set_clip_area(&ctx, &it2d_clip_area);
        ite_it2d_enable_clipping(&ctx, true);

        if(dsc->grad.stops[0].opa < LV_OPA_MAX || dsc->grad.stops[1].opa < LV_OPA_MAX) {
            ite_it2d_surface_t * surf = lv_draw_ite_it2d_mask_surf_obtain(LV_DRAW_MASK_ID_GRADIENT, rect.w, rect.h,
                                                                          ITE_IT2D_PIXEL_FORMAT_ARGB_8888);
            if(surf == NULL) {
                return;
            }

            ite_it2d_context_t ctx_grad;
            ite_it2d_ctx_init(&ctx_grad);

            ite_it2d_set_dst_surface(&ctx_grad, surf);

            ite_it2d_set_fg_color(&ctx_grad, ite_it2d_color_make(dsc->grad.stops[0].color.red,
                                                                 dsc->grad.stops[0].color.green, dsc->grad.stops[0].color.blue, dsc->grad.stops[0].opa));

            ite_it2d_set_gradient_end_color(&ctx_grad, ite_it2d_color_make(dsc->grad.stops[1].color.red,
                                                                           dsc->grad.stops[1].color.green, dsc->grad.stops[1].color.blue, dsc->grad.stops[1].opa));

            ite_it2d_set_gradient_dir(&ctx_grad, dsc->grad.dir == LV_GRAD_DIR_VER ? ITE_IT2D_GRAD_DIR_VER : ITE_IT2D_GRAD_DIR_HOR);
            ite_it2d_enable_dithering(&ctx_grad, true);

            ite_it2d_gradient_fill(&ctx_grad);

            ite_it2d_set_src_surface(&ctx, surf);
            ite_it2d_enable_blending(&ctx, true);

            ite_it2d_copy(&ctx);
        }
        else {
            ite_it2d_set_fg_color(&ctx, ite_it2d_color_make(dsc->grad.stops[0].color.red,
                                                            dsc->grad.stops[0].color.green, dsc->grad.stops[0].color.blue, dsc->grad.stops[0].opa));

            ite_it2d_set_gradient_end_color(&ctx, ite_it2d_color_make(dsc->grad.stops[1].color.red,
                                                                      dsc->grad.stops[1].color.green, dsc->grad.stops[1].color.blue, dsc->grad.stops[1].opa));

            ite_it2d_set_gradient_dir(&ctx, dsc->grad.dir == LV_GRAD_DIR_VER ? ITE_IT2D_GRAD_DIR_VER : ITE_IT2D_GRAD_DIR_HOR);
            ite_it2d_enable_dithering(&ctx, true);

            ite_it2d_gradient_fill(&ctx);
        }
    }
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
