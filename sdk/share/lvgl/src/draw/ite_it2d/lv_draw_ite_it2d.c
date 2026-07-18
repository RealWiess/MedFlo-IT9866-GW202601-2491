/**
 * @file lv_draw_ite_it2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lv_draw_private.h"
#if LV_USE_DRAW_ITE_IT2D
#include LV_ITE_IT2D_INCLUDE_PATH
#include "lv_draw_ite_it2d.h"
#include "../../core/lv_refr_private.h"
#include "../../display/lv_display_private.h"
#include "lvgl_display.h"
#include "../../misc/cache/lv_cache_entry_private.h"
#include "../../misc/lv_area_private.h"
#include "../lv_draw_buf_private.h"
#include "../lv_draw_mask_private.h"
#include "../lv_draw_vector_private.h"
#include "../sw/lv_draw_sw.h"

/*********************
 *      DEFINES
 *********************/
#define DRAW_UNIT_ID_ITE_IT2D     101

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t base_unit;
    lv_draw_task_t * task_act;
    lv_cache_t * surface_cache;
    lv_draw_buf_t render_draw_buf;
    lv_area_t area_original;
    bool last_draw_by_sw;
} lv_draw_ite_it2d_unit_t;

typedef struct {
    lv_cache_slot_size_t slot;
    lv_draw_dsc_base_t * draw_dsc;
    int32_t w;
    int32_t h;
    ite_it2d_surface_t surface;
} cache_data_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void execute_drawing(lv_draw_ite_it2d_unit_t * u);
static int32_t dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static int32_t evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static bool draw_to_surface(lv_draw_ite_it2d_unit_t * u, cache_data_t * cache_data);
static void layer_copy_image_bg(lv_layer_t * layer, lv_layer_t * dest_layer, const lv_draw_image_dsc_t * draw_dsc,
                                const lv_area_t * coords);
static void * buf_malloc(size_t size, lv_color_format_t color_format);
static void buf_free(void * buf);
static void invalidate_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void flush_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area);

