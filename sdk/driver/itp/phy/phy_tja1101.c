#include <unistd.h>
#include "ite/ite_mac.h"

//#define PHY_HW_RESET
#define REDUCE_OUTPUT_DRIVING

#if defined(PHY_HW_RESET)
#define GPIO_HW_RESET   20
#endif

/* Registers */
#define ECTRL			17
#define ECTRL_LINK_CONTROL		    (0x1 << 15)
#define ECTRL_POWER_MODE_MASK	    (0xF << 11)
#define ECTRL_POWER_MODE_SHT    	    11
#define ECTRL_POWER_MODE_NO_CHANGE	(0x0 << 11)
#define ECTRL_POWER_MODE_NORMAL	    (0x3 << 11)
#define ECTRL_POWER_MODE_STANDBY	    (0xC << 11)
#define ECTRL_LB_MODE_MASK          (0x3 << 3)
#define ECTRL_LB_MODE_SHT           3
#define ECTRL_LB_INTERNAL           (0x0 << 3)
#define ECTRL_LB_EXTERNAL           (0x1 << 3)
#define ECTRL_LB_EXTERNAL1          (0x2 << 3)
#define ECTRL_LB_REMOTE             (0x3 << 3)
#define ECTRL_CONFIG_EN		        (0x1 << 2)
#define ECTRL_WAKE_REQUEST		    (0x1 << 0)

#define CFG1			    18
#define CFG1_MASTER_SLAVE	        (0x1 << 15)
#define CFG1_INTERFACE_MODE_MASK	    (0x3 <<  8)
#define CFG1_INTERFACE_MODE_SHT     8
#define CFG1_MII_MODE               0
#define CFG1_RMII_MODE_REFCLK_IN    1
#define CFG1_RMII_MODE_REFCLK_OUT   2
#define CFG1_REVMII_MODE            3

#define CFG2			    19
#define CFG2_SLEEP_REQUEST_TO	    (0x3 <<  0)
#define CFG2_SLEEP_REQUEST_TO_16MS	0x3

#define INTSRC			21
#define INTSRC_LINK_FAIL		        (0x1 << 10)
#define INTSRC_LINK_UP		        (0x1 << 9)
#define INTSRC_MASK			        (INTSRC_LINK_FAIL | INTSRC_LINK_UP)
#define INTSRC_UV_ERR		        (0x1 << 3)
#define INTSRC_TEMP_ERR		        (0x1 << 1)

#define INTEN			22
#define INTEN_LINK_FAIL		        (0x1 << 10)
#define INTEN_LINK_UP		        (0x1 << 9)
#define INTEN_UV_ERR		            (0x1 << 3)
#define INTEN_TEMP_ERR		        (0x1 << 1)

#define COMMSTAT			23
#define COMMSTAT_LINK_UP		        (0x1 << 15)

#define GENSTAT			24
#define GENSTAT_PLL_LOCKED		    (0x1 << 14)

#define COMMCFG			27
#define COMMCFG_AUTO_OP		        (0x1 << 15)


static struct mii_ioctl_data phy_reg = { CFG_NET_ETHERNET_PHY_ADDR, 0, 0, 0 };

#define phy_reg_write(reg,val)  do { phy_reg.reg_num=(reg); phy_reg.val_write=(val); iteMacIoctl(&phy_reg, IOCSMIIREG); } while (false)

static uint16_t phy_reg_read(uint16_t reg)
{
    phy_reg.reg_num = reg;
    phy_reg.val_read = 0;
    iteMacIoctl(&phy_reg, IOCGMIIREG);
    return phy_reg.val_read;
}

static int tja1101_enable_reg_write(void)
{
    uint16_t val = 0;

    phy_reg_write(ECTRL, phy_reg_read(ECTRL)|ECTRL_CONFIG_EN);
    val = phy_reg_read(ECTRL);
    if (val & ECTRL_CONFIG_EN) {
        return 0;
    }
    else {
        printf("[err][%s] reg %u = 0x%04X \n", __func__, ECTRL, val);
        return 1;
    }
}

static void tja1101_dump_reg(void)
{
    uint16_t reg, val;
    int i;

    printf("\ntja1101 register: \n");
    for (i = 0; i <= 3; i++) {
        val = phy_reg_read(i);
        printf("reg %d: 0x%04X \n", i, val);
    }
    for (i = 15; i <= 28; i++) {
        val = phy_reg_read(i);
        printf("reg %d: 0x%04X \n", i, val);
    }
    printf("\n");
}

