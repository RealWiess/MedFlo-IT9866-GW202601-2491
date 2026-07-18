/*****************************************************************************
**
**  Name:           bta_sys.h
**
**  Description:    This is the public interface file for the BTA system
**                  manager.
**
**  Copyright (c) 2003-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_SYS_H
#define BTA_SYS_H

#include "bt_target.h"
#include "gki.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
enum
{
    BTA_VS_CREATE_SCO_EVT = 1,
    BTA_VS_SET_TBFC_MODE_EVT,
    BTA_VS_TBFC_SUSPEND_EVT,
    BTA_VS_TBFC_ENB_CHECK_EVT,
    BTA_VS_BLE_FEAT_ENABLE_EVT,
    BTA_VS_BLE_RSSI_CONN_EVT,
    BTA_VS_BLE_RSSI_ADV_EVT,
    BTA_VS_BLE_ADV_PF_ENABLE_EVT,
    BTA_VS_BLE_ADV_PF_COND_EVT,
    BTA_VS_BLE_MULTI_ADV_ENB_EVT,
    BTA_VS_BLE_MULTI_ADV_PARAM_EVT,
    BTA_VS_BLE_MULTI_ADV_DATA_EVT,
    BTA_VS_BLE_MULTI_ADV_DISABLE_EVT
};

typedef struct
{
    BOOLEAN         enable;
    tBLE_BD_ADDR    *p_target;
    void            *p_cmpl_cback;
}tBTA_SYS_VS_BLE_ADV_PF_ENABLE;

typedef struct
{
    UINT8       action;
    INT8        cond_type;
    void        *p_cond;
    void        *p_cmpl_cback;
}tBTA_SYS_VS_BLE_ADV_PF_COND;

typedef struct
{
    BD_ADDR     bd_addr;
    UINT8       alert_mask;
    INT8        low_threshold;
    INT8        range;
    INT8        hi_threshold;
    void        *p_cback;
}tBTA_SYS_VS_BLE_RSSI_MONITOR;

typedef struct
{
    UINT8       feature;
    BOOLEAN     enable;
}tBTA_SYS_VS_BLE_FEAT_ENABLE;

typedef struct
{
    BD_ADDR     bd_addr;
    UINT8       mode;
}tBTA_SYS_VS_TBFC_SCAN_MODE;

typedef struct
{
    BD_ADDR     bd_addr;
    UINT8       pm_id;
    void        *p_peer_device;
}tBTA_SYS_VS_TBFC_SUSPEND;

typedef struct
{
    BD_ADDR     bd_addr;
    UINT8       pm_id;
    UINT8       app_id;
}tBTA_SYS_VS_TBFC_CHECK;

typedef struct
{
    void        *p_param;
    void        *p_ref;
    void        *p_cback;
}tBTA_SYS_VS_BLE_MULTI_ADV_ENB;

typedef struct
{
    UINT8       inst_id;
    void        *p_param;
}tBTA_SYS_VS_BLE_MULTI_ADV_PARAM;

typedef struct
{
    UINT8       inst_id;
}tBTA_SYS_VS_BLE_MULTI_ADV_DISABLE;

typedef struct
{
    UINT8                   inst_id;
    BOOLEAN                 is_scan_rsp;
    UINT32                  data_mask;
    void                    *p_data;
}tBTA_SYS_VS_BLE_MULTI_ADV_DATA;

/* vendor specific event handler function type */
typedef BOOLEAN (tBTA_SYS_VS_EVT_HDLR)(UINT16 evt, void *p);

/* event handler function type */
typedef BOOLEAN (tBTA_SYS_EVT_HDLR)(BT_HDR *p_msg);

/* disable function type */
typedef void (tBTA_SYS_DISABLE)(void);


/* HW modules */
enum
{
    BTA_SYS_HW_BLUETOOTH,
    BTA_SYS_HW_FMRX,
    BTA_SYS_HW_FMTX,
    BTA_SYS_HW_GPS,
    BTA_SYS_HW_SENSOR,
    BTA_SYS_HW_NFC,
    BTA_SYS_HW_RT,

    BTA_SYS_MAX_HW_MODULES
};

typedef UINT16 tBTA_SYS_HW_MODULE;

