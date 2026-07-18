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
#include "ite/ite_mac.h"
#include "ite/itp.h"
#ifdef CFG_NET_LWIP_2
#include "netif/bridgeif.h" //eason
#ifdef CFG_NET_LWIP_PPPOE
#include "netif/ppp/pppoe.h"
#endif
#endif

#ifdef __OPENRTOS__
#include "openrtos/FreeRTOSConfig.h"
#define MAC_TASK_PRIORITY 5
#if defined(CFG_NET_ETHERNET) && !defined(CFG_NET_AMEBA_SDIO)
#define ITE_MAC
#endif
#endif // __OPENRTOS__

#define MAC_STACK_SIZE        (128 * 1024)

err_t ethernetif_init(struct netif *netif);
void  ethernetif_shutdown(struct netif *netif);
void  ethernetif_poll(struct netif *netif);

#ifdef CFG_NET_ETHERNET_MAC_ADDR_DEFAULT
static uint8_t macAddr[6] = { CFG_NET_ETHERNET_MAC_ADDR_DEFAULT };
#else
static uint8_t macAddr[6];
#endif

#ifdef CFG_GPIO_ETHERNET
static const uint8_t  ioConfig[] = { CFG_GPIO_ETHERNET };
#else
static const uint8_t  ioConfig[10];
#endif

#if (ETHMAC_SUPPORT_VLAN || ETHARP_SUPPORT_VLAN)
static struct netif   ethNetifs[MAX_VLAN_NETIFS];
static int vlan_num = 0;
#else
static struct netif   ethNetifs[CFG_NET_ETHERNET_COUNT];
#endif
#if LWIP_IPV4
static struct dhcp    ethNetifDhcp;
#elif LWIP_IPV6_DHCP6
static struct dhcp6   ethNetifDhcp6;
#endif

#if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
static struct netif *ecm_netif = NULL;
#endif

static bool           ethInited, ethDhcp;
static struct timeval ethLastTime;
static int            ethMscnt;
static timer_t        ethTimer;
static bool           ethInPollingFunc;

#ifdef CFG_NET_ETHERNET_AUTOIP
static struct autoip  ethNetifAutoip;
static bool           ethAutoip;
#endif

#ifdef ITE_MAC

#ifdef CFG_NET_ETHERNET_MAC_ADDR_STORAGE

static const char *ethMacVerifyCode = CFG_NET_ETHERNET_MAC_ADDR_VERIFY_CODE;

