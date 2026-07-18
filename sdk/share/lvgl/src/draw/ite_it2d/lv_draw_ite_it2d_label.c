/**
 * @file lv_draw_ite_it2d_letter.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lv_draw_private.h"
#include "../lv_draw_label_private.h"
#include "lv_draw_ite_it2d.h"
#if LV_USE_DRAW_ITE_IT2D
#include LV_ITE_IT2D_INCLUDE_PATH
#include "../../misc/lv_area_private.h"
#include "../../font/lv_font_fmt_txt.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t magic;
    const lv_font_t * font_p;
    uint32_t letter;
} lv_font_glyph_key_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void draw_letter_cb(lv_draw_task_t * t, lv_draw_glyph_dsc_t * glyph_draw_dsc,
                           lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area);

static lv_font_glyph_key_t font_key_glyph_create(const lv_font_t * font_p, uint32_t letter);

static ite_it2d_surface_t * glyph_surf_obtain(lv_draw_glyph_dsc_t * glyph_draw_dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_ite_it2d_letter(lv_draw_task_t * t, const lv_draw_letter_dsc_t * dsc, const lv_area_t * coords)
{
    if(dsc->opa <= LV_OPA_MIN)
        return;

    lv_draw_glyph_dsc_t glyph_dsc;
    lv_draw_glyph_dsc_init(&glyph_dsc);
    glyph_dsc.opa = dsc->opa;
    glyph_dsc.bg_coords = NULL;
    glyph_dsc.color = dsc->color;
    glyph_dsc.rotation = dsc->rotation;
    glyph_dsc.pivot = dsc->pivot;

    LV_PROFILER_BEGIN;
    lv_draw_unit_draw_letter(t, &glyph_dsc, &(lv_point_t) {
        .x = coords->x1, .y = coords->y1
    },
    dsc->font, dsc->unicode, draw_letter_cb);
    LV_PROFILER_END;

    if(glyph_dsc._draw_buf) {
        lv_draw_buf_destroy(glyph_dsc._draw_buf);
        glyph_dsc._draw_buf = NULL;
    }
}

void lv_draw_ite_it2d_label(lv_draw_task_t * t, const lv_draw_label_dsc_t * dsc, const lv_area_t * coords)
{
    if(dsc->opa <= LV_OPA_MIN) return;

    lv_draw_label_iterate_characters(t, dsc, coords, draw_letter_cb);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void draw_letter_cb(lv_draw_task_t * t, lv_draw_glyph_dsc_t * glyph_draw_dsc,
                           lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area)
{
    if(glyph_draw_dsc) {
        switch(glyph_draw_dsc->format) {
            case LV_FONT_GLYPH_FORMAT_NONE: {
#if LV_USE_FONT_PLACEHOLDER
                    /* Draw a placeholder rectangle*/
                    lv_draw_border_dsc_t border_draw_dsc;
                    lv_draw_border_dsc_init(&border_draw_dsc);
                    border_draw_dsc.opa = glyph_draw_dsc->opa;
                    border_draw_dsc.color = glyph_draw_dsc->color;
                    border_draw_dsc.width = 1;
                    lv_draw_ite_it2d_border(t, &border_draw_dsc, glyph_draw_dsc->bg_coords);
#endif
                }
                break;
            case LV_FONT_GLYPH_FORMAT_A1:
            case LV_FONT_GLYPH_FORMAT_A2:
            case LV_FONT_GLYPH_FORMAT_A4:
            case LV_FONT_GLYPH_FORMAT_A8:
            case LV_FONT_GLYPH_FORMAT_IMAGE: {
                    if(glyph_draw_dsc->format != LV_FONT_GLYPH_FORMAT_IMAGE) {
                        lv_layer_t * layer = t->target_layer;
                        lv_area_t clip_area;
                        if(!lv_area_intersect(&clip_area, glyph_draw_dsc->letter_coords, &t->clip_area)) return;

                        lv_area_t draw_area = *glyph_draw_dsc->letter_coords;
                        lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
                        lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

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

                        ite_it2d_color_t letter_color;
                        lv_color_to_ite_it2d_color(&glyph_draw_dsc->color, &letter_color);
                        ite_it2d_set_fg_color(&ctx, letter_color);
                        ite_it2d_enable_fg_color(&ctx, true);

                        if(glyph_draw_dsc->opa < LV_OPA_MAX) {
                            ite_it2d_set_const_alpha(&ctx, glyph_draw_dsc->opa);
                            ite_it2d_enable_const_alpha(&ctx, true);
                        }
                        ite_it2d_enable_blending(&ctx, true);

                        ite_it2d_surface_t * surf = glyph_surf_obtain(glyph_draw_dsc);
                        if(surf) {
                            ite_it2d_set_src_surface(&ctx, surf);
                        }
                        else {
                            const lv_draw_buf_t * draw_buf = lv_font_get_glyph_bitmap(glyph_draw_dsc->g, glyph_draw_dsc->_draw_buf);

                            ite_it2d_surface_t srcsurf;
                            lv_memzero(&srcsurf, sizeof(ite_it2d_surface_t));
                            srcsurf.width = draw_buf->header.w;
                            srcsurf.height = draw_buf->header.h;
                            srcsurf.pitch = draw_buf->header.stride;
                            srcsurf.flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;
                            srcsurf.buf = draw_buf->data;
                            srcsurf.id = ite_it2d_get_current_id();
                            lv_sys_cache_data_flush_range(srcsurf.buf, srcsurf.pitch * srcsurf.height);

                            ite_it2d_set_src_surface(&ctx, &srcsurf);
                        }

                        if(glyph_draw_dsc->rotation % 3600 == 0) {
                            ite_it2d_copy(&ctx);
                        }
                        else {
                            ite_it2d_transform_dsc_t xform_dsc;
                            ite_it2d_init_transform_dsc(&xform_dsc);
                            xform_dsc.center.x = glyph_draw_dsc->pivot.x;
                            xform_dsc.center.y = glyph_draw_dsc->pivot.y;
                            xform_dsc.angle = glyph_draw_dsc->rotation / 10.0f;
                            ite_it2d_transform_copy(&ctx, &xform_dsc);
                        }

                        if(!surf) {
                            ite_it2d_wait_surface_finish(ctx.op.bitblt.src_surf);
                        }
                    }
                    else {
                        glyph_draw_dsc->glyph_data = lv_font_get_glyph_bitmap(glyph_draw_dsc->g, glyph_draw_dsc->_draw_buf);
                        lv_draw_image_dsc_t img_dsc;
                        lv_draw_image_dsc_init(&img_dsc);
                        img_dsc.rotation = glyph_draw_dsc->rotation;
                        img_dsc.scale_x = LV_SCALE_NONE;
                        img_dsc.scale_y = LV_SCALE_NONE;
                        img_dsc.opa = glyph_draw_dsc->opa;
                        img_dsc.src = glyph_draw_dsc->glyph_data;
                        img_dsc.recolor = glyph_draw_dsc->color;
                        img_dsc.pivot = (lv_point_t) {
                            .x = glyph_draw_dsc->pivot.x,
                            .y = glyph_draw_dsc->g->box_h + glyph_draw_dsc->g->ofs_y
                        };
                        lv_draw_ite_it2d_image(t, &img_dsc, glyph_draw_dsc->letter_coords);
                    }
                    break;
                }
            default:
                break;
        }
    }

    if(fill_draw_dsc && fill_area) {
        lv_draw_ite_it2d_fill(t, fill_draw_dsc, fill_area);
    }
}