#ifndef BTA_DM_NUM_JV_ID
#define BTA_DM_NUM_JV_ID    2
#endif
/* SW sub-systems */
#define BTA_ID_SYS          0            /* system manager */
/* BLUETOOTH PART - from 0 to BTA_ID_BLUETOOTH_MAX */
#define BTA_ID_DM           1            /* device manager */
#define BTA_ID_DM_SEARCH    2            /* device manager search */
#define BTA_ID_DM_SEC       3            /* device manager security */
#define BTA_ID_DG           4            /* data gateway */
#define BTA_ID_AG           5            /* audio gateway */
#define BTA_ID_OPC          6            /* object push client */
#define BTA_ID_OPS          7            /* object push server */
#define BTA_ID_FTS          8            /* file transfer server */
#define BTA_ID_FTC          9            /* file transfer client */
#define BTA_ID_SS           10           /* synchronization server */
#define BTA_ID_PR           11           /* Printer client */
#define BTA_ID_BIC          12           /* Basic Imaging Client */
#define BTA_ID_PAN          13           /* Personal Area Networking */
#define BTA_ID_BIS          14           /* Basic Imaging Server */
#define BTA_ID_ACC          15           /* Advanced Camera Client */
#define BTA_ID_SC           16           /* SIM Card Access server */
#define BTA_ID_AV           17           /* Advanced audio/video */
#define BTA_ID_AVK          18           /* Audio/video sink */
#define BTA_ID_HD           19           /* HID Device */
#define BTA_ID_BP           20           /* Basic Printing Client */
#define BTA_ID_HH           21           /* Human Interface Device Host */
#define BTA_ID_PBS          22           /* Phone Book Access Server */
#define BTA_ID_PBC          23           /* Phone Book Access Client */
#define BTA_ID_JV           24           /* Java */
#define BTA_ID_HS           25           /* Headset */
#define BTA_ID_MSE          26           /* Message Server Equipment */
#define BTA_ID_MCE          27           /* Message Client Equipment */
#define BTA_ID_HL           28           /* Health Device Profile*/
#define BTA_ID_GATTC        29           /* GATT Client */
#define BTA_ID_GATTS        30           /* GATT Server */
#define BTA_ID_3DS          31           /* 3DS Client */
#define BTA_ID_BAV          32           /* Broadcast AV */
#define BTA_ID_RC           33           /* Stand-alone AVRC */
#define BTA_ID_LECOC        34           /* LE connection oriented channel */
#define BTA_ID_SAC          35           /* SIM Access Client */
#define BTA_ID_GCE          36           /* Generic PIM Profile Client */
#define BTA_ID_GSE          37           /* Generic PIM Profile Server */
#define BTA_ID_BLUETOOTH_MAX   38        /* last BT profile */


/* FM */
#define BTA_ID_FM           38           /* FM  */
#define BTA_ID_FMTX         39           /* FM TX */

/* SENSOR */
#define BTA_ID_SSR          40           /* Sensor  */

/* GPS */
#define BTA_ID_GPS          41           /* GPS  */

/* GENERIC */
#define BTA_ID_PRM          42
#define BTA_ID_SYSTEM       43           /* platform-specific */
#define BTA_ID_SWRAP        44           /* Insight script wrapper */
#define BTA_ID_BUSAPP       45           /* Broadcom Utility Service App */
#define BTA_ID_MIP          46           /* Multicase Individual Polling */
#define BTA_ID_RT           47           /* Audio Routing module: Module is always on. */

/* JV */
#define BTA_ID_JV1          48           /* JV1 */
#define BTA_ID_JV2          49           /* JV2 */

#define BTA_ID_MAX          (48 + BTA_DM_NUM_JV_ID)

typedef UINT8 tBTA_SYS_ID;

/* This should be aligned with BTA_DM_PM_NUM_EVTS (except RETRY) */
#define BTA_SYS_CONN_OPEN           0x00
#define BTA_SYS_CONN_CLOSE          0x01
#define BTA_SYS_APP_OPEN            0x02
#define BTA_SYS_APP_CLOSE           0x03
#define BTA_SYS_SCO_OPEN            0x04
#define BTA_SYS_SCO_CLOSE           0x05
#define BTA_SYS_CONN_IDLE           0x06
#define BTA_SYS_CONN_BUSY           0x07
#define BTA_SYS_CONN_TBFC           0x08
/* #define BTA_SYS_CONN_RETRY       0x09 not used (placeholder ) */

/* for link policy */
#define BTA_SYS_PLCY_SET            0x10 /* set the link policy to the given addr */
#define BTA_SYS_PLCY_CLR            0x11 /* clear the link policy to the given addr */
#define BTA_SYS_PLCY_DEF_SET        0x12 /* set the default link policy */
#define BTA_SYS_PLCY_DEF_CLR        0x13 /* clear the default link policy */
#define BTA_SYS_ROLE_CHANGE         0x14 /* role change */

typedef UINT8 tBTA_SYS_CONN_STATUS;

/* Bitmask of sys features */
#define BTA_SYS_FEAT_PCM2           0x0001
#define BTA_SYS_FEAT_PCM2_MASTER    0x0002

/* tBTA_PREF_ROLES */
typedef UINT8 tBTA_SYS_PREF_ROLES;

