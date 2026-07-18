/*
 * Copyright 2018, Broadcom Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Inc.
 */

#ifndef INCLUDED_MHD_API_H
#define INCLUDED_MHD_API_H

#include <stdint.h>
#include "mhd_constants.h"          /* For mhd_result_t */

#define PM1_POWERSAVE_MODE          ( 1 )
#define PM2_POWERSAVE_MODE          ( 2 )
#define NO_POWERSAVE_MODE           ( 0 )

/////////////////////////////
#define MHD_APPS_ERROR_VAL	0x0001
#define MHD_APPS_TRACE_VAL	0x0002
#define MHD_APPS_INFO_VAL	0x0004
#define MHD_RTOS_ERROR_VAL	0x0008
#define MHD_RTOS_TRACE_VAL	0x0010
#define MHD_RTOS_INFO_VAL	0x0020
#define MHD_NET_ERROR_VAL	0x0040
#define MHD_NET_TRACE_VAL	0x0080
#define MHD_NET_INFO_VAL	0x0100
#define MHD_HOST_ERROR_VAL	0x0200
#define MHD_HOST_TRACE_VAL	0x0400
#define MHD_HOST_INFO_VAL	0x0800
#define MHD_SDPCM_ERROR_VAL	0x1000
#define MHD_SDPCM_TRACE_VAL	0x2000
#define MHD_SDPCM_INFO_VAL	0x4000

#define MHD_DRV_ERROR_VAL	0x8000
#define MHD_DRV_DEBUG_VAL	0x10000
#define MHD_DRV_TRACE_VAL	0x20000
#define MHD_DRV_INFO_VAL	0x40000

#define MHD_THREAD_DEBUG_VAL	0x80000

#define MHD_PWR_INFO_VAL		0x100000
#define MHD_PWR_TRACE_VAL		0x200000
#define MHD_PWR_ERROR_VAL		0x400000
/////////////////////////////

typedef struct 
{
    uint8_t octet[6]; /* Unique 6-byte MAC address */
} mhd_mac_t;

typedef struct 
{
    uint8_t length;    /* SSID length */
    uint8_t value[32]; /* SSID name (AP name)  */
} mhd_ssid_t;

typedef struct 
{
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
	mhd_ssid_t ssid;
	mhd_mac_t bssid;
	uint32_t channel;
	uint32_t security;
	uint32_t rssi;
	char ccode[4];
	char vndr_ie[5];
#else
    char ssid[32];
    char bssid[6];
    uint32_t channel;
    uint32_t security;
    uint32_t rssi;
    char ccode[4];
#endif
} mhd_ap_info_t;

typedef struct 
{
    mhd_ssid_t SSID;
    uint32_t security;
    uint8_t channel;
    uint8_t security_key_length;
    char security_key[ 64 ];
    mhd_mac_t BSSID;
} mhd_ap_connect_info_t;

typedef enum 
{
    MHD_PHY_TYPE_11A = 0, // 802.11A PHY type
    MHD_PHY_TYPE_11B,     // 802.11B PHY type
    MHD_PHY_TYPE_11G,     // 802.11G PHY type
    MHD_PHY_TYPE_11N,     // 802.11N PHY type
    MHD_PHY_TYPE_11AC,    // 802.11AC PHY type
    MHD_PHY_TYPE_ERR,     // Unknown PHY type

    MHD_PHY_TYPE_MAX  // Enum count, used for sanity check.
} mhd_phy_type_t;

typedef enum
{
    MHD_STA_AP_MODE      = 0,
    MHD_P2P_MODE         = 1,
} mhd_operation_mode_t;

typedef void (*mhd_link_callback_t)(void);

extern void mhd_sdio_controller_index_register( uint32_t index );
extern void mhd_set_country_code( uint32_t country );
extern int mhd_module_init( mhd_operation_mode_t op_mode );
extern int mhd_module_exit( void );
extern void mhd_log_msg_level(uint32_t val);

extern int mhd_start_scan( void );
extern int mhd_get_scan_results( mhd_ap_info_t *results, int *num );

extern void mhd_set_wifi_11n_support(int enable);

// station connects to ap. 0:success, 1:failed
// security: 0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
extern int mhd_sta_connect( const char *ssid, char *bssid, uint8_t security, const char *password, uint8_t channel );
extern int mhd_sta_disconnect( uint8_t force );