int MacRead(uint8_t *macaddr)
{
    uint8_t  *buf   = NULL;
    uint32_t pos, blocksize = 0;
    int      ret, i;
    uint32_t offset = 0;

    ITP_LOG_INFO("Read MAC address from storage\n" );

#if defined(CFG_NET_ETHERNET_MAC_ADDR_NAND)
    int fd = open(":nand", O_RDWR, 0);
    ITP_LOG_DBG("nand fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_NOR)
    int fd = open(":nor", O_RDWR, 0);
    ITP_LOG_DBG("nor fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_SD0)
    int fd = open(":sd0", O_RDWR, 0);
    ITP_LOG_DBG("sd0 fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_SD1)
    int fd = open(":sd1", O_RDWR, 0);
    ITP_LOG_DBG("sd1 fd: 0x%X\n", fd );
#else
    int fd;
#endif
    if (fd == -1)
    {
        ITP_LOG_ERR("open device error: %d\n", fd );
        ret = __LINE__;
        goto end;
    }
    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ITP_LOG_ERR("get block size error\n" );
        ret = __LINE__;
        goto end;
    }
    ITP_LOG_DBG("blocksize=%d\n", blocksize );

    pos    = CFG_NET_ETHERNET_MAC_ADDR_POS / blocksize;

        #if defined(CFG_NET_ETHERNET_MAC_ADDR_NAND)
    offset = CFG_NET_ETHERNET_MAC_ADDR_POS % blocksize;
        #endif

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        ITP_LOG_ERR("seek to mac addr position %d(%d) error\n", CFG_NET_ETHERNET_MAC_ADDR_POS, pos );
        ret = __LINE__;
        goto end;
    }

    assert(blocksize >= 8);
    buf = (uint8_t *)malloc(blocksize);
    if (!buf)
    {
        ret = __LINE__;
        goto end;
    }

    ret = read(fd, buf, 1);
    if (ret != 1)
    {
        ITP_LOG_ERR("read mac address error: %d != 1\n", ret );
        ret = __LINE__;
        goto end;
    }

    // verify number
    if (buf[0 + offset] == ethMacVerifyCode[0] && buf[1 + offset] == ethMacVerifyCode[1])
    {
        memcpy(macaddr, &buf[2 + offset], 6);
        ret = 0;
    }
    else
    {
        ITP_LOG_WARN("mac address %02X-%02X-%02X-%02X-%02X-%02X verify code %02X%02X incorrect; expect %02X%02X\n",
        buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[0], buf[1], ethMacVerifyCode[0], ethMacVerifyCode[1]
        );
    }

end:
    if (fd != -1)
        close(fd);

    if (buf)
        free(buf);

    return ret;
}

void MacWrite(uint8_t *macaddr)
{
    uint8_t  *buf   = NULL;
    uint32_t pos, blocksize = 0;
    int      ret, i;
    uint32_t offset = 0;

    ITP_LOG_INFO("Write MAC address to storage...\n" );

#if defined(CFG_NET_ETHERNET_MAC_ADDR_NAND)
    int fd = open(":nand", O_RDWR, 0);
    ITP_LOG_DBG("nand fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_NOR)
    int fd = open(":nor", O_RDWR, 0);
    ITP_LOG_DBG("nor fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_SD0)
    int fd = open(":sd0", O_RDWR, 0);
    ITP_LOG_DBG("sd0 fd: 0x%X\n", fd );
#elif defined(CFG_NET_ETHERNET_MAC_ADDR_SD1)
    int fd = open(":sd1", O_RDWR, 0);
    ITP_LOG_DBG("sd1 fd: 0x%X\n", fd );
#else
    int fd;
#endif
    if (fd == -1)
    {
        ITP_LOG_ERR("open device error: %d\n", fd );
        ret = __LINE__;
        goto end;
    }
    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ITP_LOG_ERR("get block size error\n" );
        ret = __LINE__;
        goto end;
    }
    ITP_LOG_DBG("blocksize=%d\n", blocksize );

    pos    = CFG_NET_ETHERNET_MAC_ADDR_POS / blocksize;

        #if defined(CFG_NET_ETHERNET_MAC_ADDR_NAND)
    offset = CFG_NET_ETHERNET_MAC_ADDR_POS % blocksize;
        #endif

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        ITP_LOG_ERR("seek to mac addr position %d(%d) error\n", CFG_NET_ETHERNET_MAC_ADDR_POS, pos );
        ret = __LINE__;
        goto end;
    }

    assert(blocksize >= 8);
    buf = (uint8_t *)malloc(blocksize);
    if (!buf)
    {
        ret = __LINE__;
        goto end;
    }

        #if defined(CFG_NET_ETHERNET_MAC_ADDR_NAND)
    if (read(fd, buf, 1) != 1)
    {
        ITP_LOG_ERR("read storage error: %d != 1\n", ret );
        ret = __LINE__;
        goto end;
    }

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        ITP_LOG_ERR("seek to mac addr position %d(%d) error\n", CFG_NET_ETHERNET_MAC_ADDR_POS, pos );
        ret = __LINE__;
        goto end;
    }
        #endif

    buf[0 + offset] = ethMacVerifyCode[0];
    buf[1 + offset] = ethMacVerifyCode[1];
    for (i = 0; i < 6; i++)
    {
        buf[i + 2 + offset] = macaddr[i];
    }

    if (write(fd, buf, 1) != 1)
    {
        ITP_LOG_ERR("write mac addr fail\n" );
        goto end;
    }

end:
    if (fd != -1)
        close(fd);

    if (buf)
        free(buf);
}

#endif // CFG_NET_ETHERNET_MAC_ADDR_STORAGE



static ITE_MAC_CFG_T mac_cfg;

