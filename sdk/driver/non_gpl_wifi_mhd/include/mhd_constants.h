/*
 * Copyright 2018, Broadcom Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Inc.
 */

#ifndef INCLUDED_MHD_CONSTANTS_H_
#define INCLUDED_MHD_CONSTANTS_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


#ifndef MIN
/* LINT : This tells lint that  the parameter must be side-effect free.
 * i.e. evaluation does not change any values (since it is being evaulated more than once 
 */
extern int MIN (int x, int y);
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif /* ifndef MIN */

#ifndef MAX
/* LINT : This tells lint that  the parameter must be side-effect free. 
 * i.e. evaluation does not change any values (since it is being evaulated more than once 
 */
extern int MAX (int x, int y); 
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif /* ifndef MAX */

#ifndef ROUND_UP
/* LINT : This tells lint that  the parameter must be side-effect free. 
 * i.e. evaluation does not change any values (since it is being evaulated more than once 
 */
extern int ROUND_UP (int x, int y); 
#define ROUND_UP(x,y)    ((x) % (y) ? (x) + (y)-((x)%(y)) : (x))
#endif /* ifndef ROUND_UP */

#ifndef DIV_ROUND_UP
/* LINT : This tells lint that  the parameter must be side-effect free. 
 * i.e. evaluation does not change any values (since it is being evaulated more than once 
 */
extern int DIV_ROUND_UP (int m, int n); 
#define DIV_ROUND_UP(m, n)    (((m) + (n) - 1) / (n))
#endif /* ifndef DIV_ROUND_UP */

#ifndef PLATFORM
#define PLATFORM "Unknown"
#endif /* ifndef PLATFORM */

#ifndef RTOS_VERSION
#define RTOS_VERSION "Unknown"
#endif /* ifndef RTOS_VERSION */

#ifndef TCPIP_VERSION
#define TCPIP_VERSION "Unknown"
#endif /* ifndef TCPIP_VERSION */

#ifndef HOST_VERSION
#define HOST_VERSION "Unknown"
#endif /* ifndef HOST_VERSION */


/** @cond !ADDTHIS*/
#define WEP_ENABLED            0x0001
#define TKIP_ENABLED           0x0002
#define AES_ENABLED            0x0004
#define SHARED_ENABLED  0x00008000
#define WPA_SECURITY    0x00200000
#define WPA2_SECURITY   0x00400000
#define ENTERPRISE_ENABLED 0x02000000
#define WPS_ENABLED     0x10000000
#define IBSS_ENABLED       0x20000000
/** @endcond */

#define DSSS_PARAMETER_SET_LENGTH (1)

#define HT_CAPABILITIES_IE_LENGTH (26)
#define HT_OPERATION_IE_LENGTH    (22)

#define RRM_CAPABILITIES_LEN (5)
#define WL_RRM_RPT_VER      0
#define WL_RRM_RPT_MAX_PAYLOAD  64
#define WL_RRM_RPT_MIN_PAYLOAD  7
#define WL_RRM_RPT_FALG_ERR 0
#define WL_RRM_RPT_FALG_OK  1

/* TLV defines */
#define TLV_TAG_OFF        0    /* tag offset */
#define TLV_LEN_OFF        1    /* length offset */
#define TLV_HDR_LEN        2    /* header length */
#define TLV_BODY_OFF        2    /* body offset */

#define DOT11_NEIGHBOR_REP_IE_FIXED_LEN 13
#define DOT11_MNG_NEIGHBOR_REP_ID       52  /* 11k & 11v Neighbor report id */

/* Bitmap definitions for cap ie */
#define DOT11_RRM_CAP_LINK      0
#define DOT11_RRM_CAP_NEIGHBOR_REPORT   1
#define DOT11_RRM_CAP_PARALLEL      2
#define DOT11_RRM_CAP_REPEATED      3
#define DOT11_RRM_CAP_BCN_PASSIVE   4
#define DOT11_RRM_CAP_BCN_ACTIVE    5
#define DOT11_RRM_CAP_BCN_TABLE     6
#define DOT11_RRM_CAP_BCN_REP_COND  7
#define DOT11_RRM_CAP_FM        8
#define DOT11_RRM_CAP_CLM       9
#define DOT11_RRM_CAP_NHM       10
#define DOT11_RRM_CAP_SM        11
#define DOT11_RRM_CAP_LCIM      12
#define DOT11_RRM_CAP_LCIA      13
#define DOT11_RRM_CAP_TSCM      14
#define DOT11_RRM_CAP_TTSCM     15
#define DOT11_RRM_CAP_AP_CHANREP    16
#define DOT11_RRM_CAP_RMMIB     17
/* bit18-bit26, not used for RRM_IOVAR */
#define DOT11_RRM_CAP_MPTI      27
#define DOT11_RRM_CAP_NBRTSFO       28
#define DOT11_RRM_CAP_RCPI      29
#define DOT11_RRM_CAP_RSNI      30
#define DOT11_RRM_CAP_BSSAAD        31
#define DOT11_RRM_CAP_BSSAAC        32
#define DOT11_RRM_CAP_AI        33
#define DOT11_RRM_CAP_LAST 34


/** Enumeration of HOST interfaces. \n
 * @note The config interface is a virtual interface that shares the softAP interface
 */
typedef enum
{
    MHD_STA_INTERFACE = 0, /**< STA or Client Interface  */
    MHD_AP_INTERFACE  = 1,  /**< softAP Interface         */
    MHD_P2P_INTERFACE = 2, /**< P2P Interface         */
} mhd_interface_t;

