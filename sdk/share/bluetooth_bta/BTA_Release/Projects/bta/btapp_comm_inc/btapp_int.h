/****************************************************************************
**
**  Name:          btapp_int.h
**
**  Description:   Contains btapp internal header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_gatt_api.h"
#include "gki.h"
#include "bta_dm_co.h"
#include <stdio.h>

#ifndef BTAPP_INT_H
#define BTAPP_INT_H

#include "bta_avk_api.h"
#include "bta_hs_api.h"
#include "bta_pbc_api.h"
#include "bta_av_api.h"
#include "bta_ag_api.h"

#define BTAPP_NUM_REM_DEVICE    7                /* Algin with BTM layer device counts */
#define BTAPP_NUM_INQ_DEVICE    8
#define BTAPP_DEV_NAME_LENGTH   32
#define BTAPP_MAX_DEFAULT_PEERS      (2)         /* Maximum number of default peer bd address */
#define BTAPP_DEFAULT_INQ_DURATION     10 /* in 1.28 secs */
#define BTAPP_DEFAULT_BLE_SCAN_DURATION 5

#define BTAPP_START_TIMER       0x0001
#define BTAPP_STOP_TIMER        0x0002
#define BTAPP_TASK_SHUTDOWN     0x0003

#ifndef BTUI_SCREEN_LENGTH
#define BTUI_SCREEN_LENGTH           15
#endif

#ifndef BTUI_MAX_PATH_LENGTH
#define BTUI_MAX_PATH_LENGTH         511
#endif

#ifndef BTAPP_MAX_PATH_LENGTH
#define BTAPP_MAX_PATH_LENGTH  BTUI_MAX_PATH_LENGTH
#endif

/* service mask for GATT based services */
#define     BTAPP_BLE_GAP_SERVICE_MASK       0x00000001        /* bit 0  - GAP */
#define     BTAPP_BLE_GATT_SERVICE_MASK      0x00000002        /* bit 1  - GATT */
#define     BTAPP_BLE_IA_SERVICE_MASK        0x00000004        /* bit 2  - Immediate Alert */
#define     BTAPP_BLE_LL_SERVICE_MASK        0x00000008        /* bit 3  - Link Loss */
#define     BTAPP_BLE_TX_SERVICE_MASK        0x00000010        /* bit 4  - Tx Power */
#define     BTAPP_BLE_CT_SERVICE_MASK        0x00000020        /* bit 5  - Current Time */
#define     BTAPP_BLE_DST_SERVICE_MASK       0x00000040        /* bit 6  - DST time change */
#define     BTAPP_BLE_REF_TIME_SERVICE_MASK  0x00000080        /* bit 7  - Reference Time */
#define     BTAPP_BLE_NWA_SERVICE_MASK       0x00000100        /* bit 8  - Network Available Warning */
#define     BTAPP_BLE_PHALERT_SERVICE_MASK   0x00000200        /* bit 9  - Phone Alert */
#define     BTAPP_BLE_GM_SERVICE_MASK        0x00000400        /* bit 10 - Glucose meter */
#define     BTAPP_BLE_DEV_INFO_SERVICE_MASK  0x00000800        /* bit 11 - Device Info */
#define     BTAPP_BLE_HID_SERVICE_MASK       0x00001000        /* bit 12 - HID */
#define     BTAPP_BLE_BATTERY_SERVICE_MASK   0x00002000        /* bit 13 - Battery */
#define     BTAPP_BLE_SCANPARAM_SERVICE_MASK 0x00004000        /* bit 14 - Scan Param */
#define     BTAPP_BLE_RSC_SERVICE_MASK       0x00008000        /* bit 15 - Runners Speed and Cadence Service mask */
#define     BTAPP_BLE_CSC_SERVICE_MASK       0x00010000        /* bit 16 - Cycling Speed and Cadence Service mask */
#define     BTAPP_BLE_CP_SERVICE_MASK        0x00020000        /* bit 17 - Cycling Power Service mask */
#define     BTAPP_BLE_SERVICE_BIT_MAX        18

