#include <pthread.h>
#include "ctrlboard.h"
#include "network_config.h"
extern void NetworkWifiPreSetting(void);
extern void NetworkWifiProcess(void);


/* Eason Refined in Mar. 2020 */

/* Function Declaration */
#ifdef CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED
void WifiPowerOn(void);
void WifiPowerOff(void);
#endif

static void* NetworkEthTask(void* arg)
{
#if defined(CFG_NET_ETHERNET)
    NetworkPreSetting();

    while (!NetworkIsExit())
    {
        /* Network Functions Process*/
        NetworkEthernetProcess();

        sleep(1);
    }
#endif

    return NULL;
}

static void* NetworkWifiTask(void* arg)
{
#if defined(CFG_NET_WIFI)
    NetworkWifiPreSetting();
#ifdef CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED
    WifiPowerOn();
#endif

    while (1)
    {
        NetworkWifiProcess();

        sleep(1);
    }
#endif

    return NULL;
}

static void CreateWorkerThread(void *(*start_routine)(void *), void *arg)
{
    pthread_t      task;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, start_routine, arg);
}

#ifdef CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED
// contral wifi power on
static int gWifiPowerOn =0;
extern WIFI_MGR_SETTING     gWifiSetting;

#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
void WifiFirstPowerOn()
{
        itpRegisterDevice(ITP_DEVICE_SDIO, &itpDeviceSdio);
        ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);

        itpRegisterDevice(ITP_DEVICE_WIFI_NGPL, &itpDeviceWifiNgpl);
        printf("====>itpInit itpRegisterDevice(ITP_DEVICE_WIFI_NGPL)\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_INIT, NULL);


}

void WifiPowerOn(void)
{
    if (gWifiPowerOn==0){
        WifiFirstPowerOn();
        gWifiPowerOn++;
    } else {
        wifi_on(RTW_MODE_STA);
        wifiMgr_init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    }

}

void WifiPowerOff(void)
{
    wifiMgr_terminate();

    wifi_off();

}
#elif defined(CFG_NET_WIFI_SDIO_VND_MHD)

#if defined (CFG_SD0_ENABLE) && defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#elif defined (CFG_SD0_ENABLE)
#define SD_PORT_NUM 0
#elif defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#endif

void WifiFirstPowerOn(void)
{
    wifi_drv_on(SD_PORT_NUM);
}



void WifiPowerOn(void)
{
    if (gWifiPowerOn==0){
        WifiFirstPowerOn();
        gWifiPowerOn++;
    } else {
        //wifi_on(RTW_MODE_STA);
        wifiMgr_init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    }

}

void WifiPowerOff(void)
{
    wifiMgr_terminate();

    wifi_off();

}
#elif defined(CFG_NET_WIFI_SDIO_NGPL_ATBM6031)

void WifiFirstPowerOn(void)
{
    wifi_drv_on(1);
}



void WifiPowerOn()
{
    if (gWifiPowerOn==0){
        printf("WifiPowerOn ----------------------------- \n");
        WifiFirstPowerOn();
        gWifiPowerOn++;
    } else {
        //wifi_on(RTW_MODE_STA);
        wifiMgr_init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    }

}

void WifiPowerOff()
{

    if(gWifiPowerOn)
    {
	    //wifiMgr_terminate();
	    gWifiPowerOn =0;
	    wifi_off();
    }


}


#endif

#endif //#ifdef CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED

void NetworkInit(void)
{
    /* Create Network Thread */
    /* Ethernet Thread */
    CreateWorkerThread(NetworkEthTask,  NULL);
    /* WIFI Thread */
    CreateWorkerThread(NetworkWifiTask, NULL);
}

