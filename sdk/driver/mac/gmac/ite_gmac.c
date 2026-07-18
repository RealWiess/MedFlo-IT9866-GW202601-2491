#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "ite/ite_mac.h"

#include "mii.h"
#include "ndis.h"
#include "if_ether.h"
#include "gmac/gmac_types.h"
#include "gmac/config.h"
#include "gmac/skb.h"
#include "gmac/ip.h"
#include "gmac/ftgmac030.h"
#include "gmac/gmac_err.h"

//=============================================================================
//                              Global Data Definition
//=============================================================================
static struct eth_device eth_dev;
static struct eth_device *dev = NULL;


//=============================================================================
//                              sate Function Definition
//=============================================================================
static void MacConfig(struct ftgmac030_ctrl *ctrl, const uint8_t* ioConfig)
{
    int i;
    int io_cnt = ctrl->is_grmii ? ITE_MAC_GRMII_PIN_CNT : ITE_MAC_RMII_PIN_CNT;

    for (i = 0; i<io_cnt; i++) 
    {
#if (CFG_CHIP_FAMILY == 9860)
        ithGpioSetMode(ioConfig[i], ITH_GPIO_MODE1);
#else // 9830
        ithGpioSetMode(ioConfig[i], ITH_GPIO_MODE2);
#endif
    }

    // only for RMII: use internal refclk
	if (0)  
    { 
		ithWriteRegMaskA((uint32_t)ctrl->io_base + FTGMAC030_REG_CLKDLY, 0x400, 0x400);
		printf("0x%08" PRIX32 " = 0x%08" PRIX32 " \n", (ctrl->io_base + FTGMAC030_REG_CLKDLY), ithReadRegA(ctrl->io_base + FTGMAC030_REG_CLKDLY));
		/** enable reference clock */
		ithWriteRegA(ITH_HOST_BASE + 0x68, 0x002ac801);

        #if (CFG_CHIP_FAMILY == 9830)
        // Set in Kconfig
        //iteMacSetClock(0, 9, 0);
        #endif
	}
    if (ctrl->is_grmii)
    {
        iow32(FTGMAC030_REG_GISR, 0x2);  // D[1:0] RGMII = 10
    }
    else
    {
        iow32(FTGMAC030_REG_GISR, 0x1);  // D[1:0] RMII = 01
    }

	//printf("0x%08X = 0x%08X \n", (ctrl->io_base + FTGMAC030_REG_CLKDLY), ithReadRegA(ctrl->io_base + FTGMAC030_REG_CLKDLY));


#if defined(CFG_NET_ETHERNET_LINK_INTR)
    /* for link gpio */
    if (ctrl->cfg->linkGpio > 0)
    {
        ithGpioSetMode((unsigned int)ctrl->cfg->linkGpio, ITH_GPIO_MODE0);
    }
#endif

    /* enable gmac controller's clock setting */
    // TODO
}

