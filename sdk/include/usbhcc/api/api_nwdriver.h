/***************************************************************************
 *
 *            Copyright (c) 2012-2021 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef API_NWDRIVER_H
#define API_NWDRIVER_H


/************************************************************************
*  Include section
*************************************************************************/
#include "../psp/include/psp_types.h"

#include "../version/ver_nwdriver.h"
#if ( VER_NWDRIVER_MAJOR != 5 ) || ( VER_NWDRIVER_MINOR != 8 )
 #error Incompatible NETWORK driver version number!
#endif
#include "../version/ver_psp_types.h"
#if VER_PSP_TYPES_MAJOR != 1
 #error Incompatible PSP_TYPES version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************
*  Defines section
*************************************************************************/

/* error codes */
#define NWDRIVER_SUCCESS              0
#define NWDRIVER_INVALID              1
#define NWDRIVER_QUEUE_EMPTY          2
#define NWDRIVER_QUEUE_FULL           3
#define NWDRIVER_ERROR                4

/* network driver options */
/* HW checksum calculation enabled */
#define NWDRIVER_OPT_HW_CHECKSUM      1u

/* HW filtering enabled */
#define NWDRIVER_OPT_HW_FILTER        2u

/* HW address size */
#define NWDRIVER_HW_ADDRESS_SIZE      6u

/* Multicast HW address */
#define NWDRIVER_MULTICAST_HW_SIZE    3u
#define NWDRIVER_MULTICAST_HW_IP_MASK 0x007fffffu

/* null and broadcast HW address */
#define NWDRIVER_NULL_HW_ADDRESS      { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u }
#define NWDRIVER_BROADCAST_HW_ADDRESS { 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu }
#define NWDRIVER_MULTICAST_HW_ADDRESS { 0x01u, 0x00u, 0x5eu, 0x00u, 0x00u, 0x00u }

/* default MTU size */
#define NWDRIVER_DEF_MTU_SIZE         1514u
#define NWDRIVER_VLAN_HEADER_SIZE     4u

/* filtering options */
#define NWDRIVER_FILTER_PERFECT       0x01u
#define NWDRIVER_FILTER_BROADCAST     0x02u
#define NWDRIVER_FILTER_MULTICAST     0x04u
#define NWDRIVER_FILTER_MULTICAST_TBL 0x08u
#define NWDRIVER_FILTER_PROMISCUOUS   0x10u
#define NWDRIVER_FILTER_MULTICASTALL  0x20u

/* NW driver parameter */
#define NWDRIVER_PARAM_VLAN_ENABLE    0x80000000u

/* 340 S : MISRA-C:2012/AMD1/TC1 D.4.9: Use of function like macro. */
/*LDRA_INSPECTED 340 S*/
#define NWDRIVER_PARAM_POS( v )       ( ( v ) & 0xffu )


/***** Options (option parameter) for p_nwfn_set_option() and p_nwfn_get_option() *****/
#define NWDRIVER_OPT_SUPPORTED_FUN          1U  /* gets supported functionalities (value: uint32_t, flags NWDRIVER_SUP_FUN_*) */
#define NWDRIVER_OPT_ADVERTISED_FUN         2U  /* gets advertised functionalities (value: uint32_t, flags NWDRIVER_SUP_FUN_*) */
#define NWDRIVER_OPT_TIMESTAMP_SUP          3U  /* supported timestamp options(value: uint32_t, flags NWDRIVER_TIMESTAMP_*) */
#define NWDRIVER_OPT_TIMESTAMP_EN           4U  /* timestamp enable (value: uint32_t, flags NWDRIVER_TIMESTAMP_*) */
#define NWDRIVER_OPT_TIMESTAMP_HW_FIL       5U  /* timestamp filtering mode ( value: t_nw_ts_filter) */
#define NWDRIVER_OPT_GET_RX_TIMESTAMP       6U  /* get receive timestamp (value: t_nw_timestamp) */
#define NWDRIVER_OPT_GET_TX_TIMESTAMP       7U  /* get transmit timestamp (value: t_nw_timestamp) */
#define NWDRIVER_OPT_DUPLEX_MODE            8U  /* get duplex mode ( value: uint8_t, NWDRIVER_DUPLEX_* ) */
#define NWDRIVER_OPT_PORT_TYPE              9U  /* get port type ( value: uint8_t, NWDRIVER_PORT_* ) */
#define NWDRIVER_OPT_MDI_STATUS             10U /* get twisted pair MDI status ( value: uint8_t, NWDRIVER_ETH_TP_MDI* ) */
#define NWDRIVER_OPT_MDI_CTRL               11U /* get twisted pair MDI control ( value: uint8_t, NWDRIVER_ETH_TP_MDI* ) */

