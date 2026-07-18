/**
 * @file lv_draw_ite_it2d_img.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_area_private.h"
#include "../lv_image_decoder_private.h"
#include "../lv_draw_image_private.h"
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

typedef struct {
    uint8_t magic;
    const void * src;
} lv_draw_img_key_t;

typedef struct {
    uint8_t magic;
    const void * src;
    int16_t w, h, radius;
} lv_draw_rounded_key_t;

enum {
    ROUNDED_IMG_PART_LEFT = 0,
    ROUNDED_IMG_PART_HCENTER = 1,
    ROUNDED_IMG_PART_RIGHT = 2,
    ROUNDED_IMG_PART_TOP = 3,
    ROUNDED_IMG_PART_VCENTER = 4,
    ROUNDED_IMG_PART_BOTTOM = 5,
};

enum {
    ROUNDED_IMG_CORNER_TOP_LEFT = 0,
    ROUNDED_IMG_CORNER_TOP_RIGHT = 1,
    ROUNDED_IMG_CORNER_BOTTOM_RIGHT = 2,
    ROUNDED_IMG_CORNER_BOTTOM_LEFT = 3,
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void img_draw_core(lv_draw_task_t * t, const lv_draw_image_dsc_t * draw_dsc,
                          const lv_image_decoder_dsc_t * decoder_dsc, lv_draw_image_sup_t * sup,
                          const lv_area_t * img_coords, const lv_area_t * clipped_img_area);

static ite_it2d_surface_t * upload_img_surf(const lv_image_decoder_dsc_t * dsc);
static lv_draw_img_key_t img_key_create(const void * src);
static ite_it2d_surface_t * apply_recolor_opa(const lv_draw_image_dsc_t * draw_dsc, ite_it2d_surface_t * surf);
static ite_it2d_surface_t * rounded_frag_obtain(ite_it2d_surface_t * surf, int32_t w, int32_t h, int32_t radius);
static lv_draw_rounded_key_t rounded_key_create(const void * src, int32_t w, int32_t h, int32_t radius);
static void calc_draw_part(ite_it2d_surface_t * surf, const lv_area_t * coords, ite_it2d_rect_t * clipped_src,
                           ite_it2d_rect_t * clipped_dst);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_image(lv_draw_task_t * t, const lv_draw_image_dsc_t * draw_dsc,
                            const lv_area_t * coords)
{
    if(!draw_dsc->tile) {
        lv_draw_image_normal_helper(t, draw_dsc, coords, img_draw_core);
    }
    else {
        lv_draw_image_tiled_helper(t, draw_dsc, coords, img_draw_core);
    }
}

ite_it2d_surface_t * lv_draw_ite_it2d_img_surf_obtain(const lv_image_decoder_dsc_t * decoder_dsc)
{
    lv_draw_img_key_t key = img_key_create(decoder_dsc->src);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(!surf) {
        surf = upload_img_surf(decoder_dsc);
        if(!surf) {
            return NULL;
        }
        lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
    }

    return surf;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void img_draw_core(lv_draw_task_t * t, const lv_draw_image_dsc_t * draw_dsc,
                          const lv_image_decoder_dsc_t * decoder_dsc, lv_draw_image_sup_t * sup,
                          const lv_area_t * img_coords, const lv_area_t * clipped_img_area)
{
    bool transformed = draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                       draw_dsc->scale_y != LV_SCALE_NONE ? true : false;

    lv_draw_buf_t * decoded = (lv_draw_buf_t *)decoder_dsc->decoded;
    uint32_t img_stride = decoded->header.stride;
    lv_color_format_t cf = decoded->header.cf;
    lv_area_t draw_area = *img_coords;
    lv_area_t clip_area = *clipped_img_area;
    lv_layer_t * layer = t->target_layer;
    ite_it2d_surface_t srcsurf;
    ite_it2d_surface_t * surf;

    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t * dstsurf = lv_layer_get_ite_it2d_surface(layer);
    ite_it2d_set_dst_surface(&ctx, dstsurf);

    if(draw_dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, draw_dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    ite_it2d_enable_blending(&ctx, true);

    if(lv_ite_it2d_is_hw_cf_supported(cf) || (decoded->header.flags & LV_DRAW_ITE_IT2D_READY)) {
        lv_memzero(&srcsurf, sizeof(ite_it2d_surface_t));
        srcsurf.width = decoded->header.w;
        srcsurf.height = decoded->header.h;
        srcsurf.pitch = img_stride;
        srcsurf.flags.format = lv_color_format_to_ite_it2d_pixel_format(cf);
        srcsurf.buf = decoded->data;
        srcsurf.id = ite_it2d_get_current_id();
        surf = &srcsurf;

        if(!(decoded->header.flags & LV_DRAW_ITE_IT2D_READY)) {
            lv_sys_cache_data_flush_range(decoded->data, decoded->data_size);
            decoded->header.flags |= LV_DRAW_ITE_IT2D_READY;
        }
    }
    else {
        surf = lv_draw_ite_it2d_img_surf_obtain(decoder_dsc);
        if(!surf) {
            return;
        }
    }

    const int w = lv_area_get_width(&draw_area), h = lv_area_get_height(&draw_area);
    int32_t short_side = LV_MIN(w, h);
    int32_t real_radius = LV_MIN(draw_dsc->clip_radius, short_side >> 1);

    if(real_radius == 0) {
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

        surf = apply_recolor_opa(draw_dsc, surf);
        ite_it2d_set_src_surface(&ctx, surf);

        if(!transformed) {
            ite_it2d_copy(&ctx);
        }
        else {
            if(draw_dsc->antialias) {
                ite_it2d_enable_interpolation(&ctx, true);
            }

            ite_it2d_transform_dsc_t xform_dsc;
            ite_it2d_init_transform_dsc(&xform_dsc);
            xform_dsc.center.x = draw_dsc->pivot.x;
            xform_dsc.center.y = draw_dsc->pivot.y;
            xform_dsc.scale_x = (float)draw_dsc->scale_x / LV_SCALE_NONE;
            xform_dsc.scale_y = (float)draw_dsc->scale_y / LV_SCALE_NONE;
            xform_dsc.pos.x = xform_dsc.center.x * (1.0f - xform_dsc.scale_x);
            xform_dsc.pos.y = xform_dsc.center.y * (1.0f - xform_dsc.scale_y);
            xform_dsc.angle = draw_dsc->rotation / 10.0f;
            xform_dsc.flags.auto_dst_rect = 1;
            ite_it2d_transform_copy(&ctx, &xform_dsc);
        }
    }
    else {
        ite_it2d_surface_t * frag = rounded_frag_obtain(surf, w, h, real_radius);
        if(!frag) {
            return;
        }

        frag = apply_recolor_opa(draw_dsc, frag);
        lv_ite_it2d_draw_corners(&ctx, frag, real_radius, &draw_area, &clip_area, true);

        surf = apply_recolor_opa(draw_dsc, surf);

        ite_it2d_rect_t src_rect, dst_rect;
        /* Draw 3 parts */
        lv_area_t clip_tmp, part;
        calc_draw_part(surf, &draw_area, &src_rect, &dst_rect);
        ite_it2d_set_src_surface(&ctx, surf);
        ite_it2d_set_dst_rect(&ctx, &dst_rect);
        ite_it2d_set_src_rect(&ctx, &src_rect);
        ite_it2d_enable_clipping(&ctx, true);
        for(int i = w > h ? ROUNDED_IMG_PART_LEFT : ROUNDED_IMG_PART_TOP, j = i + 3; i < j; i++) {
            switch(i) {
                case ROUNDED_IMG_PART_LEFT:
                    lv_area_set(&part, draw_area.x1, draw_area.y1 + real_radius, draw_area.x1 + real_radius - 1,
                                draw_area.y2 - real_radius);
                    break;
                case ROUNDED_IMG_PART_HCENTER:
                    lv_area_set(&part, draw_area.x1 + real_radius, draw_area.y1, draw_area.x2 - real_radius,
                                draw_area.y2);
                    break;
                case ROUNDED_IMG_PART_RIGHT:
                    lv_area_set(&part, draw_area.x2 - real_radius + 1, draw_area.y1 + real_radius, draw_area.x2,
                                draw_area.y2 - real_radius);
                    break;
                case ROUNDED_IMG_PART_TOP:
                    lv_area_set(&part, draw_area.x1 + real_radius, draw_area.y1, draw_area.x2 - real_radius,
                                draw_area.y1 + real_radius - 1);
                    break;
                case ROUNDED_IMG_PART_VCENTER:
                    lv_area_set(&part, draw_area.x1 + real_radius, draw_area.y2 - real_radius + 1,
                                draw_area.x2 - real_radius, draw_area.y2);
                    break;
                case ROUNDED_IMG_PART_BOTTOM:
                    lv_area_set(&part, draw_area.x1, draw_area.y1 + real_radius, draw_area.x2,
                                draw_area.y2 - real_radius);
                    break;
                default:
                    break;
            }
            if(!lv_area_intersect(&clip_tmp, &part, &clip_area)) continue;

            ite_it2d_area_t it2d_clip_area;
            it2d_clip_area.x1 = clip_tmp.x1;
            it2d_clip_area.y1 = clip_tmp.y1;
            it2d_clip_area.x2 = clip_tmp.x2;
            it2d_clip_area.y2 = clip_tmp.y2;
            ite_it2d_set_clip_area(&ctx, &it2d_clip_area);

            ite_it2d_copy(&ctx);
        }
    }
}

