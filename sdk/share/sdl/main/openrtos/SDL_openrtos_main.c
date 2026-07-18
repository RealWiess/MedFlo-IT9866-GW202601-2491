/* Include the SDL main definition header */
#include "SDL_main.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "ite/itp.h"
#include "ite/ite_risc.h"

#if defined(CFG_CANBUS_HAL_ENABLE)
#include "../can_hal/can_handle.h"
#endif

#if defined(CFG_WATCHDOG_ENABLE) && defined(CFG_WATCHDOG_INTR)
#include "alt_cpu/alt_cpu_device.h"
#include "alt_cpu/watchdogMonitor/watchdogMonitor.h"
#endif

#define MAIN_STACK_SIZE 100000

static void* MainTask(void* arg)
{
    char *argv[2];
	
#if defined(CFG_CANBUS_HAL_ENABLE)
	canInit();
#endif
    // init pal
    itpInit();

    argv[0] = "SDL_app";
    argv[1] = NULL;

#if defined(CFG_WATCHDOG_ENABLE) && defined(CFG_WATCHDOG_INTR)
    int altCpuEngineType = ALT_CPU_WATCHDOG_MONITOR;
    printf("Watch Dog Interrupt Mode enable, Turn on watch dog monitor....\n");
    //Load Watch Dog Monitor on ALT CPU
    printf("Load Watchdog monitor device..\n");
    ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_ALT_CPU_SWITCH_ENG, &altCpuEngineType);
    ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_INIT, NULL);
    ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_WATCHDOG_MONITOR_RUN, NULL);
    printf("Watch Dog monitor is running\n");
#endif
    SDL_main(1, argv);
	
    return NULL;
}

#ifdef main
#undef main
int
main(void)
{
    pthread_t task;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, MAIN_STACK_SIZE);
    pthread_create(&task, &attr, MainTask, NULL);

    /* Now all the tasks have been started - start the scheduler. */
    vTaskStartScheduler();

    /* Should never reach here! */
    return 0;

}
#endif

/* vi: set ts=4 sw=4 expandtab: */
