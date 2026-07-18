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
#include "ite/ite_usbd_ecm.h"
#ifdef CFG_NET_LWIP_2
#include "netif/bridgeif.h" //eason
#ifdef CFG_NET_LWIP_PPPOE
#include "netif/ppp/pppoe.h"
#endif
#endif


extern err_t usbd_ecmif_init(struct netif *netif);
extern void  usbd_ecmif_shutdown(struct netif *netif);
extern void  usbd_ecmif_poll(struct netif *netif);

static struct netif   ethNetifs[1];
static struct dhcp    ethNetifDhcp;

static bool           ethInited, ethDhcp;
static struct timeval ethLastTime;
static int            ethMscnt;
static timer_t        ethTimer;
static volatile bool  ethInPollingFunc;

static volatile struct netif *usbd_ecm_netif = NULL;

// This function initializes all network interfaces
static void itpUsbdEcmLwipInit(void)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    (void)memset((void*)ethNetifs, 0, sizeof(ethNetifs));
    xnetif = &ethNetifs[0];

    netif_set_default(netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, &usbd_ecmif_init, &tcpip_input));
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, &usbd_ecmif_shutdown);
#endif
    netif_set_up(xnetif);

    usbd_ecm_netif = xnetif;
}

static void UsbdEcmGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (usbd_ecm_netif == NULL)
    {
        ITP_LOG_ERR("[%s]usbd_ecm is not init yet\n", __func__ );
        return;
    }

    if (info->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of usbd_ecm: %d\n", __func__, info->index );
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
    {
        info->flags |= ITP_ETH_LINKUP;
    }

    if (xnetif->ip_addr.addr)
    {
        info->flags |= ITP_ETH_ACTIVE;
    }

    info->address = xnetif->ip_addr.addr;
    info->netmask = xnetif->netmask.addr;
    (void)sprintf(info->displayName, "usbdEcm%d", info->index);
    (void)sprintf(info->name, "usbd_ecm%d", info->index);
}

static void UsbdEcmPoll(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (usbd_ecm_netif == NULL)
    {
        return;
    }

    // dhcp not verified, these code are from itp_ethernet.c
    if ((xnetif->ip_addr.addr == 0)
        && (ethDhcp
            ))
    {
        struct timeval currTime;

        (void)gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&ethLastTime, &currTime) >= DHCP_FINE_TIMER_MSECS)
        {
            if (ethDhcp)
            {
                dhcp_fine_tmr();
            }

            ethLastTime = currTime;
            ethMscnt   += DHCP_FINE_TIMER_MSECS;
        }
        else if (ethMscnt >= DHCP_COARSE_TIMER_MSECS)
        {
            if (ethDhcp)
            {
                dhcp_coarse_tmr();
            }

            ethMscnt = 0;
        }
        else
        {
            /* do nothing */
        }
    }

    // check for link status
    usbd_ecmif_poll(xnetif);

    return;
}

#define CFG_NET_ECM_POLL_INTERVAL  50

#if CFG_NET_ECM_POLL_INTERVAL > 0

static void UsbdEcmPollHandler(timer_t timerid, int arg)
{
    ethInPollingFunc = true;
    UsbdEcmPoll();
    ethInPollingFunc = false;
}

#endif // CFG_NET_ECM_POLL_INTERVAL > 0