static ite_it2d_surface_t * upload_img_surf(const lv_image_decoder_dsc_t * dsc)
{
    const lv_draw_buf_t * decoded = dsc->decoded;
    ite_it2d_surface_t * surf = lv_zalloc(sizeof(ite_it2d_surface_t));
    LV_ASSERT_MALLOC(surf);
    surf->width = decoded->header.w;
    surf->height = decoded->header.h;

    if((decoded->header.cf == LV_COLOR_FORMAT_L8) || (decoded->header.cf == LV_COLOR_FORMAT_RGB565A8) ||
       (decoded->header.cf == LV_COLOR_FORMAT_XRGB8888)) {
        surf->flags.format = ITE_IT2D_PIXEL_FORMAT_ARGB_8888;
        if(!lv_draw_ite_it2d_create_surface(surf)) {
            lv_free(surf);
            return NULL;
        }
        if(decoded->header.cf == LV_COLOR_FORMAT_L8) {
            lv_ite_it2d_l8_to_a8888(surf->buf, decoded->data, surf->width, surf->height);
        }
        else if(decoded->header.cf == LV_COLOR_FORMAT_RGB565A8) {
            lv_ite_it2d_rgb565a8_to_argb8888(surf->buf, decoded->data, surf->width, surf->height);
        }
        else {
            lv_ite_it2d_x8888_to_a8888(surf->buf, decoded->data, surf->width, surf->height);
        }
        lv_sys_cache_data_flush_range(surf->buf, surf->pitch * surf->height);
        return surf;
    }
    else {
        uint8_t bpp;
        switch(decoded->header.cf) {
            case LV_COLOR_FORMAT_A8:
                bpp = 8;
                break;
            case LV_COLOR_FORMAT_A4:
                bpp = 4;
                break;
            case LV_COLOR_FORMAT_A2:
                bpp = 2;
                break;
            case LV_COLOR_FORMAT_A1:
                bpp = 1;
                break;
            default:
                bpp = 0;
                break;
        }

        if(bpp > 0) {
            surf->flags.format = lv_color_format_to_ite_it2d_pixel_format(decoded->header.cf);
            if(!lv_draw_ite_it2d_create_surface(surf)) {
                lv_free(surf);
                return NULL;
            }
            lv_ite_it2d_to_a8421(surf->buf, decoded->data, surf->width, surf->height, surf->pitch, bpp);
            lv_sys_cache_data_flush_range(surf->buf, surf->pitch * surf->height);
            return surf;
        }
        else {
            lv_free(surf);
        }
    }
    return NULL;
}