int MacInit(bool initPhy)
{
    int ret, i;

    mac_cfg.flags |= (ITH_COUNT_OF(ioConfig) == ITE_MAC_GRMII_PIN_CNT) ? ITE_MAC_RGMII : 0;
    mac_cfg.clk_inv       = CFG_NET_MAC_CLOCK_INVERSE;
    mac_cfg.clk_delay     = CFG_NET_MAC_CLOCK_DELAY;
    mac_cfg.rxd_delay = CFG_NET_MAC_RXD_DELAY;
    mac_cfg.phyAddr       = CFG_NET_ETHERNET_PHY_ADDR;
    mac_cfg.ioConfig      = ioConfig;
    mac_cfg.linkGpio      = CFG_GPIO_ETHERNET_LINK;
    mac_cfg.phy_link_change = itpPhyLinkChange;
    mac_cfg.linkGpio_isr  = itpPhylinkIsr;
    mac_cfg.phy_link_status = itpPhyLinkStatus;
    mac_cfg.phy_read_mode = itpPhyReadMode;

    ret                   = iteMacInitialize(&mac_cfg);
    if (ret)
        return ret;

    #ifdef CFG_NET_ETHERNET_MAC_ADDR_RANDOM
    ITP_LOG_DBG("Random generate default MAC address\n" );
    srand(ithTimerGetCounter(portTIMER));
    macAddr[0] = 0;
    for (i = 1; i < 6; i++)
        macAddr[i] = (rand() % 256);

    #endif // CFG_NET_ETHERNET_MAC_ADDR_RANDOM

    #ifdef CFG_NET_ETHERNET_MAC_ADDR_STORAGE
    // read mac address from storage
    if (MacRead(macAddr))
    {
        #ifdef CFG_NET_ETHERNET_MAC_ADDR_UPGRADE
        MacWrite(macAddr);
        #endif
    }

    #endif // CFG_NET_ETHERNET_MAC_ADDR_STORAGE

    ITP_LOG_INFO("MAC address: %02X-%02X-%02X-%02X-%02X-%02X\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5] );

    ret = iteMacSetMacAddr((uint8_t *)macAddr);
    if (ret)
        return ret;

    if (initPhy)
        PhyInit(ITE_ETH_REAL);

    return 0;
}

#endif // ITE_MAC

#if LWIP_IPV4
// This function initializes all network interfaces
void itpEthernetLwipInit(void)
{
	//Normal Netif Initial
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    xnetif = &ethNetifs[0];

    #ifdef CFG_NET_ETHERNET_IPADDR
    ipaddr_aton(CFG_NET_ETHERNET_IPADDR,  &ipaddr);
    ipaddr_aton(CFG_NET_ETHERNET_NETMASK, &netmask);
    ipaddr_aton(CFG_NET_ETHERNET_GATEWAY, &gw);
    #endif // CFG_NET_ETHERNET_IPADDR
#if defined (CFG_NET_PRIO_MULTI_MAC)
    uint8_t      prio[] = { CFG_NET_PRIO_MULTI_MAC };

    netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
    printf("==== 4G[%d], WIFI[%d], ETH[%d] ====\n", prio[0], prio[1], prio[2]);

    if (prio[1] > 0) {
        if ((prio[0] > 0 && prio[2] > 0)) {
            /* Case 1: 4G + WIFI + Ethernet */
            if ((prio[2] < prio[1]) && (prio[2] < prio[0])) {
                printf("Set Eth netif as default in Multi-MAC case [G+W+E]\n");
                netif_set_default(xnetif);
            }
        } else if (prio[0] > 0) {
            /* Case 2: 4G(Use Eth middleware) + WIFI */
            if (prio[0] < prio[1]) {
                printf("Set 4G(Use Eth middleware) netif as default in Multi-MAC case [G+W]\n");
                netif_set_default(xnetif);
            }
        } else if (prio[2] > 0) {
            /* Case 3: Eth + WIFI */
            if (prio[2] < prio[1]) {
                printf("Set Eth netif as default in Multi-MAC case [E+W]\n");
                netif_set_default(xnetif);
            }
        }
    }
#else
    /* Case 4: Only Eth */
    /* Case 5: Only 4G */
    netif_set_default(netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input));
#endif
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, ethernetif_shutdown);
#endif
    netif_set_up(xnetif);
#if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
    ecm_netif = xnetif;