/* conn callback for role / low power manager*/
typedef void (tBTA_SYS_CONN_CBACK)(tBTA_SYS_CONN_STATUS status,UINT8 id, UINT8 app_id,
                                   BD_ADDR peer_addr);

/* conn callback for role / low power manager*/
typedef void (tBTA_SYS_SSR_CFG_CBACK)(UINT8 id, UINT8 app_id, UINT16 latency,
                                      UINT16 tout);

#if ( BTM_EIR_SERVER_INCLUDED == TRUE )&&(BTA_EIR_CANNED_UUID_LIST != TRUE)
/* eir callback for adding/removeing UUID */
typedef void (tBTA_SYS_EIR_CBACK)(UINT16 uuid16, BOOLEAN adding);
#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
#include "btm_api.h"
/* data type for bta subsystems sync result */
typedef struct
{
    tBTM_STATUS     status;
    tBTM_SYNC_INFO  sync_info;
}tBTA_SYS_SYNC_RESULT;

typedef void (tBTA_SYS_IPC_EVT_CBACK)(tBTM_STATUS status, BT_HDR *p_data);
typedef BOOLEAN (tBTA_SYS_NOTIFY_CBACK)(void);
#define BTA_SYNC_OPCODE_SYNC_REQ        0
#define BTA_SYNC_OPCODE_BUSY_LEVEL_REQ  1
#define BTA_SYNC_OPCODE_CODEC_CFG_IND   2   /* Codec Config done for AVK */
typedef void (tBTA_SYS_SYNC_REQ_CBACK)(UINT8 opcode, UINT8 param);

typedef void (tBTA_SYS_SYNC_RESULT_CBACK)(UINT8 subsys_id, tBTA_SYS_SYNC_RESULT *p_data);
typedef void (tBTA_SYS_IPC_REG_CBACK)(UINT8 subsys_id, tBTA_SYS_IPC_EVT_CBACK *p_cback);
typedef void (tBTA_SYS_NOTIFY_REG_CBACK)(UINT8 subsys_id,tBTA_SYS_NOTIFY_CBACK *p_cback);

#define NOT_SWITCHED        0x00
#define SWITCHED_TO_MM      0x01
#define SWITCHED_TO_BTC     0x02
typedef UINT8 tBTA_SYS_SWITCH_STATUS;

typedef tBTA_SYS_SWITCH_STATUS (tBTA_SYS_SWITCH_STATUS_CBACK)(void);
typedef void (tBTA_SYS_SYNC_REG_CBACK)(UINT8 subsys_id,
                                       tBTA_SYS_SYNC_REQ_CBACK *p_cback);
#endif

/* registration structure */
typedef struct
{
    tBTA_SYS_EVT_HDLR   *evt_hdlr;
    tBTA_SYS_DISABLE    *disable;
} tBTA_SYS_REG;

/* system manager configuration structure */
typedef struct
{
    UINT16          mbox_evt;                       /* GKI mailbox event */
    UINT8           mbox;                           /* GKI mailbox id */
#if (BTU_STACK_LITE_ENABLED == FALSE)
    UINT8           timer;                          /* GKI timer id */
#endif
    UINT8           trace_level;                    /* initial trace level */
} tBTA_SYS_CFG;

/* data type to send events to BTA SYS HW manager */
typedef struct
{
    BT_HDR                hdr;
    tBTA_SYS_HW_MODULE   hw_module;
} tBTA_SYS_HW_MSG;



/*****************************************************************************
**  Global data
*****************************************************************************/

/* trace level */
extern UINT8 appl_trace_level;

/*****************************************************************************
**  Macros
*****************************************************************************/

/* Calculate start of event enumeration; id is top 8 bits of event */
#define BTA_SYS_EVT_START(id)       ((id) << 8)

/*****************************************************************************
**  events for BTA SYS HW manager
*****************************************************************************/

/* events sent to SYS HW manager - must be kept sync'd with tables in bta_sys_main.c */
enum
{
    /* device manager local device API events */
    BTA_SYS_API_ENABLE_EVT = BTA_SYS_EVT_START(BTA_ID_SYS),
    BTA_SYS_EVT_ENABLED_EVT,
    BTA_SYS_EVT_STACK_ENABLED_EVT,
    BTA_SYS_API_DISABLE_EVT,
    BTA_SYS_EVT_DISABLED_EVT,
    BTA_SYS_ERROR_EVT,

    BTA_SYS_MAX_EVT
};



/* SYS HW status events - returned by SYS HW manager to other modules. */
enum
{
    BTA_SYS_HW_OFF_EVT,
    BTA_SYS_HW_ON_EVT,
    BTA_SYS_HW_STARTING_EVT,
    BTA_SYS_HW_STOPPING_EVT,
    BTA_SYS_HW_ERROR_EVT

};
typedef UINT8 tBTA_SYS_HW_EVT;