#if defined(PHY_HW_RESET)
static void tja1101_hw_reset(void)
{
    ithGpioEnable(GPIO_HW_RESET);
    ithGpioSetOut(GPIO_HW_RESET);
    ithGpioClear(GPIO_HW_RESET);
    ithDelay(15);  // spec. is 5 ~20us. But it seems can't pull low reset pin???
    ithGpioSet(GPIO_HW_RESET);
    printf("phy do hw reset! \n");
}
#endif // #if defined(PHY_HW_RESET)

static int tja1101_sw_reset(void)
{
    uint16_t bmcr;
    int timeout = 2000; // ms

    // Software Reset PHY
    bmcr = phy_reg_read(MII_BMCR);
    bmcr |= BMCR_RESET;
    phy_reg_write(MII_BMCR, bmcr);

    do {
        bmcr = phy_reg_read(MII_BMCR);
        usleep(1000);
    } while ((bmcr & BMCR_RESET) && --timeout);

    if (timeout <= 0) {
        printf("[err] tja1101 phy reset timeout!! \n");
        return -1;
    }

    return 0;
}

static void tja1101_wakeup(void)
{
    uint16_t val, val1 = 0;
    int timeout = 30*1000; // us

    val = phy_reg_read(ECTRL);
    printf("[tja1101] op mode: %u \n", (val & ECTRL_POWER_MODE_MASK) >> ECTRL_POWER_MODE_SHT);

    switch (val & ECTRL_POWER_MODE_MASK) {
    case ECTRL_POWER_MODE_NO_CHANGE:
        break;
    case ECTRL_POWER_MODE_NORMAL:
        phy_reg_write(ECTRL, val | ECTRL_WAKE_REQUEST);
        //phy_reg_write(ECTRL, val & ~ECTRL_WAKE_REQUEST);
        break;
    case ECTRL_POWER_MODE_STANDBY:
        val1 = (val & ~ECTRL_POWER_MODE_MASK) | ECTRL_POWER_MODE_STANDBY;
        phy_reg_write(ECTRL, val1);
        do {
            ithDelay(100);
            timeout -= 100;
            val1 = phy_reg_read(ECTRL) & ECTRL_POWER_MODE_MASK;
        } while ((val1 != ECTRL_POWER_MODE_STANDBY) && (timeout >= 0));
        if (timeout <= 0) {
            printf("[%s] standby mode timeout! \n", __func__);
            return;
        }

        val1 = (val & ~ECTRL_POWER_MODE_MASK) | ECTRL_POWER_MODE_NORMAL;
        phy_reg_write(ECTRL, val1);

        timeout = 30*1000; // us
        do {
            ithDelay(100);
            timeout -= 100;
            val1 = phy_reg_read(GENSTAT) & GENSTAT_PLL_LOCKED;
        } while ((!val1) && (timeout >= 0));

        if (timeout <= 0) {
            printf("[%s] lock timeout! \n", __func__);
            return;
        }
        /* enable link control */
        phy_reg_write(ECTRL, phy_reg_read(ECTRL) & ECTRL_LINK_CONTROL);
        break;
    default:
        break;
    }

    return;
}

static void tja1101_config_intr(void)
{
    /* ACK interrupts by reading the status register */
    // must be read out to reset
    phy_reg_read(INTSRC);

#if defined(CFG_NET_ETHERNET_LINK_INTR)
    {
        uint16_t val = 0;

        val = INTEN_LINK_FAIL | INTEN_LINK_UP | INTEN_UV_ERR | INTEN_TEMP_ERR;
        phy_reg_write(INTEN, val);
    }
#endif

    return;
}