static void phy_read_mode(struct eth_device *netdev, int* speed, int* duplex)
{
    struct ftgmac030_ctrl *ctrl = netdev->priv;
    struct mii_if_info *mii_if = &ctrl->mii;
    int i;

	*speed = SPEED_10;
	*duplex = DUPLEX_HALF;

    if (ctrl->cfg->phy_read_mode) 
    {
        int link_status;
        link_status = ctrl->cfg->phy_read_mode(speed, duplex); // 0: link up
        if (link_status) /* not link */
        {
            return;
        }
    } 
    else 
    {
        int val;
        for (i = 0; i < 2; i++)
        {
            val = mii_if->mdio_read(netdev, mii_if->phy_id, MII_BMSR);
        }
        if (val < 0) // I/O fail
        {
            return;
        }

        if (!((uint16_t)val & BMSR_LSTATUS))
        {
            return;
        }

        /* AutoNegotiate completed */
#if 0
        {
            MAC_UINT16 autoadv, autorec;
            autoadv = mii_if->mdio_read(netdev, mii_if->phy_id, MII_ADVERTISE);
            autorec = mii_if->mdio_read(netdev, mii_if->phy_id, MII_LPA);
            status = autoadv & autorec;

            if (status & (ADVERTISE_100HALF | ADVERTISE_100FULL))
            {
                *speed = SPEED_100;
            }
            if (status & (ADVERTISE_100FULL | ADVERTISE_10FULL))
            {
                *duplex = DUPLEX_FULL;
            }
        }
#else
        {
            uint32_t bmcr;
            char *speed_string[3] = { "1000Mbps", "100Mbps", "10Mbps" };
            char *str = speed_string[2];

            bmcr = (uint32_t)mii_if->mdio_read(netdev, mii_if->phy_id, MII_BMCR);
            if (bmcr & BMCR_SPEED1000) 
            {
                *speed = SPEED_1000;
                str = speed_string[0];
            }
            else if (bmcr & BMCR_SPEED100) 
            {
                *speed = SPEED_100;
                str = speed_string[1];
            }

            if (bmcr & BMCR_FULLDPLX)
            {
                *duplex = DUPLEX_FULL;
            }

            if (0)
            {
                ithPrintf(" Link On %s %s \n", str, *duplex == DUPLEX_FULL ? "full" : "half");
            }
        }
#endif
    }

    ctrl->autong_complete = 1;
}


#include "gmac.c"
#include "gmac_intr.c"
#include "gmac_netdev_ops.c"
#include "gmac_mdio.c"


//=============================================================================
//                              Public Function Definition
//=============================================================================
int iteMacInitialize(ITE_MAC_CFG_T* cfg)
{
    int rc=0;
    struct ftgmac030_ctrl *ctrl = NULL;
    unsigned long osclk;

    _mac_func_enter;

    if (dev)
    {
        return rc;
    }

	/** enable mac clock */
    ithWriteRegMaskA(ITH_HOST_BASE + 0x68, (0x1 << 21)|(0x1 << 19), (0x1 << 21)|(0x1 << 19));

    dev = &eth_dev;
    memset(dev, 0, sizeof(struct eth_device));
    ctrl = (struct ftgmac030_ctrl*)malloc(sizeof(struct ftgmac030_ctrl));
    if (!ctrl) 
    {
        rc = ERROR_MAC_PRIV_MEM_ALLOC_FAIL;
        goto end;
    }
    memset((void*)ctrl, 0, sizeof(struct ftgmac030_ctrl));
    dev->priv = ctrl;
    ctrl->netdev = dev;
    ctrl->max_hw_frame_size = DEFAULT_JUMBO;
    ctrl->io_base = ITH_GMAC_BASE;
	ctrl->cfg = cfg;
	ctrl->is_grmii = cfg->flags & ITE_MAC_RGMII;
	ctrl->phy_speed = SPEED_10;
	ctrl->phy_duplex = DUPLEX_HALF;

	printf("\n #### %s Ethernet #### \n\n", ctrl->is_grmii ? "Giga" : "Fast");

    ftgmac030_show_feature(ctrl);

    /* See ftgmac030.h for more flags */
    ctrl->flags = FLAG_HAS_JUMBO_FRAMES;

#if defined(PTP_ENABLE)
    if (ior32(FTGMAC030_REG_FEAR) & FTGMAC030_FEAR_PTP)
    {
        ctrl->flags |= FLAG_HAS_HW_TIMESTAMP;
    }
#endif

    ctrl->itr_setting = 3;
    ctrl->mta_reg_count = 4;
    /* Transmit Interrupt Delay in cycle units:
    *  1000 Mbps mode -> 16.384 繕s
    *  100 Mbps mode -> 81.92 繕s
    *  10 Mbps mode -> 819.2 繕s
    */
    ctrl->tx_int_delay = 8;


    /* Set initial default active device features */
    dev->features = (NETIF_F_RXCSUM | NETIF_F_HW_CSUM);
    //dev->features = 0;

    dev->flags = IFF_BROADCAST/* | IFF_MULTICAST*/;
    dev->mtu = 1500;

    /* Set user-changeable features (subset of all device features) */
    dev->hw_features = dev->features;
    //dev->hw_features |= NETIF_F_RXFCS;
    //dev->hw_features |= NETIF_F_RXALL;

    /* The MDC period = MDC_CYCTHR x system clock period.
    * Users must set the correct value before using MDC.
    * Note: IEEE 802.3 specifies minimum cycle is 400ns of MDC
    */
	//osclk = 40;
	osclk = ithGetA0_4clk()/1000000;  // A2CLK = AX2CLK/2  = 792/2 = 396
    ctrl->mdc_cycthr = (u8)(((400U + 10U) * osclk) / 1000);
	printf("mdc_cycthr = %X \n", ctrl->mdc_cycthr);

	/* setup ftgmac030_ctrl struct */
	rc = ftgmac030_sw_init(ctrl);
    if (rc)
    {
        goto err_sw_init;
    }

    /* reset the hardware with the new settings */
    ftgmac030_reset_hw(ctrl);

    rc = ftgmac030_mii_init(ctrl);
    if (rc)
    {
        goto err_init_miibus;
    }

    /* carrier off reporting is important to ethtool even BEFORE open */
    netif_carrier_off(dev);

    MacConfig(ctrl, cfg->ioConfig);

    return 0;

err_init_miibus:
err_sw_init:
end:
    if (ctrl)
    {
        free(ctrl);
    }

    check_result(rc);
    _mac_func_leave;
    return rc;
}

