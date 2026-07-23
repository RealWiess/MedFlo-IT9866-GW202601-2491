#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <lvgl.h>
#include "ite/itp.h"
#include "ui_screen.h"
#include "ble_init.h"
#include "wifi_app.h"
#include "mqtt_app.h"

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

    /* 4. Gateway HMI — splash first, backlight ON immediately */
    printf("[4] ui_screen_init...\n"); fflush(stdout);
    ui_screen_init();
    lv_timer_handler();  /* render splash screen */
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_ON, NULL);
    printf("[4] HMI OK + BACKLIGHT ON\n"); fflush(stdout);

    /* 5. Init WiFi + BLE while splash is visible */
    printf("[5] WiFi+BLE start...\n"); fflush(stdout);
    wifi_app_start();
    ble_init();
    printf("[5] WiFi+BLE started\n"); fflush(stdout);

    /* TEST: call mqtt_app_init with empty task — bisect crash cause */
    mqtt_app_init();
    printf("[5] MQTT init done\n"); fflush(stdout);

    printf("===== MAIN LOOP =====\n"); fflush(stdout);
    uint32_t last_update = 0;
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        usleep(sleep_ms > 100 ? 100000 : sleep_ms * 1000);

        /* Update HMI every 2 seconds */
        uint32_t now = lv_tick_get();
        if (now - last_update > 5000) {
            last_update = now;
            ui_screen_update_wifi(
                wifi_app_get_ssid(),
                wifi_app_get_ip(),
                wifi_app_is_connected());
            ui_screen_update_ble(
                ble_get_device_count(),
                true);
        }
    }
    return 0;
}