#endif
    #if defined(CFG_NET_ETHERNET_DHCP) || defined(CFG_NET_ETHERNET_AUTOIP)

    #ifdef CFG_NET_ETHERNET_DHCP
    dhcp_set_struct(xnetif, &ethNetifDhcp);
    dhcp_start(xnetif);
    ethDhcp = true;

    #endif // CFG_NET_ETHERNET_DHCP

    #ifdef CFG_NET_ETHERNET_AUTOIP
    autoip_set_struct(xnetif, &ethNetifAutoip);
    autoip_start(xnetif);
    ethAutoip = true;

    #endif // CFG_NET_ETHERNET_AUTOIP

    gettimeofday(&ethLastTime, NULL);
    ethMscnt = 0;

    #endif // defined(CFG_NET_ETHERNET_DHCP) || defined(CFG_NET_ETHERNET_AUTOIP)

    ethInited = true;
}

#if (ETHMAC_SUPPORT_VLAN || ETHARP_SUPPORT_VLAN)
#define MAX_VLAN_CONFIGS 10
static struct vlan_config vlan_configs[MAX_VLAN_CONFIGS];

static void vlan_configs_set(struct vlan_config *configs, int *count)
{
    int i = 0;

    // VLAN 0 (default VLAN)
    configs[i].pcp= 0;
    configs[i].vid= 0;
    configs[i].tci = TCI_VAL_SET(configs[i].pcp, configs[i].vid);
    IP4_ADDR(&configs[i].ip, 192,168,1,1);
    IP4_ADDR(&configs[i].netmask, 255,255,255,0);
    IP4_ADDR(&configs[i].gateway, 192,168,1,1);
    i++;

    // VLAN 10
    configs[i].pcp = 4;
    configs[i].vid = 10;
    configs[i].tci= TCI_VAL_SET(configs[i].pcp, configs[i].vid);
    IP4_ADDR(&configs[i].ip, 192,168,191,128);
    IP4_ADDR(&configs[i].netmask, 255,255,255,0);
    IP4_ADDR(&configs[i].gateway, 192,168,191,1);
    i++;

    // VLAN 20
    configs[i].pcp= 6;
    configs[i].vid= 20;
    configs[i].tci = TCI_VAL_SET(configs[i].pcp, configs[i].vid);
    IP4_ADDR(&configs[i].ip, 192,168,192,128);
    IP4_ADDR(&configs[i].netmask, 255,255,255,0);
    IP4_ADDR(&configs[i].gateway, 192,168,192,1);
    i++;

    /*
            another configs
            ....
       */

    /* Set End tag */
    configs[i].pcp= 0;
    configs[i].vid= 4095;
    configs[i].tci = TCI_VAL_SET(configs[i].pcp, configs[i].vid);

    /* return count */
    *count = i;

    for(int j = 0; j < i; j++)
        printf("[vlan_configs_set] VID 0x%X - PCP 0x%X - TCI 0x%X\n",
            configs[j].vid, configs[j].pcp, configs[j].tci);
}

void itpEthernetVlanInit(void)
{
    struct netif *default_netif = NULL;
    ip4_addr_t    ipaddr, netmask, gw;
    int vlan_count = 0;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    #ifdef CFG_NET_ETHERNET_IPADDR
    ipaddr_aton(CFG_NET_ETHERNET_IPADDR,  &ipaddr);
    ipaddr_aton(CFG_NET_ETHERNET_NETMASK, &netmask);
    ipaddr_aton(CFG_NET_ETHERNET_GATEWAY, &gw);
    #endif // CFG_NET_ETHERNET_IPADDR

    vlan_configs_set(vlan_configs, &vlan_count);
    vlan_config_init(vlan_configs, vlan_count + 1); /* +1 for end tag */
    vlan_num = vlan_count;

    printf("Set %d VLAN netif\n", vlan_count);

    /* Set VLAN ID in ethernetif_init(), it can help the coding with consistency */
    for (int i = 0; i < vlan_count; i++) {
        struct vlan_config *config = &vlan_configs[i];
        struct netif *netif = &ethNetifs[i];
        config->netif = netif;

        if (netif_add_with_vlan(netif, &config->ip, &config->netmask, &config->gateway,
                NULL, ethernetif_init, tcpip_input, config->tci) != NULL) {
            netif_set_up(netif);

            if (config->vid == 10) {
                default_netif = netif;
            }
        } else {
            printf("Failed to add VLAN netif for VID %d\n", config->vid);
        }
    }

    if (default_netif != NULL) {
        netif_set_default(default_netif);
    }

    #if defined(CFG_NET_ETHERNET_DHCP)
    dhcp_set_struct(xnetif, &ethNetifDhcp);
    dhcp_start(xnetif);
    ethDhcp = true;

    gettimeofday(&ethLastTime, NULL);
    ethMscnt = 0;
    #endif // defined(CFG_NET_ETHERNET_DHCP)

    ethInited = true;
}
#endif