/**********************
 *  GLOBAL PROTOTYPES
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
static bool ite_it2d_surface_cache_create_cb(cache_data_t * cached_data, void * user_data)
{
    return draw_to_surface((lv_draw_ite_it2d_unit_t *)user_data, cached_data);
}

static void ite_it2d_surface_cache_free_cb(cache_data_t * cached_data, void * user_data)
{
    LV_UNUSED(user_data);

    lv_free(cached_data->draw_dsc);
    ite_it2d_wait_surface_finish(&cached_data->surface);
    if(!cached_data->surface.flags.static_buf) {
        lv_free(cached_data->surface.buf);
    }
    cached_data->draw_dsc = NULL;
}

static lv_cache_compare_res_t ite_it2d_surface_cache_compare_cb(const cache_data_t * lhs, const cache_data_t * rhs)
{
    if(lhs == rhs) return 0;

    if(lhs->w != rhs->w) {
        return lhs->w > rhs->w ? 1 : -1;
    }
    if(lhs->h != rhs->h) {
        return lhs->h > rhs->h ? 1 : -1;
    }

    uint32_t lhs_dsc_size = lhs->draw_dsc->dsc_size;
    uint32_t rhs_dsc_size = rhs->draw_dsc->dsc_size;

    if(lhs_dsc_size != rhs_dsc_size) {
        return lhs_dsc_size > rhs_dsc_size ? 1 : -1;
    }

    const uint8_t * left_draw_dsc = (const uint8_t *)lhs->draw_dsc;
    const uint8_t * right_draw_dsc = (const uint8_t *)rhs->draw_dsc;
    left_draw_dsc += sizeof(lv_draw_dsc_base_t);
    right_draw_dsc += sizeof(lv_draw_dsc_base_t);

    int cmp_res = lv_memcmp(left_draw_dsc, right_draw_dsc, lhs->draw_dsc->dsc_size - sizeof(lv_draw_dsc_base_t));

    if(cmp_res != 0) {
        return cmp_res > 0 ? 1 : -1;
    }

    return 0;
}

void lv_draw_ite_it2d_init(void)
{
    lv_draw_ite_it2d_unit_t * draw_ite_it2d_unit = lv_draw_create_unit(sizeof(lv_draw_ite_it2d_unit_t));
    draw_ite_it2d_unit->base_unit.dispatch_cb = dispatch;
    draw_ite_it2d_unit->base_unit.evaluate_cb = evaluate;
    draw_ite_it2d_unit->base_unit.name = "ITE_IT2D";
    draw_ite_it2d_unit->surface_cache = lv_cache_create(&lv_cache_class_lru_ll_size,
    sizeof(cache_data_t), LV_ITE_IT2D_CACHE_SIZE, (lv_cache_ops_t) {
        .compare_cb = (lv_cache_compare_cb_t)ite_it2d_surface_cache_compare_cb,
        .create_cb = (lv_cache_create_cb_t)ite_it2d_surface_cache_create_cb,
        .free_cb = (lv_cache_free_cb_t)ite_it2d_surface_cache_free_cb,
    });
    lv_cache_set_name(draw_ite_it2d_unit->surface_cache, "ITE_IT2D_SURFACE");

    lv_draw_buf_init(&draw_ite_it2d_unit->render_draw_buf, 0, 0, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO, NULL, 0);

    lv_draw_ite_it2d_surf_cache_init();

    lv_draw_buf_handlers_t * handlers = lv_draw_buf_get_handlers();
    handlers->buf_malloc_cb = buf_malloc;
    handlers->buf_free_cb = buf_free;
    handlers->invalidate_cache_cb = invalidate_cache;
    handlers->flush_cache_cb = flush_cache;
}

void lv_draw_ite_it2d_buf_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                               const lv_draw_buf_t * src, const lv_area_t * src_area)
{
    if(LV_DRAW_ITE_IT2D_SW) {
        lv_draw_buf_copy(dest, dest_area, src, src_area);
        return;
    }

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_surface_t dest_surf;
    lv_memzero(&dest_surf, sizeof(ite_it2d_surface_t));
    dest_surf.width = dest->header.w;
    dest_surf.height = dest->header.h;
    dest_surf.flags.format = lv_color_format_to_ite_it2d_pixel_format(dest->header.cf);
    dest_surf.flags.static_buf = 1;
    dest_surf.buf = dest->data;
    ite_it2d_create_surface(&dest_surf);
    ite_it2d_set_dst_surface(&ctx, &dest_surf);

    lv_coord_t dw = lv_area_get_width(dest_area), dh = lv_area_get_height(dest_area);
    ite_it2d_rect_t dst_rect = {dest_area->x1, dest_area->y1, dw, dh};
    ite_it2d_set_dst_rect(&ctx, &dst_rect);

    ite_it2d_surface_t src_surf;
    lv_memzero(&src_surf, sizeof(ite_it2d_surface_t));
    src_surf.width = src->header.w;
    src_surf.height = src->header.h;
    src_surf.flags.format = lv_color_format_to_ite_it2d_pixel_format(src->header.cf);
    src_surf.flags.static_buf = 1;
    src_surf.buf = src->data;
    ite_it2d_create_surface(&src_surf);
    ite_it2d_set_src_surface(&ctx, &src_surf);

    lv_coord_t sw = lv_area_get_width(src_area), sh = lv_area_get_height(src_area);
    ite_it2d_rect_t src_rect = {src_area->x1, src_area->y1, sw, sh};
    ite_it2d_set_src_rect(&ctx, &src_rect);

    ite_it2d_copy(&ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_ite_it2d_unit_t * draw_ite_it2d_unit = (lv_draw_ite_it2d_unit_t *) draw_unit;

    /*Return immediately if it's busy with a draw task*/
    if(draw_ite_it2d_unit->task_act) return 0;

    /*This layer is ready, enable blending its buffer*/
    if(layer->parent && layer->all_tasks_added) {
        /*Find a draw task with TYPE_LAYER in the layer where the src is this layer*/
        lv_draw_task_t * t_src = layer->parent->draw_task_head;
        while(t_src) {
            if(t_src->type == LV_DRAW_TASK_TYPE_LAYER && t_src->state == LV_DRAW_TASK_STATE_WAITING) {
                lv_draw_image_dsc_t * draw_dsc = t_src->draw_dsc;
                if(draw_dsc->src == layer) {
                    /* Copy background image to layer */
                    layer_copy_image_bg(layer->parent, layer, draw_dsc, &t_src->area);

                    /* Draw queued layer tasks */
                    lv_draw_task_t * task = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_ITE_IT2D);
                    while(task) {
                        task->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
                        draw_ite_it2d_unit->task_act = task;

                        execute_drawing(draw_ite_it2d_unit);

                        draw_ite_it2d_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
                        draw_ite_it2d_unit->task_act = NULL;

                        /*The draw unit is free now. Request a new dispatching as it can get a new task*/
                        lv_draw_dispatch_request();

                        task = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_ITE_IT2D);
                    }

                    t_src->state = LV_DRAW_TASK_STATE_QUEUED;
                    lv_draw_dispatch_request();
                    return -1;
                }
            }
            t_src = t_src->next;
        }
    }

    lv_draw_task_t * t = NULL;
    t = lv_draw_get_available_task(layer, NULL, DRAW_UNIT_ID_ITE_IT2D);
    if(t == NULL) return -1;

    lv_display_t * disp = lv_refr_get_disp_refreshing();
    if(layer != disp->layer_head) {
        ite_it2d_surface_t * surface = layer->user_data;
        if(surface == NULL) {
            if(layer->draw_buf) {
                lv_sys_cache_data_flush_range(layer->draw_buf->data, layer->draw_buf->data_size);
            }
            void * buf = lv_draw_layer_alloc_buf(layer);
            if(buf == NULL) return -1;

            surface = lv_zalloc(sizeof(ite_it2d_surface_t));
            LV_ASSERT_MALLOC(surface);
            surface->width = lv_area_get_width(&layer->buf_area);
            surface->height = lv_area_get_height(&layer->buf_area);
            surface->flags.format = lv_color_format_to_ite_it2d_pixel_format(layer->color_format);
            surface->flags.static_buf = 1;
            surface->buf = buf;
            ite_it2d_create_surface(surface);

            layer->user_data = surface;
            lv_sys_cache_data_invd_range(buf, surface->pitch * surface->height);
        }

        if(layer->parent) {
            /* Queue layer tasks */
            lv_draw_dispatch_request();
            return -1;
        }
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_ite_it2d_unit->task_act = t;

    execute_drawing(draw_ite_it2d_unit);

    draw_ite_it2d_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
    draw_ite_it2d_unit->task_act = NULL;

    /*The draw unit is free now. Request a new dispatching as it can get a new task*/
    lv_draw_dispatch_request();
    return 1;
}

