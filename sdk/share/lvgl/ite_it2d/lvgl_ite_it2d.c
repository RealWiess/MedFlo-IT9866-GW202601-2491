#include <sys/errno.h>
#include <string.h>
#include <lvgl.h>
#include "lvgl_display.h"
#include "../src/draw/ite_it2d/lv_draw_ite_it2d.h"
#include "ite_itdcps.h"
#include "ite_itdpu.h"
#include "ite_itjpeg.h"
#include "ite_itvp.h"
#include "tslib.h"
#include "ite/itp.h"

extern uint32_t __lcd_base_c;

static lv_display_t *display;
static struct lvgl_disp_data disp_data;

static lv_indev_t *indev;
static struct tsdev *kscan_dev;

static int lvgl_allocate_rendering_buffers(lv_display_t *display)
{
	int err = 0;
  uint8_t* buf0 = (uint8_t*)ithLcdGetBaseAddrA();
  uint8_t* buf1 = (uint8_t*)ithLcdGetBaseAddrB();
  uint32_t buf_size = ithLcdGetPitch() * ithLcdGetHeight();

#if LV_USE_ROTATE_ITE_IT2D
	lv_display_set_buffers(display, (void*)&__lcd_base_c, NULL, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
	lv_display_set_buffers(display, (void*)buf1, buf0, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#endif

	memset(buf0, 0, buf_size);
	lv_sys_cache_data_flush_range(buf0, buf_size);

	err = ite_it2d_create_display_surface(&disp_data.disp_surf);
	disp_data.target_surf = &disp_data.disp_surf;

#if LV_USE_ROTATE_ITE_IT2D
	ite_it2d_ctx_init(&disp_data.rot_ctx);
	ite_it2d_init_transform_dsc(&disp_data.rot_dsc);
	disp_data.rot_surf.flags.format = disp_data.disp_surf.flags.format;
	disp_data.rot_surf.flags.static_buf = 1;
	disp_data.rot_surf.buf = (uint8_t*)&__lcd_base_c;
	ite_it2d_set_dst_surface(&disp_data.rot_ctx, &disp_data.disp_surf);
	disp_data.target_surf = &disp_data.rot_surf;
#endif /* LV_USE_DRAW_ITE_IT2D */

	return err;
}

static void lvgl_input_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	struct ts_sample samp={0,0,0};

	if (ts_read(kscan_dev, &samp, 1) < 0) {
		return;
	}

	data->point.x = samp.x;
	data->point.y = samp.y;
	data->state = samp.pressure ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static int lvgl_init_input_devices(void)
{
#if defined(CFG_TOUCH_I2C0)
    kscan_dev = ts_open(":i2c0",0);
#elif defined(CFG_TOUCH_I2C1)
    kscan_dev = ts_open(":i2c1",0);
#elif defined(CFG_TOUCH_I2C2)
    kscan_dev = ts_open(":i2c2",0);
#elif defined(CFG_TOUCH_I2C3)
    kscan_dev = ts_open(":i2c3",0);
#elif defined(CFG_TOUCH_SPI)
    kscan_dev = ts_open(":spi",0);
#elif defined(CFG_TOUCH_USB)
    kscan_dev = ts_open(":tp",0);
#endif

	if (kscan_dev == NULL) {
		LV_LOG_ERROR("Keyboard scan device not found.");
		return -ENODEV;
	}

	if (ts_config(kscan_dev) < 0) {
		LV_LOG_ERROR("Could not configure keyboard scan device.");
		return -ENODEV;
	}

	indev = lv_indev_create();
	if (indev == NULL) {
		return -EINVAL;
	}

	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, lvgl_input_read_cb);

	return 0;
}

int lvgl_init(void)
{
	int err = it2d_init();
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_ITE_ITDCPS
  err = itdcps_init();
	if (err < 0) {
		return err;
	}
#endif

#ifdef CONFIG_ITE_ITDPU
  err = itdpu_init();
	if (err < 0) {
		return err;
	}
#endif

#ifdef CONFIG_ITE_ITJPEG
  err = itjpeg_init();
	if (err < 0) {
		return err;
	}
#endif

#ifdef CONFIG_ITE_ITVP
  err = itvp_init();
	if (err < 0) {
		return err;
	}
#endif

	lv_init();
	lv_tick_set_cb(itpGetTickCount);

	display = lv_display_create(ithLcdGetWidth(), ithLcdGetHeight());
	if (!display) {
		return -ENOMEM;
	}
	lv_display_set_user_data(display, &disp_data);

	if (set_lvgl_rendering_cb(display) < 0) {
		LV_LOG_ERROR("Display not supported.");
		return -ENOTSUP;
	}

	err = lvgl_allocate_rendering_buffers(display);
	if (err < 0) {
		return err;
	}

	lv_display_set_render_mode(display, LV_DISPLAY_RENDER_MODE_DIRECT);

	err = lvgl_init_input_devices();
	if (err < 0) {
		LV_LOG_ERROR("Failed to initialize input devices.");
		return err;
	}

	return 0;
}