static void EthernetGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (ethInited == false)
    {
        ITP_LOG_ERR("ethernet is not init yet\n" );
        return;
    }

    if (info->index >= CFG_NET_ETHERNET_COUNT)
    {
        ITP_LOG_ERR("Out of ethernet: %d\n", info->index );
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
        info->flags |= ITP_ETH_LINKUP;

    if (xnetif->ip_addr.addr)
        info->flags |= ITP_ETH_ACTIVE;

    info->address = xnetif->ip_addr.addr;
    info->netmask = xnetif->netmask.addr;
    sprintf(info->displayName, "Ethernet%d", info->index);
    memcpy((void *)info->hardwareAddress, (void *)macAddr, 6);
    sprintf(info->name, "eth%d", info->index);
}

static void EthernetPoll(void)
{
    struct netif *xnetif = &ethNetifs[CFG_NET_ETHERNET_COUNT - 1];

    if (!ethInited)
        return;

    if ((xnetif->ip_addr.addr == 0)
        && (ethDhcp
#ifdef CFG_NET_ETHERNET_AUTOIP
            || ethAutoip
#endif
            ))
    {
        struct timeval currTime;

        gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&ethLastTime, &currTime) >= DHCP_FINE_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp_fine_tmr();

#ifdef CFG_NET_ETHERNET_AUTOIP
            if (ethAutoip)
                autoip_tmr();
#endif

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
    ethernetif_poll(xnetif);
}

static void EthernetReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    if (setting->index >= CFG_NET_ETHERNET_COUNT)
    {
        ITP_LOG_ERR("Out of ethernet: %d\n", setting->index );
        return;
    }

    xnetif = &ethNetifs[setting->index]; //index_0 is interface_0, index_1 is interface_main
#if (ETHARP_SUPPORT_VLAN || ETHMAC_SUPPORT_VLAN)
    u8_t pcp;
    u16_t vid;
#if LWIP_VLAN_PCP
    s32_t tci = xnetif->hints->tci;
    vid = VID_VAL_GET(xnetif->hints->tci);
    pcp = PCP_VAL_GET(xnetif->hints->tci);
#else
    u16_t *tci_val = (u16_t*)netif_get_client_data(xnetif, LWIP_NETIF_CLIENT_DATA_INDEX_VLAN);
    u16_t tci = *tci_val;
    vid = (u16_t)VID_VAL_GET(*tci_val);
    pcp = (u8_t)PCP_VAL_GET(*tci_val);