/**
 * Enumeration of Wi-Fi security modes
 */

typedef enum 
{
    MHD_SECURE_OPEN,
    MHD_WPA_PSK_AES,     // WPA-PSK AES
    MHD_WPA2_PSK_AES,    // WPA2-PSK AES
    MHD_WEP_OPEN,        // WEP+OPEN
    MHD_WEP_SHARED,      // WEP+SHARE
    MHD_WPA_PSK_TKIP,    // WPA-PSK TKIP
    MHD_WPA_PSK_MIXED,   // WPA-PSK AES & TKIP MIXED
    MHD_WPA2_PSK_TKIP,   // WPA2-PSK TKIP
    MHD_WPA2_PSK_MIXED,  // WPA2-PSK AES & TKIP MIXED
	MHD_WPA2_PSK_SHA256, // WPA2-PSK-SHA256
	MHD_WPA3_PSK_SAE,	 // WPA3-PSK SAE
    //MHD_WPA3_WPA2_PSK,   // WPA3 SAE & WPA2-PSK AES
    MHD_WPS_OPEN,        // WPS OPEN, NOT supported
    MHD_WPS_AES,         // WPS AES, NOT supported
    MHD_IBSS_OPEN,       // ADHOC, NOT supported
    MHD_WPA_ENT_AES,     // WPA-ENT AES, NOT supported
    MHD_WPA_ENT_TKIP,    // WPA-ENT TKIP, NOT supported
    MHD_WPA_ENT_MIXED,   // WPA-ENT AES & TKIP MIXED, NOT supported
    MHD_WPA2_ENT_AES,    // WPA2-ENT AES, NOT supported
    MHD_WPA2_ENT_TKIP,   // WPA2-ENT TKIP, NOT supported
    MHD_WPA2_ENT_MIXED,  // WPA2-ENT AES & TKIP MIXED, NOT supported
} mhd_sta_security_t;

typedef enum 
{
    MHD_AP_OPEN,           // 0 OPEN
    MHD_AP_WPA_AES_PSK,    // 1 WPA-PSK AES
    MHD_AP_WPA2_AES_PSK,   // 2 WPA2-PSK AES
    MHD_AP_WEP_OPEN,       // 3 WEP+OPEN
    MHD_AP_WEP_SHARED,     // 4 WEP+SHARE
    MHD_AP_WPA_TKIP_PSK,   // 5 WPA-PSK TKIP
    MHD_AP_WPA_MIXED_PSK,  // 6 WPA-PSK AES & TKIP MIXED
    MHD_AP_WPA2_TKIP_PSK,  // 7 WPA2-PSK TKIP
    MHD_AP_WPA2_MIXED_PSK, // 8 WPA2-PSK AES & TKIP MIXED
	MHD_AP_WPA3_SAE,       // 9 WPA3 SAE
	MHD_AP_WPA3_WPA2_PSK,  // 10 WPA3 & WPA2 AES
    MHD_AP_WPS_OPEN,       // 11 WPS OPEN, NOT supported
    MHD_AP_WPS_AES,        // 12 WPS AES, NOT supported
} mhd_ap_security_t;

/**
 * Enumeration of methods of scanning
 */
typedef enum
{
    HOST_SCAN_TYPE_ACTIVE              = 0x00,  /**< Actively scan a network by sending 802.11 probe(s)         */
    HOST_SCAN_TYPE_PASSIVE             = 0x01,  /**< Passively scan a network by listening for beacons from APs */
    HOST_SCAN_TYPE_PROHIBITED_CHANNELS = 0x04   /**< Passively scan on channels not enabled by the country code */
} host_scan_type_t;

/**
 * Enumeration of network types
 */
typedef enum
{
    HOST_BSS_TYPE_INFRASTRUCTURE = 0, /**< Denotes infrastructure network                  */
    HOST_BSS_TYPE_ADHOC          = 1, /**< Denotes an 802.11 ad-hoc IBSS network           */
    HOST_BSS_TYPE_ANY            = 2, /**< Denotes either infrastructure or ad-hoc network */

    HOST_BSS_TYPE_UNKNOWN        = -1 /**< May be returned by scan function if BSS type is unknown. Do not pass this to the Join function */
} host_bss_type_t;

/**
 * Enumeration of 802.11 radio bands
 */
typedef enum
{
    HOST_802_11_BAND_5GHZ   = 0, /**< Denotes 5GHz radio band   */
    HOST_802_11_BAND_2_4GHZ = 1  /**< Denotes 2.4GHz radio band */
} host_802_11_band_t;

/**
 * Enumeration of antenna selection options
 */
typedef enum
{
    HOST_ANTENNA_1    = 0,  /**< Denotes antenna 1 */
    HOST_ANTENNA_2    = 1,  /**< Denotes antenna 2 */
    HOST_ANTENNA_AUTO = 3   /**< Denotes auto diversity, the best antenna is automatically selected */
} host_antenna_t;

/**
 * Enumeration of applicable packet mask bits for custom Information Elements (IEs)
 */