static int inited = 0;

int iteMacOpen (uint8_t * mac_addr, ITE_MAC_RX_CB rx_cb, void * arg, int mode)
{
    int rc=0;
    struct ftgmac030_ctrl *ctrl;
	ITE_MAC_CFG_T* cfg;
    _mac_func_enter;

    if (!dev)
    {
        return rc;
    }

    ctrl = dev->priv;
    ctrl->netif_rx  = rx_cb;
    ctrl->netif_arg = arg;
    ctrl->mode = ((unsigned int)mode) & 0xF;
    memset((void*)&dev->stats, 0x0, sizeof(struct net_device_stats));
    printf(" eth mode: %d \n", mode);
    if (mac_addr)
    {
        memcpy(mac_addr, dev->netaddr, 6);
    }

    if (inited)
    {
        return rc;
    }

    if (ctrl->mode != ITE_ETH_REAL) 
    {
        dev->features &= ~(NETIF_F_RXCSUM | NETIF_F_HW_CSUM);
        dev->features |= NETIF_F_RXALL;
    }

    rc = ftgmac030_open(dev);
    if (rc)
    {
        mac_err("[%s] rc = 0x%08X \n", __FUNCTION__, rc);
    }

    cfg = ctrl->cfg;
    iteMacSetClock(cfg->clk_inv, cfg->clk_delay, cfg->rxd_delay);
    inited = 1;

    _mac_func_leave;
	check_result(rc);


    return rc;
}

int iteMacStop(void)
{
    int rc=0;
    _mac_func_enter;

    if (!dev)
    {
        return rc;
    }

    rc = ftgmac030_close(dev);
    if (rc)
    {
        mac_err("[%s] rc = 0x%08X \n", __FUNCTION__, rc);
    }

    netif_stop(dev);
    inited = 0;

    check_result(rc);
	_mac_func_leave;
    return rc;
}

int iteMacSend(void* packet, uint32_t len)
{
    int rc=0;
    uint16_t vlan_tci = 0;
    mac_enter("[%s] packet %p, len %" PRId32 " \n", __FUNCTION__, packet, len);

    if (!dev)
    {
        return rc;
    }

    if (!NETIF_RUNNING(dev))
    {
        return -1;
    }

    if (!NETIF_CARRIER_OK(dev))
    {
        mac_info(" tx: no carrier \n");
        return -1;
    }

    while(!NETIF_TX_QUEUE_OK(dev)) /* wait for tx queue available */
    {
        printf("&\n");
  	    udelay(50);
    }

    rc = ftgmac030_xmit_frame(packet, len, false, vlan_tci, dev);
    if (rc)
    {
        mac_err("[%s] rc = 0x%08X(%d) \n", __FUNCTION__, rc, rc);
    }

	check_result(rc);
    _mac_func_leave;
    return rc;
}