static int32_t evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);
    lv_draw_ite_it2d_unit_t * draw_ite_it2d_unit = (lv_draw_ite_it2d_unit_t *) draw_unit;
    lv_draw_dsc_base_t * draw_dsc = (lv_draw_dsc_base_t *)task->draw_dsc;

    if(!lv_ite_it2d_is_hw_cf_supported(draw_dsc->layer->color_format)) {
        return 0;
    }

    /*If not refreshing the display probably it's a canvas rendering
     *which his not support in ITE IT2D as it's not a surface.*/
    if(lv_refr_get_disp_refreshing() == NULL) return 0;

    if(task->type == LV_DRAW_TASK_TYPE_IMAGE && ((lv_draw_image_dsc_t *)draw_dsc)->blend_mode != LV_BLEND_MODE_NORMAL) {
        if(!draw_ite_it2d_unit->last_draw_by_sw) {
            ite_it2d_wait_surface_finish(lv_layer_get_ite_it2d_surface(draw_dsc->layer));
            draw_ite_it2d_unit->last_draw_by_sw = true;
        }
        return 0;
    }

    if(draw_dsc->user_data == NULL) {
        switch(task->type) {
            case LV_DRAW_TASK_TYPE_FILL:
            case LV_DRAW_TASK_TYPE_BORDER:
            case LV_DRAW_TASK_TYPE_BOX_SHADOW:
            case LV_DRAW_TASK_TYPE_LETTER:
            case LV_DRAW_TASK_TYPE_LABEL:
            case LV_DRAW_TASK_TYPE_IMAGE:
            case LV_DRAW_TASK_TYPE_LAYER:
            case LV_DRAW_TASK_TYPE_LINE:
            case LV_DRAW_TASK_TYPE_ARC:
            case LV_DRAW_TASK_TYPE_TRIANGLE:
            case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:
#if LV_USE_VECTOR_GRAPHIC
            case LV_DRAW_TASK_TYPE_VECTOR:
#endif
                break;

            default:
                /*The draw unit is not able to draw this task. */
                return 0;
        }

        /* The draw unit is able to draw this task. */
#if !LV_DRAW_ITE_IT2D_SW
        task->preference_score = 80;
        task->preferred_draw_unit_id = DRAW_UNIT_ID_ITE_IT2D;

        if(draw_ite_it2d_unit->last_draw_by_sw) {
            lv_sys_cache_data_flush_range(draw_dsc->layer->draw_buf->data, draw_dsc->layer->draw_buf->data_size);
            draw_ite_it2d_unit->last_draw_by_sw = false;
        }
#endif
    }
    return 0;
}

