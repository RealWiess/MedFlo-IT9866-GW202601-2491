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
#include "ite/ite_usbex.h"

/* === Function Declaration === */
err_t
wifiif_init (struct netif * netif);
void
wifiif_shutdown (struct netif * netif);
void
wifiif_poll (struct netif * netif);
void
wifiif_ctrl (struct netif                                       * netif,
                                       struct net_device_config * cfg);
void
wifiif_info (struct netif                                       * netif,
                                       struct net_device_info   * info);

/* === Global Variable === */
static struct dhcp      wifiNetifDhcp;
static struct timeval   wifiLastTime;

static int              WifiCurrentState = 0, WifiAddNetIf = 0, wifiMscnt, sleep_notify = 2;
static bool             wifiInited = false, wifiDhcp = false, linkUp = false;
static ip_addr_t        wifiIpaddr, wifiNetmask, wifiGw;

/* === Macro === */
#define STASTATE    1
#define APSTATE     2

typedef struct
{
    uint8_t octet[6]; /* Unique 6-byte MAC address */
} mhd_mac_t;

extern int
mhd_sta_get_mac_address (mhd_mac_t * mac);
extern uint32_t
mhd_sta_ipv4_netmask (void);
extern uint32_t
mhd_sta_ipv4_ipaddr (void);
extern void
wifi_on (void);

// keep add netif
static struct netif * keepNetif;

#ifndef WLAN0_NAME
    #define WLAN0_NAME          "wlan0"
#endif

// test example
#define CFG_NET_WIFI_IPADDR     "192.168.1.1"
#define CFG_NET_WIFI_NETMASK    "255.255.255.0"
#define CFG_NET_WIFI_GATEWAY    "192.168.1.1"

// This function initializes all network interfaces
static void
itpWifiNgplLwipInit (
    struct netif * netif)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    (void)printf("[Itp Wifi] itpWifiNgplLwipInit netif:0x%X\n", netif);

    netif_add(netif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input);
    netif_set_default(netif);
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(netif, wifiif_shutdown);
#endif
    dhcp_set_struct(netif, &wifiNetifDhcp);
}

static int
WifiRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    struct net_device_info * netinfo = (struct net_device_info *)ptr;
    netinfo->infoType = WLAN_INFO_SCAN_GET;
    wifiif_info(keepNetif, netinfo);
    return 0;
}

