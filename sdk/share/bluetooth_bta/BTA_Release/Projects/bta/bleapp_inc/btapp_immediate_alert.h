/*****************************************************************************
**                                                                            
**  Name:             btapp_immediate_alert.h                                              
**                                                                            
**  Description:     This file contains btapp interface               
**				     definition                                         
**                                                                            
**  Copyright (c) 2000-2010, Broadcom Corp., All Rights Reserved.               
**  Widcomm Bluetooth Core. Proprietary and confidential.                     
******************************************************************************/

#ifndef BTAPP_IMMEDIATE_ALERT_H
#define BTAPP_IMMEDIATE_ALERT_H


#include "bta_gatt_api.h"

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

#define BTUI_IMMEDIATE_ALERT_APP_UUID      UUID_SERVCLASS_IMMEDIATE_ALERT

#define BTUI_IMMEDIATE_ALERT_MAX_CHAR_NUM           1
#define BTUI_IMMEDIATE_ALERT_HANDLE_NUM            ((2 * BTUI_IMMEDIATE_ALERT_MAX_CHAR_NUM) + 1)

#define BTUI_IA_ALERTLEVEL_PERM            (BTA_GATT_PERM_WRITE)
#define BTUI_IA_ALERTLEVEL_PROP            (BTA_GATT_CHAR_PROP_BIT_WRITE_NR)


enum
{
    BTUI_IA_ALERT_LEVEL_NONE,
    BTUI_IA_ALERT_LEVEL_MILD,
    BTUI_IA_ALERT_LEVEL_HIGH
};
typedef UINT8 tBTUI_IA_ALERT_LEVEL;
#define BTUI_IA_ALERT_VALUE_DEF         BTUI_IA_ALERT_LEVEL_NONE

typedef struct
{
    UINT16              service_id;
    UINT16              attr_id;    /* only one chara ID */
    tBTUI_IA_ALERT_LEVEL   alert_value;
}tBTUI_IMMEDIATE_ALERT_INTS;

typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
}tBTUI_IMMEDIATE_ALERT_CLCB;

#define BTUI_IMMEDIATE_ALERT_MAX_CL GATT_CL_MAX_LCB
typedef struct
{
    BOOLEAN                         enabled;
    tBTA_GATTS_IF                   server_if;
    UINT8                           profile_count;
    tBTUI_IMMEDIATE_ALERT_INTS     srvc_inst;    /* service instance */
    tBTUI_IMMEDIATE_ALERT_CLCB     clcb[BTUI_IMMEDIATE_ALERT_MAX_CL];
}tBTUI_IMMEDIATE_ALERT_CB;


extern tBTUI_IMMEDIATE_ALERT_CB btui_immediate_alert_cb;

extern void btapp_immediate_alert_init(void);
extern void btapp_immediate_alert_instatiate(void);
extern void btapp_immediate_alert_disable(void);

extern void btapp_immediate_alert_write_alert_level(BD_ADDR remote_bda, tBTUI_IA_ALERT_LEVEL alert_level);

#endif
#endif