static bool draw_to_surface(lv_draw_ite_it2d_unit_t * u, cache_data_t * cache_data)
{
    lv_draw_task_t * task = u->task_act;

    lv_layer_t dest_layer;
    lv_layer_init(&dest_layer);

    int32_t surface_w = lv_area_get_width(&task->_real_area);
    int32_t surface_h = lv_area_get_height(&task->_real_area);

    uint32_t data_size = LV_DRAW_BUF_SIZE(surface_w, surface_h, LV_COLOR_FORMAT_ARGB8888);
    uint8_t * data = lv_zalloc(data_size);
    LV_ASSERT_MALLOC(data);

    lv_result_t init_result = lv_draw_buf_init(&u->render_draw_buf, surface_w, surface_h, LV_COLOR_FORMAT_ARGB8888,
                                               LV_STRIDE_AUTO, data, data_size);
    LV_ASSERT(init_result == LV_RESULT_OK);

    ite_it2d_surface_t * surf = &cache_data->surface;
    lv_memzero(surf, sizeof(ite_it2d_surface_t));
    surf->width = surface_w;
    surf->height = surface_h;
    surf->pitch = u->render_draw_buf.header.stride;
    surf->flags.format = ITE_IT2D_PIXEL_FORMAT_ARGB_8888;
    surf->buf = data;
    surf->id = ite_it2d_get_current_id();

    dest_layer.draw_buf = &u->render_draw_buf;
    dest_layer.color_format = LV_COLOR_FORMAT_ARGB8888;
    dest_layer.buf_area = task->_real_area;
    dest_layer._clip_area = task->_real_area;
    dest_layer.phy_clip_area = task->_real_area;
    dest_layer.draw_buf->data = data;
    dest_layer.user_data = surf;

    lv_display_t * disp = lv_refr_get_disp_refreshing();

    lv_obj_t * obj = ((lv_draw_dsc_base_t *)task->draw_dsc)->obj;
    bool original_send_draw_task_event = false;
    if(obj) {
        original_send_draw_task_event = lv_obj_has_flag(obj, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    }

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_FILL: {
                lv_draw_fill_dsc_t * fill_dsc = task->draw_dsc;
                lv_draw_rect_dsc_t rect_dsc;
                lv_draw_rect_dsc_init(&rect_dsc);
                rect_dsc.base.user_data = (void *)(uintptr_t)1;
                rect_dsc.bg_color = fill_dsc->color;
                rect_dsc.bg_grad = fill_dsc->grad;
                rect_dsc.radius = fill_dsc->radius;
                rect_dsc.bg_opa = fill_dsc->opa;
                lv_draw_rect(&dest_layer, &rect_dsc, &task->area);
            }
            break;
#if LV_DRAW_ITE_IT2D_CACHE_TEXT_STATIC
        case LV_DRAW_TASK_TYPE_LABEL: {
                lv_draw_label_dsc_t label_dsc;
                lv_memcpy(&label_dsc, task->draw_dsc, sizeof(label_dsc));
                label_dsc.base.user_data = (void *)(uintptr_t)1;
                lv_draw_label(&dest_layer, &label_dsc, &task->area);
            }
            break;
#endif
        case LV_DRAW_TASK_TYPE_TRIANGLE: {
                lv_draw_triangle_dsc_t triangle_dsc;
                lv_memcpy(&triangle_dsc, task->draw_dsc, sizeof(triangle_dsc));
                triangle_dsc.base.user_data = (void *)(uintptr_t)1;
                lv_draw_triangle(&dest_layer, &triangle_dsc);
            }
            break;
        case LV_DRAW_TASK_TYPE_IMAGE: {
                lv_sys_cache_data_flush_range(data, data_size);
                lv_draw_image_dsc_t image_dsc;
                lv_memcpy(&image_dsc, task->draw_dsc, sizeof(image_dsc));
                image_dsc.base.user_data = (void *)(uintptr_t)1;
                layer_copy_image_bg(task->target_layer, &dest_layer, &image_dsc, &u->area_original);
                ite_it2d_wait_surface_finish(surf);
                lv_sys_cache_data_invd_range(data, data_size);
                lv_area_move(&image_dsc.image_area, -u->area_original.x1, -u->area_original.y1);
                lv_draw_image(&dest_layer, &image_dsc, &task->area);
                break;
            }
#if LV_USE_VECTOR_GRAPHIC
        case LV_DRAW_TASK_TYPE_VECTOR: {
                lv_draw_vector_task_dsc_t vector_task_dsc;
                lv_memcpy(&vector_task_dsc, task->draw_dsc, sizeof(vector_task_dsc));
                vector_task_dsc.base.user_data = (void *)(uintptr_t)1;
                vector_task_dsc.base.layer = &dest_layer;
                lv_draw_sw_vector(task, &vector_task_dsc);
                break;
            }
#endif
        default: {
                lv_free(data);
                return false;
            }
    }

    while(dest_layer.draw_task_head) {
        lv_draw_dispatch_layer(disp, &dest_layer);
        if(dest_layer.draw_task_head) {
            lv_draw_dispatch_wait_for_request();
        }
    }
    lv_sys_cache_data_flush_range(data, data_size);

    lv_draw_dsc_base_t * base_dsc = task->draw_dsc;

    cache_data->slot.size = data_size + base_dsc->dsc_size;
    cache_data->draw_dsc = lv_malloc(base_dsc->dsc_size);
    LV_ASSERT_MALLOC(cache_data->draw_dsc);
    lv_memcpy((void *)cache_data->draw_dsc, base_dsc, base_dsc->dsc_size);
    cache_data->w = surface_w;
    cache_data->h = surface_h;

    if(obj) {
        lv_obj_set_flag(obj, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS, original_send_draw_task_event);
    }

    return true;
}

