#include <sys/ioctl.h>
#include <unistd.h>
#include <lvgl.h>
#include "lv_demos.h"
#include "libxml/parser.h"
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"
#include "canbus_fmt.h"
#include "bd/bt_led.h"
#include "bd/bt_main.h"
#include "custom/dashboard_data.h"
#include "wsp/peripheral.h"
#include "ui_screen.h"


//========================================================
// add by UM Eric
//--------------------------------------------------------
#define USE_BOOT_INITIAL        0   // 0 = disable , 1 = use main.c initial sensor
#define USE_SHOW_TIME           1   // 0 = disable , 1 = show the SOC time (ms)
//--------------------------------------------------------
#if ( USE_BOOT_INITIAL )
extern void reset_Sensor_start();
extern void reset_Sensor_end();
extern void initial_Sensor();
#endif
//--------------------------------------------------------
#if ( USE_SHOW_TIME )
#define Test_Time(x)        printf("<%s> %3d ms - %s \n",      __func__, SDL_GetTicks(), x)
#define Test_Info(x, y)     printf("<%s> %3d ms - %s = %d \n", __func__, SDL_GetTicks(), x, y)
#define Test_Info_f(x, y)   printf("<%s> %3d ms - %s = %f \n", __func__, SDL_GetTicks(), x, y)
#else
#define Test_Time(x)
#define Test_Info(x)
#define Test_Info(x)_f
#endif
//========================================================

extern int lvgl_init(void);

#ifdef _WIN32
    #include <crtdbg.h>
#else
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/task.h"
#endif
extern void* TestBLE(void* arg);

/**
 * GW202601_LVGL — itpInit test (proven to work)
 */
int SDL_main(int argc, char *argv[])
{
    printf("\n===== ALIVE =====\n");
    fflush(stdout);

    itpInit();
    printf(">>> itpInit OK\n"); fflush(stdout);

    lvgl_init();
    printf(">>> lvgl_init OK\n"); fflush(stdout);

    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    lv_demo_widgets();
    lv_timer_handler();

    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_ON, NULL);
    printf(">>> BACKLIGHT ON!\n"); fflush(stdout);

    while (1) {
        lv_timer_handler();
        usleep(5000);
    }
    return 0;
}