static int UsbdEcmInit(void)
{
	int rc = 0;

#if CFG_NET_ECM_POLL_INTERVAL > 0
    {
        struct itimerspec value;
        (void)timer_create(CLOCK_REALTIME, NULL, &ethTimer);
        (void)timer_connect(ethTimer, (VOIDFUNCPTR)&UsbdEcmPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec  = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_ECM_POLL_INTERVAL * 1000000;
        (void)timer_settime(ethTimer, 0, &value, NULL);
    }
#endif // CFG_NET_ECM_POLL_INTERVAL > 0

    #if defined(CFG_USBD_ECM)
    rc = iteUsbdEcmRegister();
    #endif

    ethInited = true;

    return rc;
}

static void UsbdEcmExit(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (ethInited == false)
    {
        return;
    }

    while (ethInPollingFunc)
    {
        (void)usleep(1000);
    }

    (void)timer_delete(ethTimer);

    if (usbd_ecm_netif != NULL) {
        netif_set_down(xnetif);
        netif_remove(xnetif);
        usbd_ecm_netif = NULL;
    }

    #if defined(CFG_USBD_ECM)
    (void)iteUsbdEcmUnRegister();
    #endif

    ethInited = false;

    return;
}

static void UsbdEcmReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    if (usbd_ecm_netif == NULL)
    {
        ITP_LOG_ERR("[%s]usbd_ecm is not init yet\n", __func__ );
        return;
    }

    if (setting->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of usbd_ecm: %d\n", __func__, setting->index );
        return;
    }

    xnetif = &ethNetifs[setting->index];

    xnetif->ip_addr.addr = 0;

    if (setting->dhcp || setting->autoip)
    {
        if ((setting->dhcp == 0) && ethDhcp)
        {
            dhcp_stop(xnetif);
            ethDhcp = false;
        }

        if (setting->dhcp > 0)
        {
            dhcp_set_struct(xnetif, &ethNetifDhcp);
            (void)dhcp_start(xnetif);
            ethDhcp = true;
        }

        (void)gettimeofday(&ethLastTime, NULL);
        ethMscnt = 0;
        return;
    }
    else
    {
        if (ethDhcp)
        {
            dhcp_stop(xnetif);
        }

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

static int UsbdEcmIoctl(int file, unsigned long request, void *ptr, void *info)
{
    int rc = 0;

    switch (request)
    {
    case ITP_IOCTL_POLL:
        //UsbdEcmPoll();
        break;

    case ITP_IOCTL_IS_AVAIL:
        if (usbd_ecm_netif != NULL)
        {
            rc = (ethNetifs[0].ip_addr.addr > 0u) ? 1 : 0;
        }
        else
        {
            rc = 0;
        }
        break;

    case ITP_IOCTL_IS_CONNECTED:
        if (ptr != NULL)
        {
            #if defined(CFG_USBD_ECM)
            rc = iteUsbdEcmGetLink();
            #endif
        }
        else 
        {
            if (usbd_ecm_netif != NULL)
            {
                rc = netif_is_link_up(usbd_ecm_netif);
            }
            else
            {
                rc = 0;
            }
        }
        break;

    case ITP_IOCTL_GET_INFO:
        UsbdEcmGetInfo((ITPEthernetInfo *)ptr);
        break;

    case ITP_IOCTL_INIT:
        rc = UsbdEcmInit();
        break;

    case ITP_IOCTL_EXIT:
        UsbdEcmExit();
        break;

    case ITP_IOCTL_RESET:
        UsbdEcmReset((ITPEthernetSetting *)ptr);
        break;

    case ITP_IOCTL_RESET_DEFAULT:
        if (usbd_ecm_netif == NULL)
        {
            rc = -1;
        }
        if (!netif_is_def(&ethNetifs[0])) 
        {
            (void)printf("\n ==== itp_usbd_ecm set netif 0 default ====\n");
            netif_set_default(&ethNetifs[0]);
        }
        break;

#if 0
    case ITP_IOCTL_ENABLE:
        if (!usbd_ecm_netif)
            return -1;
        if (!netif_is_link_up(&ethNetifs[0]))
            netif_set_link_up(&ethNetifs[0]);
        break;

    case ITP_IOCTL_DISABLE:
        if (!usbd_ecm_netif)
            return -1;
        if (netif_is_link_up(&ethNetifs[0]))
            netif_set_link_down(&ethNetifs[0]);
        break;

	case ITP_IOCTL_SLEEP:
#endif
    case ITP_IOCTL_NETIF_REMOVE:
        if (usbd_ecm_netif != NULL) 
        {
            while (ethInPollingFunc)
            {
                (void)usleep(1000);
            }

            if (ethDhcp) 
            {
                dhcp_stop(&ethNetifs[0]);
                ethDhcp = false;
            }
            netif_set_down(&ethNetifs[0]);
            netif_remove(&ethNetifs[0]);
            usbd_ecm_netif = NULL;
            (void)printf("[%s] usbd_ecm netif remove!! \n", __func__);
        }
		break;
#if 0
	case ITP_IOCTL_RESUME:
#endif
    case ITP_IOCTL_NETIF_ADD:
        if (usbd_ecm_netif == NULL) 
        {
            itpUsbdEcmLwipInit();
            (void)printf("[%s] usbd_ecm netif add!! \n", __func__);
        }
		break;

    case ITP_IOCTL_RENEW_DHCP:
        if (usbd_ecm_netif == NULL)
        {
            rc = -1;
        }
        dhcp_network_changed(&ethNetifs[0]);
        break;

    default:
        errno = (ITP_DEVICE_USBDECM << ITP_DEVICE_ERRNO_BIT) | 1;
        rc = -1;
        break;
    }

    return rc;
}

const ITPDevice itpDeviceUsbdEcm =
{
    ":usbdecm",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdEcmIoctl,
    NULL
};
