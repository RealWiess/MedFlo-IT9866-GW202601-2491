#include <sys/ioctl.h>
#include <unistd.h>
#include <lvgl.h>
#include "lv_demos.h"
#include "ite/itp.h"

#define DEMO_RENDER_DURATION_SEC  5

extern int lvgl_init(void);

void* MainFunc(void* arg)
{
  uint32_t total_us = 0;

  itpInit();
#if LV_USE_DRAW_ITE_IT2D
  lvgl_init();
#endif
  ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);

#if LV_USE_ROTATE_ITE_IT2D
	lv_disp_set_rotation(NULL, LV_DISPLAY_ROTATION_90);
#endif

#if defined(CFG_DEMO_MUSIC)
	lv_demo_music();
#elif defined(CFG_DEMO_BENCHMARK)
	lv_demo_benchmark();
#elif defined(CFG_DEMO_STRESS)
	lv_demo_stress();
#elif defined(CFG_DEMO_WIDGETS)
	lv_demo_widgets();
#elif defined(CFG_DEMO_KEYPAD_AND_ENCODER)
	lv_demo_keypad_encoder();
#elif defined(CFG_DEMO_RENDER)
  lv_demo_render_scene_t cur_scene = LV_DEMO_RENDER_SCENE_FILL;
  lv_demo_render(cur_scene, 255);
#elif defined(CFG_DEMO_VECTOR_GRAPHIC)
	lv_demo_vector_graphic_not_buffered();
#endif
  lv_timer_handler();
  ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_ON, NULL);

  while (1) {
		uint32_t sleep_us = lv_timer_handler() * 1000;
    if (sleep_us > 0) {
      usleep(LV_MIN(sleep_us, INT32_MAX));
      total_us += sleep_us;
    } else {
      sched_yield();
    }
#if defined(CFG_DEMO_RENDER)
    if (total_us >= DEMO_RENDER_DURATION_SEC * 1000 * 1000) {
      cur_scene = (cur_scene + 1) % LV_DEMO_RENDER_SCENE_NUM;
      lv_demo_render(cur_scene, 255);
      total_us = 0;
    }
#endif
  }

  return NULL;
}