/* Driver supported and advertised functionalities */
/* Get option values: NWDRIVER_OPT_SUPPORTED_FUN and NWDRIVER_OPT_ADVERTISED_FUN */
#define NWDRIVER_SUP_FUN_AUTONEG            0x00000001U  /* autonegotiation */
#define NWDRIVER_SUP_FUN_TP                 0x00000002U  /* twisted pair */
#define NWDRIVER_SUP_FUN_AUI                0x00000004U  /* Attachment Unit Interface */
#define NWDRIVER_SUP_FUN_MII                0x00000008U  /* MII interface supported */
#define NWDRIVER_SUP_FUN_FIBRE              0x00000010U  /* fibre connector */
#define NWDRIVER_SUP_FUN_BNC                0x00000020U  /* BNC connector */
#define NWDRIVER_SUP_FUN_DAC                0x00000040U  /* Direct Attach Copper */
#define NWDRIVER_SUP_FUN_BACKPLANE          0x00000080U  /* support for ethernet backplane */
#define NWDRIVER_SUP_FUN_10BASET_HALF       0x00000100U  /* 10MB half duplex */
#define NWDRIVER_SUP_FUN_10BASET_FULL       0x00000200U  /* 10MB full duplex */
#define NWDRIVER_SUP_FUN_100BASET_HALF      0x00000400U  /* 100MB half duplex */
#define NWDRIVER_SUP_FUN_100BASET_FULL      0x00000800U  /* 100MB full duplex */
#define NWDRIVER_SUP_FUN_1000BASET_HALF     0x00001000U  /* 1000MB half duplex */
#define NWDRIVER_SUP_FUN_1000BASET_FULL     0x00002000U  /* 1000MB full duplex */
#define NWDRIVER_SUP_FUN_10000BASET_FULL    0x00004000U  /* 10000MB full duplex */
#define NWDRIVER_SUP_FUN_2500BASEX_FULL     0x00008000U  /* support for 2.5 GB BASE */
#define NWDRIVER_SUP_FUN_1000BASEKX_FULL    0x00010000U  /* support for 1000BASE-KX */
#define NWDRIVER_SUP_FUN_10000BASEKX4_FULL  0x00020000U  /* support for 1000BASE-KX4 */
#define NWDRIVER_SUP_FUN_10000BASEKR_FULL   0x00040000U  /* support for 1000BASE-KR */
#define NWDRIVER_SUP_FUN_10000BASER_FEC     0x00080000U  /* support for 1000BASE-R */
#define NWDRIVER_SUP_FUN_20000BASEMLD2_FULL 0x00100000U  /* support for 2000BASE-MLD2 */
#define NWDRIVER_SUP_FUN_20000BASEKR2_FULL  0x00200000U  /* support for 2000BASE-KR2 */
#define NWDRIVER_SUP_FUN_40000BASEKR4_FULL  0x00400000U  /* support for 4000BASE-KR4 */
#define NWDRIVER_SUP_FUN_40000BASECR4_FULL  0x00800000U  /* support for 4000BASE-CR4 */
#define NWDRIVER_SUP_FUN_40000BASESR4_FULL  0x01000000U  /* support for 4000BASE-SR4 */
#define NWDRIVER_SUP_FUN_40000BASELR4_FULL  0x02000000U  /* support for 4000BASE-LR4 */
#define NWDRIVER_SUP_FUN_56000BASEKR4_FULL  0x04000000U  /* support for 56000BASE-KR4 */
#define NWDRIVER_SUP_FUN_56000BASECR4_FULL  0x08000000U  /* support for 56000BASE-CR4 */
#define NWDRIVER_SUP_FUN_56000BASESR4_FULL  0x10000000U  /* support for 56000BASE-SR4 */
#define NWDRIVER_SUP_FUN_56000BASELR4_FULL  0x20000000U  /* support for 56000BASE-LR4 */

