#include <unistd.h>
#include <inttypes.h>
#include "ite/ite_mac.h"


#define CLK_FROM_PHY
#define PHY_INTR_EN

#if 0 // for reference
ITE_ETH_REAL = 1,
ITE_ETH_MAC_LB = 2,
ITE_ETH_PCS_LB_10 = 3,
ITE_ETH_PCS_LB_100 = 4,
ITE_ETH_MDI_LB_10 = 5,
ITE_ETH_MDI_LB_100 = 6
#endif



static struct mii_ioctl_data phy_reg = {0, 0, 0, 0};

#define phy_set_page(page)      do { phy_reg.reg_num=31; phy_reg.val_write=page; iteMacIoctl(&phy_reg, IOCSMIIREG); } while (false)
#define phy_reg_read(reg)       do { phy_reg.reg_num=(reg); phy_reg.val_read=0; iteMacIoctl(&phy_reg, IOCGMIIREG); } while (false)
#define phy_reg_write(reg,val)  do { phy_reg.reg_num=(reg); phy_reg.val_write=(val); iteMacIoctl(&phy_reg, IOCSMIIREG); } while (false)
#define phy_reset()             do { phy_reg.reg_num=0; phy_reg.val_write=0x9200; iteMacIoctl(&phy_reg, IOCSMIIREG); } while (false)


#define ID_RTL8201  0x00000732
#define ID_SR8201   0x00000732 /* all design(include UID) same with RTL8201 */
#define ID_JL1101   0x0024DF10
#define ID_SZ18201  0x00000000
#define ID_YT8512   0x00000000 /* same with SZ18201. it seems new version of SZ18201 */

static uint32_t phy_id = 0;
static void (*phy_intr_enable)(void) = NULL;
static void (*phy_init)(int) = NULL;
static int (*phy_link_change)(void) = NULL;
static int(*phy_read_mode)(int*, int*) = NULL;

static int sz18201_link_change(void);
static int sz18201_read_mode(int* speed, int* duplex);

#if defined(PHY_INTR_EN)
static void rtl8201_intr_enable(void)
{
    //phy_reg_read(30); /** clear interrupt */
    phy_set_page(7);
    phy_reg_read(19);
    //printf(" P7 reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_reg.val_read &= ~0x30;  /** LED selection D[5:4]=00 */
    phy_reg_write(19, (phy_reg.val_read | (0x2000)));
    phy_reg_read(19);
    //printf(" P7 reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_set_page(0);
}

static void JL1101_intr_enable(void)
{
    // enable interrupt: pull high interrupt pin
    //if (phy_id == ID_JL1101)
    {
        phy_set_page(128);
        phy_reg_read(29);
        phy_reg.val_read |= (0x1 << 7);
        phy_reg_write(29, phy_reg.val_read);
    }

    //phy_reg_read(30); /** clear interrupt */
    phy_set_page(7);
    phy_reg_read(19);
    //printf(" P7 reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_reg.val_read &= ~0x30;  /** LED selection D[5:4]=00 */
    phy_reg_write(19, (phy_reg.val_read | (0x2000)));
    phy_reg_read(19);
    //printf(" P7 reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_set_page(0);
}

static void sz18201_intr_enable(void)
{
    phy_reg_read(0x12);
    phy_reg.val_read |= 0xEC00;
    phy_reg_write(0x12, phy_reg.val_read);
    phy_reg_read(0x12);
    //printf(" reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
}
#endif

static void sz18201_PhyInit(int ethMode)
{
    /* enable phy interrupt */
    if (phy_intr_enable)
        phy_intr_enable();

    if (ethMode == ITE_ETH_PCS_LB_10) {
        printf(" PHY PCS 10 loopback! \n\n");
        phy_reg_write(0, 0x4100);
    }
    else if (ethMode == ITE_ETH_PCS_LB_100) {
        printf(" PHY PCS 100 loopback! \n\n");
        phy_reg_write(0, 0x6100);
        usleep(20 * 1000);
    }
    else if (ethMode == ITE_ETH_MDI_LB_10) {
        printf(" PHY MDI 10 loopback! \n\n");
        phy_reg_write(0, 0x0100);
        usleep(100 * 1000);
    }
    else if (ethMode == ITE_ETH_MDI_LB_100) {
        printf(" PHY MDI 100 loopback! \n\n");
        phy_reg_write(0, 0x2100);
        usleep(100 * 1000);
    }
}

