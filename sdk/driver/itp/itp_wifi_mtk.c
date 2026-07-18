/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL WiFi functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "itp_cfg.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/tcpip.h"
#include "ite/ite_wifi.h"
#include "gen4m/include/typedef.h"
#include "gl_typedef.h"
#include "gl_wifi_cli.h"

/* === Function Declaration === */
err_t wifiif_init(struct netif *netif);
void  wifiif_shutdown(struct netif *netif);
void  wifiif_info(struct netif *netif, struct net_device_info* info);

int mtk_sdio_enable();

/* === Global Variable === */
static struct dhcp              wifiNetifDhcp;
static struct timeval           wifiLastTime;

static int                      WifiCurrentState = 0, WifiAddNetIf = 0, wifiMscnt, sleep_notify = 2;
static bool                     wifiInited = false, wifiDhcp = false, linkUp = false;
static ip_addr_t                wifiIpaddr, wifiNetmask, wifiGw;

/* === Macro === */
#define STASTATE                1
#define APSTATE                 2

// NULL terminated string for contents of wifi.cfg
uint8_t mtk_wifi_cfg[] = ""
#ifdef MTK_WIFI_USE_BUFFER_BIN_MODE
                         "EfuseBufferModeCal 1\n"
#endif
                         ;

#if defined (CFG_NET_WIFI_SDIO_VND_MTK)
/* ======================== WIFI SDIO type: Start ======================== */
// This function initializes all network interfaces
static void itpWifiNgplLwipInit(struct netif *netif)
{
    printf("%s %d \n", __func__, __LINE__);
    printf("[Itp Wifi] itpWifiNgplLwipInit netif(0x%X)\n", netif);
}

static int WifiRead(int file, char *ptr, int len, void* info)
{
    printf("%s %d \n", __func__, __LINE__);
    return 0;
}

static void WifiSleepNotify(int idx)
{
    sleep_notify = idx; // [initial:2], [sleep -> wakeup:1] , [wakeup -> sleep:0]
    printf("%s %d \n", __func__, __LINE__);
    printf("###sleep notify: %d\n", sleep_notify);
}

static void WifiStartDHCP(void* setting)
{
    printf("%s %d \n", __func__, __LINE__);
    printf("dhcp_start\n");
}

static void WifiReset(ITPEthernetSetting* setting)
{
    printf("%s %d \n", __func__, __LINE__);
}

/* No Use for NGPL SDIO WIFI */
void WifiConnect(int type)
{
    //no use, for compiler
}

void WifiDisconnect(int type)
{
    //no use, for compiler
}

/*************************/
static int WifiIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
        case ITP_IOCTL_INIT:
            printf("%s %d ITP_IOCTL_INIT!\n", __func__, __LINE__);
            //mmpMtkWifiDriverCmdTest(); //Init success = 0
            break;

        case ITP_IOCTL_ENABLE:
            printf("%s %d ITP_IOCTL_ENABLE\n", __func__, __LINE__);
            mtk_sdio_enable();
            break;

        case ITP_IOCTL_DISABLE:
            printf("%s %d ITP_IOCTL_DISABLE\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_IS_AVAIL:
            printf("%s %d ITP_IOCTL_IS_AVAIL\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_IS_CONNECTED:
            printf("%s %d ITP_IOCTL_IS_CONNECTED\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFI_START_DHCP:
            printf("%s %d ITP_IOCTL_WIFI_START_DHCP\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFI_STOP_DHCP:
            printf("%s %d ITP_IOCTL_WIFI_STOP_DHCP\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_RENEW_DHCP:
            printf("%s %d ITP_IOCTL_RENEW_DHCP\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            printf("%s %d ITP_IOCTL_WIFI_ADD_NETIF\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_IS_DEVICE_READY:
            printf("%s %d ITP_IOCTL_IS_DEVICE_READY\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_RESUME:
            printf("%s %d ITP_IOCTL_RESUME\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_EXIT:
            printf("%s %d ITP_IOCTL_EXIT\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_RESET:
            printf("%s %d ITP_IOCTL_RESET\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_RESET_DEFAULT:
            printf("%s %d ITP_IOCTL_RESET_DEFAULT\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_SLEEP:
            printf("%s %d ITP_IOCTL_SLEEP\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFI_SLEEP_STATUS:
            printf("%s %d ITP_IOCTL_WIFI_SLEEP_STATUS\n", __func__, __LINE__);
            break;


        case ITP_IOCTL_GET_INFO:
            printf("%s %d ITP_IOCTL_GET_INFO\n", __func__, __LINE__);
            {
                char *p[] = {"wlan", "driver", "STAT GROUP 127"};
                iwpriv_cli(3, p);
            }
            break;

        case ITP_IOCTL_WIFIAP_ENABLE:
            printf("%s %d ITP_IOCTL_WIFIAP_ENABLE\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFIAP_DISABLE:
            printf("%s %d ITP_IOCTL_WIFIAP_DISABLE\n", __func__, __LINE__);
            break;

        case ITP_IOCTL_WIFI_NETIF_SHUTDOWN:
            printf("%s %d ITP_IOCTL_WIFI_NETIF_SHUTDOWN\n", __func__, __LINE__);
            break;

        default:
            errno = (ITP_DEVICE_WIFI << ITP_DEVICE_ERRNO_BIT) | 1;
            printf("[ITP_WIFI][SDIO Type] ERROR : ITP_IOCTL (%d)\n", request);
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceWifi =
{
    ":wifi",
    itpOpenDefault,
    itpCloseDefault,
    WifiRead,
    itpWriteDefault,
    itpLseekDefault,
    WifiIoctl,
    NULL
};

int mmpMtkWifiDriverRegister(void)
{
    extern int32_t wlanProbe(void *pvData, void *pvDriverData);
    extern void wlanRemove(void *pvData);
    extern uint32_t glRegisterBus(probe_card pfProbe, remove_card pfRemove);
    int ret;

    ret = ((glRegisterBus(wlanProbe,
            wlanRemove) == WLAN_STATUS_SUCCESS) ? 0 : -1);

	return ret;
}


/* ===================== WIFI SDIO type: End ===================== */

#endif