/* Duplex modes */
/* Get option value: NWDRIVER_OPT_DUPLEX_MODE */
#define NWDRIVER_DUPLEX_HALF                0x00U
#define NWDRIVER_DUPLEX_FULL                0x01U
#define NWDRIVER_DUPLEX_UNKNOWN             0xffU

/* Port types  */
/* Get option value: NWDRIVER_OPT_PORT_TYPE */
#define NWDRIVER_PORT_TP                    0x00U /* twisted pair */
#define NWDRIVER_PORT_AUI                   0x01U /* Attachment Unit Interface */
#define NWDRIVER_PORT_MII                   0x02U /* MII port */
#define NWDRIVER_PORT_FIBRE                 0x03U /* fibre */
#define NWDRIVER_PORT_BNC                   0x04U /* BNC connector */
#define NWDRIVER_PORT_DAC                   0x05U /* Direct Attach Copper */
#define NWDRIVER_PORT_NONE                  0xefU
#define NWDRIVER_PORT_OTHER                 0xffU


/* MDI or MDI-X status/control - if MDI/MDI_X/AUTO is set then the driver is required to renegotiate link */
/* Get/set option values: NWDRIVER_OPT_MDI_STATUS and NWDRIVER_OPT_MDI_CTRL */
#define NWDRIVER_ETH_TP_MDI_INVALID         0x00U /* unsupported */
#define NWDRIVER_ETH_TP_MDI                 0x01U /* MDI;  */
#define NWDRIVER_ETH_TP_MDI_X               0x02U /* MDI-X */
#define NWDRIVER_ETH_TP_MDI_AUTO            0x03U /* MDI-AUTO */

/* Timestamp modes */
/* Get/set option values: NWDRIVER_OPT_TIMESTAMP_SUP and NWDRIVER_OPT_TIMESTAMP_EN */
#define NWDRIVER_TIMESTAMP_RX               1U /* receive timestamp is supported or enabled */
#define NWDRIVER_TIMESTAMP_TX               2U /* transmit timestamp is supported or enabled */


/************************************************************************
* TYPEDEFS
*************************************************************************/

/* HW timestamp TX types - used by t_nw_ts_filter structure when setting/getting NWDRIVER_OPT_TIMESTAMP_HW_FIL option  */
typedef enum
{
  NWDRIVER_HWTSTAMP_TX_OFF            /* No outgoing packet will need hardware timestamping */
  , NWDRIVER_HWTSTAMP_TX_ON           /* Enables hardware timestamping for outgoing packets */
  , NWDRIVER_HWTSTAMP_TX_ONESTEP_SYNC /* Enables timestamping for outgoing packets but also enables timestamp */
                                      /* insertion directly into Sync packets. Not supported */
} t_nw_hwtstamp_tx_types;

/* HW timestamp RX filters - used by t_nw_ts_filter structure when setting/getting NWDRIVER_OPT_TIMESTAMP_HW_FIL option  */
typedef enum
{
  NWDRIVER_HWTSTAMP_FILTER_NONE                  /* timestamp no incoming packet at all */
  , NWDRIVER_HWTSTAMP_FILTER_ALL                 /* timestamp any incoming packet */
  , NWDRIVER_HWTSTAMP_FILTER_SOME                /* return value: timestamp all packets requested plus some others */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V1_L4_EVENT     /* PTP v1, UDP, any kind of event packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V1_L4_SYNC      /* PTP v1, UDP, Sync packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ /* PTP v1, UDP, Delay_req packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L4_EVENT     /* PTP v2, UDP, any kind of event packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L4_SYNC      /* PTP v2, UDP, Sync packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ /* PTP v2, UDP, Delay_req packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L2_EVENT     /* 802.AS1, Ethernet, any kind of event packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L2_SYNC      /* 802.AS1, Ethernet, Sync packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ /* 802.AS1, Ethernet, Delay_req packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_EVENT        /* PTP v2/802.AS1, any layer, any kind of event packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_SYNC         /* PTP v2/802.AS1, any layer, Sync packet */
  , NWDRIVER_HWTSTAMP_FILTER_PTP_V2_DELAY_REQ    /* PTP v2/802.AS1, any layer, Delay_req packet */
} t_nw_hwtstamp_rx_filters;

