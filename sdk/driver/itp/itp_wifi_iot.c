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

/* === Function Declaration === */
err_t
mt7682_netif_init1 (struct netif * netif);

/* === Global Variable === */
static struct dhcp      wifiNetifDhcp;
static struct timeval   wifiLastTime;

static int              WifiCurrentState = 0, WifiAddNetIf = 0, wifiMscnt, sleep_notify = 2;
static bool             wifiInited = false, wifiDhcp = false, linkUp = false;
static ip_addr_t        wifiIpaddr, wifiNetmask, wifiGw;

/* === Macro === */
#define STASTATE    1
#define APSTATE     2

/* ========================== MT7682 NETIF INIT ========================== */
/* ========================= WIFI SDIO type: Start ========================= */

/* === Macro === */
#define STASTATE    1
#define APSTATE     2

/* === Extern Functions === */

/* === Global Variables === */
static struct netif * keepNetif = NULL; // Keep add Netif

// This function initializes all network interfaces
static void
itpWifiIOTLwipInit (
    struct netif * netif)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    (void)printf("[Itp Wifi] MT7682 Netif(0x%X)\n", netif);

    netif_add(netif, &ipaddr, &netmask, &gw, NULL, mt7682_netif_init1, tcpip_input);
    netif_set_default(netif);

    dhcp_set_struct(netif, &wifiNetifDhcp);
}

static int
WifiRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    return 0;
}

static void
WifiStartDHCP (
    void * setting)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);

    (void)printf("dhcp_start\n");

    netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);

    dhcp_start(keepNetif);

    wifiDhcp    = true;
    gettimeofday(&wifiLastTime, NULL);
    wifiMscnt   = 0;
}

static void
WifiReset (
    ITPEthernetSetting * setting)
{
    netif_set_down(keepNetif);

    IP4_ADDR(&wifiIpaddr,  setting->ipaddr[0],  setting->ipaddr[1],  setting->ipaddr[2],  setting->ipaddr[3]);
    IP4_ADDR(&wifiNetmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
    IP4_ADDR(&wifiGw,      setting->gw[0],      setting->gw[1],      setting->gw[2],      setting->gw[3]);

    netif_set_addr(keepNetif, &wifiIpaddr, &wifiNetmask, &wifiGw);
    netif_set_up(keepNetif);
}

/* No Use for NGPL SDIO WIFI */
void
WifiConnect (
    int type)
{
    // Useless, for compiler
}

void
WifiDisconnect (
    int type)
{
    // Useless, for compiler
}

static void
WifiGetInfo (
    ITPWifiInfo * info)
{
    static struct net_device_info   netinfo = {0};
    unsigned char                   * mac;
    memset(info, 0, sizeof(ITPWifiInfo));

    netinfo.infoType = WLAN_INFO_AP;
    strlcpy(info->displayName, "wlan0", sizeof (info->displayName));

    if (keepNetif)
    {
    }

    info->hardwareAddress[0]    = mac[0];
    info->hardwareAddress[1]    = mac[1];
    info->hardwareAddress[2]    = mac[2];
    info->hardwareAddress[3]    = mac[3];
    info->hardwareAddress[4]    = mac[4];
    info->hardwareAddress[5]    = mac[5];

    info->rfQualityQuant        = netinfo.avgQuant;
    info->rfQualityRSSI         = netinfo.avgRSSI;
    strlcpy(info->name, "wlan0", sizeof (info->name));

    (void)printf("%s\n", __func__);
}

/*************************/
static int
WifiIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    switch (request)
    {
        case ITP_IOCTL_IS_AVAIL:
            break;

        case ITP_IOCTL_IS_CONNECTED:
            break;

        case ITP_IOCTL_WIFI_START_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                WifiStartDHCP((void *)ptr);
            }
            break;

        case ITP_IOCTL_WIFI_STOP_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                dhcp_release(keepNetif);
            }
            dhcp_stop(keepNetif);
            break;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            keepNetif = ptr;
            itpWifiIOTLwipInit(ptr);
            break;

        case ITP_IOCTL_INIT:
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_INIT \n");
            mmpMT7682WifiDriverOn();
            break;

        case ITP_IOCTL_EXIT:
            break;

        case ITP_IOCTL_RESET:
            WifiReset((void *)ptr);
            break;

        case ITP_IOCTL_SLEEP:
            break;

        case ITP_IOCTL_GET_INFO:
            WifiGetInfo((ITPWifiInfo *)ptr);
            break;

        case ITP_IOCTL_WIFIAP_ENABLE:
            break;

        case ITP_IOCTL_WIFIAP_DISABLE:
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_WIFIAP_DISABLE is useless\n");
            break;

        case ITP_IOCTL_ENABLE:
            WifiCurrentState = STASTATE;
            break;

        case ITP_IOCTL_DISABLE:
        {
            ip_addr_t ipaddr, netmask, gw;

            ip_addr_set_zero(&gw);
            ip_addr_set_zero(&ipaddr);
            ip_addr_set_zero(&netmask);

            netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);
            netif_set_down(keepNetif);

            (void)printf("[ITP_WIFI][SDIO Type] Set to STA Mode\n");
            wifiInited          = false;
            linkUp              = false;
            WifiCurrentState    = 0;
        }
        break;

        case ITP_IOCTL_WIFI_NETIF_SHUTDOWN:
            netif_set_link_down(keepNetif); // close MAC & DMA
#if defined(CFG_NET_WIFI) && defined(CFG_NET_ETHERNET)
            netif_remove(keepNetif);
#else
            netif_remove_all();             // 1. call netif_set_down   2.wifiif_shutdown by callback
#endif
            break;

        default:
            errno = (ITP_DEVICE_WIFI << ITP_DEVICE_ERRNO_BIT) | 1;
            (void)printf("[ITP_WIFI][SDIO Type] ERROR : ITP_IOCTL (%d)\n", request);
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceWifi =
{
    ":SDIO_WIFI_MT7682",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    WifiIoctl,
    NULL
};

/* ================================= */
