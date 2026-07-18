#include <lvgl.h>
#include "lvgl_display.h"
#include "display/lv_display_private.h"
#include "../src/draw/ite_it2d/lv_draw_ite_it2d.h"

#if LV_USE_ROTATE_ITE_IT2D

#define DISPLAY_WIDTH  CFG_LCD_HEIGHT
#define DISPLAY_HEIGHT CFG_LCD_WIDTH
#define DISPLAY_PITCH  (DISPLAY_WIDTH * CFG_LCD_BPP)

#endif /* LV_USE_ROTATE_ITE_IT2D */

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	if (lv_display_flush_is_last(disp)) {
		struct lvgl_disp_data *data =
			(struct lvgl_disp_data *)lv_display_get_user_data(disp);
#if LV_USE_ROTATE_ITE_IT2D
		ite_it2d_transform_copy(&data->rot_ctx, &data->rot_dsc);
#endif /* LV_USE_ROTATE_ITE_IT2D */
		ite_it2d_present(&data->disp_surf);
	}

	lv_display_flush_ready(disp);
}


#if LV_USE_ROTATE_ITE_IT2D

static void res_chg_event_cb(lv_event_t *e)
{
	lv_display_t *disp = lv_event_get_current_target(e);
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(disp);
	ite_it2d_surface_t *rot_surf = &data->rot_surf;
	ite_it2d_transform_dsc_t *rot_dsc = &data->rot_dsc;
	lv_display_rotation_t rot = lv_display_get_rotation(disp);

	if ((rot == LV_DISPLAY_ROTATION_90) || (rot == LV_DISPLAY_ROTATION_270)) {
		rot_surf->width = DISPLAY_HEIGHT;
		rot_surf->height = DISPLAY_WIDTH;
		rot_surf->pitch =
			rot_surf->width * ite_it2d_bits_per_pixel(rot_surf->flags.format) / 8;
		if (rot == LV_DISPLAY_ROTATION_90) {
			rot_dsc->pos.x = 0;
			rot_dsc->pos.y = DISPLAY_HEIGHT;
			rot_dsc->angle = 270.0f;
		} else {
			rot_dsc->pos.x = DISPLAY_WIDTH;
			rot_dsc->pos.y = 0;
			rot_dsc->angle = 90.0f;
		}
	} else {
		LV_ASSERT((rot == LV_DISPLAY_ROTATION_0) || (rot == LV_DISPLAY_ROTATION_180));
		rot_surf->width = DISPLAY_WIDTH;
		rot_surf->height = DISPLAY_HEIGHT;
		rot_surf->pitch = DISPLAY_PITCH;
		if (rot == LV_DISPLAY_ROTATION_0) {
			rot_dsc->pos.x = 0;
			rot_dsc->pos.y = 0;
			rot_dsc->angle = 0.0f;
		} else {
			rot_dsc->pos.x = DISPLAY_WIDTH;
			rot_dsc->pos.y = DISPLAY_HEIGHT;
			rot_dsc->angle = 180.0f;
		}
	}
	ite_it2d_set_src_surface(&data->rot_ctx, &data->rot_surf);
}
#endif /* LV_USE_ROTATE_ITE_IT2D */

static void layer_deinit(lv_display_t * disp, lv_layer_t * layer)
{
    if (layer->user_data) {
        ite_it2d_surface_t * surface = layer->user_data;
        ite_it2d_destroy_surface(surface);
				lv_free(surface);
				layer->user_data = NULL;
    }
}

int set_lvgl_rendering_cb(lv_display_t *disp)
{
	lv_display_set_flush_cb(disp, flush_cb);

	disp->layer_deinit = layer_deinit;

#if LV_USE_ROTATE_ITE_IT2D
	lv_display_add_event_cb(disp, res_chg_event_cb, LV_EVENT_RESOLUTION_CHANGED, NULL);
#endif

	return 0;
}
