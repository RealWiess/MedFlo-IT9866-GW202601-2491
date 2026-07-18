/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 *
 * @author Irene Lin
 * @version 1.0
 */
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "itp_cfg.h"
#include "ite/itp.h"
#include "ite/ite_usbd_ncm.h"
#ifdef CFG_NET_LWIP_2
#include "netif/bridgeif.h" //eason
#ifdef CFG_NET_LWIP_PPPOE
#include "netif/ppp/pppoe.h"
#endif
#endif


extern err_t usbd_ncmif_init(struct netif *netif);
extern void  usbd_ncmif_shutdown(struct netif *netif);
extern void  usbd_ncmif_poll(struct netif *netif);

static struct netif   ethNetifs[1];
static struct dhcp    ethNetifDhcp;

static bool           ethInited, ethDhcp;
static struct timeval ethLastTime;
static int            ethMscnt;
static timer_t        ethTimer;
static volatile bool  ethInPollingFunc;

volatile static struct netif *usbd_ncm_netif = NULL;

// This function initializes all network interfaces
static void itpUsbdNcmLwipInit(void)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    memset((void*)ethNetifs, 0, sizeof(ethNetifs));
    xnetif = &ethNetifs[0];

    netif_set_default(netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, usbd_ncmif_init, tcpip_input));
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, usbd_ncmif_shutdown);
#endif
    netif_set_up(xnetif);

    usbd_ncm_netif = xnetif;
}

static void UsbdNcmGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (!usbd_ncm_netif)
    {
        ITP_LOG_ERR("[%s]usbd_ncm is not init yet\n", __func__ );
        return;
    }

    if (info->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of usbd_ncm: %d\n", __func__, info->index );
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
        info->flags |= ITP_ETH_LINKUP;

    if (xnetif->ip_addr.addr)
        info->flags |= ITP_ETH_ACTIVE;

    info->address = xnetif->ip_addr.addr;
    info->netmask = xnetif->netmask.addr;
    sprintf(info->displayName, "usbdNcm%d", info->index);
    sprintf(info->name, "usbd_ncm%d", info->index);
}

static void UsbdNcmPoll(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (!usbd_ncm_netif)
        return;

    // dhcp not verified, these code are from itp_ethernet.c
    if ((xnetif->ip_addr.addr == 0)
        && (ethDhcp
            ))
    {
        struct timeval currTime;

        gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&ethLastTime, &currTime) >= DHCP_FINE_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp_fine_tmr();

            ethLastTime = currTime;
            ethMscnt   += DHCP_FINE_TIMER_MSECS;
        }
        else if (ethMscnt >= DHCP_COARSE_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp_coarse_tmr();

            ethMscnt = 0;
        }
    }

    // check for link status
    usbd_ncmif_poll(xnetif);

    return;
}

#define CFG_NET_NCM_POLL_INTERVAL  50

#if CFG_NET_NCM_POLL_INTERVAL > 0

static void UsbdNcmPollHandler(timer_t timerid, int arg)
{
    ethInPollingFunc = true;
    UsbdNcmPoll();
    ethInPollingFunc = false;
}

#endif // CFG_NET_NCM_POLL_INTERVAL > 0

static int UsbdNcmInit(void)
{
	int rc = 0;

#if CFG_NET_NCM_POLL_INTERVAL > 0
    {
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &ethTimer);
        timer_connect(ethTimer, (VOIDFUNCPTR)UsbdNcmPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec  = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_NCM_POLL_INTERVAL * 1000000;
        timer_settime(ethTimer, 0, &value, NULL);
    }
#endif // CFG_NET_NCM_POLL_INTERVAL > 0

    #if defined(CFG_USBD_NCM)
    rc = iteUsbdNcmRegister();
    #endif

    ethInited = true;

    return rc;
}

static void UsbdNcmExit(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (ethInited == false)
        return;

    while (ethInPollingFunc)
        usleep(1000);

    timer_delete(ethTimer);

    if (usbd_ncm_netif) {
        netif_set_down(xnetif);
        netif_remove(xnetif);
        usbd_ncm_netif = NULL;
    }

    #if defined(CFG_USBD_NCM)
    (void)iteUsbdNcmUnRegister();
    #endif

    ethInited = false;

    return;
}