#endif
#endif

    if (setting->index == CFG_NET_ETHERNET_COUNT - 1)
    {
        ip_addr_set_zero(&xnetif->ip_addr);

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
#ifdef CFG_NET_ETHERNET_AUTOIP
            if (!setting->autoip && ethAutoip)
            {
                autoip_stop(xnetif);
                ethAutoip = false;
            }

            if (setting->autoip && !ethAutoip)
            {
                autoip_set_struct(xnetif, &ethNetifAutoip);
                autoip_start(xnetif);
                ethAutoip = true;
            }
#endif         // CFG_NET_ETHERNET_AUTOIP
            gettimeofday(&ethLastTime, NULL);
            ethMscnt = 0;
            return;
        }
        else
        {
            if (ethDhcp)
                dhcp_stop(xnetif);

            ethDhcp = false;

#ifdef CFG_NET_ETHERNET_AUTOIP
            if (ethAutoip)
                autoip_stop(xnetif);

            ethAutoip = false;

#endif         // CFG_NET_ETHERNET_AUTOIP
        }
    }
    //netif_set_down(xnetif);
    IP4_ADDR(&ipaddr,  setting->ipaddr[0],  setting->ipaddr[1],  setting->ipaddr[2],  setting->ipaddr[3]);
    IP4_ADDR(&netmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
    IP4_ADDR(&gw,      setting->gw[0],      setting->gw[1],      setting->gw[2],      setting->gw[3]);

    netif_set_addr(xnetif, &ipaddr, &netmask, &gw);
    netif_set_up(xnetif);
#if (ETHARP_SUPPORT_VLAN || ETHMAC_SUPPORT_VLAN)
    printf("[EthernetReset] No.1 [0x%X] %s - PCP 0x%X - VID 0x%X - TCI 0x%X\n",
        xnetif, ipaddr_ntoa(&(xnetif->ip_addr)), pcp, vid, tci);
    /* Another VLAN netif reset */
    if (vlan_num > 1) {
        for (int i = 1; i < vlan_num; i++) {
            struct vlan_config *config = &vlan_configs[i];
            xnetif = &ethNetifs[i];

            ip_addr_set_zero(&xnetif->ip_addr);
            netif_set_addr(xnetif, &config->ip, &config->netmask, &config->gateway);
            netif_set_up(xnetif);

            {
            #if LWIP_VLAN_PCP
                s32_t tci = xnetif->hints->tci;
                vid = VID_VAL_GET(xnetif->hints->tci);
                pcp = PCP_VAL_GET(xnetif->hints->tci);
            #else
                u16_t tci = *(u16_t*)netif_get_client_data(xnetif, LWIP_NETIF_CLIENT_DATA_INDEX_VLAN);
                vid = (u16_t)VID_VAL_GET(*tci_val);
                pcp = (u8_t)PCP_VAL_GET(*tci_val);
            #endif
                printf("[EthernetReset] No.%d [0x%X] %s - PCP 0x%X - VID 0x%X - TCI 0x%X\n",
                    i + 1, xnetif, ipaddr_ntoa(&(xnetif->ip_addr)),
                    pcp, vid, tci);
            }
        }
    }
#endif
}

static void EthernetSendArpToDetect(void)
{
    struct netif *xnetif;

    xnetif = &ethNetifs[CFG_NET_ETHERNET_COUNT - 1];

    if (CFG_NET_ETHERNET_COUNT == 1) //avoid multi -interface
        etharp_request(xnetif, &xnetif->ip_addr);
}
#endif

#if LWIP_IPV6
#include "lwip/dhcp6.h"
#define IPV6_PRIME ((LWIP_IPV6_NUM_ADDRESSES > 1) ? 1 : 0)
void itpEthernetLwipInit(void)
{
    struct netif *xnetif = &ethNetifs[0];
    ip6_addr_t addr_prime = IPADDR6_INIT_HOST(0x2001b030, 0x23090000, 0x0, 0x1);
    //ip6_addr_t addr_local = IPADDR6_INIT_HOST(0x20010db8, 0x0, 0x0, 0x1);

    /* Set zero IP into ip6_addr */
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
        ip_addr_set_zero_ip6(&xnetif->ip6_addr[i]);

    /* Set static IPv6 address in ip6_addr[prime] */
    netif_ip6_addr_set(xnetif, IPV6_PRIME, ip_2_ip6(&addr_prime));
    netif_set_default(netif_add(xnetif, NULL, ethernetif_init, tcpip_input));

    /* Create local address in ip6_addr[0] */
    netif_create_ip6_linklocal_address(xnetif, 1); //It likes IPv4 local: 169.254.0.0
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, ethernetif_shutdown);
#endif
    netif_set_up(xnetif);
#if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
    ecm_netif = xnetif;
#endif

    ethInited = true;
}

static void EthernetGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (ethInited == false)
    {
        ITP_LOG_ERR("ethernet is not init yet\n" );
        return;
    }

    if (info->index >= CFG_NET_ETHERNET_COUNT)
    {
        ITP_LOG_ERR("Out of ethernet: %d\n", info->index );
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
        info->flags |= ITP_ETH_LINKUP;

    if(!ip6_addr_isinvalid(netif_ip6_addr_state(xnetif, IPV6_PRIME)))
        info->flags |= ITP_ETH_ACTIVE;

    for (int i = 0; i < 4; i++) {
        info->address[i] = xnetif->ip6_addr[IPV6_PRIME].addr[i];
    }
    sprintf(info->displayName, "Ethernet%d", info->index);
    memcpy((void *)info->hardwareAddress, (void *)macAddr, 6);
    sprintf(info->name, "eth%d", info->index);
}