/* Local Constants */
#ifndef BTAPP_AG_SECURITY
#define BTAPP_AG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif
#ifndef BTAPP_HSAG_SERVICE_NAME
#define BTAPP_HSAG_SERVICE_NAME "Headset Gateway"
#endif
#ifndef BTAPP_HFAG_SERVICE_NAME
#define BTAPP_HFAG_SERVICE_NAME "Handsfree Gateway"
#endif
#ifndef BTAPP_AG_FEATURES
#define BTAPP_AG_FEATURES (BTA_AG_FEAT_3WAY | BTA_AG_FEAT_ECNR | BTA_AG_FEAT_VREC | \
                          BTA_AG_FEAT_INBAND | BTA_AG_FEAT_VTAG | BTA_AG_FEAT_REJECT | \
                          BTA_AG_FEAT_ECS  | BTA_AG_FEAT_ECC  | BTA_AG_FEAT_EXTERR | \
                          BTA_AG_FEAT_CODEC | BTA_AG_FEAT_HF_IND | BTA_AG_FEAT_ESCO | \
                          BTA_AG_FEAT_BTRH)
#endif
#ifndef BTAPP_AG_INSTANCES
#define BTAPP_AG_INSTANCES 2
#endif

#ifndef BTAPP_HS_SECURITY
#define BTAPP_HS_SECURITY       BTA_SEC_NONE //(BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTAPP_HS_FEATURES
#define BTAPP_HS_FEATURES    ( BTA_HS_FEAT_ECNR | BTA_HS_FEAT_3WAY | BTA_HS_FEAT_CLIP | \
                              BTA_HS_FEAT_VREC | BTA_HS_FEAT_RVOL | BTA_HS_FEAT_ECS  | \
                              BTA_HS_FEAT_ECC | BTA_HS_FEAT_CODEC | BTA_HS_FEAT_HF_IND | \
                              BTA_HS_FEAT_ESCO | BTA_HS_FEAT_UNAT)
#endif

#ifndef BTAPP_HSHS_SERVICE_NAME
#define BTAPP_HSHS_SERVICE_NAME "BRCM Headset"
#endif
#ifndef BTAPP_HFHS_SERVICE_NAME
#define BTAPP_HFHS_SERVICE_NAME "BRCM Handsfree"
#endif

#ifndef BTAPP_SPPDG_SECURITY
#define BTAPP_SPPDG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif
#ifndef BTAPP_SPPDG_SERVICE_NAME
#define BTAPP_SPPDG_SERVICE_NAME "Serial Port"
#endif

#ifndef BTAPP_AVK_FEATURES
#define BTAPP_AVK_FEATURES       (  BTA_AVK_FEAT_RCTG    |  \
                                    BTA_AVK_FEAT_RCCT    |  \
                                    BTA_AVK_FEAT_PROTECT |  \
                                    BTA_AVK_FEAT_BROWSE  |  \
                                    BTA_AVK_FEAT_VENDOR  |  \
                                    BTA_AVK_FEAT_DELAY_RPT | \
                                    BTA_AVK_FEAT_METADATA \
                                  )
#endif

#ifndef BTAPP_AVK_SECURITY
#define BTAPP_AVK_SECURITY  BTA_SEC_NONE //(BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif


#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)
#ifndef BTUI_PBC_FORMAT
#define BTUI_PBC_FORMAT     (BTA_PBC_FORMAT_CARD_21)
#endif

#ifndef BTUI_PBC_FILTER
#define BTUI_PBC_FILTER     (BTA_PBC_FILTER_ALL) /* 0 */
#endif

#ifndef BTUI_PBC_ORDER
#define BTUI_PBC_ORDER      (BTA_PBC_ORDER_INDEXED)
#endif

#ifndef BTUI_PBC_ATTR
#define BTUI_PBC_ATTR       (BTA_PBC_ATTR_NAME)
#endif

#ifndef BTUI_PBC_SELECTOR
#define BTUI_PBC_SELECTOR   (BTA_PBC_FILTER_ALL) /* 0 */
#endif

#ifndef BTUI_PBC_SELECTOR_OP
#define BTUI_PBC_SELECTOR_OP    (0) /* 0 OR, 1 AND */
#endif

#ifndef BTUI_PBC_MAX_LCOUNT
#define BTUI_PBC_MAX_LCOUNT       (50)
#endif