#if defined(SUPPORT_MAC_VLAN)
int iteMacSendVlan (void * packet, uint32_t len, bool is_vlan, uint16_t vlan_tci)
{
    int rc = 0;
    mac_enter("[%s] packet %p, len %" PRId32 ", vlan_tci %" PRId16 " \n",
              __FUNCTION__, packet, len, vlan_tci);

    if (!dev)
    {
        return rc;
    }

    if (!NETIF_RUNNING(dev))
    {
        return -1;
    }

    if (!NETIF_CARRIER_OK(dev))
    {
        mac_info(" tx: no carrier \n");
        return -1;
    }

    while (!NETIF_TX_QUEUE_OK(dev)) /* wait for tx queue available */
    {
        printf("&\n");
        udelay(50);
    }

    rc = ftgmac030_xmit_frame(packet, len, is_vlan, vlan_tci, dev);
    if (rc)
    {
        mac_err("[%s] rc = 0x%08X(%d) \n", __FUNCTION__, rc, rc);
    }

    check_result(rc);
    _mac_func_leave;
    return rc;
}
#endif // #if defined(SUPPORT_MAC_VLAN)

int iteMacIoctl(struct mii_ioctl_data* data, int cmd)
{
    int rc=0;

    if (!dev)
    {
        return rc;
    }

    rc = ftgmac030_ioctl(dev, data, cmd);
    if (rc)
    {
        mac_err("[%s] rc = 0x%08X \n", __FUNCTION__, rc);
    }

	check_result(rc);

    return rc;
}

int iteMacSetMacAddr(uint8_t* mac_addr)
{
    int rc=0;
    _mac_func_enter;

    if (!dev)
    {
        rc = ERROR_MAC_NO_DEV;
        goto end;
    }
    if (netif_running(dev))
    {
        rc = ERROR_MAC_DEV_BUSY;
        goto end;
    }

    memcpy(dev->netaddr, mac_addr, 6);

end:
    check_result(rc);

    _mac_func_leave;
    return rc;
}

int iteMacSetRxMode(uint32_t flag, uint8_t* mc_list, int count)
{
    int rc=0;
	struct ftgmac030_ctrl *ctrl;
    _mac_func_enter;

    if (!dev)
    {
        rc = ERROR_MAC_NO_DEV;
        goto end;
    }

	ctrl = netdev_priv(dev);

    dev->mc_list = mc_list;
	dev->mc_count = (uint32_t)count;

    if ((flag & (IFF_MULTICAST | IFF_ALLMULTI)) && (ctrl->mode != ITE_ETH_REAL))
    {
        dev->features &= ~NETIF_F_RXALL;
    }

	dev->flags = flag;
    ithEnterCritical();
    ftgmac030_set_rx_mode(dev);
    ithExitCritical();

end:
    check_result(rc);

    _mac_func_leave;
    return rc;
}

int iteMacChangeMtu(int mtu)
{
    int rc=0;
    _mac_func_enter;

    if(!dev)
    {
        rc = ERROR_MAC_NO_DEV;
        goto end;
    }

    ftgmac030_change_mtu(dev, mtu);

end:
    check_result(rc);

    _mac_func_leave;
    return rc;
}

int iteMacGetStats(uint32_t* tx_packets, uint32_t* tx_bytes, uint32_t* rx_packets, uint32_t* rx_bytes)
{
    int rc=0;

    if(!dev)
    {
        rc = ERROR_MAC_NO_DEV;
        goto end;
    }
    if (tx_packets)
    {
        (*tx_packets) = dev->stats.tx_packets;
    }
    if (tx_bytes)
    {
        (*tx_bytes) = dev->stats.tx_bytes;
    }
    if (rx_packets)
    {
        (*rx_packets) = dev->stats.rx_packets;
    }
    if (rx_bytes)
    {
        (*rx_bytes) = dev->stats.rx_bytes;
    }

end:
    check_result(rc);
    return rc;
}

