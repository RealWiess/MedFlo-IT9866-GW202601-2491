/**
 * @file lv_draw_ite_it2d_line.c
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

#if LV_USE_DRAW_ITE_IT2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t magic;
    int16_t xdiff;
    int16_t ydiff;
    int16_t width;
} lv_draw_line_key_t;

typedef struct {
    uint8_t magic;
    int16_t width;
} lv_draw_line_round_key_t;

typedef struct {
    uint8_t magic;
    int16_t line_width;
    int16_t dash_width;
    int16_t dash_gap;
    int16_t blend_area_w;
    int16_t blend_area_h;
    int16_t dash_start;
} lv_draw_line_dash_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void draw_line_hor(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc, ite_it2d_context_t * ctx);
static void draw_line_ver(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc, ite_it2d_context_t * ctx);
static void draw_line_skew(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc, ite_it2d_context_t * ctx);
static lv_draw_line_key_t line_key_create(int32_t xdiff, int32_t ydiff, int32_t width);
static lv_draw_line_round_key_t line_round_key_create(int32_t width);
static lv_draw_line_dash_key_t line_dash_key_create(int32_t line_width, int32_t dash_width, int32_t dash_gap,
                                                    int32_t blend_area_w, int32_t blend_area_h, int32_t dash_start);
static ite_it2d_surface_t * line_round_surf_obtain(const lv_draw_line_dsc_t * dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_line(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc)
{
    if(dsc->width == 0) return;
    if(dsc->opa <= LV_OPA_MIN) return;

    if(dsc->p1.x == dsc->p2.x && dsc->p1.y == dsc->p2.y) return;

    lv_area_t clip_line;
    clip_line.x1 = (int32_t)LV_MIN(dsc->p1.x, dsc->p2.x) - dsc->width / 2;
    clip_line.x2 = (int32_t)LV_MAX(dsc->p1.x, dsc->p2.x) + dsc->width / 2;
    clip_line.y1 = (int32_t)LV_MIN(dsc->p1.y, dsc->p2.y) - dsc->width / 2;
    clip_line.y2 = (int32_t)LV_MAX(dsc->p1.y, dsc->p2.y) + dsc->width / 2;

    bool is_common;
    is_common = lv_area_intersect(&clip_line, &clip_line, &t->clip_area);
    if(!is_common) return;

    lv_layer_t * layer = t->target_layer;
    lv_area_move(&clip_line, -layer->buf_area.x1, -layer->buf_area.y1);

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(t->target_layer);
    ite_it2d_set_dst_surface(&ctx, dstsurf);

    ite_it2d_area_t clip_area;
    clip_area.x1 = clip_line.x1;
    clip_area.y1 = clip_line.y1;
    clip_area.x2 = clip_line.x2;
    clip_area.y2 = clip_line.y2;
    ite_it2d_set_clip_area(&ctx, &clip_area);
    ite_it2d_enable_clipping(&ctx, true);

    ite_it2d_color_t color;
    lv_color_to_ite_it2d_color(&dsc->color, &color);
    ite_it2d_set_fg_color(&ctx, color);
    ite_it2d_enable_fg_color(&ctx, true);

    if(dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    if((int32_t)dsc->p1.y == (int32_t)dsc->p2.y) draw_line_hor(t, dsc, &ctx);
    else if((int32_t)dsc->p1.x == (int32_t)dsc->p2.x) draw_line_ver(t, dsc, &ctx);
    else draw_line_skew(t, dsc, &ctx);

    if(dsc->round_end || dsc->round_start) {
        ite_it2d_surface_t * surf = line_round_surf_obtain(dsc);
        if(surf) {
            lv_layer_t * layer = t->target_layer;
            int32_t r = (dsc->width >> 1);
            int32_t r_corr = (dsc->width & 1) ? 0 : 1;
            lv_area_t cir_area;
            ite_it2d_rect_t rect;

            ite_it2d_enable_blending(&ctx, true);
            ite_it2d_set_src_surface(&ctx, surf);

            if(dsc->round_start) {
                cir_area.x1 = (int32_t)dsc->p1.x - r;
                cir_area.y1 = (int32_t)dsc->p1.y - r;
                cir_area.x2 = (int32_t)dsc->p1.x + r - r_corr;
                cir_area.y2 = (int32_t)dsc->p1.y + r - r_corr ;
                lv_area_move(&cir_area, -layer->buf_area.x1, -layer->buf_area.y1);

                rect.x = cir_area.x1;
                rect.y = cir_area.y1;
                rect.w = lv_area_get_width(&cir_area);
                rect.h = lv_area_get_height(&cir_area);
                ite_it2d_set_dst_rect(&ctx, &rect);

                ite_it2d_copy(&ctx);
            }

            if(dsc->round_end) {
                cir_area.x1 = (int32_t)dsc->p2.x - r;
                cir_area.y1 = (int32_t)dsc->p2.y - r;
                cir_area.x2 = (int32_t)dsc->p2.x + r - r_corr;
                cir_area.y2 = (int32_t)dsc->p2.y + r - r_corr ;
                lv_area_move(&cir_area, -layer->buf_area.x1, -layer->buf_area.y1);

                rect.x = cir_area.x1;
                rect.y = cir_area.y1;
                rect.w = lv_area_get_width(&cir_area);
                rect.h = lv_area_get_height(&cir_area);
                ite_it2d_set_dst_rect(&ctx, &rect);

                ite_it2d_copy(&ctx);
            }
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void draw_line_hor(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc, ite_it2d_context_t * ctx)
{
    int32_t w = dsc->width - 1;
    int32_t w_half0 = w >> 1;
    int32_t w_half1 = w_half0 + (w & 0x1); /*Compensate rounding error*/

    lv_area_t blend_area;
    blend_area.x1 = (int32_t)LV_MIN(dsc->p1.x, dsc->p2.x);
    blend_area.x2 = (int32_t)LV_MAX(dsc->p1.x, dsc->p2.x)  - 1;
    blend_area.y1 = (int32_t)dsc->p1.y - w_half1;
    blend_area.y2 = (int32_t)dsc->p1.y + w_half0;

    bool is_common;
    is_common = lv_area_intersect(&blend_area, &blend_area, &t->clip_area);
    if(!is_common) return;

    bool dashed = dsc->dash_gap && dsc->dash_width;

    lv_layer_t * layer = t->target_layer;
    lv_area_move(&blend_area, -layer->buf_area.x1, -layer->buf_area.y1);

    ite_it2d_rect_t rect;
    rect.x = blend_area.x1;
    rect.y = blend_area.y1;
    rect.w = lv_area_get_width(&blend_area);
    rect.h = lv_area_get_height(&blend_area);
    ite_it2d_set_dst_rect(ctx, &rect);

    /*If there is no mask then simply draw a rectangle*/
    if(!dashed) {
        ite_it2d_fill(ctx);
    }
    else {
        int32_t blend_area_w = lv_area_get_width(&blend_area);
        int32_t blend_area_h = lv_area_get_height(&blend_area);
        int32_t dash_start = blend_area.x1 % (dsc->dash_gap + dsc->dash_width);
        lv_draw_line_dash_key_t key = line_dash_key_create(dsc->width, dsc->dash_width, dsc->dash_gap, blend_area_w,
                                                           blend_area_h, dash_start);
        ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
        if(surf == NULL) {
            surf = lv_zalloc(sizeof(ite_it2d_surface_t));
            LV_ASSERT_MALLOC(surf);
            surf->width = blend_area_w;
            surf->height = blend_area_h;
            surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;

            if(!lv_draw_ite_it2d_create_surface(surf)) {
                lv_free(surf);
                return;
            }

            lv_opa_t * mask_buf = surf->buf;
            int32_t h;
            for(h = blend_area.y1; h <= blend_area.y2; h++) {
                lv_memset(mask_buf, 0xff, blend_area_w);

                int32_t dash_cnt = dash_start;
                int32_t i;
                for(i = 0; i < blend_area_w; i++, dash_cnt++) {
                    if(dash_cnt <= dsc->dash_width) {
                        int16_t diff = dsc->dash_width - dash_cnt;
                        i += diff;
                        dash_cnt += diff;
                    }
                    else if(dash_cnt > dsc->dash_gap + dsc->dash_width) {
                        dash_cnt = 0;
                    }
                    else {
                        mask_buf[i] = 0x00;
                    }
                }
                mask_buf += blend_area_w;
            }
            lv_sys_cache_data_flush_range(surf->buf, surf->pitch * surf->height);
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }
        ite_it2d_enable_blending(ctx, true);
        ite_it2d_set_src_surface(ctx, surf);

        ite_it2d_copy(ctx);
    }
}