typedef enum
{
    VENDOR_IE_BEACON         = 0x1,  /**< Denotes beacon packet                  */
    VENDOR_IE_PROBE_RESPONSE = 0x2,  /**< Denotes probe response packet          */
    VENDOR_IE_ASSOC_RESPONSE = 0x4,  /**< Denotes association response packet    */
    VENDOR_IE_AUTH_RESPONSE  = 0x8,  /**< Denotes authentication response packet */
    VENDOR_IE_PROBE_REQUEST  = 0x10, /**< Denotes probe request packet           */
    VENDOR_IE_ASSOC_REQUEST  = 0x20, /**< Denotes association request packet     */
    VENDOR_IE_CUSTOM         = 0x100 /**< Denotes a custom IE identifier         */
} host_ie_packet_flag_t;

/**
 * Enumeration of custom IE management actions
 */
typedef enum
{
    HOST_ADD_CUSTOM_IE,     /**< Add a custom IE    */
    HOST_REMOVE_CUSTOM_IE   /**< Remove a custom IE */
} host_custom_ie_action_t;

/**
 * Enumeration of 802.11 QoS, i.e. WMM, traffic classes
 */
typedef enum
{
    WMM_AC_BE =         0,      /**< Best Effort */
    WMM_AC_BK =         1,      /**< Background  */
    WMM_AC_VI =         2,      /**< Video       */
    WMM_AC_VO =         3,      /**< Voice       */
} host_qos_access_category_t;

/**
 * Enumeration of IP header Type of Service (TOS) values, which map to 802.11 QoS traffic classes
 */
typedef enum
{
    TOS_VO7 = 7, /**< 0xE0, 111 0  0000 (7)  AC_VO tos/dscp values */
    TOS_VO  = 6, /**< 0xD0, 110 0  0000 (6)  AC_VO                 */
    TOS_VI  = 5, /**< 0xA0, 101 0  0000 (5)  AC_VI                 */
    TOS_VI4 = 4, /**< 0x80, 100 0  0000 (4)  AC_VI                 */
    TOS_BE  = 0, /**< 0x00, 000 0  0000 (0)  AC_BE                 */
    TOS_EE  = 3, /**< 0x60, 011 0  0000 (3)  AC_BE                 */
    TOS_BK  = 1, /**< 0x20, 001 0  0000 (1)  AC_BK                 */
    TOS_LE  = 2, /**< 0x40, 010 0  0000 (2)  AC_BK                 */
} host_ip_header_tos_t;

/**
 * Enumeration of listen interval time unit types
 */
typedef enum
{
    HOST_LISTEN_INTERVAL_TIME_UNIT_BEACON, /**< Time units specified in beacon periods */
    HOST_LISTEN_INTERVAL_TIME_UNIT_DTIM    /**< Time units specified in DTIM periods   */
} host_listen_interval_time_unit_t;

/**
 * Enumeration of packet filter modes
 */
typedef enum
{
    HOST_PACKET_FILTER_MODE_FORWARD = 1, /**< Packet filter engine forwards matching packets, discards non-matching packets */
    HOST_PACKET_FILTER_MODE_DISCARD = 0, /**< Packet filter engine discards matching packets, forwards non-matching packets */
} host_packet_filter_mode_t;

/**
 * Enumeration of packet filter rules
 */
typedef enum
{
    HOST_PACKET_FILTER_RULE_POSITIVE_MATCHING  = 0, /**< Specifies that a filter should match a given pattern     */
    HOST_PACKET_FILTER_RULE_NEGATIVE_MATCHING  = 1  /**< Specifies that a filter should NOT match a given pattern */
} host_packet_filter_rule_t;

typedef enum
{
    HOST_SCAN_INCOMPLETE,
    HOST_SCAN_COMPLETED_SUCCESSFULLY,
    HOST_SCAN_ABORTED,
} host_scan_status_t;

/** List of HT modes supported */
typedef enum
{
    HOST_HT_MODE_HT20      = 0,        /**< HT20 mode is set on the band */
    HOST_HT_MODE_HT40      = 1,        /**< HT40 mode is set on the band */
    HOST_HT_MODE_HT_MIX    = 2         /**< HT20 mode is set for 2.4 band and HT40 is set for 5 GHz band */
} host_ht_mode_t;

typedef enum
{
    HOST_11N_SUPPORT_DISABLED = 0,
    HOST_11N_SUPPORT_ENABLED  = 1,
} host_11n_support_t;

