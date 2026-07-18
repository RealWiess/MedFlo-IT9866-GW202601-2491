/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * definitions for ethernet
 *
 * @author Irene Lin
 * @version 1.0
 */
#ifndef	ITE_ETH_H
#define	ITE_ETH_H


#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                              Constant Definition
//=============================================================================
/* The forced speed, 10Mb, 100Mb, gigabit. */
#define SPEED_10		    10
#define SPEED_100		100
#define SPEED_1000		1000

/* Duplex, half or full. */
#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01

//=============================================================================
//                              Structure Definition
//=============================================================================

struct ethtool_cmd {
	unsigned int	supported;	    /* Features this interface supports */
	unsigned int	advertising;	/* Features this interface advertises */

	unsigned short	speed;		    /* The forced speed, 10Mb, 100Mb, gigabit */
	unsigned char	duplex;		    /* Duplex, half or full */
    unsigned char	port;		    /* Which connector port */
	unsigned char	phy_address;
	unsigned char	transceiver;	/* Which transceiver to use */
	unsigned char	autoneg;	    /* Enable or disable autonegotiation */
	//unsigned char	mdio_support;
	unsigned int	lp_advertising;	/* Features the link partner advertises */
};



#ifdef __cplusplus
}
#endif

#endif