static void draw_from_cached_surface(lv_draw_ite_it2d_unit_t * u)
{
    lv_draw_task_t * t = u->task_act;

    cache_data_t data_to_find;
    data_to_find.draw_dsc = (lv_draw_dsc_base_t *)t->draw_dsc;

    lv_area_t a = t->area;
    if(t->type == LV_DRAW_TASK_TYPE_IMAGE) {
        lv_draw_image_dsc_t * image_dsc = t->draw_dsc;
        if(image_dsc->rotation != 0 || image_dsc->scale_x != LV_SCALE_NONE ||
           image_dsc->scale_y != LV_SCALE_NONE) {
            a = t->_real_area = t->clip_area;
        }
        u->area_original = a;
    }

    data_to_find.w = lv_area_get_width(&t->_real_area);
    data_to_find.h = lv_area_get_height(&t->_real_area);

    data_to_find.slot.size = LV_DRAW_BUF_SIZE(data_to_find.w, data_to_find.h,
                                              LV_COLOR_FORMAT_ARGB8888) + data_to_find.draw_dsc->dsc_size;

    /*user_data stores the renderer to differentiate it from SW rendered tasks.
     *However the cached surface is independent from the renderer so use NULL user_data*/
    void * user_data_saved = data_to_find.draw_dsc->user_data;
    data_to_find.draw_dsc->user_data = NULL;

    /*Absolute coordinates are different for the same draw_dsc on a different position.
     *So make everything relative to 0;0  before caching*/
    if(t->type == LV_DRAW_TASK_TYPE_TRIANGLE) {
        lv_draw_triangle_dsc_t * tri_dsc = (lv_draw_triangle_dsc_t *)data_to_find.draw_dsc;
        tri_dsc->p[0].x -= t->area.x1;
        tri_dsc->p[0].y -= t->area.y1;
        tri_dsc->p[1].x -= t->area.x1;
        tri_dsc->p[1].y -= t->area.y1;
        tri_dsc->p[2].x -= t->area.x1;
        tri_dsc->p[2].y -= t->area.y1;
    }
#if LV_USE_VECTOR_GRAPHIC
    else if(t->type == LV_DRAW_TASK_TYPE_VECTOR) {
        lv_draw_vector_task_dsc_t * vector_task_dsc = (lv_draw_vector_task_dsc_t *)data_to_find.draw_dsc;
        vector_task_dsc->base.dsc_size = sizeof(lv_draw_vector_task_dsc_t);
    }
#endif

    lv_area_move(&t->area, -a.x1, -a.y1);
    lv_area_move(&t->_real_area, -a.x1, -a.y1);

    lv_cache_entry_t * entry_cached = lv_cache_acquire_or_create(u->surface_cache, &data_to_find, u);

    lv_area_move(&t->area, a.x1, a.y1);
    lv_area_move(&t->_real_area, a.x1, a.y1);

    if(!entry_cached) {
        return;
    }

    data_to_find.draw_dsc->user_data = user_data_saved;

    cache_data_t * data_cached = lv_cache_entry_get_data(entry_cached);
    ite_it2d_surface_t * surface = &data_cached->surface;
    ite_it2d_context_t ctx;

    ite_it2d_ctx_init(&ctx);

    lv_layer_t * dest_layer = t->target_layer;
    ite_it2d_area_t clip_area;
    clip_area.x1 = t->clip_area.x1 - dest_layer->buf_area.x1;
    clip_area.y1 = t->clip_area.y1 - dest_layer->buf_area.y1;
    clip_area.x2 = t->clip_area.x2 - dest_layer->buf_area.x1;
    clip_area.y2 = t->clip_area.y2 - dest_layer->buf_area.y1;

    ite_it2d_surface_t * dst_surface = lv_layer_get_ite_it2d_surface(dest_layer);
    ite_it2d_set_dst_surface(&ctx, dst_surface);

    ite_it2d_rect_t rect;
    rect.x = t->_real_area.x1 - dest_layer->buf_area.x1;
    rect.y = t->_real_area.y1 - dest_layer->buf_area.y1;
    rect.w = data_cached->w;
    rect.h = data_cached->h;

    ite_it2d_set_clip_area(&ctx, &clip_area);
    ite_it2d_enable_clipping(&ctx, true);
    ite_it2d_set_src_surface(&ctx, surface);
    ite_it2d_set_dst_rect(&ctx, &rect);
    ite_it2d_enable_blending(&ctx, true);
    ite_it2d_copy(&ctx);

    lv_cache_release(u->surface_cache, entry_cached, u);

#if LV_USE_VECTOR_GRAPHIC
    /*Do not cache vectors. */
    if(t->type == LV_DRAW_TASK_TYPE_VECTOR) {
        lv_cache_drop(u->surface_cache, &data_to_find, NULL);
    }
#endif
}

