/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Ethernet Driver API header file.
 *
 * @author Irene Lin
 * @version 1.0
 */
#ifndef ITE_MAC_H
#define ITE_MAC_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * DLL export API declaration for Win32.
 */
#define MAC_API extern


//=============================================================================
//                              Include Files
//=============================================================================
#include <net/if.h>
#include "ite/ith.h"
#include "ite_mii.h"
#include "ite_eth.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define ITE_ETH_REAL         	1
#define ITE_ETH_MAC_LB          2
#define ITE_ETH_PCS_LB_10       3
#define ITE_ETH_PCS_LB_100      4
#define ITE_ETH_PCS_LB_1000     5
#define ITE_ETH_MDI_LB_10       6
#define ITE_ETH_MDI_LB_100      7
#define ITE_ETH_MDI_LB_1000     8
#define ITE_ETH_MAC_LB_1000     9

#define ITE_MAC_RMII_PIN_CNT	10
#define ITE_MAC_GRMII_PIN_CNT	15

//=============================================================================
//                              Macro Definition
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct ITE_MAC_CFG_S {
#define ITE_MAC_RGMII	(0x1 << 0)
    uint32_t flags;
    uint32_t clk_inv;
    uint32_t clk_delay;
    uint32_t rxd_delay;
    const uint8_t* ioConfig;
    int phyAddr;
    int linkGpio;

	/**
	* Replace mac driver's ISR for phy's interrupt. 
	*/
    ITHGpioIntrHandler linkGpio_isr;
	
	/**
	* Returns 0 if the device reports link status up/ok 
	*/
    int (*phy_read_mode)(int* speed, int* duplex);
    uint32_t (*phy_link_status)(void);  /* get link status */
	
	/**
	* Check interrupt status for link change. 
	* Call from mac driver's internal ISR for phy's interrupt.
	*/
    int (*phy_link_change)(void);
} ITE_MAC_CFG_T;

//=============================================================================
//                              Function Declaration
//=============================================================================
/** @defgroup group15 ITE Ethernet Driver API
 *  The Ethernet module API.
 *  @{
 */
#if defined(CFG_NET_ETH_HW_VLAN)
typedef void (*ITE_MAC_RX_CB)(void * arg, void * packet, int len, bool is_vlan, uint16_t vlan_tci);
#else
typedef void (*ITE_MAC_RX_CB)(void * arg, void * packet, int len);
#endif

/**
 * Ethernet Controller Initialization.
 *
 * @param phyAddr   PHY's address
 * @param linkGpio  GPIO pin number for phy's interrupt
 * @param ioConfig  GPIO pin number for mac's I/O
 *
 * @return 0 if succeed.
 */
MAC_API int iteMacInitialize(ITE_MAC_CFG_T* cfg);


/**
 * This function is called when network device transistions to the up state.
 *
 * @param mac_addr  the buffer to receive the mac address
 * @param func      the callback for push packet to protocol stack
 * @param arg       the input parameter of callback
 *
 * @return 0 if succeed.
 */
MAC_API int  iteMacOpen (uint8_t * mac_addr, ITE_MAC_RX_CB rx_cb, void * arg,
                         int mode);


/**
 * This function is called when network device transistions to the down state.
 *
 * @return 0 if succeed.
 */
MAC_API int iteMacStop(void);


/**
 * Called when a packet needs to be transmitted.
 *
 * @param packet    the packet data want to transmitted
 * @param len       the packet length
 *
 * @return 0 if succeed.
 */
MAC_API int iteMacSend(void* packet, uint32_t len);

/**
 * Called when a packet needs to be transmitted with VLAN tag.
 *
 * @param packet    the packet data want to transmitted
 * @param len       the packet length
 * @param vlan_tci  the VLAN tag control information
 *
 * @return 0 if succeed.
 */
MAC_API int  iteMacSendVlan (void * packet, uint32_t len, bool is_vlan, uint16_t vlan_tci);

/**
 *  This function is called device changes address list filtering.
 *
 * @param flag      see <if.h> IFF_PROMISC, IFF_ALLMULTI, IFF_MULTICAST
 * @param mc_list   multicast mac address list. Byte 0~ 5 for first multicast mac address
 *                  and byte 6~11 for second multicast mac address, and so on...
 * @count           the count of multicast mac addresses
 */
MAC_API int iteMacSetRxMode(uint32_t flag, uint8_t* mc_list, int count);

/**
 *  Change the Maximum Transfer Unit. Only support for Giga Ethernet.
 *
 * @new_mtu: new value for maximum frame size
 *
 * @return 0 if succeed.
 */
MAC_API int iteMacChangeMtu(int new_mtu);

/**
 *  Called when a user request an ioctl which can't be handled by
 *  the generic interface code. If not defined ioctl's return
 *  not supported error code.
 *
 * @param data      the ioctl data
 * @param cmd       see IOCxMIIxxx ioctl calls
 *
 * @return 0 if succeed.
 */
MAC_API int iteMacIoctl(struct mii_ioctl_data* data, int cmd);

/**
 * set new Ethernet hardware address
 * @param mac_addr  hardware address of device
 *
 * This doesn't change hardware matching, so needs to be overridden
 * for most real devices.
 */
MAC_API int iteMacSetMacAddr(uint8_t* mac_addr);

MAC_API int iteMacGetStats(uint32_t* tx_packets, uint32_t* tx_bytes, uint32_t* rx_packets, uint32_t* rx_bytes);

MAC_API int iteMacSuspend(void);

MAC_API int iteMacResume(void);

MAC_API void iteMacSetClock(uint32_t clk_inv, uint32_t refclk_delay, uint32_t rxd_delay);  /* just for test clock window, only for fast ethernet */

//=============================================================================
//  APIs for alter and report network device settings.
//=============================================================================
/**
 * is link status up/ok
 *
 * @return 1 if the device reports link status up/ok, 0 otherwise.
 */
MAC_API int iteEthGetLink(void);
MAC_API int iteEthGetLink2(void);


MAC_API void* iteMacThreadFunc(void* data);



//@}

#ifdef __cplusplus
}
#endif

#endif /* ITE_MAC_H */