static void draw_line_ver(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc, ite_it2d_context_t * ctx)
{
    int32_t w = dsc->width - 1;
    int32_t w_half0 = w >> 1;
    int32_t w_half1 = w_half0 + (w & 0x1); /*Compensate rounding error*/

    lv_area_t blend_area;
    blend_area.x1 = (int32_t)dsc->p1.x - w_half1;
    blend_area.x2 = (int32_t)dsc->p1.x + w_half0;
    blend_area.y1 = (int32_t)LV_MIN(dsc->p1.y, dsc->p2.y);
    blend_area.y2 = (int32_t)LV_MAX(dsc->p1.y, dsc->p2.y) - 1;

    bool is_common;
    is_common = lv_area_intersect(&blend_area, &blend_area, &t->clip_area);
    if(!is_common) return;

    bool dashed = dsc->dash_gap && dsc->dash_width;

    lv_layer_t * layer = t->target_layer;
    lv_area_move(&blend_area, -layer->buf_area.x1, -layer->buf_area.y1);

    ite_it2d_rect_t rect;
    rect.x = blend_area.x1;
    rect.y = blend_area.y1;
    rect.w = lv_area_get_width(&blend_area);
    rect.h = lv_area_get_height(&blend_area);
    ite_it2d_set_dst_rect(ctx, &rect);

    /*If there is no mask then simply draw a rectangle*/
    if(!dashed) {
        ite_it2d_fill(ctx);
    }
    else {
        int32_t draw_area_w = lv_area_get_width(&blend_area);
        int32_t draw_area_h = lv_area_get_height(&blend_area);
        int32_t dash_start = (blend_area.y1) % (dsc->dash_gap + dsc->dash_width);
        lv_draw_line_dash_key_t key = line_dash_key_create(dsc->width, dsc->dash_width, dsc->dash_gap, draw_area_w, draw_area_h,
                                                           dash_start);
        ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
        if(surf == NULL) {
            surf = lv_zalloc(sizeof(ite_it2d_surface_t));
            LV_ASSERT_MALLOC(surf);
            surf->width = draw_area_w;
            surf->height = draw_area_h;
            surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;

            if(!lv_draw_ite_it2d_create_surface(surf)) {
                lv_free(surf);
                return;
            }

            lv_opa_t * mask_buf = surf->buf;
            int32_t dash_cnt = dash_start;
            int32_t h;
            for(h = blend_area.y1; h <= blend_area.y2; h++) {
                if(dash_cnt > dsc->dash_width) {
                    lv_memzero(mask_buf, draw_area_w);
                }
                else {
                    lv_memset(mask_buf, 0xff, draw_area_w);
                }

                if(dash_cnt >= dsc->dash_gap + dsc->dash_width) {
                    dash_cnt = 0;
                }
                dash_cnt ++;
                mask_buf += draw_area_w;
            }
            lv_sys_cache_data_flush_range(surf->buf, surf->pitch * surf->height);
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }
        ite_it2d_enable_blending(ctx, true);
        ite_it2d_set_src_surface(ctx, surf);

        ite_it2d_copy(ctx);
    }
}