static lv_draw_img_key_t img_key_create(const void * src)
{
    lv_draw_img_key_t key;
    lv_memzero(&key, sizeof(lv_draw_img_key_t));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_IMG;
    key.src = src;
    return key;
}

static ite_it2d_surface_t * apply_recolor_opa(const lv_draw_image_dsc_t * draw_dsc, ite_it2d_surface_t * surf)
{
    if(draw_dsc->recolor_opa <= LV_OPA_MIN || draw_dsc->header.cf == LV_COLOR_FORMAT_L8) return surf;

    ite_it2d_surface_t * recolor_surf = lv_draw_ite_it2d_mask_surf_obtain(LV_DRAW_MASK_ID_RECOLOR, surf->width,
                                                                          surf->height,
                                                                          surf->flags.format);
    if(recolor_surf) {
        ite_it2d_context_t ctx;
        ite_it2d_ctx_init(&ctx);

        ite_it2d_set_dst_surface(&ctx, recolor_surf);

        ite_it2d_color_t color;
        lv_color_to_ite_it2d_color(&draw_dsc->recolor, &color);
        ite_it2d_set_fg_color(&ctx, color);

        ite_it2d_fill(&ctx);
        ite_it2d_enable_fg_color(&ctx, false);

        ite_it2d_set_src_surface(&ctx, surf);
        ite_it2d_set_const_alpha(&ctx, 255 - draw_dsc->recolor_opa);
        ite_it2d_enable_const_alpha(&ctx, true);
        ite_it2d_set_alpha_dst(&ctx, ITE_IT2D_ALPHA_SRC);
        ite_it2d_copy(&ctx);

        return recolor_surf;
    }

    return surf;
}