/* 802.11 Information Element Identification Numbers (as per section 8.4.2.1 of 802.11-2012) */
typedef enum
{
    DOT11_IE_ID_SSID                                 = 0,
    DOT11_IE_ID_SUPPORTED_RATES                      = 1,
    DOT11_IE_ID_FH_PARAMETER_SET                     = 2,
    DOT11_IE_ID_DSSS_PARAMETER_SET                   = 3,
    DOT11_IE_ID_CF_PARAMETER_SET                     = 4,
    DOT11_IE_ID_TIM                                  = 5,
    DOT11_IE_ID_IBSS_PARAMETER_SET                   = 6,
    DOT11_IE_ID_COUNTRY                              = 7,
    DOT11_IE_ID_HOPPING_PATTERN_PARAMETERS           = 8,
    DOT11_IE_ID_HOPPING_PATTERN_TABLE                = 9,
    DOT11_IE_ID_REQUEST                              = 10,
    DOT11_IE_ID_BSS_LOAD                             = 11,
    DOT11_IE_ID_EDCA_PARAMETER_SET                   = 12,
    DOT11_IE_ID_TSPEC                                = 13,
    DOT11_IE_ID_TCLAS                                = 14,
    DOT11_IE_ID_SCHEDULE                             = 15,
    DOT11_IE_ID_CHALLENGE_TEXT                       = 16,
    /* 17-31 Reserved */
    DOT11_IE_ID_POWER_CONSTRAINT                     = 32,
    DOT11_IE_ID_POWER_CAPABILITY                     = 33,
    DOT11_IE_ID_TPC_REQUEST                          = 34,
    DOT11_IE_ID_TPC_REPORT                           = 35,
    DOT11_IE_ID_SUPPORTED_CHANNELS                   = 36,
    DOT11_IE_ID_CHANNEL_SWITCH_ANNOUNCEMENT          = 37,
    DOT11_IE_ID_MEASUREMENT_REQUEST                  = 38,
    DOT11_IE_ID_MEASUREMENT_REPORT                   = 39,
    DOT11_IE_ID_QUIET                                = 40,
    DOT11_IE_ID_IBSS_DFS                             = 41,
    DOT11_IE_ID_ERP                                  = 42,
    DOT11_IE_ID_TS_DELAY                             = 43,
    DOT11_IE_ID_TCLAS_PROCESSING                     = 44,
    DOT11_IE_ID_HT_CAPABILITIES                      = 45,
    DOT11_IE_ID_QOS_CAPABILITY                       = 46,
    /* 47 Reserved */
    DOT11_IE_ID_RSN                                  = 48,
    /* 49 Reserved */
    DOT11_IE_ID_EXTENDED_SUPPORTED_RATES             = 50,
    DOT11_IE_ID_AP_CHANNEL_REPORT                    = 51,
    DOT11_IE_ID_NEIGHBOR_REPORT                      = 52,
    DOT11_IE_ID_RCPI                                 = 53,
    DOT11_IE_ID_MOBILITY_DOMAIN                      = 54,
    DOT11_IE_ID_FAST_BSS_TRANSITION                  = 55,
    DOT11_IE_ID_TIMEOUT_INTERVAL                     = 56,
    DOT11_IE_ID_RIC_DATA                             = 57,
    DOT11_IE_ID_DSE_REGISTERED_LOCATION              = 58,
    DOT11_IE_ID_SUPPORTED_OPERATING_CLASSES          = 59,
    DOT11_IE_ID_EXTENDED_CHANNEL_SWITCH_ANNOUNCEMENT = 60,
    DOT11_IE_ID_HT_OPERATION                         = 61,
    DOT11_IE_ID_SECONDARY_CHANNEL_OFFSET             = 62,
    DOT11_IE_ID_BSS_AVERAGE_ACCESS_DELAY             = 63,
    DOT11_IE_ID_ANTENNA                              = 64,
    DOT11_IE_ID_RSNI                                 = 65,
    DOT11_IE_ID_MEASUREMENT_PILOT_TRANSMISSION       = 66,
    DOT11_IE_ID_BSS_AVAILABLE_ADMISSION_CAPACITY     = 67,
    DOT11_IE_ID_BSS_AC_ACCESS_DELAY                  = 68,
    DOT11_IE_ID_TIME_ADVERTISEMENT                   = 69,
    DOT11_IE_ID_RM_ENABLED_CAPABILITIES              = 70,
    DOT11_IE_ID_MULTIPLE_BSSID                       = 71,
    DOT11_IE_ID_20_40_BSS_COEXISTENCE                = 72,
    DOT11_IE_ID_20_40_BSS_INTOLERANT_CHANNEL_REPORT  = 73,
    DOT11_IE_ID_OVERLAPPING_BSS_SCAN_PARAMETERS      = 74,
    DOT11_IE_ID_RIC_DESCRIPTOR                       = 75,
    DOT11_IE_ID_MANAGEMENT_MIC                       = 76,
    DOT11_IE_ID_EVENT_REQUEST                        = 78,
    DOT11_IE_ID_EVENT_REPORT                         = 79,
    DOT11_IE_ID_DIAGNOSTIC_REQUEST                   = 80,
    DOT11_IE_ID_DIAGNOSTIC_REPORT                    = 81,
    DOT11_IE_ID_LOCATION_PARAMETERS                  = 82,
    DOT11_IE_ID_NONTRANSMITTED_BSSID_CAPABILITY      = 83,
    DOT11_IE_ID_SSID_LIST                            = 84,
    DOT11_IE_ID_MULTIPLE_BSSID_INDEX                 = 85,
    DOT11_IE_ID_FMS_DESCRIPTOR                       = 86,
    DOT11_IE_ID_FMS_REQUEST                          = 87,
    DOT11_IE_ID_FMS_RESPONSE                         = 88,
    DOT11_IE_ID_QOS_TRAFFIC_CAPABILITY               = 89,
    DOT11_IE_ID_BSS_MAX_IDLE_PERIOD                  = 90,
    DOT11_IE_ID_TFS_REQUEST                          = 91,
    DOT11_IE_ID_TFS_RESPONSE                         = 92,
    DOT11_IE_ID_WNM_SLEEP_MODE                       = 93,
    DOT11_IE_ID_TIM_BROADCAST_REQUEST                = 94,
    DOT11_IE_ID_TIM_BROADCAST_RESPONSE               = 95,
    DOT11_IE_ID_COLLOCATED_INTERFERENCE_REPORT       = 96,
    DOT11_IE_ID_CHANNEL_USAGE                        = 97,
    DOT11_IE_ID_TIME_ZONE                            = 98,
    DOT11_IE_ID_DMS_REQUEST                          = 99,
    DOT11_IE_ID_DMS_RESPONSE                         = 100,
    DOT11_IE_ID_LINK_IDENTIFIER                      = 101,
    DOT11_IE_ID_WAKEUP_SCHEDULE                      = 102,
    /* 103 Reserved */
    DOT11_IE_ID_CHANNEL_SWITCH_TIMING                = 104,
    DOT11_IE_ID_PTI_CONTROL                          = 105,
    DOT11_IE_ID_TPU_BUFFER_STATUS                    = 106,
    DOT11_IE_ID_INTERWORKING                         = 107,
    DOT11_IE_ID_ADVERTISMENT_PROTOCOL                = 108,
    DOT11_IE_ID_EXPEDITED_BANDWIDTH_REQUEST          = 109,
    DOT11_IE_ID_QOS_MAP_SET                          = 110,
    DOT11_IE_ID_ROAMING_CONSORTIUM                   = 111,
    DOT11_IE_ID_EMERGENCY_ALERT_IDENTIFIER           = 112,
    DOT11_IE_ID_MESH_CONFIGURATION                   = 113,
    DOT11_IE_ID_MESH_ID                              = 114,
    DOT11_IE_ID_MESH_LINK_METRIC_REPORT              = 115,
    DOT11_IE_ID_CONGESTION_NOTIFICATION              = 116,
    DOT11_IE_ID_MESH_PEERING_MANAGEMENT              = 117,
    DOT11_IE_ID_MESH_CHANNEL_SWITCH_PARAMETERS       = 118,
    DOT11_IE_ID_MESH_AWAKE_WINDOW                    = 119,
    DOT11_IE_ID_BEACON_TIMING                        = 120,
    DOT11_IE_ID_MCCAOP_SETUP_REQUEST                 = 121,
    DOT11_IE_ID_MCCAOP_SETUP_REPLY                   = 122,
    DOT11_IE_ID_MCCAOP_ADVERTISMENT                  = 123,
    DOT11_IE_ID_MCCAOP_TEARDOWN                      = 124,
    DOT11_IE_ID_GANN                                 = 125,
    DOT11_IE_ID_RANN                                 = 126,
    DOT11_IE_ID_EXTENDED_CAPABILITIES                = 127,
    /* 128-129 Reserved */
    DOT11_IE_ID_PREQ                                 = 130,
    DOT11_IE_ID_PREP                                 = 131,
    DOT11_IE_ID_PERR                                 = 132,
    /* 133-136 Reserved */
    DOT11_IE_ID_PXU                                  = 137,
    DOT11_IE_ID_PXUC                                 = 138,
    DOT11_IE_ID_AUTHENTICATED_MESH_PEERING_EXCHANGE  = 139,
    DOT11_IE_ID_MIC                                  = 140,
    DOT11_IE_ID_DESTINATION_URI                      = 141,
    DOT11_IE_ID_U_APSD_COEXISTENCE                   = 142,
    /* 143-173 Reserved */
    DOT11_IE_ID_MCCAOP_ADVERTISMENT_OVERVIEW         = 174,
    /* 175-220 Reserved */
    DOT11_IE_ID_VENDOR_SPECIFIC                      = 221,
    /* 222-255 Reserved */
} dot11_ie_id_t;

