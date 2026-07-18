#include <sys/ioctl.h>
#include <unistd.h>
#include "libxml/parser.h"
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"
#include "canbus_fmt.h"
#include "bd/bt_led.h"

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

#ifdef _WIN32
    #include <crtdbg.h>
#else
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/task.h"
#endif
extern void* TestBLE(void* arg);
int SDL_main(int argc, char *argv[])
{
    int ret = 0;
    int restryCount = 0;

    Test_Time("Start");
    #if ( USE_BOOT_INITIAL )
    reset_Sensor_start();   Test_Time("End reset_Sensor_start()");
    usleep(1000);           // only need 1 ms , spec. page 92 
    reset_Sensor_end();     Test_Time("End reset_Sensor_end()");
    initial_Sensor();       Test_Time("End initial_Sensor()");
    #endif
    
    #if ( 0 ) // for test
    printf("<%s> Reset Enable = %d , GPIO = %d \n", __func__, CFG_SENSOR_RESETPIN_ENABLE, CFG_SN1_GPIO_RST);
    #endif

    printBoardInfo();       Test_Time("End printBoardInfo()");
#if !defined(CFG_LCD_INIT_ON_BOOTING) && !defined(CFG_BL_SHOW_LOGO)
    ScreenClear();          Test_Time("End ScreenClear()");
#endif

    Test_Time("CFG_CHECK_FILES_CRC_ON_BOOTING");
#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    BackupInit();           Test_Time("End BackupInit()");
retry_backup:
    ret = UpgradeInit();    Test_Time("End 1 UpgradeInit()");
    if (ret)
    {
        if (++restryCount > 2)
        {
            printf("cannot recover from backup....\n");
            goto end;
        }
        BackupRestore();    Test_Time("End BackupRestore()");
        goto retry_backup;
    }
    BackupSyncFile();       Test_Time("End BackupSyncFile()");
#else
    ret = UpgradeInit();    Test_Time("End 2 UpgradeInit()");
    if (ret)
        goto end;
#endif

#ifdef	CFG_DYNAMIC_LOAD_TP_MODULE
	//This function must be in front of SDL_Init().
	DynamicLoadTpModule();  Test_Time("End DynamicLoadTpModule()");
#endif

    #if ( 0 & USE_BOOT_INITIAL )
    reset_Sensor_end();     Test_Time("End reset_Sensor_end()");
    initial_Sensor();       Test_Time("End initial_Sensor()");
    #endif

    Test_Time("Str SDL_Init()");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    Test_Time("End SDL_Init()");
    
    ConfigInit();           Test_Time("End ConfigInit()");
    CanFmtInit();           Test_Time("End CanFmtInit()");

    Test_Time("CFG_NET_ENABLE");
#ifdef CFG_NET_ENABLE
    NetworkInit();          Test_Time("End NetworkInit()");
    #ifdef CFG_NET_WIFI

    #else
    WebServerInit();        Test_Time("End WebServerInit()");
    #endif

#endif // CFG_NET_ENABLE

                            Test_Time("Str ScreenInit()");
    ScreenInit();           Test_Time("End ScreenInit()");
    PeripheralInit();       Test_Time("End PeripheralInit()");

    // BLE starts AFTER WiFi is connected to avoid RTL8821 RF conflict
    Test_Time("CFG_BTA_ENABLE");
    printf("<%s> CFG_BUILD_NIMBLE = %d \n", __func__, CFG_BUILD_NIMBLE);
#ifdef CFG_BUILD_NIMBLE
    bt_led_init();                                  Test_Time("End bt_led_init()");
    nimbleInit();                                   Test_Time("End nimbleInit()");
#endif

    //ExternalInit();

    //canMsgQueueInit();
    //dashboardQueueInit();   Test_Time("End dashboardQueueInit()");
    //initDashboardData();    Test_Time("End initDashboardData()");
#ifdef CFG_UART1_ENABLE
    SysUartInit();          Test_Time("End SysUartInit()");
#endif

    StorageInit();          Test_Time("End StorageInit()");
    AudioInit();            Test_Time("End AudioInit()");
    PhotoInit();            Test_Time("End PhotoInit()");

    SceneInit();            Test_Time("End SceneInit()");
    SceneLoad();            Test_Time("End SceneLoad()");
    ret = SceneRun();       Test_Time("End SceneRun()");

                            Test_Time("Str SceneExit()");
    SceneExit();            Test_Time("End SceneExit()");

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_EXIT, NULL);

    PhotoExit();
    AudioExit();
    //ExternalExit();
#ifdef CFG_UART1_ENABLE
    SysUartExit();
#endif
    //exitDashboardData();
    //dashboardQueueExit();


//#ifdef CFG_NET_ENABLE
//    if (ret != QUIT_UPGRADE_WEB)
//        WebServerExit();
//
//    xmlCleanupParser();
//#endif // CFG_NET_ENABLE

    ConfigExit();
    SDL_Quit();

#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    BackupDestroy();
#endif

#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#else
    if (0)
    {
    #if (configUSE_TRACE_FACILITY == 1)
        static signed char buf[2048];
        vTaskList(buf);
        puts(buf);
    #endif
        malloc_stats();

    #ifdef CFG_DBG_RMALLOC
        Rmalloc_stat(__FILE__);
    #endif
    }
#endif // _WIN32

end:
    ret = UpgradeProcess(ret);
    itp_codec_standby();
    exit(ret);
    return ret;
}