static void phy_read_id(void)
{
    phy_reg_read(2);
    printf("reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_id = phy_reg.val_read << 6;
    phy_reg_read(3);
    printf("reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_id |= ((phy_reg.val_read >> 10) & 0x3F);
    printf("PHY ID: 0x%08" PRIX32 " \n", phy_id);

    printf("\n");
    if (phy_id == ID_RTL8201)
        printf("@@@ PHY: RTL8201 or SR8201 @@@ \n");
    else if (phy_id == ID_JL1101)
        printf("@@@ PHY: JL1101 @@@ \n");
    else if (phy_id == ID_SZ18201)
        printf("@@@ PHY: SZ18201 or YT8512 @@@ \n");
    else
        printf("@@@ Unknown PHY!!!! @@@\n");
    printf("\n");

#if defined(PHY_INTR_EN)
    if (phy_id == ID_RTL8201)
        phy_intr_enable = rtl8201_intr_enable;
    else if (phy_id == ID_JL1101)
        phy_intr_enable = JL1101_intr_enable;
    else if (phy_id == ID_SZ18201)
    {
        phy_intr_enable = sz18201_intr_enable;
        phy_init = sz18201_PhyInit;
        phy_link_change = sz18201_link_change;
        phy_read_mode = sz18201_read_mode;
    }
#endif

    return;
}


void
PhyInit(int ethMode)
{
    uint16_t tmp;
    uint32_t timeout;

    if (ethMode == ITE_ETH_MAC_LB)
        return;

    phy_reg.phy_id = CFG_NET_ETHERNET_PHY_ADDR;

    phy_read_id();

    /* phy reset */
    phy_reset();
    timeout = 2000; // ms
    do {
        phy_reg_read(0);
        usleep(1000);
    } while((phy_reg.val_read & 0x8000) && timeout--);
    printf(" after reset: phy reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
	if (!timeout)
		printf("phy reset timeout! \n");

    /* sz18201 is different design */
    if (phy_init)
        return phy_init(ethMode);

    /** CRS can't toggle for RMII */
    phy_set_page(7);
    phy_reg_read(16);
    #if defined(CLK_FROM_PHY)
    phy_reg_write(16, (phy_reg.val_read|(0x1<<2)));
    #else
    phy_reg_write(16, (phy_reg.val_read|((0x1<<12)|(0x1<<2))));
    #endif
    phy_reg_read(16);
    //printf(" phy P7 reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    phy_set_page(0);

    /* enable phy interrupt */
    if (phy_intr_enable)
        phy_intr_enable();

    if (ethMode == ITE_ETH_PCS_LB_10) {
        printf(" PHY PCS 10 loopback! \n\n");
        phy_set_page(0);
        phy_reg_write(0, 0x4100);
        phy_set_page(7);
        tmp = 0x1FF8;
        #if defined(CLK_FROM_PHY)
        tmp &= ~(0x1 << 12);
        #endif
        phy_reg_write(16, tmp);
        phy_set_page(0);
        usleep(20 * 1000);
    }
    else if(ethMode == ITE_ETH_PCS_LB_100) {
        printf(" PHY PCS 100 loopback! \n\n");
        phy_set_page(0);
        phy_reg_write(0, 0x6100);
        phy_set_page(7);
        tmp = 0x1FF8;
        #if defined(CLK_FROM_PHY)
        tmp &= ~(0x1 << 12);
        #endif
        phy_reg_write(16, tmp);
        phy_set_page(0);
        usleep(20 * 1000);
    }
    else if (ethMode == ITE_ETH_MDI_LB_10) {
        printf(" PHY MDI 10 loopback! \n\n");
        phy_set_page(0);
        phy_reg_write(0, 0x0100);
        usleep(100 * 1000);
    }
    else if (ethMode == ITE_ETH_MDI_LB_100) {
        printf(" PHY MDI 100 loopback! \n\n");
        phy_set_page(0);
        phy_reg_write(0, 0x2100);
        usleep(100 * 1000);
    }
}

/* phy interrupt status definition */
#define MII_INTRSTS         0x1e        /* Interrupt Indicators   */
#define INTR_LINKCHG		0x0800
#define INTR_DUPLEXCHG      0x2000
#define INTR_SPEEDCHG		0x4000
#define INTR_ANERR			0x8000
#define INTR_MSK			0xE800

/* in interrupt context */
static int rtl8201_link_change(void)
{
    uint32_t intr;

    if (phy_link_change)
        return phy_link_change();

    phy_set_page(0);
    phy_reg_read(0x1e);
    intr = phy_reg.val_read & INTR_MSK;

#if 0
    if (intr & INTR_DUPLEXCHG)
        ithPrintf("phy: duplex change! \n");
    if (intr & INTR_SPEEDCHG)
        ithPrintf("phy: speed change! \n");
#endif
    if (intr & INTR_ANERR)
        ithPrintf("phy: ANERR! \n");
    if (intr & INTR_LINKCHG) {
        ithPrintf("phy: link change! \n");
        return 1;
    }

    return 0;
}


/*=== FOR SZ18201 ===>>*/
/* phy interrupt status definition */
#define SZ_INTR_ANERR		0x8000
#define SZ_INTR_SPEEDCHG		0x4000
#define SZ_INTR_DUPLEXCHG   0x2000
#define SZ_INTR_LINK_DOWN   0x0800
#define SZ_INTR_LINK_UP     0x0400
#define SZ_INTR_MSK			0xEC00

/* in interrupt context */
static int sz18201_link_change(void)
{
    uint32_t intr;

    phy_reg_read(0x13);
    //printf(" reg %d = 0x%04X \n", phy_reg.reg_num, phy_reg.val_read);
    intr = phy_reg.val_read & SZ_INTR_MSK;

#if 0
    if (intr & SZ_INTR_DUPLEXCHG)
        ithPrintf("phy: duplex change! \n");
    if (intr & SZ_INTR_SPEEDCHG)
        ithPrintf("phy: speed change! \n");
#endif
    if (intr & SZ_INTR_ANERR)
        ithPrintf("phy: ANERR! \n");
    if (intr & SZ_INTR_LINK_DOWN) {
        ithPrintf("phy: link down! \n");
        return 1;
    }
    if (intr & SZ_INTR_LINK_UP) {
        ithPrintf("phy: link up! \n");
        return 1;
    }

    return 0;
}
/*<<=== FOR SZ18201 ===*/

static int rtl8201_read_mode(int* speed, int* duplex)
{
    if (phy_read_mode)
        return phy_read_mode(speed, duplex);

    *speed = SPEED_10;
    *duplex = DUPLEX_HALF;

    phy_reg_read(1);
    phy_reg_read(1);
    if (!(phy_reg.val_read & 0x4))
        return -1; // link is down

    phy_reg_read(0);
    if (phy_reg.val_read & 0x2000)
        *speed = SPEED_100;
    if (phy_reg.val_read & 0x100)
        *duplex = DUPLEX_FULL;

    return 0; // link is up
}

static int sz18201_read_mode(int* speed, int* duplex)
{
    *speed = SPEED_10;
    *duplex = DUPLEX_HALF;

    phy_reg_read(17);
    if (!(phy_reg.val_read & (0x1 << 10)))
        return -1; // link is down

    if (((phy_reg.val_read >> 14) & 0x3) == 0x2)
        *speed = SPEED_1000;
    else if (((phy_reg.val_read >> 14) & 0x3) == 0x1)
        *speed = SPEED_100;
    else
        *speed = SPEED_10;

    if (phy_reg.val_read & (0x1 << 13))
        *duplex = DUPLEX_FULL;

    return 0; // link is up
}


/**
* Check interrupt status for link change.
* Call from mac driver's internal ISR for phy's interrupt.
*/
int(*itpPhyLinkChange)(void) = rtl8201_link_change;
/**
* Replace mac driver's ISR for phy's interrupt.
*/
ITHGpioIntrHandler itpPhylinkIsr = NULL;
/**
* Returns 0 if the device reports link status up/ok
*/
int(*itpPhyReadMode)(int* speed, int* duplex) = rtl8201_read_mode;
/**
* Get link status.
*/
uint32_t(*itpPhyLinkStatus)(void) = NULL;