static void UsbdNcmReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    if (!usbd_ncm_netif)
    {
        ITP_LOG_ERR("[%s]usbd_ncm is not init yet\n", __func__ );
        return;
    }

    if (setting->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of usbd_ncm: %d\n", __func__, setting->index );
        return;
    }

    xnetif = &ethNetifs[setting->index];

    xnetif->ip_addr.addr = 0;

    if (setting->dhcp || setting->autoip)
    {
        if (!setting->dhcp && ethDhcp)
        {
            dhcp_stop(xnetif);
            ethDhcp = false;
        }

        if (setting->dhcp)
        {
            dhcp_set_struct(xnetif, &ethNetifDhcp);
            dhcp_start(xnetif);
            ethDhcp = true;
        }

        gettimeofday(&ethLastTime, NULL);
        ethMscnt = 0;
        return;
    }
    else
    {
        if (ethDhcp)
            dhcp_stop(xnetif);

        ethDhcp = false;
    }

    // set static IP
    if (!setting->dhcp)
    {
        IP4_ADDR(&ipaddr, setting->ipaddr[0], setting->ipaddr[1], setting->ipaddr[2], setting->ipaddr[3]);
        IP4_ADDR(&netmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
        IP4_ADDR(&gw, setting->gw[0], setting->gw[1], setting->gw[2], setting->gw[3]);

        netif_set_addr(xnetif, &ipaddr, &netmask, &gw);
        netif_set_up(xnetif);
    }
}

static int UsbdNcmIoctl(int file, unsigned long request, void *ptr, void *info)
{
    switch (request)
    {
    case ITP_IOCTL_POLL:
        //UsbdNcmPoll();
        break;

    case ITP_IOCTL_IS_AVAIL:
        if (usbd_ncm_netif)
        {
            return ethNetifs[0].ip_addr.addr ? 1 : 0;
        }
        else
        {
            return 0;
        }

    case ITP_IOCTL_IS_CONNECTED:
        if (ptr)
        {
            #if defined(CFG_USBD_NCM)
            return iteUsbdNcmGetLink();
            #endif
        }
        else 
        {
            if (usbd_ncm_netif)
            {
                return netif_is_link_up(usbd_ncm_netif);
            }
            else
            {
                return 0;
            }
        }
        break;

    case ITP_IOCTL_GET_INFO:
        UsbdNcmGetInfo((ITPEthernetInfo *)ptr);
        break;

    case ITP_IOCTL_INIT:
        return UsbdNcmInit();
        break;

    case ITP_IOCTL_EXIT:
        UsbdNcmExit();
        break;

    case ITP_IOCTL_RESET:
        UsbdNcmReset((ITPEthernetSetting *)ptr);
        break;

    case ITP_IOCTL_RESET_DEFAULT:
        if (!usbd_ncm_netif)
        {
            return -1;
        }
        if (!netif_is_def(&ethNetifs[0])) 
        {
            printf("\n ==== itp_usbd_ncm set netif 0 default ====\n");
            netif_set_default(&ethNetifs[0]);
        }
        break;

#if 0
    case ITP_IOCTL_ENABLE:
        if (!usbd_ncm_netif)
            return -1;
        if (!netif_is_link_up(&ethNetifs[0]))
            netif_set_link_up(&ethNetifs[0]);
        break;

    case ITP_IOCTL_DISABLE:
        if (!usbd_ncm_netif)
            return -1;
        if (netif_is_link_up(&ethNetifs[0]))
            netif_set_link_down(&ethNetifs[0]);
        break;

	case ITP_IOCTL_SLEEP:
#endif
    case ITP_IOCTL_NETIF_REMOVE:
        if (usbd_ncm_netif) 
        {
            while (ethInPollingFunc)
                usleep(1000);

            if (ethDhcp) 
            {
                dhcp_stop(&ethNetifs[0]);
                ethDhcp = false;
            }
            netif_set_down(&ethNetifs[0]);
            netif_remove(&ethNetifs[0]);
            usbd_ncm_netif = NULL;
            printf("[%s] usbd_ncm netif remove!! \n", __func__);
        }
		break;
#if 0
	case ITP_IOCTL_RESUME:
#endif
    case ITP_IOCTL_NETIF_ADD:
        if (!usbd_ncm_netif) 
        {
            itpUsbdNcmLwipInit();
            printf("[%s] usbd_ncm netif add!! \n", __func__);
        }
		break;

    case ITP_IOCTL_RENEW_DHCP:
        if (!usbd_ncm_netif)
        {
            return -1;
        }
        dhcp_network_changed(&ethNetifs[0]);
        break;

    default:
        errno = (ITP_DEVICE_USBDNCM << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbdNcm =
{
    ":usbdncm",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdNcmIoctl,
    NULL
};
