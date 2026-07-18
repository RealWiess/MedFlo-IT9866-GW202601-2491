

static sem_t isr_sem;

#define BUDGET		(RX_QUEUE_ENTRIES*3/4)
/**
* ftgmac030_poll - NAPI Rx polling callback
* @napi: struct associated with this polling callback
* @weight: number of packets driver is allowed to process this poll
**/
static int ftgmac030_poll(struct eth_device *netdev)
{
    struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
    int tx_cleaned = 1, work_done = 0;
    u32 status;

    status = ior32(FTGMAC030_REG_ISR);
    iow32(FTGMAC030_REG_ISR, status);

#if 1 // add for busy & link down
    if (test_bit(__FTGMAC030_RESETTING, &ctrl->state)) 
    {
        e_warn(intr, " poll: resetting... \n");
        return -1;
    }
    set_bit(__FTGMAC030_ACCESS_SHARED_RESOURCE, &ctrl->state);
#endif

    tx_cleaned = ftgmac030_clean_tx_irq(ctrl->tx_ring, status); /* 1: tx finished, 0: tx full */

    if (status & (FTGMAC030_INT_RPKT_BUF | FTGMAC030_INT_NO_RXBUF |
        FTGMAC030_INT_RX_TMSP_VALID))
    {
        ctrl->clean_rx(ctrl->rx_ring, &work_done, BUDGET);
    }

#if 1 // add for busy & link down
    clear_bit(__FTGMAC030_ACCESS_SHARED_RESOURCE, &ctrl->state);
#endif

    if (!tx_cleaned)
    {
        work_done = BUDGET;
    }

    if (status & (FTGMAC030_INT_NO_RXBUF | FTGMAC030_INT_RPKT_LOST |
        FTGMAC030_INT_AHB_ERR)) 
    {
        if (1)
        {
            e_warn(intr, " 0x%x: %s%s%s\n", status,
                status & FTGMAC030_INT_NO_RXBUF ?
                "NO_RXBUF " : "",
                status & FTGMAC030_INT_RPKT_LOST ?
                "RPKT_LOST " : "",
                status & FTGMAC030_INT_AHB_ERR ?
                "AHB_ERR " : "");
        }
        if (status & FTGMAC030_INT_NO_RXBUF) 
        {
            /* RX buffer unavailable */
            netdev->stats.rx_over_errors++;
        }

        if (status & FTGMAC030_INT_RPKT_LOST) 
        {
            /* received packet lost due to RX FIFO full */
            netdev->stats.rx_fifo_errors++;
        }
    }

    /* If weight not fully consumed, exit the polling mode */
    if (work_done < BUDGET) 
    {  
        if (ctrl->itr_setting & 3)
        {
            ftgmac030_set_itr(ctrl);
        }

        if (!test_bit(__FTGMAC030_DOWN, &ctrl->state))
        {
            ftgmac030_irq_enable(ctrl);
        }
    }
	else 
    {
		/* tx full or rx reach BUDGET */
		e_warn(intr, "many rx packets!! \n");
		sem_post(&isr_sem);
	}
		 

    return work_done;
}

/**
* ftgmac030_intr - Interrupt Handler
* @irq: interrupt number
* @data: pointer to a network interface device structure
**/
static void ftgmac100_interrupt(void *arg)
{
    struct eth_device *netdev = arg;
    struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

    ftgmac030_irq_disable(ctrl);

    //e_dbg(intr, "[ISR] = 0x%x\n", ior32(FTGMAC030_REG_ISR));

    if (test_bit(__FTGMAC030_DOWN, &ctrl->state)) 
    {
        e_err(intr, "[ISR] Not ours\n");
        return;	/* Not our interrupt */
    }

    ctrl->total_tx_bytes = 0;
    ctrl->total_tx_packets = 0;
    ctrl->total_rx_bytes = 0;
    ctrl->total_rx_packets = 0;
    sem_post(&isr_sem);
}

static volatile int task_done = 0;

void* iteMacThreadFunc(void* data)
{
    sem_init(&isr_sem, 0, 0);
    task_done = 1;

wait:
    sem_wait(&isr_sem);
    if (dev)
    {
        ftgmac030_poll(dev);
    }
    goto wait;
}