static void EthernetPoll(void)
{
    struct netif *xnetif = &ethNetifs[CFG_NET_ETHERNET_COUNT - 1];

    if (!ethInited)
        return;

#if LWIP_IPV6_DHCP6
    if ((ip_addr_isany_val(xnetif->ip6_addr[IPV6_PRIME]) == 0)&& ethDhcp)
    {
        struct timeval currTime;

        gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&ethLastTime, &currTime) >= DHCP6_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp6_tmr();

            ethLastTime = currTime;
            ethMscnt   += DHCP6_TIMER_MSECS;
        }
    }
#endif

    // check for packets and link status
    ethernetif_poll(xnetif);
}

static void EthernetReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip6_addr_t    ip6addr;

    if (setting->index >= CFG_NET_ETHERNET_COUNT)
    {
        ITP_LOG_ERR("Out of ethernet: %d\n", setting->index );
        return;
    }
    printf("IPv6 Reset\n");

    xnetif = &ethNetifs[setting->index]; //index_0 is interface_0, index_1 is interface_main

    if (setting->index == CFG_NET_ETHERNET_COUNT - 1)
    {
        ip6_addr_set_zero(&xnetif->ip6_addr[setting->ip6addr_idx]);
#if LWIP_IPV6_DHCP6
        if (setting->dhcp || setting->autoip)
        {
            if (!setting->dhcp && ethDhcp)
            {
                dhcp6_disable(xnetif);
                ethDhcp = false;
            }

            if (setting->dhcp)
            {
                dhcp6_set_struct(xnetif, &ethNetifDhcp6);
                dhcp6_enable_stateless(xnetif);
                ethDhcp = true;
            }
            gettimeofday(&ethLastTime, NULL);
            ethMscnt = 0;
            return;
        }
        else
        {
            if (ethDhcp)
                dhcp6_disable(xnetif);

            ethDhcp = false;
        }
#endif
    }
    IP6_ADDR(&ip6addr, (setting->ip6addr[0]), (setting->ip6addr[1]), (setting->ip6addr[2]), (setting->ip6addr[3]));

    netif_ip6_addr_set(xnetif, IPV6_PRIME, &ip6addr);
    netif_set_up(xnetif);
}

static void EthernetSendArpToDetect(void)
{
    /* Useless for LWIP IPv6 */
}

#endif

#if CFG_NET_ETHERNET_POLL_INTERVAL > 0

static void EthernetPollHandler(timer_t timerid, int arg)
{
    ethInPollingFunc = true;
    EthernetPoll();
    ethInPollingFunc = false;
}

#endif // CFG_NET_ETHERNET_POLL_INTERVAL > 0

static int EthernetInit(void)
{
    // create mac poll task
#ifdef ITE_MAC
    {
        pthread_t          task;
        pthread_attr_t     attr;
        struct sched_param param;
        int                res;

    #ifdef CFG_NET_MAC_INIT_ON_BOOTLOADER
        res = MacInit(false);
    #else
        res = MacInit(true);
    #endif
        if (res)
        {
            errno = (ITP_DEVICE_ETHERNET << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }

        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, MAC_STACK_SIZE);
        param.sched_priority = MAC_TASK_PRIORITY;
        pthread_attr_setschedparam(&attr, &param);
        pthread_create(&task, &attr, iteMacThreadFunc, NULL);
    }
#endif // ITE_MAC

#if CFG_NET_ETHERNET_POLL_INTERVAL > 0
    {
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &ethTimer);
        timer_connect(ethTimer, (VOIDFUNCPTR)EthernetPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec  = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_ETHERNET_POLL_INTERVAL * 1000000;
        timer_settime(ethTimer, 0, &value, NULL);
    }
#endif // CFG_NET_ETHERNET_POLL_INTERVAL > 0

    return 0;
}