#ifndef BTUI_PBC_PASSWORD
#define BTUI_PBC_PASSWORD "1234"
#endif

#ifndef BTUI_PBC_USERID
#define BTUI_PBC_USERID "guest"
#endif

#ifndef BTUI_PBC_SECURITY
#define BTUI_PBC_SECURITY (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_OPS_SECURITY
#define BTUI_OPS_SECURITY ( BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_OPS_SERVICE_NAME
#define BTUI_OPS_SERVICE_NAME "Object Push"
#endif

#ifndef BTUI_OPS_SRM
#define BTUI_OPS_SRM    TRUE
#endif

#ifndef BTUI_OPC_SECURITY
#define BTUI_OPC_SECURITY (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_OPC_SERVICE_NAME
#define BTUI_OPC_SERVICE_NAME "Object Push"
#endif

#ifndef BTUI_OPC_SRM
#define BTUI_OPC_SRM    TRUE
#endif

#endif /* BTA_PBC_INCLUDED*/

#ifndef BTUI_MAX_AUTH_KEY_LENGTH
#define BTUI_MAX_AUTH_KEY_LENGTH 16
#endif

enum
{
    BTAPP_UNKNOWN_DEVICE    ,
    BTAPP_DATA_BASE_FULL    ,
    BTAPP_NOT_INITIALISED,
    BTAPP_FAILED_TO_STORE_EVENT    ,
    BTAPP_INVALID_DATA,
    BTAPP_INVALID_EVENT,
    BTAPP_INVALID_ORIG,
    BTAPP_INVAILD_PARAM,
    BTAPP_UNABLE_TO_CREATE_EVT_BUF,
    BTAPP_FAIL,
    BTAPP_SUCCESS
};

typedef UINT32 tBTAPP_BLE_SERVICE_MASK;

/* remote device */
typedef struct
{
     BOOLEAN    in_use;
     BD_ADDR    bd_addr;
     char       name[BTAPP_DEV_NAME_LENGTH+1];
     UINT8      pin_code[PIN_CODE_LEN];
     DEV_CLASS  dev_class;
     char       short_name[BTAPP_DEV_NAME_LENGTH+1];      /* short name which user can assign to a device */
     LINK_KEY   link_key;
     UINT8      key_type;
     tBTA_IO_CAP peer_io_cap;
     tBTA_AUTH_REQ auth_mode;
     BOOLEAN    link_key_present;
     BOOLEAN    is_trusted;
     tBTA_SERVICE_MASK trusted_mask;
     BOOLEAN    is_default;
     tBTA_SERVICE_MASK services;
     INT8       rssi;
     INT8       rssi_offset;

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
     /* inquiry related information */
     tBTAPP_BLE_SERVICE_MASK ble_services  ;
     BOOLEAN                bond_with_peer;
     /* security related information */
     tBLE_ADDR_TYPE         addr_type;
     tBT_DEVICE_TYPE        device_type;
     tBTA_LE_KEY_TYPE       key_mask;
     tBTA_LE_PENC_KEYS      penc_key;       /* received peer encryption key */
     tBTA_LE_PCSRK_KEYS     pcsrk_key;      /* received peer device SRK */
     tBTA_LE_PID_KEYS       pid_key;        /* peer device ID key */
     tBTA_LE_LENC_KEYS      lenc_key;       /* local encryption reproduction keys LTK = = d1(ER,DIV,0)*/
     tBTA_LE_LCSRK_KEYS     lcsrk_key;      /* local device CSRK = d1(ER,DIV,1)*/
     tBTA_LE_PID_KEYS       lid_key;        /* local device ID key */
     BOOLEAN                bg_conn;        /* in background connection mode */
#endif
}tBTAPP_REM_DEVICE;

typedef struct
{
     BOOLEAN    in_use;
     BD_ADDR    bd_addr;
} tBTAPP_CONN_DEVICE;
#define BTAPP_MAX_CONN_DEVICE    7

