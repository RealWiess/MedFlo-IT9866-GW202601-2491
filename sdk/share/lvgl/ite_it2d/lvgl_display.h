#ifndef LVGL_DISPLAY_H_
#define LVGL_DISPLAY_H_

#include <lvgl.h>
#include "ite_it2d.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lvgl_disp_data {
	ite_it2d_surface_t *target_surf;
	ite_it2d_surface_t disp_surf;
#if LV_USE_ROTATE_ITE_IT2D
	ite_it2d_surface_t rot_surf;
	ite_it2d_context_t rot_ctx;
	ite_it2d_transform_dsc_t rot_dsc;
#endif
};

int set_lvgl_rendering_cb(lv_display_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_DISPLAY_H_ */