void
PhyInit(int ethMode)
{
    int rc;
    uint16_t cfg1, mode, ext_ctrl = 0;
    char* mii_mode[4] = {
        "MII mode",
        "RMII mode (refclk in)",
        "RMII mode (refclk out)",
        "Revrse MII mode"
    };

    if (ethMode == ITE_ETH_MAC_LB)
        return;

#if defined(PHY_HW_RESET)
    tja1101_hw_reset();
#endif

    rc = tja1101_enable_reg_write();
    if (rc)
        return;

#if defined(REDUCE_OUTPUT_DRIVING)
    // for MII output driver strength
    printf("[tja1101] output driver strength: reduced \n");
    phy_reg_write(18, phy_reg_read(18) | (0x1 << 7));
#endif

    /* phy reset */
    if (tja1101_sw_reset()) {
        printf("[err] tja1101 sw reset fail! \n");
        return;
    }
    printf("[tja1101] after reset: reg %u = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);

    /* check PHY config */
    cfg1 = phy_reg_read(CFG1);
    if (cfg1 & CFG1_MASTER_SLAVE)
        printf("[tja1101] config as Master! \n");
    else
        printf("[tja1101] config as Slave! \n");

    mode = (cfg1 & CFG1_INTERFACE_MODE_MASK) >> CFG1_INTERFACE_MODE_SHT;
    printf("[tja1101] mode: %s \n", mii_mode[mode]);
    if (mode != CFG1_RMII_MODE_REFCLK_OUT) {
        printf("[err] check mode fail!! cfg1:0x%X \n", cfg1);
        return;
    }

#if 0
    tja1101_dump_reg();
    //while (1);
#endif

#if 1
    /* enable autonomous operatio */
    phy_reg_write(COMMCFG, phy_reg_read(COMMCFG)| COMMCFG_AUTO_OP);

    /* set sleep request/ack timeout to 16 ms / 8 ms */
    phy_reg_write(CFG2, (phy_reg_read(CFG2) & ~0x3) | 0x3);

    tja1101_wakeup();

    /* config interrupt */
    tja1101_config_intr();
#endif

    if ((ethMode == ITE_ETH_PCS_LB_100) || (ethMode == ITE_ETH_MDI_LB_100)) {
        uint16_t lb_mode;

        if (ethMode == ITE_ETH_PCS_LB_100) {
            printf(" PHY PCS 100 loopback! \n\n");
            lb_mode = ECTRL_LB_INTERNAL;
        }
        else {
            printf(" PHY MDI 100 loopback! \n\n");
            lb_mode = ECTRL_LB_EXTERNAL;
        }

        /* step 0: switch to manager mode */
        printf(" switch to manager mode. reg 27 D[15] = 0 \n ");
        phy_reg_write(COMMCFG, phy_reg_read(COMMCFG) & ~COMMCFG_AUTO_OP);

        /* step 1: disable link control */
        printf("1: set to normal mode and disable link control \n");
        ext_ctrl = phy_reg_read(ECTRL);
        ext_ctrl = (ext_ctrl & ~ECTRL_POWER_MODE_MASK) | ECTRL_POWER_MODE_NORMAL;
        ext_ctrl &= ~ECTRL_LINK_CONTROL;
        phy_reg_write(ECTRL, ext_ctrl);
        printf("reg %u: 0x%04X \n", ECTRL, phy_reg_read(ECTRL));

        /* step 2: select loopback mode: D[4:3] = 00 internal loopback */
        printf("2: select loopback mode: D[4:3] lb_mode: 0x%X \n", lb_mode);
        ext_ctrl = (ext_ctrl & ~ECTRL_LB_MODE_MASK) | lb_mode;
        phy_reg_write(ECTRL, ext_ctrl);
        printf("reg %u: 0x%04X \n", ECTRL, phy_reg_read(ECTRL));

        /* step 3: enable loopback operation */
        printf("3: enable loopback operation \n");
        phy_reg_write(0, 0x6100);
        printf("reg 0: 0x%04X \n", phy_reg_read(0));

        /* step 4: enable link control */
        printf("4: enable link control \n");
        phy_reg_write(ECTRL, ext_ctrl | ECTRL_LINK_CONTROL);
        printf("reg %u: 0x%04X \n", ECTRL, phy_reg_read(ECTRL));
        usleep(20 * 1000);
    }

    return;
}

/* phy interrupt status definition */

/* in interrupt context */
static int tja1101_link_change(void)
{
    uint16_t intr;

    intr = phy_reg_read(INTSRC);

    if (intr & INTSRC_TEMP_ERR)
        ithPrintf("[tja1101] Overtemperature error detected (temp > 155C°) \n");
    if (intr & INTSRC_UV_ERR)
        ithPrintf("[tja1101] Undervoltage error detected. \n");
    if (intr & INTEN_LINK_FAIL)
        ithPrintf("[tja1101] Link down!!  \n");
    if (intr & INTEN_LINK_UP) {
        ithPrintf("[tja1101] Link up!!  \n");
        return 1;
    }

    return 0;
}

static int tja1101_read_mode(int* speed, int* duplex)
{
    uint16_t link = phy_reg_read(COMMSTAT) & COMMSTAT_LINK_UP;

    *speed = SPEED_100;
    *duplex = DUPLEX_FULL;

    if (link)
        return 0;
    else
        return 1;
}


/**
* Check interrupt status for link change.
* Call from mac driver's internal ISR for phy's interrupt.
*/
int(*itpPhyLinkChange)(void) = tja1101_link_change;
/**
* Replace mac driver's ISR for phy's interrupt.
*/
ITHGpioIntrHandler itpPhylinkIsr = NULL;
/**
* Returns 0 if the device reports link status up/ok
*/
int(*itpPhyReadMode)(int* speed, int* duplex) = tja1101_read_mode;
/**
* Get link status.
*/
uint32_t(*itpPhyLinkStatus)(void) = NULL;
