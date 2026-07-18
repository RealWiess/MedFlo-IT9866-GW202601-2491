#include <lvgl.h>
#include "ite/ith.h"

extern void BootInit(void);
extern void* MainFunc(void* arg);

int main(void)
{
    BootInit();
    lv_init();

    int32_t        zoom_level         = 100;
    bool           allow_dpi_override = false;
    bool           simulator_mode     = false;
    lv_display_t * display            = lv_windows_create_display(
        L"LVGL Windows Application Display 1", CFG_LCD_WIDTH, CFG_LCD_HEIGHT, zoom_level,
        allow_dpi_override, simulator_mode);
    if (!display)
    {
        return -1;
    }

    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return -1;
    }

    lv_indev_t * pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev)
    {
        return -1;
    }

    lv_indev_t * keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev)
    {
        return -1;
    }

    lv_indev_t * encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev)
    {
        return -1;
    }

    MainFunc(NULL);

    return 0;
}
