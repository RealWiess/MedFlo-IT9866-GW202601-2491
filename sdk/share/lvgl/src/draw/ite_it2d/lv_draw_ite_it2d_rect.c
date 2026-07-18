/**
 * @file lv_draw_ite_it2d_rect.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_ite_it2d.h"
#include "lv_draw_ite_it2d_rect.h"
#include "lv_draw_ite_it2d_mask.h"
#if LV_USE_DRAW_ITE_IT2D

#include "../../misc/lv_area_private.h"
#include "../sw/lv_draw_sw_mask_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t magic;
    int16_t radius;
} lv_draw_fill_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_draw_fill_key_t bg_key_create(int32_t radius);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

ite_it2d_surface_t * lv_draw_ite_it2d_bg_frag_obtain(int32_t radius)
{
    lv_draw_fill_key_t key = bg_key_create(radius);
    lv_area_t coords = {0, 0, radius * 2 - 1, radius * 2 - 1};
    lv_area_t coords_frag = {0, 0, radius - 1, radius - 1};
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&key, sizeof(key));
    if(surf == NULL) {
        lv_draw_sw_mask_radius_param_t param;
        lv_draw_sw_mask_radius_init(&param, &coords, radius, false);
        void * masks[2] = {0};
        masks[0] = &param;
        surf = lv_draw_ite_it2d_mask_dump_surf(&coords_frag, masks);
        if(surf) {
            lv_draw_ite_it2d_surf_cache_put(&key, sizeof(key), surf);
        }
        lv_draw_sw_mask_free_param(&param);
    }
    return surf;
}

void lv_ite_it2d_draw_corners(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                              const lv_area_t * coords, const lv_area_t * clip, bool full)
{
    if(!clip) clip = coords;
    lv_area_t corner_area, dst_area;
    ite_it2d_transform_dsc_t xform_dsc;
    ite_it2d_init_transform_dsc(&xform_dsc);
    ite_it2d_set_src_surface(ctx, frag);
    /* Upper left */
    corner_area.x1 = coords->x1;
    corner_area.y1 = coords->y1;
    corner_area.x2 = coords->x1 + frag_size - 1;
    corner_area.y2 = coords->y1 + frag_size - 1;
    if(lv_area_intersect(&dst_area, &corner_area, clip)) {
        int32_t dw = lv_area_get_width(&dst_area), dh = lv_area_get_height(&dst_area);
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, dw, dh};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        int32_t sx = dst_area.x1 - corner_area.x1, sy = dst_area.y1 - corner_area.y1;
        ite_it2d_rect_t src_rect = {sx, sy, dw, dh};
        ite_it2d_set_src_rect(ctx, &src_rect);
        ite_it2d_transform_copy(ctx, &xform_dsc);
        ite_it2d_enable_clipping(ctx, false);
    }
    /* Upper right, clip right edge if too big */
    corner_area.x1 = LV_MAX(coords->x2 - frag_size + 1, coords->x1 + frag_size);
    corner_area.x2 = coords->x2;
    if(lv_area_intersect(&dst_area, &corner_area, clip)) {
        int32_t dw = lv_area_get_width(&dst_area), dh = lv_area_get_height(&dst_area);
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, dw, dh};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        if(full) {
            int32_t sx = (int32_t)(dst_area.x1 - corner_area.x1),
                    sy = (int32_t)(dst_area.y1 - corner_area.y1);
            ite_it2d_rect_t src_rect = {frag_size + 3 + sx, sy, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            ite_it2d_copy(ctx);
        }
        else {
            ite_it2d_rect_t src_rect = {corner_area.x2 - dst_area.x2, dst_area.y1 - corner_area.y1, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            xform_dsc.pos.x = src_rect.w;
            xform_dsc.scale_x = -1.0f;
            ite_it2d_transform_copy(ctx, &xform_dsc);
            ite_it2d_enable_clipping(ctx, false);
        }
    }
    /* Lower right, clip bottom edge if too big */
    corner_area.y1 = LV_MAX(coords->y2 - frag_size + 1, coords->y1 + frag_size);
    corner_area.y2 = coords->y2;
    if(lv_area_intersect(&dst_area, &corner_area, clip)) {
        int32_t dw = lv_area_get_width(&dst_area), dh = lv_area_get_height(&dst_area);
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, dw, dh};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        if(full) {
            int32_t sx = (int32_t)(dst_area.x1 - corner_area.x1),
                    sy = (int32_t)(dst_area.y1 - corner_area.y1);
            ite_it2d_rect_t src_rect = {frag_size + 3 + sx, frag_size + 3 + sy, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            ite_it2d_copy(ctx);
        }
        else {
            ite_it2d_rect_t src_rect = {corner_area.x2 - dst_area.x2, corner_area.y2 - dst_area.y2, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            xform_dsc.pos.x = src_rect.w;
            xform_dsc.pos.y = src_rect.h;
            xform_dsc.scale_x = -1.0f;
            xform_dsc.scale_y = -1.0f;
            ite_it2d_transform_copy(ctx, &xform_dsc);
            ite_it2d_enable_clipping(ctx, false);
        }
    }
    /* Lower left, right edge should not be clip */
    corner_area.x1 = coords->x1;
    corner_area.x2 = coords->x1 + frag_size - 1;
    if(lv_area_intersect(&dst_area, &corner_area, clip)) {
        int32_t dw = lv_area_get_width(&dst_area), dh = lv_area_get_height(&dst_area);
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, dw, dh};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        if(full) {
            int32_t sx = (int32_t)(dst_area.x1 - corner_area.x1),
                    sy = (int32_t)(dst_area.y1 - corner_area.y1);
            ite_it2d_rect_t src_rect = {sx, frag_size + 3 + sy, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            ite_it2d_copy(ctx);
        }
        else {
            ite_it2d_rect_t src_rect = {dst_area.x1 - corner_area.x1, corner_area.y2 - dst_area.y2, dw, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            xform_dsc.pos.x = 0;
            xform_dsc.pos.y = src_rect.h;
            xform_dsc.scale_x = 1.0f;
            xform_dsc.scale_y = -1.0f;
            ite_it2d_transform_copy(ctx, &xform_dsc);
            ite_it2d_enable_clipping(ctx, false);
        }
    }
}

void lv_ite_it2d_draw_borders(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                              const lv_area_t * coords, const lv_area_t * clipped, bool full)
{
    lv_area_t border_area, dst_area;
    ite_it2d_transform_dsc_t xform_dsc;
    ite_it2d_init_transform_dsc(&xform_dsc);
    xform_dsc.flags.tile_mode = ITE_IT2D_TILE_PAD;
    ite_it2d_set_src_surface(ctx, frag);
    /* Top border */
    border_area.x1 = coords->x1 + frag_size;
    border_area.y1 = coords->y1;
    border_area.x2 = coords->x2 - frag_size;
    border_area.y2 = coords->y1 + frag_size - 1;
    if(lv_area_intersect(&dst_area, &border_area, clipped)) {
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, lv_area_get_width(&dst_area), lv_area_get_height(&dst_area)};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        int32_t sy = (int32_t)(dst_area.y1 - border_area.y1);
        if(full) {
            ite_it2d_set_src_pos(ctx, frag_size + 1, sy);
        }
        else {
            ite_it2d_set_src_pos(ctx, frag_size - 1, sy);
        }
        ite_it2d_set_src_size(ctx, 1, lv_area_get_height(&dst_area));
        ite_it2d_transform_copy(ctx, &xform_dsc);
        ite_it2d_enable_clipping(ctx, false);
    }
    /* Bottom border */
    border_area.y1 = LV_MAX(coords->y2 - frag_size + 1, coords->y1 + frag_size);
    border_area.y2 = coords->y2;
    if(lv_area_intersect(&dst_area, &border_area, clipped)) {
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, lv_area_get_width(&dst_area), lv_area_get_height(&dst_area)};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        int32_t dh = lv_area_get_height(&dst_area);
        if(full) {
            int32_t sy = (int32_t)(dst_area.y1 - border_area.y1);
            ite_it2d_rect_t src_rect = {frag_size + 1, frag_size + 3 + sy, 1, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
        }
        else {
            int32_t sy = (int32_t)(border_area.y2 - dst_area.y2);
            ite_it2d_rect_t src_rect = {frag_size - 1, sy, 1, dh};
            ite_it2d_set_src_rect(ctx, &src_rect);
            xform_dsc.pos.y = src_rect.h;
            xform_dsc.scale_y = -1.0f;
        }
        ite_it2d_transform_copy(ctx, &xform_dsc);
        ite_it2d_enable_clipping(ctx, false);
    }
    /* Left border */
    border_area.x1 = coords->x1;
    border_area.y1 = coords->y1 + frag_size;
    border_area.x2 = coords->x1 + frag_size - 1;
    border_area.y2 = coords->y2 - frag_size;
    if(lv_area_intersect(&dst_area, &border_area, clipped)) {
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, lv_area_get_width(&dst_area), lv_area_get_height(&dst_area)};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        int32_t dw = lv_area_get_width(&dst_area);
        int32_t sx = (int32_t)(dst_area.x1 - border_area.x1);
        if(full) {
            ite_it2d_set_src_pos(ctx, sx, frag_size + 1);
        }
        else {
            ite_it2d_set_src_pos(ctx, sx, frag_size - 1);
        }
        ite_it2d_set_src_size(ctx, dw, 1);
        xform_dsc.pos.y = 0;
        xform_dsc.scale_y = 1.0f;
        ite_it2d_transform_copy(ctx, &xform_dsc);
        ite_it2d_enable_clipping(ctx, false);
    }
    /* Right border */
    border_area.x1 = LV_MAX(coords->x2 - frag_size + 1, coords->x1 + frag_size);
    border_area.x2 = coords->x2;
    if(lv_area_intersect(&dst_area, &border_area, clipped)) {
        ite_it2d_rect_t dst_rect = {dst_area.x1, dst_area.y1, lv_area_get_width(&dst_area), lv_area_get_height(&dst_area)};
        ite_it2d_set_dst_rect(ctx, &dst_rect);
        int32_t dw = lv_area_get_width(&dst_area);
        if(full) {
            int32_t sx = (int32_t)(dst_area.x1 - border_area.x1);
            ite_it2d_rect_t src_rect = {frag_size + 3 + sx, frag_size + 1, dw, 1};
            ite_it2d_set_src_rect(ctx, &src_rect);
        }
        else {
            int32_t sx = (int32_t)(border_area.x2 - dst_area.x2);
            ite_it2d_rect_t src_rect = {sx, frag_size - 1, dw, 1};
            ite_it2d_set_src_rect(ctx, &src_rect);
            xform_dsc.pos.x = src_rect.w;
            xform_dsc.scale_x = -1.0f;
        }
        xform_dsc.pos.y = 0;
        xform_dsc.scale_y = 1.0f;
        ite_it2d_transform_copy(ctx, &xform_dsc);
        ite_it2d_enable_clipping(ctx, false);
    }
}

