/**
 * @file lv_draw_ite_it2d_arc.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_area_private.h"
#include "../sw/lv_draw_sw_mask_private.h"
#include "../lv_image_decoder_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_img.h"
#include "lv_draw_ite_it2d_mask.h"
#if LV_USE_DRAW_ITE_IT2D

#include "../lv_draw_private.h"

static void get_rounded_area(int16_t angle, int32_t radius, uint8_t thickness, lv_area_t * res_area);

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t magic;
    int16_t width;
    int16_t start_angle;
    int16_t end_angle;
    uint16_t radius;
} lv_draw_arc_key_t;

typedef struct {
    uint8_t magic;
    int16_t width;
} lv_draw_arc_rounded_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_draw_arc_key_t arc_key_create(int32_t width, int32_t start_angle, int32_t end_angle, uint16_t radius);
static lv_draw_arc_rounded_key_t arc_rounded_key_create(int32_t width);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_arc(lv_draw_task_t * t, const lv_draw_arc_dsc_t * dsc, const lv_area_t * coords)
{
    if(dsc->opa <= LV_OPA_MIN) return;
    if(dsc->width == 0) return;
    if(dsc->start_angle == dsc->end_angle) return;

    int32_t width = dsc->width;
    if(width > dsc->radius) width = dsc->radius;

    lv_layer_t * layer = t->target_layer;
    lv_area_t area_out = *coords;
    lv_area_t clip_area;
    if(!lv_area_intersect(&clip_area, &area_out, &t->clip_area)) return;

    lv_area_t draw_area = area_out;
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);
#if 0
    /*Draw a full ring*/
    if(dsc->img_src == NULL &&
       (dsc->start_angle + 360 == dsc->end_angle || dsc->start_angle == dsc->end_angle + 360)) {
        lv_draw_border_dsc_t cir_dsc;
        lv_draw_border_dsc_init(&cir_dsc);
        cir_dsc.opa = dsc->opa;
        cir_dsc.color = dsc->color;
        cir_dsc.width = width;
        cir_dsc.radius = LV_RADIUS_CIRCLE;
        cir_dsc.side = LV_BORDER_SIDE_FULL;
        lv_draw_ite_it2d_border(t, &cir_dsc, &area_out);
        return;
    }