static void execute_drawing(lv_draw_ite_it2d_unit_t * u)
{
    lv_draw_task_t * t = u->task_act;

    if(t->type == LV_DRAW_TASK_TYPE_FILL) {
        lv_draw_fill_dsc_t * fill_dsc = t->draw_dsc;
        if((fill_dsc->grad.dir == LV_GRAD_DIR_NONE) ||
           ((fill_dsc->grad.dir <= LV_GRAD_DIR_HOR) && (fill_dsc->radius == 0) && (fill_dsc->grad.stops_count <= 2))) {
            lv_draw_ite_it2d_fill(t, fill_dsc, &t->area);
            return;
        }
    }

    if(t->type == LV_DRAW_TASK_TYPE_BORDER) {
        lv_draw_ite_it2d_border(t, t->draw_dsc, &t->area);
        return;
    }

    if(t->type == LV_DRAW_TASK_TYPE_BOX_SHADOW) {
        lv_draw_ite_it2d_box_shadow(t, t->draw_dsc, &t->area);
        return;
    }

    if(t->type == LV_DRAW_TASK_TYPE_LABEL) {
#if LV_DRAW_ITE_IT2D_CACHE_TEXT_STATIC
        lv_draw_label_dsc_t * label_dsc = t->draw_dsc;
        if(!label_dsc->text_static) {
            lv_draw_ite_it2d_label(t, label_dsc, &t->area);
            return;
        }
#else
        lv_draw_ite_it2d_label(t, t->draw_dsc, &t->area);
        return;
#endif
    }

    if(t->type == LV_DRAW_TASK_TYPE_IMAGE) {
        lv_draw_image_dsc_t * image_dsc = t->draw_dsc;
        if(lv_ite_it2d_is_src_cf_supported(image_dsc->header.cf) &&
           (lv_area_get_width(&image_dsc->image_area) >= image_dsc->header.w) &&
           (lv_area_get_height(&image_dsc->image_area) >= image_dsc->header.h)) {
            lv_draw_ite_it2d_image(t, image_dsc, &t->area);
            return;
        }
    }

    if(t->type == LV_DRAW_TASK_TYPE_LAYER) {
        lv_draw_ite_it2d_layer(t, t->draw_dsc, &t->area);
        return;
    }

    if(t->type == LV_DRAW_TASK_TYPE_LINE) {
        lv_draw_ite_it2d_line(t, t->draw_dsc);
        return;
    }

    if(t->type == LV_DRAW_TASK_TYPE_ARC) {
        lv_draw_ite_it2d_arc(t, t->draw_dsc, &t->area);
        return;
    }

    if(t->type == LV_DRAW_TASK_TYPE_TRIANGLE) {
        lv_draw_triangle_dsc_t * triangle_dsc = t->draw_dsc;
        if(triangle_dsc->grad.dir == LV_GRAD_DIR_NONE) {
            lv_draw_ite_it2d_triangle(t, t->draw_dsc);
            return;
        }
    }

    if(t->type == LV_DRAW_TASK_TYPE_MASK_RECTANGLE) {
        lv_draw_ite_it2d_mask_rect(t, t->draw_dsc, &t->area);
        return;
    }

    draw_from_cached_surface(u);
}

