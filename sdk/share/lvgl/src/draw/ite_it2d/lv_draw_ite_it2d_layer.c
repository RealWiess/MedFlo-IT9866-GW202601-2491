/**
 * @file lv_draw_ite_it2d_layer.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_area_private.h"
#include "../lv_image_decoder_private.h"
#include "../lv_draw_private.h"
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_img.h"
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

static bool apply_mask(const lv_draw_image_dsc_t * draw_dsc, ite_it2d_context_t * ctx, bool * wait_finish);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_layer(lv_draw_task_t * t, const lv_draw_image_dsc_t * draw_dsc, const lv_area_t * coords)
{
    lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;

    /*It can happen that nothing was draw on a layer and therefore its buffer is not allocated.
     *In this case just return. */
    if(layer_to_draw->draw_buf == NULL) return;

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    bool wait_finish = false;

    if(draw_dsc->bitmap_mask_src) {
        bool visible = apply_mask(draw_dsc, &ctx, &wait_finish);
        if(!visible) return;
    }

    bool transformed = draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                       draw_dsc->scale_y != LV_SCALE_NONE ? true : false;

    ite_it2d_area_t clip_area;
    clip_area.x1 = t->clip_area.x1;
    clip_area.y1 = t->clip_area.y1;
    clip_area.x2 = t->clip_area.x2;
    clip_area.y2 = t->clip_area.y2;

    ite_it2d_surface_t * src_surface = lv_layer_get_ite_it2d_surface(layer_to_draw);

    if(draw_dsc->opa < LV_OPA_MAX) {
        ite_it2d_set_const_alpha(&ctx, draw_dsc->opa);
        ite_it2d_enable_const_alpha(&ctx, true);
    }

    ite_it2d_surface_t * dst_surface = lv_layer_get_ite_it2d_surface(t->target_layer);
    ite_it2d_set_dst_surface(&ctx, dst_surface);
    ite_it2d_set_clip_area(&ctx, &clip_area);
    ite_it2d_enable_clipping(&ctx, true);
    ite_it2d_set_src_surface(&ctx, src_surface);

    ite_it2d_rect_t rect;

    if(!transformed) {
        rect.x = coords->x1;
        rect.y = coords->y1;
        rect.w = lv_area_get_width(coords);
        rect.h = lv_area_get_height(coords);
        ite_it2d_set_dst_rect(&ctx, &rect);

        ite_it2d_copy(&ctx);
    }
    else {
        rect.w = (lv_area_get_width(coords) * draw_dsc->scale_x) / 256;
        rect.h = (lv_area_get_height(coords) * draw_dsc->scale_y) / 256;
        rect.x = -draw_dsc->pivot.x;
        rect.y = -draw_dsc->pivot.y;
        rect.x = (rect.x * draw_dsc->scale_x) / 256;
        rect.y = (rect.y * draw_dsc->scale_y) / 256;
        rect.x += coords->x1 + draw_dsc->pivot.x;
        rect.y += coords->y1 + draw_dsc->pivot.y;
        ite_it2d_set_dst_rect(&ctx, &rect);

        if(draw_dsc->antialias) {
            ite_it2d_enable_interpolation(&ctx, true);
        }

        ite_it2d_transform_dsc_t xform_dsc;
        ite_it2d_init_transform_dsc(&xform_dsc);
        xform_dsc.center.x = draw_dsc->pivot.x;
        xform_dsc.center.y = draw_dsc->pivot.y;
        xform_dsc.scale_x = (float)draw_dsc->scale_x / LV_SCALE_NONE;
        xform_dsc.scale_y = (float)draw_dsc->scale_y / LV_SCALE_NONE;
        xform_dsc.angle = draw_dsc->rotation / 10.0f;
        xform_dsc.flags.auto_dst_rect = 1;
        ite_it2d_transform_copy(&ctx, &xform_dsc);
    }

    if(wait_finish) {
        ite_it2d_wait_surface_finish(src_surface);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool apply_mask(const lv_draw_image_dsc_t * draw_dsc, ite_it2d_context_t * ctx, bool * wait_finish)
{
    lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;
    lv_image_decoder_dsc_t mask_decoder_dsc;
    lv_area_t mask_area;

    lv_result_t decoder_res = lv_image_decoder_open(&mask_decoder_dsc, draw_dsc->bitmap_mask_src, NULL);
    if(decoder_res != LV_RESULT_OK || mask_decoder_dsc.decoded == NULL) {
        LV_LOG_WARN("Could open the mask. The mask is not applied.");
        return true;
    }

    if(mask_decoder_dsc.decoded->header.cf != LV_COLOR_FORMAT_A8 &&
       mask_decoder_dsc.decoded->header.cf != LV_COLOR_FORMAT_L8) {
        LV_LOG_WARN("The mask image is not A8/L8 format. The mask is not applied.");
        return true;
    }

    const lv_draw_buf_t * mask_draw_buf = mask_decoder_dsc.decoded;

    /*Align the mask to the center*/
    lv_area_t image_area;
    image_area = draw_dsc->image_area;  /*Use the whole image area for the alignment*/
    lv_area_set(&mask_area, 0, 0, mask_draw_buf->header.w - 1, mask_draw_buf->header.h - 1);
    lv_area_align(&image_area, &mask_area, LV_ALIGN_CENTER, 0, 0);

    image_area =
        layer_to_draw->buf_area; /*The image can be smaller if only a part was rendered. Use this are during rendering*/

    /*Only the intersection of the mask and image needs to be rendered
     *If if there is no intersection there is nothing to render as the image is out of the mask.*/
    lv_area_t masked_area;
    if(!lv_area_intersect(&masked_area, &mask_area, &image_area)) return false;

    ((lv_draw_buf_t *)mask_draw_buf)->header.cf = LV_COLOR_FORMAT_A8;
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_img_surf_obtain(&mask_decoder_dsc);
    if(surf) {
        ite_it2d_set_mask_surface(ctx, surf);
    }
    else {
        static ite_it2d_surface_t mask_surf;
        lv_memzero(&mask_surf, sizeof(ite_it2d_surface_t));
        mask_surf.width = mask_draw_buf->header.w;
        mask_surf.height = mask_draw_buf->header.h;
        mask_surf.pitch = mask_draw_buf->header.stride;
        mask_surf.flags.format = lv_color_format_to_ite_it2d_pixel_format(mask_decoder_dsc.decoded->header.cf);
        mask_surf.buf = (uint8_t *)mask_draw_buf->data;
        mask_surf.id = ite_it2d_get_current_id();
        ite_it2d_set_mask_surface(ctx, &mask_surf);

        *wait_finish = true;
    }
    ite_it2d_set_mask_pos(ctx, masked_area.x1 - mask_area.x1, masked_area.y1 - mask_area.y1);
    ite_it2d_enable_masking(ctx, true);

    return true;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
