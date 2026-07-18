#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "mii.h"
#include "ndis.h"
#include "gmac/gmac_types.h"


#define netif_carrier_ok    NETIF_CARRIER_OK
#define netif_carrier_on    NETIF_CARRIER_ON
#define netif_carrier_off   NETIF_CARRIER_OFF
#define EINVAL              (-77)


/**
 * link status
 * @mii: the MII interface
 *
 * Returns 1 if link up, 0 otherwise.
 */
int mii_link_ok (struct mii_if_info *mii)
{
    int val;

    val = mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR); /* a dummy read, needed to latch some MII phys */
    if (val < 0)
    {
        return 0; // I/O fail
    }
    val = mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR);
    if ((unsigned short)val & BMSR_LSTATUS)
    {
        return 1;
    }
    return 0;
}

/**
 * MII ioctl interface
 * @mii_if: the MII interface
 * @mii_data: MII ioctl data structure
 * @cmd: MII ioctl command
 *
 * Returns 0 on success, negative on error.
 */
int generic_mii_ioctl(struct mii_if_info *mii_info, struct mii_ioctl_data *data, int cmd)
{
    int rc = 0, val;

    data->phy_id &= mii_info->phy_id_mask;
    data->reg_num &= mii_info->reg_num_mask;

    switch(cmd) 
    {
    case IOCGMIIPHY:
        data->phy_id = mii_info->phy_id;
        /* fall through */

    case IOCGMIIREG:
        val = mii_info->mdio_read(mii_info->dev, data->phy_id, data->reg_num);
        if (val < 0)
        {
            rc = -2;
        }
        data->val_read = (unsigned short)val;
        break;

    case IOCSMIIREG: 
        mii_info->mdio_write(mii_info->dev, data->phy_id, data->reg_num, data->val_write);
        break;

    default:
        rc = -1;
        break;
    }

    if (rc)
    {
        printf("[%s] rc = %d \n", __FUNCTION__, rc);
    }
    return rc;
}