int iteMacSuspend(void)
{
    int rc=0;
	struct ftgmac030_ctrl *ctrl;
    _mac_func_enter;

    if (!dev)
    {
        return rc;
    }

    ctrl = netdev_priv(dev);

	if (netif_running(dev)) 
    {
		int count = FTGMAC030_CHECK_RESET_COUNT;

        while (test_bit(__FTGMAC030_RESETTING, &ctrl->state) && count--)
        {
            usleep(15000);
        }

		WARN_ON(test_bit(__FTGMAC030_RESETTING, &ctrl->state));

		/* Quiesce the device without resetting the hardware */
		ftgmac030_down(ctrl, false);
		ftgmac030_free_irq(ctrl);
	}

    _mac_func_leave;
    return rc;
}

int iteMacResume(void)
{
    int rc=0;
	struct ftgmac030_ctrl *ctrl;
    _mac_func_enter;

    if (!dev)
    {
        return rc;
    }

    ctrl = netdev_priv(dev);

	ftgmac030_reset(ctrl);

	if (netif_running(dev)) 
    {
		ftgmac030_request_irq(ctrl);
		ftgmac030_up(ctrl);
	}

    _mac_func_leave;
    return rc;
}

/* this setting only for fast ethernet (RMII) */
/**
  for RGMII interface:
   param1: 0xFF
   param2: txclk_delay
   param3: rxclk_delay
*/
void iteMacSetClock(uint32_t clk_inv, uint32_t refclk_delay, uint32_t rxd_delay)
{
    struct ftgmac030_ctrl *ctrl = netdev_priv(dev);

    /** for RGMII interface */
    if (clk_inv == 0xFF) 
    {
        uint32_t delay = 0;
        delay |= FTGMAC030_TXCLK_DELAY(refclk_delay);
        delay |= FTGMAC030_RXCLK_DELAY(rxd_delay);
        ithWriteRegMaskA(ctrl->io_base + FTGMAC030_REG_CLKDLY, delay, FTGMAC030_GCLKDLY_MSK);
        return;
    }

    uint32_t val = clk_inv ? FTGMAC030_REFCLK_INV : 0;

    val |= FTGMAC030_REFCLK_DELAY(refclk_delay);
    val |= FTGMAC030_RXD_DELAY(rxd_delay);

    ithWriteRegMaskA(ctrl->io_base + FTGMAC030_REG_CLKDLY, val, FTGMAC030_CLKDLY_MSK);
}

//=============================================================================
//  APIs for alter and report network device settings.
//=============================================================================
/**
 * is link status up/ok
 *
 * Returns 1 if the device reports link status up/ok, 0 otherwise.
 */
int iteEthGetLink(void)
{
    struct ftgmac030_ctrl *ctrl;
    uint32_t new_link;

    if (!dev)
    {
        return 0;
    }

    if (!NETIF_RUNNING(dev))
    {
        return 0;
    }

    ctrl = netdev_priv(dev);

    if ((ctrl->mode == ITE_ETH_MAC_LB) || (ctrl->mode == ITE_ETH_MAC_LB_1000))
    {
        return 1;
    }

#if defined(CFG_NET_ETHERNET_LINK_INTR)
    if (ctrl->cfg->phy_link_status) /* for 8304mb switch, down/up hw here not in isr */
    {
        new_link = ctrl->cfg->phy_link_status();
        processLinkChang((void*)dev, new_link);
    }
#else
	{	// polling from application layer
        if (ctrl->cfg->phy_link_status)
        {
            new_link = ctrl->cfg->phy_link_status();
        }
        else
        {
            new_link = mii_link_ok(&ctrl->mii);
        }

        processLinkChang((void*)dev, new_link);
	}
#endif

    return NETIF_CARRIER_OK(dev) ? 1 : 0;
}

int iteEthGetLink2(void)
{
    struct ftgmac030_ctrl *ctrl;

    if (!dev)
    {
        return 0;
    }

    ctrl = (struct ftgmac030_ctrl *)dev->priv;

    if (ctrl->cfg->phy_link_status)
    {
        return (int)ctrl->cfg->phy_link_status();
    }

    return mii_link_ok(&ctrl->mii);
}
