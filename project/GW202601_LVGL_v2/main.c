/**
 * GW202601_LVGL_v2 — main.c
 *
 * Clean LVGL entry point based on SDK lv_demos pattern.
 * Init sequence: itpInit() → lvgl_init() → POST_RESET → display → BACKLIGHT_ON
 *
 * Stage 1: lv_demo_widgets — verify LCD backlight + display works
 * Stage 2: ui_screen.c dual-ring HMI
 * Stage 3: WiFi + BLE + MQTT Gateway functionality
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <lvgl.h>
#include "lv_demos.h"
#include "ite/itp.h"

extern int lvgl_init(void);

void* MainFunc(void* arg)
{
    printf("\n===== GW202601_LVGL_v2 MainFunc START =====\n");
    fflush(stdout);

    /* 1. Initialize all ITE platform hardware (LCD, backlight, I2C, WiFi, etc.) */
    printf("[1/6] itpInit()...\n");
    fflush(stdout);
    itpInit();
    printf("[1/6] itpInit() DONE\n");
    fflush(stdout);

#if LV_USE_DRAW_ITE_IT2D
    /* 2. Initialize LVGL with IT2D hardware acceleration */
    printf("[2/6] lvgl_init()...\n");
    fflush(stdout);
    lvgl_init();
    printf("[2/6] lvgl_init() DONE\n");
    fflush(stdout);
#endif

    /* 3. Post-reset screen (load next LCD init script) */
    printf("[3/6] POST_RESET...\n");
    fflush(stdout);
    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    printf("[3/6] POST_RESET DONE\n");
    fflush(stdout);

    /* 4. Launch demo / UI */
    printf("[4/6] lv_demo_widgets()...\n");
    fflush(stdout);
#if defined(CFG_DEMO_WIDGETS)
    lv_demo_widgets();
#elif defined(CFG_DEMO_BENCHMARK)
    lv_demo_benchmark();
#elif defined(CFG_DEMO_STRESS)
    lv_demo_stress();
#elif defined(CFG_DEMO_MUSIC)
    lv_demo_music();
#endif
    printf("[4/6] demo DONE\n");
    fflush(stdout);

    /* 5. Process first frame */
    printf("[5/6] lv_timer_handler()...\n");
    fflush(stdout);
    lv_timer_handler();
    printf("[5/6] lv_timer_handler() DONE\n");
    fflush(stdout);

    /* 6. Turn on backlight — THIS IS THE CRITICAL LINE */
    printf("[6/6] BACKLIGHT_ON...\n");
    fflush(stdout);
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_ON, NULL);
    printf("[6/6] BACKLIGHT_ON DONE!\n");
    fflush(stdout);

    printf("===== LVGL main loop starting =====\n");

    /* 7. Main loop */
    while (1) {
        uint32_t sleep_us = lv_timer_handler() * 1000;
        if (sleep_us > 0) {
            usleep(LV_MIN(sleep_us, INT32_MAX));
        } else {
            sched_yield();
        }
    }

    return NULL;
}