static void EthernetExit(void)
{
    int i;

    timer_delete(ethTimer);
    while (ethInPollingFunc)
        usleep(1000);
    for (i = 0; i < CFG_NET_ETHERNET_COUNT; ++i)
    {
        struct netif *xnetif = &ethNetifs[i];
        netif_set_down(xnetif);
        netif_remove(xnetif);
#if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
        ecm_netif = NULL;
#endif
    }
}

static int EthernetIoctl(int file, unsigned long request, void *ptr, void *info)
{
    switch (request)
    {
    case ITP_IOCTL_POLL:
        EthernetPoll();
        break;

#if !defined(WIN32) && defined(CFG_NET_ETHERNET_DETECT_IP)
	case ITP_IOCTL_ETHERNET_SEND_ARP:
		EthernetSendArpToDetect();
		break;

	case ITP_IOCTL_IP_DUPLICATE:
		return etharp_ip_conflict();
#endif

    case ITP_IOCTL_IS_AVAIL:
#ifdef CFG_NET_ETHERNET_MULTI_INTERFACE
        eth_netif[0] = ethNetifs[0].ip_addr.addr ? 1 : 0;
		eth_netif[1] = ethNetifs[1].ip_addr.addr ? 1 : 0;

        return eth_netif[0] && eth_netif[1];
#elif defined(CFG_IOT_ENABLE)
		return 1;
#else
    #if LWIP_IPV4
        return ethNetifs[0].ip_addr.addr ? 1 : 0;
    #elif LWIP_IPV6
        return (!ip_addr_isany(&ethNetifs[CFG_NET_ETHERNET_COUNT - 1].ip6_addr[IPV6_PRIME]));
    #endif
#endif

    case ITP_IOCTL_IS_CONNECTED:
        return netif_is_link_up(&ethNetifs[CFG_NET_ETHERNET_COUNT - 1]);


    case ITP_IOCTL_GET_INFO:
        EthernetGetInfo((ITPEthernetInfo *)ptr);
        break;

    case ITP_IOCTL_INIT:
        return EthernetInit();
        break;

    case ITP_IOCTL_EXIT:
        EthernetExit();
        break;

    case ITP_IOCTL_RESET:
        EthernetReset((ITPEthernetSetting *)ptr);
        break;

    case ITP_IOCTL_RESET_DEFAULT:
        if (!netif_is_def(&ethNetifs[0])) {
            netif_set_default(&ethNetifs[0]);
        }
        break;

    case ITP_IOCTL_ENABLE:
        if (!netif_is_link_up(&ethNetifs[0]))
            netif_set_link_up(&ethNetifs[0]);
        break;

    case ITP_IOCTL_DISABLE:
        netif_set_link_down(&ethNetifs[0]);
        break;

#if defined(NET_RTL8201FL)
    case ITP_IOCTL_ON:
    #if defined(ITE_MAC)
        if (((int)ptr) == ITP_PHY_WOL)
            PhyWolEnter();
    #endif
        break;

    case ITP_IOCTL_OFF:
    #if defined(ITE_MAC)
        if (((int)ptr) == ITP_PHY_WOL)
            PhyWolExit();
    #endif
        break;
#endif

    case ITP_IOCTL_WIRTE_MAC:
#ifdef CFG_NET_ETHERNET_MAC_ADDR_STORAGE
        MacWrite((uint8_t *)ptr);
#endif
        break;

	#if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
	case ITP_IOCTL_SLEEP:
    case ITP_IOCTL_NETIF_REMOVE:
        if (ecm_netif) {
            if (ethDhcp) {
                dhcp_stop(&ethNetifs[0]);
                ethDhcp = false;
            }
            netif_set_down(&ethNetifs[0]);
            netif_remove(&ethNetifs[0]);

            ecm_netif = NULL;
        }
		break;

	case ITP_IOCTL_RESUME:
    case ITP_IOCTL_NETIF_ADD:
        if (!ecm_netif)
        	itpEthernetLwipInit();
		break;
	#endif // #if defined(CFG_NET_AMEBA_SDIO) || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)

    case ITP_IOCTL_RENEW_DHCP:
        #if LWIP_IPV4
        dhcp_network_changed(&ethNetifs[0]);
        #endif
    break;

    default:
        errno = (ITP_DEVICE_ETHERNET << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceEthernet =
{
    ":ethernet",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    EthernetIoctl,
    NULL
};
