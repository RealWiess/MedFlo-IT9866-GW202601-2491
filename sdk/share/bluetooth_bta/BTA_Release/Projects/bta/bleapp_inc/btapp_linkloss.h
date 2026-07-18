/*****************************************************************************
**                                                                            
**  Name:             btapp_linkloss.h                                              
**                                                                            
**  Description:     This file contains btapp interface               
**				     definition                                         
**                                                                            
**  Copyright (c) 2000-2010, Broadcom Corp., All Rights Reserved.               
**  Widcomm Bluetooth Core. Proprietary and confidential.                     
******************************************************************************/

#ifndef BTAPP_LINKLOSS_H
#define BTAPP_LINKLOSS_H


#include "bta_gatt_api.h"

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

    #define BTUI_LINKLOSS_APP_UUID      UUID_SERVCLASS_LINKLOSS

    #define BTUI_LINKLOSS_MAX_CHAR_NUM           1
    #define BTUI_LINKLOSS_HANDLE_NUM            ((2 * BTUI_LINKLOSS_MAX_CHAR_NUM) + 1)

    #define BTUI_ALERTLEVEL_PERM            (BTA_GATT_PERM_READ|BTA_GATT_PERM_WRITE)
    #define BTUI_ALERTLEVEL_PROP            (BTA_GATT_CHAR_PROP_BIT_READ |BTA_GATT_CHAR_PROP_BIT_WRITE)

enum
{
    BTUI_ALERT_LEVEL_NONE,
    BTUI_ALERT_LEVEL_MILD,
    BTUI_ALERT_LEVEL_HIGH
};
typedef UINT8 tBTUI_ALERT_LEVEL;
    #define BTUI_LL_ALERT_VALUE_DEF         BTUI_ALERT_LEVEL_NONE

    #define BTUI_LINKLOSS_INST_MAX          1

    #define BTUI_LINKLOSS_PREP_MAX          4

typedef struct
{
    UINT16              service_id;
    UINT16              attr_id;    /* only one chara ID */
    tBTUI_ALERT_LEVEL   alert_value;
}tBTUI_LINKLOSS_INTS;

typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
}tBTUI_LINKLOSS_CLCB;

#define BTUI_LINKLOSS_MAX_CL GATT_CL_MAX_LCB

typedef struct
{
    BOOLEAN                 enabled;
    tBTA_GATTS_IF           server_if;
    UINT8                   inst_id;
    tBTUI_LINKLOSS_INTS     srvc_inst[BTUI_LINKLOSS_INST_MAX];    /* service instance */
    tBTUI_ALERT_LEVEL       prep_q[BTUI_LINKLOSS_PREP_MAX];
    UINT8                   prep_num;
    tBTUI_LINKLOSS_CLCB     clcb[BTUI_LINKLOSS_MAX_CL];
}tBTUI_LINKLOSS_CB;


extern tBTUI_LINKLOSS_CB btui_linkloss_cb;

extern void btapp_linkloss_init(void);
extern void btapp_linkloss_instatiate(void);
extern void btapp_linkloss_disable(void);

#endif
#endif


