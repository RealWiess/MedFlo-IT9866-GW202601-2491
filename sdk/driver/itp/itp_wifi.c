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
wifiif_init (struct netif * netif);
void
wifiif_shutdown (struct netif * netif);
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

#if defined (CFG_NET_WIFI_SDIO_VND_RTK)
/* ================== RTK 8189FTV/8723DS/8821CS NETIF INIT ================= */
/* ======================== WIFI SDIO type: Start ======================== */
    #if defined (CFG_NET_WIFI_SDIO_NGPL_8189FTV)
        #include <../wifi_rtk/non_gpl_wifi_8189f/include/autoconf.h>
        #include <../wifi_rtk/non_gpl_wifi_8189f/include/wifi_constants.h>
    #elif defined (CFG_NET_WIFI_SDIO_NGPL_8723DS)
        #include <../wifi_rtk/non_gpl_wifi_8723d/include/autoconf.h>
        #include <../wifi_rtk/non_gpl_wifi_8723d/include/wifi_constants.h>
    #elif defined (CFG_NET_WIFI_SDIO_NGPL_8821CS)
        #include <../wifi_rtk/non_gpl_wifi_8821c/include/autoconf.h>
        #include <../wifi_rtk/non_gpl_wifi_8821c/include/wifi_constants.h>
    #elif defined (CFG_NET_WIFI_SDIO_NGPL_8733BS)
        #include <../wifi_rtk/non_gpl_wifi_8733b/include/autoconf.h>
        #include <../wifi_rtk/non_gpl_wifi_8733b/include/wifi_constants.h>
    #endif

/* === Macro === */
    #define CFG_NET_WIFI_IPADDR     "192.168.1.1"   // test example
    #define CFG_NET_WIFI_NETMASK    "255.255.255.0"
    #define CFG_NET_WIFI_GATEWAY    "192.168.1.1"

/* === Extern Functions === */
extern struct netif xnetif[NET_IF_NUM];             // extern netif from wifi_conf.c
extern uint8_t *
LwIP_GetIP (struct netif * pnetif);
extern uint8_t *
LwIP_GetMASK (struct netif * pnetif);
extern uint8_t *
LwIP_GetGW (struct netif * pnetif);
extern uint8_t *
LwIP_GetMAC (struct netif * pnetif);
extern int
wifi_on (rtw_mode_t mode);
extern int
wifi_off (void);
extern int
wext_get_ssid (const char                                       * ifname,
                                                  unsigned char * ssid);

#if defined (CFG_NET_WIFI_SDIO_NGPL_8821CS) 
extern int mmpRelWifiDiverCmdMode(int mode);
#endif


/* === Global Variables === */
static struct netif * keepNetif     = NULL; // Keep add Netif
static struct netif * keepNetif_ap  = NULL; // Keep add Netif

    #ifndef WLAN0_NAME
        #define WLAN0_NAME "wlan0"
    #endif