static lv_font_glyph_key_t font_key_glyph_create(const lv_font_t * font_p, uint32_t letter)
{
    lv_font_glyph_key_t key;
    /* VERY IMPORTANT! Padding between members is uninitialized, so we have to wipe them manually */
    lv_memzero(&key, sizeof(key));
    key.magic = LV_ITE_IT2D_CACHE_KEY_MAGIC_FONT_GLYPH;
    key.font_p = font_p;
    key.letter = letter;
    return key;
}

static ite_it2d_surface_t * glyph_surf_obtain(lv_draw_glyph_dsc_t * glyph_draw_dsc)
{
    lv_font_glyph_dsc_t * g = glyph_draw_dsc->g;
    const lv_font_t * font_p = g->resolved_font;
    LV_ASSERT(font_p);
    uint32_t letter = g->gid.index;
    lv_font_glyph_key_t glyph_key = font_key_glyph_create(font_p, letter);
    ite_it2d_surface_t * surf = lv_draw_ite_it2d_surf_cache_get(&glyph_key, sizeof(glyph_key));
    if(!surf) {
        uint8_t * bmp = NULL;
        lv_font_glyph_format_t format = g->format;
        if(lv_font_has_static_bitmap(font_p)) {
            glyph_draw_dsc->glyph_data = lv_font_get_glyph_static_bitmap(g);
            bmp = (uint8_t *)glyph_draw_dsc->glyph_data;
        }
        else {
            glyph_draw_dsc->glyph_data = lv_font_get_glyph_bitmap(g, glyph_draw_dsc->_draw_buf);
            const lv_draw_buf_t * draw_buf = glyph_draw_dsc->glyph_data;
            bmp = draw_buf->data;
            format = LV_FONT_GLYPH_FORMAT_A8;
        }

        surf = lv_zalloc(sizeof(ite_it2d_surface_t));
        LV_ASSERT_MALLOC(surf);
        surf->width = g->box_w;
        surf->height = g->box_h;

        int bpp = 0;
        switch(format) {
            case LV_FONT_GLYPH_FORMAT_A1:
                surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_1;
                bpp = 1;
                break;
            case LV_FONT_GLYPH_FORMAT_A2:
                surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_2;
                bpp = 2;
                break;
            case LV_FONT_GLYPH_FORMAT_A4:
                surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_4;
                bpp = 4;
                break;
            case LV_FONT_GLYPH_FORMAT_A8:
                surf->flags.format = ITE_IT2D_PIXEL_FORMAT_A_8;
                bpp = 8;
                break;
            default:
                LV_ASSERT(0);
                lv_free(surf);
                return NULL;
        }

        if(font_p->subpx) {
            surf->width /= 3;
            surf->pitch = surf->width * 4;
            surf->flags.format = ITE_IT2D_PIXEL_FORMAT_XARGB;
            if(!lv_draw_ite_it2d_create_surface(surf)) {
                lv_free(surf);
                return NULL;
            }
            lv_ite_it2d_to_xargb(surf->buf, bmp, g->box_w, g->box_h, bpp);
        }
        else {
            if(lv_font_has_static_bitmap(font_p) && (g->box_w % (8 / bpp) == 0)) {
                surf->flags.static_buf = true;
                surf->buf = bmp;

                if(!lv_draw_ite_it2d_create_surface(surf)) {
                    lv_free(surf);
                    return NULL;
                }
            }
            else {
                if(!lv_draw_ite_it2d_create_surface(surf)) {
                    lv_free(surf);
                    return NULL;
                }
                lv_ite_it2d_to_a8421(surf->buf, bmp, g->box_w, g->box_h, surf->pitch, bpp);
            }
        }
        lv_sys_cache_data_flush_range(surf->buf, surf->pitch * surf->height);
        lv_draw_ite_it2d_surf_cache_put(&glyph_key, sizeof(glyph_key), surf);
    }
    return surf;
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
