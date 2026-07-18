/*****************************************************************************
**
**  Name:             btapp_gattc_profile.h
**
**  Description:     This file contains btapp gattc profile interface
**				     definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_GATTC_PROFILE_H
#define BTAPP_GATTC_PROFILE_H

#include "bta_gatt_api.h"
#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)
#include "btapp_linkloss.h"
#include "btapp_immediate_alert.h"
#include "btapp_tx_power.h"
#endif

/* GATTC CLCB data structure */
typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;

    UINT32  service_mask;

    UINT8   search_idx;
    tBTAPP_REM_DEVICE    *p_selected_rem_device;

#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)
    //it could add the dedicated profile gatt clcb
    //#if( defined BTA_PROXIMITY_INCLUDED ) && (BTA_PROXIMITY_INCLUDED == TRUE)
        /* data needed for proximity monitor */
        tBTA_GATT_SRVC_ID           ll_srvc_id; /* link loss */
        tBTA_GATT_ID                ll_alert_level_id;
        tBTA_GATT_CHAR_PROP         ll_al_prop;
        tBTUI_ALERT_LEVEL           ll_alert_level_value;

        tBTA_GATT_SRVC_ID           ia_srvc_id; /* immediate alert service */
        tBTA_GATT_ID                ia_alert_level_id;
        tBTA_GATT_CHAR_PROP         ia_al_prop;
        tBTUI_IA_ALERT_LEVEL        ia_alert_level_value;

        tBTA_GATT_SRVC_ID           tx_power_id;    /* tx power service */
        tBTA_GATT_ID                tx_pwr_level_id;
        tBTA_GATT_CHAR_PROP         tx_pwr_prop;
        tBTA_GATT_CLT_CHAR_CONFIG   tx_pwr_cfg;
        INT8                        tx_pwr_level_value;
    //#endif
    #if( defined BTAPP_GLUCOSE_INCLUDED ) && (BTAPP_GLUCOSE_INCLUDED == TRUE)
        tBTUI_GLUCOSE_CLCB      glucose_clcb;
    #endif
    #if( defined BTAPP_RSC_INCLUDED ) && (BTAPP_RSC_INCLUDED == TRUE)
        tBTUI_RSC_CLCB          rsc_clcb;
    #endif
    #if( defined BTAPP_CSC_INCLUDED ) && (BTAPP_CSC_INCLUDED == TRUE)
        tBTUI_CSC_CLCB          csc_clcb;
    #endif
    #if( defined BTAPP_CP_INCLUDED ) && (BTAPP_CP_INCLUDED == TRUE)
        tBTUI_CP_CLCB           cp_clcb;    /* Cycling Power */
    #endif
#endif

}tBTAPP_GATTC_CLCB;

#define BTAPP_GATTC_MAX_CL            GATT_CL_MAX_LCB

typedef struct
{
    BOOLEAN                 enabled;
    tBTA_GATTC_IF           client_if;
    tBTAPP_GATTC_CLCB        clcb[BTAPP_GATTC_MAX_CL];
    UINT8                   sel_item_data;
    UINT8                   profiles;
    BD_ADDR                 connecting_bda;

}tBTAPP_GATTC_PROFILE_CB;

typedef void (tBTAPP_GATTC_PROFILE_CONN_ACT)(void);

extern tBTAPP_GATTC_PROFILE_CB   btapp_gattc_profile_cb;

extern void btapp_gattc_profile_init(void);
extern void btapp_gattc_profile_disable(void);
extern void btapp_gattc_profile_connect(BD_ADDR remote_bda, BOOLEAN is_direct);
extern void btapp_gattc_profile_disconnect(BD_ADDR remote_bda);
extern tBTAPP_GATTC_CLCB * btapp_gattc_profile_find_clcb_by_bda(BD_ADDR remote_bda);
extern tBTAPP_GATTC_CLCB * btapp_gattc_profile_find_clcb_by_conn_id(UINT16 conn_id);
extern void btapp_register_4_notification(BD_ADDR  bda, tBTA_GATTC_CHAR_ID *p_char_id, BOOLEAN is_register);
extern void btapp_gattc_send_confirm(UINT16 conn_id, tBTA_GATTC_CHAR_ID *p_char_id);
extern void btapp_gattc_profile_remove_bg_dev(BD_ADDR remote_bda);

#endif