extern int mhd_sta_network_up( uint32_t ip, uint32_t gateway, uint32_t netmask );
extern int mhd_sta_network_down( void );

extern int mhd_sta_register_link_callback( mhd_link_callback_t link_up_cb, mhd_link_callback_t link_down_cb );
extern int mhd_sta_deregister_link_callback( mhd_link_callback_t link_up_cb, mhd_link_callback_t link_down_cb );

extern int mhd_sta_get_rssi( void );
extern int mhd_sta_get_rate( void );
extern int mhd_sta_get_mac_address( mhd_mac_t *mac );
extern void mhd_sta_get_last_connected_ap_info( mhd_ap_connect_info_t *ap_info );
extern int mhd_sta_get_noise( void );
extern uint32_t mhd_sta_get_nrate( void );
extern mhd_phy_type_t mhd_sta_get_phy_type( void );
extern int mhd_sta_get_connection( void );

extern uint32_t mhd_sta_ipv4_ipaddr( void );
extern uint32_t mhd_sta_ipv4_gateway( void );
extern uint32_t mhd_sta_ipv4_netmask( void );

extern int mhd_sta_set_powersave( uint8_t mode, uint8_t time_ms );
extern int mhd_sta_get_powersave( uint8_t *mode, uint8_t *time_ms );

extern int mhd_sta_set_bcn_li_dtim( uint8_t dtim );
extern int mhd_sta_get_bcn_li_dtim( void );
extern int mhd_sta_set_dtim_interval( int dtim_interval_ms );

extern void mhd_get_ampdu_hostreorder_support();
extern void mhd_set_ampdu_hostreorder_support(int enable);

extern int mhd_sta_wowl_init( const char *pattern_string, int offset );
extern int mhd_sta_wowl_init_ext( const char *pattern_string, const char *mask_string, int offset );
extern int mhd_sta_wowl_enable( void );
extern int mhd_sta_wowl_disable( void );
extern int mhd_sta_wowl_status( void );
extern int mhd_sta_wowl_clear( void );

// ssid:  less than 32 bytes
// password: less than 32 bytes
// security: 0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
// channel: 1~13
extern int mhd_softap_start( const unsigned char *ssid, const char *password, uint8_t security, uint8_t channel );
extern int mhd_softap_stop( uint8_t force );

extern int mhd_softap_get_mac_address( mhd_mac_t *mac );

extern int mhd_softap_set_hidden( int enable );
extern int mhd_softap_get_hidden( void );

extern int mhd_softap_start_dhcpd( uint32_t ip_address);
extern int mhd_softap_stop_dhcpd( void );

extern int mhd_softap_get_mac_list( mhd_mac_t *mac_list, uint32_t *count );
extern int mhd_softap_get_rssi( mhd_mac_t *mac_addr );
extern int mhd_softap_deauth_assoc_sta(const mhd_mac_t* mac);

typedef void (*mhd_client_callback_t)(mhd_mac_t);
int mhd_softap_register_client_callback( mhd_client_callback_t client_assoc_cb, mhd_client_callback_t client_disassoc_cb );

extern int mhd_wifi_get_channel( int interface, uint32_t* channel );
extern int mhd_wifi_set_channel( int interface, uint32_t channel );
extern int mhd_wifi_get_max_associations( uint32_t* max_assoc );
extern int mhd_wifi_set_max_associations( uint32_t max_assoc );
extern int mhd_softap_add_custom_ie( const uint8_t* oui, uint8_t sub_type, void* data, uint16_t data_length, uint16_t which_packets );
extern int mhd_softap_remove_custom_ie( const uint8_t* oui, uint8_t sub_type, void* data, uint16_t data_length, uint16_t which_packets );


// p2p api
extern void mhd_get_valid_channels( void );
extern void mhd_set_channel_for_P2P( uint channel );
extern int mhd_start_P2P_service( char *device_name, char *ssid, char *password );
extern void mhd_stop_P2P_service( void );
extern int mhd_p2p_network_up( uint32_t ip, uint32_t gateway, uint32_t netmask );
extern int mhd_p2p_network_down( void );

extern void host_rtos_delay_milliseconds( uint32_t num_ms );

#endif /* ifndef INCLUDED_MHD_API_H */