/* Structure to set/get timestamp HW RX/TX filtering */
/* Get/set option value: NWDRIVER_OPT_TIMESTAMP_HW_FIL */
typedef struct
{
  t_nw_hwtstamp_tx_types   nwtsf_tx;          /* NWDRIVER_HWTSTAMP_* when setting on read bitmap of supported NWDRIVER_HWTSTAMP_* options */
  t_nw_hwtstamp_rx_filters nwtsf_rx_filter;   /* NWDRIVER_HWTSTAMP_FILTER_* when setting on read bitmap of supported NWDRIVER_HWTSTAMP_FILTER_* options */
} t_nw_ts_filter;

/* structure to obtain timestamp data */
/* Get option values: NWDRIVER_OPT_GET_RX_TIMESTAMP and NWDRIVER_OPT_GET_TX_TIMESTAMP */
typedef struct
{
  const uint8_t * p_nwts_buf; /* buffer pointer for with times tamp will be obtained. Set by caller */
  uint32_t        nwts_sec;   /* timestamp seconds part. Set by NW driver on return */
  uint32_t        nwts_nsec;  /* timestamp nano seconds part. Set by NW driver on return */
} t_nw_timestamp;


/* return code type */
/* 495 S : MISRA-C:2012/AMD1/TC1 D.4.6: Typedef name has no size indication. */
/*LDRA_INSPECTED 495 S*/
typedef int  t_nwdriver_ret;


/* network states */
typedef enum
{
  NWDRIVER_ST_STOPPED = 0
  , NWDRIVER_ST_CONNECTED
  , NWDRIVER_ST_DISCONNECTED
  , NWDRIVER_ST_OFFLINE
}
t_nwdriver_state;


/*
** Notification functions
*/
typedef struct
{
  /*
  ** p_nwcb_state_state
  **   State change notification
  **
  ** Input:
  **   param    - parameter
  **   state    - NWDRIVER_ST_...
  **   from_isr - function called from ISR
  */
  void ( * p_nwcb_ntf_state )( uint32_t param
                              , uint8_t state
                              , uint8_t from_isr );


  /*
  ** p_nwcb_ntf_rx
  **  Packet was received
  **
  ** Input:
  **   param    - parameter
  **   len      - length of the received frame
  **   from_isr - function called from ISR
  */
  void ( * p_nwcb_ntf_rx )( uint32_t param
                           , uint16_t len
                           , uint8_t from_isr );


  /*
  ** p_nwcb_ntf_tx
  **   Packet was sent
  **
  ** Input:
  **   param    - parameter
  **   ret      - completion code
  **   from_isr - function called from ISR
  */
  void ( * p_nwcb_ntf_tx )( uint32_t param
                           , t_nwdriver_ret ret
                           , uint8_t from_isr );
} t_nwdriver_cb_dsc;