/* Protected Management Frame Capability */
typedef enum
{
    MFP_NONE = 0,
    MFP_CAPABLE,
    MFP_REQUIRED
} mfp_capability_t;

#ifndef RESULT_ENUM
#define RESULT_ENUM( prefix, name, value )  prefix ## name = (value)
#endif /* ifndef RESULT_ENUM */


/* These Enum result values are for MHD errors
 * Values: 1000 - 1999
 */

#define MHD_RESULT_LIST( prefix )  \
    RESULT_ENUM( prefix, SUCCESS,                         0 ),   /**< Success */                           \
    RESULT_ENUM( prefix, PENDING,                         1 ),   /**< Pending */                           \
    RESULT_ENUM( prefix, TIMEOUT,                         2 ),   /**< Timeout */                           \
    RESULT_ENUM( prefix, ERROR,                           4 ),   /**< Error */                          \
    RESULT_ENUM( prefix, BADARG,                          5 ),   /**< Bad Arguments */                  \
    RESULT_ENUM( prefix, BADOPTION,                       6 ),   /**< Mode not supported */             \
    RESULT_ENUM( prefix, PARTIAL_RESULTS,              1003 ),   /**< Partial results */                   \
    RESULT_ENUM( prefix, INVALID_KEY,                  1004 ),   /**< Invalid key */                       \
    RESULT_ENUM( prefix, DOES_NOT_EXIST,               1005 ),   /**< Does not exist */                    \
    RESULT_ENUM( prefix, NOT_AUTHENTICATED,            1006 ),   /**< Not authenticated */                 \
    RESULT_ENUM( prefix, NOT_KEYED,                    1007 ),   /**< Not keyed */                         \
    RESULT_ENUM( prefix, IOCTL_FAIL,                   1008 ),   /**< IOCTL fail */                        \
    RESULT_ENUM( prefix, BUFFER_UNAVAILABLE_TEMPORARY, 1009 ),   /**< Buffer unavailable temporarily */    \
    RESULT_ENUM( prefix, BUFFER_UNAVAILABLE_PERMANENT, 1010 ),   /**< Buffer unavailable permanently */    \
    RESULT_ENUM( prefix, WPS_PBC_OVERLAP,              1011 ),   /**< WPS PBC overlap */                   \
    RESULT_ENUM( prefix, CONNECTION_LOST,              1012 ),   /**< Connection lost */                   \
    RESULT_ENUM( prefix, OUT_OF_EVENT_HANDLER_SPACE,   1013 ),   /**< Cannot add extra event handler */    \
    RESULT_ENUM( prefix, SEMAPHORE_ERROR,              1014 ),   /**< Error manipulating a semaphore */    \
    RESULT_ENUM( prefix, FLOW_CONTROLLED,              1015 ),   /**< Packet retrieval cancelled due to flow control */ \
    RESULT_ENUM( prefix, NO_CREDITS,                   1016 ),   /**< Packet retrieval cancelled due to lack of bus credits */ \
    RESULT_ENUM( prefix, NO_PACKET_TO_SEND,            1017 ),   /**< Packet retrieval cancelled due to no pending packets */ \
    RESULT_ENUM( prefix, CORE_CLOCK_NOT_ENABLED,       1018 ),   /**< Core disabled due to no clock */    \
    RESULT_ENUM( prefix, CORE_IN_RESET,                1019 ),   /**< Core disabled - in reset */         \
    RESULT_ENUM( prefix, UNSUPPORTED,                  1020 ),   /**< Unsupported function */             \
    RESULT_ENUM( prefix, BUS_WRITE_REGISTER_ERROR,     1021 ),   /**< Error writing to WLAN register */   \
    RESULT_ENUM( prefix, SDIO_BUS_UP_FAIL,             1022 ),   /**< SDIO bus failed to come up */       \
    RESULT_ENUM( prefix, JOIN_IN_PROGRESS,             1023 ),   /**< Join not finished yet */   \
    RESULT_ENUM( prefix, NETWORK_NOT_FOUND,            1024 ),   /**< Specified network was not found */   \
    RESULT_ENUM( prefix, INVALID_JOIN_STATUS,          1025 ),   /**< Join status error */   \
    RESULT_ENUM( prefix, UNKNOWN_INTERFACE,            1026 ),   /**< Unknown interface specified */ \
    RESULT_ENUM( prefix, SDIO_RX_FAIL,                 1027 ),   /**< Error during SDIO receive */   \
    RESULT_ENUM( prefix, HWTAG_MISMATCH,               1028 ),   /**< Hardware tag header corrupt */   \
    RESULT_ENUM( prefix, RX_BUFFER_ALLOC_FAIL,         1029 ),   /**< Failed to allocate a buffer to receive into */   \
    RESULT_ENUM( prefix, BUS_READ_REGISTER_ERROR,      1030 ),   /**< Error reading a bus hardware register */   \
    RESULT_ENUM( prefix, THREAD_CREATE_FAILED,         1031 ),   /**< Failed to create a new thread */   \
    RESULT_ENUM( prefix, QUEUE_ERROR,                  1032 ),   /**< Error manipulating a queue */   \
    RESULT_ENUM( prefix, BUFFER_POINTER_MOVE_ERROR,    1033 ),   /**< Error moving the current pointer of a packet buffer  */   \
    RESULT_ENUM( prefix, BUFFER_SIZE_SET_ERROR,        1034 ),   /**< Error setting size of packet buffer */   \
    RESULT_ENUM( prefix, THREAD_STACK_NULL,            1035 ),   /**< Null stack pointer passed when non null was reqired */   \
    RESULT_ENUM( prefix, THREAD_DELETE_FAIL,           1036 ),   /**< Error deleting a thread */   \
    RESULT_ENUM( prefix, SLEEP_ERROR,                  1037 ),   /**< Error sleeping a thread */ \
    RESULT_ENUM( prefix, BUFFER_ALLOC_FAIL,            1038 ),   /**< Failed to allocate a packet buffer */ \
    RESULT_ENUM( prefix, NO_PACKET_TO_RECEIVE,         1039 ),   /**< No Packets waiting to be received */ \
    RESULT_ENUM( prefix, INTERFACE_NOT_UP,             1040 ),   /**< Requested interface is not active */ \
    RESULT_ENUM( prefix, DELAY_TOO_LONG,               1041 ),   /**< Requested delay is too long */ \
    RESULT_ENUM( prefix, INVALID_DUTY_CYCLE,           1042 ),   /**< Duty cycle is outside limit 0 to 100 */ \
    RESULT_ENUM( prefix, PMK_WRONG_LENGTH,             1043 ),   /**< Returned pmk was the wrong length */ \
    RESULT_ENUM( prefix, UNKNOWN_SECURITY_TYPE,        1044 ),   /**< AP security type was unknown */ \
    RESULT_ENUM( prefix, WEP_NOT_ALLOWED,              1045 ),   /**< AP not allowed to use WEP - it is not secure - use Open instead */ \
    RESULT_ENUM( prefix, WPA_KEYLEN_BAD,               1046 ),   /**< WPA / WPA2 key length must be between 8 & 64 bytes */ \
    RESULT_ENUM( prefix, FILTER_NOT_FOUND,             1047 ),   /**< Specified filter id not found */ \
    RESULT_ENUM( prefix, SPI_ID_READ_FAIL,             1048 ),   /**< Failed to read 0xfeedbead SPI id from chip */ \
    RESULT_ENUM( prefix, SPI_SIZE_MISMATCH,            1049 ),   /**< Mismatch in sizes between SPI header and SDPCM header */ \
    RESULT_ENUM( prefix, ADDRESS_ALREADY_REGISTERED,   1050 ),   /**< Attempt to register a multicast address twice */ \
    RESULT_ENUM( prefix, SDIO_RETRIES_EXCEEDED,        1051 ),   /**< SDIO transfer failed too many times. */ \
    RESULT_ENUM( prefix, NULL_PTR_ARG,                 1052 ),   /**< Null Pointer argument passed to function. */ \
    RESULT_ENUM( prefix, THREAD_FINISH_FAIL,           1053 ),   /**< Error deleting a thread */ \
    RESULT_ENUM( prefix, WAIT_ABORTED,                 1054 ),   /**< Semaphore/mutex wait has been aborted */ \
    RESULT_ENUM( prefix, SET_BLOCK_ACK_WINDOW_FAIL,    1055 ),   /**< Failed to set block ack window */ \
    RESULT_ENUM( prefix, DELAY_TOO_SHORT,              1056 ),   /**< Requested delay is too short */ \
    RESULT_ENUM( prefix, INVALID_INTERFACE,            1057 ),   /**< Invalid interface provided */ \
    RESULT_ENUM( prefix, WEP_KEYLEN_BAD,               1058 ),   /**< WEP / WEP_SHARED key length must be 5 or 13 bytes */ \
    RESULT_ENUM( prefix, HANDLER_ALREADY_REGISTERED,   1059 ),   /**< EAPOL handler already registered */ \
    RESULT_ENUM( prefix, AP_ALREADY_UP,                1060 ),   /**< Soft AP or P2P group owner already up */ \
    RESULT_ENUM( prefix, EAPOL_KEY_PACKET_M1_TIMEOUT,  1061 ),   /**< Timeout occurred while waiting for EAPOL packet M1 from AP */ \
    RESULT_ENUM( prefix, EAPOL_KEY_PACKET_M3_TIMEOUT,  1062 ),   /**< Timeout occurred while waiting for EAPOL packet M3 from AP, which may indicate incorrect WPA2/WPA passphrase */ \
    RESULT_ENUM( prefix, EAPOL_KEY_PACKET_G1_TIMEOUT,  1063 ),   /**< Timeout occurred while waiting for EAPOL packet G1 from AP */ \
    RESULT_ENUM( prefix, EAPOL_KEY_FAILURE,            1064 ),   /**< Unknown failure occurred during the EAPOL key handshake */ \
    RESULT_ENUM( prefix, MALLOC_FAILURE,               1065 ),   /**< Memory allocation failure */ \
    RESULT_ENUM( prefix, ACCESS_POINT_NOT_FOUND,       1066 ),   /**< Access point not found */


