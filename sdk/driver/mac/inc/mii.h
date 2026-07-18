#ifndef	MII_H
#define	MII_H


#ifdef __cplusplus
extern "C" {
#endif

#include "ite/ite_eth.h" 
#include "ite/ite_mii.h" 


struct mii_if_info {
    unsigned short phy_id;
    unsigned short phy_id_mask;
    unsigned short reg_num_mask;
    unsigned short reserved;

    struct eth_device *dev;
    int (*mdio_read) (struct eth_device *dev, int phy_id, int reg);
    void (*mdio_write) (struct eth_device *dev, int phy_id, int reg, int val);
};

extern int mii_link_ok (struct mii_if_info *mii);
extern int generic_mii_ioctl(struct mii_if_info *mii_if, struct mii_ioctl_data *mii_data, int cmd);


#ifdef __cplusplus
}
#endif

#endif