/* typedef for all data that application needs to store in nvram */
typedef struct
{
     BOOLEAN                    bt_enabled;  /* bluetooth enabled or not */
     UINT8                      count;
     char                       local_device_name[BTAPP_DEV_NAME_LENGTH+1];     /* local bluetooth name */
     BOOLEAN                    visibility;
     BOOLEAN                    pairability;

     tBTAPP_REM_DEVICE          device[BTAPP_NUM_REM_DEVICE];
     TIMER_LIST_ENT             dev_tle;
     BOOLEAN                    conn_paired_only;
#if BLE_INCLUDED == TRUE
     UINT16                     le_conn_mode;
     UINT16                     le_disc_mode;
#endif
}tBTAPP_DEV_DB;
extern tBTAPP_DEV_DB btapp_device_db;

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
#define BTAPP_GATT_MAX_ATTR        30
typedef struct
{
    BOOLEAN             in_use;
    UINT8               device_idx;
    tBTA_GATTC_NV_ATTR  attr;
}tBTAPP_GATT_CACHE;

typedef struct
{
    tBTAPP_GATT_CACHE    attr_cache[BTAPP_GATT_MAX_ATTR];   /* server cache */
}tBTAPP_GATT_CACHE_DB;
extern tBTAPP_GATT_CACHE_DB btapp_gatt_cache_db;

/* service handle range db */
#define BTAPP_GATT_MAX_HANDLE_MAP_SIZE       20

typedef struct
{
    UINT8                       num_services;
    tBTA_GATTS_HNDL_RANGE       hndl_range[BTAPP_GATT_MAX_HANDLE_MAP_SIZE];  /* array upto (num_services - 1) are in use*/
}tBTAPP_GATTS_HNDL_RANGE_DB;
extern tBTAPP_GATTS_HNDL_RANGE_DB   btapp_gatts_hndl_range_db;  /* server cache */

/* service change db */
#define BTAPP_GATT_MAX_SRV_CHG_CLT_SIZE       BTM_SEC_MAX_SERVICE_RECORDS

typedef struct
{
    UINT8                       num_clients;
    tBTA_GATTS_SRV_CHG         srv_chg[BTM_SEC_MAX_SERVICE_RECORDS];  /* array upto (num_clients - 1) are in use*/
}tBTAPP_GATTS_SRV_CHG_DB;

extern tBTAPP_GATTS_SRV_CHG_DB   btapp_gatts_srv_chg_db;  /* server cache */

#endif

#if( defined BLE_INCLUDED ) && (BLE_INCLUDED == TRUE)
typedef struct
{
    BT_HDR      hdr;
    BD_ADDR     bd_addr;
}tBTAPP_BLE_REQ;

typedef struct
{
    BT_HDR      hdr;
    BD_ADDR     bd_addr;
    UINT32      passkey;
}tBTAPP_BLE_PK_NOTIF;

typedef struct
{
    BT_HDR              hdr;
    tBTA_GATT_STATUS    status;
    tBTA_GATT_ID        srvc_id;
    tBTA_GATT_ID        find_id;
    tBTA_GATT_CHAR_PROP property;
}tBTAPP_BLE_GATTC_FIND;

typedef struct
{
    BT_HDR              hdr;
    tBTA_GATT_STATUS    status;
    tBTA_GATTC_CHAR_ID  char_id;
    tBT_UUID            descr_uuid;
}tBTAPP_BLE_GATTC_DESCR_FIND;

#endif

typedef struct
{
    BT_HDR              hdr;
    BD_NAME             remote_bd_name;
}tBTAPP_READ_REMOTE_NAME;