/* These Enum result values are returned directly from the WLAN during an ioctl or iovar call.
 * Values: 1100 - 1200
 */
#define WLAN_ENUM_OFFSET  (2000)

#define WLAN_RESULT_LIST( prefix ) \
    RESULT_ENUM( prefix, ERROR,                       2001 ),  /**< Generic Error */                     \
    RESULT_ENUM( prefix, BADARG,                      2002 ),  /**< Bad Argument */                      \
    RESULT_ENUM( prefix, BADOPTION,                   2003 ),  /**< Bad option */                        \
    RESULT_ENUM( prefix, NOTUP,                       2004 ),  /**< Not up */                            \
    RESULT_ENUM( prefix, NOTDOWN,                     2005 ),  /**< Not down */                          \
    RESULT_ENUM( prefix, NOTAP,                       2006 ),  /**< Not AP */                            \
    RESULT_ENUM( prefix, NOTSTA,                      2007 ),  /**< Not STA  */                          \
    RESULT_ENUM( prefix, BADKEYIDX,                   2008 ),  /**< BAD Key Index */                     \
    RESULT_ENUM( prefix, RADIOOFF,                    2009 ),  /**< Radio Off */                         \
    RESULT_ENUM( prefix, NOTBANDLOCKED,               2010 ),  /**< Not  band locked */                  \
    RESULT_ENUM( prefix, NOCLK,                       2011 ),  /**< No Clock */                          \
    RESULT_ENUM( prefix, BADRATESET,                  2012 ),  /**< BAD Rate valueset */                 \
    RESULT_ENUM( prefix, BADBAND,                     2013 ),  /**< BAD Band */                          \
    RESULT_ENUM( prefix, BUFTOOSHORT,                 2014 ),  /**< Buffer too short */                  \
    RESULT_ENUM( prefix, BUFTOOLONG,                  2015 ),  /**< Buffer too long */                   \
    RESULT_ENUM( prefix, BUSY,                        2016 ),  /**< Busy */                              \
    RESULT_ENUM( prefix, NOTASSOCIATED,               2017 ),  /**< Not Associated */                    \
    RESULT_ENUM( prefix, BADSSIDLEN,                  2018 ),  /**< Bad SSID len */                      \
    RESULT_ENUM( prefix, OUTOFRANGECHAN,              2019 ),  /**< Out of Range Channel */              \
    RESULT_ENUM( prefix, BADCHAN,                     2020 ),  /**< Bad Channel */                       \
    RESULT_ENUM( prefix, BADADDR,                     2021 ),  /**< Bad Address */                       \
    RESULT_ENUM( prefix, NORESOURCE,                  2022 ),  /**< Not Enough Resources */              \
    RESULT_ENUM( prefix, UNSUPPORTED,                 2023 ),  /**< Unsupported */                       \
    RESULT_ENUM( prefix, BADLEN,                      2024 ),  /**< Bad length */                        \
    RESULT_ENUM( prefix, NOTREADY,                    2025 ),  /**< Not Ready */                         \
    RESULT_ENUM( prefix, EPERM,                       2026 ),  /**< Not Permitted */                     \
    RESULT_ENUM( prefix, NOMEM,                       2027 ),  /**< No Memory */                         \
    RESULT_ENUM( prefix, ASSOCIATED,                  2028 ),  /**< Associated */                        \
    RESULT_ENUM( prefix, RANGE,                       2029 ),  /**< Not In Range */                      \
    RESULT_ENUM( prefix, NOTFOUND,                    2030 ),  /**< Not Found */                         \
    RESULT_ENUM( prefix, WME_NOT_ENABLED,             2031 ),  /**< WME Not Enabled */                   \
    RESULT_ENUM( prefix, TSPEC_NOTFOUND,              2032 ),  /**< TSPEC Not Found */                   \
    RESULT_ENUM( prefix, ACM_NOTSUPPORTED,            2033 ),  /**< ACM Not Supported */                 \
    RESULT_ENUM( prefix, NOT_WME_ASSOCIATION,         2034 ),  /**< Not WME Association */               \
    RESULT_ENUM( prefix, SDIO_ERROR,                  2035 ),  /**< SDIO Bus Error */                    \
    RESULT_ENUM( prefix, WLAN_DOWN,                   2036 ),  /**< WLAN Not Accessible */               \
    RESULT_ENUM( prefix, BAD_VERSION,                 2037 ),  /**< Incorrect version */                 \
    RESULT_ENUM( prefix, TXFAIL,                      2038 ),  /**< TX failure */                        \
    RESULT_ENUM( prefix, RXFAIL,                      2039 ),  /**< RX failure */                        \
    RESULT_ENUM( prefix, NODEVICE,                    2040 ),  /**< Device not present */                \
    RESULT_ENUM( prefix, UNFINISHED,                  2041 ),  /**< To be finished */                    \
    RESULT_ENUM( prefix, NONRESIDENT,                 2042 ),  /**< access to nonresident overlay */     \
    RESULT_ENUM( prefix, DISABLED,                    2043 ),  /**< Disabled in this build */