static ite_it2d_surface_t * rounded_frag_obtain(ite_it2d_surface_t * surf, int32_t w, int32_t h, int32_t radius)
{
    lv_draw_rounded_key_t key = rounded_key_create(surf->buf, w, h, radius);
    ite_it2d_surface_t * img_frag = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(img_frag == NULL) {
        const int32_t full_frag_size = radius * 2 + 3;
        ite_it2d_surface_t * mask_frag = lv_draw_ite_it2d_bg_frag_obtain(radius);
        if(mask_frag == NULL) {
            return NULL;
        }

        ite_it2d_surface_t * img_mask_frag = lv_draw_ite_it2d_mask_surf_obtain(LV_DRAW_MASK_ID_FRAG, full_frag_size,
                                                                               full_frag_size,
                                                                               ITE_IT2D_PIXEL_FORMAT_A_8);
        if(img_mask_frag == NULL) {
            return NULL;
        }

        img_frag = lv_zalloc(sizeof(ite_it2d_surface_t));
        LV_ASSERT_MALLOC(img_frag);
        img_frag->width = full_frag_size;
        img_frag->height = full_frag_size;
        img_frag->flags.format = ITE_IT2D_PIXEL_FORMAT_ARGB_8888;

        if(!lv_draw_ite_it2d_create_surface(img_frag)) {
            lv_free(img_frag);
            return NULL;
        }

        ite_it2d_context_t ctx;
        ite_it2d_ctx_init(&ctx);

        lv_area_t coords = {0, 0, w - 1, h - 1};
        lv_area_t frag_coords = {0, 0, full_frag_size - 1, full_frag_size - 1};
        ite_it2d_set_dst_surface(&ctx, img_mask_frag);
        lv_ite_it2d_draw_corners(&ctx, mask_frag, radius, &frag_coords, NULL, false);

        ite_it2d_rect_t srcrect, dstrect = {0, 0, radius, radius};
        ite_it2d_area_t clip_area;

        ite_it2d_set_dst_surface(&ctx, img_frag);
        ite_it2d_set_src_surface(&ctx, surf);
        ite_it2d_enable_clipping(&ctx, true);

        for(int i = 0; i <= ROUNDED_IMG_CORNER_BOTTOM_LEFT; i++) {
            switch(i) {
                case ROUNDED_IMG_CORNER_TOP_LEFT:
                    clip_area.x1 = 0;
                    clip_area.y1 = 0;
                    lv_area_align(&frag_coords, &coords, LV_ALIGN_TOP_LEFT, 0, 0);
                    break;
                case ROUNDED_IMG_CORNER_TOP_RIGHT:
                    clip_area.x1 = full_frag_size - radius;
                    clip_area.y1 = 0;
                    lv_area_align(&frag_coords, &coords, LV_ALIGN_TOP_RIGHT, 0, 0);
                    break;
                case ROUNDED_IMG_CORNER_BOTTOM_RIGHT:
                    clip_area.x1 = full_frag_size - radius;
                    clip_area.y1 = full_frag_size - radius;
                    lv_area_align(&frag_coords, &coords, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
                    break;
                case ROUNDED_IMG_CORNER_BOTTOM_LEFT:
                    clip_area.x1 = 0;
                    clip_area.y1 = full_frag_size - radius;
                    lv_area_align(&frag_coords, &coords, LV_ALIGN_BOTTOM_LEFT, 0, 0);
                    break;
                default:
                    break;
            }
            clip_area.x2 = clip_area.x1 + radius - 1;
            clip_area.y2 = clip_area.y1 + radius - 1;
            calc_draw_part(surf, &coords, &srcrect, &dstrect);
            ite_it2d_set_clip_area(&ctx, &clip_area);
            ite_it2d_set_dst_rect(&ctx, &dstrect);
            ite_it2d_set_src_rect(&ctx, &srcrect);
            ite_it2d_copy(&ctx);
        }

        ite_it2d_wait_surface_finish(img_frag);
        lv_sys_cache_data_invd_range(img_frag->buf, img_frag->pitch * img_frag->height);
        lv_sys_cache_data_invd_range(img_mask_frag->buf, img_mask_frag->pitch * img_mask_frag->height);
        lv_ite_it2d_a8888_mix_a8(img_frag->buf, img_mask_frag->buf, full_frag_size, full_frag_size);
        lv_sys_cache_data_flush_range(img_frag->buf, img_frag->pitch * img_frag->height);

        lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), img_frag);
    }
    return img_frag;
}

static lv_draw_rounded_key_t rounded_key_create(const void * src, int32_t w, int32_t h, int32_t radius)
{
    lv_draw_rounded_key_t key;
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_IMG_ROUNDED_CORNERS;
    key.src = src;
    key.w = w;
    key.h = h;
    key.radius = radius;
    return key;
}

static void calc_draw_part(ite_it2d_surface_t * surf, const lv_area_t * coords, ite_it2d_rect_t * clipped_src,
                           ite_it2d_rect_t * clipped_dst)
{
    float x = 0, y = 0, w, h;
    w = surf->width;
    h = surf->height;

    lv_area_to_ite_it2d_rect(coords, clipped_dst);

    int32_t coords_w = lv_area_get_width(coords), coords_h = lv_area_get_height(coords);
    clipped_src->x = (int16_t)(x + (clipped_dst->x - coords->x1) * w / coords_w);
    clipped_src->y = (int16_t)(y + (clipped_dst->y - coords->y1) * h / coords_h);
    clipped_src->w = (int16_t)(w - (coords_w - clipped_dst->w) * w / coords_w);
    clipped_src->h = (int16_t)(h - (coords_h - clipped_dst->h) * h / coords_h);
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
