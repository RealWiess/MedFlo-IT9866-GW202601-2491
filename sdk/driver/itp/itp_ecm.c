/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Ethernet functions.
 *
 * @author Jim Tan
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
#ifdef CFG_NET_LWIP_2
#include "netif/bridgeif.h" //eason
#ifdef CFG_NET_LWIP_PPPOE
#include "netif/ppp/pppoe.h"
#endif
#endif


extern err_t ecmif_init(struct netif *netif);
extern void  ecmif_shutdown(struct netif *netif);
extern void  ecmif_poll(struct netif *netif);

static struct netif   ethNetifs[1];
static struct dhcp    ethNetifDhcp;

static bool           ethInited, ethDhcp;
static struct timeval ethLastTime;
static int            ethMscnt;
static timer_t        ethTimer;
static volatile bool  ethInPollingFunc;

volatile static struct netif *ecm_netif = NULL;

// This function initializes all network interfaces
static void itpEcmLwipInit(void)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    memset((void*)ethNetifs, 0, sizeof(ethNetifs));
    xnetif = &ethNetifs[0];

    netif_set_default(netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, ecmif_init, tcpip_input));
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, ecmif_shutdown);
#endif
    netif_set_up(xnetif);

    ecm_netif = xnetif;
}

static void EcmGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (!ecm_netif)
    {
        ITP_LOG_ERR("[%s]ecm is not init yet\n", __func__ );
        return;
    }

    if (info->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of ecm: %d\n", __func__, info->index );
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
        info->flags |= ITP_ETH_LINKUP;

    if (xnetif->ip_addr.addr)
        info->flags |= ITP_ETH_ACTIVE;

    info->address = xnetif->ip_addr.addr;
    info->netmask = xnetif->netmask.addr;
    sprintf(info->displayName, "Ecm%d", info->index);
    sprintf(info->name, "ecm%d", info->index);
}

static void EcmPoll(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (!ecm_netif)
        return;

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

    // check for packets and link status
    ecmif_poll(xnetif);
}

#define CFG_NET_ECM_POLL_INTERVAL  50

#if CFG_NET_ECM_POLL_INTERVAL > 0

static void EcmPollHandler(timer_t timerid, int arg)
{
    ethInPollingFunc = true;
    EcmPoll();
    ethInPollingFunc = false;
}

#endif // CFG_NET_ECM_POLL_INTERVAL > 0

static int EcmInit(void)
{
#if CFG_NET_ECM_POLL_INTERVAL > 0
    {
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &ethTimer);
        timer_connect(ethTimer, (VOIDFUNCPTR)EcmPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec  = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_ECM_POLL_INTERVAL * 1000000;
        timer_settime(ethTimer, 0, &value, NULL);
    }
#endif // CFG_NET_ECM_POLL_INTERVAL > 0

    ethInited = true;

    return 0;
}

static void EcmExit(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (ethInited == false)
        return;

    while (ethInPollingFunc)
        usleep(1000);

    timer_delete(ethTimer);

    if (ecm_netif) {
        netif_set_down(xnetif);
        netif_remove(xnetif);
        ecm_netif = NULL;
    }

    ethInited = false;
}

static void EcmReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    if (!ecm_netif)
    {
        ITP_LOG_ERR("[%s]ecm is not init yet\n", __func__ );
        return;
    }

    if (setting->index >= 1)
    {
        ITP_LOG_ERR("[%s]Out of ecm: %d\n", __func__, setting->index );
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

static int EcmIoctl(int file, unsigned long request, void *ptr, void *info)
{
    switch (request)
    {
    case ITP_IOCTL_POLL:
        //EcmPoll();
        break;

    case ITP_IOCTL_IS_AVAIL:
        if (ecm_netif)
            return ethNetifs[0].ip_addr.addr ? 1 : 0;
        else
            return 0;

    case ITP_IOCTL_IS_CONNECTED:
        if (ecm_netif)
            return netif_is_link_up(ecm_netif);
        else
            return 0;

    case ITP_IOCTL_GET_INFO:
        EcmGetInfo((ITPEthernetInfo *)ptr);
        break;

    case ITP_IOCTL_INIT:
        return EcmInit();
        break;

    case ITP_IOCTL_EXIT:
        EcmExit();
        break;

    case ITP_IOCTL_RESET:
        EcmReset((ITPEthernetSetting *)ptr);
        break;

    case ITP_IOCTL_RESET_DEFAULT:
        if (!ecm_netif)
            return -1;
        if (!netif_is_def(&ethNetifs[0])) {
            printf("\n ==== itp ecm set netif 0 default ====\n");
            netif_set_default(&ethNetifs[0]);
        }
        break;

    case ITP_IOCTL_ENABLE:
        if (!ecm_netif)
            return -1;
        if (!netif_is_link_up(&ethNetifs[0]))
            netif_set_link_up(&ethNetifs[0]);
        break;

    case ITP_IOCTL_DISABLE:
        if (!ecm_netif)
            return -1;
        if (netif_is_link_up(&ethNetifs[0]))
            netif_set_link_down(&ethNetifs[0]);
        break;

	case ITP_IOCTL_SLEEP:
    case ITP_IOCTL_NETIF_REMOVE:
        if (ecm_netif) {
            while (ethInPollingFunc)
                usleep(1000);

            if (ethDhcp) {
                dhcp_stop(&ethNetifs[0]);
                ethDhcp = false;
            }
            netif_set_down(&ethNetifs[0]);
            netif_remove(&ethNetifs[0]);
            ecm_netif = NULL;
            printf("[%s] ecm netif remove!! \n", __func__);
        }
		break;

	case ITP_IOCTL_RESUME:
    case ITP_IOCTL_NETIF_ADD:
        if (!ecm_netif) {
            itpEcmLwipInit();
            printf("[%s] ecm netif add!! \n", __func__);
        }
		break;

    case ITP_IOCTL_RENEW_DHCP:
        if (!ecm_netif)
            return -1;
        dhcp_network_changed(&ethNetifs[0]);
        break;

    default:
        errno = (ITP_DEVICE_ECM_EX << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbEcmex =
{
    ":ecmex",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    EcmIoctl,
    NULL
};