static void
WifiSleepNotify (
    int idx)
{
    sleep_notify = idx; // [initial:2], [sleep -> wakeup:1] , [wakeup -> sleep:0]
    if (sleep_notify == sleep_to_wakeup)
    {
        wifiInited = true;
    }
    (void)printf("###sleep notify: %d\n", sleep_notify);
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

    IP4_ADDR(&wifiIpaddr, setting->ipaddr[0], setting->ipaddr[1], setting->ipaddr[2], setting->ipaddr[3]);
    IP4_ADDR(&wifiNetmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
    IP4_ADDR(&wifiGw, setting->gw[0], setting->gw[1], setting->gw[2], setting->gw[3]);

    netif_set_addr(keepNetif, &wifiIpaddr, &wifiNetmask, &wifiGw);
    netif_set_up(keepNetif);
}

/* No Use for NGPL SDIO WIFI */
void
WifiConnect (
    int type)
{
    // no use, for compiler
}

void
WifiDisconnect (
    int type)
{
    // no use, for compiler
}

static void
WifiGetInfo (
    ITPWifiInfo * info)
{
    static struct net_device_info   netinfo = {0};
    char                            ip[16]  = {0};

    mhd_mac_t                       mac;
    mhd_sta_get_mac_address(&mac);
    memset(info, 0, sizeof(ITPWifiInfo));

    netinfo.infoType = WLAN_INFO_AP;
    strlcpy(info->displayName, "wlan0", sizeof (info->displayName));

    if (1)
    {
        if (WifiCurrentState == STASTATE)
        {
            info->netmask = mhd_sta_ipv4_netmask();
        }
        else
        {
            info->netmask = 0x00ffffff;
        }

        if (WifiCurrentState == STASTATE)
        {
            info->address = mhd_sta_ipv4_ipaddr();
        }
        else
        {
            info->address = 0x0101a8c0;      // #define CFG_NET_WIFI_IPADDR     "192.168.1.1"
        }
        info->active = info->address? 1 : 0;
        // (void)printf("WifiGetInfo active %d \n",info->active);
#if 0
        char    ip[16]      = {0};
        char    mask[16]    = {0};
        ipaddr_ntoa_r((const ip_addr_t *)ip_t, ip, sizeof(ip));
        ipaddr_ntoa_r((const ip_addr_t *)ip_mask, mask, sizeof(mask));

        (void)printf("info = %s %s \n", ip, mask);
#endif
        // ipaddr_ntoa_r((const ip_addr_t*) info->address, ip, sizeof(ip));
    }

    info->hardwareAddress[0]    = (unsigned char)mac.octet[0];
    info->hardwareAddress[1]    = (unsigned char)mac.octet[1];
    info->hardwareAddress[2]    = (unsigned char)mac.octet[2];
    info->hardwareAddress[3]    = (unsigned char)mac.octet[3];
    info->hardwareAddress[4]    = (unsigned char)mac.octet[4];
    info->hardwareAddress[5]    = (unsigned char)mac.octet[5];

    (void)printf("WifiMgr_Get_MAC_Address %0x:%0x:%0x:%0x:%0x:%0x   \n", info->hardwareAddress[0], info->hardwareAddress[1], info->hardwareAddress[2], info->hardwareAddress[3], info->hardwareAddress[4], info->hardwareAddress[5]);
    // (void)printf("ip %s \n",ip);

    info->rfQualityQuant    = netinfo.avgQuant;
    info->rfQualityRSSI     = netinfo.avgRSSI;
    strlcpy(info->name, "wlan0", sizeof (info->name));
    (void)printf("%s\n", __func__);
}

/*************************/
extern int
wifi_off (void);

static int
WifiIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    char essid[33];

    switch (request)
    {
        case ITP_IOCTL_WIFI_START_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                WifiStartDHCP((void *)ptr);
            }
            break;

        case ITP_IOCTL_WIFI_STOP_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                dhcp_stop(keepNetif);
            }
            break;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            keepNetif = ptr;
            itpWifiNgplLwipInit(ptr);
            break;

        case ITP_IOCTL_INIT:
            (void)printf("WifiIoctl ITP_IOCTL_INIT AP62xx\n");
            // sleep to wake up
            wifi_on();

            break;

        case ITP_IOCTL_RESUME:
        {
            int rc, mode;
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_RESUME \n");
            mode = (int)ptr;

            if (mode == 0)
            {
                WifiCurrentState = STASTATE;
            }
            else if (mode == 1)
            {
                WifiCurrentState = APSTATE;
            }

            if (rc == 0)
            {
                wifiInited = true;
            }
        }
        break;

        case ITP_IOCTL_EXIT:
            (void)printf("WifiIoctl ITP_IOCTL_EXIT \n");

            wifi_off();
            sleep(1);
            break;

        case ITP_IOCTL_RESET:
            WifiReset((void *)ptr);
            break;

        case ITP_IOCTL_SLEEP:
            WifiSleepNotify((int)ptr);
            break;

        case ITP_IOCTL_WIFI_SLEEP_STATUS:
            return sleep_notify;

        case ITP_IOCTL_GET_INFO:
        {
            WifiGetInfo((ITPWifiInfo *)ptr);
        }
        break;

        case ITP_IOCTL_IS_AVAIL:

#if 0
            if (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0)
            {
                // (void)printf("\n\rWIFI disconnected");
                return 0;
            }
            else
            {
            }
#endif
            break;

        case ITP_IOCTL_RESET_DEFAULT:
            (void)printf("itp wifi set netif  default \n");
            netif_set_default(keepNetif);
            break;

        case ITP_IOCTL_WIFI_MODE:
            return STASTATE;

        case ITP_IOCTL_RENEW_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                dhcp_network_changed(keepNetif);
            }
            break;

        case ITP_IOCTL_ENABLE:
            WifiCurrentState = STASTATE;
            break;

        case ITP_IOCTL_WIFIAP_ENABLE:
        {
            WifiCurrentState = APSTATE;
            (void)printf(" - %s  ITP_IOCTL_WIFIAP_ENABLE \n", __FUNCTION__);
        }
        break;

        case ITP_IOCTL_WIFIAP_DISABLE:
        {
            WifiCurrentState = STASTATE;
            (void)printf(" - %s  ITP_IOCTL_WIFIAP_DISABLE \n", __FUNCTION__);
        }
        break;

        case ITP_IOCTL_DISABLE:
        {
            ip_addr_t ipaddr, netmask, gw;

            ip_addr_set_zero(&gw);
            ip_addr_set_zero(&ipaddr);
            ip_addr_set_zero(&netmask);

            netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);
            netif_set_down(keepNetif);
            (void)printf("[itp wifi] ITP_IOCTL_DISABLE \n");
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

/* =================================================== */
