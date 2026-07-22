#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <lvgl.h>
#include "ite/itp.h"
#include "ui_screen.h"
#include "ble_init.h"
#include "wifi_app.h"

extern int lvgl_init(void);

void* MainFunc(void* arg)
{
    printf("\n===== gw_lvgl_test START =====\n");
    fflush(stdout);

    /* 1. itpInit — SDK lv_demos standard flow */
    printf("[1] itpInit...\n"); fflush(stdout);
    itpInit();
    printf("[1] itpInit OK\n"); fflush(stdout);

#if LV_USE_DRAW_ITE_IT2D
    /* 2. LVGL init */
    printf("[2] lvgl_init...\n"); fflush(stdout);
    lvgl_init();
    printf("[2] lvgl_init OK\n"); fflush(stdout);
#endif

    /* 3. POST_RESET */
    printf("[3] POST_RESET...\n"); fflush(stdout);
    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    printf("[3] POST_RESET OK\n"); fflush(stdout);

    /* 4. Gateway HMI */
    printf("[4] ui_screen_init...\n"); fflush(stdout);
    ui_screen_init();
    printf("[4] HMI OK\n"); fflush(stdout);

    /* 5. WiFi disabled for now */
    printf("[5] WiFi disabled\n"); fflush(stdout);

    /* 6. First frame */
    lv_timer_handler();

    /* 7. BACKLIGHT ON */
    printf("[7] BACKLIGHT_ON!\n"); fflush(stdout);
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_ON, NULL);

    printf("===== MAIN LOOP =====\n"); fflush(stdout);
    uint32_t last_update = 0;
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        usleep(sleep_ms > 100 ? 100000 : sleep_ms * 1000);

        /* Update HMI every 2 seconds */
        uint32_t now = lv_tick_get();
        if (now - last_update > 2000) {
            last_update = now;
            ui_screen_update_wifi(
                wifi_app_get_ssid(),
                wifi_app_get_ip(),
                wifi_app_is_connected());
        }
    }
    return 0;
}