/**
 * Common result type for HOST functions
 */
typedef enum
{
    MHD_RESULT_LIST( MHD_ )
    WLAN_RESULT_LIST( MHD_WLAN_ )
} mhd_result_t;

/**
 * Boolean values
 */
typedef enum
{
    MHD_FALSE = 0,
    MHD_TRUE  = 1
} mhd_bool_t;

/**
 * I/O State Values
 */
typedef enum
{
    MHD_ACTIVE_LOW = 0,
    MHD_ACTIVE_HIGH = 1
} mhd_io_state_t;

/**
 * Enumeration of Dot11 Reason Codes
 */
typedef enum
{
    MHD_DOT11_RC_RESERVED  = 0,    /**< Reserved     */
    MHD_DOT11_RC_UNSPECIFIED  = 1  /**< Unspecified  */
} mhd_dot11_reason_code_t;


/******************************************************
 *            Constants
 ******************************************************/

/**
 * Transfer direction for the HOST platform bus interface
 */
typedef enum
{
    /* If updating this enum, the bus_direction_mapping variable will also need to be updated */
    BUS_READ,
    BUS_WRITE
} mhd_bus_transfer_direction_t;

/**
 * Macro for creating country codes according to endianness
 */

#ifdef IL_BIGENDIAN
#define MK_CNTRY( a, b, rev )  (((unsigned char)(b)) + (((unsigned char)(a))<<8) + (((unsigned short)(rev))<<16) )
#else /* ifdef IL_BIGENDIAN */
#define MK_CNTRY( a, b, rev )  (((unsigned char)(a)) + (((unsigned char)(b))<<8) + (((unsigned short)(rev))<<16) )
#endif /* ifdef IL_BIGENDIAN */