/* type for all configuartion parameters for the BTAPP. */
typedef struct
{
     UINT8                  stack_trace_enable;
     char                   cfg_dev_name[BTAPP_DEV_NAME_LENGTH+1];     /* local bluetooth name */
     /* Mask for services supported by application */
     tBTA_SERVICE_MASK      supported_services;

     /* event handler for BTAPP device manager */
//     tBTAPP_EVENT_HDLR       *p_dm_event_hdlr;
     tBTA_DM_INQ_FILT       dm_inq_filt_type;
     tBTA_DM_INQ_COND       dm_inq_filt_cond;     /* Filter condition data. */
     UINT16                 di_version;
     UINT16                 vendor_id;
     UINT16                 vendor_id_source;
     UINT16                 product_id;

     UINT8                 sp_loc_io_caps;
     tBTA_AUTH_REQ         sp_auth_req;
     BOOLEAN               sp_auto_reply;

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
     /* HS configuartion parameters */
     tBTA_SEC                 hs_security;
     char                     hshs_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     char                     hfhs_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     int                      hs_features;
     BOOLEAN                  hs_sco_over_hci;
     BOOLEAN                  hs_slc_auto_answer;
     BOOLEAN                  hs_outgoing_clcc;

     BOOLEAN                  sco_use_mic;
     BOOLEAN                  sco_file_dump;
#endif

#if( defined BTA_DG_INCLUDED ) && (BTA_DG_INCLUDED == TRUE)
     /* DG configuartion parameters */
     char                     dg_port_name[BTAPP_DEV_NAME_LENGTH + 1];
     char                     dg_port_baud[BTAPP_DEV_NAME_LENGTH + 1];
     tBTA_SEC                sppdg_security;
     char                     sppdg_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     tBTA_SEC                dundg_security;
     char                     dundg_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     tBTA_SEC                faxdg_security;
     char                     faxdg_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     BOOLEAN                 spp_loopback_mode;
     BOOLEAN                 spp_senddata_mode;

     /* DG client configuartion parameters */
     char                     dg_client_port_name[BTAPP_DEV_NAME_LENGTH + 1];
     char                     dg_client_port_baud[BTAPP_DEV_NAME_LENGTH + 1];
     char                     dg_client_service_id[2];
     tBTA_SEC                 dg_client_security;
     char                     dg_client_peer_name[BTAPP_DEV_NAME_LENGTH + 1];
#endif

#if( defined BTA_HD_INCLUDED ) && (BTA_HD_INCLUDED == TRUE)
    tBTA_SEC                  hd_security;
    char                      hd_service_name[BTAPP_DEV_NAME_LENGTH + 1];
#endif

#if( defined BTA_AVK_INCLUDED ) && (BTA_AVK_INCLUDED == TRUE)
    /* AVK (Audio-Video Sink) configuration parameters */
    BOOLEAN             avk_included;
    BOOLEAN             avk_vdp_support;
    tBTA_SEC            avk_security;
    tBTA_AVK_FEAT       avk_features;
    UINT8               avk_channel_mode;
    char                avk_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    BOOLEAN             avk_sbc_decoder;
    BOOLEAN             avk_file_dump;
    BOOLEAN             avk_use_btc;
    UINT8               avk_btc_i2s_rate;
#endif

#if( defined BTA_PAN_INCLUDED ) && (BTA_PAN_INCLUDED == TRUE)
    /* PAN configuration parameters */
    BOOLEAN             pan_included;
    BOOLEAN             panu_supported;
    BOOLEAN             pangn_supported;
    BOOLEAN             pannap_supported;
    tBTA_SEC            pan_security;
    char                panu_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                pangn_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                pannap_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                pan_port_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                pan_port_baud[BTAPP_DEV_NAME_LENGTH + 1];
#endif

#if ( defined BTA_OP_INCLUDED) && (BTA_OP_INCLUDED == TRUE)
     /* OPS configuration parameters */
     BOOLEAN                 ops_included;
     //tBTUI_INIT_FN         * p_ops_init;
     tBTA_SEC                ops_security;
     char                     ops_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     BOOLEAN                ops_srm;

     /* OPC configuration parameters */
     BOOLEAN                 opc_included;
     //tBTUI_INIT_FN         * p_opc_init;
     //tBTUI_INIT_FN         * p_opc_menu;
     //tBTUI_EVENT_HDLR     * p_opc_event_hdlr;
     tBTA_SEC                opc_security;
     BOOLEAN                opc_single_op;
     char                     opc_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     BOOLEAN                opc_srm;
#endif
    /* number of devices to be found during inquiry */
    UINT32              num_inq_devices;

    /* reconfig baud rate */
    UINT8               reconfig_baudrate;

    /* Patchram baud rate */
    UINT8               patchram_baudrate;

    /* patch ram */
    BOOLEAN             patchram_enable;
    UINT32              patchram_address;
    UINT8               set_local_addr[BD_ADDR_LEN];

    /* HCI settings */
    UINT8               hci_port_type;

#if LPM_INCLUDED == TRUE
    /* sleep mode */
    UINT8               lpm_sleep_mode; /* set it by HCISU config */
    UINT8               lpm_host_stack_idle_threshold;
    UINT8               lpm_host_controller_idle_threshold;
    UINT8               lpm_bt_wake_polarity;
    UINT8               lpm_host_wake_polarity;
    UINT8               lpm_allow_host_sleep_during_sco;
    UINT8               lpm_combine_sleep_mode_and_lpm;
    UINT8               lpm_enable_uart_txd_tri_state;
    UINT8               lpm_pulsed_host_wake;
    UINT8               lpm_sleep_guard_time;
    UINT8               lpm_wakeup_guard_time;
    UINT8               lpm_txd_config;
    UINT16              lpm_bt_wake_idle_timeout;
#endif

#if L2CAP_CORRUPT_ERTM_PKTS == TRUE
    UINT16              l2c_corrupt_test_count;
#endif

    /* bte stack run-time configurable items */
    UINT16              l2c_hi_pri_chan_quota;  /* acl buffs for hi-pri l2c channel (0=use bte default) */
    /* Stack configurable items */

    UINT16              sys_features;

#if BLE_INCLUDED == TRUE
    BOOLEAN             ble_stack_spt;
    BOOLEAN             ble_included;
    /* GATT configuration parameters */
//    tBTAPP_EVENT_HDLR    * p_gattc_event_hdlr;
///    tBTAPP_EVENT_HDLR    * p_gatts_event_hdlr;

    BOOLEAN             ble_profile_included;
    BOOLEAN             privacy_enabled;

#if SMP_INCLUDED == TRUE
    UINT8               ble_loc_io_caps;
    UINT8               ble_init_key;
    UINT8               ble_resp_key;
    UINT8               ble_max_key_size;
    UINT8               ble_min_key_size;
    tBTA_LE_AUTH_REQ    ble_auth_req;
    UINT8               ble_accept_auth_enable;
#if SMP_LE_SC_INCLUDED == TRUE
    BOOLEAN             le_sc_auto_reply;
#endif

#endif
    UINT16              ble_scan_int;
    UINT16              ble_scan_win;
    UINT16              ble_scan_type;

#endif /* BLE_INCLUDED */


#if( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
    /* AV configuration parameters */
    BOOLEAN             av_included;
    //tBTUI_EVENT_HDLR    *p_av_event_hdlr;
    //tBTUI_INIT_FN       *p_av_init;
    //tBTUI_INIT_FN       *p_av_menu;
    tBTA_SEC            av_security;
    BOOLEAN             use_avrc;
    tBTA_AV_FEAT        av_features;
    UINT16              av_line_speed_kbps;
    UINT16              av_line_speed_busy;
    UINT16              av_line_speed_swampd;
    UINT16              av_line_speed_count;
    UINT8               av_sampling_freq;
    UINT8               av_channel_mode;
    UINT8               av_num_blocks;
    UINT8               av_num_subbands;
    UINT8               av_allocation_mode;
    UINT8               av_max_bitpool;
    UINT8               av_mp3_ch_mode;
    UINT8               av_mp3_samp_freq;
    UINT16              av_mp3_bitrate;
    BOOLEAN             av_repeat_list;
    char                av_audio_file_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_md_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_file_name2[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_md_name2[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_file_name3[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_md_name3[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_audio_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    char                av_video_service_name[BTAPP_DEV_NAME_LENGTH + 1];
    tBTA_SERVICE_MASK   av_services_used;                      /* Mask of services to use when printing */
    BOOLEAN             av_vdp_support;
    UINT8               av_num_audio;
    BOOLEAN             av_use_player;
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE )
    BOOLEAN             av_auto_switch;
#endif
    UINT8               av_min_trace;
    BOOLEAN             av_suspd_rcfg;
    BOOLEAN             av_reconn_rej; /* TRUE to reconnect to rejected connection */
#endif

#if (defined BTA_AG_INCLUDED) && (BTA_AG_INCLUDED == TRUE)
     /* AG configuartion parameters */
     tBTA_SEC               ag_security;
     int                    ag_features;
     char                   hsag_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     char                   hfag_service_name[BTAPP_DEV_NAME_LENGTH + 1];
     int                    ag_instances;
#if (BTM_WBS_INCLUDED == TRUE)
     int                    ag_sco_codec;
#endif
#endif

    UINT8               bt_controller_without_lpo;

}tBTAPP_CFG;

extern tBTAPP_CFG btapp_cfg;

extern UINT8 btapp_user_set_log;

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
typedef struct
{
    UINT8               spk_vol;
    UINT8               mic_vol;
    BD_ADDR             las_cnt_bda;
}tBTAPP_HS_DB;
extern tBTAPP_HS_DB btapp_hs_db;
#endif

#if (defined BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE)
typedef struct
{
    BD_ADDR             last_cnt_src;
} tBTAPP_AVK_DB;
extern tBTAPP_AVK_DB btapp_avk_db;
#endif

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
/* Local Device BLE keys: IR, ER */
typedef struct
{
    tBTA_DM_BLE_LOCAL_KEY_MASK      keys_mask;
    tBTA_BLE_LOCAL_ID_KEYS          id_keys;
    BT_OCTET16                      er;
} tBTAPP_BLE_INFO;

extern tBTAPP_BLE_INFO   btapp_ble_local_key;

#endif

/* Inquiry results database */
typedef struct
{
     tBTAPP_REM_DEVICE     remote_device[BTAPP_NUM_INQ_DEVICE];
     UINT8                 rem_index;
} tBTAPP_INQ_DB;

extern tBTAPP_INQ_DB btapp_inq_db;

/* BTAPP main control block */
typedef struct
{
     BD_ADDR    local_bd_addr;                          /* local bdaddr */
     char    bd_addr_str[BD_ADDR_LEN*3];                /* local bdaddr */

     tBTA_SERVICE_MASK ui_current_active_connection;    /* active connection mask */
     tBTAPP_REM_DEVICE * p_selected_rem_device;         /* pointer to device selected by UI */

     BD_ADDR peer_bdaddr;                               /* peer bdaddr stored for pin_reply etc*/
     BT_OCTET16 sp_c;
     BT_OCTET16 sp_r;
#if (BTM_BR_SC_INCLUDED == TRUE)
     BT_OCTET16 sp_c_192;
     BT_OCTET16 sp_r_192;
     BT_OCTET16 sp_c_256;
     BT_OCTET16 sp_r_256;
#endif
     BD_ADDR    oob_bdaddr;                             /* peer bdaddr stored for SP OOB process etc*/
     UINT32     pass_key;
     UINT8      notif_index;
     UINT8      sp_bond_bits;
     tBTA_IO_CAP    sp_io_cap;
     BD_ADDR    sp_bond_bdaddr;                           /* peer bdaddr stored for SP IO process etc*/
     BOOLEAN    is_dd_bond;
     BOOLEAN    bond_transport;                           /* bond transport */
     char     peer_name[BTAPP_DEV_NAME_LENGTH+1];         /* bluetooth name of peer device for pin reply etc */

     tBTA_SERVICE_ID peer_service;                        /* service for authorization */
     UINT8 num_devices;                                   /* num_devices in db */
     UINT8 num_audio_devices;                             /* num audio devices in db */
     UINT8 num_op_devices;                                /* num op devices in db */

     tBTAPP_CONN_DEVICE     conn_dev[BTAPP_MAX_CONN_DEVICE];
     UINT8 num_conn_devices;                                    /* num connected devices in db */
     UINT8 sel_conn_devices;                                    /* selected connected devices in db */

     tBTA_SERVICE_MASK search_services;                    /* services to search for */
     UINT8             search_num_uuid;

     UINT32  current_bonding_handle;
     BOOLEAN is_bonding;

     TIMER_LIST_Q  timer_queue;                              /* timer queue */

     tBTA_DISCOVERY_DB  *p_di_db;
     UINT32             di_handle;

     BOOLEAN        sco_hci;
     BOOLEAN        dm_test_mode_enabled;

#define INVALID_NUM_DI_RECORD   0xFFFF
     UINT16             num_di_record;

     /* Default peer bda */
     BD_ADDR peer_bda[BTAPP_MAX_DEFAULT_PEERS];

} tBTAPP_CB;

extern tBTAPP_CB btapp_cb;

#endif