/*
** Network driver properties
*/
typedef struct
{
  /* option flags (NWDRIVER_OPT_...) */
  uint8_t  nwp_options;

  /* MTU size */
  uint16_t  nwp_mtu_size;

  /* RX buffer count, used to know how many MTU size RX buffers are required */
  /* by the driver. */
  uint16_t  nwp_rxbuf_count;

  /* base address of the transfer area */
  uint8_t * p_nwp_buf;

  /* size of the transfer area */
  uint32_t  nwp_buf_size;

  /* NW driver header size. The driver can request for an area preceeding */
  /* the physical frame. This can be used if the driver needs to send an */
  /* extra hedare with the frame. */
  uint8_t  nwp_header_size;

  /* buffer pad bytes (required amount of unused bytes to be allocated after */
  /* MTU packets). It can be required if for example the NW driver writes */
  /* CRC after the packet. */
  uint8_t  nwp_buf_pad;

  /* required alignment of the packet. alignment = ( 1u << nwbd_align_sh ). */
  /* This option can be useful if for example the system can only do DMA */
  /* transfers from an aligned address. */
  uint8_t  nwp_buf_align_sh;

  /* unit size shift, required to define packet properties added with */
  /* p_nwfn_add_buf or sent with p_nwfn_send. All packets must be assigned */
  /* in a buffer area aligned to unit size and of size that is multiple of */
  /* unit size. This doesn't mean that len field of p_nwfn_send should be */
  /* multiple of unit size, it only means the caller must make sure nobody */
  /* else uses that area during the transfer. This can be used for example */
  /* to be able to provide a cached transfer area to the higher layer and be */
  /* able to call cache invalidate in p_nwfn_add_buf and flush in p_nwfn_send. */
  /* unit size = ( 1u << nwbd_unit_sh ). */
  uint8_t  nwp_buf_unit_sh;
} t_nwdriver_prop;


/* Network driver functions */
typedef struct s_nwdriver  t_nwdriver;