/* Suppress unused parameter warning */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) ( (void)(x) )
#endif

/* Suppress unused variable warning */
#ifndef UNUSED_VARIABLE
#define UNUSED_VARIABLE(x) ( (void)(x) )
#endif


/* Suppress unused variable warning occurring due to an assert which is disabled in release mode */
#ifndef REFERENCE_DEBUG_ONLY_VARIABLE
#define REFERENCE_DEBUG_ONLY_VARIABLE(x) ( (void)(x) )
#endif

/* Allow functions to be deprecated */
#ifdef __GNUC__
#define DEPRECATE( name )  name __attribute__ ((deprecated))
#else
#define DEPRECATE( name )  deprecated_ ## name
#endif

#ifdef MHD_GOTHAM
#define MHD_COUNTRY_DEFAULT    MK_CNTRY( 'W', 'W', 999 )
#else
#define MHD_COUNTRY_DEFAULT    MK_CNTRY( 'U', 'S', 0 )
#endif

typedef uint32_t mhd_country_code_t;

/* HOST Radio Resource Management Report Types */
typedef enum
{
   HOST_RRM_CHLOAD_REPORT = 0,
   HOST_RRM_NBR_LIST_REPORT,
   HOST_RRM_BCN_REPORT,
   HOST_LM_REPORT
} mhd_rrm_report_type_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_MHD_CONSTANTS_H_ */