static void layer_copy_image_bg(lv_layer_t * layer, lv_layer_t * dest_layer, const lv_draw_image_dsc_t * draw_dsc,
                                const lv_area_t * coords)
{
    bool transformed = (draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                        draw_dsc->scale_y != LV_SCALE_NONE ? true : false) &&
                       (draw_dsc->base.user_data == NULL);
    ite_it2d_surface_t * dst_surface = lv_layer_get_ite_it2d_surface(layer);
    ite_it2d_surface_t * src_surface = lv_layer_get_ite_it2d_surface(dest_layer);

    ite_it2d_context_t ctx;
    ite_it2d_ctx_init(&ctx);

    ite_it2d_rect_t rect;

    if(!transformed) {
        ite_it2d_set_dst_surface(&ctx, src_surface);
        ite_it2d_set_src_surface(&ctx, dst_surface);

        rect.x = coords->x1;
        rect.y = coords->y1;
        rect.w = lv_area_get_width(coords);
        rect.h = lv_area_get_height(coords);
        ite_it2d_set_src_rect(&ctx, &rect);

        ite_it2d_copy(&ctx);
    }
    else {
        ite_it2d_set_dst_surface(&ctx, dst_surface);
        ite_it2d_set_src_surface(&ctx, src_surface);

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
        xform_dsc.flags.inverse = 1;
        ite_it2d_transform_copy(&ctx, &xform_dsc);
    }
}

static void * buf_malloc(size_t size_bytes, lv_color_format_t color_format)
{
    LV_UNUSED(color_format);

    /*Allocate larger memory to be sure it can be aligned as needed*/
    size_bytes += LV_DRAW_BUF_ALIGN - 1;
    return lv_draw_ite_it2d_alloc_buffer(size_bytes);
}

static void buf_free(void * buf)
{
    ite_it2d_wait_idle();
    lv_draw_ite_it2d_free_buffer(buf);
}

static void draw_buf_to_region(
    const lv_draw_buf_t * draw_buf, const lv_area_t * area,
    lv_uintptr_t * start, lv_uintptr_t * end)
{
    LV_ASSERT_NULL(draw_buf);
    LV_ASSERT_NULL(area);
    LV_ASSERT_NULL(start);
    LV_ASSERT_NULL(end);

    void * buf = draw_buf->data;
    uint32_t stride = draw_buf->header.stride;

    int32_t h = lv_area_get_height(area);
    *start = (lv_uintptr_t)buf + area->y1 * stride;
    *end = *start + h * stride;
}

static void invalidate_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    lv_uintptr_t start;
    lv_uintptr_t end;
    draw_buf_to_region(draw_buf, area, &start, &end);
    lv_sys_cache_data_invd_range((void *)start, end - start);
}

static void flush_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    lv_uintptr_t start;
    lv_uintptr_t end;
    draw_buf_to_region(draw_buf, area, &start, &end);
    lv_sys_cache_data_flush_range((void *)start, end - start);
}

#endif /*LV_USE_DRAW_ITE_IT2D*/
