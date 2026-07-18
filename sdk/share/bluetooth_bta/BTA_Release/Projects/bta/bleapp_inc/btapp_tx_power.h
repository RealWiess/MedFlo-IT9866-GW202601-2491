/*****************************************************************************
**                                                                            
**  Name:             btapp_tx_power.h                                              
**                                                                            
**  Description:     This file contains btapp interface               
**				     definition                                         
**                                                                            
**  Copyright (c) 2000-2010, Broadcom Corp., All Rights Reserved.               
**  Widcomm Bluetooth Core. Proprietary and confidential.                     
******************************************************************************/

#ifndef BTAPP_TX_POWER_H
#define BTAPP_TX_POWER_H


#include "bta_gatt_api.h"

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

#define BTUI_TX_PWR_INST_INVALID    0xff

#define BTUI_TX_POWER_APP_UUID      UUID_SERVCLASS_TX_POWER 

#ifndef BTUI_TX_POWER_INST_MAX
#define BTUI_TX_POWER_INST_MAX          2
#endif

#ifndef BTUI_TX_MAX_CLIENT_NUM
#define BTUI_TX_MAX_CLIENT_NUM          4
#endif

#define BTUI_TX_POWER_MAX_CHAR_NUM           1
#define BTUI_TX_POWER_HANDLE_NUM            ((4 * BTUI_TX_POWER_MAX_CHAR_NUM) + 1)

#define BTUI_TX_PWR_LEVEL_PERM                (BTA_GATT_PERM_READ)
#define BTUI_TX_PWR_LEVEL_PROP_DEF            (BTA_GATT_CHAR_PROP_BIT_READ)

#define BTUI_TX_PWR_VALUE_DEF       4

typedef struct
{
    BOOLEAN                     in_use;
    BD_ADDR                     client_bda;
    tBTA_GATT_CLT_CHAR_CONFIG   config;
}tBTUI_CONFIG_DATA;

typedef struct
{
    tBTUI_CONFIG_DATA    client_cfg[BTUI_TX_MAX_CLIENT_NUM];  
    UINT16               service_id;
    UINT16               tx_pwr_id;    /* only one chara ID */
    UINT16               pre_fmt_id;
    UINT16               config_id;     /* client configuration ID */
    UINT8                tx_pwr_level;
    UINT32               trans_id;
    UINT16               conn_id;
}tBTUI_TX_POWER_INTS;

typedef void (tBTUI_TX_PWR_CREATE_CBACK) (UINT8 inst_id, tBTA_GATT_STATUS status);

typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
}tBTUI_TX_POWER_CLCB;

#define BTUI_TX_POWER_MAX_CL GATT_CL_MAX_LCB

typedef struct
{
    BOOLEAN                 enabled;
    tBTA_GATTS_IF           server_if;
    UINT8                   inst_id;
    tBTA_GATT_CHAR_PRES     pre_fmt;        /* temp value for presentation format */
    BOOLEAN                 notify_spt;
    UINT8                   descr_step;
    tBTUI_TX_POWER_INTS     srvc_inst[BTUI_TX_POWER_INST_MAX];    /* service instance */
    tBTUI_TX_PWR_CREATE_CBACK   *p_create_cback;
    UINT8                   pending_tx_read_inst;
    tBTUI_TX_POWER_CLCB     clcb[BTUI_TX_POWER_MAX_CL];

}tBTUI_TX_POWER_CB;


extern tBTUI_TX_POWER_CB btui_tx_power_cb;

extern void btapp_tx_power_init(void);
extern void btapp_tx_power_instatiate(tBTA_GATT_CHAR_PRES *p_pre_fmt, BOOLEAN notify_spt,BOOLEAN is_pri, tBTUI_TX_PWR_CREATE_CBACK *p_create_cback);
extern void btapp_tx_power_disable(UINT8 inst_id);

#endif
#endif