// This function initializes all network interfaces
static void
itpWifiNgplLwipInit (
    struct netif * netif)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    (void)printf("[Itp Wifi] itpWifiNgplLwipInit netif(%p)\n", netif);

    #if defined (CFG_NET_PRIO_MULTI_MAC)
    uint8_t prio[] = {CFG_NET_PRIO_MULTI_MAC};

    netif_add(netif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input);

    if ((prio[0] > 0) && (prio[2] > 0))
    {
        /* Case 1: 4G + WIFI + Ethernet */
        if ((prio[1] < prio[0]) && (prio[0] < prio[2]))
        {
            (void)printf("Set WIFI netif as default in Multi-MAC case [G+W+E]\n");
            netif_set_default(netif);
        }
        else if (prio[0] > 0)
        {
            /* Case 2: 4G(Use Eth middleware) + WIFI */
            if (prio[1] < prio[0])
            {
                (void)printf("Set WIFI netif as default in Multi-MAC case [G+W]\n");
                netif_set_default(netif);
            }
        }
        else if (prio[2] > 0)
        {
            /* Case 3: Eth + WIFI */
            if (prio[1] < prio[2])
            {
                (void)printf("Set WIFI netif as default in Multi-MAC case [E+W]\n");
                netif_set_default(netif);
            }
        }
    }
    #else
    /* Case 4: Only WIFI */
    netif_set_default(netif_add(netif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input));
    #endif
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
    if (wifiDhcp == false)
    {
        netif_set_up(keepNetif);
    }
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
    unsigned char                   * mac;
    memset(info, 0, sizeof(ITPWifiInfo));

    netinfo.infoType = WLAN_INFO_AP;
    strlcpy(info->displayName, "wlan0", sizeof (info->displayName));

    if (keepNetif)
    {
        mac = LwIP_GetMAC(keepNetif);

        ip_addr_t * ip_t = (ip_addr_t *) LwIP_GetIP(keepNetif);
        info->address = ip_t->addr;

        ip_addr_t * mask_t = (ip_addr_t *) LwIP_GetMASK(keepNetif);
        info->netmask = mask_t->addr;

        ip_addr_t * gw_t = (ip_addr_t *) LwIP_GetGW(keepNetif);
        info->gateway   = gw_t->addr;

        info->active    = info->address? 1 : 0;
    #if 0
        char    ip[16]      = {0};
        char    mask[16]    = {0};
        ipaddr_ntoa_r((const ip_addr_t *)ip_t, ip, sizeof(ip));
        ipaddr_ntoa_r((const ip_addr_t *)ip_mask, mask, sizeof(mask));

        (void)printf("info = %s %s \n", ip, mask);
    #endif
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
            if (WifiCurrentState == STASTATE)
            {
                unsigned char xnetif_set_up, xnetif_set_link_up;

                xnetif_set_up       = netif_is_up(keepNetif);       // SW stack ready (Get DHCP IP)
                xnetif_set_link_up  = netif_is_link_up(keepNetif);  // HW stack ready (Link layer OK)
                // (void)printf("\n\r[ITP_WIFI][SDIO Type] NETIF(0x%X) FLAG UP(%d)/FLAG LINK UP(%d).\n", keepNetif, xnetif_set_up, xnetif_set_link_up);

                return xnetif_set_up && xnetif_set_link_up;
            }
            else
            {
                return netif_is_up(keepNetif);
            }
            break;

        case ITP_IOCTL_IS_CONNECTED:
            if (WifiCurrentState == STASTATE)
            {
                int     condition[3];
                char    essid[33];

                /* There are 3 conditions for Connected */
                condition[0]    = (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) > 0)? 1 : 0;  /* SSID len > 0 */
                condition[1]    = netif_is_up(keepNetif);                                           /* SW stack ready (DHCP OK) */
                condition[2]    = keepNetif->ip_addr.addr? 1 : 0;                                   /* Already hook IP into netif */

                /*(void)printf("[ITP_WIFI] Connected cond: [1] SSID len %s 0, [2] DHCP is %s, [3] Netif %s IP\n",
                    condition[0] ? ">":"=",
                    condition[1] ? "OK":"not OK",
                    condition[2] ? "have":"have no");*/

                /* Check SSID was set */
                if (condition[0])
                {
                    return condition[1] && condition[2];
                }
                else
                {
                    // (void)printf("\n\r[ITP_WIFI][SDIO Type] WIFI disconnected and have NO SSID.\n");
                    return -1;
                }
            }
            else if (WifiCurrentState == APSTATE)
            {
                return netif_is_link_up(keepNetif);
            }
            else
            {
                return -1;
            }
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
                dhcp_stop(keepNetif);
            }
            break;

        case ITP_IOCTL_RENEW_DHCP:
            if (WifiCurrentState == STASTATE)
            {
                dhcp_network_changed(keepNetif);
            }
            break;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            if (keepNetif == NULL)
            {
                keepNetif = ptr;
                itpWifiNgplLwipInit(ptr);
            }
            else
            {
                keepNetif_ap = ptr;
                itpWifiNgplLwipInit(ptr);
            }

            break;

        case ITP_IOCTL_INIT:
        {
            int rc , mode;            
            mode = (rtw_mode_t)ptr;
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_INIT mode %d \n",mode);
#if defined (CFG_NET_WIFI_SDIO_NGPL_8821CS) 
            rc = mmpRelWifiDiverCmdMode(mode);
#endif
            rc = mmpRtlWifiDriverCmdTest();     // Init success = 0

            if (rc == 0)
            {
                wifiInited = true;
            }
            else if (rc == RTW_NODEVICE)
            {
                wifiInited = false;
            }
        }
        break;

        case ITP_IOCTL_IS_DEVICE_READY:
            return wifiInited;

        case ITP_IOCTL_RESUME:
        {
            int rc, mode;
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_RESUME \n");
            mode    = (rtw_mode_t)ptr;
            rc      = wifi_on(mode);

            if (mode == 1)
            {
                WifiCurrentState = STASTATE;
            }
            else if (mode == 2)
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
            wifi_off();
            break;

        case ITP_IOCTL_RESET:
            WifiReset((void *)ptr);
            break;

        case ITP_IOCTL_RESET_DEFAULT:
            if (!netif_is_def(keepNetif))
            {
                (void)printf("\n ==== itp wifi set netif[%p] default ====\n", keepNetif);
                netif_set_default(keepNetif);
            }
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

        case ITP_IOCTL_WIFIAP_ENABLE:
        {
            ip_addr_t ipaddr, netmask, gw;
    #if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI)
            // itpWifiLwipInitNetif();
    #endif
            ipaddr_aton(CFG_NET_WIFI_IPADDR, &ipaddr);
            ipaddr_aton(CFG_NET_WIFI_NETMASK, &netmask);
            ipaddr_aton(CFG_NET_WIFI_GATEWAY, &gw);

            if (ptr)
            {
                keepNetif_ap = ptr;
            }

            if (keepNetif_ap)
            {
                (void)printf("[ITP_WIFI][SDIO Type] netif(%p)\n", keepNetif_ap);
                netif_set_addr(keepNetif_ap, &ipaddr, &netmask, &gw);
                netif_set_up(keepNetif_ap);
                dhcp_start(keepNetif_ap);

                netif_set_default(keepNetif_ap);
            }
            else
            {
                (void)printf("[ITP_WIFI][SDIO Type] Error of netif is NULL \n");
            }

            WifiCurrentState = APSTATE;
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_WIFIAP_ENABLE \n");
        }
        break;

        case ITP_IOCTL_WIFIAP_DISABLE:
            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_WIFIAP_DISABLE is useless for 8189F SDIO WIFI, ignore its.\n");
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

            (void)printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_DISABLE(STA Mode) \n");
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
    ":wifi",
    itpOpenDefault,
    itpCloseDefault,
    WifiRead,
    itpWriteDefault,
    itpLseekDefault,
    WifiIoctl,
    NULL
};

/* ===================== WIFI SDIO type: End ===================== */

#endif