static void LV_ATTRIBUTE_FAST_MEM draw_line_skew(lv_draw_task_t * t, const lv_draw_line_dsc_t * dsc,
                                                 ite_it2d_context_t * ctx)
{
    lv_point_t p1;
    lv_point_t p2;
    if(dsc->p1.y < dsc->p2.y) {
        p1 = lv_point_from_precise(&dsc->p1);
        p2 = lv_point_from_precise(&dsc->p2);
    }
    else {
        p1 = lv_point_from_precise(&dsc->p2);
        p2 = lv_point_from_precise(&dsc->p1);
    }

    if(dsc->width == 1) {
        ite_it2d_set_line_start(ctx, p1.x, p1.y);
        ite_it2d_set_line_end(ctx, p2.x, p2.y);
        ite_it2d_set_line_width(ctx, dsc->width);
        ite_it2d_enable_line_antialiasing(ctx, true);
        ite_it2d_line(ctx);
    }
    else {
        int32_t xdiff = p2.x - p1.x;
        int32_t ydiff = p2.y - p1.y;
        bool flat = LV_ABS(xdiff) > LV_ABS(ydiff);

        static const uint8_t wcorr[] = {
            128, 128, 128, 129, 129, 130, 130, 131,
            132, 133, 134, 135, 137, 138, 140, 141,
            143, 145, 147, 149, 151, 153, 155, 158,
            160, 162, 165, 167, 170, 173, 175, 178,
            181,
        };

        int32_t w = dsc->width;
        int32_t wcorr_i = 0;
        if(flat) wcorr_i = (LV_ABS(ydiff) << 5) / LV_ABS(xdiff);
        else wcorr_i = (LV_ABS(xdiff) << 5) / LV_ABS(ydiff);

        w = (w * wcorr[wcorr_i] + 63) >> 7;     /*+ 63 for rounding*/
        int32_t w_half0 = w >> 1;
        int32_t w_half1 = w_half0 + (w & 0x1); /*Compensate rounding error*/

        lv_area_t blend_area;
        blend_area.x1 = LV_MIN(p1.x, p2.x) - w;
        blend_area.x2 = LV_MAX(p1.x, p2.x) + w;
        blend_area.y1 = LV_MIN(p1.y, p2.y) - w;
        blend_area.y2 = LV_MAX(p1.y, p2.y) + w;

        /*Get the union of `coords` and `clip`*/
        /*`clip` is already truncated to the `draw_buf` size
        *in 'lv_refr_area' function*/
        lv_layer_t * layer = t->target_layer;
        lv_area_t clip_area;
        bool is_common = lv_area_intersect(&clip_area, &blend_area, &t->clip_area);
        if(is_common == false) return;

        lv_area_t draw_area = blend_area;
        lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

        lv_draw_line_key_t key = line_key_create(xdiff, ydiff, dsc->width);
        ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
        if(surf == NULL) {
            lv_draw_sw_mask_line_param_t mask_left_param;
            lv_draw_sw_mask_line_param_t mask_right_param;
            lv_draw_sw_mask_line_param_t mask_top_param;
            lv_draw_sw_mask_line_param_t mask_bottom_param;

            void * masks[5] = {&mask_left_param, & mask_right_param, NULL, NULL, NULL};

            if(flat) {
                if(xdiff > 0) {
                    lv_draw_sw_mask_line_points_init(&mask_left_param, p1.x, p1.y - w_half0, p2.x, p2.y - w_half0,
                                                     LV_DRAW_SW_MASK_LINE_SIDE_LEFT);
                    lv_draw_sw_mask_line_points_init(&mask_right_param, p1.x, p1.y + w_half1, p2.x, p2.y + w_half1,
                                                     LV_DRAW_SW_MASK_LINE_SIDE_RIGHT);
                }
                else {
                    lv_draw_sw_mask_line_points_init(&mask_left_param, p1.x, p1.y + w_half1, p2.x, p2.y + w_half1,
                                                     LV_DRAW_SW_MASK_LINE_SIDE_LEFT);
                    lv_draw_sw_mask_line_points_init(&mask_right_param, p1.x, p1.y - w_half0, p2.x, p2.y - w_half0,
                                                     LV_DRAW_SW_MASK_LINE_SIDE_RIGHT);
                }
            }
            else {
                lv_draw_sw_mask_line_points_init(&mask_left_param, p1.x + w_half1, p1.y, p2.x + w_half1, p2.y,
                                                 LV_DRAW_SW_MASK_LINE_SIDE_LEFT);
                lv_draw_sw_mask_line_points_init(&mask_right_param, p1.x - w_half0, p1.y, p2.x - w_half0, p2.y,
                                                 LV_DRAW_SW_MASK_LINE_SIDE_RIGHT);

            }

            /*Use the normal vector for the endings*/
            if(!dsc->raw_end) {
                lv_draw_sw_mask_line_points_init(&mask_top_param, p1.x, p1.y, p1.x - ydiff, p1.y + xdiff,
                                                 LV_DRAW_SW_MASK_LINE_SIDE_BOTTOM);
                lv_draw_sw_mask_line_points_init(&mask_bottom_param, p2.x, p2.y, p2.x - ydiff, p2.y + xdiff,
                                                 LV_DRAW_SW_MASK_LINE_SIDE_TOP);
                masks[2] = &mask_top_param;
                masks[3] = &mask_bottom_param;
            }

            surf = lv_draw_ite_it2d_mask_dump_surf(&blend_area, masks);
            if(surf) {
                lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
            }

            lv_draw_sw_mask_free_param(&mask_left_param);
            lv_draw_sw_mask_free_param(&mask_right_param);
            if(!dsc->raw_end) {
                lv_draw_sw_mask_free_param(&mask_top_param);
                lv_draw_sw_mask_free_param(&mask_bottom_param);
            }
        }

        if(surf == NULL) return;

        ite_it2d_rect_t rect;
        rect.x = draw_area.x1;
        rect.y = draw_area.y1;
        rect.w = lv_area_get_width(&draw_area);
        rect.h = lv_area_get_height(&draw_area);
        ite_it2d_set_dst_rect(ctx, &rect);

        ite_it2d_enable_blending(ctx, true);
        ite_it2d_set_src_surface(ctx, surf);

        ite_it2d_copy(ctx);
    }
}