#endif
    lv_area_t img_area;
    ite_it2d_surface_t srcsurf;
    ite_it2d_surface_t * img_surf = NULL;

    if(dsc->img_src != NULL) {
        lv_image_decoder_dsc_t decoder_dsc;
        lv_result_t res = lv_image_decoder_open(&decoder_dsc, dsc->img_src, NULL);
        if(res == LV_RESULT_INVALID || decoder_dsc.decoded == NULL) {
            LV_LOG_WARN("Can't decode the background image");
        }
        else {
            lv_draw_buf_t * decoded = (lv_draw_buf_t *)decoder_dsc.decoded;
            uint32_t img_stride = decoded->header.stride;
            lv_color_format_t cf = decoded->header.cf;

            img_area.x1 = decoder_dsc.decoded->header.w / 2;
            img_area.y1 = decoder_dsc.decoded->header.h / 2;
            lv_area_move(&img_area, -dsc->radius, -dsc->radius);

            if(lv_ite_it2d_is_hw_cf_supported(cf) || (decoded->header.flags & LV_DRAW_ITE_IT2D_READY)) {
                lv_memzero(&srcsurf, sizeof(ite_it2d_surface_t));
                srcsurf.width = decoded->header.w;
                srcsurf.height = decoded->header.h;
                srcsurf.pitch = img_stride;
                srcsurf.flags.format = lv_color_format_to_ite_it2d_pixel_format(cf);
                srcsurf.buf = decoded->data;
                srcsurf.id = ite_it2d_get_current_id();
                img_surf = &srcsurf;

                if(!(decoded->header.flags & LV_DRAW_ITE_IT2D_READY)) {
                    lv_sys_cache_data_flush_range(decoded->data, decoded->data_size);
                    decoded->header.flags |= LV_DRAW_ITE_IT2D_READY;
                }
            }
            else {
                img_surf = lv_draw_ite_it2d_img_surf_obtain(&decoder_dsc);
            }
            lv_image_decoder_close(&decoder_dsc);
        }
    }

    lv_area_t area_in;
    lv_area_copy(&area_in, &area_out);
    area_in.x1 += dsc->width;
    area_in.y1 += dsc->width;
    area_in.x2 -= dsc->width;
    area_in.y2 -= dsc->width;

    int32_t start_angle = (int32_t)dsc->start_angle;
    int32_t end_angle = (int32_t)dsc->end_angle;
    while(start_angle >= 360) start_angle -= 360;
    while(end_angle >= 360) end_angle -= 360;

    lv_draw_arc_key_t key = arc_key_create(width, start_angle, end_angle, dsc->radius);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(surf == NULL) {
        void * mask_list[4] = {0};
        /*Create an angle mask*/
        lv_draw_sw_mask_angle_param_t mask_angle_param;
        lv_draw_sw_mask_angle_init(&mask_angle_param, dsc->center.x, dsc->center.y, start_angle, end_angle);
        mask_list[0] = &mask_angle_param;

        /*Create an outer mask*/
        lv_draw_sw_mask_radius_param_t mask_out_param;
        lv_draw_sw_mask_radius_init(&mask_out_param, &area_out, LV_RADIUS_CIRCLE, false);
        mask_list[1] = &mask_out_param;

        /*Create inner the mask*/
        lv_draw_sw_mask_radius_param_t mask_in_param;
        bool mask_in_param_valid = false;
        if(lv_area_get_width(&area_in) > 0 && lv_area_get_height(&area_in) > 0) {
            lv_draw_sw_mask_radius_init(&mask_in_param, &area_in, LV_RADIUS_CIRCLE, true);
            mask_list[2] = &mask_in_param;
            mask_in_param_valid = true;
        }

        surf = lv_draw_ite_it2d_mask_dump_surf(&area_out, mask_list);
        if(surf) {
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }

        lv_draw_sw_mask_free_param(&mask_angle_param);
        lv_draw_sw_mask_free_param(&mask_out_param);
        if(mask_in_param_valid) {
            lv_draw_sw_mask_free_param(&mask_in_param);
        }
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

    if(dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    ite_it2d_enable_blending(&ctx, true);

    if(img_surf) {
        ite_it2d_set_src_surface(&ctx, img_surf);

        rect.x = img_area.x1;
        rect.y = img_area.y1;
        rect.w = surf->width;
        rect.h = surf->height;
        ite_it2d_set_src_rect(&ctx, &rect);

        ite_it2d_set_mask_surface(&ctx, surf);
        ite_it2d_enable_masking(&ctx, true);
    }
    else {
        ite_it2d_color_t color;
        lv_color_to_ite_it2d_color(&dsc->color, &color);
        ite_it2d_set_fg_color(&ctx, color);
        ite_it2d_enable_fg_color(&ctx, true);

        ite_it2d_set_src_surface(&ctx, surf);
    }
    ite_it2d_copy(&ctx);

    lv_area_t round_area_1;
    lv_area_t round_area_2;

    if(dsc->rounded) {
        lv_draw_arc_rounded_key_t key = arc_rounded_key_create(width);
        surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
        if(surf == NULL) {
            lv_area_t circle_area = {0, 0, width - 1, width - 1};
            lv_draw_sw_mask_radius_param_t circle_mask_param;
            lv_draw_sw_mask_radius_init(&circle_mask_param, &circle_area, width / 2, false);
            void * circle_mask_list[2] = {&circle_mask_param, NULL};

            surf = lv_draw_ite_it2d_mask_dump_surf(&circle_area, circle_mask_list);
            if(surf) {
                lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
            }
        }

        if(surf == NULL) return;

        get_rounded_area(start_angle, dsc->radius, width, &round_area_1);

        if(img_surf) {
            rect.x = img_area.x1 + dsc->radius + round_area_1.x1;
            rect.y = img_area.y1 + dsc->radius + round_area_1.y1;
            rect.w = surf->width;
            rect.h = surf->height;
            ite_it2d_set_src_rect(&ctx, &rect);

            ite_it2d_set_mask_surface(&ctx, surf);
            ite_it2d_set_mask_pos(&ctx, 0, 0);
        }
        else {
            ite_it2d_set_src_surface(&ctx, surf);
        }

        lv_area_move(&round_area_1, dsc->center.x - layer->buf_area.x1, dsc->center.y - layer->buf_area.y1);

        rect.x = round_area_1.x1;
        rect.y = round_area_1.y1;
        rect.w = lv_area_get_width(&round_area_1);
        rect.h = lv_area_get_height(&round_area_1);
        ite_it2d_set_dst_rect(&ctx, &rect);

        ite_it2d_copy(&ctx);

        get_rounded_area(end_angle, dsc->radius, width, &round_area_2);

        if(img_surf) {
            rect.x = img_area.x1 + dsc->radius + round_area_2.x1;
            rect.y = img_area.y1 + dsc->radius + round_area_2.y1;
            rect.w = surf->width;
            rect.h = surf->height;
            ite_it2d_set_src_rect(&ctx, &rect);
            ite_it2d_set_mask_pos(&ctx, 0, 0);
        }

        lv_area_move(&round_area_2, dsc->center.x - layer->buf_area.x1, dsc->center.y - layer->buf_area.y1);

        rect.x = round_area_2.x1;
        rect.y = round_area_2.y1;
        rect.w = lv_area_get_width(&round_area_2);
        rect.h = lv_area_get_height(&round_area_2);
        ite_it2d_set_dst_rect(&ctx, &rect);
        ite_it2d_copy(&ctx);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void get_rounded_area(int16_t angle, int32_t radius, uint8_t thickness, lv_area_t * res_area)
{
    int32_t thick_half = thickness / 2;
    uint8_t thick_corr = (thickness & 0x01) ? 0 : 1;

    int32_t cir_x;
    int32_t cir_y;

    cir_x = ((radius - thick_half) * lv_trigo_cos(angle)) >> (LV_TRIGO_SHIFT - 8);
    cir_y = ((radius - thick_half) * lv_trigo_sin(angle)) >> (LV_TRIGO_SHIFT - 8);

    /*The center of the pixel need to be calculated so apply 1/2 px offset*/
    if(cir_x > 0) {
        cir_x = (cir_x - 128) >> 8;
        res_area->x1 = cir_x - thick_half + thick_corr;
        res_area->x2 = cir_x + thick_half;
    }
    else {
        cir_x = (cir_x + 128) >> 8;
        res_area->x1 = cir_x - thick_half;
        res_area->x2 = cir_x + thick_half - thick_corr;
    }

    if(cir_y > 0) {
        cir_y = (cir_y - 128) >> 8;
        res_area->y1 = cir_y - thick_half + thick_corr;
        res_area->y2 = cir_y + thick_half;
    }
    else {
        cir_y = (cir_y + 128) >> 8;
        res_area->y1 = cir_y - thick_half;
        res_area->y2 = cir_y + thick_half - thick_corr;
    }
}

static lv_draw_arc_key_t arc_key_create(int32_t width, int32_t start_angle, int32_t end_angle, uint16_t radius)
{
    lv_draw_arc_key_t key;
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_ARC;
    key.width = width;
    key.start_angle = start_angle;
    key.end_angle = end_angle;
    key.radius = radius;

    return key;
}

static lv_draw_arc_rounded_key_t arc_rounded_key_create(int32_t width)
{
    lv_draw_arc_rounded_key_t key;
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_ARC_ROUNDED;
    key.width = width;

    return key;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