typedef struct
{
  /*
  ** p_nwfn_start
  **   Start network driver
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - failed to start
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   p_cb_dsc   - pointer to callback functions
  **   param      - parameter for the callback functions
  */
  t_nwdriver_ret  ( * p_nwfn_start )( const t_nwdriver * const p_nwdriver
                                     , const t_nwdriver_cb_dsc * const p_cb_dsc
                                     , const uint32_t param );


  /*
  ** p_nwfn_stop
  **   Stop network driver
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - failed to stop
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  */
  t_nwdriver_ret  ( * p_nwfn_stop )( const t_nwdriver * const p_nwdriver );


  /*
  ** p_nwfn_delete
  **   Delete network driver (free all resources)
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - failed to delete
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  */
  t_nwdriver_ret  ( * p_nwfn_delete )( const t_nwdriver * const p_nwdriver );


  /*
  ** p_nwfn_get_state
  **   Get the state of the network.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **
  ** Output:
  **   p_state - where to write the state
  */
  t_nwdriver_ret ( * p_nwfn_get_state )( const t_nwdriver * const p_nwdriver
                                        , t_nwdriver_state * const p_state );


  /*
  ** p_nwfn_receive
  **   Receieve a packet.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS     - successful execution
  **   NWDRIVER_QUEUE_EMPTY - receive queue is empty
  **   NWDRIVER_INVALID     - received packet is invalid
  **   NWDRIVER_ERROR       - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **
  ** Output:
  **   p_buf    - where to write the pointer of the buffer where data was read to
  **   p_rlen   - where to write the number of read bytes
  */
  t_nwdriver_ret  ( * p_nwfn_receive )( const t_nwdriver * const p_nwdriver
                                       , uint8_t * * const pp_buf
                                       , uint16_t * const p_rlen );


  /*
  ** p_nwfn_send
  **   Send data, this should be a non-blocking function.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS    - successful execution
  **   NWDRIVER_QUEUE_FULL - transmit queue is full
  **   NWDRIVER_ERROR      - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   p_buf      - where to send from
  **   len        - no. bytes to send
  */
  t_nwdriver_ret  ( * p_nwfn_send )( const t_nwdriver * const p_nwdriver
                                    , uint8_t * const p_buf
                                    , const uint16_t len );


  /*
  ** p_nwfn_add_buf
  **   Add buffer to the network driver
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS    - successful execution
  **   NWDRIVER_QUEUE_FULL - buffer queue is full
  **   NWDRIVER_ERROR      - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   p_buf      - pointer to the buffer (size of it should be big enough
  **                to hold the typical frame - 1536 bytes)
  */
  t_nwdriver_ret  ( * p_nwfn_add_buf )( const t_nwdriver * const p_nwdriver
                                       , uint8_t * const p_buf );


  /*
  ** p_nwfn_get_hw_addr
  **   Get HW address.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **
  ** Output:
  **   p_hw_address - where to write the pointer to the HW address
  */
  t_nwdriver_ret  ( * p_nwfn_get_hw_addr )( const t_nwdriver * const p_nwdriver
                                           , uint8_t * * const pp_hw_address );


  /*
  ** p_nwfn_set_hw_addr
  **   Set HW address. This function is only allowed if the driver is
  **   in stopped state.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver   - pointer to network driver
  **   p_hw_address - pointer to the HW address
  */
  t_nwdriver_ret  ( * p_nwfn_set_hw_addr )( const t_nwdriver * const p_nwdriver
                                           , const uint8_t * const p_hw_address );


  /*
  ** p_nwfn_set_filter
  **   Set filte mode.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error
  **
  ** Input:
  **   p_nwdriver   - pointer to network driver
  **   mode         - filtering mode mask (NWDRIVER_FILTER_...)
  */
  t_nwdriver_ret  ( * p_nwfn_set_filter )( const t_nwdriver * const p_nwdriver
                                          , const uint8_t mode );


  /*
  ** p_nwfn_set_multicast_table
  **   Set multicast table. This function is only useful if
  **   NWDRIVER_FILTER_MULTICAST_TBL was set with set_filter().
  **   If the table is empty then all multicast packets will be
  **   dropped.
  **   This function should be in a protected state when called.
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   p_table    - pointer to the multicast table
  **   table_size - no. elements in the multicast table
  */
  t_nwdriver_ret  ( * p_nwfn_set_multicast_table )( const t_nwdriver * const p_nwdriver
                                                   , const uint8_t * const p_table
                                                   , const uint16_t table_size );


  /*
  ** p_nwfn_get_link_speed
  **   Get the speed of the link
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **
  ** Output:
  **   p_link_speed - where to write link speed
  */
  t_nwdriver_ret  ( * p_nwfn_get_link_speed )( const t_nwdriver * const p_nwdriver
                                              , uint32_t * const p_link_speed );


  /*
  ** p_nwfn_set_option
  **   Set the nwdriver option
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   option     - option type ( NWDRIVER_OPT_
  **   p_val      - pointer to variable specific for given option
  **   val_length - variable length
  **
  ** Output:
  **   None
  */
  t_nwdriver_ret  ( * p_nwfn_set_option )( const t_nwdriver * const p_nwdriver
                                          , const uint16_t option
                                          , const void * const p_val
                                          , const uint32_t val_length );


  /*
  ** p_nwfn_get_option
  **   Get the nwdriver option
  **
  ** Return:
  **   NWDRIVER_SUCCESS - successful execution
  **   NWDRIVER_ERROR   - general error (link is down, not started, etc.)
  **
  ** Input:
  **   p_nwdriver - pointer to network driver
  **   option     - option type ( NWDRIVER_OPT_*)
  **
  ** Output:
  **   p_val        - pointer to variable where to write given option data
  **   p_val_length - variable length
  */
  t_nwdriver_ret  ( * p_nwfn_get_option )( const t_nwdriver * const p_nwdriver
                                          , const uint16_t option
                                          , void * const p_val
                                          , uint32_t * const p_val_length );
} t_nwdriver_fn;



struct s_nwdriver
{
  /* Network driver properties */
  t_nwdriver_prop  nwd_prop;

  /* Network driver functions */
  const t_nwdriver_fn * p_nwd_fn;

  /* used by the network driver for internal purposes */
  uint32_t  nwd_data;
};


/*
** t_nwdriver_init - initialize the network driver
**
** Return:
**   NWDRIVER_SUCCESS : initialization successsful
**   NWDRIVER_ERROR : initialization error
**
** Input:
**   param - driver specific parameter (e.g.: unit ID)
**
** Output:
**   p_nwdriver - where to write the pointer of the network driver
*/
typedef t_nwdriver_ret ( * t_nwdriver_init )( const uint32_t param
                                             , t_nwdriver * * const pp_nwdriver );


/************************************************************************
* EXTERNAL VARIABLES
*************************************************************************/


/************************************************************************
* Function Prototype Section
*************************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* ifndef API_NWDRIVER_H */