static lv_draw_line_key_t line_key_create(int32_t xdiff, int32_t ydiff, int32_t width)
{
    lv_draw_line_key_t key;
    lv_memzero(&key, sizeof(lv_draw_line_key_t));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE;
    key.xdiff = xdiff;
    key.ydiff = ydiff;
    key.width = width;
    return key;
}

static lv_draw_line_round_key_t line_round_key_create(int32_t width)
{
    lv_draw_line_round_key_t key;
    lv_memzero(&key, sizeof(lv_draw_line_round_key_t));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE_ROUNDED;
    key.width = width;
    return key;
}

static lv_draw_line_dash_key_t line_dash_key_create(int32_t line_width, int32_t dash_width, int32_t dash_gap,
                                                    int32_t blend_area_w, int32_t blend_area_h, int32_t dash_start)
{
    lv_draw_line_dash_key_t key;
    lv_memzero(&key, sizeof(lv_draw_line_dash_key_t));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_LINE_DASHED;
    key.line_width = line_width;
    key.dash_width = dash_width;
    key.dash_gap = dash_gap;
    key.blend_area_w = blend_area_w;
    key.blend_area_h = blend_area_h;
    key.dash_start = dash_start;
    return key;
}

static ite_it2d_surface_t * line_round_surf_obtain(const lv_draw_line_dsc_t * dsc)
{
    lv_draw_line_round_key_t key = line_round_key_create(dsc->width);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(surf == NULL) {
        lv_draw_sw_mask_radius_param_t param;
        lv_area_t round_area = {0, 0, dsc->width - 1, dsc->width - 1};
        lv_draw_sw_mask_radius_init(&param, &round_area, LV_RADIUS_CIRCLE, false);
        void * masks[2] = {0};
        masks[0] = &param;
        surf = lv_draw_ite_it2d_mask_dump_surf(&round_area, masks);
        if(surf) {
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }
        lv_draw_sw_mask_free_param(&param);
    }
    return surf;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