/* HW enable callback type */
typedef void (tBTA_SYS_HW_CBACK)(tBTA_SYS_HW_EVT status);

/*****************************************************************************
**  Function declarations
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

BTA_API extern void bta_sys_init(void);
BTA_API extern void bta_sys_event(BT_HDR *p_msg);
BTA_API extern void bta_sys_timer_update(void);
BTA_API extern void bta_sys_disable_timers(void);
BTA_API extern void bta_sys_set_trace_level(UINT8 level);
extern void bta_sys_register(UINT8 id, const tBTA_SYS_REG *p_reg);
extern void bta_sys_deregister(UINT8 id);
extern BOOLEAN bta_sys_is_register(UINT8 id);
extern UINT16 bta_sys_get_sys_features(void);
extern void bta_sys_sendmsg(void *p_msg);
extern void bta_sys_start_timer(TIMER_LIST_ENT *p_tle, UINT16 type, INT32 timeout);
extern UINT32 bta_sys_get_remaining_ticks(TIMER_LIST_ENT *p_tle);

extern void bta_sys_stop_timer(TIMER_LIST_ENT *p_tle);
extern void bta_sys_disable(tBTA_SYS_HW_MODULE module);

extern void bta_sys_hw_register( tBTA_SYS_HW_MODULE module, tBTA_SYS_HW_CBACK *cback);
extern void bta_sys_hw_unregister( tBTA_SYS_HW_MODULE module );


extern void bta_sys_rm_register(tBTA_SYS_CONN_CBACK * p_cback);
extern void bta_sys_pm_register(tBTA_SYS_CONN_CBACK * p_cback);

extern void bta_sys_policy_register(tBTA_SYS_CONN_CBACK * p_cback);
extern void bta_sys_sco_register(tBTA_SYS_CONN_CBACK * p_cback);

extern void bta_sys_conn_open(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_conn_close(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_app_open(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_app_close(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_sco_open(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_sco_close(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_sco_use(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_sco_unuse(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_idle(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);
extern void bta_sys_busy(UINT8 id, UINT8 app_id, BD_ADDR peer_addr);

extern void bta_sys_role_chg_register(tBTA_SYS_CONN_CBACK * p_cback);
extern void bta_sys_notify_role_chg(BD_ADDR_PTR p_bda, UINT8 new_role, UINT8 hci_status);
extern void bta_sys_collision_register(UINT8 bta_id, tBTA_SYS_CONN_CBACK *p_cback);
extern void bta_sys_notify_collision (BD_ADDR_PTR p_bda);

#if ( BTM_EIR_SERVER_INCLUDED == TRUE )&&(BTA_EIR_CANNED_UUID_LIST != TRUE)
extern void bta_sys_eir_register(tBTA_SYS_EIR_CBACK * p_cback);
extern void bta_sys_add_uuid(UINT16 uuid16);
extern void bta_sys_remove_uuid(UINT16 uuid16);
#else
#define bta_sys_eir_register(ut)
#define bta_sys_add_uuid(ut)
#define bta_sys_remove_uuid(ut)
#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
extern void bta_sys_sync_result_register(tBTA_SYS_SYNC_RESULT_CBACK *p_cback);
extern void bta_sys_notify_cback_register(tBTA_SYS_NOTIFY_REG_CBACK *p_cback);
extern void bta_sys_ipc_cback_register(tBTA_SYS_IPC_REG_CBACK *p_cback);
extern void bta_sys_switch_status_register(tBTA_SYS_SWITCH_STATUS_CBACK *p_cback);
extern void bta_sys_sync_cback_register(tBTA_SYS_SYNC_REG_CBACK *p_cback);

extern void bta_sys_sync_result(UINT8 subsys_id, tBTA_SYS_SYNC_RESULT *p_data);
extern void bta_sys_notify_register(UINT8 subsys_id, tBTA_SYS_NOTIFY_CBACK *p_cback);
extern void bta_sys_ipc_register(UINT8 subsys_id, tBTA_SYS_IPC_EVT_CBACK *p_cback);
extern tBTA_SYS_SWITCH_STATUS bta_sys_is_lite_active(void);
extern void bta_sys_sync_register(UINT8 subsys_id, tBTA_SYS_SYNC_REQ_CBACK *p_cback);
#endif

extern void bta_sys_set_policy (UINT8 id, UINT8 policy, BD_ADDR peer_addr);
extern void bta_sys_clear_policy (UINT8 id, UINT8 policy, BD_ADDR peer_addr);
extern void bta_sys_set_default_policy (UINT8 id, UINT8 policy);
extern void bta_sys_clear_default_policy (UINT8 id, UINT8 policy);

extern BOOLEAN bta_sys_vs_hdl(UINT16 evt, void *p);
#ifdef __cplusplus
}
#endif

#endif /* BTA_SYS_H */
