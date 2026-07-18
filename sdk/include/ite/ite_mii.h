/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * definitions for MII-compatible transceivers
 *
 * @author Irene Lin
 * @version 1.0
 */
#ifndef	ITE_MII_H
#define	ITE_MII_H

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//                              Constant Definition
//=============================================================================

/* Generic MII registers. */

/* Basic mode control register */
#define MII_BMCR                0x00    
/* Basic mode status register */
#define MII_BMSR                0x01    
/* PHYS ID 1 */
#define MII_PHYSID1             0x02    
/* PHYS ID 2 */
#define MII_PHYSID2             0x03
/* MSB of Speed (1000) */
#define BMCR_SPEED1000		    0x0040
/* Full duplex */
#define BMCR_FULLDPLX           0x0100
/* Enable auto negotiation */
#define BMCR_ANENABLE           0x1000  
/* Select 100Mbps */
#define BMCR_SPEED100           0x2000 
/* TXD loopback bits */
#define BMCR_LOOPBACK           0x4000  
/* Reset the DP83840 */
#define BMCR_RESET              0x8000  
/* Link status */
#define BMSR_LSTATUS            0x0004U  


/* These definitions are ioctl codes for MII PHY use. */
#define IOCGMIIPHY	0x9001		/* Get address of MII PHY in use. */
#define IOCGMIIREG	0x9002		/* Read MII PHY register.	*/
#define IOCSMIIREG	0x9003		/* Write MII PHY register.	*/


//=============================================================================
//                              Structure Definition
//=============================================================================

/* This structure is used in all IOCxMIIxxx ioctl calls */
struct mii_ioctl_data {
    unsigned short		phy_id;
    unsigned short		reg_num;
    unsigned short		val_write;
    unsigned short		val_read;
};



#ifdef __cplusplus
}
#endif

#endif