void processLinkChang(void* arg, uint32_t new_link)
{
    struct eth_device *netdev = (struct eth_device *)arg;
    struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	uint32_t prev_link = NETIF_CARRIER_OK(netdev);
	int speed, duplex;
	
	phy_read_mode(netdev, &speed, &duplex);
	
    if ((!!prev_link == !!new_link) &&
        (ctrl->phy_duplex == duplex) &&
        (ctrl->phy_speed == speed))
    {
        return;
    }
	
	printf("new speed %d, old speed %d, link %s\n",
		speed, ctrl->phy_speed, new_link ? "up" : "down");
	
#if 1 // add for busy & link down
    test_and_set_bit(__FTGMAC030_RESETTING, &ctrl->state);

    while (test_bit(__FTGMAC030_ACCESS_SHARED_RESOURCE, &ctrl->state))
    {
        usleep(2 * 1000);
    }
#endif

    if (!test_bit(__FTGMAC030_DOWN, &ctrl->state))
    {
        ftgmac030_down(ctrl, true);
    }
	
	ctrl->phy_speed = speed;
	ctrl->phy_duplex = duplex;
	ctrl->phy_link = new_link;
	
	if (new_link) 
    {
		ftgmac030_up(ctrl);
		NETIF_CARRIER_ON(netdev);
	}
    else
    {
        NETIF_CARRIER_OFF(netdev);
    }

#if 1 // add for busy & link down
    clear_bit(__FTGMAC030_RESETTING, &ctrl->state);
#endif
}

static void linkIntrHandler(unsigned int pin, void *arg)
{
    struct eth_device *netdev = (struct eth_device *)arg;
    struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

    //ithPrintf(" gpio isr \n");

    if (pin != (unsigned int)ctrl->cfg->linkGpio)
    {
        ithPrintf(" MAC: phy link gpio error! %d \n", pin);
        return;
    }

    if (!ctrl->cfg->phy_link_change) 
    {
        ithPrintf("phy_link_change callback is NULL! Please implement it! \n");
        return;
    }

    if (netif_running(netdev) && ctrl->cfg->phy_link_change()) 
    {
        int new_link = mii_link_ok(&ctrl->mii);

		itpPendFunctionCallFromISR(processLinkChang, arg, (uint32_t)new_link);
    }

    return;
}

/**
* ftgmac030_request_irq - initialize interrupts
*
* Attempts to configure interrupts using the best available
* capabilities of the hardware and kernel.
**/
static void ftgmac030_request_irq(struct ftgmac030_ctrl *ctrl)
{
    struct eth_device *netdev = ctrl->netdev;
    int link_gpio = ctrl->cfg->linkGpio;

    /** register interrupt handler to interrupt mgr */
    ithIntrRegisterHandlerIrq(ITH_INTR_MAC, ftgmac100_interrupt, (void*)netdev);
    ithIntrSetTriggerModeIrq(ITH_INTR_MAC, ITH_INTR_LEVEL);
    ithIntrSetTriggerLevelIrq(ITH_INTR_MAC, ITH_INTR_HIGH_RISING);
    ithIntrEnableIrq(ITH_INTR_MAC);

#if defined(CFG_NET_ETHERNET_LINK_INTR)
	ithPrintf(" link gpio %d \n", link_gpio);
    if (link_gpio > 0)
    {
        unsigned int gpio = (unsigned int)link_gpio;
        /** register phy link up/down interrupt handler to gpio mgr, GPIO */
        ithGpioEnable(gpio);
        ithGpioSetIn(gpio);
        ithGpioClearIntr(gpio);
        if (ctrl->cfg->linkGpio_isr)
        {
            ithGpioRegisterIntrHandler(gpio, ctrl->cfg->linkGpio_isr, (void*)netdev);
        }
        else
        {
            ithGpioRegisterIntrHandler(gpio, linkIntrHandler, (void*)netdev);
        }
        ithGpioCtrlEnable(gpio, ITH_GPIO_INTR_LEVELTRIGGER);  /* use level trigger mode */
        ithGpioCtrlEnable(gpio, ITH_GPIO_INTR_TRIGGERFALLING); /* low active */
        ithGpioEnableIntr(gpio);
    }
#endif

    if (!task_done)
    {
        mac_err(" Task not create!!! \n\n");
    }
}

static void ftgmac030_free_irq(struct ftgmac030_ctrl *ctrl)
{
    ithIntrDisableIrq(ITH_INTR_MAC);

    if (ctrl->cfg->linkGpio > 0)
    {
        ithGpioDisableIntr((unsigned int)ctrl->cfg->linkGpio);
    }
}