void lv_ite_it2d_draw_center(ite_it2d_context_t * ctx, ite_it2d_surface_t * frag, int32_t frag_size,
                             const lv_area_t * coords,
                             const lv_area_t * clipped, bool full)
{
    lv_area_t center_area = {
        coords->x1 + frag_size,
        coords->y1 + frag_size,
        coords->x2 - frag_size,
        coords->y2 - frag_size,
    };
    if(center_area.x2 < center_area.x1 || center_area.y2 < center_area.y1) return;
    lv_area_t draw_area;
    if(!lv_area_intersect(&draw_area, &center_area, clipped)) {
        return;
    }
    ite_it2d_transform_dsc_t xform_dsc;
    ite_it2d_init_transform_dsc(&xform_dsc);
    xform_dsc.flags.tile_mode = ITE_IT2D_TILE_PAD;
    ite_it2d_set_src_surface(ctx, frag);
    ite_it2d_rect_t dst_rect = {draw_area.x1, draw_area.y1, lv_area_get_width(&draw_area), lv_area_get_height(&draw_area)};
    ite_it2d_set_dst_rect(ctx, &dst_rect);
    if(full) {
        ite_it2d_set_src_pos(ctx, frag_size, frag_size);
    }
    else {
        ite_it2d_set_src_pos(ctx, frag_size - 1, frag_size - 1);
    }
    ite_it2d_set_src_size(ctx, 1, 1);
    ite_it2d_transform_copy(ctx, &xform_dsc);
    ite_it2d_enable_clipping(ctx, false);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_draw_fill_key_t bg_key_create(int32_t radius)
{
    lv_draw_fill_key_t key;
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_BG;
    key.radius = radius;
    return key;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
